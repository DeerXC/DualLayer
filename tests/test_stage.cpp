// test_stage.cpp - plays the first stage (src/scene/Stage.h) with the real walking body (Player.h) and the real puzzle
// (Puzzle.h), no window, no GPU. Each part is shown to be solved by switching layers and not without it, and every way
// round the courtyard that was found is tried and must fail.
//   g++ -std=c++20 -O2 -Wall -Wextra -I src tests/test_stage.cpp -o test_stage
//   MSVC (Developer PowerShell):  .\tests\run_tests.ps1
// Header-only: there is no .cpp to add to the project. What is checked:
//   0. the tables: the walls of the courtyard are exactly the rows of scene/Courtyard.h; the drawn and the solid tables
//   1. the gate: the red wall stops the body in A (also at the edge of the terrace); in B the body walks through
//   2. the gap: two jumps in a row never cross it in A alone or in B alone; the jump from the stone with a switch to A in
//      the air lands on the strip at several timings; from the stone in B no jump reaches the courtyard
//   3. the overlook: walking off the strip lands in the courtyard
//   4. the courtyard: every push of the proved solution, played by the body from the cell behind the block (four of them
//      from inside a blue wall, in A); pushes in a row go on without a stop; the push that looks right from A is refused and
//      names the blue wall
//   5. the finale: with the block on the plate, the body climbs the block, the low red wall, runs along it, jumps and
//      switches to B in the air, lands on the apron, walks through the open door and reaches the goal
//   6. no way round: the strip reaches no wall top; the low wall cannot be climbed from the floor; the apron cannot be
//      reached from the block or without the switch; the door stays shut with the block off the plate; the doorway cannot
//      be reached from the low wall in A
//   7. the checkpoints: each a place to stand in both layers; the one in the courtyard is a cell the block can never be in
// Moves between the parts place the body (the floor between them is flat and needs no proof); every jump, push and switch
// is played frame by frame. Walking between pushes is not played: the solver proves that the body can get there (union of
// the two layers, climbing over the block). Returns 1 when a check fails.

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
namespace mz = dl::maze;

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

    /// Runs along `look` from where the body stands, jumps when the feet pass `jumpAt` along that axis (x for east, z for
    /// north), keeps going until the body lands or falls. `switchAfter` > 0: switch to `switchTo` that long after the jump.
    /// Returns true when it landed.
    bool RunJump(float jumpAt, const Vec3& look = kNorth, float switchAfter = -1.0f, int switchTo = 0)
    {
        const auto along = [&]() { return look.x > 0.5f ? player.Feet().x : player.Feet().z; };
        int guard = 0;
        while (along() < jumpAt && player.Grounded() && guard++ < 2000)
        {
            Frame(Keys(1.0f), look);
        }
        if (!player.Grounded())
        {
            return false; // it walked off before the jump
        }
        Frame(Keys(1.0f, true), look);
        float air = 0.0f;
        bool switched = false;
        for (int i = 0; i < 900; ++i)
        {
            if (switchAfter > 0.0f && !switched && air >= switchAfter)
            {
                switched = Switch(switchTo);
            }
            Frame(Keys(1.0f), look);
            air += kDt;
            if (player.Grounded() && air > 0.1f)
            {
                Run(0.2f); // let go: the body stops
                return !Fallen();
            }
            if (Fallen())
            {
                return false;
            }
        }
        return false;
    }

    /// Walks along (dx, dz) until the puzzle has finished `moves` more moves or the time is up, and lets go of the key as
    /// soon as the last of them has started (holding it would push the block on: see Puzzle.h). Returns the seconds.
    float PushMoves(int moves, int dx, int dz, float limitSeconds = 6.0f)
    {
        const Vec3 look{static_cast<float>(dx), 0.0f, static_cast<float>(dz)};
        const int goal = puzzle.Moves() + moves;
        float t = 0.0f;
        while (puzzle.Moves() < goal && t < limitSeconds)
        {
            const bool lastStarted = puzzle.BlockMoving() && puzzle.Moves() == goal - 1;
            Frame(Keys(lastStarted ? 0.0f : 1.0f), look);
            t += kDt;
        }
        Run(0.1f);
        return t;
    }
};

/// The courtyard grid of scene/Courtyard.h.
mz::Grid Courtyard()
{
    mz::Grid g;
    mz::Parse(dl::courtyard::kRows, dl::courtyard::kRowCount, g);
    return g;
}

Vec3 CellFeet(int gx, int gz)
{
    return {static_cast<float>(gx + st::kCellX0), kFloorY, static_cast<float>(gz + st::kCellZ0)};
}

const float kJumpPoints[] = {5.2f, 5.4f, 5.5f, 5.6f, 5.7f, 5.8f, 5.9f, 6.0f, 6.1f, 6.2f};

void TestTables()
{
    // every wall cell of the rows is inside exactly one generated wall of its layer, and every floor cell in none
    const mz::Grid g = Courtyard();
    bool match = true;
    for (int gz = 0; gz < g.d; ++gz)
    {
        for (int gx = 0; gx < g.w; ++gx)
        {
            const Vec3 c = CellFeet(gx, gz);
            int inA = 0;
            int inB = 0;
            for (int i = st::kFixedCount; i < st::kStageStaticCount; ++i)
            {
                const SceneObject& o = st::kStageStatics[static_cast<std::size_t>(i)];
                const bool inside = std::fabs(c.x - o.position[0]) < 0.5f * o.scale[0] && std::fabs(c.z - o.position[2]) < 0.5f * o.scale[2];
                inA += (inside && o.layerMask == kLayerA) ? 1 : 0;
                inB += (inside && o.layerMask == kLayerB) ? 1 : 0;
            }
            const mz::Cell cell = g.At(gx, gz);
            match = match && inA == (cell == mz::Cell::RedWall ? 1 : 0) && inB == (cell == mz::Cell::BlueWall ? 1 : 0);
        }
    }
    CHECK(match);
    // the low wall of the finale is low; every other wall of the courtyard is high
    for (int i = st::kFixedCount; i < st::kStageStaticCount; ++i)
    {
        const SceneObject& o = st::kStageStatics[static_cast<std::size_t>(i)];
        const bool finale = Near(o.position[0], st::kFinaleX, 1e-4f) && o.layerMask == kLayerA;
        CHECK(Near(o.scale[1], finale ? st::kFinaleHeight : st::kWallHeight, 1e-4f));
        CHECK(Near(o.position[1] - 0.5f * o.scale[1], kFloorY, 1e-4f));
    }
    World w;
    int count = 0;
    w.puzzle.Solids(1, count);
    CHECK(count == st::kStageStaticCount + 2); // the fixed objects, the block, the closed door
    SceneObject drawn[st::kMaxDrawObjects];
    CHECK(st::BuildDrawTable(w.puzzle, false, drawn) == st::kStageStaticCount + 4); // + block, door, plate, goal light
    CHECK(st::BuildDrawTable(w.puzzle, true, drawn) == st::kStageStaticCount + 3);  // the goal light taken
}

void TestHeights()
{
    // the measures the stage is built on, against the walking body (scene/Player.h): a wall the solver treats as a wall must
    // be out of reach from the top of the block; the low wall must be out of reach from the floor and in reach from the
    // block; the walls round the courtyard must be out of reach from the terrace
    const float jump = Player::Settings{}.jumpHeight;
    const float block = dl::puzzle::Puzzle::kBlockSize;
    CHECK(st::kWallHeight > block + jump + 0.3f);
    CHECK(st::kFinaleHeight > jump + 0.3f && st::kFinaleHeight < block + jump - 0.3f);
    CHECK(kFloorY + st::kBoundaryHeight > st::kTerraceY + jump + 0.3f);
    CHECK(st::kTerraceY - kFloorY > jump + 0.3f); // the terrace cannot be climbed back from the courtyard
}

void TestGate()
{
    World a; // A: the red gate stops the body, also at the edge of the terrace
    a.Run(3.0f, Keys(1.0f), kNorth);
    CHECK(a.player.Feet().z < st::kGateZ - 0.25f - 0.3f);
    World edge;
    edge.PlaceFeet({2.7f, st::kTerraceY, -1.0f});
    edge.Run(3.0f, Keys(1.0f), kNorth);
    CHECK(edge.player.Feet().z < st::kGateZ - 0.25f - 0.3f);
    World b; // B: it is not there
    CHECK(b.Switch(1));
    b.Run(2.0f, Keys(1.0f), kNorth);
    CHECK(b.player.Feet().z > 4.0f && b.player.Grounded() && Near(b.player.Feet().y, st::kTerraceY, 0.02f));
}

void TestGap()
{
    const auto onStone = [](const Player& p, int layer) {
        return layer == 1 && p.Feet().z >= st::kStoneNear - 0.4f && p.Feet().z <= st::kStoneFar + 0.4f;
    };
    for (int layer = 0; layer < 2; ++layer) // one layer alone, two jumps in a row: never beyond the gap
    {
        for (float at : kJumpPoints)
        {
            World w;
            w.layer = layer;
            w.PlaceFeet({0.0f, st::kTerraceY, 4.5f});
            bool beyond = false;
            if (w.RunJump(at))
            {
                beyond = w.player.Feet().z > st::kTerraceEnd + 0.4f && !onStone(w.player, layer);
                if (!beyond && w.player.Feet().z > st::kTerraceEnd + 0.4f && w.RunJump(w.player.Feet().z + 0.05f))
                {
                    beyond = !onStone(w.player, layer);
                }
            }
            CHECK(!beyond);
        }
    }
    int ways = 0; // the way: B, onto the stone, then on with a switch to A in the air
    for (float at : kJumpPoints)
    {
        World w;
        CHECK(w.Switch(1));
        w.PlaceFeet({0.0f, st::kTerraceY, 4.5f});
        if (!w.RunJump(at) || !onStone(w.player, 1))
        {
            continue;
        }
        for (float after : {0.15f, 0.25f, 0.35f})
        {
            World t = w;
            if (t.RunJump(t.player.Feet().z + 0.05f, kNorth, after, 0) && t.layer == 0 && t.player.Feet().z > st::kStripNear &&
                Near(t.player.Feet().y, st::kTerraceY, 0.02f))
            {
                ++ways;
            }
        }
    }
    std::printf("  gap: %d of the tried timings cross it\n", ways);
    CHECK(ways >= 3); // not a pixel-perfect trick
    for (float at : {8.0f, 8.3f, 8.6f, 8.7f}) // from the stone in B, the courtyard is out of reach: the strip must be used
    {
        World w;
        w.layer = 1;
        w.PlaceFeet({0.0f, st::kTerraceY, 7.9f});
        CHECK(!w.RunJump(at));
    }
}

void TestOverlook()
{
    World w;
    w.PlaceFeet({0.0f, st::kTerraceY, 12.5f});
    w.Run(2.0f, Keys(1.0f), kNorth);
    w.Run(0.5f);
    CHECK(w.player.Grounded() && Near(w.player.Feet().y, kFloorY, 0.02f) && w.player.Feet().z > st::kCourtSouth);
    CHECK(st::CheckpointFor(w.player.Feet(), true, 1) == 2);
}

/// Plays the proved solution: before each push the body is put in the cell behind the block, in a layer where it fits
/// there, and walks into the block until it has moved one cell. Returns the world with the block on the plate.
World PlaySolution(int& fromInsideAWall, bool& allMoved)
{
    const mz::Grid g = Courtyard();
    const mz::Result solution = mz::Solve(g);
    World w;
    w.PlaceFeet(CellFeet(3, 0));
    fromInsideAWall = 0;
    allMoved = solution.solved;
    for (const mz::Push& p : solution.path)
    {
        const int bx = g.X(p.blockCell);
        const int bz = g.Z(p.blockCell);
        const mz::Cell behind = g.At(bx - p.dx, bz - p.dz);
        w.layer = behind == mz::Cell::RedWall ? 1 : 0; // stand where the body fits (a blue wall cell: A)
        fromInsideAWall += behind == mz::Cell::BlueWall || behind == mz::Cell::RedWall ? 1 : 0;
        Vec3 feet = CellFeet(bx - p.dx, bz - p.dz);
        feet.x += 0.1f * static_cast<float>(p.dx); // close to the face, not touching
        feet.z += 0.1f * static_cast<float>(p.dz);
        w.PlaceFeet(feet);
        const dl::puzzle::Cell before = w.puzzle.BlockCell();
        w.PushMoves(1, p.dx, p.dz, 3.0f);
        const dl::puzzle::Cell after = w.puzzle.BlockCell();
        allMoved = allMoved && after.x == before.x + p.dx && after.z == before.z + p.dz;
    }
    return w;
}

void TestCourtyard()
{
    int inside = 0;
    bool moved = false;
    World w = PlaySolution(inside, moved);
    CHECK(moved);
    CHECK(inside >= 1);
    std::printf("  courtyard: the %d pushes of the solution played, %d from inside a wall of the other layer\n", dl::courtyard::kPushes, inside);
    CHECK(w.puzzle.BlockCell().x == st::kPlateCell.x && w.puzzle.BlockCell().z == st::kPlateCell.z);
    CHECK(w.puzzle.DoorOpen(0) && w.puzzle.DoorOpen(1));
    {
        // the push that looks right from A: west into the blue wall. Refused, and the blue wall is named (it flashes)
        World a;
        const Vec3 block = CellFeet(5, 4);
        a.PlaceFeet({block.x + 0.9f, kFloorY, block.z});
        a.PushMoves(1, -1, 0, 1.0f);
        CHECK(a.puzzle.Moves() == 0 && a.puzzle.BlockedFlash() > 0.0f);
        const int blocker = a.puzzle.LastBlocker();
        CHECK(blocker >= st::kFixedCount && st::kStageStatics[static_cast<std::size_t>(blocker)].layerMask == kLayerB);
    }
    {
        // pushes in a row go on without a stop: from the start, push the block south twice (two free cells below it)
        World c;
        c.layer = 1; // north of the block is the low red wall: the body stands there in B
        const Vec3 block = CellFeet(5, 4);
        c.PlaceFeet({block.x, kFloorY, block.z + 0.9f});
        const float seconds = c.PushMoves(2, 0, -1, 3.0f);
        CHECK(c.puzzle.Moves() == 2);
        // the first push waits kPushSeconds, then two cells at kSlideSpeed: no second wait
        CHECK(seconds < dl::puzzle::Puzzle::kPushSeconds + 2.0f / dl::puzzle::Puzzle::kSlideSpeed + 0.15f);
    }
}

void TestFinale()
{
    int inside = 0;
    bool moved = false;
    World w = PlaySolution(inside, moved);
    // onto the block in A: from the floor south of it, a jump north
    w.layer = 0;
    const Vec3 plate = CellFeet(4, 5);
    w.PlaceFeet({plate.x, kFloorY, plate.z - 1.0f});
    w.Frame(Keys(1.0f, true), kNorth);
    w.Run(0.3f, Keys(1.0f), kNorth); // forward for a moment, then let go: the block top is only 1 m deep
    w.Run(0.6f);
    CHECK(w.player.Grounded() && Near(w.player.Feet().y, kFloorY + 1.0f, 0.02f));
    // onto the low red wall: a jump east from the top of the block
    w.PlaceFeet({plate.x - 0.2f, kFloorY + 1.0f, plate.z});
    CHECK(w.RunJump(plate.x, kEast) && Near(w.player.Feet().y, st::kFinaleTop, 0.02f));
    // along its top, a jump over the moat, a switch to B in the air: the apron
    w.PlaceFeet({st::kFinaleX, st::kFinaleTop, plate.z});
    World noSwitch = w;
    World early = w;
    CHECK(w.RunJump(st::kCourtNorth - 0.25f, kNorth, 0.2f, 1) && w.layer == 1 && Near(w.player.Feet().y, st::kDoorwayLow, 0.02f));
    CHECK(w.player.Feet().z > st::kApronSouth);
    // through the open door to the goal
    w.Run(2.0f, Keys(1.0f), kNorth);
    CHECK(st::ReachedGoal(w.player));
    // without the switch the body falls into the moat
    CHECK(!noSwitch.RunJump(st::kCourtNorth - 0.25f));
    // with the switch made before the jump, the low wall is gone and the body is on the floor
    CHECK(early.Switch(1));
    early.Run(0.6f);
    CHECK(early.player.Grounded() && Near(early.player.Feet().y, kFloorY, 0.02f));
}

void TestNoWayRound()
{
    // from the strip (A), jumps north, north-east and north-west land on the courtyard floor, never on a wall
    for (float x : {-2.5f, 0.0f, 1.5f, 2.5f})
    {
        for (const Vec3& look : {kNorth, Vec3{0.6f, 0.0f, 0.8f}, Vec3{-0.6f, 0.0f, 0.8f}})
        {
            World w;
            w.PlaceFeet({x, st::kTerraceY, 12.6f});
            int guard = 0;
            while (w.player.Feet().z < 13.3f && w.player.Grounded() && guard++ < 400)
            {
                w.Frame(Keys(1.0f), look);
            }
            w.Frame(Keys(1.0f, true), look);
            w.Run(1.5f, Keys(1.0f), look);
            w.Run(0.5f);
            CHECK(w.player.Grounded() && w.player.Feet().y < kFloorY + 0.05f);
        }
    }
    // the low red wall cannot be climbed from the floor (A), at any point along it
    for (int gz = 5; gz <= 7; ++gz)
    {
        World w;
        const Vec3 c = CellFeet(4, gz);
        w.PlaceFeet({c.x - 0.6f, kFloorY, c.z});
        w.RunJump(c.x - 0.3f, kEast);
        w.Run(0.5f, Keys(1.0f), kEast);
        CHECK(w.player.Feet().y < kFloorY + 0.5f);
    }
    // the apron cannot be reached from the block on the plate (B), however the jump is aimed
    {
        int inside = 0;
        bool moved = false;
        const World solved = PlaySolution(inside, moved);
        const Vec3 plate = CellFeet(4, 5);
        for (const Vec3& look : {kNorth, Vec3{0.45f, 0.0f, 0.9f}, Vec3{0.7f, 0.0f, 0.7f}})
        {
            World w = solved;
            w.layer = 1;
            w.PlaceFeet({plate.x, kFloorY + 1.0f, plate.z - 0.3f});
            w.Frame(Keys(1.0f, true), look);
            w.Run(1.5f, Keys(1.0f), look);
            CHECK(w.Fallen() || w.player.Feet().y < st::kFinaleTop - 0.5f);
        }
    }
    // with the block off the plate the door stays shut: on the apron in B, the body cannot pass
    {
        World w;
        w.layer = 1;
        w.PlaceFeet({st::kFinaleX, st::kDoorwayLow, st::kApronSouth + 0.5f});
        CHECK(w.player.Grounded());
        w.Run(2.0f, Keys(1.0f), kNorth);
        CHECK(w.player.Feet().z < st::kDoorSouth - 0.3f && !st::ReachedGoal(w.player));
    }
    // from the top of the low wall in A the doorway (3.5 m away) is out of reach: the apron of B is the only landing
    {
        World w;
        w.PlaceFeet({st::kFinaleX, st::kFinaleTop, st::kCourtNorth - 2.5f});
        CHECK(!w.RunJump(st::kCourtNorth - 0.25f));
    }
}

void TestCheckpoints()
{
    mz::BlockCells cells;
    const mz::Grid g = Courtyard();
    mz::Analyse(g, &cells);
    const st::Checkpoint& court = st::kCheckpoints[2];
    const int gx = static_cast<int>(std::lround(court.feet.x)) - st::kCellX0;
    const int gz = static_cast<int>(std::lround(court.feet.z)) - st::kCellZ0;
    CHECK(g.Inside(gx, gz) && cells.kind[static_cast<std::size_t>(g.Index(gx, gz))] == 0); // the block is never there
    for (const st::Checkpoint& c : st::kCheckpoints)
    {
        for (int layer = 0; layer < 2; ++layer)
        {
            World w;
            w.layer = layer;
            w.PlaceFeet(c.feet);
            CHECK(w.player.Grounded() && Near(w.player.Feet().y, c.feet.y, 0.02f) && w.puzzle.BodyFits(w.player, layer, w.spin));
        }
    }
    CHECK(st::CheckpointFor({0.0f, st::kTerraceY, -3.0f}, true, 0) == 0);
    CHECK(st::CheckpointFor({0.0f, st::kTerraceY, 5.0f}, true, 0) == 1);
    CHECK(st::CheckpointFor({0.0f, st::kTerraceY, 5.0f}, true, 2) == 2);  // never back
    CHECK(st::CheckpointFor({0.0f, kFloorY, 15.0f}, false, 1) == 1); // only while standing
}

} // namespace

int main()
{
    TestTables();
    TestHeights();
    TestGate();
    TestGap();
    TestOverlook();
    TestCourtyard();
    TestFinale();
    TestNoWayRound();
    TestCheckpoints();
    std::printf("%d checks passed, %d failed\n", g_passed, g_failed);
    return g_failed == 0 ? 0 : 1;
}
