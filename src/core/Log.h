// Log.h - leveled, module-tagged logging with a file sink, the debugger sink and a ring buffer for the in-game panel.
// Owns:      the open log file, the ring buffer of recent lines.
// Threading: main thread only. Revisit when a worker thread logs.
// Format:    "[  12.345] [INFO ] [Device] message"  (seconds since the first log call)
#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace dl::log
{
enum class Level
{
    Trace,
    Info,
    Warn,
    Error
};

void SetMinLevel(Level level);
Level MinLevel();

/// Opens (truncates) the log file. Returns false if it cannot be opened; logging then continues without a file.
bool OpenFile(const char* path);
void Close();

#if defined(__GNUC__)
void Write(Level level, const char* module, const char* fmt, ...) __attribute__((format(printf, 3, 4)));
#else
void Write(Level level, const char* module, const char* fmt, ...);
#endif

/// The last lines written (oldest first), at most kRecentCapacity. For the in-game debug panel (planned).
std::vector<std::string> Recent();
constexpr std::size_t kRecentCapacity = 256;

/// Pure function, exposed for tests.
std::string FormatLine(double seconds, Level level, const char* module, const char* message);

/// Tests only: forget the ring buffer and restart the clock.
void ResetForTest();
} // namespace dl::log

#define DL_LOG_TRACE(module, ...) ::dl::log::Write(::dl::log::Level::Trace, module, __VA_ARGS__)
#define DL_LOG_INFO(module, ...) ::dl::log::Write(::dl::log::Level::Info, module, __VA_ARGS__)
#define DL_LOG_WARN(module, ...) ::dl::log::Write(::dl::log::Level::Warn, module, __VA_ARGS__)
#define DL_LOG_ERROR(module, ...) ::dl::log::Write(::dl::log::Level::Error, module, __VA_ARGS__)
