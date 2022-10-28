#pragma once

#include <common/defines.h>
#include <aarch64/mmu.h>

WARN_RESULT void* kalloc_page();
void kfree_page(void*);

WARN_RESULT void* kalloc(isize);
void kfree(void*);
