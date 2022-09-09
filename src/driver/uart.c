#include <aarch64/intrinsic.h>
#include <driver/aux.h>
#include <driver/gpio.h>
// #include <driver/interrupt.h>
#include <driver/uart.h>
#include <kernel/init.h>

define_early_init(uart) {
    device_put_u32(GPPUD, 0);
    delay_us(5);
    device_put_u32(GPPUDCLK0, (1 << 14) | (1 << 15));
    delay_us(5);
    device_put_u32(GPPUDCLK0, 0);

    // enable mini uart and access to its registers.
    device_put_u32(AUX_ENABLES, 1);
    // disable auto flow control, receiver and transmitter (for now).
    device_put_u32(AUX_MU_CNTL_REG, 0);
    // enable receiving interrupts.
    device_put_u32(AUX_MU_IER_REG, 3 << 2 | 1);
    // enable 8-bit mode.
    device_put_u32(AUX_MU_LCR_REG, 3);
    // set RTS line to always high.
    device_put_u32(AUX_MU_MCR_REG, 0);
    // set baud rate to 115200.
    device_put_u32(AUX_MU_BAUD_REG, AUX_MU_BAUD(115200));
    // clear receive and transmit FIFO.
    device_put_u32(AUX_MU_IIR_REG, 6);
    // finally, enable receiver and transmitter.
    device_put_u32(AUX_MU_CNTL_REG, 3);

    // set_interrupt_handler(IRQ_AUX, uart_intr);
}

char uart_get_char() {
    u32 state = device_get_u32(AUX_MU_IIR_REG);
    if ((state & 1) || (state & 6) != 4)
        return (char)-1;

    return device_get_u32(AUX_MU_IO_REG) & 0xff;
}

void uart_put_char(char c) {
    while (!(device_get_u32(AUX_MU_LSR_REG) & 0x20)) {}

    device_put_u32(AUX_MU_IO_REG, c);

    // fix Windows's '\r'.
    if (c == '\n')
        uart_put_char('\r');
}

__attribute__((weak, alias("uart_put_char"))) void putch(char);
