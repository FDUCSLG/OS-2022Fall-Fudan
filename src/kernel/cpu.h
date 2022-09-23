#pragma once

#include <kernel/schinfo.h>

#define NCPU 4

struct cpu
{
    bool online;
    struct sched sched;
};

extern struct cpu cpus[NCPU];

void set_cpu_on();
void set_cpu_off();
