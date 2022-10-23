#include <thread>

extern "C" {
void yield() {
    std::this_thread::yield();
}
}
