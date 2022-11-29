#include <test/test.h>
#include <common/rc.h>
#include <common/string.h>
#include <kernel/pt.h>
#include <kernel/mem.h>
#include <kernel/printk.h>
#include <common/sem.h>
#include <kernel/sched.h>
#include <kernel/proc.h>
#include <kernel/syscall.h>
#include <kernel/paging.h>

// only need the heap section
// the heap section start at 0x0
void pgfault_first_test(){
	//init
	i64 limit = 10; //do not need too big
	struct proc* p = thisproc();
	struct pgdir* pd = &p->pgdir;
	ASSERT(pd->pt);//make sure the attached pt is valid
	attach_pgdir(pd);
	struct section* st = NULL;
	_for_in_list(node, &pd->section_head){
		if(node == &pd->section_head)continue;
		st = container_of(node, struct section, stnode);
		if(st->flags & ST_HEAP)break;
	}
	ASSERT(st);

	//lazy allocation
	u64 pc = left_page_cnt();
	printk("in lazy allocation\n");
	for(i64 i = 1; i < limit; i++){
		sbrk(i); 
		ASSERT(pc == left_page_cnt());
		sbrk(-i);
	}
	for(i64 i = 1; i < limit; i++){
		u64 addr = sbrk(i);
		*(i64*)addr = i; ASSERT(*(i64*)addr == i);
		sbrk(-i);
	}
	sbrk(limit);
	for(i64 i = 0; i < limit; i++){
		u64 addr = (u64)i * PAGE_SIZE;
		*(i64*)addr = i; 
		ASSERT(*(i64*)addr == i);
	}
	sbrk(-limit);

	//COW
	printk("in COW\n");
	pc = left_page_cnt();
	sbrk(limit);
	for(i64 i = 0; i < limit; ++i){
		u64 va = i * PAGE_SIZE;
		vmmap(pd, va, get_zero_page(), PTE_RO | PTE_USER_DATA);
		ASSERT(*(i64*)va == 0);
	}
	ASSERT(pc == left_page_cnt());
	arch_tlbi_vmalle1is(); //WHY need this line	
	for(i64 i = 0; i < limit; ++i){
		u64 va = (u64)i * PAGE_SIZE;
		*(i64*)va = i;
	}
	if(!check_zero_page())PANIC();

	//swap
	printk("in swap\n");
	swapout(pd, st);
	arch_tlbi_vmalle1is(); 
	for(i64 i = 0; i < limit; ++i){
		u64 va = (u64)i * PAGE_SIZE;
		ASSERT(*(i64*)va == i);
	}	
	sbrk(-limit);
	free_pgdir(pd);
	printk("pgfault_first_test PASS!\n");
}

void pgfault_second_test(){
	//init
	i64 limit = 10; //do not need too big
	struct pgdir* pd = &thisproc()->pgdir;
	init_pgdir(pd); 
	attach_pgdir(pd);
	struct section* st = NULL;
	_for_in_list(node, &pd->section_head){
		if(node == &pd->section_head)continue;
		st = container_of(node, struct section, stnode);
		if(st->flags & ST_HEAP)break;
	}
	ASSERT(st);

	//COW + lazy allocation
	sbrk(limit);
	for(i64 i = 0; i < limit/2; ++i){
		u64 va = i * PAGE_SIZE;
		vmmap(pd, va, get_zero_page(), PTE_RO | PTE_USER_DATA);
	}
	arch_tlbi_vmalle1is();
	for(i64 i = 0; i < limit; ++i){
		u64 va = (u64)i * PAGE_SIZE;
		*(i64*)va = i;
	}
	sbrk(-limit);
	if(!check_zero_page())PANIC();

	//COW + SWAP
	printk("COW + SWAP\n");
	sbrk(limit);
	for(i64 i = 0; i < limit; ++i){
		u64 va = i * PAGE_SIZE;
		vmmap(pd, va, get_zero_page(), PTE_RO | PTE_USER_DATA);
	}
	swapout(pd, st);
	arch_tlbi_vmalle1is();
	for(i64 i = 0; i < limit; ++i){
		u64 va = (u64)i * PAGE_SIZE;
		*(i64*)va = i;
	}
	sbrk(-limit);
	if(!check_zero_page())PANIC();

	//SWAP + lazy allocation
	printk("SWAP + lazy allocation\n");
	for(i64 i = 2; i < limit; ++i){
		u64 va = sbrk(i); 
		swapout(pd, st); //should not swapout
		arch_tlbi_vmalle1is();
		*(i64*)va = i;
		swapout(pd, st);  //only need to swapout one page
		arch_tlbi_vmalle1is();
		ASSERT(*(i64*)va == i);//swapin one page too
		swapout(pd, st);
		arch_tlbi_vmalle1is();
		*(i64*)(va + PAGE_SIZE)= -i; //swapin one page and allocate a new one
		swapout(pd, st);
		arch_tlbi_vmalle1is();
		ASSERT(*(i64*)va + *(i64*)(va+PAGE_SIZE) == 0);
		sbrk(-i);
	}	
	
	//SWAP + COW + lazy allocation
	printk("SWAP + COW + lazy allocation\n");
	for(i64 i = 2; i < limit; ++i){
		sbrk(i);
		vmmap(pd, 0, get_zero_page(), PTE_RO | PTE_USER_DATA);
		swapout(pd, st);
		arch_tlbi_vmalle1is();
		*(i64*)0 = i; *(i64*)PAGE_SIZE = -i;
		swapout(pd, st);
		arch_tlbi_vmalle1is();
		ASSERT(*(i64*)0 + *(i64*)PAGE_SIZE == 0);
		sbrk(-i);
	}
	if(!check_zero_page())PANIC();

	printk("pgfault_second_test PASS!\n");
}
