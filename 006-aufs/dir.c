// SPDX-License-Identifier: GPL-2.0
/*
 *	fs/bfs/dir.c
 *	BFS directory operations.
 *	Copyright (C) 1999-2018  Tigran Aivazian <aivazian.tigran@gmail.com>
 *  Made endianness-clean by Andrew Stribblehill <ads@wompom.org> 2005
 */

#include <linux/time.h>
#include <linux/string.h>
#include <linux/fs.h>
#include <linux/buffer_head.h>
#include <linux/sched.h>
#include "aufs.h"

#undef DEBUG

#ifdef DEBUG
#define dprintf(x...)	printf(x)
#else
#define dprintf(x...)
#endif

static int aufs_add_entry(struct inode *dir, const struct qstr *child, int ino);
static struct buffer_head *aufs_find_entry(struct inode *dir,
				const struct qstr *child,
				struct aufs_dirent **res_dir);

static int aufs_readdir(struct file *f, struct dir_context *ctx)
{
	struct inode *dir = file_inode(f);
	struct buffer_head *bh;
	struct aufs_dirent *de;
	unsigned int offset;
	int block;

	if (ctx->pos & (aufs_DIRENT_SIZE - 1)) {
		printf("Bad f_pos=%08lx for %s:%08lx\n",
					(unsigned long)ctx->pos,
					dir->i_sb->s_id, dir->i_ino);
		return -EINVAL;
	}

	while (ctx->pos < dir->i_size) {
		offset = ctx->pos & (aufs_BSIZE - 1);
		block = aufs_I(dir)->i_sblock + (ctx->pos >> aufs_BSIZE_BITS);
		bh = sb_bread(dir->i_sb, block);
		if (!bh) {
			ctx->pos += aufs_BSIZE - offset;
			continue;
		}
		do {
			de = (struct aufs_dirent *)(bh->b_data + offset);
			if (de->ino) {
				int size = strnlen(de->name, aufs_NAMELEN);
				if (!dir_emit(ctx, de->name, size,
						le16_to_cpu(de->ino),
						DT_UNKNOWN)) {
					brelse(bh);
					return 0;
				}
			}
			offset += aufs_DIRENT_SIZE;
			ctx->pos += aufs_DIRENT_SIZE;
		} while ((offset < aufs_BSIZE) && (ctx->pos < dir->i_size));
		brelse(bh);
	}
	return 0;
}

const struct file_operations aufs_dir_operations = {
	.read		= generic_read_dir,
	.iterate_shared	= aufs_readdir,
	.fsync		= generic_file_fsync,
	.llseek		= generic_file_llseek,
};

static int aufs_create(struct user_namespace *mnt_userns, struct inode *dir,
		      struct dentry *dentry, umode_t mode, bool excl)
{
	int err;
	struct inode *inode;
	struct super_block *s = dir->i_sb;
	struct aufs_sb_info *info = aufs_SB(s);
	unsigned long ino;

	inode = new_inode(s);
	if (!inode)
		return -ENOMEM;
	mutex_lock(&info->aufs_lock);
	ino = find_first_zero_bit(info->si_imap, info->si_lasti + 1);
	if (ino > info->si_lasti) {
		mutex_unlock(&info->aufs_lock);
		iput(inode);
		return -ENOSPC;
	}
	set_bit(ino, info->si_imap);
	info->si_freei--;
	inode_init_owner(&init_user_ns, inode, dir, mode);
	inode->i_mtime = inode->i_atime = inode->i_ctime = current_time(inode);
	inode->i_blocks = 0;
	inode->i_op = &aufs_file_inops;
	inode->i_fop = &aufs_file_operations;
	inode->i_mapping->a_ops = &aufs_aops;
	inode->i_ino = ino;
	aufs_I(inode)->i_dsk_ino = ino;
	aufs_I(inode)->i_sblock = 0;
	aufs_I(inode)->i_eblock = 0;
	insert_inode_hash(inode);
        mark_inode_dirty(inode);
	aufs_dump_imap("create", s);

	err = aufs_add_entry(dir, &dentry->d_name, inode->i_ino);
	if (err) {
		inode_dec_link_count(inode);
		mutex_unlock(&info->aufs_lock);
		iput(inode);
		return err;
	}
	mutex_unlock(&info->aufs_lock);
	d_instantiate(dentry, inode);
	return 0;
}

static struct dentry *aufs_lookup(struct inode *dir, struct dentry *dentry,
						unsigned int flags)
{
	struct inode *inode = NULL;
	struct buffer_head *bh;
	struct aufs_dirent *de;
	struct aufs_sb_info *info = aufs_SB(dir->i_sb);

	if (dentry->d_name.len > aufs_NAMELEN)
		return ERR_PTR(-ENAMETOOLONG);

	mutex_lock(&info->aufs_lock);
	bh = aufs_find_entry(dir, &dentry->d_name, &de);
	if (bh) {
		unsigned long ino = (unsigned long)le16_to_cpu(de->ino);
		brelse(bh);
		inode = aufs_iget(dir->i_sb, ino);
	}
	mutex_unlock(&info->aufs_lock);
	return d_splice_alias(inode, dentry);
}

static int aufs_link(struct dentry *old, struct inode *dir,
						struct dentry *new)
{
	struct inode *inode = d_inode(old);
	struct aufs_sb_info *info = aufs_SB(inode->i_sb);
	int err;

	mutex_lock(&info->aufs_lock);
	err = aufs_add_entry(dir, &new->d_name, inode->i_ino);
	if (err) {
		mutex_unlock(&info->aufs_lock);
		return err;
	}
	inc_nlink(inode);
	inode->i_ctime = current_time(inode);
	mark_inode_dirty(inode);
	ihold(inode);
	d_instantiate(new, inode);
	mutex_unlock(&info->aufs_lock);
	return 0;
}

static int aufs_unlink(struct inode *dir, struct dentry *dentry)
{
	int error = -ENOENT;
	struct inode *inode = d_inode(dentry);
	struct buffer_head *bh;
	struct aufs_dirent *de;
	struct aufs_sb_info *info = aufs_SB(inode->i_sb);

	mutex_lock(&info->aufs_lock);
	bh = aufs_find_entry(dir, &dentry->d_name, &de);
	if (!bh || (le16_to_cpu(de->ino) != inode->i_ino))
		goto out_brelse;

	if (!inode->i_nlink) {
		printf("unlinking non-existent file %s:%lu (nlink=%d)\n",
					inode->i_sb->s_id, inode->i_ino,
					inode->i_nlink);
		set_nlink(inode, 1);
	}
	de->ino = 0;
	mark_buffer_dirty_inode(bh, dir);
	dir->i_ctime = dir->i_mtime = current_time(dir);
	mark_inode_dirty(dir);
	inode->i_ctime = dir->i_ctime;
	inode_dec_link_count(inode);
	error = 0;

out_brelse:
	brelse(bh);
	mutex_unlock(&info->aufs_lock);
	return error;
}

static int aufs_rename(struct user_namespace *mnt_userns, struct inode *old_dir,
		      struct dentry *old_dentry, struct inode *new_dir,
		      struct dentry *new_dentry, unsigned int flags)
{
	struct inode *old_inode, *new_inode;
	struct buffer_head *old_bh, *new_bh;
	struct aufs_dirent *old_de, *new_de;
	struct aufs_sb_info *info;
	int error = -ENOENT;

	if (flags & ~RENAME_NOREPLACE)
		return -EINVAL;

	old_bh = new_bh = NULL;
	old_inode = d_inode(old_dentry);
	if (S_ISDIR(old_inode->i_mode))
		return -EINVAL;

	info = aufs_SB(old_inode->i_sb);

	mutex_lock(&info->aufs_lock);
	old_bh = aufs_find_entry(old_dir, &old_dentry->d_name, &old_de);

	if (!old_bh || (le16_to_cpu(old_de->ino) != old_inode->i_ino))
		goto end_rename;

	error = -EPERM;
	new_inode = d_inode(new_dentry);
	new_bh = aufs_find_entry(new_dir, &new_dentry->d_name, &new_de);

	if (new_bh && !new_inode) {
		brelse(new_bh);
		new_bh = NULL;
	}
	if (!new_bh) {
		error = aufs_add_entry(new_dir, &new_dentry->d_name,
					old_inode->i_ino);
		if (error)
			goto end_rename;
	}
	old_de->ino = 0;
	old_dir->i_ctime = old_dir->i_mtime = current_time(old_dir);
	mark_inode_dirty(old_dir);
	if (new_inode) {
		new_inode->i_ctime = current_time(new_inode);
		inode_dec_link_count(new_inode);
	}
	mark_buffer_dirty_inode(old_bh, old_dir);
	error = 0;

end_rename:
	mutex_unlock(&info->aufs_lock);
	brelse(old_bh);
	brelse(new_bh);
	return error;
}

const struct inode_operations aufs_dir_inops = {
	.create			= aufs_create,
	.lookup			= aufs_lookup,
	.link			= aufs_link,
	.unlink			= aufs_unlink,
	.rename			= aufs_rename,
};

static int aufs_add_entry(struct inode *dir, const struct qstr *child, int ino)
{
	const unsigned char *name = child->name;
	int namelen = child->len;
	struct buffer_head *bh;
	struct aufs_dirent *de;
	int block, sblock, eblock, off, pos;
	int i;

	dprintf("name=%s, namelen=%d\n", name, namelen);

	if (!namelen)
		return -ENOENT;
	if (namelen > aufs_NAMELEN)
		return -ENAMETOOLONG;

	sblock = aufs_I(dir)->i_sblock;
	eblock = aufs_I(dir)->i_eblock;
	for (block = sblock; block <= eblock; block++) {
		bh = sb_bread(dir->i_sb, block);
		if (!bh)
			return -EIO;
		for (off = 0; off < aufs_BSIZE; off += aufs_DIRENT_SIZE) {
			de = (struct aufs_dirent *)(bh->b_data + off);
			if (!de->ino) {
				pos = (block - sblock) * aufs_BSIZE + off;
				if (pos >= dir->i_size) {
					dir->i_size += aufs_DIRENT_SIZE;
					dir->i_ctime = current_time(dir);
				}
				dir->i_mtime = current_time(dir);
				mark_inode_dirty(dir);
				de->ino = cpu_to_le16((u16)ino);
				for (i = 0; i < aufs_NAMELEN; i++)
					de->name[i] =
						(i < namelen) ? name[i] : 0;
				mark_buffer_dirty_inode(bh, dir);
				brelse(bh);
				return 0;
			}
		}
		brelse(bh);
	}
	return -ENOSPC;
}

static inline int aufs_namecmp(int len, const unsigned char *name,
							const char *buffer)
{
	if ((len < aufs_NAMELEN) && buffer[len])
		return 0;
	return !memcmp(name, buffer, len);
}

static struct buffer_head *aufs_find_entry(struct inode *dir,
			const struct qstr *child,
			struct aufs_dirent **res_dir)
{
	unsigned long block = 0, offset = 0;
	struct buffer_head *bh = NULL;
	struct aufs_dirent *de;
	const unsigned char *name = child->name;
	int namelen = child->len;

	*res_dir = NULL;
	if (namelen > aufs_NAMELEN)
		return NULL;

	while (block * aufs_BSIZE + offset < dir->i_size) {
		if (!bh) {
			bh = sb_bread(dir->i_sb, aufs_I(dir)->i_sblock + block);
			if (!bh) {
				block++;
				continue;
			}
		}
		de = (struct aufs_dirent *)(bh->b_data + offset);
		offset += aufs_DIRENT_SIZE;
		if (le16_to_cpu(de->ino) &&
				aufs_namecmp(namelen, name, de->name)) {
			*res_dir = de;
			return bh;
		}
		if (offset < bh->b_size)
			continue;
		brelse(bh);
		bh = NULL;
		offset = 0;
		block++;
	}
	brelse(bh);
	return NULL;
}
