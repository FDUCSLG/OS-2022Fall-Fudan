#pragma once

#include <sstream>

#include "exception.hpp"

[[maybe_unused]] static inline auto
get_source_location(size_t line, const char *file, const char *func) -> std::string {
    std::stringstream buf;
    buf << "'" << func << "' (" << file << ":L" << line << ")";
    return buf.str();
}

template <typename X, typename Y>
void _assert_eq(const X &actual, const Y &expect, const char *expr, const std::string &location) {
    if (actual != static_cast<X>(expect)) {
        std::stringstream buf;
        buf << location << ": ";
        buf << "assert_eq failed: '" << expr << "': expect '" << expect << "', got '" << actual
            << "'";
        throw AssertionFailure(buf.str());
    }
}

#define assert_eq(actual, expect)                                                                  \
    _assert_eq(                                                                                    \
        (actual), (expect), #actual, get_source_location(__LINE__, __FILE__, __PRETTY_FUNCTION__))

template <typename X, typename Y>
void _assert_ne(const X &actual, const Y &expect, const char *expr, const std::string &location) {
    if (actual == static_cast<X>(expect)) {
        std::stringstream buf;
        buf << location << ": ";
        buf << "assert_ne failed: '" << expr << "': expect ≠ '" << expect << "', got '" << actual
            << "'";
        throw AssertionFailure(buf.str());
    }
}

#define assert_ne(actual, expect)                                                                  \
    _assert_ne(                                                                                    \
        (actual), (expect), #actual, get_source_location(__LINE__, __FILE__, __PRETTY_FUNCTION__))

[[maybe_unused]] static inline void
_assert_true(bool predicate, const char *expr, const std::string &location) {
    if (!predicate) {
        std::stringstream buf;
        buf << location << ": ";
        buf << "assert_true failed: '" << expr << "'";
        throw AssertionFailure(buf.str());
    }
}

#define assert_true(expr)                                                                          \
    _assert_true((expr), #expr, get_source_location(__LINE__, __FILE__, __PRETTY_FUNCTION__))
