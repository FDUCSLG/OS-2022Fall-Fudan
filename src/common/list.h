#pragma once

#include <common/defines.h>
#include <common/spinlock.h>

// ListNode represents one node on a circular list.
typedef struct ListNode {
    struct ListNode *prev, *next;
} ListNode;

// initialize a sigle node circular list.
void init_list_node(ListNode* node);

// * List operations without locks: USE THEM CAREFULLY
// - merge the list containing `node1` and the list containing `node2`
// into one list. It guarantees `node1->next == node2`. Both lists can be
// empty. This function will return the merged list.
ListNode* _merge_list(ListNode* node1, ListNode* node2);
// - syntax sugar: insert a single new node into the list
#define _insert_into_list(list, node) (init_list_node(node), _merge_list(list, node))
// - remove `node` from the list, and then `node` becomes a single
// node list. It usually returns `node->prev`. If `node` is
// the last one in the list, it will return NULL.
ListNode* _detach_from_list(ListNode* node);
// - walk through the list
#define _for_in_list(valptr, list) \
    for (ListNode* __flag = (list), *valptr = __flag->next; valptr; \
         valptr = valptr == __flag ? (void*)0 : valptr->next)
// - test if the list is empty
#define _empty_list(list) ((list)->next == (list))

// * List operations with locks
#define merge_list(lock, node1, node2) \
    ({ \
        _acquire_spinlock(lock); \
        ListNode* __t = _merge_list(node1, node2); \
        _release_spinlock(lock); \
        __t; \
    })
#define insert_into_list(lock, list, node) \
    ({ \
        _acquire_spinlock(lock); \
        ListNode* __t = _insert_into_list(list, node); \
        _release_spinlock(lock); \
        __t; \
    })
#define detach_from_list(lock, node) \
    ({ \
        _acquire_spinlock(lock); \
        ListNode* __t = _detach_from_list(node); \
        _release_spinlock(lock); \
        __t; \
    })

// Lockfree Queue: implemented as a lock-free single linked list.
typedef struct QueueNode {
    struct QueueNode* next;
} QueueNode;
// add a node to the queue and return the added node
QueueNode* add_to_queue(QueueNode** head, QueueNode* node);
// remove the last added node from the queue and return it
QueueNode* fetch_from_queue(QueueNode** head);
// remove all nodes from the queue and return them as a single list
QueueNode* fetch_all_from_queue(QueueNode** head);

typedef struct Queue {
    ListNode* begin;
    ListNode* end;
    int sz;
    SpinLock lk;
} Queue;
void queue_init(Queue* x);
void queue_lock(Queue* x);
void queue_unlock(Queue* x);
void queue_push(Queue* x, ListNode* item);
void queue_pop(Queue* x);
ListNode* queue_front(Queue* x);
bool queue_empty(Queue* x);