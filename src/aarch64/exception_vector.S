#define enter_trap .align 7; b trap_entry
#define trap_error(type) .align 7; mov x0, #(type); b trap_error_handler

.globl exception_vector

.align 12
exception_vector:
el1_sp0:
    trap_error(0)
    trap_error(1)
    trap_error(2)
    trap_error(3)

el1_spx:
    /* if you want to disable in-kernel traps, just replace `enter_trap` with `trap_error` */
    //trap_error(4)
    //trap_error(5)
    enter_trap
    enter_trap
    trap_error(6)
    trap_error(7)

el0_aarch64:
    enter_trap
    enter_trap
    trap_error(10)
    trap_error(11)

el0_aarch32:
    trap_error(12)
    trap_error(13)
    trap_error(14)
    trap_error(15)
