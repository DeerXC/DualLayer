// test_hints.cpp - tests of the outlines of the other layer (no window, no GPU): src/scene/StageHints.h on the stage of
// src/scene/Stage.h.
//   g++ -std=c++20 -Wall -Wextra -I src tests/test_hints.cpp -o test_hints
//   MSVC (Developer PowerShell):  .\tests\run_tests.ps1
// Header-only: there is no .cpp to add to the project. What is checked:
//   1. AddGhosts adds four bars per listed object, after the objects that were there, and changes none of them
//   2. every bar is in the OTHER layer only: an object of A is outlined in B, an object of B in A
//   3. the bars lie on the edges of the footprint of their object, are thin, and do not overlap at the corners
//   4. a flat object is outlined at its top, a tall one at its base, a little above the surface
//   5. the colour is the colour of the object, lighter
//   6. a table that is too full, a null table or a bad count gets no outline at all
// Returns 1 when a check fails.

#include "scene/StageHints.h"

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

using namespace dl::scene;
namespace st = dl::stage;

bool Near(float a, float b, float eps = 1e-5f)
{
    return std::fabs(a - b) <= eps;
}

void TestAdd()
{
    SceneObject table[st::kMaxDrawObjects] = {};
    const dl::puzzle::Puzzle puzzle;
    const int base = st::BuildDrawTable(puzzle, table);
    SceneObject before[st::kMaxDrawObjects] = {};
    for (int i = 0; i < base; ++i)
    {
        before[i] = table[i];
    }
    const int count = st::AddGhosts(table, base, st::kMaxDrawObjects);
    CHECK(count == base + st::kGhostCount * st::kBarsPerGhost);
    bool unchanged = true;
    for (int i = 0; i < base; ++i)
    {
        unchanged = unchanged && table[i].layerMask == before[i].layerMask && table[i].position[2] == before[i].position[2] &&
                    table[i].scale[1] == before[i].scale[1];
    }
    CHECK(unchanged);

    for (int g = 0; g < st::kGhostCount; ++g)
    {
        const SceneObject& o = st::kStageStatics[st::kGhostIndices[g]];
        const unsigned other = o.layerMask == kLayerA ? kLayerB : kLayerA;
        const float hx = 0.5f * o.scale[0];
        const float hz = 0.5f * o.scale[2];
        const float minX = o.position[0] - hx;
        const float maxX = o.position[0] + hx;
        const float minZ = o.position[2] - hz;
        const float maxZ = o.position[2] + hz;
        const bool flat = o.scale[1] <= st::kFlatHeight;
        const float surface = flat ? o.position[1] + 0.5f * o.scale[1] : o.position[1] - 0.5f * o.scale[1];
        float area = 0.0f;
        for (int b = 0; b < st::kBarsPerGhost; ++b)
        {
            const SceneObject& bar = table[base + g * st::kBarsPerGhost + b];
            CHECK(bar.layerMask == other);                       // only in the other layer
            CHECK(bar.shape == ShapeKind::Cube);
            CHECK(bar.scale[1] <= 0.05f);                        // thin: it must not look like a step
            CHECK(std::fmin(bar.scale[0], bar.scale[2]) <= st::kGhostBar + 1e-5f); // narrow
            const float bottom = bar.position[1] - 0.5f * bar.scale[1];
            CHECK(Near(bottom, surface + st::kGhostLift, 1e-4f)); // just above the top of a flat object / the base of a wall
            // inside the footprint, and touching its edge
            const float bx0 = bar.position[0] - 0.5f * bar.scale[0];
            const float bx1 = bar.position[0] + 0.5f * bar.scale[0];
            const float bz0 = bar.position[2] - 0.5f * bar.scale[2];
            const float bz1 = bar.position[2] + 0.5f * bar.scale[2];
            CHECK(bx0 >= minX - 1e-4f && bx1 <= maxX + 1e-4f && bz0 >= minZ - 1e-4f && bz1 <= maxZ + 1e-4f);
            CHECK(Near(bx0, minX, 1e-4f) || Near(bx1, maxX, 1e-4f) || Near(bz0, minZ, 1e-4f) || Near(bz1, maxZ, 1e-4f));
            // the colour of the object, lighter
            for (int c = 0; c < 3; ++c)
            {
                CHECK(bar.colorSrgb[c] >= o.colorSrgb[c] && bar.colorSrgb[c] <= 1.0f);
            }
            area += bar.scale[0] * bar.scale[2];
        }
        // the four bars together are exactly the ring of width kGhostBar round the footprint: no corner is covered twice
        const float ring = 2.0f * hx * 2.0f * hz - (2.0f * hx - 2.0f * st::kGhostBar) * (2.0f * hz - 2.0f * st::kGhostBar);
        CHECK(Near(area, ring, 1e-4f));
    }
    // the wall of part 1 is outlined on the floor (its base), the stone of part 2 at its top
    CHECK(Near(st::GhostY(st::kStageStatics[1]) - 0.5f * st::kGhostThick, kFloorY + st::kGhostLift, 1e-5f));
    CHECK(Near(st::GhostY(st::kStageStatics[2]) - 0.5f * st::kGhostThick, kFloorY + st::kGhostLift, 1e-5f));
    CHECK(Near(st::GhostY(st::kStageStatics[10]) - 0.5f * st::kGhostThick, st::kDoorwayLow + st::kGhostLift, 1e-5f));
}

void TestNoRoom()
{
    SceneObject table[st::kMaxDrawObjects] = {};
    const int need = st::kGhostCount * st::kBarsPerGhost;
    CHECK(st::AddGhosts(table, st::kMaxDrawObjects - need + 1, st::kMaxDrawObjects) == st::kMaxDrawObjects - need + 1); // one short: none
    CHECK(st::AddGhosts(table, st::kMaxDrawObjects - need, st::kMaxDrawObjects) == st::kMaxDrawObjects);               // exactly enough
    CHECK(st::AddGhosts(nullptr, 3, st::kMaxDrawObjects) == 3);
    CHECK(st::AddGhosts(table, -1, st::kMaxDrawObjects) == -1);
}

void TestList()
{
    CHECK(st::GhostsAreValid());
    // every listed object is in one layer only; the red panel in the doorway (index 9) is not listed
    bool panelListed = false;
    for (int index : st::kGhostIndices)
    {
        const unsigned mask = st::kStageStatics[index].layerMask;
        CHECK(mask == kLayerA || mask == kLayerB);
        panelListed = panelListed || index == 9;
    }
    CHECK(!panelListed);
}

} // namespace

int main()
{
    TestAdd();
    TestNoRoom();
    TestList();
    std::printf("%d checks passed, %d failed\n", g_passed, g_failed);
    return g_failed == 0 ? 0 : 1;
}
