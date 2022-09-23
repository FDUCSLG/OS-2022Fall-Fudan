#include <kernel/cpu.h>
#include <kernel/printk.h>
#include <kernel/init.h>
#include <driver/clock.h>
#include <kernel/sched.h>
#include <kernel/proc.h>
#include <aarch64/mmu.h>

struct cpu cpus[NCPU];

static void cpu_clock_handler() {
    // TODO:?
}

define_early_init(clock_handler) {
    set_clock_handler(&cpu_clock_handler);
}

void set_cpu_on() {
    ASSERT(!_arch_disable_trap());
    // disable the lower-half address to prevent stupid errors
    extern PTEntries invalid_pt;
    arch_set_ttbr0(K2P(&invalid_pt));
    extern char exception_vector[];
    arch_set_vbar(exception_vector);
    arch_reset_esr();
    init_clock();
    cpus[cpuid()].online = true;
    printk("CPU %d: hello\n", cpuid());
}

void set_cpu_off() {
    _arch_disable_trap();
    cpus[cpuid()].online = false;
    printk("CPU %d: stopped\n", cpuid());
}
