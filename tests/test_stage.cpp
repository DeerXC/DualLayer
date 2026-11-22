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
//   4. the square: in A the pool is water (a body that walks off the rim goes under it), in B it is dry and walked down one
//      stair, round the cathedral and up the other; a jump climbs out of it anywhere; the causeway leads to the portal in
//      both layers. The cathedral: shut until the three seals are taken, then walked up its six ramps to the balcony, the goal
//   5. no way round: the tower door; the shortcut door from both sides; the east wing from the terrace; the courtyard wall
//      top and the ledge from the hub; from the ledge, no jump in any direction lands on the tower or on the hub walls; from
//      the landing of the north wing, no jump lands on its walls; from the courtyard wall top, none lands on the hub walls
//   6. the rooms: every entry is in its own room and is a place to stand in both layers; the courtyard entry is a cell the
//      block can never be in
// The walks between the points of a path are played frame by frame (the body steers to each point). Returns 1 when a check
// fails.

#include "scene/Stage.h"
#include "scene/WestWing.h"

#include <cmath>
#include <cstdio>
#include <array>
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

constexpr int kMaxSolids = dl::puzzle::Puzzle::kMaxSolids + 4 + dl::mech::kMaxGates + st::kLiftCount;

int Room(const char* name)
{
    const int i = st::RoomIndex(name);
    if (i < 0)
    {
        std::printf("FAIL: no room called %s\n", name);
        ++g_failed;
    }
    return i;
}

/// The map as the program runs it: the solid table of the layer of the body (the puzzle, the closed doors of the map and the
/// gates of the west wing), then the body, then the puzzle, the orbs and the seals.
struct World
{
    dl::puzzle::Puzzle puzzle;
    st::SealSet seals;
    dl::mech::MechanismSet mech;
    std::array<dl::shapes::Lift, st::kLiftCount> lifts = st::MakeLifts();
    Player player;
    int layer = 0;
    float spin = 0.0f;
    SceneObject solids[2][kMaxSolids] = {};
    int counts[2] = {0, 0};

    World()
    {
        if (!puzzle.Init(st::MakeStage()) || !mech.Init(st::WestWing()))
        {
            ++g_failed;
            std::printf("FAIL: the stage does not Init\n");
        }
        player.SetSpawn(st::kRooms[st::kStartRoom].entry);
        player.Respawn();
    }

    World(const World& o) : puzzle(o.puzzle), seals(o.seals), mech(o.mech), lifts(o.lifts), player(o.player), layer(o.layer), spin(o.spin)
    {
    }

    /// The solid tables of both layers, as they are now.
    void BuildSolids()
    {
        for (int l = 0; l < 2; ++l)
        {
            int count = 0;
            const SceneObject* table = puzzle.Solids(l, count);
            for (int i = 0; i < count; ++i)
            {
                solids[l][i] = table[i];
            }
            count = st::AddMapSolids(seals, solids[l], count, kMaxSolids);
            count = st::AddLifts(lifts, solids[l], count, kMaxSolids);
            counts[l] = mech.AddSolids(l, solids[l], count, kMaxSolids);
        }
    }

    /// E, looking along `look`.
    dl::mech::Action Use(const Vec3& look)
    {
        BuildSolids();
        int which = -1;
        const dl::mech::Action a = mech.Interact(player, layer, look, solids[layer], counts[layer], which);
        Run(0.6f); // a put-down orb falls and settles
        return a;
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
        st::UpdateLifts(lifts, player, [this](int seal) { return seals.Taken(seal); }, kDt);
        BuildSolids();
        player.SetScene(solids[layer], counts[layer]);
        spin += kDt;
        player.Update(input, look, layer, spin, kDt);
        puzzle.Update(player, kDt);
        BuildSolids();
        const int orbCountA = st::AddWaterForOrbs(solids[0], counts[0], kMaxSolids); // the orbs float on the water of A
        mech.Update(player, layer, solids[0], orbCountA, solids[1], counts[1], kDt);
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
    CHECK(n == st::kStageStaticCount + static_cast<int>(st::kDecor.size()) + 4 + st::kLandmarkCount + 2 + 2 * st::kSealCount);
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
    World side; // the town wall beside the breach holds in B too: only the breach is a way
    side.layer = 1;
    side.PlaceFeet({2.4f, st::kTerraceY, -1.0f});
    side.Run(2.0f, Keys(1.0f), kNorth);
    CHECK(side.player.Feet().z < st::kGateZ - 0.25f - 0.3f);
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
    CHECK(o.player.Grounded() && Near(o.player.Feet().y, kFloorY, 0.02f) && st::RoomAt(o.player.Feet()) == Room("courtyard"));
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
    ok = ok && w.WalkPath({{st::kDoorX, kFloorY, 23.5f}}) && st::RoomAt(w.player.Feet()) == Room("square");
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

const Vec3 kSouth{0.0f, 0.0f, -1.0f};
const Vec3 kWest{-1.0f, 0.0f, 0.0f};
// the middle of the rims of the square: the walks round it stay on them (in A the pool is water)
constexpr float kWestRim = 0.5f * (st::kHubWest + st::kPoolWest) + 0.3f; // a little east of the middle: the arcade stands on it
constexpr float kEastRim = 0.5f * (st::kHubEast + st::kPoolEast);
constexpr float kNorthRim = 0.5f * (st::kPoolNorth + st::kHubNorth);

/// Plays the west wing from the hub to the seal: W1 (the orb onto the socket in B, the bridge), W2 (the other orb onto the
/// plate in A, through the door in B, the lever, the orb fetched), W3 (the orb let fall through the hole in B), W4 (the seal).
/// Each step is checked; returns true when all of them worked.
bool PlayWestWing(World& w)
{
    bool ok = true;
    const auto step = [&](bool good, const char* what) {
        if (!good)
        {
            std::printf("  (west wing: %s failed, at %.2f %.2f %.2f, layer %c)\n", what, static_cast<double>(w.player.Feet().x),
                        static_cast<double>(w.player.Feet().y), static_cast<double>(w.player.Feet().z), w.layer == 0 ? 'A' : 'B');
        }
        ok = ok && good;
        return good;
    };
    w.layer = 0;
    // ---- W1: the orb on the north floor, to the socket by the trench
    step(w.WalkPath({{kWestRim, kFloorY, 23.5f}, {kWestRim, kFloorY, 31.0f}, {-12.0f, kFloorY, 31.0f}, {-21.0f, kFloorY, 35.8f}}), "walk to the W1 orb");
    step(w.Use(kNorth) == dl::mech::Action::Took, "take the W1 orb");
    step(w.WalkPath({{-17.0f, kFloorY, 32.7f}}), "walk to the socket");
    {
        Vec3 spot{};
        int other = dl::mech::MechanismSet::kSpotNone;
        step(w.mech.SpotAhead(w.player, 0, kSouth, spot, &other) == dl::mech::MechanismSet::kSpotNone && other == st::kW1Socket,
             "in A the socket of B is not a place, and is reported as the other world");
    }
    step(w.Use(kSouth) == dl::mech::Action::Put, "put it down in A");
    step(!w.mech.Active(st::kW1Socket) && !w.mech.GatePresent(st::kW1Bridge), "in A nothing happens");
    step(w.Use(kSouth) == dl::mech::Action::Took, "take it again");
    step(w.Switch(1), "switch to B");
    step(w.Use(kSouth) == dl::mech::Action::Put && w.mech.GatePresent(st::kW1Bridge), "the orb on the socket in B lays the bridge");
    {
        World a = w; // the bridge is of B: in A the body falls into the trench
        a.Switch(0);
        a.PlaceFeet({-19.5f, kFloorY, 30.5f});
        a.WalkTo(-19.5f, 23.4f, 4.0f);
        step(a.Fallen() || a.player.Feet().y < kFloorY - 1.0f, "in A the bridge is not there");
    }
    step(w.WalkPath({{-19.5f, kFloorY, 30.5f}, {-19.5f, kFloorY, 23.4f}}) && w.At(-19.5f, kFloorY, 23.4f), "over the bridge in B");
    // ---- W2: the other orb onto the plate in A; through the door in B; the lever; back for the orb
    step(w.Switch(0), "switch to A");
    step(w.WalkPath({{-15.6f, kFloorY, 23.25f}}) && w.Use(kWest) == dl::mech::Action::Took, "take the W2 orb");
    step(w.WalkPath({{-21.9f, kFloorY, 23.25f}}) && w.Use(kWest) == dl::mech::Action::Put, "put it onto the plate");
    step(w.mech.Active(st::kW2Plate) && w.mech.GateOpen(st::kW2Door), "the plate opens the door");
    step(w.WalkPath({{-20.0f, kFloorY, 21.0f}}) && w.Switch(1), "to the ramp, switch to B");
    step(w.WalkPath({{-20.0f, kFloorY, 11.0f}}) && w.At(-20.0f, st::kWestLowY, 11.0f), "down the ramp, through the door in B");
    step(w.WalkPath({{-22.9f, kFloorY, 10.8f}}) && w.mech.Active(st::kW2Lever), "the lever");
    step(w.WalkPath({{-20.0f, kFloorY, 13.0f}, {-20.0f, kFloorY, 21.5f}, {-21.9f, kFloorY, 23.25f}}), "back up to the plate");
    step(w.Switch(0) && w.Use(kWest) == dl::mech::Action::Took, "take the orb off the plate in A");
    step(w.mech.GateOpen(st::kW2Door), "the lever keeps the door open");
    step(w.Switch(1) && w.mech.HeldOrb() == 1, "switch to B with the orb");
    step(w.WalkPath({{-20.0f, kFloorY, 21.0f}, {-20.0f, kFloorY, 10.5f}}) && w.At(-20.0f, st::kWestLowY, 10.5f), "down with the orb");
    // ---- W3: up to the mezzanine, the orb over the hole: in A it lies on the lid, in B it falls onto the socket
    step(w.WalkPath({{-15.0f, kFloorY, 10.5f}, {-15.0f, kFloorY, 4.0f}}) && w.At(-15.0f, st::kMezzY, 4.0f), "up to the mezzanine");
    step(w.Switch(0) && w.WalkPath({{-18.9f, kFloorY, 3.5f}}) && w.Use(kWest) == dl::mech::Action::Put, "put it over the hole in A");
    step(w.mech.OrbAt(1).resting && w.mech.OrbAt(1).centre.y > st::kMezzY && !w.mech.Active(st::kW3Socket), "in A it lies on the lid");
    step(w.Use(kWest) == dl::mech::Action::Took && w.Switch(1) && w.Use(kWest) == dl::mech::Action::Put, "again, in B");
    step(w.mech.OrbAt(1).centre.y < st::kWestLowY + 0.5f && w.mech.Active(st::kW3Socket) && !w.mech.GatePresent(st::kW3Bars),
         "in B it falls onto the socket and the bars open");
    // ---- W4: down, into the vault, the seal
    step(w.WalkPath({{-15.0f, kFloorY, 4.0f}, {-15.0f, kFloorY, 10.0f}, {-21.0f, kFloorY, 7.0f}, {-22.8f, kFloorY, 3.8f}}), "into the vault");
    step(w.seals.Taken(st::kWestSeal), "the seal");
    return ok;
}

void TestWestWing()
{
    bool ok = false;
    const World hub = ReachTheHub(ok);
    CHECK(ok);
    // W1: the trench is not crossed without the bridge, in either layer, even with a running jump
    for (int layer = 0; layer < 2; ++layer)
    {
        for (float runUp : {0.0f, 0.2f, 0.4f})
        {
            World t = hub;
            t.layer = layer;
            t.PlaceFeet({-19.5f, kFloorY, 31.8f});
            t.Run(runUp, Keys(1.0f), kSouth);
            t.Frame(Keys(1.0f, true), kSouth);
            bool across = false;
            for (int i = 0; i < 240 && !t.Fallen(); ++i)
            {
                t.Frame(Keys(1.0f), kSouth);
                across = across || (t.player.Grounded() && t.player.Feet().z < st::kTrenchSouth + 0.1f && t.player.Feet().y > kFloorY - 0.1f);
            }
            CHECK(!across);
        }
    }
    // W2: the door holds in A (the red panel) and in B while the plate is up
    for (int layer = 0; layer < 2; ++layer)
    {
        World t = hub;
        t.layer = layer;
        t.PlaceFeet({-20.0f, kFloorY, 21.0f});
        t.WalkTo(-20.0f, 10.5f, 6.0f);
        CHECK(t.player.Feet().z > st::kWestDoorNorth);
    }
    // W2: the body on the plate opens the door only for A, where the red panel holds; the orb on the plate opens it for B
    {
        World t = hub;
        t.layer = 0;
        t.PlaceFeet({-23.0f, kFloorY, 23.25f});
        CHECK(t.mech.Active(st::kW2Plate));
        t.layer = 1;
        t.Run(0.2f);
        CHECK(!t.mech.Active(st::kW2Plate));
    }
    // W2: without the lever, the orb cannot be carried through the door: taken off the plate, the door closes
    {
        World t = hub;
        t.layer = 0;
        t.PlaceFeet({-15.6f, kFloorY, 23.25f});
        CHECK(t.Use(kWest) == dl::mech::Action::Took);
        t.WalkPath({{-21.9f, kFloorY, 23.25f}});
        CHECK(t.Use(kWest) == dl::mech::Action::Put && t.mech.GateOpen(st::kW2Door));
        CHECK(t.Use(kWest) == dl::mech::Action::Took && !t.mech.GateOpen(st::kW2Door));
        CHECK(t.Switch(1));
        t.WalkPath({{-20.0f, kFloorY, 21.0f}});
        t.WalkTo(-20.0f, 10.5f, 6.0f);
        CHECK(t.player.Feet().z > st::kWestDoorNorth);
        // and the orb is not put down through the closed door
        t.PlaceFeet({-20.0f, st::kWestLowY + 0.3f, 12.95f});
        CHECK(t.Use(kSouth) == dl::mech::Action::Refused && t.mech.HeldOrb() == 1);
    }
    // W3: the body does not fall through the hole, in either layer; the mezzanine is out of reach from the lower floor
    for (int layer = 0; layer < 2; ++layer)
    {
        World t = hub;
        t.layer = layer;
        t.PlaceFeet({st::kHoleX, st::kMezzY, st::kHoleZ});
        t.Run(1.0f);
        CHECK(t.player.Feet().y > st::kMezzY - 0.5f);
        CHECK(t.JumpFrom({-17.0f, st::kWestLowY, 6.5f}, kSouth, 0.3f) < st::kMezzY - 1.0f);
    }
    // W3: the bars hold until the socket is worked, and the seal is out of reach through them
    {
        World t = hub;
        t.layer = 0;
        t.PlaceFeet({-23.0f, st::kWestLowY, 7.0f});
        t.Run(1.5f, Keys(1.0f), kSouth);
        CHECK(t.player.Feet().z > st::kMezzNorth + 0.4f && !t.seals.Taken(st::kWestSeal));
    }
    // the water of the trench: a body that falls in, in A, goes under it; in B the water has gone; an orb floats on it
    {
        World t = hub;
        t.layer = 0;
        t.PlaceFeet({-19.5f, kFloorY, 30.2f});
        bool under = false;
        for (int i = 0; i < 240 && !t.Fallen(); ++i)
        {
            t.Frame(Keys(1.0f), kSouth);
            under = under || st::Submerged(t.player.Feet(), 0);
        }
        CHECK(under);
        CHECK(!st::Submerged({-19.5f, kFloorY - 2.0f, 27.0f}, 1));
        World o = hub;
        o.layer = 0;
        o.PlaceFeet({-21.0f, kFloorY, 35.8f});
        CHECK(o.Use(kNorth) == dl::mech::Action::Took);
        o.WalkPath({{-21.0f, kFloorY, 30.1f}});
        CHECK(o.Use(kSouth) == dl::mech::Action::Put);
        o.Run(1.0f);
        const dl::mech::MechanismSet::Orb& orb = o.mech.OrbAt(0);
        CHECK(orb.resting && Near(orb.centre.y, st::kWaters[0].top + dl::mech::kOrbRadius, 0.03f));
        CHECK(o.Use(kSouth) == dl::mech::Action::Took); // and it is in reach from the edge
    }
    // R in the hall puts its orbs back
    {
        World t = hub;
        t.layer = 0;
        t.PlaceFeet({-21.0f, kFloorY, 35.8f});
        CHECK(t.Use(kNorth) == dl::mech::Action::Took);
        t.mech.ResetRoom(Room("west hall"));
        t.Run(0.5f);
        CHECK(t.mech.HeldOrb() < 0 && t.mech.OrbAt(0).resting && std::fabs(t.mech.OrbAt(0).centre.z - 37.0f) < 0.05f);
    }
}

void TestWings()
{
    bool ok = false;
    const World hub = ReachTheHub(ok);
    CHECK(ok);
    {
        // the west wing, played: W1, W2, W3, the seal, the shortcut (the ways round are in TestWestWing)
        World w = hub;
        CHECK(PlayWestWing(w));
        CHECK(w.seals.Taken(st::kWestSeal) && w.seals.ShortcutOpen());
        CHECK(w.WalkPath({{-21.0f, kFloorY, 7.0f}, {-17.0f, kFloorY, 11.6f}, {-11.0f, kFloorY, 11.6f}, {-11.0f, kFloorY, 23.5f},
                          {-7.0f, kFloorY, 23.5f}})); // round the foot of the ramp up to the mezzanine
        CHECK(w.At(-7.0f, kFloorY, 23.5f) && st::RoomAt(w.player.Feet()) == Room("square"));
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
        CHECK(w.WalkPath({{kEastRim, kFloorY, 23.5f}, {kEastRim, kFloorY, 31.0f}, {12.0f, kFloorY, 31.0f}, {20.0f, kFloorY, 31.0f},
                          {20.0f, kFloorY, 23.0f}, {20.0f, kFloorY, 8.0f}}));
        CHECK(w.At(20.0f, st::kEastHighY, 8.0f) && st::RoomAt(w.player.Feet()) == Room("east upper"));
        CHECK(w.WalkPath({{st::kCourtEast + 0.75f, kFloorY, 8.0f}, {st::kCourtEast + 0.75f, kFloorY, 20.0f}}));
        CHECK(w.seals.Taken(1) && w.At(st::kCourtEast + 0.75f, st::kEastHighY, 20.0f));
        CHECK(w.WalkPath({{st::kCourtEast + 0.75f, kFloorY, 24.0f}})); // off the north end: onto the gallery
        CHECK(w.At(st::kCourtEast + 0.75f, st::kGalleryY, 24.0f) && std::string_view(st::kRooms[st::RoomAt(w.player.Feet())].name) == "gallery");
        CHECK(w.WalkPath({{8.0f, kFloorY, 24.0f}, {8.0f, kFloorY, 33.0f}, {8.0f, kFloorY, 35.6f}})); // along it, and off its north end
        w.Run(0.5f);
        CHECK(w.At(8.0f, kFloorY, 35.6f) && st::RoomAt(w.player.Feet()) == Room("square")); // onto the east rim
        for (const Vec3& look : {kSouth, kNorth}) // the gallery is not reached again from the rim, from its end or from under it
        {
            World back = w;
            back.PlaceFeet({8.0f, kFloorY, look.z < 0.0f ? 35.6f : 29.0f});
            back.Run(0.3f, Keys(1.0f), look);
            back.Frame(Keys(1.0f, true), look);
            back.Run(1.2f, Keys(1.0f), look);
            CHECK(back.player.Feet().y < kFloorY + 0.5f);
        }
    }
    {
        // the north wing: up two ramps to the seal, along the ledge through the window of the hub wall, and off it
        World w = hub;
        w.layer = 0;
        CHECK(w.WalkPath({{kEastRim, kFloorY, 23.5f}, {kEastRim, kFloorY, kNorthRim}, {0.0f, kFloorY, kNorthRim}, {0.0f, kFloorY, 43.0f},
                          {7.0f, kFloorY, 45.0f}, {7.0f, kFloorY, 59.0f}, {2.0f, kFloorY, 59.0f}, {2.0f, kFloorY, 45.0f}}));
        CHECK(w.At(2.0f, st::kNorthHighY, 45.0f));
        CHECK(w.WalkPath({{-7.0f, kFloorY, 45.0f}}));
        CHECK(w.seals.Taken(2));
        CHECK(w.WalkPath({{-7.0f, kFloorY, 40.25f}})); // in the window
        CHECK(w.At(-7.0f, st::kNorthHighY, 40.25f));
        CHECK(w.WalkPath({{-7.0f, kFloorY, 34.0f}}));
        w.Run(1.0f); // the drop is 7 m
        CHECK(w.At(-7.0f, kFloorY, 34.0f) && st::RoomAt(w.player.Feet()) == Room("square"));
    }
}

void TestRing()
{
    bool ok = false;
    const World hub = ReachTheHub(ok);
    CHECK(ok);
    {
        // B: the hub, the west hall, the passage under the ledge, the north corridor, back into the hub
        World w = hub;
        w.layer = 1;
        CHECK(w.WalkPath({{kWestRim, kFloorY, 23.5f}, {kWestRim, kFloorY, 31.0f}, {-12.0f, kFloorY, 31.0f}, {-20.0f, kFloorY, 31.0f},
                          {-20.0f, kFloorY, 38.0f}, {-20.0f, kFloorY, 42.0f}}));
        CHECK(st::RoomAt(w.player.Feet()) == Room("west passage"));
        CHECK(w.WalkPath({{-3.0f, kFloorY, 42.5f}, {0.0f, kFloorY, 42.5f}, {0.0f, kFloorY, kNorthRim}}));
        CHECK(w.At(0.0f, kFloorY, kNorthRim) && st::RoomAt(w.player.Feet()) == Room("square"));
        // A: both breaches are shut
        World a = hub;
        a.layer = 0;
        a.PlaceFeet({-20.0f, kFloorY, 38.0f});
        a.Run(1.5f, Keys(1.0f), kNorth);
        CHECK(a.player.Feet().z < 40.0f - 0.3f);
        World e = hub;
        e.layer = 0;
        e.PlaceFeet({0.0f, kFloorY, 42.5f});
        e.Run(1.5f, Keys(1.0f), {-1.0f, 0.0f, 0.0f});
        CHECK(e.player.Feet().x > -1.0f + 0.3f);
    }
    {
        // A: the north hall, landing 1, the sky bridge, the lookout, the stairs down into the east hall
        World w = hub;
        w.layer = 0;
        CHECK(w.WalkPath({{kEastRim, kFloorY, 23.5f}, {kEastRim, kFloorY, kNorthRim}, {0.0f, kFloorY, kNorthRim}, {0.0f, kFloorY, 43.0f},
                          {7.0f, kFloorY, 45.0f}, {7.0f, kFloorY, 59.0f}, {10.5f, kFloorY, 59.0f}}));
        CHECK(w.At(10.5f, st::kBridgeY, 59.0f) && st::RoomAt(w.player.Feet()) == Room("sky bridge"));
        World b = w; // B: the middle of the bridge is gone
        CHECK(b.Switch(1));
        float lowest = b.player.Feet().y;
        for (int i = 0; i < 360 && !b.player.JustRespawned(); ++i)
        {
            b.Frame(Keys(1.0f), kEast);
            lowest = b.player.Feet().y < lowest ? b.player.Feet().y : lowest;
        }
        CHECK(lowest < st::kBridgeY - 1.0f);
        for (float runUp : {0.15f, 0.25f, 0.3f}) // B: a running jump from the edge does not clear the gap
        {
            World j = w;
            CHECK(j.Switch(1));
            CHECK(j.JumpFrom({st::kBridgeGapX0 - 0.2f - 5.5f * runUp, st::kBridgeY, 59.0f}, kEast, runUp) < st::kBridgeY - 1.0f);
        }
        CHECK(w.WalkPath({{18.0f, kFloorY, 59.0f}, {20.0f, kFloorY, 58.5f}}));
        CHECK(w.At(20.0f, st::kBridgeY, 58.5f) && st::RoomAt(w.player.Feet()) == Room("east stairs"));
        CHECK(w.WalkPath({{20.0f, kFloorY, 55.0f}, {20.0f, kFloorY, 41.0f}, {20.0f, kFloorY, 38.0f}}));
        CHECK(w.At(20.0f, kFloorY, 38.0f) && st::RoomAt(w.player.Feet()) == Room("east hall"));
    }
    {
        // the lift of the north hall: shut until the north seal, then up to the landing of the seal
        World w = hub;
        w.layer = 0;
        w.PlaceFeet({-8.0f, kFloorY, 47.0f});
        w.Run(2.0f);
        CHECK(w.lifts[0].AtBottom() && w.At(-8.0f, kFloorY, 47.0f));
        w.TakeAllSeals();
        int air = 0;
        for (int i = 0; i < 900 && !w.lifts[0].AtTop(); ++i)
        {
            w.Frame({}, kNorth);
            air += (i > 60 && !w.player.Grounded()) ? 1 : 0;
        }
        CHECK(w.lifts[0].AtTop() && air == 0);
        CHECK(w.WalkPath({{-8.0f, kFloorY, 45.0f}, {-4.0f, kFloorY, 45.0f}}));
        CHECK(w.At(-4.0f, st::kNorthHighY, 45.0f));
    }
}

void TestSquare()
{
    bool ok = false;
    const World hub = ReachTheHub(ok);
    CHECK(ok);
    {
        // A: the pool is water. A body that walks off the rim goes under it (main.cpp then puts it back at the entry)
        World a = hub;
        a.layer = 0;
        a.PlaceFeet({kEastRim, kFloorY, 30.0f});
        bool under = false;
        for (int i = 0; i < 240 && !under; ++i)
        {
            a.Frame(Keys(1.0f), kWest);
            under = st::Submerged(a.player.Feet(), 0);
        }
        CHECK(under);
        CHECK(!st::Submerged({0.0f, st::kSquareLowY, 36.0f}, 1)); // B: the pool is dry
        CHECK(!st::Submerged({-2.5f, kFloorY, 26.0f}, 0));        // the causeway is over the water
    }
    {
        // B: down the south-east stairs onto the floor of the pool, round the cathedral to the north-west stairs and up
        World b = hub;
        b.layer = 1;
        const float sx = st::kPoolEast - 0.5f * st::kStairWidth;
        const float nx = st::kPoolWest + 0.5f * st::kStairWidth;
        CHECK(b.WalkPath({{sx, kFloorY, 23.5f}, {sx, kFloorY, st::kPoolSouth + st::kStairRun + 0.5f}}));
        CHECK(b.At(sx, st::kSquareLowY, st::kPoolSouth + st::kStairRun + 0.5f) && st::RoomAt(b.player.Feet()) == Room("square"));
        CHECK(b.WalkPath({{5.5f, kFloorY, 36.3f}, {-3.8f, kFloorY, 36.3f}, {-4.5f, kFloorY, 34.0f}, {nx, kFloorY, 34.0f}}));
        CHECK(b.At(nx, st::kSquareLowY, 34.0f)); // round the cathedral, between it and the stairs
        CHECK(b.WalkPath({{nx, kFloorY, kNorthRim}}) && b.At(nx, kFloorY, kNorthRim));
        // a jump climbs out of the pool anywhere: nobody is trapped in it in B
        World j = hub;
        j.layer = 1;
        CHECK(j.JumpFrom({5.6f, st::kSquareLowY, 31.0f}, kEast, 0.2f) > kFloorY - 0.05f);
        // the stairs are drawn as steps of at most one rise and walked as their slope
        CHECK(st::kFloorY - st::kSquareLowY < Player::Settings{}.jumpHeight - 0.3f);
    }
    {
        // the causeway: from the south rim to the portal, in both layers, without a jump
        for (int layer = 0; layer < 2; ++layer)
        {
            World c = hub;
            c.layer = layer;
            c.TakeAllSeals();
            CHECK(c.WalkPath({{-2.5f, kFloorY, 23.5f}, {-2.5f, kFloorY, 28.5f}}));
            CHECK(c.player.Grounded() && st::RoomAt(c.player.Feet()) == Room("cathedral"));
        }
    }
}

void TestTower()
{
    bool ok = false;
    World w = ReachTheHub(ok);
    CHECK(ok);
    w.layer = 0;
    CHECK(w.WalkPath({{-2.5f, kFloorY, 23.5f}, {-2.5f, kFloorY, 25.5f}})); // along the causeway; shut: the door holds before the seals
    w.Run(2.0f, Keys(1.0f), kNorth);
    CHECK(w.player.Feet().z < st::kTowerSouth);
    w.TakeAllSeals(); // the walks to them are in TestWings
    CHECK(w.seals.TowerOpen());
    CHECK(w.WalkPath({{-2.5f, kFloorY, 28.2f}}));
    CHECK(st::RoomAt(w.player.Feet()) == Room("cathedral"));
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
    const st::Room& court = st::kRooms[Room("courtyard")];
    CHECK(court.puzzle);
    const int gx = static_cast<int>(std::lround(court.entry.x)) - st::kCellX0;
    const int gz = static_cast<int>(std::lround(court.entry.z)) - st::kCellZ0;
    CHECK(g.Inside(gx, gz) && cells.kind[static_cast<std::size_t>(g.Index(gx, gz))] == 0); // the block is never there
    const int last = Room("east hall");
    CHECK(st::CurrentRoom({0.0f, kFloorY, 30.0f}, false, last) == last);       // in the air: the last room
    CHECK(st::CurrentRoom({0.0f, kFloorY, 25.0f}, true, last) == Room("square")); // standing in the hub
    CHECK(st::CurrentRoom({50.0f, kFloorY, 25.0f}, true, last) == last);      // standing in no room: the last one
}

} // namespace

int main()
{
    TestTables();
    TestPrologue();
    TestCourtyard();
    TestWings();
    TestWestWing();
    TestRing();
    TestSquare();
    TestTower();
    TestNoWayRound();
    TestRooms();
    std::printf("%d checks passed, %d failed\n", g_passed, g_failed);
    return g_failed == 0 ? 0 : 1;
}




