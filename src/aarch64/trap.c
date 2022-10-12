#include <aarch64/trap.h>
#include <aarch64/intrinsic.h>
#include <kernel/sched.h>
#include <kernel/printk.h>
#include <driver/interrupt.h>
#include <kernel/proc.h>
#include <kernel/syscall.h>

void trap_global_handler(UserContext* context)
{
    thisproc()->ucontext = context;

    u64 esr = arch_get_esr();
    u64 ec = esr >> ESR_EC_SHIFT;
    u64 iss = esr & ESR_ISS_MASK;
    u64 ir = esr & ESR_IR_MASK;
    (void)iss;
    arch_reset_esr();

    switch (ec)
    {
        case ESR_EC_UNKNOWN:
        {
            if (ir)
            {
                printk("Broken pc?\n");
                PANIC();
            }
            else
                interrupt_global_handler();
        } break;
        case ESR_EC_SVC64:
        {
            syscall_entry(context);
        } break;
        case ESR_EC_IABORT_EL0:
        case ESR_EC_IABORT_EL1:
        case ESR_EC_DABORT_EL0:
        case ESR_EC_DABORT_EL1:
        {
            printk("Page fault %llu\n", ec);
            PANIC();
        } break;
        default:
        {
            printk("Unknwon exception %llu\n", ec);
            PANIC();
        }
    }

    // TODO: stop killed process while returning to user space

}

NO_RETURN void trap_error_handler(u64 type)
{
    printk("Unknown trap type %llu\n", type);
    PANIC();
}
