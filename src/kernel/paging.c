#include <kernel/proc.h>
#include <aarch64/mmu.h>
#include <fs/block_device.h>
#include <fs/cache.h> 
#include <kernel/paging.h>
#include <common/defines.h>
#include <kernel/pt.h>
#include <common/sem.h>
#include <common/list.h>
#include <kernel/mem.h>
#include <kernel/sched.h>
#include <common/string.h>
#include <kernel/printk.h>
#include <kernel/init.h>

define_rest_init(paging){
	//TODO init		
}

u64 sbrk(i64 size){
	//TODO
}	


void* alloc_page_for_user(){
	while(left_page_cnt() <= REVERSED_PAGES){ //this is a soft limit
		//TODO
	}
	return kalloc_page();
}

//caller must have the pd->lock
void swapout(struct pgdir* pd, struct section* st){
	ASSERT(!(st->flags & ST_SWAP));
	st->flags |= ST_SWAP;
	//TODO

}
//Free 8 continuous disk blocks
void swapin(struct pgdir* pd, struct section* st){
	ASSERT(st->flags & ST_SWAP);
	//TODO
	st->flags &= ~ST_SWAP;
}

int pgfault(u64 iss){
	struct proc* p = thisproc();
	struct pgdir* pd = &p->pgdir;
	u64 addr = arch_get_far();
	//TODO
}
