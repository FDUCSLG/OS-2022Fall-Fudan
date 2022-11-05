#include "lock_config.hpp"
#include "map.hpp"
#include "errno.h"

#include <condition_variable>
#include <semaphore.h>
#include <time.h>
#include <cassert>
#include <map>
#include <unistd.h>
namespace {

struct Mutex {
    bool locked;
    std::mutex mutex;

    void lock() {
        mutex.lock();
        locked = true;
    }

    void unlock() {
        locked = false;
        mutex.unlock();
    }
};

struct Signal {
    // use a pointer to avoid `pthread_cond_destroy` blocking process exit.
    std::condition_variable_any* cv;

    Signal() {
        cv = new std::condition_variable_any;
    }
};

Map<void*, Mutex> mtx_map;

thread_local int holding = 0;
static struct Blocker {
    sem_t sem;
    Blocker() {
        sem_init(&sem, 0, 4);
        mtx_map.try_add(&sem);
    }
    void p() {
        if constexpr (MockLockConfig::SpinLockBlocksCPU) {
            struct timespec ts;
            assert(clock_gettime(CLOCK_REALTIME, &ts) == 0);
            ts.tv_sec += MockLockConfig::WaitTimeoutSeconds;
            assert(sem_timedwait(&sem, &ts) == 0);
        }
    }
    void v() {
        if constexpr (MockLockConfig::SpinLockBlocksCPU)
            sem_post(&sem);
    }
} blocker;
}

extern "C" {

void init_spinlock(struct SpinLock* lock, const char* name [[maybe_unused]]) {
    mtx_map.try_add(lock);
}

void _acquire_spinlock(struct SpinLock* lock) {
    if (holding++ == 0)
        blocker.p();
    mtx_map[lock].lock();
}

void _release_spinlock(struct SpinLock* lock) {
    mtx_map[lock].unlock();
    if (--holding == 0)
        blocker.v();
}

bool holding_spinlock(struct SpinLock* lock) {
    return mtx_map[lock].locked;
}

struct Semaphore;
#define sa(x) ((uint64_t*)x)[0]
#define sb(x) ((uint64_t*)x)[1]
void init_sem(Semaphore* x, int val) {
    init_spinlock((SpinLock*)x, "");
    sa(x) = 0;
    sb(x) = val;
}
void _lock_sem(Semaphore* x) {
    _acquire_spinlock((SpinLock*)x);
}
void _unlock_sem(Semaphore* x) {
    _release_spinlock((SpinLock*)x);
}
bool _get_sem(Semaphore* x)
{
    bool ret = false;
    if (sa(x) < sb(x))
    {
        sa(x)++;
        ret = true;
    }
    return ret;
}
int _query_sem(Semaphore* x)
{
    return sb(x) - sa(x);
}
void _post_sem(Semaphore* x) {
    sb(x)++;
}
bool _wait_sem(Semaphore* x, bool alertable [[maybe_unused]]) {
    if constexpr (MockLockConfig::SpinLockForbidsWait)
        assert(holding == 1);
    auto t = sa(x)++;
    int t0 = time(NULL);
    while (1)
    {
        if (time(NULL) - t0 > MockLockConfig::WaitTimeoutSeconds)
        {
            return false;
        }
        if (sb(x) > t)
            break;
        _unlock_sem(x);
        if (holding)
            blocker.v();
        usleep(5);
        if (holding)
            blocker.p();
        _lock_sem(x);
    }
    _unlock_sem(x);
    return true;
}
int get_all_sem(Semaphore* x)
{
    int ret = 0;
    _lock_sem(x);
    if (sa(x) < sb(x))
    {
        ret = sb(x) - sa(x);
        sa(x) = sb(x);
    }
    _unlock_sem(x);
    return ret;
}
int post_all_sem(Semaphore* x)
{
    int ret = 0;
    _lock_sem(x);
    if (sa(x) > sb(x))
    {
        ret = sa(x) - sb(x);
        sb(x) = sa(x);
    }
    _unlock_sem(x);
    return ret;
}
#undef sa
#undef sb

}
