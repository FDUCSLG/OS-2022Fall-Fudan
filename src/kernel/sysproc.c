#include <kernel/syscall.h>
#include <kernel/sched.h>
#include <kernel/printk.h>
#include <kernel/proc.h>
#include <kernel/mem.h>
#include <kernel/paging.h>

define_syscall(gettid) {
    return thisproc()->localpid;
}

define_syscall(set_tid_address, int* tidptr) {
    (void)tidptr;
    return thisproc()->localpid;
}

define_syscall(sigprocmask) {
    return 0;
}

define_syscall(rt_sigprocmask) {
    return 0;
}

define_syscall(myyield) {
    yield();
    return 0;
}

define_syscall(yield) {
    yield();
    return 0;
}

define_syscall(pstat) {
    return (u64)left_page_cnt();
}

define_syscall(sbrk, i64 size) {
    return sbrk(size);
}

define_syscall(clone, int flag, void* childstk) {
    if (flag != 17) {
        printk("sys_clone: flags other than SIGCHLD are not supported.\n");
        return -1;
    }
    return fork();
}

define_syscall(myexit, int n) {
    exit(n);
}

define_syscall(exit, int n) {
    exit(n);
}

int execve(const char* path, char* const argv[], char* const envp[]);
define_syscall(execve, const char* p, void* argv, void* envp) {
    if (!user_strlen(p, 256))
        return -1;
    return execve(p, argv, envp);
}

define_syscall(sys_wait4, int pid, int options, int* wstatus, void* rusage) {
    if (pid != -1 || wstatus != 0 || options != 0 || rusage != 0) {
        printk("sys_wait4: unimplemented. pid %d, wstatus 0x%p, options 0x%x, rusage 0x%p\n",
               pid,
               wstatus,
               options,
               rusage);
        while (1) {}
        return -1;
    }
    int code, id;
    return wait(&code, &id);
}
