#pragma once

#include "cache.hpp"

static MockBlockCache mock;
static SuperBlock sblock;
static BlockCache cache;

static void stub_begin_op(OpContext *ctx) {
    mock.begin_op(ctx);
}

static void stub_end_op(OpContext *ctx) {
    mock.end_op(ctx);
}

static usize stub_alloc(OpContext *ctx) {
    return mock.alloc(ctx);
}

static void stub_free(OpContext *ctx, usize block_no) {
    mock.free(ctx, block_no);
}

static Block *stub_acquire(usize block_no) {
    return mock.acquire(block_no);
}

static void stub_release(Block *block) {
    return mock.release(block);
}

static void stub_sync(OpContext *ctx, Block *block) {
    mock.sync(ctx, block);
}

static struct _Loader {
    _Loader() {
        sblock = mock.get_sblock();

        cache.begin_op = stub_begin_op;
        cache.end_op = stub_end_op;
        cache.alloc = stub_alloc;
        cache.free = stub_free;
        cache.acquire = stub_acquire;
        cache.release = stub_release;
        cache.sync = stub_sync;
    }
} _loader;
