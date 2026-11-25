// test_guidance.cpp - tests of the help of the game, the lantern as compass (no window, no GPU): src/scene/Guidance.h
// and src/scene/Flame.h, on a small town made here:
//   three steps: a door (a thing, handle at z 20), the lantern (a platform at z 40), a bell (a thing at z 60); a mast
//   with three emblems over x 0, z 90. g++ -std=c++20 -O2 -Wall -Wextra -I src tests/test_guidance.cpp -o test_guidance
//   MSVC (Developer PowerShell):  .\tests\run_tests.ps1 -Only guidance
// What is checked:
//   1. the goal: the first step that is not done, in order; a thing is done when it is open; the lantern when it has
//   stood at
//      its spot (and stays done when it is taken up); the signal mast when every step is done; none when the guidance
//      is off
//   2. the flame: on, leaning toward the goal (its bearing) by leanDegrees; restless (further, with sparks) after
//   idleSeconds
//      without progress; calm again after progress; upright at the goal; the same for the lantern set down
//   3. progress: a body that keeps reaching new ground is never idle; one that stands or goes round in one place is; a
//   step
//      done starts the count again
//   4. H: the first press raises the lantern and sweeps a beam for sweepSeconds; a second press puts it away unless the
//   player
//      has been stuck for ghostAfterSeconds, when it shows a frame round the thing and a line to it for ghostSeconds; a
//      step done puts everything away; nothing without a goal or without room in the table
//   5. the flame as objects: the plume leans along its bearing, the sparks only when restless; the avatar and the
//   set-down
//      lantern carry it
// Returns 1 when a check fails.

#include "scene/Guidance.h"
#include "scene/Lantern.h"
#include "scene/Moments.h"
#include "scene/Things.h"
#include "scene/Town.h"
#include "scene/TownPlayer.h"
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

using dl::Vec3;
using namespace dl::scene;
namespace tn = dl::town;

constexpr float kR = 0.35f; // the radius of the body

bool Near(float a, float b, float eps)
{
    return std::fabs(a - b) <= eps;
}

float Degrees(float radians)
{
    return radians * 57.29578f;
}

/// A town of three steps. The door and the bell are things with one box each; the platform of the lantern is a floor of
/// A.
struct World
{
    tn::TownData town;
    tn::TriMesh a;
    tn::TriMesh b;
    tn::Moments moments;
    tn::Things things;
    tn::Traces traces;
    tn::Lantern lantern;
    tn::Guidance guidance;

    explicit World(const tn::GuidanceSettings& settings = {}) : moments(&a, &b)
    {
        town.materials.push_back(tn::Material{});
        for (const auto& [name, z] : {std::pair<const char*, float>{"door", 20.0f}, {"bell", 60.0f}})
        {
            tn::ThingDef d;
            d.name = name;
            d.handle = {0.0f, 1.1f, z};
            d.boxes.push_back({1, 3, 0, {0.0f, 1.0f, z + 3.0f}, {1.0f, 1.0f, 0.2f}, 0.0f});
            town.things.push_back(d);
        }
        town.steps.push_back({"door", tn::StepKind::Thing, {0.0f, 1.1f, 20.0f}});
        town.steps.push_back({"lantern", tn::StepKind::Lantern, {10.0f, 3.0f, 40.0f}});
        town.steps.push_back({"bell", tn::StepKind::Thing, {0.0f, 1.1f, 60.0f}});
        for (const char* glyph : {"well", "cliff", "lock"})
        {
            tn::Emblem e;
            e.glyph = glyph;
            e.at = {0.0f, 60.0f, 90.0f};
            town.emblems.push_back(e);
        }
        a.Add({-15.0f, 3.0f, 35.0f}, {-15.0f, 3.0f, 45.0f}, {15.0f, 3.0f, 45.0f}); // the platform of the lantern, in A
        a.Add({-15.0f, 3.0f, 35.0f}, {15.0f, 3.0f, 45.0f}, {15.0f, 3.0f, 35.0f});
        a.Build();
        b.Build();
        things.Init(town);
        things.ApplyTo(moments);
        guidance.Init(town, settings);
    }
    World(const World&) = delete;
    World& operator=(const World&) = delete;

    void Open(int thing)
    {
        const tn::ThingDef& d = town.things[static_cast<std::size_t>(thing)];
        const Vec3 feet{d.handle.x, d.handle.y - 1.1f, d.handle.z};
        const Vec3 balls[1] = {{0.0f, -100.0f, 0.0f}};
        things.Use(feet, balls, 1, kR, 0, moments, traces);
    }

    /// Runs `seconds` in steps of 0.1 s with the body at `feet`.
    void Stand(float seconds, const Vec3& feet, bool walking = true)
    {
        for (int i = 0; i < static_cast<int>(std::lround(seconds * 10.0f)); ++i)
        {
            guidance.Update(0.1f, feet, things, lantern, walking);
        }
    }
};

void TestGoal()
{
    World w;
    const Vec3 at{0.0f, 0.0f, 0.0f};
    w.guidance.Update(0.1f, at, w.things, w.lantern, true);
    CHECK(w.guidance.Goal() == 0 && w.guidance.Done() == 0 && w.guidance.GoalStep()->name == "door");
    w.Open(0);
    w.guidance.Update(0.1f, at, w.things, w.lantern, true);
    CHECK(w.guidance.Goal() == 1 && w.guidance.Done() == 1 && w.guidance.GoalStep()->kind == tn::StepKind::Lantern);
    // the lantern set down far from its spot is not the step; at its spot it is, and it stays done when it is taken up
    const Vec3 north{0.0f, 0.0f, 1.0f};
    CHECK(w.lantern.Use({0.0f, 3.0f, 36.0f}, true, north, 0, kR, w.moments) == tn::Lantern::Result::Put);
    w.guidance.Update(0.1f, at, w.things, w.lantern, true);
    CHECK(w.guidance.Goal() == 1);
    w.lantern.Reset(w.moments);
    CHECK(w.lantern.Use({10.0f, 3.0f, 38.5f}, true, north, 0, kR, w.moments) == tn::Lantern::Result::Put);
    w.guidance.Update(0.1f, at, w.things, w.lantern, true);
    CHECK(w.guidance.Goal() == 2 && w.guidance.Done() == 2);
    w.lantern.Reset(w.moments);
    w.guidance.Update(0.1f, at, w.things, w.lantern, true);
    CHECK(w.guidance.Goal() == 2); // taken up again: still done
    w.Open(1);
    w.guidance.Update(0.1f, at, w.things, w.lantern, true);
    CHECK(w.guidance.Goal() == -1 && w.guidance.GoalStep() == nullptr && w.guidance.Done() == 3);
    // every step done: the flame leans toward the signal mast over the tower
    Vec3 target;
    CHECK(w.guidance.Target(target) && Near(target.z, 90.0f, 1e-4f));
    const tn::Flame f = w.guidance.FlameFrom({0.0f, 0.0f, 0.0f});
    CHECK(f.on && Near(f.yaw, 0.0f, 1e-4f));
    tn::GuidanceSettings off;
    off.enabled = false;
    World o(off);
    o.guidance.Update(0.1f, at, o.things, o.lantern, true);
    CHECK(o.guidance.Goal() == -1 && !o.guidance.FlameFrom(at).on);
    o.guidance.Press();
    CHECK(o.guidance.GetTier() == tn::Guidance::Tier::None);
}

void TestFlame()
{
    World w;
    const Vec3 start{0.0f, 0.0f, 0.0f}; // the door is 20 m north
    w.guidance.Update(0.1f, start, w.things, w.lantern, true);
    // always on, leaning toward the goal; it gets to the lean by easing
    w.Stand(4.0f, start);
    tn::Flame f = w.guidance.FlameFrom(start);
    CHECK(f.on && Near(f.yaw, 0.0f, 1e-4f));
    CHECK(Near(Degrees(f.tilt), 18.0f, 1.0f) && f.sparks < 0.1f);
    // the bearing follows where the body is: the goal at (0, 20) seen from the east is to the north-west
    f = w.guidance.FlameFrom({20.0f, 0.0f, 20.0f});
    CHECK(Near(f.yaw, -1.5707963f, 1e-3f));
    f = w.guidance.FlameFrom({0.0f, 0.0f, 40.0f});
    CHECK(Near(std::fabs(f.yaw), 3.1415927f, 1e-3f)); // the goal is behind
    // at the goal the flame stands upright; two metres off it leans again
    f = w.guidance.FlameFrom({0.0f, 0.0f, 19.5f});
    CHECK(f.tilt < 0.02f && f.sparks < 0.02f);
    f = w.guidance.FlameFrom({0.0f, 0.0f, 14.0f});
    CHECK(Near(Degrees(f.tilt), 18.0f, 1.0f));
    // restless after 30 s without progress: further, and with sparks
    w.Stand(34.0f, start);
    CHECK(w.guidance.IdleSeconds() > 30.0f);
    f = w.guidance.FlameFrom(start);
    CHECK(Near(Degrees(f.tilt), 40.0f, 1.5f) && f.sparks > 0.9f);
    // progress calms it: new ground
    for (int i = 1; i <= 6; ++i)
    {
        w.Stand(1.0f, {0.0f, 0.0f, 7.0f * static_cast<float>(i) * 0.3f});
    }
    w.Stand(6.0f, {0.0f, 0.0f, 6.0f});
    f = w.guidance.FlameFrom(start);
    CHECK(Near(Degrees(f.tilt), 18.0f, 1.5f) && f.sparks < 0.1f);
    // time stands still while the body does not walk (the map is open)
    World p;
    p.guidance.Update(0.1f, start, p.things, p.lantern, true);
    p.Stand(100.0f, start, false);
    CHECK(p.guidance.IdleSeconds() < 1.0f);
}

void TestProgress()
{
    {
        World w; // always new ground: never idle
        const Vec3 begin{0.0f, 0.0f, -60.0f};
        w.guidance.Update(0.1f, begin, w.things, w.lantern, true);
        float idle = 0.0f;
        for (int step = 0; step < 40; ++step)
        {
            w.Stand(5.0f, {0.0f, 0.0f, -60.0f + 7.0f * static_cast<float>(step)});
            idle = std::max(idle, w.guidance.IdleSeconds());
        }
        CHECK(idle < 6.0f);
    }
    {
        World w; // going round in one cell is not progress
        const Vec3 begin{0.0f, 0.0f, 0.0f};
        w.guidance.Update(0.1f, begin, w.things, w.lantern, true);
        w.Stand(2.0f, begin);
        for (int i = 0; i < 40; ++i)
        {
            w.Stand(1.0f, {static_cast<float>(i % 2) * 2.0f, 0.0f, static_cast<float>((i / 2) % 2) * 2.0f});
        }
        CHECK(w.guidance.IdleSeconds() > 35.0f);
    }
    {
        World w; // a step done: the count starts again, and what was shown is put away
        const Vec3 at{0.0f, 0.0f, 0.0f};
        w.guidance.Update(0.1f, at, w.things, w.lantern, true);
        w.Stand(40.0f, at);
        w.guidance.Press();
        CHECK(w.guidance.GetTier() == tn::Guidance::Tier::Sweep && w.guidance.IdleSeconds() > 30.0f);
        w.Open(0);
        w.Stand(0.2f, at);
        CHECK(w.guidance.Goal() == 1 && w.guidance.GetTier() == tn::Guidance::Tier::None &&
              w.guidance.IdleSeconds() < 1.0f);
    }
}

void TestPress()
{
    const Vec3 at{0.0f, 0.0f, 0.0f};
    SceneObject table[64] = {};
    {
        World w; // first press: the lantern is raised, a beam sweeps, and it ends by itself after 6 s
        w.guidance.Update(0.1f, at, w.things, w.lantern, true);
        CHECK(w.guidance.GetTier() == tn::Guidance::Tier::None && w.guidance.AddDraw(table, 0, 64, 0.0f, at) == 0);
        w.guidance.Press();
        CHECK(w.guidance.GetTier() == tn::Guidance::Tier::Sweep);
        w.Stand(1.0f, at);
        CHECK(w.guidance.Raise() > 0.9f);
        const int n = w.guidance.AddDraw(table, 0, 64, 1.0f, {0.0f, 1.3f, 0.0f});
        CHECK(n == tn::Guidance::kBeamObjects);
        // the beam settles on the bearing of the goal (here north): its far end is nearer the goal than its start
        w.Stand(4.5f, at);
        const int late = w.guidance.AddDraw(table, 0, 64, 5.5f, {0.0f, 1.3f, 0.0f});
        CHECK(late == tn::Guidance::kBeamObjects && std::fabs(table[late - 1].position[0]) < 2.0f &&
              table[late - 1].position[2] > 10.0f);
        w.Stand(1.5f, at);
        CHECK(w.guidance.GetTier() == tn::Guidance::Tier::None && w.guidance.Raise() < 0.5f);
        w.Stand(2.0f, at);
        CHECK(w.guidance.Raise() < 0.05f);
        // a second press during the sweep, before the player has been stuck long enough: put away, not the frame
        w.guidance.Press();
        w.Stand(0.5f, at);
        w.guidance.Press();
        CHECK(w.guidance.GetTier() == tn::Guidance::Tier::None);
        w.guidance.Press(); // and a press again starts the sweep again
        CHECK(w.guidance.GetTier() == tn::Guidance::Tier::Sweep);
    }
    {
        World w; // stuck for 90 s: the second press shows the thing, a frame round the handle and a line to it
        w.guidance.Update(0.1f, at, w.things, w.lantern, true);
        w.Stand(95.0f, at);
        w.guidance.Press();
        w.guidance.Press();
        CHECK(w.guidance.GetTier() == tn::Guidance::Tier::Ghost);
        const Vec3 hand{0.0f, 1.3f, 0.0f};
        const int n = w.guidance.AddDraw(table, 0, 64, 0.0f, hand);
        CHECK(n == tn::Guidance::kGhostObjects);
        // the frame: 12 thin edges within a metre of the handle (0, 0.7, 20); the line: motes between the hand and it
        int edges = 0;
        int motes = 0;
        for (int i = 0; i < n; ++i)
        {
            const bool thin = table[i].scale[0] < 0.1f || table[i].scale[1] < 0.1f || table[i].scale[2] < 0.1f;
            const float dz = std::fabs(table[i].position[2] - 20.0f);
            if (thin && dz <= 1.0f && table[i].scale[0] + table[i].scale[1] + table[i].scale[2] > 1.0f)
            {
                ++edges;
            }
            else if (table[i].scale[0] < 0.1f && table[i].position[2] > 0.0f && table[i].position[2] < 20.0f)
            {
                ++motes;
            }
        }
        CHECK(edges == 12 && motes == 10);
        w.Stand(13.0f, at);
        CHECK(w.guidance.GetTier() == tn::Guidance::Tier::None);
        w.guidance.Press();
        w.guidance.Press();
        CHECK(w.guidance.GetTier() == tn::Guidance::Tier::Ghost); // still stuck: it can be shown again
        w.guidance.Press();
        CHECK(w.guidance.GetTier() == tn::Guidance::Tier::None); // a press while it shows puts it away
        // the frame of the lantern step stands on the platform
        w.Open(0);
        w.guidance.Update(0.1f, at, w.things, w.lantern, true);
        w.Stand(95.0f, at);
        w.guidance.Press();
        w.guidance.Press();
        w.guidance.AddDraw(table, 0, 64, 0.0f, hand);
        CHECK(std::fabs(table[0].position[2] - 40.0f) <= 1.0f);
    }
    {
        World w; // nothing without a goal, nothing without room
        w.Open(0);
        w.Open(1);
        w.guidance.Update(0.1f, at, w.things, w.lantern, true); // the lantern step is not done: still a goal
        CHECK(w.guidance.Goal() == 1);
        w.guidance.Press();
        CHECK(w.guidance.AddDraw(table, 60, 64, 0.0f, at) == 60 && w.guidance.AddDraw(nullptr, 0, 64, 0.0f, at) == 0);
        w.lantern.Use({10.0f, 3.0f, 38.5f}, true, {0.0f, 0.0f, 1.0f}, 0, kR, w.moments);
        w.guidance.Update(0.1f, at, w.things, w.lantern, true);
        CHECK(w.guidance.Goal() == -1);
        w.guidance.Press();
        CHECK(w.guidance.GetTier() == tn::Guidance::Tier::None);
    }
}

void TestFlameObjects()
{
    SceneObject table[40] = {};
    tn::Flame f;
    f.on = true;
    f.yaw = 0.5f;
    f.tilt = 0.4f;
    f.sparks = 0.0f;
    const Vec3 base{10.0f, 2.0f, 5.0f};
    CHECK(tn::AddFlame(table, 0, 40, base, tn::Flame{}, 0.0f) == 0); // off: nothing
    CHECK(tn::AddFlame(table, 38, 40, base, f, 0.0f) == 38 && tn::AddFlame(nullptr, 0, 40, base, f, 0.0f) == 0);
    const int n = tn::AddFlame(table, 0, 40, base, f, 0.0f);
    CHECK(n == tn::kFlameObjects);
    // the plume leans along its bearing: from its base to its middle it goes toward (sin yaw, cos yaw) and up
    const float dx = table[0].position[0] - base.x;
    const float dy = table[0].position[1] - base.y;
    const float dz = table[0].position[2] - base.z;
    CHECK(Near(std::atan2(dx, dz), 0.5f, 1e-3f) && dy > 0.0f &&
          Near(std::sqrt(dx * dx + dy * dy + dz * dz), 0.5f * tn::kFlameLength, 1e-3f));
    CHECK(Near(table[0].tilt, 0.4f, 1e-6f) && Near(table[0].yaw, 0.5f, 1e-6f) && table[0].glow > 1.0f);
    CHECK(table[1].scale[0] < 0.001f); // no sparks while it is calm: too small to be seen
    f.sparks = 1.0f;
    tn::AddFlame(table, 0, 40, base, f, 0.3f);
    CHECK(table[1].scale[0] > 0.01f && table[1].glow > 1.0f && table[1].position[1] > base.y);
    // the set-down lantern carries it
    World w;
    w.lantern.Use({10.0f, 3.0f, 38.5f}, true, {0.0f, 0.0f, 1.0f}, 0, kR, w.moments);
    SceneObject lamp[16] = {};
    CHECK(w.lantern.AddDraw(lamp, 0, 16) == 2);
    CHECK(w.lantern.AddDraw(lamp, 0, 16, &f, 0.0f) == 2 + tn::kFlameObjects);
    CHECK(w.lantern.AddDraw(lamp, 0, 4, &f, 0.0f) == 2); // no room for the flame: the lantern alone
    // the avatar carries it, and the arm goes up with the lantern
    dl::Player player;
    SceneObject avatar[40] = {};
    const int plain = tn::AddTownAvatar(avatar, 0, 40, player, 0.0f, 0.0f, 0.0f, true);
    CHECK(plain == 19);
    const float lampLow = avatar[17].position[1];
    const int with = tn::AddTownAvatar(avatar, 0, 40, player, 0.0f, 0.0f, 0.0f, true, &f, 1.0f, 0.0f);
    CHECK(with == 19 + tn::kFlameObjects && avatar[17].position[1] > lampLow + 0.3f);
    CHECK(tn::AddTownAvatar(avatar, 0, 40, player, 0.0f, 0.0f, 0.0f, false, &f, 1.0f, 0.0f) ==
          15); // nothing in the hand: no flame
}

} // namespace

int main()
{
    TestGoal();
    TestFlame();
    TestProgress();
    TestPress();
    TestFlameObjects();
    std::printf("%d checks passed, %d failed\n", g_passed, g_failed);
    return g_failed == 0 ? 0 : 1;
}
