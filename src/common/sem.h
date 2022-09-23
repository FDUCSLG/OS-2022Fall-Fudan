#pragma once

#include <common/list.h>

struct proc;

typedef struct
{
    bool up;
    struct proc* proc;
    ListNode slnode;
} WaitData;

typedef struct
{
    SpinLock lock;
    int val;
    ListNode sleeplist;
} Semaphore;

void init_sem(Semaphore*, int val);
void post_sem(Semaphore*);
bool wait_sem(Semaphore*);
bool get_sem(Semaphore*);
int get_all_sem(Semaphore*);

#define SleepLock Semaphore
#define init_sleeplock(lock) init_sem(lock, 1)
#define acquire_sleeplock(checker, lock) (checker_begin_ctx(checker), wait_sem(lock))
#define release_sleeplock(checker, lock) (checker_end_ctx(checker), post_sem(lock))

