#include <kernel/syscallno.h>
#include <kernel/proc.h>
#include <kernel/init.h>

#define NR_SYSCALL 512

extern void* syscall_table[NR_SYSCALL];

void syscall_entry(UserContext* context);

#define define_syscall(name, ...) \
static u64 sys_##name(__VA_ARGS__); \
define_early_init(__syscall_##name) { syscall_table[SYS_##name] = &sys_##name; } \
static u64 sys_##name(__VA_ARGS__)
