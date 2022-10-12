#pragma once

#include <common/list.h>

struct proc;

typedef struct {
    bool up;
    struct proc* proc;
    ListNode slnode;
} WaitData;

typedef struct {
    SpinLock lock;
    int val;
    ListNode sleeplist;
} Semaphore;
void init_sem(Semaphore*, int val);
int get_all_sem(Semaphore*);
int post_all_sem(Semaphore*);
bool _get_sem(Semaphore*);
int _query_sem(Semaphore*);
void _lock_sem(Semaphore*);
void _unlock_sem(Semaphore*);
bool _wait_sem(Semaphore*);
void _post_sem(Semaphore*);
#define lock_sem(checker, sem) checker_begin_ctx_before_call(checker, _lock_sem, sem)
#define unlock_sem(checker, sem) checker_end_ctx_after_call(checker, _unlock_sem, sem)
#define prelocked_wait_sem(checker, sem) checker_end_ctx_after_call(checker, _wait_sem, sem)
#define delayed_wait_sem(checker, sem) {checker_set_delayed_task(checker, _wait_sem, sem); _lock_sem(sem);}
#define wait_sem(sem) (_lock_sem(sem), _wait_sem(sem))
#define post_sem(sem) (_lock_sem(sem), _post_sem(sem), _unlock_sem(sem))
#define get_sem(sem) ({_lock_sem(sem); bool __ret = _get_sem(sem); _unlock_sem(sem); __ret;})

#define SleepLock Semaphore
#define init_sleeplock(lock) init_sem(lock, 1)
#define acquire_sleeplock(checker, lock) (checker_begin_ctx(checker), wait_sem(lock))
#define release_sleeplock(checker, lock) (post_sem(lock), checker_end_ctx(checker))
