#include "ipc.h"
#include "string.h"
#include "kernel/mem.h"
#include "kernel/sched.h"
#include "kernel/init.h"
#include "kernel/printk.h"
static ipc_ids msg_ids;
void init_ipc() {
    init_spinlock(&msg_ids.lock);
    msg_ids.in_use = 0;
    msg_ids.seq = 0;
    msg_ids.size = 16;
    memset(msg_ids.entries, 0, sizeof(msg_ids.entries));
}
define_early_init(ipc_msg) {
    init_ipc();
}
static int ipc_addid(msg_queue* que) {
    int id = 0;
    for (; id < msg_ids.size; id++) {
        if (msg_ids.entries[id] == NULL)
            goto found;
    }
    return -1;
found:
    msg_ids.in_use++;
    que->seq = msg_ids.seq++;
    msg_ids.entries[id] = que;
    return id;
}
static inline int ipc_buildin(int id, int seq) {
    return seq * SEQ_MULTIPLIER + id;
}
static int newque(int key) {
    int id;
    msg_queue* que = (msg_queue*)kalloc(sizeof(msg_queue));
    if (que == NULL)
        return ENOMEM;
    if ((id = ipc_addid(que)) < 0) {
        kfree(que);
        return ENOSEQ;
    }
    que->key = key;
    que->max_msg = MAX_MSGNUM;
    que->sum_msg = 0;
    init_list_node(&que->q_message);
    init_list_node(&que->q_receiver);
    init_list_node(&que->q_sender);
    return ipc_buildin(id, que->seq);
}
static int ipc_findkey(int key) {
    int id = 0;
    for (; id < msg_ids.size; id++) {
        if (msg_ids.entries[id] != NULL && msg_ids.entries[id]->key == key)
            return id;
    }
    return -1;
}
int sys_msgget(int key, int msgflg) {
    int ret;
    _acquire_spinlock(&msg_ids.lock);
    if (key == IPC_PRIVATE)
        ret = newque(key);
    else {
        int id = ipc_findkey(key);
        if (id == -1) {  // not found
            if (msgflg & IPC_CREATE)
                ret = newque(key);
            else
                ret = ENOENT;
        } else {  // found
            if (msgflg & IPC_EXCL)
                ret = EEXIST;
            else
                ret = ipc_buildin(id, msg_ids.entries[id]->seq);
        }
    }
    _release_spinlock(&msg_ids.lock);
    return ret;
}
static void free_msg(msg_msg* msg) {
    msg_msgseg *mseg = msg->nxt, *nxtseg;
    while (mseg) {
        nxtseg = mseg->nxt;
        kfree_page((void*)mseg);
        mseg = nxtseg;
    }
    kfree_page(msg);
}
static msg_msg* load_msg(void* src, int len) {
    msg_msg* msg = (msg_msg*)kalloc_page();
    if (msg == NULL)
        return NULL;
    memcpy(msg->data, src, MIN(MSG_MSGSZ, len));
    len -= MIN(MSG_MSGSZ, len);
    src += MIN(MSG_MSGSZ, len);
    msg->nxt = NULL;
    msg_msgseg** lst = &msg->nxt;
    while (len > 0) {
        msg_msgseg* mseg = (msg_msgseg*)kalloc_page();
        if (mseg == NULL)
            goto free_obj;
        memcpy(mseg->data, src, MIN(MSG_MSGSEGSZ, len));
        *lst = mseg;
        mseg->nxt = NULL;
        lst = &mseg->nxt;
        len -= MIN(MSG_MSGSEGSZ, len);
        src += MIN(MSG_MSGSEGSZ, len);
    }
    return msg;
free_obj:
    free_msg(msg);
    return NULL;
}
static msg_queue* get_msgq(int msgid) {
    int id = msgid % SEQ_MULTIPLIER;
    if (id >= msg_ids.size || msg_ids.entries[id] == NULL)
        return NULL;
    if (msgid / SEQ_MULTIPLIER != msg_ids.entries[id]->seq)
        return NULL;
    return msg_ids.entries[id];
}
static int testmsg(int rqtype, int type) {
    if (rqtype == 0)
        return 1;
    else if (rqtype < 0)
        return rqtype + type <= 0;
    else
        return rqtype == type;
}
static int pipeline_send(msg_queue* msgq, msg_msg* msg) {
    _for_in_list(node, &msgq->q_receiver) {
        if (node == &msgq->q_receiver)
            break;
        msg_receiver* rcver = container_of(node, msg_receiver, node);
        if (testmsg(rcver->mtype, msg->mtype)) {
            _detach_from_list(node);
            if (msg->size > rcver->size) {
                rcver->r_msg = NULL;
                activate_proc(rcver->proc);
            } else {
                rcver->r_msg = msg;
                activate_proc(rcver->proc);
                return 1;
            }
        }
    }
    return 0;
}
int sys_msgsnd(int msgid, msgbuf* msgp, int msgsz, int msgflg) {
    int err = EINVAL;
    if (msgsz < 0 || msgp == NULL || msgp->mtype < 1)
        return EINVAL;
    msg_msg* msg = load_msg((void*)msgp->data, msgsz);
    if (msg == NULL)
        return ENOMEM;
    msg->mtype = msgp->mtype;
    msg->size = msgsz;
retry:
    _acquire_spinlock(&msg_ids.lock);
    msg_queue* msgq = get_msgq(msgid);
    if (msgq == NULL) {
        err = EIDRM;
        goto free_obj;
    }
    if (msgq->sum_msg + 1 > msgq->max_msg) {
        if (msgflg & IPC_NOWAIT) {
            err = EAGAIN;
            goto free_obj;
        }
        msg_sender sender;
        sender.proc = thisproc();
        _insert_into_list(msgq->q_sender.prev, &sender.node);
        _acquire_sched_lock();
        _release_spinlock(&msg_ids.lock);
        _sched(SLEEPING);
        goto retry;
    }
    if (!pipeline_send(msgq, msg)) {
        _insert_into_list(msgq->q_message.prev, &msg->node);
        msgq->sum_msg++;
    }
    _release_spinlock(&msg_ids.lock);
    return 0;
free_obj:
    _release_spinlock(&msg_ids.lock);
    free_msg(msg);
    return err;
}
static void ss_wakeup(ListNode* head) {
    while (!_empty_list(head)) {
        ListNode* node = head->next;
        _detach_from_list(node);
        activate_proc(container_of(node, msg_sender, node)->proc);
    }
}
static void store_msg(void* dst, msg_msg* msg, int msgsz) {
    memcpy(dst, (void*)msg->data, MIN(msgsz, MSG_MSGSZ));
    msgsz -= MIN(MSG_MSGSZ, msgsz);
    dst += MIN(MSG_MSGSZ, msgsz);
    msg_msgseg* sg = msg->nxt;
    while (msgsz > 0) {
        memcpy(dst, (void*)sg->data, MIN(msgsz, MSG_MSGSEGSZ));
        msgsz -= MIN(msgsz, MSG_MSGSEGSZ);
        dst += MIN(msgsz, MSG_MSGSEGSZ);
        sg = sg->nxt;
    }
}
int sys_msgrcv(int msgid, msgbuf* msgp, int msgsz, int mtype, int msgflg) {
    int err = EINVAL;
    if (msgsz < 0 || msgp == NULL)
        return EINVAL;
    _acquire_spinlock(&msg_ids.lock);
    msg_queue* msgq = get_msgq(msgid);
    if (msgq == NULL) {
        err = EIDRM;
        goto out_lock;
    }
    msg_msg* found_msg = NULL;
    _for_in_list(node, &msgq->q_message) {
        if (node == &msgq->q_message)
            break;
        msg_msg* msg = container_of(node, msg_msg, node);
        if (testmsg(mtype, msg->mtype)) {
            found_msg = msg;
            if (mtype < -1) {
                mtype++;
            } else
                break;
        }
    }
    if (found_msg) {
        if (found_msg->size > msgsz) {
            err = E2BIG;
            goto out_lock;
        }
        _detach_from_list(&found_msg->node);
        msgq->sum_msg--;
        ss_wakeup(&msgq->q_sender);
        _release_spinlock(&msg_ids.lock);
    } else {
        if (msgflg & IPC_NOWAIT) {
            err = ENOMSG;
            goto out_lock;
        }
        msg_receiver receiver;
        _insert_into_list(msgq->q_receiver.prev, &receiver.node);
        receiver.mtype = mtype;
        receiver.proc = thisproc();
        receiver.size = msgsz;
        _acquire_sched_lock();
        _release_spinlock(&msg_ids.lock);
        _sched(SLEEPING);
        found_msg = receiver.r_msg;
        if (found_msg == NULL)
            return E2BIG;
    }
    msgsz = MIN(msgsz, found_msg->size);
    store_msg(msgp->data, found_msg, msgsz);
    free_msg(found_msg);
    return msgsz;
out_lock:
    _release_spinlock(&msg_ids.lock);
    return err;
}
static void expunge_all(msg_queue* que) {
    while (!_empty_list(&que->q_receiver)) {
        ListNode* node = que->q_receiver.next;
        _detach_from_list(node);
        container_of(node, msg_receiver, node)->r_msg = NULL;
        activate_proc(container_of(node, msg_receiver, node)->proc);
    }
}
static void freeque(int id) {
    _acquire_spinlock(&msg_ids.lock);
    msg_queue* msgq = get_msgq(id);
    if (msgq != NULL) {
        msg_ids.entries[id % SEQ_MULTIPLIER] = NULL;
        ss_wakeup(&msgq->q_receiver);
        expunge_all(msgq);
        while (!_empty_list(&msgq->q_message)) {
            ListNode* node = msgq->q_message.next;
            _detach_from_list(node);
            free_msg(container_of(node, msg_msg, node));
        }
        msg_ids.in_use--;
        kfree((void*)msgq);
    }
    _release_spinlock(&msg_ids.lock);
}
int sys_msgctl(int msgid, int cmd) {
    if (cmd == IPC_RMID) {
        freeque(msgid);
        return 0;
    }
    return EINVAL;
}