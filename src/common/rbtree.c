#include "rbtree.h"
#define RB_RED 0
#define RB_BLACK 1
#define rb_parent(r) ((rb_node)((r)->__rb_parent_color & ~3))
#define __rb_parent(pc) ((rb_node)(pc & ~3))

#define __rb_color(pc) ((pc)&1)
#define __rb_is_black(pc) __rb_color(pc)
#define __rb_is_red(pc) (!__rb_color(pc))
#define rb_color(rb) __rb_color((rb)->__rb_parent_color)
#define rb_is_red(rb) __rb_is_red((rb)->__rb_parent_color)
#define rb_is_black(rb) __rb_is_black((rb)->__rb_parent_color)
static inline void rb_set_black(rb_node rb) {
    rb->__rb_parent_color |= RB_BLACK;
}
static inline rb_node rb_red_parent(rb_node red) {
    return (rb_node)red->__rb_parent_color;
}
static inline void rb_set_parent(rb_node rb, rb_node p) {
    rb->__rb_parent_color = rb_color(rb) | (unsigned long)p;
}
static inline void rb_set_parent_color(rb_node rb, rb_node p, int color) {
    rb->__rb_parent_color = (unsigned long)p | color;
}
static inline void __rb_change_child(rb_node old, rb_node new, rb_node parent, rb_root root) {
    if (parent) {
        if (parent->rb_left == old)
            parent->rb_left = new;
        else
            parent->rb_right = new;
    } else
        root->rb_node = new;
}
static inline void __rb_rotate_set_parents(rb_node old, rb_node new, rb_root root, int color) {
    rb_node parent = rb_parent(old);
    new->__rb_parent_color = old->__rb_parent_color;
    rb_set_parent_color(old, new, color);
    __rb_change_child(old, new, parent, root);
}
static void __rb_insert_fix(rb_node node, rb_root root) {
    rb_node parent = rb_red_parent(node), gparent, tmp;
    while (1) {
        if (!parent) {
            rb_set_parent_color(node, NULL, RB_BLACK);
            root->rb_node = node;
            break;
        } else if (rb_is_black(parent))
            break;

        gparent = rb_red_parent(parent);
        tmp = gparent->rb_right;
        if (parent != tmp) { /* parent == gparent->rb_left */
            if (tmp && rb_is_red(tmp)) { /*Case 1,uncle is red*/
                rb_set_parent_color(tmp, gparent, RB_BLACK);
                rb_set_parent_color(parent, gparent, RB_BLACK);
                node = gparent;
                parent = rb_parent(node);
                rb_set_parent_color(node, parent, RB_RED);
                continue;
            }
            // Uncle is black
            tmp = parent->rb_right;
            if (node == tmp) { /*Case 2,node is right child,left rotate*/
                parent->rb_right = tmp = node->rb_left;
                if (tmp)
                    rb_set_parent_color(tmp, parent, RB_BLACK);
                node->rb_left = parent;
                rb_set_parent_color(parent, node, RB_RED);
                parent = node;
                tmp = node->rb_right;
            }
            /*Case 3,can break*/
            gparent->rb_left = tmp;
            if (tmp)
                rb_set_parent_color(tmp, gparent, RB_BLACK);
            parent->rb_right = gparent;
            __rb_rotate_set_parents(gparent, parent, root, RB_RED);
            break;
        } else {
            tmp = gparent->rb_left;
            if (tmp && rb_is_red(tmp)) { /*Case 1,uncle is red*/
                rb_set_parent_color(tmp, gparent, RB_BLACK);
                rb_set_parent_color(parent, gparent, RB_BLACK);
                node = gparent;
                parent = rb_parent(node);
                rb_set_parent_color(node, parent, RB_RED);
                continue;
            }
            // Uncle is black
            tmp = parent->rb_left;
            if (node == tmp) { /*Case 2,node is right child,left rotate*/
                parent->rb_left = tmp = node->rb_right;
                if (tmp)
                    rb_set_parent_color(tmp, parent, RB_BLACK);
                node->rb_right = parent;
                rb_set_parent_color(parent, node, RB_RED);
                parent = node;
                tmp = node->rb_left;
            }
            /*Case 3,can break*/
            gparent->rb_right = tmp;
            if (tmp)
                rb_set_parent_color(tmp, gparent, RB_BLACK);
            parent->rb_left = gparent;
            __rb_rotate_set_parents(gparent, parent, root, RB_RED);
            break;
        }
    }
}
static rb_node __rb_erase(rb_node node, rb_root root) {
    rb_node child = node->rb_right, tmp = node->rb_left;
    rb_node parent, rebalance;
    unsigned long pc;
    if (!tmp) {
        pc = node->__rb_parent_color;
        parent = __rb_parent(pc);
        __rb_change_child(node, child, parent, root);
        if (child) {
            child->__rb_parent_color = pc;
            rebalance = NULL;
        } else
            rebalance = __rb_is_black(pc) ? parent : NULL;
    } else if (!child) {
        tmp->__rb_parent_color = pc = node->__rb_parent_color;
        parent = __rb_parent(pc);
        __rb_change_child(node, tmp, parent, root);
        rebalance = NULL;
    } else {
        rb_node successor = child, child2;
        tmp = child->rb_left;
        if (!tmp) {
            parent = successor;
            child2 = successor->rb_right;
        } else {
            do {
                parent = successor;
                successor = tmp;
                tmp = tmp->rb_left;
            } while (tmp);
            parent->rb_left = child2 = successor->rb_right;
            successor->rb_right = child;
            rb_set_parent(child, successor);
        }
        successor->rb_left = tmp = node->rb_left;
        rb_set_parent(tmp, successor);
        pc = node->__rb_parent_color;
        tmp = __rb_parent(pc);
        __rb_change_child(node, successor, tmp, root);
        if (child2) {
            successor->__rb_parent_color = pc;
            rb_set_parent_color(child2, parent, RB_BLACK);
            rebalance = NULL;
        } else {
            unsigned long pc2 = successor->__rb_parent_color;
            successor->__rb_parent_color = pc;
            rebalance = __rb_is_black(pc2) ? parent : NULL;
        }
    }
    return rebalance;
}
static void __rb_erase_fix(rb_node parent, rb_root root) {
    rb_node node = NULL, sibling, tmp1, tmp2;
    while (1) {
        sibling = parent->rb_right;
        if (node != sibling) {
            if (rb_is_red(sibling)) { /*Case 1,sibling is red*/
                parent->rb_right = tmp1 = sibling->rb_left;
                rb_set_parent_color(tmp1, parent, RB_BLACK);
                sibling->rb_left = parent;
                __rb_rotate_set_parents(parent, sibling, root, RB_RED);
                sibling = tmp1;
            }
            tmp1 = sibling->rb_right;
            if (!tmp1 || rb_is_black(tmp1)) {
                tmp2 = sibling->rb_left;
                if (!tmp2 || rb_is_black(tmp2)) { /*Case 2,sibling black,ch1,ch2 black*/
                    rb_set_parent_color(sibling, parent, RB_RED);
                    if (rb_is_red(parent)) {
                        rb_set_black(parent);
                    } else {
                        node = parent;
                        parent = rb_parent(node);
                        if (parent)
                            continue;
                    }
                    break;
                } else { /*Case 3*/
                    sibling->rb_left = tmp1 = tmp2->rb_right;
                    if (tmp1)
                        rb_set_parent_color(tmp1, sibling, RB_BLACK);
                    tmp2->rb_right = sibling;
                    parent->rb_right = tmp2;
                    tmp1 = sibling;
                    sibling = tmp2;
                }
            }
            parent->rb_right = tmp2 = sibling->rb_left;
            if (tmp2)
                rb_set_parent(tmp2, parent);
            sibling->rb_left = parent;
            rb_set_parent_color(tmp1, sibling, RB_BLACK);
            __rb_rotate_set_parents(parent, sibling, root, RB_BLACK);
            break;
        } else {
            sibling = parent->rb_left;
            if (rb_is_red(sibling)) { /*Case 1,sibling is red*/
                parent->rb_left = tmp1 = sibling->rb_right;
                rb_set_parent_color(tmp1, parent, RB_BLACK);
                sibling->rb_right = parent;
                __rb_rotate_set_parents(parent, sibling, root, RB_RED);
                sibling = tmp1;
            }
            tmp1 = sibling->rb_left;
            if (!tmp1 || rb_is_black(tmp1)) {
                tmp2 = sibling->rb_right;
                if (!tmp2 || rb_is_black(tmp2)) { /*Case 2,sibling black,ch1,ch2 black*/
                    rb_set_parent_color(sibling, parent, RB_RED);
                    if (rb_is_red(parent)) {
                        rb_set_black(parent);
                    } else {
                        node = parent;
                        parent = rb_parent(node);
                        if (parent)
                            continue;
                    }
                    break;
                } else { /*Case 3*/
                    sibling->rb_right = tmp1 = tmp2->rb_left;
                    if (tmp1)
                        rb_set_parent_color(tmp1, sibling, RB_BLACK);
                    tmp2->rb_left = sibling;
                    parent->rb_left = tmp2;
                    tmp1 = sibling;
                    sibling = tmp2;
                }
            }
            parent->rb_left = tmp2 = sibling->rb_right;
            if (tmp2)
                rb_set_parent(tmp2, parent);
            sibling->rb_right = parent;
            rb_set_parent_color(tmp1, sibling, RB_BLACK);
            __rb_rotate_set_parents(parent, sibling, root, RB_BLACK);
            break;
        }
    }
}
int _rb_insert(rb_node node, rb_root rt, bool (*cmp)(rb_node lnode, rb_node rnode)) {
    rb_node nw = rt->rb_node, parent = NULL;
    node->rb_left = node->rb_right = NULL;
    node->__rb_parent_color = 0;
    while (nw) {
        parent = nw;
        if (cmp(node, nw)) {
            nw = nw->rb_left;
            if (nw == NULL) {
                parent->rb_left = node;
                node->__rb_parent_color = (unsigned long)parent;
            }
        } else if (cmp(nw, node)) {
            nw = nw->rb_right;
            if (nw == NULL) {
                parent->rb_right = node;
                node->__rb_parent_color = (unsigned long)parent;
            }
        } else
            return -1;
    }
    __rb_insert_fix(node, rt);
    return 0;
}
void _rb_erase(rb_node node, rb_root root) {
    rb_node rebalance;
    rebalance = __rb_erase(node, root);
    if (rebalance)
        __rb_erase_fix(rebalance, root);
}
rb_node _rb_lookup(rb_node node, rb_root rt, bool (*cmp)(rb_node lnode, rb_node rnode)) {
    rb_node nw = rt->rb_node;
    while (nw) {
        if (cmp(node, nw)) {
            nw = nw->rb_left;
        } else if (cmp(nw, node)) {
            nw = nw->rb_right;
        } else
            return nw;
    }
    return NULL;
}
rb_node _rb_first(rb_root root) {
    rb_node n;
    n = root->rb_node;
    if (!n)
        return NULL;
    while (n->rb_left)
        n = n->rb_left;
    return n;
}