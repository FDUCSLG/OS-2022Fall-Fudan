#include <kernel/syscallno.h>

.global loop_start
.global loop_end

.align 12
loop_start:
    mov x4, x0
    mov x8, #SYS_myreport
delay:
    mov x0, #10000
    mov x1, #0
    mov x2, #1
loop:
    add x1, x1, x2
    cmp x0, x1
    bne loop
    mov x0, x4
    mov x1, #0
    svc #0
    cmp x0, x1
    beq delay
    mov x0, #100
    svc #0

.align 12
loop_end:
