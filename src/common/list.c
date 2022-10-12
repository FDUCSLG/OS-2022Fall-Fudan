#include <common/list.h>

void init_list_node(ListNode* node) {
    node->prev = node;
    node->next = node;
}

ListNode* _merge_list(ListNode* node1, ListNode* node2) {
    if (!node1)
        return node2;
    if (!node2)
        return node1;

    // before: (arrow is the next pointer)
    //   ... --> node1 --> node3 --> ...
    //   ... <-- node2 <-- node4 <-- ...
    //
    // after:
    //   ... --> node1 --+  +-> node3 --> ...
    //                   |  |
    //   ... <-- node2 <-+  +-- node4 <-- ...

    ListNode* node3 = node1->next;
    ListNode* node4 = node2->prev;

    node1->next = node2;
    node2->prev = node1;
    node4->next = node3;
    node3->prev = node4;

    return node1;
}

ListNode* _detach_from_list(ListNode* node) {
    ListNode* prev = node->prev;

    node->prev->next = node->next;
    node->next->prev = node->prev;
    init_list_node(node);

    if (prev == node)
        return NULL;
    return prev;
}

QueueNode* add_to_queue(QueueNode** head, QueueNode* node) {
    do
        node->next = *head;
    while (!__atomic_compare_exchange_n(head, &node->next, node, true, __ATOMIC_ACQ_REL,
                                        __ATOMIC_RELAXED));
    return node;
}

QueueNode* fetch_from_queue(QueueNode** head) {
    QueueNode* node;
    do
        node = *head;
    while (node
           && !__atomic_compare_exchange_n(head, &node, node->next, true, __ATOMIC_ACQ_REL,
                                           __ATOMIC_RELAXED));
    return node;
}

QueueNode* fetch_all_from_queue(QueueNode** head) {
    return __atomic_exchange_n(head, NULL, __ATOMIC_ACQ_REL);
}

void queue_init(Queue* x) {
    x->begin = x->end = 0;
    x->sz = 0;
    init_spinlock(&x->lk);
}
void queue_lock(Queue* x) {
    _acquire_spinlock(&x->lk);
}
void queue_unlock(Queue* x) {
    _release_spinlock(&x->lk);
}
void queue_push(Queue* x, ListNode* item) {
    init_list_node(item);
    if (x->sz == 0) {
        x->begin = x->end = item;

    } else {
        _merge_list(x->end, item);
        x->end = item;
    }
    x->sz++;
}
void queue_pop(Queue* x) {
    if (x->sz == 0)
        PANIC();
    if (x->sz == 1) {
        x->begin = x->end = 0;
    } else {
        auto t = x->begin;
        x->begin = x->begin->next;
        _detach_from_list(t);
    }
    x->sz--;
}
ListNode* queue_front(Queue* x) {
    if (!x || !x->begin)
        PANIC();
    return x->begin;
}
bool queue_empty(Queue* x) {
    return x->sz == 0;
}