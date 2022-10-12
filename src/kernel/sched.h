#pragma once

#include <kernel/proc.h>

void init_schinfo(struct schinfo*);

bool activate_proc(struct proc*);
bool is_zombie(struct proc*);
bool is_unused(struct proc*);
void _acquire_sched_lock();
#define lock_for_sched(checker) (checker_begin_ctx(checker), _acquire_sched_lock())
void _sched(enum procstate new_state);
// MUST call lock_for_sched() before sched() !!!
#define sched(checker, new_state) (checker_end_ctx(checker), _sched(new_state))
#define yield() (_acquire_sched_lock(), _sched(RUNNABLE))

struct proc* thisproc();
