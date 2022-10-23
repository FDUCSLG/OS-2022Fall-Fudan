extern "C" {
#include <common/defines.h>
}

#include <cstdarg>
#include <cstdint>
#include <cstdio>

#include <sstream>

#include "../exception.hpp"

extern "C" {
void _panic(const char* file, int line) {
    printf("(fatal) ");
    puts("");
    std::stringstream buf;
    buf << file << ":L" << line << ": kernel panic";
    throw Panic(buf.str());
}
}
