/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
/*
 *	include/linux/aufs_fs.h - BFS data structures on disk.
 *	Copyright (C) 1999-2018 Tigran Aivazian <aivazian.tigran@gmail.com>
 */

#ifndef _LINUX_MY_FS_H
#define _LINUX_MY_FS_H

#include <linux/types.h>

#define aufs_BSIZE_BITS		9
#define aufs_BSIZE		(1<<aufs_BSIZE_BITS)

#define aufs_MAGIC		0x1BADFAFF
#define aufs_ROOT_INO		2
#define aufs_INODES_PER_BLOCK	8

/* SVR4 vnode type values (aufs_inode->i_vtype) */
#define aufs_VDIR 2L
#define aufs_VREG 1L

/* BFS inode layout on disk */
struct aufs_inode {
	__le16 i_ino;
	__u16 i_unused;
	__le32 i_sblock;
	__le32 i_eblock;
	__le32 i_eoffset;
	__le32 i_vtype;
	__le32 i_mode;
	__le32 i_uid;
	__le32 i_gid;
	__le32 i_nlink;
	__le32 i_atime;
	__le32 i_mtime;
	__le32 i_ctime;
	__u32 i_padding[4];
};

#define aufs_NAMELEN		14	
#define aufs_DIRENT_SIZE		16
#define aufs_DIRS_PER_BLOCK	32

struct aufs_dirent {
	__le16 ino;
	char name[aufs_NAMELEN];
};

/* BFS superblock layout on disk */
struct aufs_super_block {
	__le32 s_magic;
	__le32 s_start;
	__le32 s_end;
	__le32 s_from;
	__le32 s_to;
	__s32 s_bfrom;
	__s32 s_bto;
	char  s_fsname[6];
	char  s_volume[6];
	__u32 s_padding[118];
};


#define aufs_OFF2INO(offset) \
        ((((offset) - aufs_BSIZE) / sizeof(struct aufs_inode)) + aufs_ROOT_INO)

#define aufs_INO2OFF(ino) \
	((__u32)(((ino) - aufs_ROOT_INO) * sizeof(struct aufs_inode)) + aufs_BSIZE)
#define aufs_NZFILESIZE(ip) \
        ((le32_to_cpu((ip)->i_eoffset) + 1) -  le32_to_cpu((ip)->i_sblock) * aufs_BSIZE)

#define aufs_FILESIZE(ip) \
        ((ip)->i_sblock == 0 ? 0 : aufs_NZFILESIZE(ip))

#define aufs_FILEBLOCKS(ip) \
        ((ip)->i_sblock == 0 ? 0 : (le32_to_cpu((ip)->i_eblock) + 1) -  le32_to_cpu((ip)->i_sblock))
#define aufs_UNCLEAN(aufs_sb, sb)	\
	((le32_to_cpu(aufs_sb->s_from) != -1) && (le32_to_cpu(aufs_sb->s_to) != -1) && !(sb->s_flags & SB_RDONLY))


#endif	/* _LINUX_aufs_FS_H */
