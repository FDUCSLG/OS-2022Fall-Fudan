#pragma once

extern "C" {
#include <fs/inode.h>
}

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <random>
#include <unordered_map>

#include "../exception.hpp"

struct MockBlockCache {
    static constexpr usize num_blocks = 2000;
    static constexpr usize inode_start = 200;
    static constexpr usize block_start = 1000;
    static constexpr usize num_inodes = 1000;

    static auto get_sblock() -> SuperBlock {
        SuperBlock sblock;
        sblock.num_blocks = num_blocks;
        sblock.num_data_blocks = num_blocks - block_start;
        sblock.num_inodes = num_inodes;
        sblock.num_log_blocks = 50;
        sblock.log_start = 2;
        sblock.inode_start = inode_start;
        sblock.bitmap_start = 900;
        return sblock;
    }

    struct Meta {
        bool mark = false;
        std::mutex mutex;
        bool used;

        auto operator=(const Meta &rhs) -> Meta & {
            used = rhs.used;
            return *this;
        }
    };

    struct Cell {
        bool mark = false;
        usize index;
        std::mutex mutex;
        Block block;

        auto operator=(const Cell &rhs) -> Cell & {
            block = rhs.block;
            return *this;
        }

        void zero() {
            for (usize i = 0; i < BLOCK_SIZE; i++) {
                block.data[i] = 0;
            }
        }

        void random(std::mt19937 &gen) {
            for (usize i = 0; i < BLOCK_SIZE; i++) {
                block.data[i] = gen() & 0xff;
            }
        }
    };

    // board: record all uncommitted atomic operations. `board[i] = true` means
    // atomic operation i has called `end_op` and waits for final commit.
    // oracle: to allocate id for each atomic operation.
    // top: the maximum id of committed atomic operation.
    // mutex & cv: protects board.
    std::mutex mutex;
    std::condition_variable cv;
    std::atomic<usize> oracle, top_oracle;
    std::unordered_map<usize, bool> scoreboard;

    // mbit: bitmap cached in memory, which is volatile
    // sbit: bitmap on SD card, which is persistent
    // mblk: data blocks cached in memory, volatile
    // sblk: data blocks on SD card, persistent
    Meta mbit[num_blocks], sbit[num_blocks];
    Cell mblk[num_blocks], sblk[num_blocks];

    MockBlockCache() {
        std::mt19937 gen(0x19260817);

        oracle.store(1);
        top_oracle.store(0);

        // fill disk with junk.
        for (usize i = 0; i < num_blocks; i++) {
            mbit[i].used = false;
            mblk[i].index = i;
            mblk[i].random(gen);
            sbit[i].used = false;
            sblk[i].index = i;
            sblk[i].random(gen);
        }

        // mock superblock.
        auto sblock = get_sblock();
        u8 *buf = reinterpret_cast<u8 *>(&sblock);
        for (usize i = 0; i < sizeof(sblock); i++) {
            sblk[1].block.data[i] = buf[i];
        }

        // mock inodes.
        InodeEntry node[num_inodes];
        for (usize i = 0; i < num_inodes; i++) {
            node[i].type = INODE_INVALID;
            node[i].major = gen() & 0xffff;
            node[i].minor = gen() & 0xffff;
            node[i].num_links = gen() & 0xffff;
            node[i].num_bytes = gen() & 0xffff;
            for (usize j = 0; j < INODE_NUM_DIRECT; j++) {
                node[i].addrs[j] = gen();
            }
            node[i].indirect = gen();
        }

        // mock root inode.
        node[1].type = INODE_DIRECTORY;
        node[1].major = 0;
        node[1].minor = 0;
        node[1].num_links = 1;
        node[1].num_bytes = 0;
        for (usize i = 0; i < INODE_NUM_DIRECT; i++) {
            node[1].addrs[i] = 0;
        }
        node[1].indirect = 0;

        usize step = 0;
        for (usize i = 0, j = inode_start; i < num_inodes; i += step, j++) {
            step = std::min(num_inodes - i, static_cast<usize>(INODE_PER_BLOCK));
            buf = reinterpret_cast<u8 *>(&node[i]);
            for (usize k = 0; k < step * sizeof(InodeEntry); k++) {
                sblk[j].block.data[k] = buf[k];
            }
        }
    }

    // invalidate all cached blocks and fill them with random data.
    void fill_junk() {
        std::mt19937 gen(0xdeadbeef);

        for (usize i = 0; i < num_blocks; i++) {
            std::scoped_lock guard(mbit[i].mutex);
            if (mbit[i].mark)
                throw Internal("marked by others");
        }

        for (usize i = 0; i < num_blocks; i++) {
            std::scoped_lock guard(mblk[i].mutex);
            if (mblk[i].mark)
                throw Internal("marked by others");
            mblk[i].random(gen);
        }
    }

    // count how many inodes on disk are valid.
    auto count_inodes() -> usize {
        std::unique_lock lock(mutex);

        usize step = 0, count = 0;
        for (usize i = 0, j = inode_start; i < num_inodes; i += step, j++) {
            step = std::min(num_inodes - i, static_cast<usize>(INODE_PER_BLOCK));
            auto *inodes = reinterpret_cast<InodeEntry *>(sblk[j].block.data);
            for (usize k = 0; k < step; k++) {
                if (inodes[k].type != INODE_INVALID)
                    count++;
            }
        }

        return count;
    }

    // count how many blocks on disk are allocated.
    auto count_blocks() -> usize {
        std::unique_lock lock(mutex);

        usize count = 0;
        for (usize i = block_start; i < num_blocks; i++) {
            std::scoped_lock guard(sbit[i].mutex);
            if (sbit[i].used)
                count++;
        }

        return count;
    }

    // inspect on disk inode at specified inode number.
    auto inspect(usize i) -> InodeEntry * {
        usize j = inode_start + i / INODE_PER_BLOCK;
        usize k = i % INODE_PER_BLOCK;
        auto *arr = reinterpret_cast<InodeEntry *>(sblk[j].block.data);
        return &arr[k];
    }

    void check_block_no(usize i) {
        if (i >= num_blocks)
            throw AssertionFailure("block number out of range");
    }

    auto check_and_get_cell(Block *b) -> Cell * {
        Cell *p = container_of(b, Cell, block);
        isize offset = reinterpret_cast<u8 *>(p) - reinterpret_cast<u8 *>(mblk);
        if (offset % sizeof(Cell) != 0)
            throw AssertionFailure("pointer not aligned");

        isize i = p - mblk;
        if (i < 0 || static_cast<usize>(i) >= num_blocks)
            throw AssertionFailure("block is not managed by cache");

        return p;
    }

    template <typename T>
    void load(T &a, T &b) {
        if (!a.mark) {
            a = b;
            a.mark = true;
        }
    }

    template <typename T>
    void store(T &a, T &b) {
        if (a.mark) {
            b = a;
            a.mark = false;
        }
    }

    void begin_op(OpContext *ctx) {
        std::unique_lock lock(mutex);
        ctx->ts = oracle.fetch_add(1);
        scoreboard[ctx->ts] = false;
    }

    void end_op(OpContext *ctx) {
        std::unique_lock lock(mutex);
        scoreboard[ctx->ts] = true;

        // is it safe to checkpoint now?
        bool do_checkpoint = true;
        for (const auto &e : scoreboard) {
            do_checkpoint &= e.second;
        }

        if (do_checkpoint) {
            for (usize i = 0; i < num_blocks; i++) {
                std::scoped_lock guard(mbit[i].mutex, sbit[i].mutex);
                store(mbit[i], sbit[i]);
            }

            for (usize i = 0; i < num_blocks; i++) {
                std::scoped_lock guard(mblk[i].mutex, sblk[i].mutex);
                store(mblk[i], sblk[i]);
            }

            usize max_oracle = 0;
            for (const auto &e : scoreboard) {
                max_oracle = std::max(max_oracle, e.first);
            }
            top_oracle.store(max_oracle);
            scoreboard.clear();

            cv.notify_all();
        } else {
            // if there are other running atomic operations, just wait for them.
            cv.wait(lock, [&] { return ctx->ts <= top_oracle.load(); });
        }
    }

    auto alloc(OpContext *ctx) -> usize {
        for (usize i = block_start; i < num_blocks; i++) {
            std::scoped_lock guard(mbit[i].mutex, sbit[i].mutex);
            load(mbit[i], sbit[i]);

            if (!mbit[i].used) {
                mbit[i].used = true;
                if (!ctx)
                    store(mbit[i], sbit[i]);

                std::scoped_lock guard(mblk[i].mutex, sblk[i].mutex);
                load(mblk[i], sblk[i]);
                mblk[i].zero();
                if (!ctx)
                    store(mblk[i], sblk[i]);

                return i;
            }
        }

        throw AssertionFailure("no free block");
    }

    void free(OpContext *ctx, usize i) {
        check_block_no(i);

        std::scoped_lock guard(mbit[i].mutex, sbit[i].mutex);
        load(mbit[i], sbit[i]);
        if (!mbit[i].used)
            throw AssertionFailure("free unused block");

        mbit[i].used = false;
        if (!ctx)
            store(mbit[i], sbit[i]);
    }

    auto acquire(usize i) -> Block * {
        check_block_no(i);

        mblk[i].mutex.lock();

        {
            std::scoped_lock guard(sblk[i].mutex);
            load(mblk[i], sblk[i]);
        }

        return &mblk[i].block;
    }

    void release(Block *b) {
        auto *p = check_and_get_cell(b);
        p->mutex.unlock();
    }

    void sync(OpContext *ctx, Block *b) {
        auto *p = check_and_get_cell(b);
        usize i = p->index;

        if (!ctx) {
            std::scoped_lock guard(sblk[i].mutex);
            store(mblk[i], sblk[i]);
        }
    }
};

namespace {
#include "cache.ipp"
}  // namespace
