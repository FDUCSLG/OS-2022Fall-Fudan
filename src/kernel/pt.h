#pragma once

#include <aarch64/mmu.h>

struct pgdir
{
    PTEntriesPtr pt;
};

void init_pgdir(struct pgdir* pgdir);
void free_pgdir(struct pgdir* pgdir);
void attach_pgdir(struct pgdir* pgdir);
