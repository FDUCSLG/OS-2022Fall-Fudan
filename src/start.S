/* SCR_EL3, Secure Configuration Register (EL3). */
#define SCR_RESERVED (3 << 4)
#define SCR_RW       (1 << 10)
#define SCR_HCE      (1 << 8)
#define SCR_SMD      (1 << 7)
#define SCR_NS       (1 << 0)
#define SCR_VALUE    (SCR_RESERVED | SCR_RW | SCR_HCE | SCR_SMD | SCR_NS)

/* SPSR_EL1/2/3, Saved Program Status Register. */
#define SPSR_MASK_ALL  (7 << 6)
#define SPSR_EL1h      (5 << 0)
#define SPSR_EL2h      (9 << 0)
#define SPSR_EL3_VALUE (SPSR_MASK_ALL | SPSR_EL2h)
#define SPSR_EL2_VALUE (SPSR_MASK_ALL | SPSR_EL1h)

/* HCR_EL2, Hypervisor Configuration Register (EL2). */
#define HCR_RW    (1 << 31)
#define HCR_VALUE HCR_RW

/* SCTLR_EL1, System Control Register (EL1). */
#define SCTLR_RESERVED           ((3 << 28) | (3 << 22) | (1 << 20) | (1 << 11) | (1 << 8) | (1 << 7))
#define SCTLR_EE_LITTLE_ENDIAN   (0 << 25)
#define SCTLR_E0E_LITTLE_ENDIAN  (0 << 24)
#define SCTLR_I_CACHE            (1 << 12)
#define SCTLR_D_CACHE            (1 << 2)
#define SCTLR_MMU_DISABLED       (0 << 0)
#define SCTLR_MMU_ENABLED        (1 << 0)
#define SCTLR_VALUE_MMU_DISABLED (SCTLR_RESERVED | SCTLR_EE_LITTLE_ENDIAN | SCTLR_E0E_LITTLE_ENDIAN | \
                                  SCTLR_I_CACHE | SCTLR_D_CACHE | SCTLR_MMU_DISABLED)

/* CPACR_EL1, Architectural Feature Access Control Register. */
#define CPACR_FP_EN    (3 << 20)
#define CPACR_TRACE_EN (0 << 28)
#define CPACR_VALUE    (CPACR_FP_EN | CPACR_TRACE_EN)

/* Translation Control Register */
#define TCR_T0SZ        (64 - 48)
#define TCR_T1SZ        ((64 - 48) << 16)
#define TCR_TG0_4K      (0 << 14)
#define TCR_TG1_4K      (2 << 30)
#define TCR_SH0_INNER   (3 << 12)
#define TCR_SH1_INNER   (3 << 28)
#define TCR_SH0_OUTER   (2 << 12)
#define TCR_SH1_OUTER   (2 << 28)
#define TCR_ORGN0_IRGN0 ((1 << 10) | (1 << 8))
#define TCR_ORGN1_IRGN1 ((1 << 26) | (1 << 24))
#define TCR_VALUE       (TCR_T0SZ | TCR_T1SZ | TCR_TG0_4K | TCR_TG1_4K | \
                         TCR_SH0_OUTER | TCR_SH1_OUTER | TCR_ORGN0_IRGN0 | TCR_ORGN1_IRGN1)

/* memory region attributes */
#define MT_DEVICE_nGnRnE       0x0
#define MT_NORMAL              0x1
#define MT_NORMAL_NC           0x2
#define MT_DEVICE_nGnRnE_FLAGS 0x00
#define MT_NORMAL_FLAGS        0xFF  /* Inner/Outer Write-back Non-transient RW-Allocate */
#define MT_NORMAL_NC_FLAGS     0x44  /* Inner/Outer Non-cacheable */
#define MAIR_VALUE             ((MT_DEVICE_nGnRnE_FLAGS << (8 * MT_DEVICE_nGnRnE)) | \
                                (MT_NORMAL_FLAGS << (8 * MT_NORMAL)) | \
                                (MT_NORMAL_NC_FLAGS << (8 * MT_NORMAL_NC)))

/* kernel stack size per CPU. Kernel stack sizes are fixed. */
#define KERNEL_STACK_SIZE 4096

.section ".text.boot"

.global _start
_start:
    /**
     * wake up other CPUs.
     * see <https://github.com/raspberrypi/tools/blob/master/armstubs/armstub8.S>.
     * which has been incorporated in `start.elf` (or `bootcode.bin`?).
     */
    mov x15, x0
    adr     x0, entry
    mov     x1, #1
    mov     x2, #2
    mov     x3, #3
    mov     x9, 0xd8
    str     x0, [x9, x1, lsl #3]
    str     x0, [x9, x2, lsl #3]
    str     x0, [x9, x3, lsl #3]

    dsb     sy
    isb

    sev

entry:
    /* get current exception level from CurrentEL[3:2]. */
    mrs     x9, CurrentEL
    and     x9, x9, #0xc
    cmp     x9, #8
    beq     el2
    bgt     el3

el3:
    mov     x9, #SCR_VALUE
    msr     scr_el3, x9
    mov     x9, #SPSR_EL3_VALUE
    msr     spsr_el3, x9
    adr     x9, el2
    msr     elr_el3, x9
    eret

el2:
    /**
     * HCR_EL2.RW, bit[31] = 1: the execution state for EL1 is AArch64.
     * the execution state for EL0 is determined by the current
     * value of PSTATE.nRW when executing at EL0.
     */
    mov     x9, #HCR_VALUE
    msr     hcr_el2, x9

    /* setup SCTLR access. */
    ldr     x9, =SCTLR_VALUE_MMU_DISABLED
    msr     sctlr_el1, x9

    /* enable SIMD instructions. */
    ldr     x9, =CPACR_VALUE
    msr     cpacr_el1, x9

    /* change execution level to EL1. */
    mov     x9, #SPSR_EL2_VALUE
    msr     spsr_el2, x9
    adr     x9, el1
    msr     elr_el2, x9
    eret

el1:
    adr     x9, kernel_pt

    /* higher and lower half map to same physical memory region. */
    msr     ttbr0_el1, x9
    msr     ttbr1_el1, x9

    ldr     x9, =TCR_VALUE
    msr     tcr_el1, x9

    ldr     x9, =MAIR_VALUE
    msr     mair_el1, x9

    isb

    /* enable MMU. */
    mrs     x9, sctlr_el1
    orr     x9, x9, #SCTLR_MMU_ENABLED
    msr     sctlr_el1, x9

    isb

    /* read CPUID. */
    mrs     x0, mpidr_el1
    and     x0, x0, #3

    /* sp = _start - CPUID * KERNEL_STACK_SIZE */
    mov     x10, #KERNEL_STACK_SIZE
    mul     x10, x10, x0
    ldr     x9, =_start
    sub     x9, x9, x10

    /* set up stack pointer, must always be aligned to 16 bytes. */
    msr     spsel, #1
    mov     sp, x9

    mov x0, x15
    ldr     x9, =main
    br      x9
