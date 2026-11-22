// test_hints.cpp - tests of what the map shows without words (no window, no GPU): src/scene/StageHints.h on the map of
// src/scene/Stage.h.
//   g++ -std=c++20 -Wall -Wextra -I src tests/test_hints.cpp -o test_hints
//   MSVC (Developer PowerShell):  .\tests\run_tests.ps1 -Only hints
// Header-only: there is no .cpp to add to the project. What is checked:
//   1. the outlines: one per axis-aligned single-layer box of the map, four bars each, after the objects that were there,
//      which stay unchanged; each bar only in the OTHER layer, thin, narrow, inside the footprint and on its edge; the four
//      together exactly the ring round the footprint; flat objects at their top, walls at their base; the colour of the
//      object, lighter; a table that is too full gets none
//   2. a refused push (the body walks into the block toward a blue wall): the blue wall stays in layer B in the drawn table
//      (it is not drawn as a solid box in A); its outline is drawn in both layers, wider than a hint, inside its footprint;
//      nothing once the flash is over
//   3. the frame of the door: four bars in layer A round the doorway, just in front of the wall, only while the door is closed
//   4. the beacon: in both layers, seen from the start over every axis-aligned box of A and of B (a beacon at the height of
//      the gate would not be: the test can tell the difference); after the clear it grows wide
//   5. the landmarks: each seen over the walls from the hub, each outside every room (drawn only, never walked into)
//   6. the clear rings: none before the clear and none after kClearSeconds; in between they grow
//   7. the previews: the outline of the bridge of the west wing in both layers until it is laid, then none; doors and bars get
//      none (they are there while closed)
//   8. the whole drawn table of a frame (the map, its state, the outlines, the flash, the frame, the beacon, the rings, the
//      lights) fits
// Returns 1 when a check fails.

#include "scene/CityParts.h"
#include "scene/StageHints.h"
#include "scene/StageLights.h"
#include "scene/WestWing.h"

#include <cmath>
#include <cstdio>
#include <vector>

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
    for (const SceneObject& o : st::kStageStatics)
    {
        single += (o.tilt == 0.0f && (o.layerMask == kLayerA || o.layerMask == kLayerB)) ? 1 : 0;
    }
    CHECK(st::kGhostCount == single && st::kRunCount > 0);

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
            const SceneObject world = st::WorldTinted(o); // red for a box of A, blue for B, made lighter
            for (int c = 0; c < 3; ++c)
            {
                ok = ok && Near(bar.colorSrgb[c], world.colorSrgb[c] + (1.0f - world.colorSrgb[c]) * st::kGhostLight, 1e-4f);
            }
            area += bar.scale[0] * bar.scale[2];
        }
        CHECK(ok);
        const float w = maxX - minX;
        const float d = maxZ - minZ;
        CHECK(Near(area, w * d - (w - 2.0f * st::kGhostBar) * (d - 2.0f * st::kGhostBar), 1e-3f));
        ++g;
    }
    const int need = st::kGhostCount * st::kBarsPerGhost; // no room: no outline at all
    CHECK(st::AddGhosts(table, st::kMaxDrawObjects - need + 1, st::kMaxDrawObjects) == st::kMaxDrawObjects - need + 1);
    CHECK(st::AddGhosts(nullptr, 3, st::kMaxDrawObjects) == 3 && st::AddGhosts(table, -1, st::kMaxDrawObjects) == -1);
}

void TestWorldEdges()
{
    // a box of one world is drawn in its material, never as a plain red or blue box; its edge in its own world is red or blue
    for (const SceneObject& o : st::kStageStatics)
    {
        if (st::HasGhost(o))
        {
            CHECK(!st::SameRgb(o, st::kRed) && !st::SameRgb(o, st::kBlue));
        }
    }
    static SceneObject table[st::kMaxDrawObjects] = {};
    const int n = st::AddWorldEdges(table, 0, st::kMaxDrawObjects);
    CHECK(n == st::kGhostCount * st::kBarsPerGhost);
    int g = 0;
    for (const SceneObject& o : st::kStageStatics)
    {
        if (!st::HasGhost(o))
        {
            continue;
        }
        const SceneObject world = st::WorldTinted(o);
        bool ok = true;
        for (int b = 0; b < st::kBarsPerGhost; ++b)
        {
            const SceneObject& bar = table[g * st::kBarsPerGhost + b];
            ok = ok && bar.layerMask == o.layerMask && std::fmin(bar.scale[0], bar.scale[2]) <= st::kEdgeBar + 1e-4f &&
                 std::fabs(bar.position[0] - o.position[0]) <= 0.5f * o.scale[0] + 1e-4f &&
                 std::fabs(bar.position[2] - o.position[2]) <= 0.5f * o.scale[2] + 1e-4f && bar.colorSrgb[0] >= world.colorSrgb[0] - 1e-4f &&
                 bar.colorSrgb[2] >= world.colorSrgb[2] - 1e-4f;
            const bool redder = bar.colorSrgb[0] > bar.colorSrgb[2];
            ok = ok && (o.layerMask == kLayerA ? redder : !redder);
        }
        CHECK(ok);
        ++g;
    }
    CHECK(st::AddWorldEdges(table, st::kMaxDrawObjects - 3, st::kMaxDrawObjects) == st::kMaxDrawObjects - 3); // no room: none
    SceneObject both = st::kStageStatics[0];
    CHECK(st::SameRgb(st::WorldTinted(both), st::kGrey)); // a box of both worlds keeps its colour
}

void TestBlockedFlash()
{
    // the body walks west into the block, toward the blue wall that a player in A does not see
    dl::puzzle::Puzzle puzzle;
    CHECK(puzzle.Init(st::MakeStage()));
    dl::Player body;
    const float bx = static_cast<float>(st::kBlockStart.x);
    const float bz = static_cast<float>(st::kBlockStart.z);
    body.Place({bx + 0.88f, kFloorY + body.GetSettings().eyeHeight, bz});
    dl::Player::Input walk;
    walk.moveForward = 1.0f;
    for (int i = 0; i < 60 && puzzle.BlockedFlash() <= 0.0f; ++i)
    {
        int n = 0;
        const SceneObject* solids = puzzle.Solids(0, n);
        body.SetScene(solids, n);
        body.Update(walk, {-1.0f, 0.0f, 0.0f}, 0, 0.0f, 1.0f / 60.0f);
        puzzle.Update(body, 1.0f / 60.0f);
    }
    const int blocker = puzzle.LastBlocker();
    CHECK(puzzle.BlockedFlash() > 0.0f && blocker >= 0);
    if (blocker < 0)
    {
        return;
    }
    const SceneObject& wall = st::kStageStatics[static_cast<std::size_t>(blocker)];
    CHECK(wall.layerMask == kLayerB);
    static SceneObject table[st::kMaxDrawObjects] = {};
    const int base = st::BuildDrawTable(puzzle, false, table);
    CHECK(table[blocker].layerMask == kLayerB); // the wall itself stays in its own layer
    const int count = st::AddBlockedFlash(puzzle, table, base, st::kMaxDrawObjects);
    CHECK(count == base + st::kBarsPerGhost);
    for (int i = base; i < count; ++i)
    {
        const SceneObject& bar = table[i];
        CHECK(bar.layerMask == kLayerBoth && bar.scale[1] <= 0.05f);
        CHECK(std::fmin(bar.scale[0], bar.scale[2]) > st::kGhostBar && std::fmin(bar.scale[0], bar.scale[2]) <= st::kBlockedBar + 1e-4f);
        CHECK(std::fabs(bar.position[0] - wall.position[0]) <= 0.5f * wall.scale[0] && std::fabs(bar.position[2] - wall.position[2]) <= 0.5f * wall.scale[2]);
    }
    // once the flash is over: nothing
    for (int i = 0; i < 60; ++i)
    {
        puzzle.Update(dl::Player{}, 1.0f / 60.0f);
    }
    CHECK(puzzle.BlockedFlash() == 0.0f && st::AddBlockedFlash(puzzle, table, base, st::kMaxDrawObjects) == base);
    CHECK(st::AddBlockedFlash(puzzle, nullptr, base, st::kMaxDrawObjects) == base);
}

void TestDoorFrame()
{
    SceneObject table[16] = {};
    CHECK(st::AddDoorFrame(table, 0, 16, true) == 0);    // open for A: no frame
    CHECK(st::AddDoorFrame(table, 13, 16, false) == 13); // no room for four bars: none
    const int n = st::AddDoorFrame(table, 0, 16, false);
    CHECK(n == st::kFrameBars);
    const float x0 = st::kDoorX - st::kDoorwayHalfWidth;
    const float x1 = st::kDoorX + st::kDoorwayHalfWidth;
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

/// True when the segment from p to q meets the axis-aligned box `o`.
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

/// True when `point` is seen from the eye of a body at the start in `layer`. The ramps (turned boxes) are left out: they
/// lie inside rooms, far below the lines that are tested.
bool SeenFromStart(const float point[3], int layer)
{
    const dl::Vec3 start = st::kRooms[st::kStartRoom].entry;
    const float eye[3] = {start.x, start.y + dl::Player::Settings{}.eyeHeight, start.z};
    const unsigned bit = 1u << layer;
    for (const SceneObject& o : st::kStageStatics)
    {
        if (o.tilt == 0.0f && (o.layerMask & bit) != 0u && SegmentHitsBox(eye, point, o))
        {
            return false;
        }
    }
    for (const SceneObject& o : st::kDecor) // the drawn-only city: battlements, the skyline, the cathedral
    {
        if (o.tilt == 0.0f && (o.layerMask & bit) != 0u && SegmentHitsBox(eye, point, o))
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

/// True when the segment from the eye to `point` meets no axis-aligned box of the map (both layers: the walls of the hub are
/// in both).
bool SeenFrom(const float eye[3], const float point[3])
{
    for (const SceneObject& o : st::kStageStatics)
    {
        if (o.tilt == 0.0f && o.layerMask == kLayerBoth && SegmentHitsBox(eye, point, o))
        {
            return false;
        }
    }
    return true;
}

void TestLandmarks()
{
    // each landmark is seen over the walls from a place on the hub floor (the corner across from it, so that the tower is not
    // in the way), and a lower point at the same place is not: the test can tell
    struct View
    {
        float eye[3];
        float top[3];
        float low[3];
    };
    const float eyeY = kFloorY + dl::Player::Settings{}.eyeHeight;
    const View views[] = {
        {{8.0f, eyeY, 39.0f}, {-27.5f, 19.5f, 31.0f}, {-27.5f, 3.0f, 31.0f}}, // the archive, from the north-east corner
        {{-8.0f, eyeY, 24.0f}, {29.0f, 20.5f, 31.0f}, {29.0f, 3.0f, 31.0f}},  // the water ring, from the south-west corner
        {{-8.0f, eyeY, 23.0f}, {0.0f, 24.0f, 65.0f}, {0.0f, 5.0f, 65.0f}},    // the bell, from the south-west corner
    };
    for (const View& v : views)
    {
        if (!SeenFrom(v.eye, v.top))
        {
            std::printf("  landmark top (%.1f %.1f %.1f) not seen\n", static_cast<double>(v.top[0]), static_cast<double>(v.top[1]), static_cast<double>(v.top[2]));
        }
        CHECK(SeenFrom(v.eye, v.top));
        CHECK(!SeenFrom(v.eye, v.low));
    }
    // drawn only: each is apart from every room, also from the highest point a body in it can reach (its feet at the top
    // of the room, a jump, the body over it)
    const float reach = dl::Player::Settings{}.jumpHeight + dl::Player::Settings{}.eyeHeight + dl::Player::Settings{}.radius + 0.3f;
    for (const SceneObject& o : st::kLandmarks)
    {
        float half[3] = {0.5f * o.scale[0], 0.5f * o.scale[1], 0.5f * o.scale[2]};
        if (o.shape == ShapeKind::Torus) // a turning ring sweeps a disc of its whole outer radius, round its centre
        {
            const float outer = (dl::scene::kTorusMajor + dl::scene::kTorusMinor) * o.scale[0];
            half[0] = outer;
            half[1] = outer;
            half[2] = outer;
        }
        for (const st::Room& room : st::kRooms)
        {
            const bool apart = o.position[0] + half[0] < room.x0 || o.position[0] - half[0] > room.x1 || o.position[2] + half[2] < room.z0 ||
                               o.position[2] - half[2] > room.z1 || o.position[1] - half[1] > room.y1 + reach;
            CHECK(apart);
        }
    }
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

void TestOutlines()
{
    // the moving parts: each single-layer box gets its outline in the other layer; a sphere and a box of both layers get none
    SceneObject table[64] = {};
    const dl::mech::MechanismSet set = [] {
        dl::mech::MechanismSet m;
        m.Init(st::WestWing());
        return m;
    }();
    int n = set.AddFixedDraw(table, 0, 64);
    int single = 0;
    for (int i = 0; i < n; ++i)
    {
        single += st::HasGhost(table[i]) ? 1 : 0;
    }
    const int before = n;
    n = st::AddOutlines(table, before, table, before, 64);
    CHECK(n == before + single * st::kBarsPerGhost && single > 0);
    for (int i = before; i < n; ++i)
    {
        CHECK(table[i].layerMask == kLayerA || table[i].layerMask == kLayerB);
    }
    CHECK(st::AddOutlines(table, before, table, 62, 64) == 62); // no room for all: none
}

void TestPreviews()
{
    dl::mech::MechanismSet set;
    CHECK(set.Init(st::WestWing()));
    SceneObject table[32] = {};
    int bridges = 0;
    for (int i = 0; i < set.GateCount(); ++i)
    {
        bridges += set.GateAt(i).presentWhenOpen ? 1 : 0;
    }
    CHECK(bridges == 1);
    const int n = st::AddGatePreviews(set, table, 0, 32);
    CHECK(n == bridges * st::kBarsPerGhost);
    const SceneObject& bridge = set.GateAt(st::kW1Bridge).box;
    for (int i = 0; i < n; ++i)
    {
        CHECK(table[i].layerMask == kLayerBoth && table[i].glow > st::kHintGlow);
        CHECK(std::fabs(table[i].position[0] - bridge.position[0]) <= 0.5f * bridge.scale[0] &&
              std::fabs(table[i].position[2] - bridge.position[2]) <= 0.5f * bridge.scale[2]);
        CHECK(table[i].position[1] > bridge.position[1]); // on its top
    }
    CHECK(st::AddGatePreviews(set, table, 30, 32) == 30); // no room: none
    // an open door is not previewed (it is a door, not a bridge): the body stands on the plate of W2 in A, the door opens
    dl::Player body;
    const dl::mech::Trigger& plate = set.TriggerAt(st::kW2Plate);
    body.SetScene(st::kStageStatics.data(), st::kStageStaticCount);
    body.Place({plate.at.x, plate.at.y + body.GetSettings().eyeHeight, plate.at.z});
    for (int i = 0; i < 30; ++i)
    {
        body.Update({}, {0.0f, 0.0f, 1.0f}, 0, 0.0f, 1.0f / 60.0f);
        set.Update(body, 0, st::kStageStatics.data(), st::kStageStaticCount, st::kStageStatics.data(), st::kStageStaticCount, 1.0f / 60.0f);
    }
    CHECK(set.GateOpen(st::kW2Door) && !set.GatePresent(st::kW2Door));
    CHECK(st::AddGatePreviews(set, table, 0, 32) == bridges * st::kBarsPerGhost);
}

void TestSilhouette()
{
    // battlements: each merlon stands on the top of a wall that has them, inside its footprint, and in no other box of the map
    int merlons = 0;
    bool onWall = true;
    bool free = true;
    for (const SceneObject& m : st::kDecor)
    {
        if (!(st::SameRgb(m, st::kStone) && Near(m.scale[1], st::kMerlonHeight) &&
              (Near(m.scale[0], st::kMerlonWidth) || Near(m.scale[2], st::kMerlonWidth))))
        {
            continue;
        }
        ++merlons;
        const float bottom = m.position[1] - 0.5f * m.scale[1];
        bool found = false;
        for (const SceneObject& w : st::kStageStatics)
        {
            found = found || (st::HasBattlements(w) && Near(bottom, w.position[1] + 0.5f * w.scale[1]) &&
                              std::fabs(m.position[0] - w.position[0]) <= 0.5f * w.scale[0] + 1e-3f &&
                              std::fabs(m.position[2] - w.position[2]) <= 0.5f * w.scale[2] + 1e-3f);
        }
        onWall = onWall && found;
        for (const SceneObject& o : st::kStageStatics)
        {
            free = free && !st::BoxesOverlap(m, o);
        }
    }
    std::printf("  silhouette: %d merlons, %zu drawn-only boxes in all\n", merlons, st::kDecor.size());
    CHECK(merlons > 150 && onWall && free);
    // none on the ledge of the north wing: its floor lies on the wall under the window
    bool onLedge = false;
    for (const SceneObject& m : st::kDecor)
    {
        onLedge = onLedge || (Near(m.scale[1], st::kMerlonHeight) && m.position[0] > st::kLedgeWest && m.position[0] < st::kLedgeEast &&
                              m.position[2] > st::kLedgeSouth && m.position[2] < 44.0f && m.position[1] < st::kNorthHighY + 1.0f);
    }
    CHECK(!onLedge);
    // the skyline: every house and tower stands apart from every room by more than a running jump, and in no box of the map
    std::vector<SceneObject> skyline;
    st::Skyline(skyline);
    bool apart = true;
    for (const SceneObject& o : skyline)
    {
        if (o.tilt != 0.0f)
        {
            continue; // a roof slope lies on its house
        }
        for (const st::Room& r : st::kRooms)
        {
            apart = apart && st::FootprintGap(o.position[0] - 0.5f * o.scale[0], o.position[2] - 0.5f * o.scale[2], o.position[0] + 0.5f * o.scale[0],
                                              o.position[2] + 0.5f * o.scale[2], r.x0, r.z0, r.x1, r.z1) > dl::Player::Settings{}.walkSpeed * 0.75f;
        }
        for (const SceneObject& m : st::kStageStatics)
        {
            apart = apart && !st::BoxesOverlap(o, m);
        }
    }
    std::printf("  skyline: %zu boxes\n", skyline.size());
    CHECK(apart && skyline.size() > 300);
    // the spire is seen from every district, over the walls and the battlements (the cathedral itself not counted)
    const float tip[3] = {0.0f, st::kSpireTop - 0.5f, 0.5f * (st::kTowerInnerNorth + st::kTowerNorth)};
    const float eyeY = dl::Player::Settings{}.eyeHeight;
    const float eyes[][3] = {
        {0.0f, kFloorY + eyeY, 15.0f},   // the courtyard
        {-20.0f, kFloorY + eyeY, 36.0f}, // the west hall
        {20.0f, kFloorY + eyeY, 36.0f},  // the east hall
        {0.0f, kFloorY + eyeY, 52.0f},   // the north hall
        {20.0f, st::kEastHighY + eyeY, 6.0f}, // the upper room of the east wing
    };
    for (const auto& eye : eyes)
    {
        bool seen = true;
        for (const SceneObject& o : st::kStageStatics)
        {
            seen = seen && !(o.tilt == 0.0f && !st::SameRgb(o, st::kTowerStone) && SegmentHitsBox(eye, tip, o));
        }
        for (const SceneObject& o : st::kDecor)
        {
            seen = seen && !(o.tilt == 0.0f && !st::SameRgb(o, st::kTowerStone) && !st::SameRgb(o, st::kGold) && SegmentHitsBox(eye, tip, o));
        }
        if (!seen)
        {
            std::printf("  the spire is not seen from (%.1f %.1f %.1f)\n", static_cast<double>(eye[0]), static_cast<double>(eye[1]), static_cast<double>(eye[2]));
        }
        CHECK(seen);
    }
}

void TestWholeFrameFits()
{
    const dl::mech::WingDesc west = st::CityParts(); // every moving part of the city
    const int moving = west.gateCount + west.triggerCount; // the gates and the pads, each outlined, the orbs with their marks,
                                                           // a column per trigger, the previews of the gates
    const int largest = st::StaticDrawCount() + 4 + st::kLandmarkCount + 2 + 2 * st::kSealCount + 2 * st::kGhostCount * st::kBarsPerGhost +
                        9 /* the canal */ + 2 /* its beacons */ + st::kWaterDrawCount + st::kAnchorCount + 2 /* the respawn rings */ + moving * st::kBarsPerGhost /* own edges */ +
                        st::kBarsPerGhost + moving + moving * st::kBarsPerGhost + 2 * west.orbCount + st::kFrameBars + 1 + st::kClearRings +
                        st::kHiddenLightCount + west.triggerCount + west.gateCount * st::kBarsPerGhost + st::kLiftCount;
    std::printf("  the largest drawn table: %d of %d\n", largest, st::kMaxDrawObjects);
    CHECK(largest <= st::kMaxDrawObjects);
}

} // namespace

int main()
{
    TestGhosts();
    TestWorldEdges();
    TestBlockedFlash();
    TestDoorFrame();
    TestBeacon();
    TestLandmarks();
    TestClearRings();
    TestOutlines();
    TestPreviews();
    TestSilhouette();
    TestWholeFrameFits();
    std::printf("%d checks passed, %d failed\n", g_passed, g_failed);
    return g_failed == 0 ? 0 : 1;
}
