// test_core.cpp - tests of the parts of src/core that need no window and no GPU.
//   g++ -std=c++20 -Wall -Wextra -I src tests/test_core.cpp src/core/Config.cpp src/core/Log.cpp src/core/Assert.cpp -o
//   test_core MSVC (Developer PowerShell):  cl /nologo /std:c++20 /EHsc /W4 /I src tests\test_core.cpp
//   src\core\Config.cpp src\core\Log.cpp src\core\Assert.cpp
#include "core/Assert.h"
#include "core/Config.h"
#include "core/Log.h"

#include <cstdio>
#include <string>

namespace
{
int g_failed = 0;
int g_passed = 0;
int g_assertCalls = 0;

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

void CountingHandler(const char*, int, const char*, const char*)
{
    ++g_assertCalls;
}

void TestConfig()
{
    dl::Config c;
    const auto r = c.ParseString("# comment\n"
                                 "[Window]\n"
                                 "width = 1600   # pixels\n"
                                 "height=900\n"
                                 "\n"
                                 "[render]\n"
                                 "vsync = Yes\n"
                                 "clear_r = 0.25\n"
                                 "name = a#b\n"
                                 "bad_number = 12abc\n"
                                 "adapter = -1\r\n"
                                 "[unterminated\n"
                                 "no equals sign here\n"
                                 " = nokey\n");
    CHECK(r.entries == 7);
    CHECK(r.errors.size() == 3);
    CHECK(r.errors.size() == 3 && r.errors[0] == "line 12: section is missing ']'");
    CHECK(c.GetInt("window", "width", 0) == 1600); // inline comment removed, section and key are case-insensitive
    CHECK(c.GetInt("WINDOW", "HEIGHT", 0) == 900);
    CHECK(c.GetBool("render", "vsync", false) == true);
    CHECK(c.GetFloat("render", "clear_r", 0.0f) == 0.25f);
    CHECK(c.GetString("render", "name", "") == "a#b"); // '#' without whitespace before it is not a comment
    CHECK(c.GetInt("render", "bad_number", 7) == 7);   // malformed: the default
    CHECK(c.GetInt("render", "adapter", 0) == -1);     // CRLF and a negative number
    CHECK(c.GetInt("render", "missing", 42) == 42);
    CHECK(!c.Has("render", "missing") && c.Has("render", "vsync"));
    CHECK(c.GetBool("render", "bad_number", true) == true); // not a boolean: the default

    // a failed load reports the problem and keeps nothing half-read
    dl::Config d;
    const auto bad = d.LoadFile("this/file/does/not/exist.ini");
    CHECK(bad.errors.size() == 1 && bad.entries == 0);
    CHECK(d.GetInt("a", "b", 3) == 3);

    // a second parse replaces the first (this is what F5 does)
    c.ParseString("[window]\nwidth = 800\n");
    CHECK(c.GetInt("window", "width", 0) == 800 && !c.Has("window", "height"));
}

void TestLog()
{
    dl::log::ResetForTest();
    CHECK(dl::log::FormatLine(12.3456, dl::log::Level::Warn, "Device", "hello") == "[  12.346] [WARN ] [Device] hello");
    CHECK(dl::log::FormatLine(0.0, dl::log::Level::Info, nullptr, "x") == "[   0.000] [INFO ] [-] x");

    dl::log::SetMinLevel(dl::log::Level::Warn);
    DL_LOG_INFO("T", "dropped %d", 1);
    DL_LOG_WARN("T", "kept %d", 2);
    const auto lines = dl::log::Recent();
    CHECK(lines.size() == 1);
    CHECK(!lines.empty() && lines[0].find("[WARN ] [T] kept 2") != std::string::npos);

    dl::log::SetMinLevel(dl::log::Level::Trace);
    for (int i = 0; i < 400; ++i)
    {
        DL_LOG_TRACE("T", "line %d", i);
    }
    const auto recent = dl::log::Recent();
    CHECK(recent.size() == dl::log::kRecentCapacity);            // the ring is bounded
    CHECK(recent.back().find("line 399") != std::string::npos);  // newest last
    CHECK(recent.front().find("line 144") != std::string::npos); // 400 - 256 = 144 is the oldest kept
    dl::log::ResetForTest();
}

void TestAssert()
{
    dl::assertion::SetHandler(CountingHandler);
    g_assertCalls = 0;
    DL_ASSERT(1 + 1 == 2, "true conditions do nothing");
    CHECK(g_assertCalls == 0);
    DL_ASSERT(1 + 1 == 3, "false conditions call the handler once");
    CHECK(g_assertCalls == 1);
    dl::assertion::SetHandler(nullptr);
}
} // namespace

int main()
{
    TestConfig();
    TestLog();
    TestAssert();
    std::printf("%d checks passed, %d failed\n", g_passed, g_failed);
    return g_failed == 0 ? 0 : 1;
}
