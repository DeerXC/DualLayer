// check.h - the CHECK macro and its two counters, shared by the logic tests (each test is a program of its own).
// Use:   #include "check.h", call CHECK(condition) as often as needed, print the two counters at the end and return 1 when
//        g_failed is not 0 (tests/run_tests.ps1 and tools/check.ps1 read the line "N checks passed, M failed" and the
//        exit code).
// Plain C++, no Windows: the tests build with MSVC and with g++.
#pragma once

#include <cstdio>

inline int g_passed = 0; ///< how many checks held
inline int g_failed = 0; ///< how many did not; every one of them has printed its line

#define CHECK(cond)                                                                                                    \
    do                                                                                                                 \
    {                                                                                                                  \
        if (cond)                                                                                                      \
        {                                                                                                              \
            ++g_passed;                                                                                                \
        }                                                                                                              \
        else                                                                                                           \
        {                                                                                                              \
            ++g_failed;                                                                                                \
            std::printf("FAIL line %d: %s\n", __LINE__, #cond);                                                        \
        }                                                                                                              \
    } while (0)
