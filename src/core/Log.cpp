#include "core/Log.h"

#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <deque>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif

namespace dl::log
{
namespace
{
// WHY: one state object behind a function-local static instead of loose file-level globals (project rule: no file-level globals).
struct State
{
    Level minLevel = Level::Trace;
    std::FILE* file = nullptr;
    std::deque<std::string> recent;
    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
};

State& S()
{
    static State state;
    return state;
}

const char* LevelName(Level level)
{
    switch (level)
    {
    case Level::Trace:
        return "TRACE";
    case Level::Info:
        return "INFO ";
    case Level::Warn:
        return "WARN ";
    case Level::Error:
        return "ERROR";
    }
    return "?????";
}
} // namespace

void SetMinLevel(Level level)
{
    S().minLevel = level;
}

Level MinLevel()
{
    return S().minLevel;
}

bool OpenFile(const char* path)
{
    Close();
#if defined(_WIN32)
    if (fopen_s(&S().file, path, "w") != 0)
    {
        S().file = nullptr;
    }
#else
    S().file = std::fopen(path, "w");
#endif
    return S().file != nullptr;
}

void Close()
{
    if (S().file != nullptr)
    {
        std::fclose(S().file);
        S().file = nullptr;
    }
}

std::string FormatLine(double seconds, Level level, const char* module, const char* message)
{
    char head[64];
    std::snprintf(head, sizeof(head), "[%8.3f] [%s] [%s] ", seconds, LevelName(level), module != nullptr ? module : "-");
    return std::string(head) + (message != nullptr ? message : "");
}

void Write(Level level, const char* module, const char* fmt, ...)
{
    State& s = S();
    if (level < s.minLevel)
    {
        return;
    }
    char message[1024];
    va_list args;
    va_start(args, fmt);
    std::vsnprintf(message, sizeof(message), fmt, args);
    va_end(args);

    const double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - s.start).count();
    const std::string line = FormatLine(seconds, level, module, message);

    s.recent.push_back(line);
    while (s.recent.size() > kRecentCapacity)
    {
        s.recent.pop_front();
    }
    if (s.file != nullptr)
    {
        std::fprintf(s.file, "%s\n", line.c_str());
        std::fflush(s.file); // WHY: a crash must not lose the last lines
    }
#if defined(_WIN32)
    OutputDebugStringA((line + "\n").c_str());
#else
    std::fprintf(stderr, "%s\n", line.c_str());
#endif
}

std::vector<std::string> Recent()
{
    const State& s = S();
    return std::vector<std::string>(s.recent.begin(), s.recent.end());
}

void ResetForTest()
{
    State& s = S();
    s.recent.clear();
    s.start = std::chrono::steady_clock::now();
    s.minLevel = Level::Trace;
}
} // namespace dl::log
