#pragma once

typedef int Checker;

__attribute__((error("Checker: context mismatching"), unused)) void __checker_fail();
__attribute__((always_inline)) static inline void __checker_check(Checker* x) { if (*x) __checker_fail(); }

#define setup_checker(id) Checker __chkcounter_##id __attribute__((cleanup(__checker_check))) = 0
#define checker_begin_ctx(id) (__chkcounter_##id++)
#define checker_end_ctx(id) (__chkcounter_##id--)
