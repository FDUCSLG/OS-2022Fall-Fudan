/* See https://github.com/raspberrypi/firmware/wiki. */
#include <driver/base.h>
#include <driver/mbox.h>

#include <aarch64/intrinsic.h>
#include <aarch64/mmu.h>
#include <common/defines.h>
#include <driver/memlayout.h>
#include <kernel/printk.h>

#define VIDEOCORE_MBOX (MMIO_BASE + 0x0000B880)
#define MBOX_READ ((volatile unsigned int*)(VIDEOCORE_MBOX + 0x00))
#define MBOX_POLL ((volatile unsigned int*)(VIDEOCORE_MBOX + 0x10))
#define MBOX_SENDER ((volatile unsigned int*)(VIDEOCORE_MBOX + 0x14))
#define MBOX_STATUS ((volatile unsigned int*)(VIDEOCORE_MBOX + 0x18))
#define MBOX_CONFIG ((volatile unsigned int*)(VIDEOCORE_MBOX + 0x1C))
#define MBOX_WRITE ((volatile unsigned int*)(VIDEOCORE_MBOX + 0x20))
#define MBOX_RESPONSE 0x80000000
#define MBOX_FULL 0x80000000
#define MBOX_EMPTY 0x40000000

#define MBOX_TAG_GET_ARM_MEMORY 0x00010005
#define MBOX_TAG_GET_CLOCK_RATE 0x00030002
#define MBOX_TAG_END 0x0
#define MBOX_TAG_REQUEST

int mbox_read(u8 chan) {
    while (1) {
        arch_dsb_sy();
        while (*MBOX_STATUS & MBOX_EMPTY)
            ;
        arch_dsb_sy();
        u32 r = *MBOX_READ;
        if ((r & 0xF) == chan) {
            printk("- mbox_read: 0x%x\n", r);
            return (i32)(r >> 4);
        }
    }
    arch_dsb_sy();
    return 0;
}

void mbox_write(u32 buf, u8 chan) {
    arch_dsb_sy();
    // assert((buf & 0xF) == 0 && (chan & ~0xF) == 0);
    if (!((buf & 0xF) == 0 && (chan & ~0xF) == 0)) {
        PANIC();
    }
    while (*MBOX_STATUS & MBOX_FULL)
        ;
    arch_dsb_sy();
    printk("- mbox write: 0x%x\n", (buf & ~0xFu) | chan);
    *MBOX_WRITE = (buf & ~0xFu) | chan;
    arch_dsb_sy();
}

int mbox_get_arm_memory() {
    __attribute__((aligned(16)))
    u32 buf[] = {36, 0, MBOX_TAG_GET_ARM_MEMORY, 8, 0, 0, 0, MBOX_TAG_END};
    // asserts((K2P(buf) & 0xF) == 0, "Buffer should align to 16 bytes. ");
    if (!((K2P(buf) & 0xF) == 0)) {
        printk("Buffer should align to 16 bytes. \n");
        PANIC();
    }

    arch_dsb_sy();
    arch_dccivac(buf, sizeof(buf));
    arch_dsb_sy();
    mbox_write((u32)K2P(buf), 8);
    arch_dsb_sy();
    mbox_read(8);
    arch_dsb_sy();
    arch_dccivac(buf, sizeof(buf));
    arch_dsb_sy();

    // asserts(buf[5] == 0, "Memory base address should be zero. ");
    if (buf[5] != 0) {
        printk("Memory base address should be zero. \n");
        PANIC();
    }

    // asserts(buf[6] != 0, "Memory size should not be zero. ");
    if (buf[6] == 0) {
        printk("Memory size should not be zero. \n");
        PANIC();
    }
    return (int)buf[6];
}

int mbox_get_clock_rate() {
    __attribute__((aligned(16)))
    u32 buf[] = {36, 0, MBOX_TAG_GET_CLOCK_RATE, 8, 0, 1, 0, MBOX_TAG_END};
    // asserts((K2P(buf) & 0xF) == 0, "Buffer should align to 16 bytes. ");
    if (!((K2P(buf) & 0xF) == 0)) {
        printk("Buffer should align to 16 bytes. \n");
        PANIC();
    }
    arch_dsb_sy();
    arch_dccivac(buf, sizeof(buf));
    arch_dsb_sy();
    mbox_write((u32)K2P(buf), 8);
    arch_dsb_sy();
    mbox_read(8);
    arch_dsb_sy();
    arch_dccivac(buf, sizeof(buf));
    arch_dsb_sy();

    printk("- clock rate %d\n", buf[6]);
    return (int)buf[6];
}
