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
//   7. the frame of the blue door: four thin bars in layer A round the doorway, just in front of the wall, only while the door
//      is closed for a body in A; none when the table has no room
//   8. the beacon over the goal: drawn in both layers, and seen from the start of the stage over every solid of A and of B
//      (a line from the eye at the start to the top part of the beacon meets no box of the stage)
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

/// True when the segment from p to q meets the box `o` (axis-aligned: the stage boxes are not turned).
bool SegmentHitsBox(const float p[3], const float q[3], const SceneObject& o)
{
    float t0 = 0.0f;
    float t1 = 1.0f;
    for (int a = 0; a < 3; ++a)
    {
        const float lo = o.position[a] - 0.5f * o.scale[a];
        const float hi = o.position[a] + 0.5f * o.scale[a];
        const float d = q[a] - p[a];
        if (std::fabs(d) < 1e-9f)
        {
            if (p[a] < lo || p[a] > hi)
            {
                return false;
            }
            continue;
        }
        float ta = (lo - p[a]) / d;
        float tb = (hi - p[a]) / d;
        if (ta > tb)
        {
            const float t = ta;
            ta = tb;
            tb = t;
        }
        t0 = ta > t0 ? ta : t0;
        t1 = tb < t1 ? tb : t1;
        if (t0 > t1)
        {
            return false;
        }
    }
    return true;
}

void TestDoorFrame()
{
    SceneObject table[16] = {};
    CHECK(st::AddDoorFrame(table, 0, 16, true) == 0);  // open for A: no frame
    CHECK(st::AddDoorFrame(table, 13, 16, false) == 13); // no room for four bars: none
    CHECK(st::AddDoorFrame(nullptr, 0, 16, false) == 0);
    const int n = st::AddDoorFrame(table, 0, 16, false);
    CHECK(n == st::kFrameBars);
    const float x0 = st::kDoor.position[0] - 0.5f * st::kDoor.scale[0];
    const float x1 = st::kDoor.position[0] + 0.5f * st::kDoor.scale[0];
    const float wallSouth = st::kWallZ - 0.25f;
    float area = 0.0f;
    for (int i = 0; i < n; ++i)
    {
        const SceneObject& b = table[i];
        CHECK(b.layerMask == kLayerA);
        CHECK(b.position[2] + 0.5f * b.scale[2] < wallSouth && b.position[2] - 0.5f * b.scale[2] > wallSouth - 0.1f); // in front of the wall
        CHECK(b.position[0] - 0.5f * b.scale[0] >= x0 - 1e-4f && b.position[0] + 0.5f * b.scale[0] <= x1 + 1e-4f);
        CHECK(b.position[1] - 0.5f * b.scale[1] >= st::kDoorwayLow - 1e-4f && b.position[1] + 0.5f * b.scale[1] <= st::kDoorwayHigh + 1e-4f);
        CHECK(b.colorSrgb[2] >= st::kDoor.colorSrgb[2]); // the blue of the door, lighter
        area += b.scale[0] * b.scale[1];
    }
    // the four bars are exactly the frame of width kGhostBar round the doorway (no corner twice)
    const float wDoor = x1 - x0;
    const float hDoor = st::kDoorwayHigh - st::kDoorwayLow;
    CHECK(Near(area, wDoor * hDoor - (wDoor - 2.0f * st::kGhostBar) * (hDoor - 2.0f * st::kGhostBar), 1e-4f));
}

void TestBeacon()
{
    SceneObject table[4] = {};
    CHECK(st::AddGoalBeacon(table, 4, 4) == 4 && st::AddGoalBeacon(nullptr, 0, 4) == 0);
    CHECK(st::AddGoalBeacon(table, 0, 4) == 1);
    const SceneObject& b = table[0];
    CHECK(b.layerMask == kLayerBoth && Near(b.position[2], st::kGoalZ) && Near(b.position[0], st::kBeaconX));
    CHECK(Near(b.position[1] - 0.5f * b.scale[1], st::kBeaconBottom) && Near(b.position[1] + 0.5f * b.scale[1], st::kBeaconTop));
    // seen from the start: the eye of a body standing at the first checkpoint (feet at z = -3, eye 1.55 m above the floor)
    const float eye[3] = {0.0f, kFloorY + 1.55f, -3.0f};
    const float top[3] = {st::kBeaconX, st::kBeaconTop - 0.5f, st::kGoalZ}; // a point near the top of the beacon
    for (int layer = 0; layer < 2; ++layer)
    {
        const unsigned bit = 1u << layer;
        bool blocked = false;
        for (const SceneObject& o : st::kStageStatics)
        {
            if ((o.layerMask & bit) != 0u && SegmentHitsBox(eye, top, o))
            {
                blocked = true;
            }
        }
        if (layer == 1)
        {
            blocked = blocked || SegmentHitsBox(eye, top, st::kDoor); // the closed door of B
        }
        CHECK(!blocked);
    }
    // and a beacon that ends at the height of the walls would NOT be seen in A (the test can tell the difference)
    const float low[3] = {st::kBeaconX, 3.5f, st::kGoalZ};
    bool lowBlocked = false;
    for (const SceneObject& o : st::kStageStatics)
    {
        lowBlocked = lowBlocked || ((o.layerMask & kLayerA) != 0u && SegmentHitsBox(eye, low, o));
    }
    CHECK(lowBlocked);
}

} // namespace

int main()
{
    TestAdd();
    TestNoRoom();
    TestList();
    TestDoorFrame();
    TestBeacon();
    std::printf("%d checks passed, %d failed\n", g_passed, g_failed);
    return g_failed == 0 ? 0 : 1;
}
