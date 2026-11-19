// test_puzzle.cpp - tests of the puzzle pieces (no window, no GPU): src/scene/Puzzle.h with the walking body of
// src/scene/Player.h, on a small stage of its own.
//   g++ -std=c++20 -Wall -Wextra -I src tests/test_puzzle.cpp -o test_puzzle
//   MSVC (Developer PowerShell):  .\tests\run_tests.ps1
// Header-only: there is no .cpp to add to the project. What is checked:
//   1. Init refuses a stage that makes no sense; the solid table has the fixed objects, the block and the closed door
//   2. pushing: walking against a face for 0.2 s moves the block exactly one cell, along the axis of that face, in both
//      layers; a short tap, a walk at 45 degrees, or walking on top of the block does not push; the body never ends inside it
//   3. a push is refused when the next cell is not free in BOTH layers: a wall of A stops it while the body is in B, a wall of
//      B stops it while the body is in A, the door stops it (open or not), the edge of the allowed cells stops it; the
//      refusal names what was in the way and flashes it
//   4. the plate: pressed by the body standing on it in A (not in B), and by the block resting on it in both layers; the door
//      is open exactly while the plate is pressed, and a closed door stops the body in B
//   5. the guard: a switch into a place where the body does not fit is refused, the old layer stays solid, the ripple is
//      turned back once after 0.15 s, requests during a refusal are ignored, a blocked request during a ripple is ignored
//   6. THE STAGE IS SOLVED BY THE BLOCK: the high platform of B cannot be reached from the floor with any jump timing, and
//      can be reached from the block once the block rests on the plate (which also opens the door)
// Returns 1 when a check fails.

#include "scene/Puzzle.h"

#include <cmath>
#include <cstdio>
#include <initializer_list>

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
using dl::puzzle::LayerGuard;
using dl::puzzle::Puzzle;
using namespace dl::scene;

constexpr float kFloor = kFloorY;
constexpr float kDt = 1.0f / 60.0f;
const Vec3 kEast{1.0f, 0.0f, 0.0f};
const Vec3 kWest{-1.0f, 0.0f, 0.0f};
const Vec3 kNorth{0.0f, 0.0f, 1.0f};
const Vec3 kSouth{0.0f, 0.0f, -1.0f};

constexpr SceneObject Make(float x, float y, float z, float sx, float sy, float sz, unsigned mask)
{
    SceneObject o{};
    o.shape = ShapeKind::Cube;
    o.position[0] = x;
    o.position[1] = y;
    o.position[2] = z;
    o.scale[0] = sx;
    o.scale[1] = sy;
    o.scale[2] = sz;
    o.layerMask = mask;
    return o;
}

// THE TEST STAGE (cells are 1 m, cell (x, z) has its centre at world (x, z)).
constexpr int kWallA = 1;
constexpr int kWallB = 2;
constexpr SceneObject kStatics[] = {
    Make(0.0f, kFloorY - 0.1f, 0.0f, 24.0f, 0.2f, 24.0f, kLayerBoth), // 0 the floor, top at -1
    Make(-3.0f, kFloorY + 1.0f, 0.0f, 1.0f, 2.0f, 1.0f, kLayerA),    // 1 a wall of A in cell (-3, 0)
    Make(0.0f, kFloorY + 1.0f, -3.0f, 1.0f, 2.0f, 1.0f, kLayerB),    // 2 a wall of B in cell (0, -3)
    Make(2.0f, kFloorY + 2.3f, 2.2f, 1.4f, 0.2f, 1.4f, kLayerB),     // 3 a high platform of B: top 2.4 m above the floor
};
constexpr float kPlatformTop = kFloorY + 2.4f;

dl::puzzle::StageDesc Stage()
{
    dl::puzzle::StageDesc d;
    d.statics = kStatics;
    d.staticCount = static_cast<int>(sizeof(kStatics) / sizeof(kStatics[0]));
    d.blockStart = {0, 0};
    d.plate = {2, 0};
    d.door = Make(6.0f, kFloorY + 1.25f, 0.0f, 0.2f, 2.5f, 2.0f, kLayerBoth); // Init makes it layer B
    d.minCell = {-4, -4};
    d.maxCell = {4, 4};
    return d;
}

bool Near(float a, float b, float eps)
{
    return std::fabs(a - b) <= eps;
}

Player::Input Keys(float forward, float right = 0.0f, bool jump = false)
{
    Player::Input input;
    input.moveForward = forward;
    input.moveRight = right;
    input.jumpPressed = jump;
    return input;
}

/// The body, the puzzle and the clock, run together the way the program will run them.
struct World
{
    Puzzle puzzle;
    Player player;
    int layer = 0;
    float spin = 0.0f;

    World()
    {
        if (!puzzle.Init(Stage()))
        {
            ++g_failed; // (not a CHECK: a World is made many times, and only a failure is worth counting)
            std::printf("FAIL: the test stage does not Init\n");
        }
        player.Respawn();
    }

    void Frame(Player::Input input, const Vec3& look, float dt = kDt)
    {
        int count = 0;
        const SceneObject* solids = puzzle.Solids(layer, count);
        player.SetScene(solids, count);
        spin += dt;
        player.Update(input, look, layer, spin, dt);
        puzzle.Update(player, dt);
    }

    void Run(float seconds, Player::Input input = {}, const Vec3& look = kNorth)
    {
        const int frames = static_cast<int>(std::lround(seconds / kDt));
        for (int i = 0; i < frames; ++i)
        {
            Frame(input, look);
            input.jumpPressed = false;
        }
    }

    void PlaceFeet(const Vec3& feet)
    {
        player.Place({feet.x, feet.y + player.GetSettings().eyeHeight, feet.z});
        Run(0.2f); // settle on the ground
    }

    /// Walks against the block (or anything) in `look` direction until the block has finished `moves` more moves or the
    /// time is up. Returns the seconds it took.
    float PushMoves(int moves, const Vec3& look, float limitSeconds = 6.0f)
    {
        const int goal = puzzle.Moves() + moves;
        float t = 0.0f;
        while (puzzle.Moves() < goal && t < limitSeconds)
        {
            Frame(Keys(1.0f), look);
            t += kDt;
        }
        Run(0.1f); // stop walking
        return t;
    }
};

bool CellIs(const Puzzle& p, int x, int z)
{
    return p.BlockCell().x == x && p.BlockCell().z == z && !p.BlockMoving();
}

void TestInit()
{
    Puzzle p;
    dl::puzzle::StageDesc d = Stage();
    d.statics = nullptr;
    CHECK(!p.Init(d));
    d = Stage();
    d.minCell = {3, 0};
    d.maxCell = {1, 0};
    CHECK(!p.Init(d));
    d = Stage();
    d.blockStart = {9, 0};
    CHECK(!p.Init(d));
    d = Stage();
    d.staticCount = Puzzle::kMaxSolids; // too many
    CHECK(!p.Init(d));
    CHECK(p.Init(Stage()));
    int count = 0;
    const SceneObject* solids = p.Solids(0, count);
    CHECK(count == 4 + 1 + 1); // the fixed objects, the block, the closed door
    CHECK(solids[4].layerMask == kLayerBoth && Near(solids[4].position[1], kFloor + 0.5f, 1e-6f)); // the block: both layers, on the floor
    CHECK(solids[5].layerMask == kLayerB);                                                          // the door: B
    CHECK(CellIs(p, 0, 0) && p.Moves() == 0 && p.LastBlocker() == dl::puzzle::kBlockerNone);
}

void TestPush()
{
    {
        World w; // pushing east from the west face: one cell per 0.2 s of pushing, exactly on the grid
        w.PlaceFeet({-1.6f, kFloor, 0.0f});
        w.Run(0.15f, Keys(1.0f), kEast); // touching only for a moment: nothing yet
        CHECK(CellIs(w.puzzle, 0, 0));
        const float t = w.PushMoves(1, kEast);
        CHECK(CellIs(w.puzzle, 1, 0) && t < 1.5f);
        CHECK(Near(w.puzzle.BlockCentre().x, 1.0f, 1e-5f) && Near(w.puzzle.BlockCentre().z, 0.0f, 1e-5f));
        CHECK(w.puzzle.BodyFits(w.player, 0, w.spin) && w.puzzle.BodyFits(w.player, 1, w.spin)); // never inside the block
        w.PushMoves(1, kEast);
        CHECK(CellIs(w.puzzle, 2, 0) && w.puzzle.Moves() == 2);
    }
    {
        World w; // the same in layer B: the block is in both layers
        w.layer = 1;
        w.PlaceFeet({0.0f, kFloor, 1.6f});
        w.PushMoves(1, kSouth);
        CHECK(CellIs(w.puzzle, 0, -1));
    }
    {
        World w; // a tap shorter than the push time does nothing, again and again
        w.PlaceFeet({-0.9f, kFloor, 0.0f}); // already touching
        for (int i = 0; i < 5; ++i)
        {
            w.Run(0.1f, Keys(1.0f), kEast);
            w.Run(0.1f);
        }
        CHECK(CellIs(w.puzzle, 0, 0) && w.puzzle.Moves() == 0);
    }
    {
        World w; // walking at 45 degrees against the face does not push
        w.PlaceFeet({-0.9f, kFloor, 0.0f});
        w.Run(1.0f, Keys(1.0f, -1.0f), kEast); // forward + left: north-east
        CHECK(w.puzzle.Moves() == 0);
    }
    {
        // a body that cannot move (walking speed 0) still has a walk direction: it pins the body against the face, so the
        // rules are seen without the body sliding away
        World straight;
        Player::Settings still;
        still.walkSpeed = 0.0f;
        straight.player.SetSettings(still);
        straight.PlaceFeet({-0.88f, kFloor, 0.0f}); // touching the west face
        straight.Run(0.5f, Keys(1.0f), kEast);
        CHECK(straight.puzzle.Moves() == 1 || straight.puzzle.BlockMoving()); // straight at the face: it pushes
        World diagonal;
        diagonal.player.SetSettings(still);
        diagonal.PlaceFeet({-0.88f, kFloor, 0.0f});
        diagonal.Run(1.0f, Keys(1.0f, -1.0f), kEast); // at 45 degrees: it does not
        CHECK(diagonal.puzzle.Moves() == 0 && !diagonal.puzzle.BlockMoving());
        World top; // standing on the block near its east edge and walking west (toward the middle) is not a push either
        top.player.SetSettings(still);
        top.PlaceFeet({0.3f, kFloor + 1.0f, 0.0f});
        CHECK(top.player.Grounded());
        top.Run(1.0f, Keys(1.0f), kWest);
        CHECK(top.puzzle.Moves() == 0 && !top.puzzle.BlockMoving());
    }
    {
        World w; // walking on top of the block does not push it
        w.PlaceFeet({0.0f, kFloor + 1.0f, 0.0f});
        CHECK(w.player.Grounded() && Near(w.player.Feet().y, kFloor + 1.0f, 0.01f));
        w.Run(0.25f, Keys(1.0f), kEast);
        CHECK(w.puzzle.Moves() == 0 && !w.puzzle.BlockMoving());
    }
}

void TestBlocked()
{
    {
        World w; // a wall of A stops the block while the body is in B (the wall is not even solid for the body there)
        w.layer = 1;
        w.PlaceFeet({1.6f, kFloor, 0.0f});
        w.PushMoves(2, kWest);
        CHECK(CellIs(w.puzzle, -2, 0));
        w.PushMoves(1, kWest, 1.5f); // the next cell holds the wall of A
        CHECK(CellIs(w.puzzle, -2, 0));
        CHECK(w.puzzle.LastBlocker() == kWallA);
        w.Run(0.1f);
        CHECK(w.puzzle.BlockedFlash() > 0.0f); // it flashes for a moment ...
        w.Run(1.0f);
        CHECK(w.puzzle.BlockedFlash() == 0.0f); // ... and stops
    }
    {
        World w; // a wall of B stops the block while the body is in A
        w.PlaceFeet({0.0f, kFloor, 1.6f});
        w.PushMoves(2, kSouth);
        CHECK(CellIs(w.puzzle, 0, -2));
        w.PushMoves(1, kSouth, 1.5f);
        CHECK(CellIs(w.puzzle, 0, -2) && w.puzzle.LastBlocker() == kWallB);
    }
    {
        World w; // the edge of the allowed cells
        w.PlaceFeet({0.0f, kFloor, -1.6f});
        w.PushMoves(4, kNorth, 12.0f);
        CHECK(CellIs(w.puzzle, 0, 4));
        w.PushMoves(1, kNorth, 1.5f);
        CHECK(CellIs(w.puzzle, 0, 4) && w.puzzle.LastBlocker() == dl::puzzle::kBlockerBounds);
    }
    {
        // the door stops the block (the door is never a place for the block, open or not); the stage is widened so that
        // the edge of the allowed cells is not reached first
        dl::puzzle::StageDesc d = Stage();
        d.maxCell = {8, 4};
        World v;
        CHECK(v.puzzle.Init(d));
        v.PlaceFeet({-1.6f, kFloor, 0.0f});
        v.PushMoves(5, kEast, 15.0f);
        CHECK(CellIs(v.puzzle, 5, 0));
        v.PushMoves(1, kEast, 1.5f);
        CHECK(CellIs(v.puzzle, 5, 0) && v.puzzle.LastBlocker() == dl::puzzle::kBlockerDoor);
    }
}

void TestPlateAndDoor()
{
    {
        World w; // the body on the plate in A presses it; for a body in B it is not pressed
        w.PlaceFeet({2.0f, kFloor, 0.0f});
        CHECK(w.puzzle.PlatePressed(0) && w.puzzle.DoorOpen(0));
        CHECK(!w.puzzle.PlatePressed(1) && !w.puzzle.DoorOpen(1));
        int countA = 0;
        int countB = 0;
        w.puzzle.Solids(0, countA);
        w.puzzle.Solids(1, countB);
        CHECK(countA == 5 && countB == 6); // the door is in the table only while it is closed
        w.Run(0.5f, Keys(1.0f), kNorth); // off the plate
        CHECK(!w.puzzle.PlatePressed(0));
    }
    {
        World w; // the block resting on the plate presses it in both layers
        w.PlaceFeet({-1.6f, kFloor, 0.0f});
        w.PushMoves(2, kEast);
        CHECK(CellIs(w.puzzle, 2, 0));
        CHECK(w.puzzle.PlatePressed(0) && w.puzzle.PlatePressed(1) && w.puzzle.DoorOpen(1));
    }
    {
        World closed; // in B the closed door stops the body ...
        closed.layer = 1;
        closed.PlaceFeet({4.0f, kFloor, 0.0f});
        closed.Run(1.5f, Keys(1.0f), kEast);
        CHECK(closed.player.Feet().x < 6.0f - 0.1f - 0.3f);
        World open; // ... and with the block on the plate the body walks through
        open.PlaceFeet({-1.6f, kFloor, 0.0f});
        open.PushMoves(2, kEast);
        open.layer = 1;
        open.player.Place({4.0f, kFloor + open.player.GetSettings().eyeHeight, 0.0f});
        open.Run(0.2f);
        open.Run(1.5f, Keys(1.0f), kEast);
        CHECK(open.player.Feet().x > 7.0f);
    }
}

void TestGuard()
{
    {
        LayerGuard g; // fits: switch, whatever the ripple does
        CHECK(g.OnSwitchRequest(0, false, true) == LayerGuard::Action::Switch);
        CHECK(g.OnSwitchRequest(0, true, true) == LayerGuard::Action::Switch);
        CHECK(!g.Refusing() && g.Refusals() == 0);
    }
    {
        LayerGuard g; // does not fit: refuse, hold the old layer, turn the ripple back once after 0.15 s
        CHECK(g.OnSwitchRequest(1, false, false) == LayerGuard::Action::Refuse);
        CHECK(g.Refusing() && g.HeldLayer() == 1 && g.Refusals() == 1);
        CHECK(g.OnSwitchRequest(1, true, true) == LayerGuard::Action::Ignore); // nothing else while it plays
        int turns = 0;
        float t = 0.0f;
        float turnedAt = -1.0f;
        for (int i = 0; i < 60; ++i)
        {
            t += 0.01f;
            if (g.Update(0.01f))
            {
                ++turns;
                turnedAt = t;
            }
        }
        CHECK(turns == 1 && Near(turnedAt, LayerGuard::kRefuseSeconds, 0.011f));
        CHECK(!g.Refusing());
        CHECK(g.OnSwitchRequest(1, false, true) == LayerGuard::Action::Switch); // and afterwards it works again
    }
    {
        LayerGuard g; // does not fit while a ripple runs: dropped, no refusal on top of a ripple
        CHECK(g.OnSwitchRequest(0, true, false) == LayerGuard::Action::Ignore);
        CHECK(!g.Refusing() && g.Refusals() == 0);
        CHECK(!g.Update(0.0f) && !g.Update(-1.0f)); // no time, nothing happens
    }
    {
        World w; // with the stage: standing where the door is (in A) the body does not fit in B
        w.PlaceFeet({6.0f, kFloor, 0.0f});
        CHECK(w.puzzle.BodyFits(w.player, 0, w.spin));
        CHECK(!w.puzzle.BodyFits(w.player, 1, w.spin));
        World p; // standing in the cell of the wall of B (in A): does not fit in B, fits in A
        p.PlaceFeet({0.0f, kFloor, -3.0f});
        CHECK(p.puzzle.BodyFits(p.player, 0, p.spin) && !p.puzzle.BodyFits(p.player, 1, p.spin));
        World o; // with the block on the plate the door is open in B: the doorway fits
        o.PlaceFeet({-1.6f, kFloor, 0.0f});
        o.PushMoves(2, kEast);
        o.player.Place({6.0f, kFloor + o.player.GetSettings().eyeHeight, 0.0f});
        o.Run(0.2f);
        CHECK(o.puzzle.BodyFits(o.player, 1, o.spin));
    }
}

/// Tries jumps toward the high platform of B from `feet`, walking north (+Z) with a few timings. True when one of them
/// ends with the body standing on the platform.
bool CanReachPlatform(const Vec3& feet, bool blockOnPlate)
{
    const float walkTimes[] = {0.0f, 0.05f, 0.1f, 0.15f, 0.2f, 0.25f, 0.3f, 0.35f, 0.4f, 0.5f, 0.6f, 0.8f};
    const float holdTimes[] = {0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.8f, 1.0f};
    for (float before : walkTimes)
    {
        for (float hold : holdTimes)
        {
            World w;
            if (blockOnPlate)
            {
                w.PlaceFeet({-1.6f, kFloor, 0.0f});
                w.PushMoves(2, kEast);
            }
            w.layer = 1;
            w.player.Place({feet.x, feet.y + w.player.GetSettings().eyeHeight, feet.z});
            w.Run(0.3f);
            w.Run(before, Keys(1.0f), kNorth);           // a run-up
            w.Run(kDt, Keys(1.0f, 0.0f, true), kNorth);  // the jump
            w.Run(hold, Keys(1.0f), kNorth);             // keep going
            w.Run(0.8f);                                 // and stop
            if (w.player.Grounded() && Near(w.player.Feet().y, kPlatformTop, 0.02f))
            {
                return true;
            }
        }
    }
    return false;
}

void TestStageIsSolvedByTheBlock()
{
    // without the block: from the floor in front of the platform, no timing reaches its top (2.4 m above a 2.0 m jump)
    CHECK(!CanReachPlatform({2.0f, kFloor, 0.0f}, false));
    CHECK(!CanReachPlatform({2.0f, kFloor, 0.6f}, false));
    // with the block resting on the plate: from the top of the block, a jump reaches the platform
    CHECK(CanReachPlatform({2.0f, kFloor + 1.0f, 0.0f}, true));
    // and the same push has opened the door of B: one push answers both layers
    World w;
    w.PlaceFeet({-1.6f, kFloor, 0.0f});
    w.PushMoves(2, kEast);
    CHECK(w.puzzle.DoorOpen(1));
}

} // namespace

int main()
{
    TestInit();
    TestPush();
    TestBlocked();
    TestPlateAndDoor();
    TestGuard();
    TestStageIsSolvedByTheBlock();
    std::printf("%d checks passed, %d failed\n", g_passed, g_failed);
    return g_failed == 0 ? 0 : 1;
}
