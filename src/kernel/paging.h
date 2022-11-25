#pragma once

#include <kernel/proc.h>
#include <aarch64/mmu.h>

#define ST_FILE   1
#define ST_SWAP  (1<<1)
#define ST_RO    (1<<2)
#define ST_HEAP  (1<<3)
#define ST_TEXT  (ST_FILE | ST_RO)
#define ST_DATA   ST_FILE 
#define ST_BSS    ST_FILE	

struct section{
	u64 flags;
	SleepLock sleeplock;
	u64 begin;
	u64 end;
	ListNode stnode;
	//struct file* fp;
	//u64 offset;	
};

WARN_RESULT void* alloc_page_for_user();
int pgfault(u64 iss);
void swapout(struct pgdir* pd, struct section* st);
void swapin(struct pgdir* pd, struct section* st);
void* alloc_page_for_user();
void init_sections(ListNode* section_head);
void free_sections(struct pgdir* pd);
u64 sbrk(i64 size);
