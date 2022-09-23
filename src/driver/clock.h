#pragma once

#include <common/defines.h>

typedef void (*ClockHandler)(void);

void init_clock();
void reset_clock(u64 countdown_ms);
void set_clock_handler(ClockHandler handler);
void invoke_clock_handler();
