// test_layer_guard.cpp - tests of the guard of the layer switch (no window, no GPU): src/scene/LayerGuard.h.
//   g++ -std=c++20 -O2 -Wall -Wextra -I src tests/test_layer_guard.cpp -o test_layer_guard
//   MSVC (Developer PowerShell):  .\tests\run_tests.ps1 -Only layer_guard
// What is checked: a switch that fits is done whatever the ripple does; one that does not fit is refused, the old layer
// is held, the ripple is turned back once after kRefuseSeconds, and requests during a refusal or during a ripple are
// ignored. Returns 1 when a check fails.

#include "scene/LayerGuard.h"

#include <cmath>
#include <cstdio>

namespace
{

int g_failed = 0;
int g_passed = 0;

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

bool Near(float a, float b, float eps)
{
    return std::fabs(a - b) <= eps;
}

void TestGuard()
{
    {
        dl::LayerGuard g; // fits: switch, whatever the ripple does
        CHECK(g.OnSwitchRequest(0, false, true) == dl::LayerGuard::Action::Switch);
        CHECK(g.OnSwitchRequest(0, true, true) == dl::LayerGuard::Action::Switch);
        CHECK(!g.Refusing() && g.Refusals() == 0);
    }
    {
        dl::LayerGuard g; // does not fit: refuse, hold the old layer, turn the ripple back once after 0.15 s
        CHECK(g.OnSwitchRequest(1, false, false) == dl::LayerGuard::Action::Refuse);
        CHECK(g.Refusing() && g.HeldLayer() == 1 && g.Refusals() == 1);
        CHECK(g.OnSwitchRequest(1, true, true) == dl::LayerGuard::Action::Ignore); // nothing else while it plays
        int turns = 0;
        float t = 0.0f;
        float turnedAt = -1.0f;
        for (int i = 0; i < 60; ++i)
        {
            t += 0.01f;
            if (g.Update(0.01f))
            {
                ++turns;
                turnedAt = t;
            }
        }
        CHECK(turns == 1 && Near(turnedAt, dl::LayerGuard::kRefuseSeconds, 0.011f));
        CHECK(!g.Refusing());
        CHECK(g.OnSwitchRequest(1, false, true) == dl::LayerGuard::Action::Switch);
    }
    {
        dl::LayerGuard g; // does not fit while a ripple runs: dropped, no refusal on top of a ripple
        CHECK(g.OnSwitchRequest(0, true, false) == dl::LayerGuard::Action::Ignore);
        CHECK(!g.Refusing() && g.Refusals() == 0);
        CHECK(!g.Update(0.0f) && !g.Update(-1.0f));
    }
}

} // namespace

int main()
{
    TestGuard();
    std::printf("%d checks passed, %d failed\n", g_passed, g_failed);
    return g_failed == 0 ? 0 : 1;
}
