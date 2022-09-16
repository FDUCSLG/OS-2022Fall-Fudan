#include <common/rc.h>
#include <kernel/init.h>
#include <kernel/mem.h>

RefCount alloc_page_cnt;

define_early_init(alloc_page_cnt)
{
    init_rc(&alloc_page_cnt);
}

void* kalloc_page()
{
    _increment_rc(&alloc_page_cnt);
    // TODO
    return NULL;
}

void kfree_page(void* p)
{
    _decrement_rc(&alloc_page_cnt);
    // TODO
}

// TODO: kalloc kfree
