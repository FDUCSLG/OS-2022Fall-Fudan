#include <kernel/syscall.h>
#include <kernel/sched.h>
#include <kernel/printk.h>
#include <kernel/proc.h>
#include <kernel/mem.h>
#include <kernel/paging.h>

define_syscall(myyield) {
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