// test_hints.cpp - tests of what the stage shows without words (no window, no GPU): src/scene/StageHints.h on the stage of
// src/scene/Stage.h.
//   g++ -std=c++20 -Wall -Wextra -I src tests/test_hints.cpp -o test_hints
//   MSVC (Developer PowerShell):  .\tests\run_tests.ps1
// Header-only: there is no .cpp to add to the project. What is checked:
//   1. the outlines: one per single-layer box of the stage (every wall of the courtyard among them), four bars each, after
//      the objects that were there, which stay unchanged; each bar only in the OTHER layer, thin, narrow, inside the footprint
//      and on its edge; the four together exactly the ring round the footprint; flat objects at their top, walls at their
//      base; the colour of the object, lighter; a table that is too full gets none
//   2. the frame of the door: four bars in layer A round the doorway, just in front of the wall, only while the door is closed
//   3. the beacon: in both layers, seen from the start over every box of A and of B (and a beacon at the height of the walls
//      would not be: the test can tell the difference); after the clear it grows wide
//   4. the clear rings: none before the clear and none after kClearSeconds; in between they grow
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

bool Near(float a, float b, float eps = 1e-4f)
{
    return std::fabs(a - b) <= eps;
}

void TestGhosts()
{
    int single = 0;
    int courtyardWalls = 0;
    for (int i = 0; i < st::kStageStaticCount; ++i)
    {
        const SceneObject& o = st::kStageStatics[static_cast<std::size_t>(i)];
        single += (o.layerMask == kLayerA || o.layerMask == kLayerB) ? 1 : 0;
        courtyardWalls += i >= st::kFixedCount ? 1 : 0;
    }
    CHECK(st::kGhostCount == single && courtyardWalls == st::kRunCount && st::kRunCount > 0);

    static SceneObject table[st::kMaxDrawObjects] = {};
    const dl::puzzle::Puzzle puzzle;
    const int base = st::BuildDrawTable(puzzle, false, table);
    static SceneObject before[st::kMaxDrawObjects] = {};
    for (int i = 0; i < base; ++i)
    {
        before[i] = table[i];
    }
    const int count = st::AddGhosts(table, base, st::kMaxDrawObjects);
    CHECK(count == base + st::kGhostCount * st::kBarsPerGhost);
    bool unchanged = true;
    for (int i = 0; i < base; ++i)
    {
        unchanged = unchanged && table[i].layerMask == before[i].layerMask && table[i].position[2] == before[i].position[2];
    }
    CHECK(unchanged);

    int g = 0;
    for (const SceneObject& o : st::kStageStatics)
    {
        if (!st::HasGhost(o))
        {
            continue;
        }
        const unsigned other = o.layerMask == kLayerA ? kLayerB : kLayerA;
        const float minX = o.position[0] - 0.5f * o.scale[0];
        const float maxX = o.position[0] + 0.5f * o.scale[0];
        const float minZ = o.position[2] - 0.5f * o.scale[2];
        const float maxZ = o.position[2] + 0.5f * o.scale[2];
        const bool flat = o.scale[1] <= st::kFlatHeight;
        const float surface = flat ? o.position[1] + 0.5f * o.scale[1] : o.position[1] - 0.5f * o.scale[1];
        float area = 0.0f;
        bool ok = true;
        for (int b = 0; b < st::kBarsPerGhost; ++b)
        {
            const SceneObject& bar = table[base + g * st::kBarsPerGhost + b];
            const float bx0 = bar.position[0] - 0.5f * bar.scale[0];
            const float bx1 = bar.position[0] + 0.5f * bar.scale[0];
            const float bz0 = bar.position[2] - 0.5f * bar.scale[2];
            const float bz1 = bar.position[2] + 0.5f * bar.scale[2];
            ok = ok && bar.layerMask == other && bar.shape == ShapeKind::Cube && bar.scale[1] <= 0.05f &&
                 std::fmin(bar.scale[0], bar.scale[2]) <= st::kGhostBar + 1e-4f &&
                 Near(bar.position[1] - 0.5f * bar.scale[1], surface + st::kGhostLift) && bx0 >= minX - 1e-4f && bx1 <= maxX + 1e-4f &&
                 bz0 >= minZ - 1e-4f && bz1 <= maxZ + 1e-4f &&
                 (Near(bx0, minX) || Near(bx1, maxX) || Near(bz0, minZ) || Near(bz1, maxZ));
            for (int c = 0; c < 3; ++c)
            {
                ok = ok && bar.colorSrgb[c] >= o.colorSrgb[c] && bar.colorSrgb[c] <= 1.0f;
            }
            area += bar.scale[0] * bar.scale[2];
        }
        CHECK(ok);
        const float w = maxX - minX;
        const float d = maxZ - minZ;
        CHECK(Near(area, w * d - (w - 2.0f * st::kGhostBar) * (d - 2.0f * st::kGhostBar), 1e-3f));
        ++g;
    }
    // no room: no outline at all
    const int need = st::kGhostCount * st::kBarsPerGhost;
    CHECK(st::AddGhosts(table, st::kMaxDrawObjects - need + 1, st::kMaxDrawObjects) == st::kMaxDrawObjects - need + 1);
    CHECK(st::AddGhosts(nullptr, 3, st::kMaxDrawObjects) == 3 && st::AddGhosts(table, -1, st::kMaxDrawObjects) == -1);
}

void TestDoorFrame()
{
    SceneObject table[16] = {};
    CHECK(st::AddDoorFrame(table, 0, 16, true) == 0);    // open for A: no frame
    CHECK(st::AddDoorFrame(table, 13, 16, false) == 13); // no room for four bars: none
    const int n = st::AddDoorFrame(table, 0, 16, false);
    CHECK(n == st::kFrameBars);
    const float x0 = st::kFinaleX - st::kDoorwayHalfWidth;
    const float x1 = st::kFinaleX + st::kDoorwayHalfWidth;
    float area = 0.0f;
    for (int i = 0; i < n; ++i)
    {
        const SceneObject& b = table[i];
        CHECK(b.layerMask == kLayerA);
        CHECK(b.position[2] + 0.5f * b.scale[2] < st::kDoorSouth && b.position[2] - 0.5f * b.scale[2] > st::kDoorSouth - 0.1f);
        CHECK(b.position[0] - 0.5f * b.scale[0] >= x0 - 1e-4f && b.position[0] + 0.5f * b.scale[0] <= x1 + 1e-4f);
        CHECK(b.position[1] - 0.5f * b.scale[1] >= st::kDoorwayLow - 1e-4f && b.position[1] + 0.5f * b.scale[1] <= st::kDoorwayHigh + 1e-4f);
        area += b.scale[0] * b.scale[1];
    }
    const float wDoor = x1 - x0;
    const float hDoor = st::kDoorwayHigh - st::kDoorwayLow;
    CHECK(Near(area, wDoor * hDoor - (wDoor - 2.0f * st::kGhostBar) * (hDoor - 2.0f * st::kGhostBar), 1e-4f));
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

bool SeenFromStart(const float point[3], int layer)
{
    const float eyeHeight = dl::Player::Settings{}.eyeHeight;
    const float eye[3] = {st::kCheckpoints[0].feet.x, st::kCheckpoints[0].feet.y + eyeHeight, st::kCheckpoints[0].feet.z};
    const unsigned bit = 1u << layer;
    for (const SceneObject& o : st::kStageStatics)
    {
        if ((o.layerMask & bit) != 0u && SegmentHitsBox(eye, point, o))
        {
            return false;
        }
    }
    return layer == 0 || !SegmentHitsBox(eye, point, st::kDoor); // the closed door of B
}

void TestBeacon()
{
    SceneObject table[4] = {};
    CHECK(st::AddGoalBeacon(table, 4, 4, -1.0f) == 4 && st::AddGoalBeacon(nullptr, 0, 4, -1.0f) == 0);
    CHECK(st::AddGoalBeacon(table, 0, 4, -1.0f) == 1);
    const SceneObject b = table[0];
    CHECK(b.layerMask == kLayerBoth && Near(b.position[2], st::kGoal.z) && Near(b.position[0], st::kGoal.x));
    CHECK(Near(b.position[1] + 0.5f * b.scale[1], st::kBeaconTop) && Near(b.scale[0], st::kBeaconWidth));
    const float top[3] = {st::kGoal.x, st::kBeaconTop - 0.5f, st::kGoal.z};
    CHECK(SeenFromStart(top, 0) && SeenFromStart(top, 1));
    const float low[3] = {st::kGoal.x, st::kTerraceY + 2.0f, st::kGoal.z};
    CHECK(!SeenFromStart(low, 0)); // the test can tell a hidden point from a seen one
    st::AddGoalBeacon(table, 0, 4, st::kClearSeconds); // after the clear: wide
    CHECK(Near(table[0].scale[0], st::kClearBeaconWidth));
}

void TestClearRings()
{
    SceneObject table[8] = {};
    CHECK(st::AddClearRings(table, 0, 8, -1.0f) == 0);
    CHECK(st::AddClearRings(table, 0, 8, st::kClearSeconds) == 0);
    const int early = st::AddClearRings(table, 0, 8, 0.2f);
    CHECK(early == 1 && table[0].shape == ShapeKind::Torus && table[0].layerMask == kLayerBoth);
    const float small = table[0].scale[0];
    const int later = st::AddClearRings(table, 0, 8, 0.9f);
    CHECK(later == st::kClearRings && table[0].scale[0] > small);
    CHECK(st::AddClearRings(table, 7, 8, 0.9f) == 8); // only as many as there is room for
}

} // namespace

int main()
{
    TestGhosts();
    TestDoorFrame();
    TestBeacon();
    TestClearRings();
    std::printf("%d checks passed, %d failed\n", g_passed, g_failed);
    return g_failed == 0 ? 0 : 1;
}
