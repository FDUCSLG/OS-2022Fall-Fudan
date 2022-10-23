#pragma once

#include <csignal>
#include <cstdio>

#include <atomic>
#include <unistd.h>

#define PAUSE                                                                                      \
    { Pause().pause(); }

class Pause {
public:
    __attribute__((__noinline__, optimize(3))) void pause() {
        int pid = getpid();
        printf("(debug) process %d paused.\n", pid);
        raise(SIGTSTP);
    }
};
