#pragma once

#include <common/defines.h>

#define ESR_EC_SHIFT 26
#define ESR_ISS_MASK 0xFFFFFF
#define ESR_IR_MASK  (1 << 25)

#define ESR_EC_UNKNOWN 0x00
#define ESR_EC_SVC64   0x15
#define ESR_EC_IABORT_EL0  0x20
#define ESR_EC_IABORT_EL1  0x21
#define ESR_EC_DABORT_EL0  0x24
#define ESR_EC_DABORT_EL1  0x25
