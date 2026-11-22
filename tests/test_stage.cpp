// test_stage.cpp - walks the whole map (src/scene/Stage.h) with the real walking body (Player.h), the real puzzle
// (Puzzle.h) and the seals, no window, no GPU: every room reached, every shortcut taken, every closed door holding, and the
// jumps that would skip a part tried.
//   g++ -std=c++20 -O2 -Wall -Wextra -I src tests/test_stage.cpp -o test_stage
//   MSVC (Developer PowerShell):  .\tests\run_tests.ps1 -Only stage
// Header-only: there is no .cpp to add to the project. What is checked:
//   0. the tables: the walls of the courtyard are exactly the rows of scene/Courtyard.h; every ramp is at most 27 degrees;
//      the heights against the body (every wall out of reach from every place next to it); the drawn and solid tables fit
//   1. the prologue: the gate (A stops, B passes); the gap (neither layer alone crosses it, even with the running jump; from
//      the stone in B alone the curtain stops every jump; the stone in B and a switch to A in the air crosses it at most of
//      the tried moments); the overlook lands in the courtyard
//   2. the courtyard: the 10 pushes of the proved solution played by the body; the push that looks right from A is refused
//      and names the blue wall; the door to the hub holds in A (the red panel) and in B until the block is on the plate
//   3. the wings, walked from the hub with the real body (it walks up and down every ramp): the west wing down to its seal
//      and back up the shortcut through the door that the seal opened; the east wing up to the top of the courtyard wall,
//      its seal, the drop onto the gallery and from there into the hub (and the gallery not reached again from the floor); the north wing up two ramps to its seal, along the ledge through the window of
//      the hub wall and off it
//   4. the tower: shut until the three seals are taken, then walked up its six ramps to the balcony, the goal
//   5. no way round: the tower door; the shortcut door from both sides; the east wing from the terrace; the courtyard wall
//      top and the ledge from the hub; from the ledge, no jump in any direction lands on the tower or on the hub walls; from
//      the landing of the north wing, no jump lands on its walls; from the courtyard wall top, none lands on the hub walls
//   6. the rooms: every entry is in its own room and is a place to stand in both layers; the courtyard entry is a cell the
//      block can never be in
// The walks between the points of a path are played frame by frame (the body steers to each point). Returns 1 when a check
// fails.

#include "scene/Stage.h"

#include <cmath>
#include <cstdio>
#include <initializer_list>
#include <string_view>

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

/// A point that touches a seal: what a body standing at it would do (for the tests that need the seals taken without the
/// walk; the walks themselves are in TestWings).
struct Touch
{
    Vec3 c;
    float radius = 0.35f;
    Vec3 Ball(int) const
    {
        return c;
    }
    const Touch& GetSettings() const
    {
        return *this;
    }
};

constexpr int kMaxSolids = dl::puzzle::Puzzle::kMaxSolids + 4;

/// The map as the program runs it: the solid table of the layer of the body (the puzzle and the closed doors of the map),
/// then the body, then the puzzle and the seals.
struct World
{
    dl::puzzle::Puzzle puzzle;
    st::SealSet seals;
    Player player;
    int layer = 0;
    float spin = 0.0f;
    SceneObject solids[kMaxSolids] = {};

    World()
    {
        if (!puzzle.Init(st::MakeStage()))
        {
            ++g_failed;
            std::printf("FAIL: the stage does not Init\n");
        }
        player.SetSpawn(st::kRooms[st::kStartRoom].entry);
        player.Respawn();
    }

    World(const World& o) : puzzle(o.puzzle), seals(o.seals), player(o.player), layer(o.layer), spin(o.spin)
    {
    }

    void TakeAllSeals()
    {
        for (const st::Seal& s : st::kSeals)
        {
            seals.Collect(Touch{s.centre});
        }
    }

    void Frame(const Player::Input& input, const Vec3& look)
    {
        int count = 0;
        const SceneObject* table = puzzle.Solids(layer, count);
        for (int i = 0; i < count; ++i)
        {
            solids[i] = table[i];
        }
        count = st::AddMapSolids(seals, solids, count, kMaxSolids);
        player.SetScene(solids, count);
        spin += kDt;
        player.Update(input, look, layer, spin, kDt);
        puzzle.Update(player, kDt);
        seals.Collect(player);
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

    /// Steers the body to (x, z) on foot: it looks at the point and walks until it is within 0.3 m or `limit` seconds
    /// pass. Returns true when it got there and is not falling.
    bool WalkTo(float x, float z, float limit = 12.0f)
    {
        float t = 0.0f;
        while (t < limit && !Fallen())
        {
            const float dx = x - player.Feet().x;
            const float dz = z - player.Feet().z;
            const float d = std::sqrt(dx * dx + dz * dz);
            if (d < 0.3f)
            {
                Run(0.4f); // let go: the body stops and lands
                return !Fallen();
            }
            Frame(Keys(1.0f), {dx / d, 0.0f, dz / d});
            t += kDt;
        }
        return false;
    }

    /// Walks along a list of points.
    bool WalkPath(std::initializer_list<Vec3> points)
    {
        for (const Vec3& p : points)
        {
            if (!WalkTo(p.x, p.z))
            {
                std::printf("  (stopped on the way to %.1f %.1f, at %.2f %.2f %.2f)\n", static_cast<double>(p.x), static_cast<double>(p.z),
                            static_cast<double>(player.Feet().x), static_cast<double>(player.Feet().y), static_cast<double>(player.Feet().z));
                return false;
            }
        }
        return true;
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

    /// Runs north from where the body stands, jumps when the feet pass `jumpAt`, keeps going until the body lands or falls.
    /// `switchAfter` > 0: switch to `switchTo` that long after the jump. Returns true when it landed.
    bool RunJump(float jumpAt, float switchAfter = -1.0f, int switchTo = 0)
    {
        int guard = 0;
        while (player.Feet().z < jumpAt && player.Grounded() && guard++ < 2000)
        {
            Frame(Keys(1.0f), kNorth);
        }
        if (!player.Grounded())
        {
            return false;
        }
        Frame(Keys(1.0f, true), kNorth);
        float air = 0.0f;
        bool switched = false;
        for (int i = 0; i < 900; ++i)
        {
            if (switchAfter > 0.0f && !switched && air >= switchAfter)
            {
                switched = Switch(switchTo);
            }
            Frame(Keys(1.0f), kNorth);
            air += kDt;
            if (player.Grounded() && air > 0.1f)
            {
                Run(0.2f);
                return !Fallen();
            }
            if (Fallen())
            {
                return false;
            }
        }
        return false;
    }

    /// A running jump along `look` from `feet` (a run-up of `runUp` seconds first), the key held until the body lands or 2 s
    /// pass. Returns the height of the feet where it ends.
    float JumpFrom(const Vec3& feet, const Vec3& look, float runUp)
    {
        PlaceFeet(feet);
        Run(runUp, Keys(1.0f), look);
        Frame(Keys(1.0f, true), look);
        for (int i = 0; i < 240 && !(player.Grounded() && i > 12); ++i)
        {
            Frame(Keys(1.0f), look);
        }
        Run(0.3f);
        return player.Feet().y;
    }

    /// Walks along (dx, dz) until the puzzle has finished `moves` more moves, letting go as soon as the last one started.
    void PushMoves(int moves, int dx, int dz, float limitSeconds = 6.0f)
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
    }

    bool At(float x, float y, float z, float eps = 0.6f) const
    {
        return player.Grounded() && Near(player.Feet().x, x, eps) && Near(player.Feet().y, y, 0.05f) && Near(player.Feet().z, z, eps);
    }
};

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

void TestTables()
{
    const mz::Grid g = Courtyard();
    bool match = true; // every wall cell of the rows is inside exactly one generated wall of its layer
    for (int gz = 0; gz < g.d; ++gz)
    {
        for (int gx = 0; gx < g.w; ++gx)
        {
            const Vec3 c = CellFeet(gx, gz);
            int inA = 0;
            int inB = 0;
            for (int i = st::kFixedCount + st::kRampCount; i < st::kStageStaticCount; ++i)
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
    for (const SceneObject& r : st::kRamps) // every ramp: at most 27 degrees, both layers, a box with a size
    {
        CHECK(std::fabs(r.tilt) <= 0.4637f + 1e-4f && r.layerMask == kLayerBoth && r.scale[0] > 0.0f && r.scale[2] > 0.0f);
    }
    const float jump = Player::Settings{}.jumpHeight;
    const float margin = 0.3f;
    CHECK(st::kWallHeight > dl::puzzle::Puzzle::kBlockSize + jump + margin);              // a puzzle wall, from the block
    CHECK(st::kCityWallTop - kFloorY > dl::puzzle::Puzzle::kBlockSize + jump + margin);   // a city wall, from the block
    CHECK(st::kCourtWallTop > st::kTerraceY + jump + margin);                            // the courtyard walls, from the terrace
    CHECK(st::kCourtWallTop > kFloorY + st::kWallHeight + jump + margin);                // and from the tops of the puzzle walls
    CHECK(st::kCurtainTop > st::kEastHighY + jump + margin);                             // the curtain, from the east path
    CHECK(st::kCurtainTop > st::kTerraceY + jump + margin);                              // and from the strip
    CHECK(st::kParapet > jump + margin);                                                 // a parapet, from its floor
    CHECK(st::kGalleryY - kFloorY > jump + margin);                                      // the gallery, from the hub floor
    CHECK(st::kEastHighY - st::kGalleryY > jump + margin);                               // the east path, from the gallery
    CHECK(st::kEastHighY > st::kTerraceY + jump + margin);                               // the east wing, from the terrace
    CHECK(st::kNorthHighY - 0.2f > kFloorY + jump + 1.55f + 0.5f);                       // the ledge, from the hub floor
    CHECK(st::kHubWallTop > st::kNorthHighY + jump + margin);                            // the hub walls, from the ledge
    CHECK(st::kNorthWallTop > st::kNorthHighY + jump + margin);                          // the north hall walls, from its landing
    CHECK(st::kTowerTop > st::kNorthHighY + jump + margin);                              // the tower, from the ledge
    CHECK(st::kTowerTop > st::kHubWallTop + jump + margin);                              // and from the hub walls
    CHECK(st::kWindowTop - st::kNorthHighY > Player::Settings{}.eyeHeight + 0.6f);       // the window is high enough to walk through
    CHECK(st::kFallY < st::kWestLowY - 2.0f);                                            // the lowest room is far above the fall line
    World w; // the drawn and solid tables fit
    static SceneObject drawn[st::kMaxDrawObjects];
    int n = st::BuildDrawTable(w.puzzle, false, drawn);
    n = st::AddMapDraw(w.seals, drawn, n, st::kMaxDrawObjects);
    CHECK(n == st::kStageStaticCount + 4 + st::kLandmarkCount + 2 + 2 * st::kSealCount);
    CHECK(n <= st::kMaxDrawObjects);
}

void TestPrologue()
{
    World a; // the gate: A stops, B passes
    a.Run(3.0f, Keys(1.0f), kNorth);
    CHECK(a.player.Feet().z < st::kGateZ - 0.25f - 0.3f);
    World b;
    CHECK(b.Switch(1));
    b.Run(1.2f, Keys(1.0f), kNorth);
    b.Run(0.3f);
    CHECK(b.player.Feet().z > st::kGateZ + 0.5f && b.player.Feet().z < st::kTerraceEnd && Near(b.player.Feet().y, st::kTerraceY, 0.02f));
    // the gap: one layer alone never crosses it, two jumps in a row, from several jump points
    const float points[] = {4.6f, 5.0f, 5.4f, 5.6f, 5.8f, 6.0f, 6.2f};
    for (int layer = 0; layer < 2; ++layer)
    {
        for (float at : points)
        {
            World w;
            w.layer = layer;
            w.PlaceFeet({0.0f, st::kTerraceY, 3.6f});
            bool beyond = false;
            if (w.RunJump(at))
            {
                const bool onStone = layer == 1 && w.player.Feet().z < st::kStoneFar + 0.4f;
                beyond = w.player.Feet().z > st::kTerraceEnd + 0.4f && !onStone;
                if (!beyond && w.player.Feet().z > st::kTerraceEnd + 0.4f && w.RunJump(w.player.Feet().z + 0.05f))
                {
                    beyond = w.player.Feet().z > st::kStoneFar + 0.4f;
                }
            }
            CHECK(!beyond);
        }
    }
    for (float at : {7.2f, 8.0f, 9.0f, 9.9f}) // from the stone in B, alone: the curtain stops the flight, the body falls into the gap
    {
        World w;
        w.layer = 1;
        w.PlaceFeet({0.0f, st::kTerraceY, 7.1f});
        CHECK(!w.RunJump(at));
    }
    int ways = 0; // the way: B, onto the stone, then on with a switch to A in the air
    int tried = 0;
    for (float at : {5.0f, 5.4f, 5.8f, 6.0f})
    {
        World w;
        CHECK(w.Switch(1));
        w.PlaceFeet({0.0f, st::kTerraceY, 4.0f});
        if (!w.RunJump(at) || w.player.Feet().z < st::kStoneNear - 0.4f || w.player.Feet().z > st::kStoneFar + 0.4f)
        {
            continue;
        }
        for (float after : {0.10f, 0.20f, 0.30f, 0.40f})
        {
            World t = w;
            ++tried;
            if (t.RunJump(t.player.Feet().z + 0.05f, after, 0) && t.layer == 0 && t.player.Feet().z > st::kStripNear &&
                Near(t.player.Feet().y, st::kTerraceY, 0.02f))
            {
                ++ways;
            }
        }
    }
    std::printf("  gap: %d of %d tried timings cross it\n", ways, tried);
    CHECK(ways * 4 >= tried * 3); // most moments work: a window, not a trick
    World o; // the overlook: walking off the strip lands in the courtyard
    o.PlaceFeet({0.0f, st::kTerraceY, 12.5f});
    o.Run(2.0f, Keys(1.0f), kNorth);
    o.Run(0.5f);
    CHECK(o.player.Grounded() && Near(o.player.Feet().y, kFloorY, 0.02f) && st::RoomAt(o.player.Feet()) == 3);
}

/// Plays the proved solution of the courtyard. Returns the world with the block on the plate, the body in the courtyard.
World PlaySolution(bool& allMoved)
{
    const mz::Grid g = Courtyard();
    const mz::Result solution = mz::Solve(g);
    World w;
    w.PlaceFeet(CellFeet(3, 0));
    allMoved = solution.solved;
    for (const mz::Push& p : solution.path)
    {
        const int bx = g.X(p.blockCell);
        const int bz = g.Z(p.blockCell);
        const mz::Cell behind = g.At(bx - p.dx, bz - p.dz);
        w.layer = behind == mz::Cell::RedWall ? 1 : 0;
        Vec3 feet = CellFeet(bx - p.dx, bz - p.dz);
        feet.x += 0.1f * static_cast<float>(p.dx);
        feet.z += 0.1f * static_cast<float>(p.dz);
        w.PlaceFeet(feet);
        const dl::puzzle::Cell before = w.puzzle.BlockCell();
        w.PushMoves(1, p.dx, p.dz, 3.0f);
        const dl::puzzle::Cell after = w.puzzle.BlockCell();
        allMoved = allMoved && after.x == before.x + p.dx && after.z == before.z + p.dz;
    }
    return w;
}

/// From the solved courtyard through the door, in B, into the hub. Returns the world standing in the hub.
World ReachTheHub(bool& ok)
{
    World w = PlaySolution(ok);
    w.layer = 1;
    w.PlaceFeet({st::kDoorX, kFloorY, 20.5f});
    ok = ok && w.WalkPath({{st::kDoorX, kFloorY, 23.5f}}) && st::RoomAt(w.player.Feet()) == 4;
    return w;
}

void TestCourtyard()
{
    bool moved = false;
    World w = PlaySolution(moved);
    CHECK(moved);
    CHECK(w.puzzle.BlockCell().x == st::kPlateCell.x && w.puzzle.BlockCell().z == st::kPlateCell.z && w.puzzle.DoorOpen(1));
    {
        World a; // the push that looks right from A: refused, and the blue wall is named
        const Vec3 block = CellFeet(5, 4);
        a.PlaceFeet({block.x + 0.9f, kFloorY, block.z});
        a.PushMoves(1, -1, 0, 1.0f);
        CHECK(a.puzzle.Moves() == 0 && a.puzzle.BlockedFlash() > 0.0f);
        const int blocker = a.puzzle.LastBlocker();
        CHECK(blocker >= 0 && st::kStageStatics[static_cast<std::size_t>(blocker)].layerMask == kLayerB);
    }
    for (int layer = 0; layer < 2; ++layer) // before the block is on the plate, the door holds in both layers
    {
        World c;
        c.layer = layer;
        c.PlaceFeet({st::kDoorX, kFloorY, 20.5f});
        c.Run(2.0f, Keys(1.0f), kNorth);
        CHECK(c.player.Feet().z < st::kDoorSouth);
    }
    {
        World a = w; // with the block on the plate: in A the red panel still holds
        a.layer = 0;
        a.PlaceFeet({st::kDoorX, kFloorY, 20.5f});
        a.Run(2.0f, Keys(1.0f), kNorth);
        CHECK(a.player.Feet().z < st::kDoorSouth);
    }
    bool ok = false;
    ReachTheHub(ok);
    CHECK(ok);
}

void TestWings()
{
    bool ok = false;
    const World hub = ReachTheHub(ok);
    CHECK(ok);
    {
        // the west wing: down to the seal, back up the shortcut through the door that the seal opened
        World w = hub;
        w.layer = 0;
        CHECK(w.WalkPath({{-6.0f, kFloorY, 25.0f}, {-6.0f, kFloorY, 31.0f}, {-12.0f, kFloorY, 31.0f}, {-20.0f, kFloorY, 31.0f},
                          {-20.0f, kFloorY, 23.0f}, {-20.0f, kFloorY, 10.0f}}));
        CHECK(w.At(-20.0f, st::kWestLowY, 10.0f) && st::RoomAt(w.player.Feet()) == 6);
        CHECK(!w.seals.Taken(st::kWestSeal));
        CHECK(w.WalkPath({{-22.0f, kFloorY, 4.0f}}));
        CHECK(w.seals.Taken(st::kWestSeal) && w.seals.ShortcutOpen());
        CHECK(w.WalkPath({{-11.0f, kFloorY, 10.0f}, {-11.0f, kFloorY, 23.5f}, {-7.0f, kFloorY, 23.5f}}));
        CHECK(w.At(-7.0f, kFloorY, 23.5f) && st::RoomAt(w.player.Feet()) == 4);
        World before = hub; // without the seal, the same door holds from the hub side
        before.PlaceFeet({-7.0f, kFloorY, 23.5f});
        before.Run(2.0f, Keys(1.0f), {-1.0f, 0.0f, 0.0f});
        CHECK(before.player.Feet().x > st::kHubWest - 0.1f);
        World under = hub; // and from below, up the shortcut ramp, it holds too
        under.PlaceFeet({-11.0f, st::kWestLowY, 10.0f});
        CHECK(under.WalkPath({{-11.0f, kFloorY, 23.5f}}));
        under.Run(2.0f, Keys(1.0f), kEast);
        CHECK(under.player.Feet().x < st::kHubWest - 0.4f);
    }
    {
        // the east wing: up to the top of the courtyard wall, the seal, the drop into the hub
        World w = hub;
        w.layer = 0;
        CHECK(w.WalkPath({{6.0f, kFloorY, 25.0f}, {6.0f, kFloorY, 31.0f}, {12.0f, kFloorY, 31.0f}, {20.0f, kFloorY, 31.0f},
                          {20.0f, kFloorY, 23.0f}, {20.0f, kFloorY, 8.0f}}));
        CHECK(w.At(20.0f, st::kEastHighY, 8.0f) && st::RoomAt(w.player.Feet()) == 8);
        CHECK(w.WalkPath({{st::kCourtEast + 0.75f, kFloorY, 8.0f}, {st::kCourtEast + 0.75f, kFloorY, 20.0f}}));
        CHECK(w.seals.Taken(1) && w.At(st::kCourtEast + 0.75f, st::kEastHighY, 20.0f));
        CHECK(w.WalkPath({{st::kCourtEast + 0.75f, kFloorY, 24.0f}})); // off the north end: onto the gallery
        CHECK(w.At(st::kCourtEast + 0.75f, st::kGalleryY, 24.0f) && std::string_view(st::kRooms[st::RoomAt(w.player.Feet())].name) == "gallery");
        CHECK(w.WalkPath({{8.0f, kFloorY, 24.0f}, {8.0f, kFloorY, 32.0f}, {5.5f, kFloorY, 32.0f}})); // along it, and off its west edge
        w.Run(0.5f);
        CHECK(w.At(5.5f, kFloorY, 32.0f) && st::RoomAt(w.player.Feet()) == 4);
        World back = w; // the gallery is not reached again from the hub floor
        back.PlaceFeet({5.5f, kFloorY, 30.0f});
        back.Run(0.3f, Keys(1.0f), kEast);
        back.Frame(Keys(1.0f, true), kEast);
        back.Run(1.2f, Keys(1.0f), kEast);
        CHECK(back.player.Feet().y < kFloorY + 0.5f);
    }
    {
        // the north wing: up two ramps to the seal, along the ledge through the window of the hub wall, and off it
        World w = hub;
        w.layer = 0;
        CHECK(w.WalkPath({{6.0f, kFloorY, 25.0f}, {6.0f, kFloorY, 38.0f}, {0.0f, kFloorY, 38.0f}, {0.0f, kFloorY, 43.0f},
                          {7.0f, kFloorY, 45.0f}, {7.0f, kFloorY, 59.0f}, {2.0f, kFloorY, 59.0f}, {2.0f, kFloorY, 45.0f}}));
        CHECK(w.At(2.0f, st::kNorthHighY, 45.0f));
        CHECK(w.WalkPath({{-7.0f, kFloorY, 45.0f}}));
        CHECK(w.seals.Taken(2));
        CHECK(w.WalkPath({{-7.0f, kFloorY, 40.25f}})); // in the window
        CHECK(w.At(-7.0f, st::kNorthHighY, 40.25f));
        CHECK(w.WalkPath({{-7.0f, kFloorY, 34.0f}}));
        w.Run(1.0f); // the drop is 7 m
        CHECK(w.At(-7.0f, kFloorY, 34.0f) && st::RoomAt(w.player.Feet()) == 4);
    }
}

void TestTower()
{
    bool ok = false;
    World w = ReachTheHub(ok);
    CHECK(ok);
    w.layer = 0;
    CHECK(w.WalkPath({{-2.5f, kFloorY, 25.5f}})); // shut: the door holds before the seals
    w.Run(2.0f, Keys(1.0f), kNorth);
    CHECK(w.player.Feet().z < st::kTowerSouth);
    w.TakeAllSeals(); // the walks to them are in TestWings
    CHECK(w.seals.TowerOpen());
    CHECK(w.WalkPath({{-2.5f, kFloorY, 28.2f}}));
    CHECK(st::RoomAt(w.player.Feet()) == 0);
    // six ramps: lanes 1, 2, 3 and again 1, 2, 3, each one level (2 m) higher, turning at the landings
    const float lane[3] = {-2.33f, 0.0f, 2.33f};
    const float north = 33.8f;
    const float south = 28.3f;
    for (int ramp = 0; ramp < st::kTowerRamps; ++ramp)
    {
        const float x = lane[ramp % 3];
        const float z = ramp % 2 == 0 ? north : south; // ramps 1, 3, 5 run north, 2, 4, 6 run south
        const float from = ramp % 2 == 0 ? south : north;
        CHECK(w.WalkPath({{x, kFloorY, from}, {x, kFloorY, z}}));
        CHECK(w.At(x, st::Level(ramp + 1), z, 0.5f));
    }
    CHECK(st::ReachedGoal(w.player));
}

/// True when a jump from `feet` along `look` (with a run-up of 0, 0.15 or 0.4 s) ever ends higher than `ceiling` or on
/// anything that is not the floor it started on or lower.
bool AnyJumpEndsAbove(const World& base, const Vec3& feet, float ceiling)
{
    const Vec3 dirs[8] = {{1, 0, 0}, {-1, 0, 0}, {0, 0, 1}, {0, 0, -1}, {0.7071f, 0, 0.7071f}, {0.7071f, 0, -0.7071f}, {-0.7071f, 0, 0.7071f}, {-0.7071f, 0, -0.7071f}};
    for (const Vec3& d : dirs)
    {
        for (float runUp : {0.0f, 0.15f, 0.4f})
        {
            World w = base;
            if (w.JumpFrom(feet, d, runUp) > ceiling)
            {
                return true;
            }
        }
    }
    return false;
}

void TestNoWayRound()
{
    {
        World w; // the east wing from the terrace: out of reach
        w.PlaceFeet({2.6f, st::kTerraceY, 5.0f});
        w.Frame(Keys(1.0f, true), kEast);
        w.Run(1.5f, Keys(1.0f), kEast);
        CHECK(w.player.Feet().y < st::kEastHighY - 1.0f);
    }
    bool ok = false;
    World hub = ReachTheHub(ok);
    {
        World w = hub; // the top of the courtyard wall from the hub: a jump at its north end does not reach it
        w.PlaceFeet({st::kCourtEast + 0.75f, kFloorY, 23.8f});
        w.Frame(Keys(-1.0f, true), kNorth);
        w.Run(1.5f, Keys(-1.0f), kNorth);
        CHECK(w.player.Feet().y < kFloorY + 0.5f);
    }
    {
        World w = hub; // the ledge of the north wing from the hub: out of reach
        w.PlaceFeet({-7.0f, kFloorY, 36.0f});
        w.Frame(Keys(1.0f, true), kNorth);
        w.Run(1.5f, Keys(1.0f), kNorth);
        CHECK(w.player.Feet().y < kFloorY + 0.5f);
    }
    for (int layer = 0; layer < 2; ++layer) // the tower door, in both layers, before the seals
    {
        World t = hub;
        t.layer = layer;
        t.PlaceFeet({-2.5f, kFloorY, 26.0f});
        t.Run(2.0f, Keys(1.0f), kNorth);
        CHECK(t.player.Feet().z < st::kTowerSouth);
    }
    // from the ledge (6 m), from the landing of the north wing (6 m) and from the courtyard wall top (4 m): no jump, in eight
    // directions with three run-ups, ends anywhere higher than the place it started from (the tower and the hub walls were
    // reached this way before)
    for (const Vec3& f : {Vec3{-5.4f, st::kNorthHighY, 37.4f}, Vec3{-7.0f, st::kNorthHighY, 37.4f}, Vec3{-5.4f, st::kNorthHighY, 39.0f},
                          Vec3{-8.6f, st::kNorthHighY, 38.0f}})
    {
        CHECK(!AnyJumpEndsAbove(hub, f, st::kNorthHighY + 0.3f));
    }
    for (const Vec3& f : {Vec3{-8.6f, st::kNorthHighY, 45.6f}, Vec3{3.6f, st::kNorthHighY, 45.6f}, Vec3{0.0f, st::kNorthHighY, 44.4f}})
    {
        CHECK(!AnyJumpEndsAbove(hub, f, st::kNorthHighY + 0.3f));
    }
    for (const Vec3& f : {Vec3{st::kCourtEast + 0.75f, st::kEastHighY, 21.5f}, Vec3{st::kCourtEast + 0.75f, st::kEastHighY, 22.6f}})
    {
        CHECK(!AnyJumpEndsAbove(hub, f, st::kEastHighY + 0.3f));
    }
}

void TestRooms()
{
    mz::BlockCells cells;
    const mz::Grid g = Courtyard();
    mz::Analyse(g, &cells);
    for (int i = 0; i < st::kRoomCount; ++i)
    {
        const st::Room& r = st::kRooms[i];
        CHECK(st::RoomAt(r.entry) == i);
        for (int layer = 0; layer < 2; ++layer)
        {
            World w;
            w.TakeAllSeals(); // the tower entry is behind its door
            w.layer = layer;
            w.PlaceFeet(r.entry);
            CHECK(w.player.Grounded() && Near(w.player.Feet().y, r.entry.y, 0.02f) && w.puzzle.BodyFits(w.player, layer, w.spin));
        }
    }
    const st::Room& court = st::kRooms[3];
    CHECK(court.puzzle);
    const int gx = static_cast<int>(std::lround(court.entry.x)) - st::kCellX0;
    const int gz = static_cast<int>(std::lround(court.entry.z)) - st::kCellZ0;
    CHECK(g.Inside(gx, gz) && cells.kind[static_cast<std::size_t>(g.Index(gx, gz))] == 0); // the block is never there
    CHECK(st::CurrentRoom({0.0f, kFloorY, 30.0f}, false, 7) == 7);  // in the air: the last room
    CHECK(st::CurrentRoom({0.0f, kFloorY, 25.0f}, true, 7) == 4);   // standing in the hub
    CHECK(st::CurrentRoom({50.0f, kFloorY, 25.0f}, true, 7) == 7);  // standing in no room: the last one
}

} // namespace

int main()
{
    TestTables();
    TestPrologue();
    TestCourtyard();
    TestWings();
    TestTower();
    TestNoWayRound();
    TestRooms();
    std::printf("%d checks passed, %d failed\n", g_passed, g_failed);
    return g_failed == 0 ? 0 : 1;
}
