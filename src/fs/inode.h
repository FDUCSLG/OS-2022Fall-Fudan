#pragma once

#include <common/list.h>
#include <common/rc.h>
#include <common/spinlock.h>
#include <fs/cache.h>
#include <fs/defines.h>
#include <sys/stat.h>

#define ROOT_INODE_NO 1

struct InodeTree;

typedef struct {
    // lock protects:
    // 1. metadata of inode
    // 2. file content managed by this inode
    SleepLock lock;

    RefCount rc;
    ListNode node;
    usize inode_no;

    bool valid;        // is `entry` loaded?
    InodeEntry entry;  // real inode data on the disk.
} Inode;

typedef struct InodeTree {
    Inode* root;

    // allocate a new zero-initialized inode on disk.
    // return a non-zero inode number if allocation succeeds. Otherwise `alloc`
    // panics.
    usize (*alloc)(OpContext* ctx, InodeType type);

    // acquire the lock of `inode`.
    void (*lock)(Inode* inode);

    // release the lock of `inode`.
    void (*unlock)(Inode* inode);

    // synchronize inode entry between in-memory and on-disk inodes.
    // if `inode->valid` and `do_write` are true, write `inode->entry` to disk.
    // if `inode->valid` is false, read the content of `inode->entry` from disk
    // and set `inode->valid` to true.
    //
    // NOTE: caller must hold the lock of `inode`.
    void (*sync)(OpContext* ctx, Inode* inode, bool do_write);

    // return a pointer to in-memory inode of `inode_no` and increment its
    // reference count by one.
    // caller should guarantee `inode_no` points to an allocated inode.
    Inode* (*get)(usize inode_no);

    // originally named `itrunc` in xv6, i.e. "truncate".
    //
    // discard all contents of `inode`, reset `inode->entry.num_bytes` to zero.
    //
    // NOTE: caller must hold the lock of `inode`.
    void (*clear)(OpContext* ctx, Inode* inode);

    // originally named `idup` in xv6.
    //
    // increment reference count of `inode` by one and return a pointer to
    // `inode`.
    Inode* (*share)(Inode* inode);

    // decrement reference count of `inode` by one.
    // if reference count drops to zero and there's no file or directory linked
    // to this inode, `put` is in charge of freeing this inode both in memory
    // and on disk.
    //
    // NOTE: caller must NOT hold the lock of `inode`.
    void (*put)(OpContext* ctx, Inode* inode);

    // read `count` bytes from `inode`, beginning at `offset`, to `dest`.
    // return the size you read
    // NOTE: caller must hold the lock of `inode`.
    usize (*read)(Inode* inode, u8* dest, usize offset, usize count);

    // write exactly `count` bytes from `src` to `inode`, beginning at `offset`.
    // return the size you write
    // NOTE: caller must hold the lock of `inode`.
    usize (*write)(OpContext* ctx,
                   Inode* inode,
                   u8* src,
                   usize offset,
                   usize count);

    // for directory inode only.
    //
    // look up `name` in directory `inode`.
    // if directory entry with `name` is found, the corresponding non-zero inode
    // number is returned, and the index of directory entry is copied to
    // `*index`. Otherwise it returns zero.
    //
    // NOTE: caller must hold the lock of `inode`.
    usize (*lookup)(Inode* inode, const char* name, usize* index);

    // for directory inode only.
    // originally named `dirlink` in xv6. The name `dirlink` is a misnomer,
    // since it will not increment target inode's link count.
    //
    // add a new directory entry in `inode` with `name`, which points to another
    // inode with `inode_no`.
    // the index of new directory entry is returned.
    // `insert` does not ensure all directory entries have unique names,
    // if have same names,return -1
    //
    // NOTE: caller must hold the lock of `inode`.
    usize (*insert)(OpContext* ctx,
                    Inode* inode,
                    const char* name,
                    usize inode_no);

    // for directory inode only.
    //
    // remove the directory entry at `index`.
    // if the corresponding entry is not used before, `remove` does nothing.
    //
    // NOTE: caller must hold the lock of `inode`.
    void (*remove)(OpContext* ctx, Inode* inode, usize index);
} InodeTree;

extern InodeTree inodes;

void init_inodes(const SuperBlock* sblock, const BlockCache* cache);
Inode* namei(const char* path, OpContext* ctx);
Inode* nameiparent(const char* path, char* name, OpContext* ctx);
void stati(Inode* ip, struct stat* st);
