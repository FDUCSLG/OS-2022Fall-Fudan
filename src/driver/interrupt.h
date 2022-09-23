#pragma once

/* GPU Routed IRQs */
#define NUM_IRQ_TYPES 64

typedef enum {
    IRQ_AUX = 29,
    IRQ_GPIO0 = 49,
    IRQ_SDIO = 56,
    IRQ_ARASANSDIO = 62,
} InterruptType;

typedef void (*InterruptHandler)();

void interrupt_global_handler();
void set_interrupt_handler(InterruptType type, InterruptHandler handler);
