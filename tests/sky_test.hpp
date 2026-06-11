#pragma once

// Minimal assertion harness for Sky Engine unit tests: CHECK records
// failures and the process exit code reports them to CTest.

#include <cstdio>

namespace sky::test {

inline int failures = 0;
inline int checks = 0;

inline int summary(const char* suite) {
    std::printf("%s: %d checks, %d failures\n", suite, checks, failures);
    return failures == 0 ? 0 : 1;
}

} // namespace sky::test

#define CHECK(condition)                                                      \
    do {                                                                      \
        ++sky::test::checks;                                                  \
        if (!(condition)) {                                                   \
            ++sky::test::failures;                                            \
            std::printf("FAILED %s:%d: %s\n", __FILE__, __LINE__, #condition); \
        }                                                                     \
    } while (false)
