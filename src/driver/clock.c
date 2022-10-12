#include <aarch64/intrinsic.h>
#include <kernel/sched.h>
#include <driver/base.h>
#include <driver/clock.h>

#define CORE_CLOCK_CTRL(id) (LOCAL_BASE + 0x40 + 4 * (id))
#define CORE_CLOCK_ENABLE   (1 << 1)

static struct {
    u64 one_ms;
    ClockHandler handler;
} clock;

void init_clock()
{
    clock.one_ms = get_clock_frequency() / 1000;

    // reserve one second for the first time.
    asm volatile("msr cntp_ctl_el0, %[x]" ::[x] "r"(1ll));
    reset_clock(1000);

    device_put_u32(CORE_CLOCK_CTRL(cpuid()), CORE_CLOCK_ENABLE);
}

u64 get_timestamp_ms()
{
    return get_timestamp() / clock.one_ms;
}

void reset_clock(u64 countdown_ms)
{
    u64 t = countdown_ms * clock.one_ms;
    ASSERT(t <= 0x7fffffff);
    asm volatile("msr cntp_tval_el0, %[x]" ::[x] "r"(t));
}

void set_clock_handler(ClockHandler handler)
{
    clock.handler = handler;
}

void invoke_clock_handler()
{
    if (!clock.handler)
        PANIC();
    clock.handler();
}
