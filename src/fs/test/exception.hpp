#pragma once

#include <cstdio>

#include <exception>
#include <string>

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
