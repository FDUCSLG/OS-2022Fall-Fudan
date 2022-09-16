#include <common/rc.h>

void init_rc(RefCount *rc) {
    rc->count = 0;
}

void _increment_rc(RefCount *rc) {
    __atomic_fetch_add(&rc->count, 1, __ATOMIC_ACQ_REL);
}

bool _decrement_rc(RefCount *rc) {
    i64 r = __atomic_sub_fetch(&rc->count, 1, __ATOMIC_ACQ_REL);
    return r <= 0;
}
