#include "common/ipc.h"
#include "test.h"
#include "kernel/printk.h"
#include "kernel/proc.h"
#include "kernel/mem.h"
struct mytype {
    int mtype;
    int sum;
};
static int msg[10001];
static void sender(u64 start) {
    int msgid = sys_msgget(114514, 0);
    ASSERT(msgid >= 0);
    for (int i = start; i < (int)start + 100; i++) {
        struct mytype* k = (struct mytype*)kalloc(sizeof(struct mytype));
        k->mtype = i + 1;
        k->sum = -i + 1;
        ASSERT(sys_msgsnd(msgid, (msgbuf*)k, 4, 0)>=0);
    }
    exit(0);
}
static void receiver(u64 start) {
    int msgid = sys_msgget(114514, 0);
    ASSERT(msgid >= 0);
    for (int i = start; i < (int)start + 1000; i++) {
        struct mytype k;
        ASSERT(sys_msgrcv(msgid, (msgbuf*)&k, 4, 0, 0) >= 0);
        msg[k.mtype] = k.sum;
    }
    exit(0);
}
void ipc_test() {
    printk("ipc_test\n");
    int key = 114514;
    int msgid;
    msgid = sys_msgget(key, IPC_CREATE | IPC_EXCL);
    for (int i = 0; i < 100; i++) {
        struct proc* p = create_proc();
        start_proc(p, sender, i * 100);
    }
    for (int i = 0; i < 10; i++) {
        struct proc* p = create_proc();
        start_proc(p, receiver, i * 1000);
    }
    while (1) {
        int code;
        int id = wait(&code);
        if (id == -1)
            break;
    }
    ASSERT(sys_msgctl(msgid, IPC_RMID) >= 0);
    for (int i = 1; i < 10001; i++)
        ASSERT(msg[i] = -i);
    printk("ipc_test PASS\n");
}