// test_shapes.cpp - tests of the vocabulary of the city (no window, no GPU): src/scene/Shapes.h with the walking body of
// src/scene/Player.h, each piece in a small room of its own.
//   g++ -std=c++20 -O2 -Wall -Wextra -I src tests/test_shapes.cpp -o test_shapes
//   MSVC (Developer PowerShell):  .\tests\run_tests.ps1 -Only shapes
// Header-only: there is no .cpp to add to the project. What is checked:
//   1. stairs: no rise above kStepRise; every step's front edge on the slope and its tread above the slope by at most one
//      rise (the body never sinks more than one rise into a step); up and down at full speed, never in the air on the way
//      down; both directions of description give the same stairs
//   2. railing: a walk into it is stopped; a running jump clears it (a railing marks an edge, a parapet seals it)
//   3. arcade: a body walks between the columns and under the lintel; it is stopped by a column
//   4. bridge: whole, both layers walk across; broken in B, A walks across and B falls into the gap
//   5. breach wall: A is stopped by the panel, B walks through the breach, B is stopped beside the breach
//   6. roof: the slopes are steeper than a body can stand on (it slides), the ridge cap is a place to stand
//   7. water: under the surface in A only; the drawn box is in A only; the surface is a slab of A
//   8. lift: a body standing still on it is carried to the top without a single frame in the air; it waits while the body is on
//      it, comes back down when it is left, and does not start for a body that is on it for less than kBoardSeconds
// Returns 1 when a check fails.

#include "scene/Player.h"
#include "scene/Shapes.h"

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

using dl::Player;
using dl::Vec3;
using namespace dl::scene;
namespace sh = dl::shapes;

constexpr float kDt = 1.0f / 120.0f;
const Vec3 kNorth{0.0f, 0.0f, 1.0f};
const Vec3 kSouth{0.0f, 0.0f, -1.0f};
const Vec3 kEast{1.0f, 0.0f, 0.0f};

bool Near(float a, float b, float eps)
{
    return std::fabs(a - b) <= eps;
}

Player::Input Keys(float forward, bool jump = false)
{
    Player::Input in;
    in.moveForward = forward;
    in.jumpPressed = jump;
    return in;
}

/// A body in a room of its own.
struct Room
{
    std::vector<SceneObject> solid;
    Player body;
    int layer = 0;

    void Floor(float x0, float z0, float x1, float z1, float top)
    {
        solid.push_back(sh::Box(x0, top - 0.2f, z0, x1, top, z1, sh::kStoneDark, kLayerBoth));
    }
    void Place(float x, float y, float z)
    {
        body.SetScene(solid.data(), static_cast<int>(solid.size()));
        body.Place({x, y + body.GetSettings().eyeHeight, z});
        Run(0.3f);
    }
    void Run(float seconds, Player::Input in = {}, const Vec3& look = kNorth)
    {
        body.SetScene(solid.data(), static_cast<int>(solid.size()));
        const int n = static_cast<int>(std::lround(seconds / kDt));
        for (int i = 0; i < n; ++i)
        {
            body.Update(in, look, layer, 0.0f, kDt);
            in.jumpPressed = false;
        }
    }
};

void TestStairs()
{
    std::vector<SceneObject> solid;
    std::vector<SceneObject> drawn;
    const int n = sh::Stairs(-1.0f, 1.0f, 0.0f, 0.0f, 8.0f, 3.6f, sh::kStoneLight, solid, drawn);
    CHECK(n == 12 && solid.size() == 1 && static_cast<int>(drawn.size()) == n);
    const float slope = 3.6f / 8.0f;
    for (int i = 0; i < n; ++i)
    {
        const SceneObject& s = drawn[static_cast<std::size_t>(i)];
        const float top = s.position[1] + 0.5f * s.scale[1];
        const float front = s.position[2] - 0.5f * s.scale[2];
        const float back = s.position[2] + 0.5f * s.scale[2];
        CHECK(Near(top - static_cast<float>(i) * 0.3f, 0.3f, 1e-4f));      // each one rise higher
        CHECK(Near(front * slope, top - 0.3f, 1e-3f));                     // its front edge on the slope (at the rise below)
        CHECK(top - front * slope <= sh::kStepRise + 1e-3f && back * slope <= top + 1e-3f); // never more than a rise above it
    }
    std::vector<SceneObject> solid2;
    std::vector<SceneObject> drawn2;
    sh::Stairs(-1.0f, 1.0f, 8.0f, 3.6f, 0.0f, 0.0f, sh::kStoneLight, solid2, drawn2); // described from the head
    bool same = drawn2.size() == drawn.size();
    for (std::size_t i = 0; same && i < drawn.size(); ++i)
    {
        same = Near(drawn[i].position[2], drawn2[i].position[2], 1e-4f) && Near(drawn[i].position[1], drawn2[i].position[1], 1e-4f);
    }
    CHECK(same && Near(solid[0].tilt, solid2[0].tilt, 1e-5f));
    // walked: up and down at full speed, never in the air on the way down
    Room r;
    r.Floor(-3.0f, -4.0f, 3.0f, 0.0f, 0.0f);
    r.Floor(-3.0f, 8.0f, 3.0f, 12.0f, 3.6f);
    r.solid.push_back(solid[0]);
    r.Place(0.0f, 0.0f, -2.0f);
    for (int i = 0; i < 600 && r.body.Feet().z < 9.5f; ++i)
    {
        r.Run(kDt, Keys(1.0f), kNorth);
    }
    r.Run(0.3f);
    CHECK(r.body.Feet().z > 9.0f && Near(r.body.Feet().y, 3.6f, 0.03f));
    int air = 0;
    for (int i = 0; i < 360 && r.body.Feet().z > -1.0f; ++i)
    {
        r.Run(kDt, Keys(1.0f), kSouth);
        air += r.body.Grounded() ? 0 : 1;
    }
    CHECK(air == 0 && Near(r.body.Feet().y, 0.0f, 0.03f));
}

void TestRailing()
{
    Room r;
    r.Floor(-5.0f, -5.0f, 5.0f, 5.0f, 0.0f);
    const int n = sh::Railing(-4.0f, 2.0f, 4.0f, 2.0f, 0.0f, kLayerBoth, r.solid);
    CHECK(n == 5 + 1);
    r.Place(0.3f, 0.0f, 0.0f); // between two posts: the rail stops the walk
    r.Run(2.0f, Keys(1.0f), kNorth);
    CHECK(r.body.Feet().z < 2.0f - 0.08f - 0.3f);
    Room j = r; // a running jump clears it
    j.Place(0.3f, 0.0f, -2.0f);
    j.Run(0.2f, Keys(1.0f), kNorth);
    j.Run(kDt, Keys(1.0f, true), kNorth);
    j.Run(1.2f, Keys(1.0f), kNorth);
    CHECK(j.body.Feet().z > 2.5f);
}

void TestArcade()
{
    Room r;
    r.Floor(-5.0f, -5.0f, 5.0f, 10.0f, 0.0f);
    const int n = sh::Arcade(-0.3f, 0.3f, 0.0f, 6.0f, 0.0f, 3.0f, 2.6f, r.solid);
    CHECK(n == 3 + 1);
    r.Place(-2.0f, 0.0f, 1.5f); // between two columns, under the lintel: through
    r.Run(0.7f, Keys(1.0f), kEast);
    CHECK(r.body.Feet().x > 1.5f && r.body.Grounded());
    r.Place(-2.0f, 0.0f, 3.0f); // at a column: stopped
    r.Run(0.7f, Keys(1.0f), kEast);
    CHECK(r.body.Feet().x < -0.3f - 0.3f);
}

void TestBridge()
{
    for (int layer = 0; layer < 2; ++layer)
    {
        Room whole;
        whole.Floor(-3.0f, -4.0f, 3.0f, 0.0f, 0.0f);
        whole.Floor(-3.0f, 10.0f, 3.0f, 14.0f, 0.0f);
        CHECK(sh::Bridge(-1.0f, 1.0f, 0.0f, 10.0f, 0.0f, false, 0.0f, whole.solid) == 1);
        whole.layer = layer;
        whole.Place(0.0f, 0.0f, -2.0f);
        whole.Run(2.6f, Keys(1.0f), kNorth);
        CHECK(whole.body.Feet().z > 11.0f && whole.body.Grounded());
        Room broken;
        broken.Floor(-3.0f, -4.0f, 3.0f, 0.0f, 0.0f);
        broken.Floor(-3.0f, 10.0f, 3.0f, 14.0f, 0.0f);
        CHECK(sh::Bridge(-1.0f, 1.0f, 0.0f, 10.0f, 0.0f, true, 5.0f, broken.solid) == 3);
        broken.layer = layer;
        broken.Place(0.0f, 0.0f, -2.0f);
        float lowest = 0.0f; // (a long fall ends in a respawn: the lowest point tells the fall)
        for (int i = 0; i < 312; ++i)
        {
            broken.Run(kDt, Keys(1.0f), kNorth);
            lowest = broken.body.Feet().y < lowest ? broken.body.Feet().y : lowest;
        }
        if (layer == 0)
        {
            CHECK(broken.body.Feet().z > 11.0f && broken.body.Grounded()); // A: the structure is the way
        }
        else
        {
            CHECK(lowest < -1.0f); // B: the bridge is broken
        }
    }
}

void TestBreachWall()
{
    Room base;
    base.Floor(-5.0f, -5.0f, 5.0f, 5.0f, 0.0f);
    CHECK(sh::BreachWall(-5.0f, 5.0f, 2.0f, 0.5f, 0.0f, 3.5f, -1.5f, 1.5f, base.solid) == 5);
    Room a = base;
    a.layer = 0;
    a.Place(0.0f, 0.0f, -2.0f);
    a.Run(2.0f, Keys(1.0f), kNorth);
    CHECK(a.body.Feet().z < 2.0f - 0.25f - 0.3f); // A: the panel
    Room b = base;
    b.layer = 1;
    b.Place(0.0f, 0.0f, -2.0f);
    b.Run(2.0f, Keys(1.0f), kNorth);
    CHECK(b.body.Feet().z > 3.0f); // B: through the breach
    Room side = base;
    side.layer = 1;
    side.Place(3.0f, 0.0f, -2.0f);
    side.Run(2.0f, Keys(1.0f), kNorth);
    CHECK(side.body.Feet().z < 2.0f - 0.25f - 0.3f); // B: beside the breach the wall holds
}

void TestRoof()
{
    std::vector<SceneObject> drawn;
    CHECK(sh::Roof(-4.0f, 4.0f, 0.0f, 4.0f, 3.0f, 6.0f, drawn) == 3);
    CHECK(std::fabs(drawn[0].tilt) > 0.93f); // 56 degrees: steeper than the 53 a body stands on
    Room r;
    r.Floor(-8.0f, -8.0f, 8.0f, 12.0f, 0.0f);
    r.solid.push_back(drawn[0]); // the south slope, as if it were solid
    r.Place(0.0f, 4.5f, 1.0f);    // dropped onto it
    r.Run(2.0f);
    CHECK(r.body.Feet().y < 0.1f); // it slid off
    Room cap;
    cap.solid.push_back(drawn[2]);
    cap.Place(0.0f, 6.15f, 2.0f);
    cap.Run(1.0f);
    CHECK(cap.body.Grounded() && Near(cap.body.Feet().y, 6.15f, 0.02f));
}

void TestWater()
{
    const sh::Water w{-2.0f, -2.0f, 2.0f, 2.0f, -0.4f, 2.0f};
    CHECK(sh::Submerged(w, {0.0f, -1.0f, 0.0f}, 0));
    CHECK(!sh::Submerged(w, {0.0f, -1.0f, 0.0f}, 1));   // B: the water has gone
    CHECK(!sh::Submerged(w, {0.0f, -0.5f, 0.0f}, 0));   // feet just at the surface: not yet
    CHECK(!sh::Submerged(w, {3.0f, -1.0f, 0.0f}, 0));   // beside it
    const SceneObject box = sh::WaterBox(w);
    const SceneObject surface = sh::WaterSurface(w);
    CHECK(box.layerMask == kLayerA && surface.layerMask == kLayerA && box.glow > 0.0f);
    CHECK(Near(box.position[1] + 0.5f * box.scale[1], w.top, 1e-5f) && Near(surface.position[1] + 0.5f * surface.scale[1], w.top, 1e-5f));
}

void TestLift()
{
    Room r;
    r.Floor(-5.0f, -5.0f, 5.0f, 0.0f, 0.0f);
    r.Floor(-5.0f, 4.0f, 5.0f, 8.0f, 6.0f);
    sh::Lift lift(-1.0f, 1.0f, 0.0f, 3.5f, 0.0f, 6.0f);
    const auto frame = [&](Player::Input in, const Vec3& look) {
        std::vector<SceneObject> solids = r.solid;
        const float moved = lift.Update(r.body.Feet(), r.body.Grounded(), kDt);
        solids.push_back(lift.Platform());
        if (lift.OnIt({r.body.Feet().x, r.body.Feet().y + moved, r.body.Feet().z}, r.body.Grounded()))
        {
            r.body.Carry({0.0f, moved, 0.0f});
        }
        r.body.SetScene(solids.data(), static_cast<int>(solids.size()));
        r.body.Update(in, look, 0, 0.0f, kDt);
    };
    // a body that only passes over it (on it for less than kBoardSeconds) does not start it
    r.body.Place({0.0f, 0.0f + 1.55f, 0.7f});
    for (int i = 0; i < 14; ++i) // about 0.12 s standing
    {
        frame({}, kSouth);
    }
    for (int i = 0; i < 60; ++i) // then off it, south, onto the floor
    {
        frame(Keys(1.0f), kSouth);
    }
    CHECK(lift.AtBottom() && r.body.Feet().z < -0.5f);
    // a body standing still on it rides up without a frame in the air
    r.body.Place({0.0f, 0.0f + 1.55f, 2.0f});
    int air = 0;
    for (int i = 0; i < 600 && !lift.AtTop(); ++i)
    {
        frame({}, kNorth);
        air += (i > 30 && !r.body.Grounded()) ? 1 : 0;
    }
    CHECK(lift.AtTop() && air == 0 && Near(r.body.Feet().y, 6.0f, 0.05f));
    for (int i = 0; i < 300; ++i) // it waits while the body is on it
    {
        frame({}, kNorth);
    }
    CHECK(lift.AtTop());
    for (int i = 0; i < 120; ++i) // off onto the upper floor
    {
        frame(Keys(1.0f), kNorth);
    }
    CHECK(r.body.Feet().z > 4.2f && Near(r.body.Feet().y, 6.0f, 0.05f));
    for (int i = 0; i < 1200 && !lift.AtBottom(); ++i) // left alone: it comes back down
    {
        frame({}, kNorth);
    }
    CHECK(lift.AtBottom());
    // locked, it does not start; a body under a raised lift keeps it up
    sh::Lift locked(-1.0f, 1.0f, 0.0f, 3.5f, 0.0f, 6.0f);
    for (int i = 0; i < 240; ++i)
    {
        locked.Update({0.0f, 0.0f, 2.0f}, true, kDt, false);
    }
    CHECK(locked.AtBottom());
    sh::Lift up(-1.0f, 1.0f, 0.0f, 3.5f, 0.0f, 6.0f);
    for (int i = 0; i < 600 && !up.AtTop(); ++i)
    {
        up.Update({0.0f, up.Height(), 2.0f}, true, kDt);
    }
    CHECK(up.AtTop());
    for (int i = 0; i < 600; ++i) // left, but a body stands on the floor under it
    {
        up.Update({0.0f, 0.0f, 2.0f}, true, kDt);
    }
    CHECK(up.AtTop());
    for (int i = 0; i < 1200 && !up.AtBottom(); ++i) // the body has gone: it comes down
    {
        up.Update({0.0f, 0.0f, -3.0f}, true, kDt);
    }
    CHECK(up.AtBottom());
}

} // namespace

int main()
{
    TestStairs();
    TestRailing();
    TestArcade();
    TestBridge();
    TestBreachWall();
    TestRoof();
    TestWater();
    TestLift();
    std::printf("%d checks passed, %d failed\n", g_passed, g_failed);
    return g_failed == 0 ? 0 : 1;
}
