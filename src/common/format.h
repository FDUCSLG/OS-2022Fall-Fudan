#pragma once

#include <common/variadic.h>

typedef void (*PutCharFunc)(void *ctx, char c);

void vformat(PutCharFunc put_char, void *ctx, const char *fmt, va_list arg);
void format(PutCharFunc put_char, void *ctx, const char *fmt, ...);
