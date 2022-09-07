#pragma once

#include <common/defines.h>

#define early_init_func(func) __attribute__((section(".init.early"), used)) static void* __initcall_##func = &func;
#define init_func(func) __attribute__((section(".init"), used)) static void* __initcall_##func = &func;
#define rest_init_func(func) __attribute__((section(".init.rest"), used)) static void* __initcall_##func = &func;

void do_early_init();
void do_init();
void do_rest_init();
