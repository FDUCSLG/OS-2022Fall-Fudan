#include "common/rbtree.h"
#include "aarch64/intrinsic.h"
#include "common/rc.h"
#include "common/spinlock.h"
#include "test.h"
#include <kernel/printk.h>
struct mytype {
    struct rb_node_ node;
    int key;
    int data;
};
static bool rb_cmp(rb_node n1, rb_node n2) {
    return container_of(n1, struct mytype, node)->key <
           container_of(n2, struct mytype, node)->key;
}
static struct mytype p[4][1000], tmp;
static struct rb_root_ rt;
static SpinLock lock;
#define FAIL(...)                                                              \
    {                                                                          \
        printk(__VA_ARGS__);                                                   \
        while (1)                                                              \
            ;                                                                  \
    }
static RefCount x;

void rbtree_test() {
    int cid = cpuid();
    for (int i = 0; i < 1000; i++) {
        p[cid][i].key = cid * 100000 + i;
        p[cid][i].data = -p[cid][i].key;
    }
    if (cid == 0) init_spinlock(&lock);
    arch_dsb_sy();
    _increment_rc(&x);
    while (x.count < 4)
        ;
    arch_dsb_sy();
    for (int i = 0; i < 1000; i++) {
        setup_checker(0);
        acquire_spinlock(0, &lock);
        int ok = _rb_insert(&p[cid][i].node, &rt, rb_cmp);
        if (ok) FAIL("insert failed!\n");
        release_spinlock(0, &lock);
    }
    for (int i = 0; i < 1000; i++) {
        setup_checker(0);
        acquire_spinlock(0, &lock);
        tmp.key = cid * 100000 + i;
        rb_node np = _rb_lookup(&tmp.node, &rt, rb_cmp);
        if (np == NULL) FAIL("NULL\n");
        if (tmp.key != -container_of(np, struct mytype, node)->data) {
            FAIL("data error! %d %d %d\n", tmp.key,
                 container_of(np, struct mytype, node)->key,
                 container_of(np, struct mytype, node)->data);
        }
        release_spinlock(0, &lock);
    }
    arch_dsb_sy();
    _increment_rc(&x);
    while (x.count < 8)
        ;
    arch_dsb_sy();
    if (cid == 0) printk("rbtree_test PASS\n");
}