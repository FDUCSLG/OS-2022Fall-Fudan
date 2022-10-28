#pragma once

#include <common/defines.h>

void *memset(void *s, int c, usize n);
void *memcpy(void *restrict dest, const void *restrict src, usize n);
WARN_RESULT int memcmp(const void *s1, const void *s2, usize n);

// NOTE: memmove does not allocate extra memory and handles
// overlapped memory regions correctly, but it does not take side
// effects into consideration (e.g. two virtual memory regions mapped
// to the same physical memory region).
void *memmove(void *dest, const void *src, usize n);

// note: for string functions, please specify `n` explicitly.

// strncpy will `dest` with zeroes if the length of `src` is less than `n`.
// strncpy_fast will not do that.
char *strncpy(char *restrict dest, const char *restrict src, usize n);
char *strncpy_fast(char *restrict dest, const char *restrict src, usize n);

WARN_RESULT int strncmp(const char *s1, const char *s2, usize n);

WARN_RESULT usize strlen(const char *s);
