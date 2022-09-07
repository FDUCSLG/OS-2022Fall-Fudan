#include <aarch64/intrinsic.h>
#include <common/spinlock.h>

void _init_spinlock(SpinLock *lock) {
    lock->locked = 0;
}

bool _acquire_spinlock(SpinLock *lock) {
    if (!lock->locked && !__atomic_test_and_set(&lock->locked, __ATOMIC_ACQUIRE)) {
        return true;
    } else {
        return false;
    }
}

void _release_spinlock(SpinLock *lock) {
    __atomic_clear(&lock->locked, __ATOMIC_RELEASE);
}
