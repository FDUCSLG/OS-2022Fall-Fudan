#pragma once
#include <common/list.h>
#include <common/sem.h>
#include <fs/block_device.h>
#include <fs/defines.h>

// maximum number of distinct blocks that one atomic operation can hold.
#define OP_MAX_NUM_BLOCKS 10

// if the number of cached blocks is no less than this threshold, we can
// evict some blocks in `acquire` to keep block cache small.
#define EVICTION_THRESHOLD 20

// hint: `cache_test` only requires `block_no`, `valid` and `data` are present
// in this struct. All other struct members can be customized by yourself.
// for example, if you want to implement LFU strategy instead, you can add a
// counter inside `Block` to maintain the number of times it was accessed.
typedef struct {
    // accesses to the following 4 members should be guarded by the lock
    // of the block cache.
    usize block_no;
    ListNode node;
    bool acquired;   // is the block already acquired by some thread?
    bool pinned;     // if a block is pinned, it should not be evicted from the
                     // cache.
    SleepLock lock;  // this lock protects `valid` and `data`.
    bool valid;      // is the content of block loaded from disk?
    u8 data[BLOCK_SIZE];
} Block;

// `OpContext` represents an atomic operation.
// see `begin_op` and `end_op`.
typedef struct {
    usize rm;
    usize ts;
    // hint: you may want to add something else here.
} OpContext;

typedef struct BlockCache {
    // for testing.
    // get the number of cached blocks.
    // or in other words, the number of allocated `Block` struct.
    usize (*get_num_cached_blocks)();

    // read the content of block at `block_no` from disk, and lock the block.
    // return the pointer to the locked block.
    Block* (*acquire)(usize block_no);

    // unlock `block`.
    // NOTE: it does not need to write the block content back to disk.
    void (*release)(Block* block);

    // NOTES FOR ATOMIC OPERATIONS
    //
    // atomic operation has three states:
    // * running: this atomic operation may have more modifications.
    // * committed: this atomic operation is ended. No more modifications.
    // * checkpointed: all modifications have been already persisted to disk.
    //
    // `begin_op` creates a new running atomic operation.
    // `end_op` commits an atomic operation, and waits for it to be
    // checkpointed.

    // begin a new atomic operation and initialize `ctx`.
    // `OpContext` represents an outstanding atomic operation. You can mark the
    // end of atomic operation by `end_op`.
    void (*begin_op)(OpContext* ctx);

    // synchronize the content of `block` to disk.
    // `ctx` can be NULL, which indicates this operation does not belong to any
    // atomic operation and it immediately writes block content back to disk.
    // However this is very dangerous, since it may break atomicity of
    // concurrent atomic operations. YOU SHOULD USE THIS MODE WITH CARE. if
    // `ctx` is not NULL, the actual writeback is delayed until `end_op`.
    //
    // NOTE: the caller must hold the lock of `block`.
    // NOTE: if the number of blocks associated with `ctx` is larger than
    // `OP_MAX_NUM_BLOCKS` after `sync`, `sync` should panic.
    void (*sync)(OpContext* ctx, Block* block);

    // end the atomic operation managed by `ctx`.
    // it returns when all associated blocks are persisted to disk.
    void (*end_op)(OpContext* ctx);

    // NOTES FOR BITMAP
    //
    // every block on disk has a bit in bitmap, including blocks inside bitmap!
    //
    // usually, MBR block, super block, inode blocks, log blocks and bitmap
    // blocks are preallocated on disk, i.e. those bits for them are already set
    // in bitmap. therefore when we allocate a new block, it usually returns a
    // data block. however, nobody can prevent you freeing a non-data block :)

    // allocate a new zero-initialized block, by searching bitmap for a free
    // block. block number is returned.
    //
    // NOTE: if there's no free block on disk, `alloc` should panic.
    usize (*alloc)(OpContext* ctx);

    // mark block at `block_no` is free in bitmap.
    void (*free)(OpContext* ctx, usize block_no);
} BlockCache;

extern BlockCache bcache;

void init_bcache(const SuperBlock* sblock, const BlockDevice* device);
