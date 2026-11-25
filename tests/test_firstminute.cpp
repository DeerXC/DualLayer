// test_firstminute.cpp - the first minute of the game, proved on the town of the model (no window, no GPU). Reads
// data/town.dlt (python tools/layout/city_model.py export).
//   g++ -std=c++20 -O2 -Wall -Wextra -I src tests/test_firstminute.cpp -o test_firstminute
//   MSVC (Developer PowerShell):  .\tests\run_tests.ps1 -Only firstminute
// Why: a player who does not press a help key must still know what the game asks (docs/DESIGN.md 2, principle 5; 10.6).
// The goal is the signal mast over the bell tower with its three dark lamps. The sea wall 8 m in front of the quay
// hides the tower itself in both moments, so the test casts the rays and measures the first frame. What is checked:
//   1. the rays: from the camera of the first frame, and from the eye, every lamp is seen (nothing of the town in the
//   way) in
//      the moment the game starts in and in the other; the wall does hide the tower itself (the reason for the mast)
//   2. the first frame: for the lenses of 60, 70 and 90 degrees and the screens 16:9, 16:10 and 4:3, the three lamps,
//   the winch
//      of the sea gate and the heap of the quay are all inside the picture
//   3. the lamps: three, dark at the start, each lit by the bell of its district and no other, each with its own shape;
//   the
//      objects drawn for them, lit or dark
//   4. the compass: at the start the guidance has the winch of the sea gate for goal and the flame leans toward it, and
//   that
//      bearing is inside the first frame; the prompt of TAB is offered after 8 s of walking and not before
//   5. the first lesson can be played: from the start, in A, the winch is reached (tests/test_route.cpp proves the
//   whole way;
//      here only that the heap of B closes the way to it and A does not)
// Returns 1 when a check fails.

#include "scene/Emblems.h"
#include "scene/Guidance.h"
#include "scene/Prompts.h"
#include "scene/Things.h"
#include "scene/Town.h"
#include "scene/TownPlayer.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

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
namespace town = dl::town;
namespace tri = dl::tri;

const town::TownData& Town()
{
    static town::TownData data;
    static bool loaded = false;
    if (!loaded)
    {
        std::string error;
        loaded = data.Load(town::kPath, error);
        if (!loaded)
        {
            std::printf("  cannot load %s: %s\n", town::kPath, error.c_str());
        }
    }
    return data;
}

struct Meshes
{
    town::TriMesh a;
    town::TriMesh b;
    town::Moments moments;
    Meshes() : a(Town().BuildMesh(0)), b(Town().BuildMesh(1)), moments(&a, &b)
    {
    }
    Meshes(const Meshes&) = delete;
    Meshes& operator=(const Meshes&) = delete;
};

/// True when the segment from `from` to `to` meets a triangle of `mesh` (Moeller-Trumbore, along cells of the grid).
bool Blocked(const town::TriMesh& mesh, const Vec3& from, const Vec3& to)
{
    const Vec3 d = tri::Sub(to, from);
    const float length = std::sqrt(tri::Dot(d, d));
    bool hit = false;
    for (float t = 0.0f; t <= length && !hit; t += 2.0f)
    {
        const float f = t / length;
        mesh.ForEachNear(from.x + d.x * f, from.z + d.z * f, 3.0f,
                         [&](const Vec3& a, const Vec3& b, const Vec3& c)
                         {
                             const Vec3 e1 = tri::Sub(b, a);
                             const Vec3 e2 = tri::Sub(c, a);
                             const Vec3 h = tri::Cross(d, e2);
                             const float det = tri::Dot(e1, h);
                             if (hit || std::fabs(det) < 1.0e-9f)
                             {
                                 return;
                             }
                             const float inv = 1.0f / det;
                             const Vec3 s = tri::Sub(from, a);
                             const float u = inv * tri::Dot(s, h);
                             const Vec3 q = tri::Cross(s, e1);
                             const float v = inv * tri::Dot(d, q);
                             const float tt = inv * tri::Dot(e2, q);
                             hit = u >= 0.0f && u <= 1.0f && v >= 0.0f && u + v <= 1.0f && tt > 0.001f && tt < 0.999f;
                         });
    }
    return hit;
}

/// The camera of the first frame: the pivot over the body, and the camera behind it along the view, as far as nothing
/// is in the way (app/TownSession.cpp PlaceEye).
struct View
{
    Vec3 eye;    ///< the eye of the body
    Vec3 camera; ///< the camera
    float yaw = 0.0f;
    float pitch = 0.0f;
};

View FirstFrame(const Meshes& m)
{
    const town::TownData& t = Town();
    View v;
    v.yaw = t.spawnYaw;
    v.pitch = std::clamp(t.spawnPitch, -0.90f, 0.38f);
    v.eye = {t.spawn.x, t.spawn.y + dl::Player().GetSettings().eyeHeight, t.spawn.z};
    const Vec3 pivot{v.eye.x, v.eye.y - 0.20f, v.eye.z};
    const float cp = std::cos(v.pitch);
    const Vec3 forward{std::sin(v.yaw) * cp, std::sin(v.pitch), std::cos(v.yaw) * cp};
    const float clear = town::CameraClearDistance(m.moments, pivot, forward, 3.8f, t.startLayer, false);
    v.camera = town::Along(pivot, forward, -clear);
    return v;
}

/// Where a point is in the picture, -1 .. 1 on both axes; false when it is behind the camera.
bool Project(const View& v, const Vec3& p, float fovDegrees, float aspect, float& x, float& y)
{
    const float cp = std::cos(v.pitch);
    const float sp = std::sin(v.pitch);
    const Vec3 f{std::sin(v.yaw) * cp, sp, std::cos(v.yaw) * cp};
    const Vec3 r{std::cos(v.yaw), 0.0f, -std::sin(v.yaw)};
    const Vec3 u{-sp * std::sin(v.yaw), cp, -sp * std::cos(v.yaw)};
    const Vec3 d = tri::Sub(p, v.camera);
    const float z = tri::Dot(d, f);
    if (!(z > 0.1f))
    {
        return false;
    }
    const float t = std::tan(0.5f * fovDegrees * 0.017453292f);
    x = tri::Dot(d, r) / z / (t * aspect);
    y = tri::Dot(d, u) / z / t;
    return true;
}

int Thing(const char* name)
{
    const town::TownData& t = Town();
    for (std::size_t i = 0; i < t.things.size(); ++i)
    {
        if (t.things[i].name == name)
        {
            return static_cast<int>(i);
        }
    }
    return -1;
}

void TestRays()
{
    const town::TownData& t = Town();
    CHECK(t.emblems.size() == 3);
    static Meshes m;
    const View v = FirstFrame(m);
    std::printf("  first frame: yaw %.2f pitch %.2f, eye %.1f %.1f %.1f, camera %.1f %.1f %.1f\n",
                static_cast<double>(v.yaw), static_cast<double>(v.pitch), static_cast<double>(v.eye.x),
                static_cast<double>(v.eye.y), static_cast<double>(v.eye.z), static_cast<double>(v.camera.x),
                static_cast<double>(v.camera.y), static_cast<double>(v.camera.z));
    int seen = 0;
    for (const town::Emblem& e : t.emblems)
    {
        // the middle, the top and the bottom of the lamp, in both moments, from the camera and from the eye
        const Vec3 at[3] = {e.at, {e.at.x, e.at.y + 0.6f * e.size, e.at.z}, {e.at.x, e.at.y - 0.6f * e.size, e.at.z}};
        for (const Vec3& p : at)
        {
            for (const town::TriMesh* mesh : {&m.a, &m.b})
            {
                seen += !Blocked(*mesh, v.camera, p) && !Blocked(*mesh, v.eye, p) ? 1 : 0;
            }
        }
    }
    CHECK(seen == 3 * 3 * 2);
    // the reason for the mast: the tower itself (its south face, 30 m up) is hidden by the sea wall in both moments
    const Vec3 face{0.0f, 36.0f, 5.0f};
    CHECK(Blocked(m.b, v.eye, face) && Blocked(m.a, v.eye, face));
}

void TestFirstFrame()
{
    const town::TownData& t = Town();
    static Meshes m;
    const View v = FirstFrame(m);
    CHECK(t.spawnYaw < -0.2f && t.spawnYaw > -1.0f && t.spawnPitch > 0.05f && t.spawnPitch < 0.38f);
    // what the player should see: the three lamps (their tops and bottoms), the winch of the sea gate, the heap of the
    // quay
    std::vector<Vec3> wanted;
    for (const town::Emblem& e : t.emblems)
    {
        wanted.push_back(e.at);
        wanted.push_back({e.at.x, e.at.y + 0.65f * e.size, e.at.z});
        wanted.push_back({e.at.x, e.at.y - 0.65f * e.size, e.at.z});
    }
    const int gate = Thing("sea gate");
    CHECK(gate >= 0);
    if (gate >= 0)
    {
        wanted.push_back(t.things[static_cast<std::size_t>(gate)].handle);
    }
    wanted.push_back(
        {11.0f, t.spawn.y + 2.0f, -69.4f}); // the heap of the quay (tools/layout/city_model.py build_harbour_beats)
    float worst = 0.0f;
    int outside = 0;
    for (const float fov : {60.0f, 70.0f, 90.0f})
    {
        for (const float aspect : {16.0f / 9.0f, 16.0f / 10.0f, 4.0f / 3.0f})
        {
            for (const Vec3& p : wanted)
            {
                float x = 0.0f;
                float y = 0.0f;
                const bool in = Project(v, p, fov, aspect, x, y) && std::fabs(x) <= 0.92f && std::fabs(y) <= 0.92f;
                outside += in ? 0 : 1;
                if (in)
                {
                    worst = std::max({worst, std::fabs(x), std::fabs(y)});
                }
            }
        }
    }
    std::printf("  inside the picture for every lens and screen; the nearest to an edge: %.2f of the half\n",
                static_cast<double>(worst));
    CHECK(outside == 0);
}

void TestLamps()
{
    const town::TownData& t = Town();
    town::Things things;
    things.Init(t);
    town::Moments moments;
    town::Traces traces;
    CHECK(town::LitEmblems(t, things) == 0);
    // three shapes, three bells, none twice
    bool shapes[3] = {};
    bool bells = true;
    for (const town::Emblem& e : t.emblems)
    {
        shapes[e.shape] = true;
        bells = bells && Thing(e.bell.c_str()) >= 0;
    }
    CHECK(shapes[0] && shapes[1] && shapes[2] && bells);
    CHECK(t.emblems[0].bell != t.emblems[1].bell && t.emblems[1].bell != t.emblems[2].bell &&
          t.emblems[0].bell != t.emblems[2].bell);
    // dark lamps are drawn plain, with their collars; no glow
    dl::scene::SceneObject table[16] = {};
    const int n = town::AddEmblems(t, things, table, 0, 16, 0.0f);
    CHECK(n == town::kEmblemObjects * 3);
    float darkGlow = 0.0f;
    for (int i = 0; i < n; i += 2)
    {
        darkGlow = std::max(darkGlow, table[i].glow);
    }
    CHECK(darkGlow == 0.0f);
    CHECK(town::AddEmblems(t, things, table, 0, 5, 0.0f) == 0 &&
          town::AddEmblems(t, things, nullptr, 0, 16, 0.0f) == 0);
    // each bell lights its own lamp and no other; lit, the lamp glows and is warm
    for (std::size_t k = 0; k < t.emblems.size(); ++k)
    {
        town::Things one;
        one.Init(t);
        town::Moments m;
        town::Traces tr;
        const town::ThingDef& bell = t.things[static_cast<std::size_t>(Thing(t.emblems[k].bell.c_str()))];
        const Vec3 feet{bell.handle.x, bell.handle.y - 1.1f, bell.handle.z};
        const Vec3 none[1] = {{0.0f, -100.0f, 0.0f}};
        CHECK(one.Use(feet, none, 1, 0.35f, 0, m, tr) == town::Things::Result::Opened);
        CHECK(town::LitEmblems(t, one) == 1);
        for (std::size_t j = 0; j < t.emblems.size(); ++j)
        {
            CHECK(town::EmblemLit(t, one, t.emblems[j]) == (j == k));
        }
        dl::scene::SceneObject lit[16] = {};
        town::AddEmblems(t, one, lit, 0, 16, 0.0f);
        CHECK(lit[2 * k].glow > 1.0f && lit[2 * k].colorSrgb[0] > lit[2 * k].colorSrgb[2]);
        CHECK(lit[2 * ((k + 1) % 3)].glow == 0.0f);
    }
}

void TestCompass()
{
    const town::TownData& t = Town();
    static Meshes m;
    town::Things things;
    things.Init(t);
    town::Lantern lantern;
    town::Guidance g;
    g.Init(t, town::GuidanceSettings{});
    g.Update(0.016f, t.spawn, things, lantern, true);
    CHECK(g.Goal() == 0 && g.GoalStep() != nullptr && g.GoalStep()->name == "sea gate");
    const town::Flame f = g.FlameFrom(t.spawn);
    const Vec3 winch = g.GoalStep()->at;
    CHECK(f.on && std::fabs(f.yaw - std::atan2(winch.x - t.spawn.x, winch.z - t.spawn.z)) < 1.0e-4f);
    // the flame points into the first frame: within 45 degrees of where the camera looks
    float off = std::remainder(f.yaw - t.spawnYaw, 6.2831853f);
    CHECK(std::fabs(off) < 0.785f);
    // the prompts of the first lesson: nothing at once, TAB once the player has walked a while and has not switched
    dl::prompt::Input in;
    in.active = true;
    CHECK(dl::prompt::Choose(in).kind == dl::prompt::Kind::Count);
    in.offerSwitch = true;
    CHECK(dl::prompt::Choose(in).kind == dl::prompt::Kind::Switch);
    (void)m;
}

void TestFirstLesson()
{
    const town::TownData& t = Town();
    static Meshes m;
    // the quay: walking west from the start at the height of the chest, the heap of wreck is in the way in B and
    // nothing is in A (tests/test_things.cpp floods the whole quay for both moments; this is the heap itself)
    const float chest = t.spawn.y + 1.0f;
    int blockedB = 0;
    int blockedA = 0;
    for (const float z : {-71.0f, -69.0f, -67.0f, -66.0f})
    {
        blockedB += Blocked(m.b, {15.0f, chest, z}, {7.0f, chest, z}) ? 1 : 0;
        blockedA += Blocked(m.a, {15.0f, chest, z}, {7.0f, chest, z}) ? 1 : 0;
    }
    CHECK(blockedB >= 3 && blockedA == 0);
    CHECK(t.startLayer == 1); // the game starts in the present: the heap is the first thing seen
}

} // namespace

int main()
{
    TestRays();
    TestFirstFrame();
    TestLamps();
    TestCompass();
    TestFirstLesson();
    std::printf("%d checks passed, %d failed\n", g_passed, g_failed);
    return g_failed == 0 ? 0 : 1;
}
