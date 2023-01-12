#pragma once
#include <sys/syscall.h>

#define SYS_myreport 499
#define SYS_pstat 500
#define SYS_sbrk 12

#define SYS_clone 220
#define SYS_myexit 457
#define SYS_myyield 459

#define SYS_exit 93
#define SYS_execve 221

#define SYS_tid_address 96
#define SYS_ioctl 29
#define SYS_sigprocmask 135
#define SYS_wait4 260
#define SYS_exit_group 94
#define SYS_unlinkat 35

// find more in musl/arch/aarch64/bits/syscall.h