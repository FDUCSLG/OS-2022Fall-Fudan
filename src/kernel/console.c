#include<kernel/console.h>
#include<kernel/init.h>
#include<aarch64/intrinsic.h>
#include<kernel/sched.h>
#include<driver/uart.h>
#define INPUT_BUF 128
struct {
    char buf[INPUT_BUF];
    usize r;  // Read index
    usize w;  // Write index
    usize e;  // Edit index
} input;
#define C(x)      ((x) - '@')  // Control-x


isize console_write(Inode *ip, char *buf, isize n) {
    // TODO
}

isize console_read(Inode *ip, char *dst, isize n) {
    // TODO
}

void console_intr(char (*getc)()) {
    // TODO
}
