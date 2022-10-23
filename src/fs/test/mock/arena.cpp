extern "C" {
#include <common/defines.h>
}

#include "map.hpp"

namespace {
Map<struct Arena*, usize> map;
Map<u8*, u8*> ref;
}  // namespace

extern "C" {

void* kalloc(isize x) {
    return malloc(x);
}

void kfree(void* object) {
    free(object);
}
}
