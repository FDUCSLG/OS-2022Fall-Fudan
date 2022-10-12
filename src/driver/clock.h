#pragma once

#include <common/defines.h>

typedef void (*ClockHandler)(void);

u64 get_timestamp_ms();
void init_clock();
void reset_clock(u64 countdown_ms);
void set_clock_handler(ClockHandler handler);
void invoke_clock_handler();


