// test_mechanisms.cpp - tests of the moving parts (no window, no GPU): src/scene/Mechanisms.h with the walking body of
// src/scene/Player.h, on a small room of its own.
//   g++ -std=c++20 -O2 -Wall -Wextra -I src tests/test_mechanisms.cpp -o test_mechanisms
//   MSVC (Developer PowerShell):  .\tests\run_tests.ps1 -Only mechanisms
// Header-only: there is no .cpp to add to the project. What is checked:
//   1. Init refuses a wing that does not fit or names a trigger that does not exist
//   2. the orb: it falls onto the floor of its layer and rests; taken (E), it is held, follows the body and switches with it;
//      put down (E), it is placed one step ahead in the layer of the body; an orb of the other layer cannot be taken; an orb
//      out of reach cannot be taken; an orb behind a wall cannot be taken through it
//   3. snapping: put down near a socket, a plate or an extra spot, it is placed over it; far from every spot, it is not
//   4. falling: with no floor in its own layer (a floor of the other layer under it) it falls through to the next one; below
//      the fall line it goes back to its start; when what it lies on goes away (a bridge), it falls
//   5. a put-down through a wall is refused and the orb stays held
//   6. triggers: a socket works only with an orb of its layer resting on it; a plate also with the body standing on it in its
//      layer; a lever from a touch, in either layer, for good; a latching socket stays active when the orb is taken
//   7. gates: a bridge is there while open, a door while closed; a door with two triggers opens with either; AddSolids gives
//      only the gates of the layer that are there; the drawn tables hold the gates, the pads and the orbs
//   8. ResetRoom puts back the orbs of that room only, also a held one; latched triggers stay
// Returns 1 when a check fails.

#include "scene/Mechanisms.h"

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
using namespace dl::scene;
namespace mech = dl::mech;

constexpr float kDt = 1.0f / 60.0f;
const Vec3 kNorth{0.0f, 0.0f, 1.0f};
const Vec3 kEast{1.0f, 0.0f, 0.0f};

constexpr SceneObject Box(float x0, float y0, float z0, float x1, float y1, float z1, unsigned mask)
{
    SceneObject o{};
    o.shape = ShapeKind::Cube;
    o.position[0] = 0.5f * (x0 + x1);
    o.position[1] = 0.5f * (y0 + y1);
    o.position[2] = 0.5f * (z0 + z1);
    o.scale[0] = x1 - x0;
    o.scale[1] = y1 - y0;
    o.scale[2] = z1 - z0;
    o.layerMask = mask;
    return o;
}

// THE TEST ROOM: a floor of both layers from z -5 to z 5, a wall of both layers at x 3 (z -5 .. 0), a floor of A only from
// z 5 to z 8 (over a lower floor of both layers at y -3), and nothing beyond z 8.
constexpr SceneObject kRoom[] = {
    Box(-5.0f, -1.2f, -5.0f, 5.0f, -1.0f, 5.0f, kLayerBoth),
    Box(3.0f, -1.0f, -5.0f, 3.5f, 2.0f, 0.0f, kLayerBoth),
    Box(-5.0f, -1.2f, 5.0f, 5.0f, -1.0f, 8.0f, kLayerA),
    Box(-5.0f, -3.2f, 5.0f, 5.0f, -3.0f, 8.0f, kLayerBoth),
};
constexpr int kRoomCount = static_cast<int>(sizeof(kRoom) / sizeof(kRoom[0]));

constexpr int kSocketB = 0;
constexpr int kPlateA = 1;
constexpr int kLever = 2;
constexpr int kLatchSocketA = 3;
const mech::Trigger kTriggers[] = {
    {mech::TriggerKind::Socket, {-2.0f, -1.0f, 2.0f}, 1, false, "socket B"},
    {mech::TriggerKind::Plate, {2.0f, -1.0f, 2.0f}, 0, false, "plate A"},
    {mech::TriggerKind::Lever, {-4.0f, 0.2f, -4.0f}, 0, true, "lever"},
    {mech::TriggerKind::Socket, {0.0f, -1.0f, -4.0f}, 0, true, "latching socket A"},
};
constexpr int kBridge = 0;
constexpr int kDoor = 1;
const mech::Gate kGates[] = {
    {Box(-4.0f, 1.0f, -1.0f, -3.0f, 1.2f, 1.0f, kLayerB), kSocketB, -1, true, "bridge B"},
    {Box(4.0f, -1.0f, -4.0f, 5.0f, 1.0f, -3.8f, kLayerBoth), kPlateA, kLever, false, "door"},
};
const mech::OrbStart kOrbs[] = {
    {{0.0f, 0.0f, 0.0f}, 0, 7},  // orb 0: room 7, in A
    {{-4.0f, 0.0f, 3.0f}, 1, 8}, // orb 1: room 8, in B
};
const Vec3 kSpots[] = {{0.0f, -1.0f, 4.0f}};

mech::WingDesc Desc()
{
    mech::WingDesc d;
    d.triggers = kTriggers;
    d.triggerCount = 4;
    d.gates = kGates;
    d.gateCount = 2;
    d.orbs = kOrbs;
    d.orbCount = 2;
    d.spots = kSpots;
    d.spotCount = 1;
    d.fallY = -10.0f;
    return d;
}

bool Near(float a, float b, float eps)
{
    return std::fabs(a - b) <= eps;
}

/// The room, the set and a body, run the way the program runs them.
struct World
{
    mech::MechanismSet set;
    Player body;
    int layer = 0;
    SceneObject solids[2][32] = {};
    int counts[2] = {0, 0};

    World()
    {
        CHECK(set.Init(Desc()));
        Build();
    }

    void Build()
    {
        for (int l = 0; l < 2; ++l)
        {
            int n = 0;
            for (const SceneObject& o : kRoom)
            {
                solids[l][n++] = o;
            }
            counts[l] = set.AddSolids(l, solids[l], n, 32);
        }
    }

    void Run(float seconds)
    {
        const int frames = static_cast<int>(std::lround(seconds / kDt));
        for (int i = 0; i < frames; ++i)
        {
            Build();
            body.SetScene(solids[layer], counts[layer]);
            body.Update({}, kNorth, layer, 0.0f, kDt);
            Build();
            set.Update(body, layer, solids[0], counts[0], solids[1], counts[1], kDt);
        }
    }

    void Stand(float x, float z, float y = -1.0f)
    {
        body.Place({x, y + body.GetSettings().eyeHeight, z});
        Run(0.3f);
    }

    mech::Action Use(const Vec3& look)
    {
        Build();
        int which = -1;
        const mech::Action a = set.Interact(body, layer, look, solids[layer], counts[layer], which);
        Run(0.8f);
        return a;
    }
};

void TestInit()
{
    mech::MechanismSet set;
    mech::WingDesc d = Desc();
    d.triggerCount = mech::kMaxTriggers + 1;
    CHECK(!set.Init(d));
    d = Desc();
    const mech::Gate bad[] = {{kGates[0].box, 9, -1, true, "bad"}};
    d.gates = bad;
    d.gateCount = 1;
    CHECK(!set.Init(d));
    CHECK(set.Init(Desc()) && set.OrbCount() == 2 && set.GateCount() == 2 && set.TriggerCount() == 4);
}

void TestOrb()
{
    World w;
    w.Run(1.0f);
    const mech::MechanismSet::Orb& o = w.set.OrbAt(0);
    CHECK(o.resting && Near(o.centre.y, -1.0f + mech::kOrbRadius, 0.02f)); // it fell onto the floor
    // out of reach: nothing; in reach but the body in B (the orb is in A): nothing
    w.Stand(0.0f, -3.0f);
    CHECK(w.Use(kNorth) == mech::Action::Nothing);
    w.Stand(0.0f, -1.0f);
    w.layer = 1;
    CHECK(w.Use(kNorth) == mech::Action::Nothing);
    w.layer = 0;
    CHECK(w.Use(kNorth) == mech::Action::Took && w.set.HeldOrb() == 0);
    // held: it follows the body and switches with it
    w.Stand(-1.0f, -2.0f);
    w.layer = 1;
    w.Run(0.1f);
    CHECK(w.set.OrbAt(0).held && w.set.OrbAt(0).layer == 1 && Near(w.set.OrbAt(0).centre.x, -1.0f, 0.05f));
    // put down: one step ahead, in the layer of the body, and it stays there
    CHECK(w.Use(kNorth) == mech::Action::Put && w.set.HeldOrb() < 0);
    CHECK(w.set.OrbAt(0).layer == 1 && w.set.OrbAt(0).resting && Near(w.set.OrbAt(0).centre.z, -1.0f, 0.1f) &&
          Near(w.set.OrbAt(0).centre.x, -1.0f, 0.05f));
    w.layer = 0;
    w.Run(0.5f);
    CHECK(w.set.OrbAt(0).layer == 1); // the body switched back; the orb did not
}

void TestSnapAndFall()
{
    World w;
    w.Run(1.0f);
    w.Stand(0.0f, -1.0f);
    CHECK(w.Use(kNorth) == mech::Action::Took);
    // near the plate (2, 2): snapped onto it
    w.Stand(1.6f, 1.2f);
    CHECK(w.Use(kNorth) == mech::Action::Put && Near(w.set.OrbAt(0).centre.x, 2.0f, 1e-3f) && Near(w.set.OrbAt(0).centre.z, 2.0f, 1e-3f));
    CHECK(w.set.Active(kPlateA)); // an orb of A on a plate of A
    // far from every spot: one step ahead, not snapped
    CHECK(w.Use(kNorth) == mech::Action::Took);
    w.Stand(0.0f, -3.0f);
    CHECK(w.Use(kEast) == mech::Action::Put && Near(w.set.OrbAt(0).centre.x, 1.0f, 0.05f) && Near(w.set.OrbAt(0).centre.z, -3.0f, 0.05f));
    // the extra spot (0, 4) at the edge of the floor of both layers; beyond it, the floor of A only: put down in B over it, the
    // orb falls through to the lower floor; in A it lies on the floor of A
    CHECK(w.Use(kEast) == mech::Action::Took);
    w.Stand(0.0f, 4.0f);
    w.Stand(0.0f, 4.5f);
    CHECK(w.Use(kNorth) == mech::Action::Put);
    CHECK(w.set.OrbAt(0).resting && Near(w.set.OrbAt(0).centre.y, -1.0f + mech::kOrbRadius, 0.02f) && w.set.OrbAt(0).centre.z > 5.0f);
    CHECK(w.Use(kNorth) == mech::Action::Took);
    w.layer = 1;
    CHECK(w.Use(kNorth) == mech::Action::Put);
    CHECK(w.set.OrbAt(0).resting && Near(w.set.OrbAt(0).centre.y, -3.0f + mech::kOrbRadius, 0.02f));
    // below the fall line: back to its start (orb 1 starts in B over the floor; dropped off the north end it falls out)
    World f;
    f.layer = 1;
    f.Run(1.0f);
    f.Stand(-4.0f, 2.0f);
    CHECK(f.Use(kNorth) == mech::Action::Took && f.set.HeldOrb() == 1);
    f.body.Place({-4.0f, -3.0f + f.body.GetSettings().eyeHeight, 7.5f}); // on the lower floor, at the north end
    f.Run(0.3f);
    CHECK(f.Use(kNorth) == mech::Action::Put);
    f.Run(2.0f);
    CHECK(Near(f.set.OrbAt(1).centre.x, -4.0f, 0.05f) && Near(f.set.OrbAt(1).centre.z, 3.0f, 0.05f) && f.set.OrbAt(1).resting);
}

void TestRefused()
{
    World w;
    w.Run(1.0f);
    w.Stand(0.0f, -1.0f);
    CHECK(w.Use(kNorth) == mech::Action::Took);
    w.Stand(2.6f, -2.0f); // in front of the wall at x 3
    CHECK(w.Use(kEast) == mech::Action::Refused && w.set.HeldOrb() == 0);
    // an orb behind the wall is not taken through it: put down just east of the wall, then reached for from the west of it
    w.Stand(3.9f, -2.0f);
    CHECK(w.Use(kNorth) == mech::Action::Put && Near(w.set.OrbAt(0).centre.x, 3.9f, 0.05f));
    w.Stand(2.6f, -1.0f); // 1.3 m away, in reach, with the wall between
    CHECK(w.Use(kEast) == mech::Action::Nothing);
    w.Stand(3.9f, -2.0f);
    CHECK(w.Use(kNorth) == mech::Action::Took);
}

void TestTriggers()
{
    World w;
    w.Run(1.0f);
    // the socket of B: an orb of A on it does nothing; an orb of B does
    w.Stand(0.0f, -1.0f);
    CHECK(w.Use(kNorth) == mech::Action::Took);
    w.Stand(-2.0f, 0.9f);
    CHECK(w.Use(kNorth) == mech::Action::Put && Near(w.set.OrbAt(0).centre.x, -2.0f, 1e-3f));
    CHECK(!w.set.Active(kSocketB) && !w.set.GatePresent(kBridge));
    CHECK(w.Use(kNorth) == mech::Action::Took);
    w.layer = 1;
    CHECK(w.Use(kNorth) == mech::Action::Put && w.set.Active(kSocketB) && w.set.GatePresent(kBridge));
    // the bridge is a gate of B: in the solid table of B, not of A
    w.Build();
    CHECK(w.counts[1] == kRoomCount + 2 && w.counts[0] == kRoomCount + 1); // B: bridge and door; A: the door only
    // taken off: not latched, the bridge goes
    CHECK(w.Use(kNorth) == mech::Action::Took && !w.set.Active(kSocketB) && !w.set.GatePresent(kBridge));
    // the plate of A: the body on it in A works it, in B it does not; the door opens with it
    w.layer = 0;
    w.Stand(2.0f, 2.0f);
    CHECK(w.set.Active(kPlateA) && w.set.GateOpen(kDoor) && !w.set.GatePresent(kDoor));
    w.layer = 1;
    w.Run(0.1f);
    CHECK(!w.set.Active(kPlateA) && w.set.GatePresent(kDoor));
    // the lever: a touch, in B, for good; the door opens with it as well
    w.Stand(-4.0f, -3.4f);
    CHECK(w.set.Active(kLever) && w.set.GateOpen(kDoor));
    w.Stand(0.0f, 3.0f);
    CHECK(w.set.Active(kLever));
    // the latching socket of A: active once, stays active when the orb is taken
    w.layer = 0;
    w.Stand(0.0f, -2.9f);
    CHECK(w.set.HeldOrb() == 0 && w.Use({0.0f, 0.0f, -1.0f}) == mech::Action::Put && w.set.Active(kLatchSocketA));
    CHECK(w.Use({0.0f, 0.0f, -1.0f}) == mech::Action::Took && w.set.Active(kLatchSocketA));
    CHECK(w.set.Changed(kLatchSocketA) == false); // it changed once, frames ago
}

void TestDraw()
{
    World w;
    w.Run(1.0f);
    SceneObject table[32] = {};
    const int fixed = w.set.AddFixedDraw(table, 0, 32);
    CHECK(fixed == 1 + 4); // the door (the bridge is not there), four pads
    CHECK(table[0].layerMask == kLayerBoth && table[1].layerMask == kLayerB && table[2].layerMask == kLayerA);
    CHECK(table[3].layerMask == kLayerBoth); // the lever: both layers
    const int orbs = w.set.AddOrbDraw(w.body, kNorth, table, fixed, 32);
    CHECK(orbs == fixed + 4); // two orbs, each with a mark in the other layer
    CHECK(table[fixed].shape == ShapeKind::Sphere && table[fixed].layerMask == kLayerA && table[fixed + 1].layerMask == kLayerB);
    CHECK(w.set.AddFixedDraw(table, 31, 32) == 32 && w.set.AddFixedDraw(nullptr, 0, 32) == 0);
}

void TestResetRoom()
{
    World w;
    w.Run(1.0f);
    w.Stand(0.0f, -1.0f);
    CHECK(w.Use(kNorth) == mech::Action::Took);
    w.Stand(-4.0f, -3.4f); // the lever
    CHECK(w.set.Active(kLever));
    w.set.ResetRoom(8); // the room of the other orb: the held one stays held
    CHECK(w.set.HeldOrb() == 0);
    w.set.ResetRoom(7);
    w.Run(0.8f);
    CHECK(w.set.HeldOrb() < 0 && w.set.OrbAt(0).resting && Near(w.set.OrbAt(0).centre.x, 0.0f, 1e-3f) && Near(w.set.OrbAt(0).centre.z, 0.0f, 1e-3f));
    CHECK(w.set.Active(kLever)); // latched: stays
    w.Stand(0.0f, 3.0f); // away from the lever, which would latch again
    w.set.Reset();
    w.Run(0.1f);
    CHECK(!w.set.Active(kLever) && w.set.HeldOrb() < 0);
}

} // namespace

int main()
{
    TestInit();
    TestOrb();
    TestSnapAndFall();
    TestRefused();
    TestTriggers();
    TestDraw();
    TestResetRoom();
    std::printf("%d checks passed, %d failed\n", g_passed, g_failed);
    return g_failed == 0 ? 0 : 1;
}
