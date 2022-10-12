#include <kernel/cpu.h>
#include <kernel/printk.h>
#include <kernel/init.h>
#include <driver/clock.h>
#include <kernel/sched.h>
#include <kernel/proc.h>
#include <aarch64/mmu.h>

struct cpu cpus[NCPU];

static bool __timer_cmp(rb_node lnode, rb_node rnode)
{
    return container_of(lnode, struct timer, _node)->_key < container_of(rnode, struct timer, _node)->_key;
}

static void __timer_set_clock()
{
    auto node = _rb_first(&cpus[cpuid()].timer);
    if (!node)
    {
        reset_clock(1000);
        return;
    }
    auto t1 = container_of(node, struct timer, _node)->_key;
    auto t0 = get_timestamp_ms();
    if (t1 <= t0)
        reset_clock(0);
    else
        reset_clock(t1 - t0);
}

static void timer_clock_handler() {
    reset_clock(1000);
    // printk("cpu %d aha\n", cpuid());
    while (1)
    {
        auto node = _rb_first(&cpus[cpuid()].timer);
        if (!node)
            break;
        auto timer = container_of(node, struct timer, _node);
        if (get_timestamp_ms() < timer->_key)
            break;
        cancel_cpu_timer(timer);
        timer->triggered = true;
        timer->handler(timer);
    }
}

define_early_init(clock_handler) {
    set_clock_handler(&timer_clock_handler);
}

void set_cpu_timer(struct timer* timer)
{
    timer->triggered = false;
    timer->_key = get_timestamp_ms() + timer->elapse;
    _rb_insert(&timer->_node, &cpus[cpuid()].timer, __timer_cmp);
    __timer_set_clock();
}

void cancel_cpu_timer(struct timer* timer)
{
    ASSERT(!timer->triggered);
    _rb_erase(&timer->_node, &cpus[cpuid()].timer);
    __timer_set_clock();
}

static struct timer hello_timer[4];
static void hello(struct timer* t)
{
    printk("CPU %d: living\n", cpuid());
    t->data++;
    set_cpu_timer(&hello_timer[cpuid()]);
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
    hello_timer[cpuid()].elapse = 5000;
    hello_timer[cpuid()].handler = hello;
    set_cpu_timer(&hello_timer[cpuid()]);
}

void set_cpu_off() {
    _arch_disable_trap();
    cpus[cpuid()].online = false;
    printk("CPU %d: stopped\n", cpuid());
}
