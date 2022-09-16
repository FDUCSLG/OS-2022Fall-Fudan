#include <common/format.h>
#include <common/spinlock.h>
#include <kernel/init.h>
#include <kernel/printk.h>

static SpinLock printk_lock;

define_early_init(printk)
{
    init_spinlock(&printk_lock);
}

static void _put_char(void *_ctx, char c) {
    (void)_ctx;
    putch(c);
}

static void _vprintf(const char *fmt, va_list arg) {
    setup_checker(0);
    acquire_spinlock(0, &printk_lock);
    vformat(_put_char, NULL, fmt, arg);
    release_spinlock(0, &printk_lock);
}

void printk(const char *fmt, ...) {
    va_list arg;
    va_start(arg, fmt);
    _vprintf(fmt, arg);
    va_end(arg);
}