#include <kernel/cpu.h>
#include <kernel/printk.h>
#include <kernel/init.h>
#include <kernel/sched.h>
#include <test/test.h>
// #include <driver/sd.h>

bool panic_flag;

NO_RETURN void idle_entry() {
    set_cpu_on();
    while (1) {
        yield();
        if (panic_flag)
            break;
        arch_with_trap {
            arch_wfi();
        }
    }
    set_cpu_off();
    arch_stop_cpu();
}

NO_RETURN void kernel_entry() {
    printk("hello world %d\n", (int)sizeof(struct proc));

    // proc_test();
    vm_test();
    user_proc_test();
    
    do_rest_init();

    while (1)
        yield();
}

NO_INLINE NO_RETURN void _panic(const char* file, int line) {
    printk("=====%s:%d PANIC%d!=====\n", file, line, cpuid());
    panic_flag = true;
    set_cpu_off();
    for (int i = 0; i < NCPU; i++) {
        if (cpus[i].online)
            i--;
    }
    printk("Kernel PANIC invoked at %s:%d. Stopped.\n", file, line);
    arch_stop_cpu();
}
