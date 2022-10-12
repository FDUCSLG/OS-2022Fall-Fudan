#pragma once

#include <common/defines.h>
#include <aarch64/intrinsic.h>
#include <common/checker.h>

typedef struct {
    volatile bool locked;
} SpinLock;

bool _try_acquire_spinlock(SpinLock*);
void _acquire_spinlock(SpinLock*);
void _release_spinlock(SpinLock*);

// Init a spinlock. It's optional for static objects.
void init_spinlock(SpinLock*);

// Try to acquire a spinlock. Return true on success.
#define try_acquire_spinlock(checker, lock) (_try_acquire_spinlock(lock) && checker_begin_ctx(checker))

// Acquire a spinlock. Spin until success.
#define acquire_spinlock(checker, lock) checker_begin_ctx_before_call(checker, _acquire_spinlock, lock)

// Release a spinlock
#define release_spinlock(checker, lock) checker_end_ctx_after_call(checker, _release_spinlock, lock)

