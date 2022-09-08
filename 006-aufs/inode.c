#include <linux/module.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/buffer_head.h>
#include <linux/vfs.h>
#include <linux/writeback.h>
#include <linux/uio.h>
#include <linux/uaccess.h>
#include "aufs.h"

MODULE_DESCRIPTION("SCO UnixWare AUFS filesystem for Linux");
MODULE_LICENSE("GPL");

#undef DEBUG
#define DEBUG       1

#ifdef DEBUG
#define dprintf(x...)	printf(x)
#else
#define dprintf(x...)
#endif

struct inode *aufs_iget(struct super_block *sb, unsigned long ino)
{
    struct aufs_inode *di;
    struct inode *inode;
    struct buffer_head *bh;
    int block, off;

    inode = iget_locked(sb, ino);
    if (!inode) {
        return ERR_PTR(-ENOMEM);
    }

    if (!(inode->i_state & I_NEW)) {
        return inode;
    }

    if ((ino < aufs_ROOT_INO) || (ino > aufs_SB(inode->i_sb)->si_lasti)) {
        printf("Bad inode number %s:%08lx\n", inode->i_sb->s_id, ino);
        goto error;
    }

    block = (ino - aufs_ROOT_INO) / aufs_INODES_PER_BLOCK + 1;
    bh = sb_bread(inode->i_sb, block);
    if (!bh) {
        printf("Unable to read inode %s:%08lx\n", inode->i_sb->s_id, ino);
        goto error;
    }

    off = (ino - aufs_ROOT_INO) % aufs_INODES_PER_BLOCK;
    di = (struct aufs_inode *)bh->b_data + off;

    inode->i_mode = 0x0000FFFF & le32_to_cpu(di->i_mode);
    if (le32_to_cpu(di->i_vtype) == aufs_VDIR) {
        inode->i_mode |= S_IFDIR;
        inode->i_op = &aufs_dir_inops;
        inode->i_fop = &aufs_dir_operations;
    } else if (le32_to_cpu(di->i_vtype) == aufs_VREG) {
        inode->i_mode |= S_IFREG;
        inode->i_op = &aufs_file_inops;
        inode->i_fop = &aufs_file_operations;
        inode->i_mapping->a_ops = &aufs_aops;
    }

    aufs_I(inode)->i_sblock = le32_to_cpu(di->i_sblock);
    aufs_I(inode)->i_eblock = le32_to_cpu(di->i_eblock);
    aufs_I(inode)->i_dsk_ino = le16_to_cpu(di->i_ino);
    i_uid_write(inode, le32_to_cpu(di->i_uid));
    i_gid_write(inode, le32_to_cpu(di->i_gid));
    set_nlink(inode, le32_to_cpu(di->i_nlink));
    inode->i_size = aufs_FILESIZE(di);
    inode->i_blocks = aufs_FILEBLOCKS(di);
    inode->i_atime.tv_sec = le32_to_cpu(di->i_atime);
    inode->i_mtime.tv_sec = le32_to_cpu(di->i_mtime);
    inode->i_ctime.tv_sec = le32_to_cpu(di->i_ctime);
    inode->i_atime.tv_nsec = 0;
    inode->i_mtime.tv_nsec = 0;
    inode->i_ctime.tv_nsec = 0;

    brelse(bh);
    unlock_new_inode(inode);

    return inode;

error:
    iget_failed(inode);
    return ERR_PTR(-EIO);
}

static struct aufs_inode *find_inode(struct super_block *sb, u16 ino, struct buffer_head **p)
{
    if ((ino < aufs_ROOT_INO) || (ino > aufs_SB(sb)->si_lasti)) {
        printf("Bad inode number %s:%08x\n", sb->s_id, ino);
        return ERR_PTR(-EIO);
    }

    ino -= aufs_ROOT_INO;

    *p = sb_bread(sb, 1 + ino / aufs_INODES_PER_BLOCK);
    if (!*p) {
        printf("Unable to read inode %s:%08x\n", sb->s_id, ino);
        return ERR_PTR(-EIO);
    }

    return (struct aufs_inode *)(*p)->b_data + ino % aufs_INODES_PER_BLOCK;
}

static int aufs_write_inode(struct inode *inode, struct writeback_control *wbc)
{
    struct aufs_sb_info *info = aufs_SB(inode->i_sb);
    unsigned int ino = (u16)inode->i_ino;
    unsigned long i_sblock;
    struct aufs_inode *di;
    struct buffer_head *bh;
    int err = 0;

    dprintf("ino=%08x\n", ino);

    di = find_inode(inode->i_sb, ino, &bh);
    if (IS_ERR(di)) {
        return PTR_ERR(di);
    }

    mutex_lock(&info->aufs_lock);

    if (ino == aufs_ROOT_INO) {
        di->i_vtype = cpu_to_le32(aufs_VDIR);
    } else {
        di->i_vtype = cpu_to_le32(aufs_VREG);
    }

    di->i_ino = cpu_to_le16(ino);
    di->i_mode = cpu_to_le32(inode->i_mode);
    di->i_uid = cpu_to_le32(i_uid_read(inode));
    di->i_gid = cpu_to_le32(i_gid_read(inode));
    di->i_nlink = cpu_to_le32(inode->i_nlink);
    di->i_atime = cpu_to_le32(inode->i_atime.tv_sec);
    di->i_mtime = cpu_to_le32(inode->i_mtime.tv_sec);
    di->i_ctime = cpu_to_le32(inode->i_ctime.tv_sec);
    i_sblock = aufs_I(inode)->i_sblock;
    di->i_sblock = cpu_to_le32(i_sblock);
    di->i_eblock = cpu_to_le32(aufs_I(inode)->i_eblock);
    di->i_eoffset = cpu_to_le32(i_sblock * aufs_BSIZE + inode->i_size - 1);

    mark_buffer_dirty(bh);
    if (wbc->sync_mode == WB_SYNC_ALL) {
        sync_dirty_buffer(bh);
        if (buffer_req(bh) && !buffer_uptodate(bh)) {
            err = -EIO;
        }
    }
    
    brelse(bh);
    mutex_unlock(&info->aufs_lock);

    return err;
}

static void aufs_evict_inode(struct inode *inode)
{
    unsigned long ino = inode->i_ino;
    struct aufs_inode *di;
    struct buffer_head *bh;
    struct super_block *s = inode->i_sb;
    struct aufs_sb_info *info = aufs_SB(s);
    struct aufs_inode_info *bi = aufs_I(inode);

    dprintf("ino=%08lx\n", ino);

    truncate_inode_pages_final(&inode->i_data);
    invalidate_inode_buffers(inode);
    clear_inode(inode);

    if (inode->i_nlink) {
        return;
    }

    di = find_inode(s, inode->i_ino, &bh);
    if (IS_ERR(di)) {
        return;
    }

    mutex_lock(&info->aufs_lock);
    /* clear on-disk inode */
    memset(di, 0, sizeof(struct aufs_inode));
    mark_buffer_dirty(bh);
    brelse(bh);

    if (bi->i_dsk_ino) {
        if (bi->i_sblock) {
            info->si_freeb += bi->i_eblock + 1 - bi->i_sblock;
        }

        info->si_freei++;
        clear_bit(ino, info->si_imap);
        aufs_dump_imap("evict_inode", s);
    }

    if (info->si_lf_eblk == bi->i_eblock) {
        info->si_lf_eblk = bi->i_sblock - 1;
    }
    
    mutex_unlock(&info->aufs_lock);
}

static void aufs_put_super(struct super_block *s)
{
    struct aufs_sb_info *info = aufs_SB(s);

    if (!info) {
        return;
    }

    mutex_destroy(&info->aufs_lock);
    kfree(info);
    s->s_fs_info = NULL;
}

static int aufs_statfs(struct dentry *dentry, struct kstatfs *buf)
{
    struct super_block *s = dentry->d_sb;
    struct aufs_sb_info *info = aufs_SB(s);
    u64 id = huge_encode_dev(s->s_bdev->bd_dev);
    buf->f_type = aufs_MAGIC;
    buf->f_bsize = s->s_blocksize;
    buf->f_blocks = info->si_blocks;
    buf->f_bfree = buf->f_bavail = info->si_freeb;
    buf->f_files = info->si_lasti + 1 - aufs_ROOT_INO;
    buf->f_ffree = info->si_freei;
    buf->f_fsid = u64_to_fsid(id);
    buf->f_namelen = aufs_NAMELEN;

    return 0;
}

static struct kmem_cache *aufs_inode_cachep;

static struct inode *aufs_alloc_inode(struct super_block *sb)
{
    struct aufs_inode_info *bi;
    bi = alloc_inode_sb(sb, aufs_inode_cachep, GFP_KERNEL);
    if (!bi) {
        return NULL;
    }

    return &bi->vfs_inode;
}

static void aufs_free_inode(struct inode *inode)
{
    kmem_cache_free(aufs_inode_cachep, aufs_I(inode));
}

static void init_once(void *foo)
{
    struct aufs_inode_info *bi = foo;

    inode_init_once(&bi->vfs_inode);
}


static const struct super_operations aufs_sops = 
{
    .alloc_inode    = aufs_alloc_inode,
    .free_inode	    = aufs_free_inode,
    .write_inode    = aufs_write_inode,
    .evict_inode    = aufs_evict_inode,
    .put_super      = aufs_put_super,
    .statfs	        = aufs_statfs,
};

void aufs_dump_imap(const char *prefix, struct super_block *s)
{
#ifdef DEBUG
    int i;
    char *tmpbuf = (char *)get_zeroed_page(GFP_KERNEL);

    if (!tmpbuf) {
		return;
    }

    for (i = aufs_SB(s)->si_lasti; i >= 0; i--) {
        if (i > PAGE_SIZE - 100) break;
        if (test_bit(i, aufs_SB(s)->si_imap)) {
            strcat(tmpbuf, "1");
        } else {
			strcat(tmpbuf, "0");
        }
    }

    printf("%s: lasti=%08lx <%s>\n", prefix, aufs_SB(s)->si_lasti, tmpbuf);
    free_page((unsigned long)tmpbuf);
#endif
}

static int aufs_fill_super(struct super_block *s, void *data, int silent)
{
    struct buffer_head *bh, *sbh;
    struct aufs_super_block *aufs_sb;
    struct inode *inode;
    unsigned i;
    struct aufs_sb_info *info;
    int ret = -EINVAL;
    unsigned long i_sblock, i_eblock, i_eoff, s_size;

    info = kzalloc(sizeof(*info), GFP_KERNEL);
    if (!info) {
        return -ENOMEM;
    }

    mutex_init(&info->aufs_lock);
    s->s_fs_info = info;
    s->s_time_min = 0;
    s->s_time_max = U32_MAX;

    sb_set_blocksize(s, aufs_BSIZE);

    sbh = sb_bread(s, 0);
    if (!sbh) {
        goto out;
    }

    aufs_sb = (struct aufs_super_block *)sbh->b_data;
    if (le32_to_cpu(aufs_sb->s_magic) != aufs_MAGIC) {
        if (!silent) {
            printf("No AUFS filesystem on %s (magic=%08x)\n", s->s_id,  le32_to_cpu(aufs_sb->s_magic));
        }

        goto out1;
    }

    if (aufs_UNCLEAN(aufs_sb, s) && !silent) {
        printf("%s is unclean, continuing\n", s->s_id);
    }

    s->s_magic = aufs_MAGIC;

    if (le32_to_cpu(aufs_sb->s_start) > le32_to_cpu(aufs_sb->s_end)
            || le32_to_cpu(aufs_sb->s_start) < sizeof(struct aufs_super_block) + sizeof(struct aufs_dirent)) {
        printf("Superblock is corrupted on %s\n", s->s_id);
        goto out1;
    }

    info->si_lasti = (le32_to_cpu(aufs_sb->s_start) - aufs_BSIZE) / sizeof(struct aufs_inode) + aufs_ROOT_INO - 1;
    if (info->si_lasti == aufs_MAX_LASTI) {
        printf("NOTE: filesystem %s was created with 512 inodes, the real maximum is 511, mounting anyway\n", s->s_id);
    } else if (info->si_lasti > aufs_MAX_LASTI) {
        printf("Impossible last inode number %lu > %d on %s\n", info->si_lasti, aufs_MAX_LASTI, s->s_id);
        goto out1;
    }

    for (i = 0; i < aufs_ROOT_INO; i++) {
        set_bit(i, info->si_imap);
    }

    s->s_op = &aufs_sops;
    inode = aufs_iget(s, aufs_ROOT_INO);
    if (IS_ERR(inode)) {
        ret = PTR_ERR(inode);
        goto out1;
    }

    s->s_root = d_make_root(inode);
    if (!s->s_root) {
        ret = -ENOMEM;
        goto out1;
    }

    info->si_blocks = (le32_to_cpu(aufs_sb->s_end) + 1) >> aufs_BSIZE_BITS;
    info->si_freeb = (le32_to_cpu(aufs_sb->s_end) + 1 - le32_to_cpu(aufs_sb->s_start)) >> aufs_BSIZE_BITS;
    info->si_freei = 0;
    info->si_lf_eblk = 0;

    /* can we read the last block? */
    bh = sb_bread(s, info->si_blocks - 1);
    if (!bh) {
        printf("Last block not available on %s: %lu\n", s->s_id, info->si_blocks - 1);
        ret = -EIO;
        goto out2;
    }
    brelse(bh);

    bh = NULL;
    for (i = aufs_ROOT_INO; i <= info->si_lasti; i++) {
        struct aufs_inode *di;
        int block = (i - aufs_ROOT_INO) / aufs_INODES_PER_BLOCK + 1;
        int off = (i - aufs_ROOT_INO) % aufs_INODES_PER_BLOCK;
        unsigned long eblock;

        if (!off) {
            brelse(bh);
            bh = sb_bread(s, block);
        }

        if (!bh) {
            continue;
        }

        di = (struct aufs_inode *)bh->b_data + off;

        /* test if filesystem is not corrupted */

        i_eoff = le32_to_cpu(di->i_eoffset);
        i_sblock = le32_to_cpu(di->i_sblock);
        i_eblock = le32_to_cpu(di->i_eblock);
        s_size = le32_to_cpu(aufs_sb->s_end);

        if (i_sblock > info->si_blocks 
                || i_eblock > info->si_blocks 
                || i_sblock > i_eblock 
                || (i_eoff != le32_to_cpu(-1) && i_eoff > s_size) 
                || i_sblock * aufs_BSIZE > i_eoff) {

            printf("Inode 0x%08x corrupted on %s\n", i, s->s_id);

            brelse(bh);
            ret = -EIO;
            goto out2;
        }

        if (!di->i_ino) {
            info->si_freei++;
            continue;
        }

        set_bit(i, info->si_imap);
        info->si_freeb -= aufs_FILEBLOCKS(di);

        eblock =  le32_to_cpu(di->i_eblock);
        if (eblock > info->si_lf_eblk) {
            info->si_lf_eblk = eblock;
        }
    }

    brelse(bh);
    brelse(sbh);
    aufs_dump_imap("fill_super", s);

    return 0;

out2:
    dput(s->s_root);
    s->s_root = NULL;

out1:
    brelse(sbh);

out:
    mutex_destroy(&info->aufs_lock);
    kfree(info);
    s->s_fs_info = NULL;

    return ret;
}

static struct file_system_type aufs_fs_type = 
{
    .owner          = THIS_MODULE,
    .name           = "aufs",
    .kill_sb        = kill_block_super,
    //.fs_flags       = FS_REQUIRES_DEV,
};
MODULE_ALIAS_FS("aufs");

static int __init init_aufs_fs(void)
{
    int err = register_filesystem(&aufs_fs_type);
    if (err) {
        goto out;
    }

    // mount
    aufs_mount = kern_mount (&aufs_fs_type);
    if (IS_ERR(aufs_mount)) {
        printk (KERN_ERR "aufs: could not mount!\n");
        unregister_filesystem (&aufs_fs_type);
        goto out;
    }

    return 0;

out:
    return err;
}

static void __exit exit_aufs_fs(void)
{
    unregister_filesystem(&aufs_fs_type);
}

module_init(init_aufs_fs)
module_exit(exit_aufs_fs)
