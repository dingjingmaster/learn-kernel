#ifndef _FS_AUFS_H
#define _FS_AUFS_H

#include "mfs.h"

#define aufs_MAX_LASTI              513

struct aufs_sb_info
{
    unsigned long                   si_blocks;
    unsigned long                   si_freeb;
    unsigned long                   si_freei;
    unsigned long                   si_lf_eblk;
    unsigned long                   si_lasti;
    DECLARE_BITMAP(si_imap, aufs_MAX_LASTI+1);
    struct mutex                    aufs_lock;
};

struct aufs_inode_info
{
    unsigned long                   i_dsk_ino; /* inode number from the disk, can be 0 */
    unsigned long                   i_sblock;
    unsigned long                   i_eblock;
    struct inode                    vfs_inode;
};

static inline struct aufs_sb_info *aufs_SB(struct super_block *sb)
{
    return sb->s_fs_info;
}

static inline struct aufs_inode_info *aufs_I(struct inode *inode)
{
    return container_of(inode, struct aufs_inode_info, vfs_inode);
}


#define printf(format, args...) \
    printk(KERN_ERR "AUFS-fs: %s(): " format, __func__, ## args)

/* inode.c */
extern struct inode *aufs_iget(struct super_block *sb, unsigned long ino);
extern void aufs_dump_imap(const char *, struct super_block *);

/* file.c */
extern const struct inode_operations aufs_file_inops;
extern const struct file_operations aufs_file_operations;
extern const struct address_space_operations aufs_aops;

/* dir.c */
extern const struct inode_operations aufs_dir_inops;
extern const struct file_operations aufs_dir_operations;

#endif /* _FS_aufs_aufs_H */
