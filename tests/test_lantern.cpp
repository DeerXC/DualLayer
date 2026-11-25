// test_lantern.cpp - tests of the lantern, its circle of the past and the traces (no window, no GPU):
// src/scene/Lantern.h, src/scene/Moments.h and the walking body on them (src/scene/Player.h SetTown). Built on a small
// town made here, so that it does not depend on the exported town:
//   A: a floor from z -10 to 16 at y 0 (a bridge over a gap), a wall across it at z 14 from y 0 to 3.
//   B: the same floor, broken: z -10 .. 0 and 6 .. 16 (a gap of 6 m, past any jump); no wall.
//   g++ -std=c++20 -O2 -Wall -Wextra -I src tests/test_lantern.cpp -o test_lantern
//   MSVC (Developer PowerShell):  .\tests\run_tests.ps1 -Only lantern
// What is checked:
//   1. the circle: a sphere; nothing without it
//   2. the solid with the circle: in A nothing changes; in B a ball over the gap is held inside the circle and not
//   outside it;
//      a wall of A inside the circle is solid in B, outside it is not
//   3. the rules of the lantern: set down only in A and standing; on the floor of A ahead (at the feet at an edge);
//   taken up
//      within reach in either moment; refused while the body stands on what only the circle keeps; allowed elsewhere
//   4. the walk: in B the body cannot cross the gap; with the lantern set down on the bridge in A it walks across in B
//   5. the water of A is water inside the circle in B
//   6. traces: none until a change, coming up over kTraceRise, marked toward the colour of the lantern, gone with an
//   undo
// Returns 1 when a check fails.

#include "scene/Lantern.h"
#include "scene/Moments.h"
#include "scene/Player.h"
#include "scene/Town.h"
#include "scene/TriMesh.h"

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

using dl::Player;
using dl::Vec3;
namespace town = dl::town;

constexpr float kDt = 1.0f / 60.0f;
constexpr float kR = 0.35f; // the radius of the body

bool Near(float a, float b, float eps)
{
    return std::fabs(a - b) <= eps;
}

void Floor(town::TriMesh& m, float z0, float z1)
{
    m.Add({-3.0f, 0.0f, z0}, {-3.0f, 0.0f, z1}, {3.0f, 0.0f, z1});
    m.Add({-3.0f, 0.0f, z0}, {3.0f, 0.0f, z1}, {3.0f, 0.0f, z0});
}

struct World
{
    town::TriMesh a;
    town::TriMesh b;
    town::Moments moments;

    World() : moments(&a, &b)
    {
        Floor(a, -10.0f, 16.0f);
        a.Add({-3.0f, 0.0f, 14.0f}, {-3.0f, 3.0f, 14.0f}, {3.0f, 3.0f, 14.0f}); // a wall of A across the floor at z 14
        a.Add({-3.0f, 0.0f, 14.0f}, {3.0f, 3.0f, 14.0f}, {3.0f, 0.0f, 14.0f});
        Floor(b, -10.0f, 0.0f);
        Floor(b, 6.0f, 16.0f);
        a.Build();
        b.Build();
    }
    World(const World&) = delete;
    World& operator=(const World&) = delete;
};

void TestCircle()
{
    town::Circle none;
    CHECK(!none.On() && !none.Contains({0.0f, 0.0f, 0.0f}));
    town::Circle c{{0.0f, 1.0f, 0.0f}, 2.0f};
    CHECK(c.Contains({0.0f, 2.9f, 0.0f}) && !c.Contains({0.0f, 3.1f, 0.0f}) && c.Contains({1.4f, 1.0f, 1.4f}) &&
          !c.Contains({1.5f, 1.0f, 1.5f}));
}

void TestSolid()
{
    World w;
    const Vec3 overGap{0.0f, 0.2f, 3.0f}; // a ball sunk into where the bridge of A is
    Vec3 p = overGap;
    CHECK(w.moments.PushOut(p, kR, 0) > 0.1f && Near(p.y, kR, 1e-4f)); // A: the bridge
    p = overGap;
    CHECK(w.moments.PushOut(p, kR, 1) == 0.0f); // B: the gap
    w.moments.SetCircle({{0.0f, town::kLanternUp, 3.0f}, town::kRadius});
    p = overGap;
    CHECK(w.moments.PushOut(p, kR, 1) > 0.1f && Near(p.y, kR, 1e-4f)); // B with the circle: the bridge of A
    p = overGap;
    CHECK(w.moments.PushOut(p, kR, 0) > 0.1f && Near(p.y, kR, 1e-4f)); // A with the circle: as before
    const Vec3 out{0.0f, 0.2f, 5.0f}; // over the gap, outside a circle that ends at z 4
    const town::Circle small{{0.0f, town::kLanternUp, 1.0f}, 3.0f};
    w.moments.SetCircle(small);
    Vec3 q = out;
    CHECK(w.moments.PushOut(q, kR, 1) == 0.0f);
    CHECK(w.moments.Supports({0.0f, 0.0f, 1.0f}, kR, 1) && !w.moments.Supports({0.0f, 0.0f, 5.0f}, kR, 1));
    // the wall of A: inside the circle in B it stops a ball, outside it does not
    w.moments.SetCircle({{0.0f, 1.0f, 13.0f}, 3.0f});
    Vec3 wall{0.0f, 1.0f, 13.8f};
    w.moments.PushOut(wall, kR, 1);
    CHECK(Near(wall.z, 14.0f - kR, 1e-3f));
    w.moments.SetCircle({{0.0f, 1.0f, 3.0f}, 3.0f});
    wall = {0.0f, 1.0f, 13.8f};
    w.moments.PushOut(wall, kR, 1);
    CHECK(Near(wall.z, 13.8f, 1e-4f));
}

void TestRules()
{
    World w;
    town::Lantern l;
    using R = town::Lantern::Result;
    const Vec3 north{0.0f, 0.0f, 1.0f};
    CHECK(l.Carried() && !l.GetCircle().On());
    CHECK(l.Use({0.0f, 0.0f, 2.1f}, true, north, 1, kR, w.moments) == R::OnlyInPast); // not in B
    CHECK(l.Use({0.0f, 0.0f, 2.1f}, false, north, 0, kR, w.moments) == R::NoFloor);   // not in the air
    CHECK(l.Use({0.0f, 0.0f, 2.1f}, true, north, 0, kR, w.moments) == R::Put);        // in A, standing
    CHECK(!l.Carried() && Near(l.At().z, 2.1f + town::kPutAhead, 1e-4f) && Near(l.At().y, 0.0f, 1e-4f));
    CHECK(w.moments.GetCircle().On() && Near(w.moments.GetCircle().centre.y, town::kLanternUp, 1e-4f));
    CHECK(l.Use({0.0f, 0.0f, -8.0f}, true, north, 0, kR, w.moments) == R::Nothing); // out of reach: nothing
    // in B, standing on the bridge that only the circle keeps: refused; on the floor of B in reach: taken
    CHECK(l.Use({0.0f, 0.0f, 3.0f}, true, north, 1, kR, w.moments) == R::RefusedTake && !l.Carried());
    CHECK(l.Use({0.0f, 0.0f, 1.5f}, true, north, 0, kR, w.moments) == R::Taken && l.Carried() &&
          !w.moments.GetCircle().On());
    // at an edge (nothing of A ahead): at the feet
    CHECK(l.Use({0.0f, 0.0f, 15.6f}, true, north, 0, kR, w.moments) == R::Put && Near(l.At().z, 15.6f, 1e-4f));
    CHECK(l.Use({0.0f, 0.0f, 15.0f}, true, north, 1, kR, w.moments) == R::Taken); // taken up in B, on the floor of B
    // drawn: a post and a lamp, in both moments, only while it is set down
    dl::scene::SceneObject table[4] = {};
    CHECK(l.AddDraw(table, 0, 4) == 0);
    l.Use({0.0f, 0.0f, 2.1f}, true, north, 0, kR, w.moments);
    CHECK(l.AddDraw(table, 0, 4) == 2 && table[1].glow > 1.0f && table[0].layerMask == dl::scene::kLayerBoth);
    l.Reset(w.moments);
    CHECK(l.Carried() && !w.moments.GetCircle().On());
}

/// Walks the body north from z -4 for `seconds` in `layer`; returns where its feet end.
Vec3 WalkNorth(World& w, int layer, float seconds)
{
    Player body;
    body.SetScene(nullptr, 0);
    body.SetTown(&w.moments);
    body.SetSpawn({0.0f, 0.0f, -4.0f});
    body.Respawn();
    Player::Input in;
    in.moveForward = 1.0f;
    for (int i = 0; i < static_cast<int>(seconds / kDt); ++i)
    {
        body.Update(in, {0.0f, 0.0f, 1.0f}, layer, 0.0f, kDt);
        if (body.Feet().y < -5.0f)
        {
            break;
        }
    }
    return body.Feet();
}

void TestWalk()
{
    World w;
    const Vec3 a = WalkNorth(w, 0, 2.5f);
    CHECK(a.z > 9.0f && Near(a.y, 0.0f, 0.05f)); // A: over the bridge
    const Vec3 b = WalkNorth(w, 1, 2.5f);
    CHECK(b.y < -1.0f); // B: into the gap
    town::Lantern l;
    CHECK(l.Use({0.0f, 0.0f, 2.1f}, true, {0.0f, 0.0f, 1.0f}, 0, kR, w.moments) == town::Lantern::Result::Put);
    const Vec3 c = WalkNorth(w, 1, 2.5f);
    std::printf("  with the lantern on the bridge, B: the body walked to z %.1f at y %.2f\n", static_cast<double>(c.z),
                static_cast<double>(c.y));
    CHECK(c.z > 9.0f && Near(c.y, 0.0f, 0.05f)); // B with the circle: across
}

void TestWater()
{
    town::TownData t; // a pool of A over the gap, its surface at y 1
    t.materials.push_back(town::Material{});
    const Vec3 corners[4] = {{-3.0f, 1.0f, 0.0f}, {-3.0f, 1.0f, 6.0f}, {3.0f, 1.0f, 6.0f}, {3.0f, 1.0f, 0.0f}};
    town::Polygon p;
    p.worlds = 1u;
    p.kind = town::Kind::Water;
    p.first = 0;
    p.count = 4;
    t.vertices.assign(corners, corners + 4);
    t.polygons.push_back(p);
    t.waters.push_back(0u);
    const Vec3 under{0.0f, 0.0f, 3.0f};
    const town::Circle circle{{0.0f, 0.5f, 3.0f}, town::kRadius};
    CHECK(t.Submerged(under, 0) && !t.Submerged(under, 1));
    CHECK(t.Submerged(under, 1, circle) && !t.Submerged(under, 1, town::Circle{}));
    const town::Circle small{{0.0f, 0.5f, 1.0f}, 2.0f}; // ends at z 3
    const Vec3 outside{0.0f, 0.0f, 5.0f};               // in the pool, outside that circle
    CHECK(t.Submerged(outside, 0) && !t.Submerged(outside, 1, small));
}

void TestTraces()
{
    town::Traces t;
    CHECK(t.Amount(3) == 0.0f);
    dl::scene::SceneObject o{};
    o.colorSrgb[0] = o.colorSrgb[1] = o.colorSrgb[2] = 0.2f;
    t.Mark(3, o);
    CHECK(o.glow == 0.0f && o.colorSrgb[0] == 0.2f);
    t.Changed(3);
    t.Update(0.5f * town::kTraceRise);
    const float half = t.Amount(3);
    CHECK(half > 0.3f && half < 0.7f);
    t.Changed(3); // a second change does not start it again
    t.Update(town::kTraceRise);
    CHECK(t.Amount(3) == 1.0f && t.Amount(4) == 0.0f);
    t.Mark(3, o);
    CHECK(Near(o.glow, town::kTraceGlow, 1e-6f) && o.colorSrgb[0] > o.colorSrgb[2] && o.colorSrgb[0] > 0.2f);
    t.Undo(3);
    CHECK(t.Amount(3) == 0.0f);
    t.Changed(-1);
    t.Changed(town::Traces::kMax);
    CHECK(t.Amount(-1) == 0.0f && t.Amount(town::Traces::kMax) == 0.0f);
}

} // namespace

int main()
{
    TestCircle();
    TestSolid();
    TestRules();
    TestWalk();
    TestWater();
    TestTraces();
    std::printf("%d checks passed, %d failed\n", g_passed, g_failed);
    return g_failed == 0 ? 0 : 1;
}
