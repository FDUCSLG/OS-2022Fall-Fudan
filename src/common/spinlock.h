#pragma once

#include <common/defines.h>
#include <aarch64/intrinsic.h>
#include <common/checker.h>

typedef struct {
    volatile bool locked;
} SpinLock;

void _init_spinlock(SpinLock*);
bool _acquire_spinlock(SpinLock*);
void _release_spinlock(SpinLock*);


// Init a spinlock. It's optional for static objects.
#define init_spinlock(checker, lock) _init_spinlock(lock)

// Try to acquire a spinlock. Return true on success.
#define try_acquire_spinlock(checker, lock) (checker_begin_ctx(checker), _acquire_spinlock(lock) || (checker_end_ctx(checker), 0))

// Acquire a spinlock. Spin until success.
#define acquire_spinlock(checker, lock) ({checker_begin_ctx(checker); while(!_acquire_spinlock(lock));})

// Release a spinlock
#define release_spinlock(checker, lock) (checker_end_ctx(checker), _release_spinlock(lock))

