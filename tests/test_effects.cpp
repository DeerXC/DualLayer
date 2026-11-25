// test_effects.cpp - tests of the drawn effects of the town loop (no window, no GPU): src/scene/TownEffects.h.
//   g++ -std=c++20 -O2 -Wall -Wextra -I src tests/test_effects.cpp -o test_effects
//   MSVC (Developer PowerShell):  .\tests\run_tests.ps1 -Only effects
// What is checked:
//   1. the sea: one plane per layer, each in its own layer, at the level asked for, wider than the far plane round any
//   eye,
//      moving in steps (so the pattern of the waves does not slide), drawn as the sea material; nothing when the eye is
//      so far north that the sea is out of sight, nothing when the table is full
//   2. the ring of a return: only while it lasts, growing and fading, in both layers; nothing for a full table
// Returns 1 when a check fails.

#include "scene/TownEffects.h"

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

using dl::Vec3;
using namespace dl::scene;
namespace tn = dl::town;

bool Near(float a, float b, float eps)
{
    return std::fabs(a - b) <= eps;
}

void TestSea()
{
    SceneObject sea[4] = {};
    const float level = -6.2f;
    const float edge = -60.0f;
    CHECK(tn::AddSea({123.0f, 4.0f, -77.0f}, sea, 0, 4, level, edge) == 2);
    CHECK(sea[0].layerMask == kLayerA && sea[1].layerMask == kLayerB);
    CHECK(Near(sea[0].position[1] + 0.5f * sea[0].scale[1], level, 1e-4f));
    CHECK(sea[0].checker == kMatSea && sea[1].checker == kMatSea);
    // the plane reaches past the far plane in every direction the eye can look, but never north of the edge
    CHECK(Near(sea[0].scale[0], 2.0f * tn::kSeaHalf, 1e-3f));
    CHECK(sea[0].position[0] - 0.5f * sea[0].scale[0] <= 123.0f - tn::kViewDistance);
    CHECK(sea[0].position[0] + 0.5f * sea[0].scale[0] >= 123.0f + tn::kViewDistance);
    CHECK(sea[0].position[2] - 0.5f * sea[0].scale[2] <= -77.0f - tn::kViewDistance);
    CHECK(Near(sea[0].position[2] + 0.5f * sea[0].scale[2], edge, 1e-3f));
    // it moves in steps of kSeaSnap: a small move of the eye leaves it where it is
    SceneObject moved[2] = {};
    CHECK(tn::AddSea({123.0f + 4.0f, 4.0f, -77.0f + 3.0f}, moved, 0, 2, level, edge) == 2);
    CHECK(moved[0].position[0] == sea[0].position[0] && moved[0].position[2] == sea[0].position[2]);
    // the eye far to the north: the sea is out of sight
    SceneObject none[2] = {};
    CHECK(tn::AddSea({0.0f, 4.0f, 900.0f}, none, 0, 2, level, edge) == 0);
    // a table without room, and the count it was given
    CHECK(tn::AddSea({0.0f, 4.0f, -77.0f}, sea, 3, 4, level, edge) == 4); // room for one: the plane of A only
    CHECK(tn::AddSea({0.0f, 4.0f, -77.0f}, sea, 4, 4, level, edge) == 4);
    CHECK(tn::AddSea({0.0f, 4.0f, -77.0f}, nullptr, 0, 4, level, edge) == 0);
}

void TestRing()
{
    SceneObject out[2] = {};
    const Vec3 at{1.0f, 2.0f, 3.0f};
    CHECK(tn::AddRespawnRing(at, -1.0f, out, 0, 2) == 0);                   // not started
    CHECK(tn::AddRespawnRing(at, tn::kRespawnRingSeconds, out, 0, 2) == 0); // over
    CHECK(tn::AddRespawnRing(at, 0.1f, out, 2, 2) == 2);                    // no room
    CHECK(tn::AddRespawnRing(at, 0.1f, out, 0, 2) == 1);
    const SceneObject early = out[0];
    CHECK(early.shape == ShapeKind::Torus && early.layerMask == kLayerBoth);
    CHECK(Near(early.position[0], 1.0f, 1e-6f) && Near(early.position[1], 2.1f, 1e-6f) &&
          Near(early.position[2], 3.0f, 1e-6f));
    CHECK(tn::AddRespawnRing(at, 0.6f, out, 0, 2) == 1);
    CHECK(out[0].scale[0] > early.scale[0] && out[0].glow < early.glow); // it grows and fades
}

} // namespace

int main()
{
    TestSea();
    TestRing();
    std::printf("%d checks passed, %d failed\n", g_passed, g_failed);
    return g_failed == 0 ? 0 : 1;
}
