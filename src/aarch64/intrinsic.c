#include <aarch64/intrinsic.h>

void delay_us(u64 n) {
    u64 freq = get_clock_frequency();
    u64 end = get_timestamp(), now;
    end += freq / 1000000 * n;

    do {
        now = get_timestamp();
    } while (now <= end);
}
