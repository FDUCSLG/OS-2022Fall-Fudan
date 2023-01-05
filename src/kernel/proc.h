#pragma once

#include <common/defines.h>
#include <common/list.h>
#include <common/sem.h>
#include <kernel/schinfo.h>
#include <kernel/pt.h>
#include <kernel/container.h>
#include <fs/inode.h>
#include <fs/file.h>

#define NOFILE 1024 /* open files per process */

enum procstate { UNUSED, RUNNABLE, RUNNING, SLEEPING, DEEPSLEEPING, ZOMBIE };

typedef struct UserContext
{
    // TODO: customize your trap frame

} UserContext;

typedef struct KernelContext
{
    // TODO: customize your context

} KernelContext;

struct proc
{
    bool killed;
    bool idle;
    int pid;
    int localpid;
    int exitcode;
    enum procstate state;
    Semaphore childexit;
    ListNode children;
    ListNode ptnode;
    struct proc* parent;
    struct schinfo schinfo;
    struct pgdir pgdir;
    struct container* container;
    void* kstack;
    UserContext* ucontext;
    KernelContext* kcontext;
    struct oftable oftable;
    Inode* cwd; // current working dictionary
};

// void init_proc(struct proc*);
WARN_RESULT struct proc* create_proc();
void set_parent_to_this(struct proc*);
int start_proc(struct proc*, void(*entry)(u64), u64 arg);
NO_RETURN void exit(int code);
WARN_RESULT int wait(int* exitcode, int* pid);
WARN_RESULT int kill(int pid);
WARN_RESULT int fork();