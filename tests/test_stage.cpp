// test_stage.cpp - plays the first stage (src/scene/Stage.h) with the real walking body (Player.h) and the real puzzle
// (Puzzle.h), no window, no GPU. Each part is shown to be solved by switching layers, and NOT solved without it.
//   g++ -std=c++20 -Wall -Wextra -I src tests/test_stage.cpp -o test_stage
//   MSVC (Developer PowerShell):  .\tests\run_tests.ps1
// Header-only: there is no .cpp to add to the project. What is checked:
//   0. the tables: the solid table and the drawn table hold what Stage.h says; a flash shows the blocker in both layers
//   1. part 1: the red wall stops the body in A (also at the edge of the corridor); in B the body walks through
//   2. part 2: from the south floor, no jump timing crosses the gap in A alone or in B alone; the jump onto the stone (B) and
//      on from it with a switch to A in the air lands on the strip; the switch in the air is allowed by the guard
//   3. part 3: without the block, no jump timing reaches the ledge from the floor; the closed door stops the body; in A the
//      panel stops it even with the block on the plate; the block pushed onto the plate (2 east, 4 north) opens the door,
//      and from it the body climbs to the ledge, walks through the doorway and reaches the goal
//   4. the checkpoints and the fall line
// Moves between the parts place the body on the floor (the floor between them is flat and needs no proof); every jump, push
// and switch is played frame by frame. Returns 1 when a check fails.

#include "scene/Stage.h"

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
using namespace dl::scene;
namespace st = dl::stage;

constexpr float kDt = 1.0f / 120.0f; // a fine clock: the moment of a jump or a switch matters here
const Vec3 kNorth{0.0f, 0.0f, 1.0f};
const Vec3 kEast{1.0f, 0.0f, 0.0f};

bool Near(float a, float b, float eps)
{
    return std::fabs(a - b) <= eps;
}

Player::Input Keys(float forward, bool jump = false)
{
    Player::Input input;
    input.moveForward = forward;
    input.jumpPressed = jump;
    return input;
}

/// The stage as the program runs it: the solid table of the layer of the body, then the body, then the puzzle.
struct World
{
    dl::puzzle::Puzzle puzzle;
    Player player;
    int layer = 0;
    float spin = 0.0f;

    World()
    {
        if (!puzzle.Init(st::MakeStage()))
        {
            ++g_failed;
            std::printf("FAIL: the stage does not Init\n");
        }
        player.SetSpawn(st::kCheckpoints[0].feet);
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
        Run(0.3f);
    }

    bool Fallen() const
    {
        return player.Feet().y < st::kFallY;
    }

    /// Switches to `to` the way the program does: only when the guard lets it.
    bool Switch(int to)
    {
        dl::puzzle::LayerGuard guard;
        const bool fits = puzzle.BodyFits(player, to, spin);
        if (guard.OnSwitchRequest(layer, false, fits) != dl::puzzle::LayerGuard::Action::Switch)
        {
            return false;
        }
        layer = to;
        return true;
    }

    /// Runs north from where the body stands, jumps when the feet pass `jumpAtZ`, keeps going north until the body lands
    /// (or falls). `switchAfter` > 0: switch to `switchTo` that long after the jump. Returns true when it landed.
    bool RunJump(float jumpAtZ, float switchAfter = -1.0f, int switchTo = 0, bool stopOnLanding = true)
    {
        int guard = 0;
        while (player.Feet().z < jumpAtZ && player.Grounded() && guard++ < 2000)
        {
            Frame(Keys(1.0f), kNorth);
        }
        if (!player.Grounded())
        {
            return false; // it walked off before the jump
        }
        Frame(Keys(1.0f, true), kNorth);
        float air = 0.0f;
        bool switched = false;
        for (int i = 0; i < 600; ++i)
        {
            if (switchAfter > 0.0f && !switched && air >= switchAfter)
            {
                switched = Switch(switchTo);
            }
            Frame(Keys(1.0f), kNorth);
            air += kDt;
            if (player.Grounded() && air > 0.1f)
            {
                if (stopOnLanding)
                {
                    Run(0.2f); // let go: the body stops
                }
                return !Fallen();
            }
            if (Fallen())
            {
                return false;
            }
        }
        return false;
    }

    float PushMoves(int moves, const Vec3& look, float limitSeconds = 8.0f)
    {
        const int goal = puzzle.Moves() + moves;
        float t = 0.0f;
        while (puzzle.Moves() < goal && t < limitSeconds)
        {
            Frame(Keys(1.0f), look);
            t += kDt;
        }
        Run(0.1f);
        return t;
    }
};

const float kJumpPoints[] = {5.2f, 5.4f, 5.5f, 5.6f, 5.7f, 5.8f, 5.9f, 6.0f, 6.1f, 6.2f};

void TestTables()
{
    World w;
    int count = 0;
    w.puzzle.Solids(1, count);
    CHECK(count == st::kStageStaticCount + 2); // the fixed objects, the block, the closed door
    SceneObject drawn[st::kMaxDrawObjects];
    int n = st::BuildDrawTable(w.puzzle, drawn);
    CHECK(n == st::kStageStaticCount + 4); // + block, door, plate, goal light
    CHECK(drawn[st::kStageStaticCount].layerMask == kLayerBoth);     // the block
    CHECK(drawn[st::kStageStaticCount + 1].layerMask == kLayerB);    // the door
    CHECK(drawn[st::kStageStaticCount + 2].layerMask == kLayerA);    // the plate
    CHECK(drawn[st::kStageStaticCount + 3].shape == ShapeKind::Sphere); // the goal light
    for (int i = 0; i < st::kStageStaticCount; ++i)
    {
        CHECK(drawn[i].layerMask == st::kStageStatics[i].layerMask); // no flash: the fixed objects as they are
    }
    // a refused push flashes the blocker: the block pushed into the red panel's wall is stopped by the wall (both layers),
    // so use the stage edge instead: push the block west to x = -2, then once more (the edge of the allowed cells)
    World f;
    f.PlaceFeet({0.0f - 1.0f + 0.85f, kFloorY, 15.0f}); // east of the block (block at x = -1)
    f.PushMoves(1, {-1.0f, 0.0f, 0.0f});
    CHECK(f.puzzle.BlockCell().x == -2);
    f.PushMoves(1, {-1.0f, 0.0f, 0.0f}, 1.0f);
    CHECK(f.puzzle.BlockCell().x == -2 && f.puzzle.LastBlocker() == dl::puzzle::kBlockerBounds && f.puzzle.BlockedFlash() > 0.0f);
}

void TestPart1()
{
    {
        World w; // A: the red wall stops the body
        w.Run(3.0f, Keys(1.0f), kNorth);
        CHECK(w.player.Feet().z < st::kStageStatics[1].position[2] - 0.25f - 0.3f);
        World edge; // also at the edge of the corridor
        edge.PlaceFeet({2.7f, kFloorY, -1.0f});
        edge.Run(3.0f, Keys(1.0f), kNorth);
        CHECK(edge.player.Feet().z < 3.0f - 0.25f - 0.3f);
    }
    {
        World w; // B: the wall is not there
        CHECK(w.Switch(1));
        w.Run(2.0f, Keys(1.0f), kNorth);
        CHECK(w.player.Feet().z > 4.0f && w.player.Grounded());
    }
}

void TestPart2()
{
    // A alone and B alone: from the south floor, two jumps in a row never bring the body to a place beyond the gap where it
    // can stand (the stone of B, in the middle of the gap, is the only place it may land on)
    const auto onStone = [](const Player& p, int layer) {
        return layer == 1 && p.Feet().z >= st::kStoneNear - 0.4f && p.Feet().z <= st::kStoneFar + 0.4f;
    };
    for (int layer = 0; layer < 2; ++layer)
    {
        for (float at : kJumpPoints)
        {
            World w;
            w.layer = layer;
            w.PlaceFeet({0.0f, kFloorY, 4.5f});
            bool beyond = false;
            if (w.RunJump(at))
            {
                beyond = w.player.Feet().z > st::kSouthEnd + 0.4f && !onStone(w.player, layer);
                if (!beyond && w.player.Feet().z > st::kSouthEnd + 0.4f && w.RunJump(w.player.Feet().z + 0.05f)) // a second jump
                {
                    beyond = !onStone(w.player, layer);
                }
            }
            CHECK(!beyond);
        }
    }
    // B alone from the stone: no jump reaches the north floor
    for (float at : {8.0f, 8.2f, 8.4f, 8.5f, 8.6f, 8.7f})
    {
        World w;
        w.layer = 1;
        w.PlaceFeet({0.0f, kFloorY, 7.9f});
        CHECK(w.player.Grounded()); // the stone holds the body in B
        const bool landed = w.RunJump(at);
        CHECK(!landed || w.player.Feet().z < st::kStoneFar + 0.1f);
    }
    // the way: B, the jump onto the stone, then from the stone with a switch to A in the air onto the strip
    int ways = 0;
    for (float at : kJumpPoints)
    {
        World w;
        CHECK(w.Switch(1));
        w.PlaceFeet({0.0f, kFloorY, 4.5f});
        if (!w.RunJump(at) || w.player.Feet().z < st::kStoneNear || w.player.Feet().z > st::kStoneFar)
        {
            continue;
        }
        for (float after : {0.15f, 0.25f, 0.35f})
        {
            World t = w;
            if (t.RunJump(t.player.Feet().z + 0.05f, after, 0) && t.layer == 0 && t.player.Feet().z > st::kStripNear &&
                Near(t.player.Feet().y, kFloorY, 0.02f))
            {
                ++ways;
            }
        }
    }
    std::printf("  part 2: %d of the tried timings cross the gap\n", ways);
    CHECK(ways >= 3); // not a pixel-perfect trick: several timings work
}

void TestPart3()
{
    // without the block, from the floor in front of the ledge, no jump reaches the ledge (2.4 m above a 2.0 m jump)
    for (float at : {19.0f, 19.1f, 19.2f, 19.3f, 19.4f})
    {
        World w;
        w.layer = 1;
        w.PlaceFeet({1.25f, kFloorY, 17.5f});
        w.RunJump(at, -1.0f, 0, false);
        w.Run(0.5f, Keys(1.0f), kNorth);
        CHECK(w.player.Feet().y < kFloorY + 0.5f);
    }
    {
        World w; // the closed door stops the body on the ledge (B)
        w.layer = 1;
        w.PlaceFeet({1.25f, st::kDoorwayLow, 20.2f});
        CHECK(w.player.Grounded());
        w.Run(1.5f, Keys(1.0f), kNorth);
        CHECK(w.player.Feet().z < st::kWallZ - 0.1f - 0.3f);
    }
    // the solution: push the block 2 east and 4 north onto the plate
    World w;
    w.PlaceFeet({-1.0f - 0.85f, kFloorY, 15.0f});
    w.PushMoves(2, kEast);
    CHECK(w.puzzle.BlockCell().x == 1 && w.puzzle.BlockCell().z == 15);
    w.PlaceFeet({1.0f, kFloorY, 15.0f - 0.85f});
    w.PushMoves(4, kNorth);
    CHECK(w.puzzle.BlockCell().x == 1 && w.puzzle.BlockCell().z == 19 && !w.puzzle.BlockMoving());
    CHECK(w.puzzle.DoorOpen(0) && w.puzzle.DoorOpen(1)); // pressed in both layers: by the block
    {
        World a = w; // in A the red panel still closes the doorway, even from the top of the block
        a.PlaceFeet({1.0f, kFloorY + 1.0f, 18.7f});
        a.RunJump(18.8f, -1.0f, 0, false);
        a.Run(1.5f, Keys(1.0f), kNorth);
        CHECK(a.player.Feet().z < st::kWallZ);
    }
    // in B: from the floor onto the block, from the block onto the ledge, through the doorway to the goal
    CHECK(w.Switch(1));
    w.PlaceFeet({1.0f, kFloorY, 16.6f});
    const bool onBlock = w.RunJump(17.4f);
    CHECK(onBlock && Near(w.player.Feet().y, kFloorY + 1.0f, 0.02f));
    w.RunJump(w.player.Feet().z + 0.05f, -1.0f, 0, false);
    w.Run(2.0f, Keys(1.0f), kNorth); // on through the doorway, up to the back wall
    w.Run(0.3f);
    std::printf("  part 3: the body ends at z %.2f, y %.2f\n", static_cast<double>(w.player.Feet().z), static_cast<double>(w.player.Feet().y));
    CHECK(st::ReachedGoal(w.player.Feet(), w.player.Grounded()));
    CHECK(w.player.Feet().z < 24.0f); // the back wall ends the room
}

void TestCheckpoints()
{
    CHECK(st::CheckpointFor({0.0f, kFloorY, -3.0f}, true, 0) == 0);
    CHECK(st::CheckpointFor({0.0f, kFloorY, 5.0f}, true, 0) == 1);
    CHECK(st::CheckpointFor({0.0f, kFloorY, 13.2f}, true, 1) == 2);
    CHECK(st::CheckpointFor({0.0f, kFloorY, 5.0f}, true, 2) == 2);  // never back
    CHECK(st::CheckpointFor({0.0f, kFloorY, 14.0f}, false, 1) == 1); // only while standing
    CHECK(st::CheckpointFor({0.0f, kFloorY, -3.0f}, true, -5) == 0);
    {
        World w; // the block cannot be pushed south of z = 15, so the checkpoint of part 3 (z = 13.2) stays free of it
        w.PlaceFeet({-1.0f, kFloorY, 15.85f});
        w.PushMoves(1, {0.0f, 0.0f, -1.0f}, 1.0f);
        CHECK(w.puzzle.BlockCell().z == 15 && w.puzzle.LastBlocker() == dl::puzzle::kBlockerBounds);
    }
    for (const st::Checkpoint& c : st::kCheckpoints)
    {
        World w; // every checkpoint is a place to stand in both layers, free of the block
        for (int layer = 0; layer < 2; ++layer)
        {
            w.layer = layer;
            w.PlaceFeet(c.feet);
            CHECK(w.player.Grounded() && Near(w.player.Feet().y, kFloorY, 0.02f) && w.puzzle.BodyFits(w.player, layer, w.spin));
        }
    }
    CHECK(!st::ReachedGoal({1.25f, st::kDoorwayLow, 20.2f}, true));
    CHECK(st::ReachedGoal({1.25f, st::kDoorwayLow, 22.5f}, true));
}

} // namespace

int main()
{
    TestTables();
    TestPart1();
    TestPart2();
    TestPart3();
    TestCheckpoints();
    std::printf("%d checks passed, %d failed\n", g_passed, g_failed);
    return g_failed == 0 ? 0 : 1;
}
