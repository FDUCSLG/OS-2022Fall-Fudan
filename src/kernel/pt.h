#pragma once

#include <aarch64/mmu.h>

struct pgdir
{
    PTEntriesPtr pt;
};

void init_pgdir(struct pgdir* pgdir);
WARN_RESULT PTEntriesPtr get_pte(struct pgdir* pgdir, u64 va, bool alloc);
void free_pgdir(struct pgdir* pgdir);
void attach_pgdir(struct pgdir* pgdir);