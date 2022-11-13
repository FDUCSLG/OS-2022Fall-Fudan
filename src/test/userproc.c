#include <test/test.h>
#include <common/rc.h>
#include <common/string.h>
#include <kernel/pt.h>
#include <kernel/mem.h>
#include <kernel/printk.h>
#include <common/sem.h>
#include <kernel/sched.h>
#include <kernel/proc.h>
#include <kernel/syscall.h>

PTEntriesPtr get_pte(struct pgdir* pgdir, u64 va, bool alloc);

void vm_test() {
    printk("vm_test\n");
    static void* p[100000];
    extern RefCount alloc_page_cnt;
    struct pgdir pg;
    int p0 = alloc_page_cnt.count;
    init_pgdir(&pg);
    for (u64 i = 0; i < 100000; i++)
    {
        p[i] = kalloc_page();
        *get_pte(&pg, i << 12, true) = K2P(p[i]) | PTE_USER_DATA;
        *(int*)p[i] = i;
    }
    attach_pgdir(&pg);
    for (u64 i = 0; i < 100000; i++)
    {
        ASSERT(*(int*)(P2K(PTE_ADDRESS(*get_pte(&pg, i << 12, false)))) == (int)i);
        ASSERT(*(int*)(i << 12) == (int)i);
    }
    free_pgdir(&pg);
    attach_pgdir(&pg);
    for (u64 i = 0; i < 100000; i++)
        kfree_page(p[i]);
    ASSERT(alloc_page_cnt.count == p0);
    printk("vm_test PASS\n");
}

void trap_return();

static bool stop;
static u64 proc_cnt[22], cpu_cnt[4];
static int pids[22], localpids[22];
static Semaphore myrepot_done, container_done;
extern char loop_start[], loop_end[];

define_syscall(myreport, u64 id)
{
    ASSERT(id < 22);
    ASSERT(thisproc()->localpid == localpids[id]);
    ASSERT(thisproc()->pid == pids[id]);
    if (stop)
        return 0;
    if (proc_cnt[id] == 0)
        printk("proc %llu: pid=%d localpid=%d\n", id, thisproc()->pid, thisproc()->localpid);
    proc_cnt[id]++;
    cpu_cnt[cpuid()]++;
    if (proc_cnt[id] > 21000)
    {
        stop = true;
        post_sem(&myrepot_done);
    }
    return 0;
}

static void _create_user_proc(int i)
{
    auto p = create_proc();
    for (u64 q = (u64)loop_start; q < (u64)loop_end; q += PAGE_SIZE)
    {
        *get_pte(&p->pgdir, 0x400000 + q - (u64)loop_start, true) = K2P(q) | PTE_USER_DATA;
    }
    ASSERT(p->pgdir.pt);
    p->ucontext->x0 = i;
    p->ucontext->elr = 0x400000;
    p->ucontext->ttbr0 = K2P(p->pgdir.pt);
    p->ucontext->spsr = 0;
    pids[i] = p->pid;
    set_parent_to_this(p);
    set_container_to_this(p);
    localpids[i] = start_proc(p, trap_return, 0);
}

static int _wait_user_proc()
{
    int code, id = -1, pid, lpid;
    lpid = wait(&code, &pid);
    ASSERT(lpid != -1);
    for (int j = 0; j < 22; j++)
    {
        if (pids[j] == pid)
        {
            id = j;
            ASSERT(localpids[id] == lpid);
        }
    }
    ASSERT(id != -1);
    ASSERT(code == -1);
    printk("proc %d killed\n", id);
    return id;
}

void user_proc_test()
{
    printk("user_proc_test\n");
    init_sem(&myrepot_done, 0);
    memset(proc_cnt, 0, sizeof(proc_cnt));
    memset(cpu_cnt, 0, sizeof(cpu_cnt));
    stop = false;
    for (int i = 0; i < 22; i++)
        _create_user_proc(i);
    ASSERT(wait_sem(&myrepot_done));
    printk("done\n");
    for (int i = 0; i < 22; i++)
        ASSERT(kill(pids[i]) == 0);
    for (int i = 0; i < 22; i++)
        _wait_user_proc();
    printk("user_proc_test PASS\nRuntime:\n");
    for (int i = 0; i < 4; i++)
        printk("CPU %d: %llu\n", i, cpu_cnt[i]);
    for (int i = 0; i < 22; i++)
        printk("Proc %d: %llu\n", i, proc_cnt[i]);
}

static void container_root(int a)
{
    printk("Container %d\n", a);
    if (a == 0)
    {
        create_container(container_root, 2);
        create_container(container_root, 3);
    }
    for (int i = a * 4; i < a * 4 + 4; i++)
        _create_user_proc(i);
    for (int i = 0; i < 4; i++)
        ASSERT(_wait_user_proc() / 4 == a);
    post_sem(&container_done);
    setup_checker(0);
    lock_for_sched(0);
    sched(0, DEEPSLEEPING);
    // root process doesn't exit
}

void container_test()
{
    printk("container_test\n");
    init_sem(&myrepot_done, 0);
    init_sem(&container_done, 0);
    memset(proc_cnt, 0, sizeof(proc_cnt));
    memset(cpu_cnt, 0, sizeof(cpu_cnt));
    stop = false;
    extern char loop_start[], loop_end[];
    create_container(container_root, 0);
    create_container(container_root, 1);
    for (int i = 16; i < 22; i++)
        _create_user_proc(i);
    ASSERT(wait_sem(&myrepot_done));
    printk("done\n");
    for (int i = 0; i < 22; i++)
        ASSERT(kill(pids[i]) == 0);
    for (int i = 16; i < 22; i++)
        ASSERT(_wait_user_proc() >= 16);
    for (int i = 0; i < 4; i++)
        ASSERT(wait_sem(&container_done));
    printk("container_test PASS\nRuntime:\n");
    for (int i = 0; i < 4; i++)
        printk("CPU %d: %llu\n", i, cpu_cnt[i]);
    for (int i = 0; i < 22; i++)
        printk("Proc %d: %llu\n", i, proc_cnt[i]);
}

