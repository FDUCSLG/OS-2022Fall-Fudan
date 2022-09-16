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
void _rb_insert_fix(rb_node node, rb_root root);
void _rb_erase(rb_node node, rb_root root);
rb_node _rb_first(rb_root root);
/** Example:
 *   <--myrbtree_api.h-->
 *   #ifndef _MYRBTREEAPI_H
 *   #define _MYRBTREEAPI_H
 *   #include "myrbtree.h"
 *   typedef struct {
 *       struct rb_node_ node;
 *       long key;
 *   } mytype;
 *   mytype *myfind(rb_root rt, long key);
 *   int my_insert(rb_root rt, mytype *data);
 *   int my_delete(rb_root rt, mytype *data);
 *   mytype *my_first(rb_root rt);
 *   #endif
 *  <--myrbtree_api.c-->
 *  #include "myrbtree_api.h"
 *  mytype *myfind(rb_root rt, long key) {
 *      rb_node nw = rt->rb_node;
 *      while (nw) {
 *          mytype *data = container_of(nw, mytype, node);
 *          if (data->key > key)
 *              nw = nw->rb_left;
 *          else if (data->key < key)
 *              nw = nw->rb_right;
 *          else
 *              return data;
 *      }
 *      return NULL;
 *  }
 *  int my_insert(rb_root rt, mytype *data) {
 *      rb_node nw = rt->rb_node, parent = NULL;
 *      data->node.rb_left = data->node.rb_right = NULL;
 *      while (nw) {
 *          mytype *d = container_of(nw, mytype, node);
 *          parent = nw;
 *          if (d->key > data->key) {
 *              nw = (nw)->rb_left;
 *              if (nw == NULL) {
 *                  parent->rb_left = &data->node;
 *                  data->node.__rb_parent_color = (unsigned long)parent;
 *              }
 *          } else if (d->key < data->key) {
 *              nw = (nw)->rb_right;
 *              if (nw == NULL) {
 *                  parent->rb_right = &data->node;
 *                  data->node.__rb_parent_color = (unsigned long)parent;
 *              }
 *          } else
 *              return -1;
 *      }
 *      rb_insert_fix(&data->node, rt);
 *      return 0;
 *  }
 *  int my_delete(rb_root rt, mytype *data) {
 *      if (myfind(rt, data->key) != data) return -1;
 *      rb_erase(&data->node, rt);
 *      return 0;
 *  }
 *  mytype *my_first(rb_root rt) {
 *      rb_node node = rb_first(rt);
 *      return container_of(node, mytype, node);
 *  }
 */
#endif