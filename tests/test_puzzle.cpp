// test_puzzle.cpp - tests of the puzzle pieces (no window, no GPU): src/scene/Puzzle.h with the walking body of
// src/scene/Player.h, on a small room of its own.
//   g++ -std=c++20 -O2 -Wall -Wextra -I src tests/test_puzzle.cpp -o test_puzzle
//   MSVC (Developer PowerShell):  .\tests\run_tests.ps1
// Header-only: there is no .cpp to add to the project. What is checked:
//   1. Init refuses a stage that makes no sense; the solid table has the fixed objects, the block and the closed door
//   2. the feel of a push: a short tap does nothing; walking at the face for kPushSeconds moves the block one cell; held,
//      it goes on cell after cell without a stop and the body stays at the face; let go during a move, the block stops at
//      the next cell; up to about 45 degrees off the face it still pushes, at 60 degrees it does not; standing on top of the
//      block is not pushing; the body is never inside the block
//   3. a push is refused when the next cell is not free in BOTH layers: a wall of A stops it while the body is in B, a wall
//      of B while the body is in A, the door (open or not), the edge of the allowed cells; the refusal names the blocker
//      and flashes it for kFlashSeconds
//   4. the plate: pressed by the body standing on it in A (not in B), and by the block resting on it in both layers; the
//      door is open exactly while the plate is pressed, and a closed door stops the body in B
//   5. the guard: a switch into a place where the body does not fit is refused, the old layer stays solid, the ripple is
//      turned back once after 0.15 s, requests during a refusal are ignored, a blocked request during a ripple is ignored
// Returns 1 when a check fails.

#include "scene/Puzzle.h"

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
using dl::puzzle::LayerGuard;
using dl::puzzle::Puzzle;
using namespace dl::scene;

constexpr float kFloor = kFloorY;
constexpr float kDt = 1.0f / 120.0f;
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

// THE TEST ROOM (cells are 1 m, cell (x, z) has its centre at world (x, z)).
constexpr int kWallA = 1;
constexpr int kWallB = 2;
constexpr SceneObject kStatics[] = {
    Make(0.0f, kFloorY - 0.1f, 0.0f, 24.0f, 0.2f, 24.0f, kLayerBoth), // 0 the floor, top at -1
    Make(-3.0f, kFloorY + 1.0f, 0.0f, 1.0f, 2.0f, 1.0f, kLayerA),    // 1 a wall of A in cell (-3, 0)
    Make(0.0f, kFloorY + 1.0f, -3.0f, 1.0f, 2.0f, 1.0f, kLayerB),    // 2 a wall of B in cell (0, -3)
};

dl::puzzle::StageDesc Stage()
{
    dl::puzzle::StageDesc d;
    d.statics = kStatics;
    d.staticCount = static_cast<int>(sizeof(kStatics) / sizeof(kStatics[0]));
    d.blockStart = {0, 0};
    d.plate = {2, 0};
    d.door = Make(6.0f, kFloorY + 1.25f, 0.0f, 0.2f, 2.5f, 2.0f, kLayerBoth); // Init makes it layer B
    d.minCell = {-4, -4};
    d.maxCell = {8, 4};
    return d;
}

bool Near(float a, float b, float eps)
{
    return std::fabs(a - b) <= eps;
}

Player::Input Keys(float forward, float right = 0.0f)
{
    Player::Input input;
    input.moveForward = forward;
    input.moveRight = right;
    return input;
}

bool CellIs(const Puzzle& p, int x, int z)
{
    return p.BlockCell().x == x && p.BlockCell().z == z && !p.BlockMoving();
}

/// The body, the puzzle and the clock, run together the way the program runs them.
struct World
{
    Puzzle puzzle;
    Player player;
    int layer = 0;
    float spin = 0.0f;
    float widestGap = 0.0f; // the largest distance between the body and the face it pushes, while the block moved

    World()
    {
        if (!puzzle.Init(Stage()))
        {
            ++g_failed;
            std::printf("FAIL: the test room does not Init\n");
        }
        player.Respawn();
    }

    void Frame(const Player::Input& input, const Vec3& look)
    {
        int count = 0;
        const SceneObject* solids = puzzle.Solids(layer, count);
        player.SetScene(solids, count);
        spin += kDt;
        player.Update(input, look, layer, spin, kDt);
        puzzle.Update(player, kDt);
        if (puzzle.BlockMoving())
        {
            const Vec3 c = puzzle.BlockCentre();
            const float gap = std::fabs(c.x - player.Feet().x) + std::fabs(c.z - player.Feet().z) - 0.5f - player.GetSettings().radius;
            widestGap = gap > widestGap ? gap : widestGap;
        }
    }

    void Run(float seconds, Player::Input input = {}, const Vec3& look = kNorth)
    {
        const int frames = static_cast<int>(std::lround(seconds / kDt));
        for (int i = 0; i < frames; ++i)
        {
            Frame(input, look);
        }
    }

    void PlaceFeet(const Vec3& feet)
    {
        player.Place({feet.x, feet.y + player.GetSettings().eyeHeight, feet.z});
        Run(0.3f);
    }
};

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
    d.blockStart = {20, 0};
    CHECK(!p.Init(d));
    d = Stage();
    d.staticCount = Puzzle::kMaxSolids; // too many
    CHECK(!p.Init(d));
    CHECK(p.Init(Stage()));
    int count = 0;
    const SceneObject* solids = p.Solids(1, count);
    CHECK(count == 3 + 2); // the fixed objects, the block, the closed door
    CHECK(solids[3].layerMask == kLayerBoth && Near(solids[3].position[1], kFloor + 0.5f, 1e-6f));
    CHECK(solids[4].layerMask == kLayerB);
    CHECK(CellIs(p, 0, 0) && p.Moves() == 0 && p.LastBlocker() == dl::puzzle::kBlockerNone);
}

void TestPushFeel()
{
    {
        World w; // a tap shorter than kPushSeconds does nothing, again and again
        w.PlaceFeet({-0.88f, kFloor, 0.0f});
        for (int i = 0; i < 5; ++i)
        {
            w.Run(0.08f, Keys(1.0f), kEast);
            w.Run(0.1f);
        }
        CHECK(CellIs(w.puzzle, 0, 0) && w.puzzle.Moves() == 0);
    }
    {
        World w; // held: it goes on cell after cell, and the body stays at the face
        w.PlaceFeet({-1.5f, kFloor, 0.0f});
        float t = 0.0f;
        while (w.puzzle.Moves() < 3 && t < 5.0f)
        {
            w.Frame(Keys(1.0f), kEast);
            t += kDt;
        }
        CHECK(w.puzzle.Moves() == 3);
        CHECK(t < 0.6f + Puzzle::kPushSeconds + 3.0f / Puzzle::kSlideSpeed); // the walk up, the first wait, three slides
        CHECK(w.widestGap < 0.2f);
        // let go now (a move has started): the block stops at the next cell
        w.Run(1.0f);
        CHECK(!w.puzzle.BlockMoving() && w.puzzle.Moves() == 4 && CellIs(w.puzzle, 4, 0));
        // the body is never inside the block
        CHECK(w.puzzle.BodyFits(w.player, 0, w.spin) && w.puzzle.BodyFits(w.player, 1, w.spin));
    }
    {
        // the angle: walking 40 degrees off the face still pushes, 60 degrees does not. A body that cannot move (walking speed
        // 0) still has a walk direction: it stays at the face, so only the angle decides
        Player::Settings still;
        still.walkSpeed = 0.0f;
        const float a40 = 0.6981f;
        const float a60 = 1.0472f;
        World near;
        near.player.SetSettings(still);
        near.PlaceFeet({-0.88f, kFloor, 0.0f});
        near.Run(0.5f, Keys(std::cos(a40), -std::sin(a40)), kEast);
        CHECK(near.puzzle.Moves() == 1 || near.puzzle.BlockMoving());
        World far;
        far.player.SetSettings(still);
        far.PlaceFeet({-0.88f, kFloor, 0.0f});
        far.Run(1.0f, Keys(std::cos(a60), -std::sin(a60)), kEast);
        CHECK(far.puzzle.Moves() == 0 && !far.puzzle.BlockMoving());
        World top; // standing on the block and walking is not pushing
        top.player.SetSettings(still);
        top.PlaceFeet({0.3f, kFloor + 1.0f, 0.0f});
        CHECK(top.player.Grounded());
        top.Run(1.0f, Keys(1.0f), kWest);
        CHECK(top.puzzle.Moves() == 0 && !top.puzzle.BlockMoving());
    }
}

/// Walks along `look` until `moves` more moves have finished, letting go as soon as the last one has started.
void PushMoves(World& w, int moves, const Vec3& look, float limitSeconds = 8.0f)
{
    const int goal = w.puzzle.Moves() + moves;
    float t = 0.0f;
    while (w.puzzle.Moves() < goal && t < limitSeconds)
    {
        const bool lastStarted = w.puzzle.BlockMoving() && w.puzzle.Moves() == goal - 1;
        w.Frame(Keys(lastStarted ? 0.0f : 1.0f), look);
        t += kDt;
    }
    w.Run(0.1f);
}

void TestBlocked()
{
    {
        World w; // a wall of A stops the block while the body is in B (the wall is not even solid for the body there)
        w.layer = 1;
        w.PlaceFeet({0.88f, kFloor, 0.0f});
        PushMoves(w, 2, kWest);
        CHECK(CellIs(w.puzzle, -2, 0));
        PushMoves(w, 1, kWest, 1.0f); // the next cell holds the wall of A
        CHECK(CellIs(w.puzzle, -2, 0) && w.puzzle.LastBlocker() == kWallA);
        CHECK(w.puzzle.BlockedFlash() > 0.0f);
        w.Run(Puzzle::kFlashSeconds + 0.1f);
        CHECK(w.puzzle.BlockedFlash() == 0.0f);
    }
    {
        World w; // a wall of B stops the block while the body is in A
        w.PlaceFeet({0.0f, kFloor, 0.88f});
        PushMoves(w, 2, kSouth);
        CHECK(CellIs(w.puzzle, 0, -2));
        PushMoves(w, 1, kSouth, 1.0f);
        CHECK(CellIs(w.puzzle, 0, -2) && w.puzzle.LastBlocker() == kWallB);
    }
    {
        World w; // the edge of the allowed cells
        w.PlaceFeet({0.0f, kFloor, -0.88f});
        PushMoves(w, 4, kNorth);
        CHECK(CellIs(w.puzzle, 0, 4));
        PushMoves(w, 1, kNorth, 1.0f);
        CHECK(CellIs(w.puzzle, 0, 4) && w.puzzle.LastBlocker() == dl::puzzle::kBlockerBounds);
    }
    {
        World w; // the door, even open: the block on the plate holds it open and still cannot go into it
        w.PlaceFeet({-0.88f, kFloor, 0.0f});
        PushMoves(w, 5, kEast);
        CHECK(CellIs(w.puzzle, 5, 0));
        PushMoves(w, 1, kEast, 1.0f);
        CHECK(CellIs(w.puzzle, 5, 0) && w.puzzle.LastBlocker() == dl::puzzle::kBlockerDoor);
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
        CHECK(countA == 4 && countB == 5); // the door is in the table only while it is closed
        w.Run(0.5f, Keys(1.0f), kNorth); // off the plate
        CHECK(!w.puzzle.PlatePressed(0));
    }
    {
        World w; // the block resting on the plate presses it in both layers
        w.PlaceFeet({-0.88f, kFloor, 0.0f});
        PushMoves(w, 2, kEast);
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
        open.PlaceFeet({-0.88f, kFloor, 0.0f});
        PushMoves(open, 2, kEast);
        open.layer = 1;
        open.PlaceFeet({4.0f, kFloor, 0.0f});
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
        CHECK(g.OnSwitchRequest(1, false, true) == LayerGuard::Action::Switch);
    }
    {
        LayerGuard g; // does not fit while a ripple runs: dropped, no refusal on top of a ripple
        CHECK(g.OnSwitchRequest(0, true, false) == LayerGuard::Action::Ignore);
        CHECK(!g.Refusing() && g.Refusals() == 0);
        CHECK(!g.Update(0.0f) && !g.Update(-1.0f));
    }
    {
        World w; // with the room: standing in the cell of the wall of B (in A) the body does not fit in B
        w.PlaceFeet({0.0f, kFloor, -3.0f});
        CHECK(w.puzzle.BodyFits(w.player, 0, w.spin) && !w.puzzle.BodyFits(w.player, 1, w.spin));
    }
}

} // namespace

int main()
{
    TestInit();
    TestPushFeel();
    TestBlocked();
    TestPlateAndDoor();
    TestGuard();
    std::printf("%d checks passed, %d failed\n", g_passed, g_failed);
    return g_failed == 0 ? 0 : 1;
}
