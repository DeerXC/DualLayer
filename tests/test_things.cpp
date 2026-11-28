// test_things.cpp - tests of the things that change (src/scene/Things.h) and of the first two beats of the harbour, on
// the town of the model (docs/DESIGN.md 8). Reads data/town.dlt (python tools/layout/city_model.py export).
//   g++ -std=c++20 -O2 -Wall -Wextra -I src tests/test_things.cpp -o test_things
//   MSVC (Developer PowerShell):  .\tests\run_tests.ps1 -Only things
// What is checked:
//   1. the rules on a box of their own: E opens in A only; nothing happens in B; an opening into the body is refused;
//      the solid of a thing follows its state in both moments; the trace comes up in B; Reset shuts everything
//   2. the data: the game starts in B; the sea gate has a handle, and boxes of both states in both moments
//   3. beat 1, the switch: a flood of every place the body can walk to (steps up to 1.2 m, any drop that lands in the
//      map, no jump across a gap) shows that in B the start is closed off from the sea stair by the heap, and in A it
//      is not
//   4. beat 2, the causal rule: from the foot of the sea stair the top of it (the lower town) is reached in B only
//      after the gate was opened in A; never in A (the cargo), never in B with the gate shut
//   5. the same with the real body: in A it walks to the winch, E opens the gate, it switches to B where it stands, and
//      walks up through the gate; in B the handle does nothing
//   6. beat 3, the switch in the air: the lower street is closed in both moments (the flood); the real body climbs the
//      crates in A, and a jump from the top crate without a switch stays east, a jump with a switch at its top lands on
//      the heap of B and walks down its steps to the street beyond; in B alone a jump from the street does not reach
//      the heap
//   7. the ledger and the town (scene/Ledger.h, docs/DESIGN.md 3.9): the crates of the town are ballast crates, so the
//      box of B is as high as the heap that the ledger leaves, and the iron things stay open in B as A left them
// Returns 1 when a check fails.

#include "check.h"
#include "scene/Lantern.h"
#include "scene/Ledger.h"
#include "scene/Moments.h"
#include "scene/Player.h"
#include "scene/Things.h"
#include "scene/Town.h"
#include "scene/TriMesh.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <unordered_map>
#include <vector>

namespace
{

using dl::Player;
using dl::Vec3;
namespace town = dl::town;
using TR = town::Things::Result;

constexpr float kDt = 1.0f / 60.0f;

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

// ---- 1. the rules, on a thing of their own

void TestRules()
{
    town::TownData t;
    town::Material m;
    m.name = "iron";
    t.materials.push_back(m);
    town::ThingDef d;
    d.name = "door";
    d.handle = {0.0f, 1.0f, 0.0f};
    // shut: a wall across z = 3 in A and in B; open: in A a slab over the head, in B a slab at the feet (to test
    // refusal)
    d.boxes.push_back({0, 3, 0, {0.0f, 1.5f, 3.0f}, {2.0f, 1.5f, 0.1f}, 0.0f});
    d.boxes.push_back({1, 1, 0, {0.0f, 4.0f, 3.0f}, {2.0f, 0.5f, 0.1f}, 0.0f});
    d.boxes.push_back({1, 2, 0, {0.0f, 0.2f, 3.0f}, {2.0f, 0.2f, 0.1f}, 0.0f});
    t.things.push_back(d);
    town::Things things;
    things.Init(t);
    town::Moments moments;
    town::Traces traces;
    things.ApplyTo(moments);
    const Vec3 feet{0.5f, 0.0f, 0.5f};
    const Vec3 balls[3] = {{0.5f, 0.35f, 0.5f}, {0.5f, 0.95f, 0.5f}, {0.5f, 1.55f, 0.5f}};
    const auto blocked = [&](int layer)
    {
        Vec3 c{0.0f, 1.0f, 2.8f};
        return moments.PushOut(c, 0.35f, layer) > 0.01f;
    };
    CHECK(blocked(0) && blocked(1));
    CHECK(things.InReach(feet) == 0 && things.InReach({5.0f, 0.0f, 0.0f}) == -1);
    CHECK(things.Use(feet, balls, 3, 0.35f, 1, moments, traces) == TR::OnlyInPast &&
          !things.Open(0)); // nothing done in B
    CHECK(things.Use({5.0f, 0.0f, 0.0f}, balls, 3, 0.35f, 0, moments, traces) == TR::Nothing);
    CHECK(things.Use(feet, balls, 3, 0.35f, 0, moments, traces) == TR::Opened && things.Open(0));
    CHECK(!blocked(0) && !blocked(1)); // open in both moments; in B its slab lies at the feet
    Vec3 low{0.0f, 0.3f, 2.8f};
    CHECK(moments.PushOut(low, 0.35f, 1) > 0.01f);
    CHECK(things.Use(feet, balls, 3, 0.35f, 0, moments, traces) == TR::AlreadyOpen);
    CHECK(traces.Amount(0) == 0.0f);
    traces.Update(town::kTraceRise);
    CHECK(traces.Amount(0) == 1.0f);
    dl::scene::SceneObject drawn[16] = {};
    const int n = things.AddDraw(drawn, 0, 16, traces);
    int marked = 0;
    for (int i = 0; i < n; ++i)
    {
        marked += drawn[i].layerMask == dl::scene::kLayerB && drawn[i].glow > 0.0f ? 1 : 0;
    }
    CHECK(n == 2 && marked == 1); // the open boxes, no gold handle; the box of B carries the trace
    things.Reset(moments, traces);
    CHECK(!things.Open(0) && blocked(0) && traces.Amount(0) == 0.0f);
    // an opening that would put a box of A in the body is refused
    town::TownData t2 = t;
    t2.things[0].boxes[1].centre = {0.5f, 1.0f, 0.5f};
    town::Things things2;
    things2.Init(t2);
    things2.ApplyTo(moments);
    CHECK(things2.Use(feet, balls, 3, 0.35f, 0, moments, traces) == TR::Refused && !things2.Open(0));
}

// ---- the town with its things

struct World
{
    town::TriMesh a;
    town::TriMesh b;
    town::Moments moments;
    town::Things things;
    town::Traces traces;
    float radius = 0.35f;

    explicit World(const town::TownData& t) : a(t.BuildMesh(0)), b(t.BuildMesh(1)), moments(&a, &b)
    {
        things.Init(t);
        things.ApplyTo(moments);
        radius = Player().GetSettings().radius;
    }
    World(const World&) = delete;
    World& operator=(const World&) = delete;

    int Gate() const
    {
        const town::TownData& t = Town();
        for (std::size_t i = 0; i < t.things.size(); ++i)
        {
            if (t.things[i].name == "sea gate")
            {
                return static_cast<int>(i);
            }
        }
        return -1;
    }
    void OpenGate()
    {
        const town::ThingDef& d = Town().things[static_cast<std::size_t>(Gate())];
        const Vec3 feet{d.handle.x, d.handle.y - 1.1f, d.handle.z};
        const Vec3 none[1] = {{0.0f, -100.0f, 0.0f}};
        things.Use(feet, none, 1, radius, 0, moments, traces);
    }

    bool Fits(const Vec3& feet, int layer) const
    {
        for (int i = 0; i < 3; ++i)
        {
            Vec3 c{feet.x, feet.y + radius + 0.02f + 0.6f * static_cast<float>(i), feet.z};
            if (moments.PushOut(c, radius - 0.03f, layer) > 0.005f)
            {
                return false;
            }
        }
        return !Town().Submerged(feet, layer);
    }

    /// Every place the body walks to from `from` in `layer` inside the box [x0, x1] x [z0, z1]: a step up of at
    /// most 1.2 m (the jump of the main path), any drop that lands; cells of 0.5 m. True when it comes within 1 m of
    /// `to`.
    bool Reaches(const Vec3& from, const Vec3& to, int layer, float x0, float x1, float z0, float z1) const
    {
        const town::TriMesh& mesh = layer == 0 ? a : b;
        const float cell = 0.5f;
        const int nx = static_cast<int>((x1 - x0) / cell) + 1;
        const int nz = static_cast<int>((z1 - z0) / cell) + 1;
        struct Node
        {
            int i, j;
            float y;
        };
        std::unordered_map<long long, std::vector<float>> seen;
        std::vector<Node> open;
        const auto visit = [&](int i, int j, float y)
        {
            if (i < 0 || j < 0 || i >= nx || j >= nz)
            {
                return;
            }
            std::vector<float>& ys = seen[static_cast<long long>(i) * 100000 + j];
            for (float s : ys)
            {
                if (std::fabs(s - y) < 0.4f)
                {
                    return;
                }
            }
            ys.push_back(y);
            open.push_back({i, j, y});
        };
        float y0 = 0.0f;
        if (!mesh.HighestBelow(from.x, from.z, from.y + 0.5f, from.y - 2.0f, y0))
        {
            return false;
        }
        visit(static_cast<int>((from.x - x0) / cell), static_cast<int>((from.z - z0) / cell), y0);
        while (!open.empty())
        {
            const Node n = open.back();
            open.pop_back();
            const float x = x0 + static_cast<float>(n.i) * cell;
            const float z = z0 + static_cast<float>(n.j) * cell;
            if (std::fabs(x - to.x) < 1.0f && std::fabs(z - to.z) < 1.0f && std::fabs(n.y - to.y) < 2.0f)
            {
                return true;
            }
            const int di[4] = {1, -1, 0, 0};
            const int dj[4] = {0, 0, 1, -1};
            for (int k = 0; k < 4; ++k)
            {
                const float nx2 = x + static_cast<float>(di[k]) * cell;
                const float nz2 = z + static_cast<float>(dj[k]) * cell;
                float y = 0.0f;
                if (!mesh.HighestBelow(nx2, nz2, n.y + 1.2f, n.y - 12.0f, y) || y < Town().sea - 3.0f)
                {
                    continue;
                }
                // the way between: the body fits halfway at the higher of the two floors, and where it lands
                const Vec3 mid{(x + nx2) * 0.5f, std::max(n.y, y) + 0.05f, (z + nz2) * 0.5f};
                if (!Fits(mid, layer) || !Fits({nx2, y + 0.05f, nz2}, layer))
                {
                    continue;
                }
                visit(n.i + di[k], n.j + dj[k], y);
            }
        }
        return false;
    }
};

void TestData()
{
    const town::TownData& t = Town();
    CHECK(t.startLayer == 1); // the present
    World w(t);
    const int g = w.Gate();
    CHECK(g >= 0);
    if (g < 0)
    {
        return;
    }
    int kinds = 0;
    for (const town::ThingBox& b : t.things[static_cast<std::size_t>(g)].boxes)
    {
        for (int layer = 0; layer < 2; ++layer)
        {
            kinds |= (b.worlds & (1u << layer)) != 0u ? 1 << (b.state * 2 + layer) : 0;
        }
    }
    CHECK(kinds == 15); // shut and open, in A and in B
    unsigned char head[12] = {'D', 'L', 'T', 'O', 'W', 'N', 0, 0, 1, 0, 0, 0};
    town::TownData old;
    std::string error;
    CHECK(!old.Parse(head, sizeof(head), error) && error.find("version") != std::string::npos);
}

// the places of the two beats
const Vec3 kStart{14.0f, -6.2f, -72.0f};
const Vec3 kStairFoot{7.0f, -6.2f, -71.0f};
const Vec3 kLowerTown{-1.0f, -3.0f, -55.5f};

Vec3 OnFloor(const World& w, Vec3 p, int layer)
{
    float y = 0.0f;
    (layer == 0 ? w.a : w.b).HighestBelow(p.x, p.z, p.y + 3.0f, p.y - 4.0f, y);
    p.y = y;
    return p;
}

void TestBeats()
{
    const town::TownData& t = Town();
    World w(t);
    const float x0 = -40.0f, x1 = 45.0f, z0 = -90.0f, z1 = -48.0f;
    const Vec3 foot = OnFloor(w, kStairFoot, 0);
    const Vec3 top = OnFloor(w, kLowerTown, 0);
    std::printf("  stair foot y %.2f, lower town y %.2f\n", static_cast<double>(foot.y), static_cast<double>(top.y));
    // beat 1: the heap
    CHECK(!w.Reaches(kStart, foot, 1, x0, x1, z0, z1));
    CHECK(w.Reaches(kStart, foot, 0, x0, x1, z0, z1));
    // beat 2: the gate, shut
    CHECK(!w.Reaches(foot, top, 0, x0, x1, z0, z1));
    CHECK(!w.Reaches(foot, top, 1, x0, x1, z0, z1));
    w.OpenGate();
    CHECK(w.things.Open(static_cast<std::size_t>(w.Gate())));
    CHECK(!w.Reaches(foot, top, 0, x0, x1, z0, z1)); // the cargo of A
    CHECK(w.Reaches(foot, top, 1, x0, x1, z0, z1));  // the present: the cargo gone, the gate where A left it
}

struct Body
{
    World world;
    Player body;
    int layer = 0;

    explicit Body(const town::TownData& t) : world(t)
    {
        body.SetTown(&world.moments);
    }
    void PlaceAt(const Vec3& feet)
    {
        body.SetSpawn(feet);
        body.Respawn();
        for (int i = 0; i < 30; ++i)
        {
            body.Update({}, {0.0f, 0.0f, 1.0f}, layer, kDt);
        }
    }
    bool WalkTo(float x, float z)
    {
        float bestD = 1.0e9f;
        float sinceBetter = 0.0f;
        for (int i = 0; i < 60 * 40; ++i)
        {
            const float dx = x - body.Feet().x;
            const float dz = z - body.Feet().z;
            const float d = std::sqrt(dx * dx + dz * dz);
            if (d < 0.5f)
            {
                return true;
            }
            if (d < bestD - 0.05f)
            {
                bestD = d;
                sinceBetter = 0.0f;
            }
            else if ((sinceBetter += kDt) > 1.5f)
            {
                return false;
            }
            Player::Input in;
            in.moveForward = 1.0f;
            body.Update(in, {dx / d, 0.0f, dz / d}, layer, kDt);
        }
        return false;
    }
    Vec3 Balls(int i) const
    {
        return body.Ball(i);
    }
};

void TestRealBody()
{
    const town::TownData& t = Town();
    Body b(t);
    const int g = b.world.Gate();
    if (g < 0)
    {
        CHECK(false);
        return;
    }
    const town::ThingDef& gate = t.things[static_cast<std::size_t>(g)];
    const town::Street* stair = nullptr;
    for (const town::Street& s : t.streets)
    {
        stair = s.name == "sea stair" ? &s : stair;
    }
    CHECK(stair != nullptr);
    if (stair == nullptr)
    {
        return;
    }
    // in A, from the start, along the quay to the foot of the stair and up to the winch
    b.layer = 0;
    b.PlaceAt(t.spawn);
    CHECK(b.WalkTo(kStairFoot.x, kStairFoot.z));
    CHECK(b.WalkTo(gate.handle.x + 0.9f, gate.handle.z - 0.6f));
    const Vec3 balls[3] = {b.Balls(0), b.Balls(1), b.Balls(2)};
    // in B the handle does nothing
    CHECK(b.world.things.Use(b.body.Feet(), balls, 3, b.world.radius, 1, b.world.moments, b.world.traces) ==
          TR::OnlyInPast);
    CHECK(b.world.things.Use(b.body.Feet(), balls, 3, b.world.radius, 0, b.world.moments, b.world.traces) ==
          TR::Opened);
    // up the stair in A: the cargo stops it in the passage
    bool through = true;
    for (std::size_t k = 1; through && k < stair->line.size(); ++k)
    {
        if (stair->line[k].z > gate.handle.z)
        {
            through = b.WalkTo(stair->line[k].x, stair->line[k].z);
        }
    }
    CHECK(!through);
    const float stoppedZ = b.body.Feet().z;
    CHECK(stoppedZ > -64.0f && stoppedZ < -59.5f); // in the passage, behind the raised portcullis
    // the switch to B where it stands: it fits there (the portcullis of B is raised too), then up the stair
    bool fits = true;
    for (int i = 0; i < 3; ++i)
    {
        Vec3 c = b.Balls(i);
        fits = fits && b.world.moments.PushOut(c, b.world.radius - 0.03f, 1) <= 0.005f;
    }
    CHECK(fits);
    b.layer = 1;
    through = true;
    for (std::size_t k = 1; through && k < stair->line.size(); ++k)
    {
        if (stair->line[k].z > stoppedZ)
        {
            through = b.WalkTo(stair->line[k].x, stair->line[k].z);
        }
    }
    CHECK(through && b.body.Feet().z > -57.0f);
    std::printf("  the real body: through the sea gate in B, at %.1f %.1f %.1f\n", static_cast<double>(b.body.Feet().x),
                static_cast<double>(b.body.Feet().y), static_cast<double>(b.body.Feet().z));
}

/// The point `s` metres along a street, and the unit vector along it on the ground plane.
bool AlongStreet(const char* name, float s, Vec3& at, Vec3& along)
{
    for (const town::Street& st : Town().streets)
    {
        if (st.name != name)
        {
            continue;
        }
        float done = 0.0f;
        for (std::size_t i = 0; i + 1 < st.line.size(); ++i)
        {
            const Vec3& a = st.line[i];
            const Vec3& b = st.line[i + 1];
            const float l = std::sqrt((b.x - a.x) * (b.x - a.x) + (b.z - a.z) * (b.z - a.z));
            if (done + l >= s && l > 0.0f)
            {
                const float f = (s - done) / l;
                at = {a.x + (b.x - a.x) * f, a.y + (b.y - a.y) * f, a.z + (b.z - a.z) * f};
                along = {(b.x - a.x) / l, 0.0f, (b.z - a.z) / l};
                return true;
            }
            done += l;
        }
    }
    return false;
}

constexpr float kAirStepS = 36.0f; // tools/layout/city_model.py AIR_STEP_S

/// A jump along `dir` with the forward key held for `frames` steps; at step `switchAt` (if >= 0) the moment changes
/// when the body fits in the other one (as the game's guard does). Returns true when it switched.
bool JumpAlong(Body& b, const Vec3& dir, int switchAt, int frames = 90)
{
    bool switched = false;
    for (int i = 0; i < frames; ++i)
    {
        if (i == switchAt)
        {
            bool fits = true;
            for (int k = 0; k < 3; ++k)
            {
                Vec3 c = b.body.Ball(k);
                fits = fits && b.world.moments.PushOut(c, b.world.radius - 0.03f, 1 - b.layer) <= 0.005f;
            }
            if (fits)
            {
                b.layer = 1 - b.layer;
                switched = true;
            }
        }
        Player::Input in;
        in.moveForward = 1.0f;
        in.jumpPressed = i == 0;
        in.jumpHeld = true;
        b.body.Update(in, dir, b.layer, kDt);
    }
    for (int i = 0; i < 30; ++i)
    {
        b.body.Update({}, dir, b.layer, kDt);
    }
    return switched;
}

/// How far along the street the feet are from `p` (metres, + toward the market).
float UAt(const Body& b, const Vec3& p, const Vec3& v)
{
    return (b.body.Feet().x - p.x) * v.x + (b.body.Feet().z - p.z) * v.z;
}

void TestAirStep()
{
    const town::TownData& t = Town();
    Vec3 p{}, v{};
    CHECK(AlongStreet("lower street", kAirStepS, p, v));
    const Vec3 n{-v.z, 0.0f, v.x};
    const auto pt = [&](float u, float x)
    { return Vec3{p.x + v.x * u + n.x * x, p.y + 0.3f, p.z + v.z * u + n.z * x}; };
    // the flood: the street closed in both moments
    {
        World w(t);
        const Vec3 east = OnFloor(w, pt(-6.0f, 0.0f), 0);
        const Vec3 west = OnFloor(w, pt(7.0f, 0.0f), 0);
        const float x0 = std::min(east.x, west.x) - 35.0f, x1 = std::max(east.x, west.x) + 35.0f;
        const float z0 = std::min(east.z, west.z) - 35.0f, z1 = std::max(east.z, west.z) + 35.0f;
        CHECK(!w.Reaches(east, west, 0, x0, x1, z0, z1));
        CHECK(!w.Reaches(east, west, 1, x0, x1, z0, z1));
        CHECK(w.Reaches(west, OnFloor(w, pt(12.0f, 0.0f), 1), 1, x0, x1, z0, z1)); // the far side goes on to the market
        // the way to it in B: from the top of the sea stair (beat 2) along the lower street, steps of 1.2 m at most
        const Vec3 top = OnFloor(w, kLowerTown, 1);
        CHECK(w.Reaches(top, east, 1, std::min(top.x, x0) - 5.0f, std::max(top.x, x1) + 5.0f,
                        std::min(top.z, z0) - 5.0f, std::max(top.z, z1) + 5.0f));
    }
    // in B alone: a jump from the street at the heap does not get onto it
    {
        Body b(t);
        b.layer = 1;
        b.PlaceAt(pt(-3.0f, 0.0f));
        JumpAlong(b, v, -1);
        CHECK(UAt(b, p, v) < -0.3f);
    }
    // in A: up the crates; from the top one a jump without a switch stays east, with a switch it lands on the heap of B
    for (int withSwitch = 0; withSwitch < 2; ++withSwitch)
    {
        Body b(t);
        b.layer = 0;
        b.PlaceAt(pt(-5.0f, 0.6f));
        CHECK(b.body.Grounded());
        JumpAlong(b, v, -1, 22); // onto the first crate
        const float onFirst = b.body.Feet().y - p.y;
        JumpAlong(b, v, -1, 22); // onto the second
        const float onSecond = b.body.Feet().y - p.y;
        CHECK(onFirst > 0.9f && onSecond > 2.1f);
        JumpAlong(b, v, withSwitch != 0 ? 14 : -1, 40);
        const float u = UAt(b, p, v);
        const float up = b.body.Feet().y - p.y;
        std::printf("  air step, %s: u %.2f, up %.2f, moment %c\n", withSwitch != 0 ? "switch at the top" : "no switch",
                    static_cast<double>(u), static_cast<double>(up), b.layer == 0 ? 'A' : 'B');
        if (withSwitch == 0)
        {
            CHECK(u < -0.3f && b.layer == 0);
        }
        else
        {
            CHECK(b.layer == 1 && b.body.Grounded() && up > 3.0f && u > -0.4f);
            CHECK(b.WalkTo(pt(7.0f, 0.0f).x, pt(7.0f, 0.0f).z) && std::fabs(b.body.Feet().y - p.y) < 0.6f);
        }
    }
}

// ---- 7. the ledger and the town

const town::ThingDef* FindThing(const town::TownData& t, const char* name)
{
    for (const town::ThingDef& d : t.things)
    {
        if (d.name == name)
        {
            return &d;
        }
    }
    return nullptr;
}

/// The box of a thing in a state and a moment (the first), or null.
const town::ThingBox* BoxOf(const town::ThingDef& d, int state, int layer)
{
    for (const town::ThingBox& b : d.boxes)
    {
        if (b.state == state && (b.worlds & (1u << layer)) != 0u)
        {
            return &b;
        }
    }
    return nullptr;
}

void TestLedgerAgrees()
{
    namespace lg = dl::ledger;
    const town::TownData& t = Town();
    // a crate pushed in A is, in B, the stone that filled it: as high as the crate was
    for (const char* name : {"canal crate", "gatehouse crate"})
    {
        const town::ThingDef* d = FindThing(t, name);
        CHECK(d != nullptr);
        if (d == nullptr)
        {
            continue;
        }
        const town::ThingBox* a = BoxOf(*d, 0, 0);
        const town::ThingBox* pushedB = BoxOf(*d, 1, 1);
        CHECK(a != nullptr && pushedB != nullptr && d->verb == town::Verb::Push);
        if (a == nullptr || pushedB == nullptr)
        {
            continue;
        }
        const lg::Outcome heap = lg::Age(lg::DefaultTable(), lg::Subject{lg::Stuff::BallastCrate, 2.0f * a->half.y}, 0);
        std::printf("  %s: crate %.2f m, box of B %.2f m, heap of the ledger %.2f m\n", name,
                    static_cast<double>(2.0f * a->half.y), static_cast<double>(2.0f * pushedB->half.y),
                    static_cast<double>(heap.height));
        CHECK(heap.becomes == lg::Becomes::Heap && std::fabs(heap.height - 2.0f * pushedB->half.y) < 1.0e-4f);
        CHECK(std::fabs(2.0f * a->half.y - 1.2f) < 1.0e-4f); // a step of 1.2 m, the limit of docs/DESIGN.md 11
    }
    // an iron thing that was opened in A stays open in B (rust), at the sea gate and at the lift
    for (const char* name : {"sea gate", "lift"})
    {
        const town::ThingDef* d = FindThing(t, name);
        CHECK(d != nullptr);
        if (d == nullptr)
        {
            continue;
        }
        const lg::Outcome open = lg::Age(lg::DefaultTable(), lg::Subject{lg::Stuff::IronOpen, 0.0f}, 0);
        CHECK(open.becomes == lg::Becomes::StuckOpen && BoxOf(*d, 1, 1) != nullptr);
        const lg::Outcome shut = lg::Age(lg::DefaultTable(), lg::Subject{lg::Stuff::IronShut, 0.0f}, 0);
        CHECK(shut.becomes == lg::Becomes::StuckShut && BoxOf(*d, 0, 1) != nullptr);
    }
}

} // namespace

int main()
{
    TestRules();
    TestData();
    TestBeats();
    TestRealBody();
    TestAirStep();
    TestLedgerAgrees();
    std::printf("%d checks passed, %d failed\n", g_passed, g_failed);
    return g_failed == 0 ? 0 : 1;
}
