#include <common/spinlock.h>

// this file is compiled with `fs` library.
// some symbols may conflict with those in the standard libc, e.g. `sleep`, so we
// have to replace them with other symbol names and instrument them here.

extern void _fs_test_sleep(void *chan, SpinLock *lock);
extern void _fs_test_wakeup(void *chan);

void sleep(void *chan, SpinLock *lock) {
    _fs_test_sleep(chan, lock);
}

void wakeup(void *chan) {
    _fs_test_wakeup(chan);
}
