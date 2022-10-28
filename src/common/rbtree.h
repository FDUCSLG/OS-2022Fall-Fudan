#ifndef _MYRBTREE_H
#define _MYRBTREE_H
#include "common/defines.h"
struct rb_node_ {
    unsigned long __rb_parent_color;
    struct rb_node_ *rb_right;
    struct rb_node_ *rb_left;
} __attribute__((aligned(sizeof(long))));
typedef struct rb_node_ *rb_node;
struct rb_root_ {
    rb_node rb_node;
};
typedef struct rb_root_ *rb_root;
/* NOTE:You should add lock when use */
WARN_RESULT int _rb_insert(rb_node node,rb_root root,bool (*cmp)(rb_node lnode,rb_node rnode));
void _rb_erase(rb_node node, rb_root root);
rb_node _rb_lookup(rb_node node,rb_root rt,bool (*cmp)(rb_node lnode,rb_node rnode));
rb_node _rb_first(rb_root root);
#endif