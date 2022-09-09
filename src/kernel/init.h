#pragma once

#include <common/defines.h>

// #define early_init_func(func) __attribute__((section(".init.early"), used)) static void* __initcall_##func = &func;
// #define init_func(func) __attribute__((section(".init"), used)) static void* __initcall_##func = &func;
// #define rest_init_func(func) __attribute__((section(".init.rest"), used)) static void* __initcall_##func = &func;

#define define_early_init(name) \
static void init_##name(); \
__attribute__((section(".init.early"), used)) static void* __initcall_##name = &init_##name; \
static void init_##name()

#define define_init(name) \
static void init_##name(); \
__attribute__((section(".init"), used)) static void* __initcall_##name = &init_##name; \
static void init_##name()

#define define_rest_init(name) \
static void init_##name(); \
__attribute__((section(".init.rest"), used)) static void* __initcall_##name = &init_##name; \
static void init_##name()

void do_early_init();
void do_init();
void do_rest_init();
