#include <aarch64/intrinsic.h>
#include <driver/base.h>
#include <driver/clock.h>
#include <driver/interrupt.h>
#include <driver/irq.h>
#include <kernel/init.h>
#include <kernel/printk.h>
#include <kernel/sched.h>

static InterruptHandler int_handler[NUM_IRQ_TYPES];

define_early_init(interrupt)
{
    for (usize i = 0; i < NUM_IRQ_TYPES; i++)
    {
        int_handler[i] = NULL;
    }
    // device_put_u32(ENABLE_IRQS_1, AUX_INT);
    // device_put_u32(ENABLE_IRQS_2, VC_ARASANSDIO_INT);
    device_put_u32(GPU_INT_ROUTE, GPU_IRQ2CORE(0));
}

void set_interrupt_handler(InterruptType type, InterruptHandler handler)
{
    device_put_u32(ENABLE_IRQS_1 + 4 * (type / 32), 1u << (type % 32));
    int_handler[type] = handler;
}

void interrupt_global_handler()
{
    u32 source = device_get_u32(IRQ_SRC_CORE(cpuid()));

    if (source & IRQ_SRC_CNTPNSIRQ)
    {
        source ^= IRQ_SRC_CNTPNSIRQ;
        invoke_clock_handler();
    }

    if (source & IRQ_SRC_GPU)
    {
        source ^= IRQ_SRC_GPU;
        u64 map = device_get_u32(IRQ_PENDING_1) | (((u64)device_get_u32(IRQ_PENDING_2)) << 32);
        for (usize i = 0; i < NUM_IRQ_TYPES; i++)
        {
            if ((map >> i) & 1) 
            {
                if (int_handler[i])
                {
                    int_handler[i]();
                }
                else
                {
                    printk("Unknown interrupt type %lld", i);
                    PANIC();
                }
            }
        }
    }

    if (source != 0)
    {
        printk("unknown interrupt sources: %x", source);
        PANIC();
    }
}
