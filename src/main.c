#include <aarch64/intrinsic.h>

static char hello[16];

NO_RETURN void main()
{
    arch_stop_cpu();
}
