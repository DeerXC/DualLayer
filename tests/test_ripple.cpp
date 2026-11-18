// test_ripple.cpp - tests of the timing of the layer switch (no window, no GPU): src/render/LayerRipple.h.
//   g++ -std=c++20 -Wall -Wextra -I src tests/test_ripple.cpp -o test_ripple
//   MSVC (Developer PowerShell):  .\tests\run_tests.ps1
// Header-only: there is no .cpp to add to the project. What is checked:
//   1. idle -> open -> done: the other layer becomes the outer one, and the circle is closed again
//   2. Tab during a ripple turns it round: the opening of the circle does not jump, and the old layer is kept
//   3. turning round several times ends on the layer of the last press
//   4. seconds = 0 is a cut; setting it to 0 while a ripple runs finishes the ripple at once
//   5. the opening only grows while it opens, and stays in 0 .. 1
//   6. the farthest-corner distance (the radius at which the circle covers the whole screen)
// Returns 1 when a check fails.

#include "render/LayerRipple.h"

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

bool Near(float a, float b, float eps = 1e-4f)
{
    return std::fabs(a - b) <= eps;
}

void TestOpenAndDone()
{
    dl::LayerRipple r;
    r.SetSeconds(1.0f);
    CHECK(!r.Active() && r.Outer() == 0 && r.Target() == 0 && Near(r.Opened(), 0.0f));
    r.Toggle();
    CHECK(r.Active() && r.Outer() == 0 && r.Inner() == 1 && r.Target() == 1);
    r.Update(0.5f);
    CHECK(r.Active() && r.Opened() > 0.8f && r.Opened() < 0.9f); // eased: fast at the start (1 - 0.5^3 = 0.875)
    r.Update(0.6f);
    CHECK(!r.Active() && r.Outer() == 1 && r.Target() == 1 && Near(r.Opened(), 0.0f));
    r.Toggle(); // and back
    CHECK(r.Target() == 0);
    r.Update(1.1f);
    CHECK(!r.Active() && r.Outer() == 0);
    r.Update(1.0f); // an idle ripple ignores the clock
    CHECK(!r.Active() && r.Outer() == 0);
}

void TestTurnRound()
{
    dl::LayerRipple r;
    r.SetSeconds(1.0f);
    r.Toggle();
    r.Update(0.4f);
    const float before = r.Opened();
    r.Toggle(); // press again: close
    CHECK(r.Active() && r.Target() == 0);   // heading back to A
    CHECK(Near(r.Opened(), before));        // no jump at the moment of the press
    r.Update(0.2f);
    CHECK(r.Opened() < before);             // it closes
    r.Update(0.5f);
    CHECK(!r.Active() && r.Outer() == 0);   // closed: the old layer is still the one on the screen
    // several presses: the last one decides
    r.Toggle();
    r.Update(0.3f);
    r.Toggle();
    r.Update(0.1f);
    r.Toggle();
    CHECK(r.Target() == 1);
    r.Update(2.0f);
    CHECK(!r.Active() && r.Outer() == 1);
}

void TestCut()
{
    dl::LayerRipple r;
    r.SetSeconds(0.0f);
    r.Toggle();
    CHECK(!r.Active() && r.Outer() == 1);
    r.Toggle();
    CHECK(!r.Active() && r.Outer() == 0);
    // the time is set to 0 while a ripple runs: it finishes on the next update
    dl::LayerRipple s;
    s.SetSeconds(1.0f);
    s.Toggle();
    s.Update(0.3f);
    s.SetSeconds(0.0f);
    s.Update(0.016f);
    CHECK(!s.Active() && s.Outer() == 1);
    // the time is limited
    s.SetSeconds(-3.0f);
    CHECK(Near(s.Seconds(), 0.0f));
    s.SetSeconds(1000.0f);
    CHECK(Near(s.Seconds(), 10.0f));
}

void TestMonotonic()
{
    // while it opens, the circle only grows and stays in 0 .. 1; when it is done the circle is closed again (idle)
    dl::LayerRipple r;
    r.SetSeconds(1.0f);
    r.Toggle();
    float last = -1.0f;
    int steps = 0;
    while (r.Active() && steps < 100)
    {
        r.Update(0.05f);
        if (r.Active())
        {
            CHECK(r.Opened() >= last && r.Opened() >= 0.0f && r.Opened() <= 1.0f);
            last = r.Opened();
        }
        ++steps;
    }
    CHECK(!r.Active() && steps >= 19 && steps <= 22 && r.Outer() == 1); // about 1 s / 0.05 s
    CHECK(last > 0.99f);                                                // it was almost fully open before it finished
}

void TestFarthestCorner()
{
    // the middle of a 16 : 9 screen: the corner is sqrt((0.5 * 16/9)^2 + 0.5^2) screen heights away
    const float aspect = 16.0f / 9.0f;
    const float expected = std::sqrt((0.5f * aspect) * (0.5f * aspect) + 0.25f);
    CHECK(Near(dl::FarthestCornerDistance(0.5f, 0.5f, aspect), expected));
    // the top-left corner of a 2 : 1 screen: the far corner is the bottom right, sqrt(2^2 + 1^2)
    CHECK(Near(dl::FarthestCornerDistance(0.0f, 0.0f, 2.0f), std::sqrt(5.0f), 1e-5f));
    CHECK(Near(dl::FarthestCornerDistance(1.0f, 1.0f, 2.0f), std::sqrt(5.0f), 1e-5f));
}

} // namespace

int main()
{
    TestOpenAndDone();
    TestTurnRound();
    TestCut();
    TestMonotonic();
    TestFarthestCorner();
    std::printf("%d checks passed, %d failed\n", g_passed, g_failed);
    return g_failed == 0 ? 0 : 1;
}
