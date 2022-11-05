#pragma once

extern "C" {
#include "common/defines.h"
}

struct MockLockConfig {
    static constexpr bool SpinLockBlocksCPU = true;
    static constexpr bool SpinLockForbidsWait = true;
    static constexpr int WaitTimeoutSeconds = 10;
};

extern MockLockConfig mock_lock;
