// Assert.h - DL_ASSERT for programmer errors.
// Debug:   logs, then breaks into the debugger.
// Release: logs and continues (the caller must still take a safe path: an assert is not error handling).
// Recoverable failures (missing file, lost device) are NOT asserts: return a result and log it.
#pragma once

namespace dl::assertion
{
using Handler = void (*)(const char* file, int line, const char* expr, const char* message);

/// Tests only: replaces the default behaviour (no log, no break).
void SetHandler(Handler handler);

/// Returns true if the caller should break into the debugger.
bool Fail(const char* file, int line, const char* expr, const char* message);
} // namespace dl::assertion

#if defined(_MSC_VER)
#define DL_DEBUG_BREAK() __debugbreak()
#else
#define DL_DEBUG_BREAK() ((void)0)
#endif

#define DL_ASSERT(expr, message)                                                                                       \
    do                                                                                                                 \
    {                                                                                                                  \
        if (!(expr))                                                                                                   \
        {                                                                                                              \
            if (::dl::assertion::Fail(__FILE__, __LINE__, #expr, message))                                             \
            {                                                                                                          \
                DL_DEBUG_BREAK();                                                                                      \
            }                                                                                                          \
        }                                                                                                              \
    } while (0)
