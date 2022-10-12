#ifndef __IPC_H
#define __IPC_H
#include "sem.h"
#define ENOMEM -1
#define ENOSEQ -2
#define ENOENT -3
#define EEXIST -4
#define EINVAL -5
#define EAGAIN -6
#define EIDRM -7
#define E2BIG -8
#define ENOMSG -9
#define IPC_RMID 0
#define SEQ_MULTIPLIER 16
#define IPC_PRIVATE 0
#define IPC_CREATE 2
#define IPC_EXCL 1
#define IPC_NOWAIT 1
#define MSG_MSGSZ (PAGE_SIZE-(int)sizeof(msg_msg))
#define MSG_MSGSEGSZ (PAGE_SIZE-(int)sizeof(msg_msgseg))
#define MAX_MSGNUM 256
typedef struct msg_queue {
    int key;
    int seq;
    int max_msg;
    int sum_msg;
    ListNode q_message;
    ListNode q_sender;
    ListNode q_receiver;
} msg_queue;
typedef struct ipc_ids {
    int size;
    int in_use;
    unsigned short seq;
    SpinLock lock;
    msg_queue* entries[16];
} ipc_ids;
typedef struct msgbuf {
    int mtype;
    char data[];
} msgbuf;
typedef struct msg_msgseg {
    struct msg_msgseg* nxt;
    char data[];
} msg_msgseg;
typedef struct msg_msg {
    ListNode node;
    int mtype;
    int size;
    msg_msgseg* nxt;
    char data[];
} msg_msg;
typedef struct msg_sender {
    ListNode node;
    struct proc* proc;
} msg_sender;
typedef struct msg_receiver {
    ListNode node;
    struct proc* proc;
    int mtype;
    int size;
    msg_msg* r_msg;
} msg_receiver;
int sys_msgget(int key, int msgflg);
int sys_msgsnd(int msgid, msgbuf* msgp, int msgsz, int msgflg);
int sys_msgrcv(int msgid, msgbuf* msgp, int msgsz, int mtype, int msgflg);
int sys_msgctl(int msgid, int cmd);
#endif