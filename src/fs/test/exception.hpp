#pragma once

#include <cstdio>

#include <exception>
#include <string>

#include <cstdio>

static inline void backtrace() {
    printf("0\n%p\n", __builtin_return_address(0));
    printf("1\n%p\n", __builtin_return_address(1));
    printf("2\n%p\n", __builtin_return_address(2));
    printf("3\n%p\n", __builtin_return_address(3));
    printf("4\n%p\n", __builtin_return_address(4));
}

struct Exception : public std::exception {
    std::string message;

    Exception(const std::string &_message) : message(_message) {}

    const char *what() const noexcept override {
        return message.data();
    }
};

struct Internal final : Exception {
    using Exception::Exception;
    virtual ~Internal() = default;
};

struct Panic final : Exception {
    using Exception::Exception;
    virtual ~Panic() = default;
};

struct AssertionFailure final : Exception {
    using Exception::Exception;
    virtual ~AssertionFailure() = default;
};

struct Offline final : Exception {
    using Exception::Exception;
    virtual ~Offline() = default;
};
