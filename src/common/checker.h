#pragma once

#include <common/defines.h>

__attribute__((error("Checker: context mismatching"), unused)) void __checker_match_fail();
__attribute__((error("Checker: delayed task in invalid context"), unused)) void __checker_task_fail();
__attribute__((always_inline)) static inline void __checker_check(int* x) { if (*x) __checker_match_fail(); }

#define setup_checker(id) int __chk_##id __attribute__((cleanup(__checker_check))) = 0; \
void (*__chkdf_##id)(u64) = NULL; u64 __chkda_##id = 0;
#define checker_set_delayed_task(id, f, a) { if (__chkdf_##id || !__chk_##id) __checker_task_fail(); \
__chkdf_##id = (void(*)(u64))(void*)f, __chkda_##id = (u64)a; }
#define checker_begin_ctx(id) (++__chk_##id)
#define checker_end_ctx(id) ({ int __cnt = --__chk_##id; \
if (!__cnt && __chkdf_##id) __chkdf_##id(__chkda_##id), __chkdf_##id = NULL; \
__cnt; })
#define checker_begin_ctx_before_call(id, f, ...) (checker_begin_ctx(id), f(__VA_ARGS__))
#define checker_end_ctx_after_call(id, f, ...) ({ auto __ret = __builtin_choose_expr( \
__builtin_types_compatible_p(typeof(f(__VA_ARGS__)), void), \
(f(__VA_ARGS__), 0), f(__VA_ARGS__)); checker_end_ctx(id); __ret; })
