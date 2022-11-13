#pragma once

#define RAND_MAX 32768

void alloc_test();
void rbtree_test();
void proc_test();
void ipc_test();
void vm_test();
void container_test();
void user_proc_test();
unsigned rand();
void srand(unsigned seed);