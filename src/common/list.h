#pragma once

#include <common/defines.h>
#include <common/spinlock.h>

// ListNode represents one node on a circular list.
typedef struct ListNode {
    struct ListNode *prev, *next;
} ListNode;

// initialize a sigle node circular list.
void init_list_node(ListNode *node);

// * List operations without locks: USE THEM CAREFULLY
// - merge the list containing `node1` and the list containing `node2`
// into one list. It guarantees `node1->next == node2`. Both lists can be
// empty. This function will return the merged list.
ListNode *_merge_list(ListNode *node1, ListNode *node2);
// - syntax sugar: insert a single new node into the list
#define _insert_into_list(list, node) (init_list_node(node), _merge_list(list, node))
// - remove `node` from the list, and then `node` becomes a single
// node list. It usually returns `node->prev`. If `node` is
// the last one in the list, it will return NULL.
ListNode *_detach_from_list(ListNode *node);
// - walk through the list
#define _for_in_list(valptr, list) for (ListNode *__flag = (list), *valptr = __flag; \
    __flag; __flag = (valptr = valptr->next) == __flag ? (void*)0 : __flag)


// * List operations with locks
#define merge_list(checker, lock, node1, node2) ({ \
    acquire_spinlock(checker, lock); \
    ListNode* __t = _merge_list(node1, node2); \
    release_spinlock(checker, lock); \
    __t; })
#define insert_into_list(checker, lock, list, node) ({ \
    acquire_spinlock(checker, lock); \
    ListNode* __t = _insert_into_list(list, node); \
    release_spinlock(checker, lock); \
    __t; })
#define detach_from_list(checker, lock, node) ({ \
    acquire_spinlock(checker, lock); \
    ListNode* __t = _detach_from_list(node); \
    release_spinlock(checker, lock); \
    __t; })


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
