#include <kernel/sched.h>
#include <common/sem.h>
#include <test/test.h>
#include <kernel/mem.h>
#include <kernel/printk.h>

void set_parent_to_this(struct proc* proc);

static Semaphore s1, s2, s3, s4, s5, s6;

// proc_test_1
// 0: wait 10-19
// 1: give living 20-29 to root_proc
// 2: give dead 30-39 to root_proc
// 34567: 40-89 odd P(s2) even V(s2)
// 8: 90-99 V(s3) P(s4) get_all
// 9: 100-109 P(s5) V(s6) post

static void proc_test_1b(u64 a)
{
    switch (a / 10 - 1)
    {
    case 0: break;
    case 1: yield(); yield(); yield(); break;
    case 2: post_sem(&s1); break;
    case 3: case 4: case 5: case 6: case 7:
        if (a & 1)
            post_sem(&s2);
        else
            unalertable_wait_sem(&s2);
        break;
    case 8: unalertable_wait_sem(&s3); post_sem(&s4); break;
    case 9: post_sem(&s5); unalertable_wait_sem(&s6); break;
    }
    exit(a);
}

static void proc_test_1a(u64 a)
{
    for (int i = 0; i < 10; i++)
    {
        auto p = create_proc();
        set_parent_to_this(p);
        start_proc(p, proc_test_1b, a * 10 + i + 10);
    }
    switch (a)
    {
    case 0: {
        int t = 0, x, y;
        for (int i = 0; i < 10; i++)
        {
            ASSERT(wait(&x, &y) != -1);
            t |= 1 << (x - 10);
        }
        ASSERT(t == 1023);
        ASSERT(wait(&x, &y) == -1);
    } break;
    case 1: break; 
    case 2: { 
        for (int i = 0; i < 10; i++)
            unalertable_wait_sem(&s1);
        ASSERT(!get_sem(&s1));
    } break;
    case 3: case 4: case 5: case 6: case 7: {
        int x, y;
        for (int i = 0; i < 10; i++)
            ASSERT(wait(&x, &y) != -1);
        ASSERT(wait(&x, &y) == -1);
    } break;
    case 8: {
        int x, y;
        for (int i = 0; i < 10; i++)
            post_sem(&s3);
        for (int i = 0; i < 10; i++)
            ASSERT(wait(&x, &y) != -1);
        ASSERT(wait(&x, &y) == -1);
        ASSERT(s3.val == 0);
        ASSERT(get_all_sem(&s4) == 10);
    } break;
    case 9: {
        int x, y;
        for (int i = 0; i < 10; i++)
            unalertable_wait_sem(&s5);
        for (int i = 0; i < 10; i++)
            post_sem(&s6);
        for (int i = 0; i < 10; i++)
            ASSERT(wait(&x, &y) != -1);
        ASSERT(wait(&x, &y) == -1);
        ASSERT(s5.val == 0);
        ASSERT(s6.val == 0);
    } break;
    }
    exit(a);
}

static void proc_test_1()
{
    printk("proc_test_1\n");
    init_sem(&s1, 0);
    init_sem(&s2, 0);
    init_sem(&s3, 0);
    init_sem(&s4, 0);
    init_sem(&s5, 0);
    init_sem(&s6, 0);
    int pid[10];
    for (int i = 0; i < 10; i++)
    {
        auto p = create_proc();
        set_parent_to_this(p);
        pid[i] = start_proc(p, proc_test_1a, i);
    }
    for (int i = 0; i < 10; i++)
    {
        int code, id, t;
        id = wait(&code, &t);
        ASSERT(pid[code] == id);
        printk("proc %d exit\n", code);
    }
    exit(0);
}

void proc_test()
{
    printk("proc_test\n");
    auto p = create_proc();
    int pid = start_proc(p, proc_test_1, 0);
    int t = 0;
    while (1)
    {
        int code, a;
        int id = wait(&code, &a);
        if (id == -1)
            break;
        if (id == pid)
            ASSERT(code == 0);
        else
            t |= 1 << (code - 20);
    }
    ASSERT(t == 1048575);
    printk("proc_test PASS\n");
}
