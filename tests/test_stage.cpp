// test_stage.cpp - walks the whole map (src/scene/Stage.h) with the real walking body (Player.h), the real puzzle
// (Puzzle.h) and the seals, no window, no GPU: every room reached, every shortcut taken, every closed door holding, and the
// jumps that would skip a part tried.
//   g++ -std=c++20 -O2 -Wall -Wextra -I src tests/test_stage.cpp -o test_stage
//   MSVC (Developer PowerShell):  .\tests\run_tests.ps1 -Only stage
// Header-only: there is no .cpp to add to the project. What is checked:
//   0. the tables: the walls of the courtyard are exactly the rows of scene/Courtyard.h; every ramp is at most 27 degrees;
//      the heights against the body (every wall out of reach from every place next to it); the drawn and solid tables fit
//   1. the prologue: the quay of the harbour and its stair (TestHarbour), the guardhouse, the gate (A stops, B passes), the heap of B, the ditch (one layer never crosses, B
//      never climbs round the heap, the stairs lead back); the gap (neither layer alone crosses it, even with the running jump; from
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
//   7. the east canal: crossed only on the raised crate; the wheel turns only in A; the crate carries a body; B walks the bed
//   8. the anchor lamps, the fall line of every room, and the water: every water fed downhill from the aqueduct, broken in B
//   9. the undercroft (E2): the weir, the drift, the breach of B over the crate, the lever and the bars; the ways round
//  10. the prism (N1): neither world crosses the gap without it, A walks over with it
//  11. orbs: an orb at rest out of reach of its world goes back to its start, one in reach stays
//  12. the cellar: B from the canal bed to the grate, which holds until the lever in the lower room; then the way through;
//      A flooded and closed by its sluice panels
//  13. N2: no way onto the ledge before the prism (also with a switch in the air); after it, B by the step to the light
//  14. the end: the master sluice drains every water of A, the canal and its crate
//  15. the lock walk (E3): walked in both layers up its four terraces; its walls hold; each lock water in A and a dry room in
//      B with a stair back up; the light of the middle lock taken in B only
//   6. the rooms: every entry is in its own room and is a place to stand in both layers; the courtyard entry is a cell the
//      block can never be in
// The walks between the points of a path are played frame by frame (the body steers to each point). Returns 1 when a check
// fails.

#include "scene/Anchors.h"
#include "scene/CityParts.h"
#include "scene/CityState.h"
#include "scene/OrbReturn.h"
#include "scene/Recovery.h"
#include "scene/Stage.h"
#include "scene/StageLights.h"
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

constexpr int kMaxSolids = dl::puzzle::Puzzle::kMaxSolids + 4 + dl::mech::kMaxGates + st::kLiftCount + 1 + 8 + 8;

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
    dl::canal::Canal canal;
    st::LightSet lights; ///< the hidden lights taken: their memory fragments are solid in B (scene/CityState.h)
    Player player;
    int layer = 0;
    float spin = 0.0f;
    int orbsReturned = 0; ///< how many times an orb came back to its start (scene/OrbReturn.h, or a fall)
    SceneObject solids[2][kMaxSolids] = {};
    int counts[2] = {0, 0};

    World()
    {
        canal.Init(st::MakeCanal());
        if (!puzzle.Init(st::MakeStage()) || !mech.Init(st::CityParts()))
        {
            ++g_failed;
            std::printf("FAIL: the stage does not Init\n");
        }
        // the walks of these tests start at the guardhouse (beat 1); the harbour before it is walked by TestHarbour
        player.SetSpawn(st::kRooms[st::RoomIndex("guardhouse")].entry);
        player.Respawn();
    }

    World(const World& o)
        : puzzle(o.puzzle), seals(o.seals), mech(o.mech), lifts(o.lifts), canal(o.canal), lights(o.lights), player(o.player), layer(o.layer), spin(o.spin),
          orbsReturned(o.orbsReturned)
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
            count = canal.AddSolids(l, solids[l], count, kMaxSolids);
            count = st::AddFragments(lights, false, solids[l], count, kMaxSolids);
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

    void TakeAllLights()
    {
        for (const st::HiddenLight& l : st::kHiddenLights)
        {
            lights.Collect(Touch{l.centre});
        }
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
        canal.Update(player, layer, kDt);
        BuildSolids();
        player.SetScene(solids[layer], counts[layer]);
        spin += kDt;
        player.Update(input, look, layer, spin, kDt);
        puzzle.Update(player, kDt);
        BuildSolids();
        const int bodyA = counts[0];
        const int orbCountA = canal.AddWaterForOrbs(solids[0], st::AddWaterForOrbs(solids[0], bodyA, kMaxSolids), kMaxSolids); // orbs float
        mech.Update(player, layer, solids[0], orbCountA, solids[1], counts[1], kDt);
        st::ReturnStrandedOrbs(mech, solids[0], bodyA, solids[1], counts[1], canal);
        for (int i = 0; i < mech.OrbCount(); ++i)
        {
            orbsReturned += mech.ConsumeReturned(i) ? 1 : 0;
        }
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
    /// In the water of A (the pool, the trench, the canal): the program would bring the body back.
    bool InWater() const
    {
        return st::Submerged(player.Feet(), layer) || canal.Submerged(player.Feet(), layer);
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
        BuildSolids(); // the whole solid table of the target layer, as the program checks it
        const bool fits = dl::puzzle::Puzzle::FitsIn(player, to, spin, solids[to], counts[to]);
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

    /// RunJump along any flat direction: runs along `dir` until the feet pass `jumpAt` along it, jumps, switches to `switchTo`
    /// `switchAfter` seconds into the jump, keeps running until the body lands or falls. Returns true when it landed.
    bool RunJumpAlong(const Vec3& dir, float jumpAt, float switchAfter, int switchTo)
    {
        const auto along = [&]() { return player.Feet().x * dir.x + player.Feet().z * dir.z; };
        int guard = 0;
        while (along() < jumpAt && player.Grounded() && guard++ < 2000)
        {
            Frame(Keys(1.0f), dir);
        }
        if (!player.Grounded())
        {
            return false;
        }
        Frame(Keys(1.0f, true), dir);
        float air = 0.0f;
        bool switched = false;
        for (int i = 0; i < 900; ++i)
        {
            if (!switched && air >= switchAfter)
            {
                switched = Switch(switchTo);
            }
            Frame(Keys(1.0f), dir);
            air += kDt;
            if (player.Grounded() && air > 0.1f)
            {
                Run(0.2f);
                return !Fallen();
            }
            if (Fallen() || player.Feet().y < -20.0f)
            {
                return false;
            }
        }
        return false;
    }

    /// Walks along `dir` (flat, unit) until the feet pass `jumpAt` along it, jumps, keeps walking until they pass `stopAt`, then
    /// lets go (the air brakes), and returns true when the body landed on solid ground (not fallen, not in water).
    bool JumpAlong(const Vec3& dir, float jumpAt, float stopAt)
    {
        const auto along = [&]() { return player.Feet().x * dir.x + player.Feet().z * dir.z; };
        int guard = 0;
        while (along() < jumpAt && player.Grounded() && guard++ < 2000)
        {
            Frame(Keys(1.0f), dir);
        }
        if (!player.Grounded())
        {
            return false;
        }
        Frame(Keys(1.0f, true), dir);
        float air = 0.0f;
        for (int i = 0; i < 900; ++i)
        {
            Frame(Keys(along() < stopAt ? 1.0f : 0.0f), dir);
            air += kDt;
            if (InWater() || Fallen())
            {
                return false;
            }
            if (player.Grounded() && air > 0.1f)
            {
                Run(0.3f);
                return !Fallen() && !InWater() && player.Grounded();
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
    CHECK(st::kRubbleTop > st::kTerraceY + jump + margin);                               // the heap of beat 3, from the terrace
    CHECK(st::kGuardWallTop > st::kTerraceY + jump + margin && st::kArchTop < st::kGuardWallTop); // the guardhouse
    CHECK(st::kTerraceY - kFloorY > jump + margin);                                      // the ditch, from its floor
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
    CHECK(n == st::kStageStaticCount + static_cast<int>(st::kDecor.size()) + 4 + dl::puzzle::Puzzle::kCrateBands + st::kLandmarkCount + 2 + 2 * st::kSealCount);
    CHECK(n <= st::kMaxDrawObjects);
}

/// The height of the stairs of the ditch at `z` (their top at kDitchStairsTop, their foot at kDitchEdge).
float DitchStairsY(float z)
{
    return kFloorY + (st::kTerraceY - kFloorY) * (z - st::kDitchEdge) / (st::kDitchStairsTop - st::kDitchEdge);
}

void TestPrologue()
{
    const float T = st::kTerraceY;
    World a; // beats 0 and 2: out of the guardhouse through its arch; in A the gate stops the body
    CHECK(std::string_view(st::kRooms[st::RoomAt(a.player.Feet())].name) == "guardhouse");
    a.Run(4.0f, Keys(1.0f), kNorth);
    CHECK(a.player.Feet().z > st::kGuardNorth && a.player.Feet().z < st::kGateZ - 0.25f - 0.3f && Near(a.player.Feet().y, T, 0.02f));
    World b; // in B the gate has fallen; the heap of beat 3 stops the body beyond it
    b.layer = 1;
    b.PlaceFeet({0.0f, T, st::kGateZ - 2.0f});
    b.Run(2.0f, Keys(1.0f), kNorth);
    b.Run(0.3f);
    CHECK(b.player.Feet().z > st::kGateZ + 0.5f && b.player.Feet().z < st::kRubbleSouth - 0.3f && Near(b.player.Feet().y, T, 0.02f));
    World c; // beat 3 the other way round: in A the way is clear
    c.PlaceFeet({0.0f, T, st::kGateZ + 1.0f});
    c.Run(1.15f, Keys(1.0f), kNorth);
    c.Run(0.3f);
    CHECK(c.player.Feet().z > st::kRubbleNorth + 0.5f && c.player.Feet().z < st::kDitchEdge && Near(c.player.Feet().y, T, 0.02f));
    World side; // the town wall beside the breach holds in B too: only the breach is a way
    side.layer = 1;
    side.PlaceFeet({2.4f, T, st::kGateZ - 3.0f});
    side.Run(2.0f, Keys(1.0f), kNorth);
    CHECK(side.player.Feet().z < st::kGateZ - 0.25f - 0.3f);
    // B round the heap: from the stairs of the ditch, no jump lands on the terrace beyond the heap (the wall between holds)
    for (float z : {-9.6f, -8.8f, -8.0f, -7.2f, -6.4f})
    {
        World r;
        r.layer = 1;
        const float y = r.JumpFrom({4.5f, DitchStairsY(z) + 0.02f, z}, {-1.0f, 0.0f, 0.0f}, 0.15f);
        CHECK(!(y > T - 0.1f && r.player.Feet().x < 3.0f && r.player.Feet().z > st::kRubbleNorth));
    }
    // beat 4, the ditch: one layer alone never crosses it, two jumps in a row, from several jump points
    for (int layer = 0; layer < 2; ++layer)
    {
        for (float at : {-7.0f, -6.6f, -6.2f, -5.9f, -5.7f, -5.5f})
        {
            World w;
            w.layer = layer;
            w.PlaceFeet({0.0f, T, -7.6f});
            bool beyond = false;
            if (w.RunJump(at))
            {
                beyond = w.player.Feet().z > st::kDitchStripFar && Near(w.player.Feet().y, T, 0.05f);
                const bool onBeam = layer == 1 && Near(w.player.Feet().y, T, 0.05f) && w.player.Feet().z < st::kDitchStoneFar + 0.4f;
                if (onBeam && w.RunJump(w.player.Feet().z + 0.05f))
                {
                    beyond = w.player.Feet().z > st::kDitchStripFar && Near(w.player.Feet().y, T, 0.05f);
                }
            }
            CHECK(!beyond);
        }
    }
    int ditchWays = 0; // the way: B, onto the beam, then on with a switch to A in the air onto the planks
    int ditchTried = 0;
    for (float at : {-6.4f, -6.0f, -5.7f})
    {
        World w;
        w.layer = 1;
        w.PlaceFeet({0.0f, T, -7.6f});
        if (!w.RunJump(at) || !Near(w.player.Feet().y, T, 0.05f) || w.player.Feet().z > st::kDitchStoneFar + 0.4f)
        {
            continue;
        }
        for (float after : {0.10f, 0.20f, 0.30f, 0.40f})
        {
            World t = w;
            ++ditchTried;
            if (t.RunJump(t.player.Feet().z + 0.05f, after, 0) && t.layer == 0 && t.player.Feet().z > st::kDitchStripNear &&
                Near(t.player.Feet().y, T, 0.02f))
            {
                ++ditchWays;
            }
        }
    }
    std::printf("  ditch: %d of %d tried timings cross it\n", ditchWays, ditchTried);
    CHECK(ditchTried >= 8 && ditchWays * 4 >= ditchTried * 3);
    World miss; // a miss costs a walk: from the floor of the ditch, the stairs lead back up to the terrace
    miss.PlaceFeet({0.0f, kFloorY, -3.0f});
    CHECK(miss.player.Grounded() && st::RoomAt(miss.player.Feet()) == Room("ditch"));
    CHECK(miss.WalkPath({{4.5f, kFloorY, -4.9f}, {4.5f, kFloorY, -11.2f}}) && miss.At(4.5f, T, -11.2f, 0.4f));
    CHECK(st::RoomAt(miss.player.Feet()) == Room("terrace"));
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
        // the east wing: the lock wheel, the crate across the canal, the quay, up to the top of the courtyard wall, the seal,
        // the drop into the hub
        World w = hub;
        w.layer = 0;
        CHECK(w.WalkPath({{kEastRim, kFloorY, 23.5f}, {kEastRim, kFloorY, 31.0f}, {12.0f, kFloorY, 31.0f}, {16.4f, kFloorY, 33.1f}}));
        CHECK(w.canal.Open());
        w.Run(3.0f);
        CHECK(w.canal.High());
        CHECK(w.WalkPath({{20.0f, kFloorY, 31.0f}}) && w.JumpAlong(kSouth, -29.8f, -27.3f) && w.At(20.0f, kFloorY, 27.2f, 0.7f));
        CHECK(w.JumpAlong(kSouth, -26.8f, -23.6f) && st::RoomAt(w.player.Feet()) == Room("east quay"));
        CHECK(w.WalkPath({{20.0f, kFloorY, 23.0f}, {20.0f, kFloorY, 8.0f}}));
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
        CHECK(w.WalkPath({{2.6f, kFloorY, 45.0f}}) && w.mech.Active(st::kNorthPrism)); // N1: the prism
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
        CHECK(w.WalkPath({{-8.0f, kFloorY, 45.0f}, {-6.6f, kFloorY, 45.0f}}));
        CHECK(w.At(-6.6f, st::kNorthHighY, 45.0f));
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
        CHECK(st::kFloorY - st::kSquareLowY < Player::Settings{}.jumpHeight - 0.3f);
    }
    for (int layer = 0; layer < 2; ++layer) // the causeway: from the south rim to the portal, in both layers, without a jump
    {
        World c = hub;
        c.layer = layer;
        c.TakeAllSeals();
        CHECK(c.WalkPath({{-2.5f, kFloorY, 23.5f}, {-2.5f, kFloorY, 28.5f}}));
        CHECK(c.player.Grounded() && st::RoomAt(c.player.Feet()) == Room("cathedral"));
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


/// The east district played to the quay in A (the lock wheel, the crate across the canal), as in TestWings.
World ToTheQuay(const World& hub)
{
    World w = hub;
    w.layer = 0;
    CHECK(w.WalkPath({{kEastRim, kFloorY, 23.5f}, {kEastRim, kFloorY, 31.0f}, {12.0f, kFloorY, 31.0f}, {16.4f, kFloorY, 33.1f}}));
    w.Run(3.0f);
    CHECK(w.canal.High());
    CHECK(w.WalkPath({{20.0f, kFloorY, 31.0f}}) && w.JumpAlong(kSouth, -29.8f, -27.3f));
    CHECK(w.JumpAlong(kSouth, -26.8f, -23.6f) && st::RoomAt(w.player.Feet()) == Room("east quay"));
    return w;
}

/// A jump that starts along `first` and turns to `second` after `turnAfter` seconds in the air (a player steering round a
/// parapet): returns true when it ends in the undercroft.
bool SteeredJumpReachesUndercroft(const World& base, const Vec3& feet, const Vec3& first, const Vec3& second, float turnAfter, int layer)
{
    World w = base;
    w.layer = layer;
    w.PlaceFeet(feet);
    w.Run(0.25f, Keys(1.0f), first);
    w.Frame(Keys(1.0f, true), first);
    const int undercroft = Room("undercroft");
    float air = 0.0f;
    for (int i = 0; i < 300 && !w.Fallen() && !w.InWater(); ++i)
    {
        w.Frame(Keys(1.0f), air < turnAfter ? first : second);
        air += kDt;
        if (w.player.Grounded() && air > 0.1f)
        {
            break;
        }
    }
    return w.player.Grounded() && st::RoomAt(w.player.Feet()) == undercroft;
}

void TestUndercroft()
{
    bool ok = false;
    const World hub = ReachTheHub(ok);
    CHECK(ok);
    const dl::canal::Desc d = st::MakeCanal();
    const Vec3 W{-1.0f, 0.0f, 0.0f};
    // the numbers: the breach is out of reach from the bed; from the crate of B on the bed it is a step
    CHECK(st::kUndercroftY - d.bed >= 2.3f && st::kUndercroftY - (d.bed + d.crateSize) <= 1.3f + 1e-4f);
    CHECK(st::kBreachTop - st::kUndercroftY > Player::Settings{}.eyeHeight + 0.35f + 0.3f);
    // E2, played: the weir on the quay, the crate drifts west under the breach; in B down to the bed, onto the crate, into the breach,
    // through the undercroft (the lever, the light), out through the bars onto the east rim
    World w = ToTheQuay(hub);
    CHECK(!w.canal.WeirOpen() && Near(w.canal.CrateX(), d.crateX, 1e-5f));
    {
        World b = w; // the weir is machinery of A: in B it does not turn
        CHECK(b.Switch(1) && b.WalkPath({{16.4f, kFloorY, 23.2f}}));
        b.Run(1.0f);
        CHECK(!b.canal.WeirOpen() && Near(b.canal.CrateX(), d.crateX, 1e-5f));
    }
    CHECK(w.WalkPath({{16.4f, kFloorY, 23.2f}}) && w.canal.WeirOpen());
    w.Run(4.0f);
    CHECK(w.canal.Drifted() && Near(w.canal.CrateX(), st::kDriftX, 1e-5f));
    {
        World a = w; // in A the crate is a step at the west end too, but the iron panel closes the breach
        a.PlaceFeet({16.4f, d.highTop + d.draft, 27.2f}); // (on the drifted crate of A)
        CHECK(a.player.Grounded() && Near(a.player.Feet().y, d.highTop + d.draft, 0.05f));
        a.JumpAlong(W, -16.5f, -13.0f);
        CHECK(a.player.Feet().x > 15.0f && st::RoomAt(a.player.Feet()) != Room("undercroft"));
    }
    CHECK(w.WalkPath({{19.6f, kFloorY, 24.3f}}) && w.Switch(1)); // B: off the quay (east of its parapet) into the dry canal
    CHECK(w.JumpAlong(kNorth, 24.4f, 26.5f) && st::RoomAt(w.player.Feet()) == Room("canal bed"));
    CHECK(w.WalkPath({{17.6f, kFloorY, 27.2f}}) && w.JumpAlong(W, -17.3f, -16.0f) && Near(w.player.Feet().y, d.bed + d.crateSize, 0.05f));
    CHECK(w.WalkPath({{16.45f, kFloorY, 27.2f}}) && w.JumpAlong(W, -16.5f, -14.0f) && st::RoomAt(w.player.Feet()) == Room("undercroft"));
    CHECK(!w.mech.Active(st::kUndercroftLever) && w.mech.GatePresent(st::kUndercroftBars));
    CHECK(w.WalkPath({{12.5f, kFloorY, 27.2f}, {10.0f, kFloorY, 27.2f}}) && w.mech.Active(st::kUndercroftLever));
    CHECK(!w.mech.GatePresent(st::kUndercroftBars) && !w.mech.GatePresent(st::kUndercroftBars + 3));
    CHECK(w.WalkPath({{7.8f, kFloorY, 27.2f}}) && st::RoomAt(w.player.Feet()) == Room("square"));
    {
        World s = w; // the shortcut stays open, in both layers
        s.layer = 0;
        CHECK(s.WalkPath({{11.0f, kFloorY, 27.2f}}) && st::RoomAt(s.player.Feet()) == Room("undercroft"));
    }
    // R in the hall puts back the water, the weir and the crate
    w.canal.Reset();
    CHECK(!w.canal.WeirOpen() && Near(w.canal.CrateX(), d.crateX, 1e-5f));
    // the bars hold from the square before the lever, in both layers; the light and the lever are out of reach through them
    for (int layer = 0; layer < 2; ++layer)
    {
        World b = hub;
        b.layer = layer;
        b.PlaceFeet({kEastRim, kFloorY, 27.2f});
        b.Run(2.0f, Keys(1.0f), kEast);
        CHECK(b.player.Feet().x < st::kHubEast - 0.3f && !b.mech.Active(st::kUndercroftLever));
    }
    // without the drift: from the bed no jump reaches the breach (the crate of B lies in the middle of the canal)
    {
        World bed = hub;
        bed.layer = 1;
        for (float z : {26.8f, 27.2f, 27.6f})
        {
            CHECK(bed.JumpFrom({15.5f, d.bed, z}, W, 0.0f) < st::kUndercroftY - 1.0f);
            CHECK(bed.JumpFrom({17.4f, d.bed, z}, W, 0.25f) < st::kUndercroftY - 1.0f);
        }
    }
    // the ways round: from the hall floor and from the quay, a jump that turns in the air does not end in the breach
    World quay = ToTheQuay(hub);
    for (int layer = 0; layer < 2; ++layer)
    {
        for (float x : {15.4f, 16.2f, 17.0f, 17.8f, 18.8f, 19.4f, 20.2f})
        {
            for (float turn : {0.1f, 0.2f, 0.3f})
            {
                CHECK(!SteeredJumpReachesUndercroft(hub, {x, kFloorY, 30.3f}, kSouth, W, turn, layer));
                CHECK(!SteeredJumpReachesUndercroft(quay, {x, st::kQuayY, 24.3f}, kNorth, W, turn, layer));
            }
        }
    }
}

void TestPrism()
{
    bool ok = false;
    World hub = ReachTheHub(ok);
    CHECK(ok);
    const Vec3 W{-1.0f, 0.0f, 0.0f};
    const float y = st::kNorthHighY;
    // the gap of N1 is wider than a running jump; the portcullis is out of reach over the landing and reaches past it
    CHECK(st::kPrismGapX1 - st::kPrismGapX0 > 3.6f + 1.0f && st::kPortcullisZ0 < 44.0f - 2.0f && st::kPortcullisZ1 > 46.0f + 2.0f);
    // without the prism: in A the portcullis stops the walk over the bridge; in B there is no bridge
    {
        World a = hub;
        a.layer = 0;
        a.PlaceFeet({-0.5f, y, 45.0f});
        a.Run(2.5f, Keys(1.0f), W);
        CHECK(a.player.Feet().x > st::kPortcullisX1 + 0.3f && Near(a.player.Feet().y, y, 0.05f));
        World b = hub;
        b.layer = 1;
        b.PlaceFeet({-0.5f, y, 45.0f});
        b.Run(1.5f, Keys(1.0f), W);
        CHECK(b.player.Feet().y < y - 1.0f);
    }
    // nor a running jump in B with a switch to A in the air: the bridge beyond the portcullis is out of reach
    for (float after : {0.15f, 0.3f, 0.45f, 0.6f})
    {
        for (float z : {44.4f, 45.0f, 45.6f})
        {
            World j = hub;
            j.layer = 1;
            j.PlaceFeet({2.0f, y, z});
            CHECK(!(j.RunJumpAlong(W, -(st::kPrismGapX1 + 0.1f), after, 0) && j.player.Feet().x < st::kPortcullisX0 && Near(j.player.Feet().y, y, 0.05f)));
        }
    }
    // the prism: stepped on (in B as well), the portcullis is in B; then A walks over the bridge to the seal
    World w = hub;
    w.layer = 1;
    w.PlaceFeet({2.6f, y, 45.0f});
    CHECK(w.mech.Active(st::kNorthPrism) && w.mech.GateBox(st::kPortcullis).layerMask == kLayerB);
    CHECK(w.Switch(0));
    CHECK(w.WalkPath({{-7.5f, kFloorY, 45.0f}}) && w.At(-7.5f, y, 45.0f) && w.seals.Taken(2));
    World back = w; // and back over the bridge
    CHECK(back.WalkPath({{1.5f, kFloorY, 45.0f}}) && back.At(1.5f, y, 45.0f));
    // a switch into a gate is refused like a switch into a wall (the guard asks the whole solid table): standing on the bridge
    // of A where the portcullis now is in B, the switch to B does not happen
    World g = w;
    g.PlaceFeet({0.5f * (st::kPortcullisX0 + st::kPortcullisX1), y, 45.0f});
    CHECK(g.layer == 0 && !g.Switch(1) && g.layer == 0);
}

void TestOrbReturn()
{
    bool ok = false;
    const World hub = ReachTheHub(ok);
    CHECK(ok);
    const Vec3 start = hub.mech.StartOf(0);
    {
        // the orb of W1, carried to the east hall and put down over the canal in A: it floats on the low water, out of reach
        // of every place to stand, and goes back to its start
        World w = hub;
        w.layer = 0;
        w.PlaceFeet({-21.0f, kFloorY, 35.8f});
        CHECK(w.Use(kNorth) == dl::mech::Action::Took);
        w.PlaceFeet({21.0f, kFloorY, 30.05f});
        const int before = w.orbsReturned;
        CHECK(w.Use(kSouth) == dl::mech::Action::Put);
        w.Run(1.5f);
        const dl::mech::MechanismSet::Orb& o = w.mech.OrbAt(0);
        CHECK(w.orbsReturned == before + 1 && o.resting && Near(o.centre.x, start.x, 1e-3f) && Near(o.centre.z, start.z, 1e-3f));
    }
    {
        // in B the canal is dry: the orb falls onto the bed, which is walked; it stays
        World w = hub;
        w.layer = 0;
        w.PlaceFeet({-21.0f, kFloorY, 35.8f});
        CHECK(w.Use(kNorth) == dl::mech::Action::Took);
        CHECK(w.Switch(1));
        w.PlaceFeet({21.0f, kFloorY, 30.05f});
        const int before = w.orbsReturned;
        CHECK(w.Use(kSouth) == dl::mech::Action::Put);
        w.Run(1.5f);
        CHECK(w.orbsReturned == before && w.mech.OrbAt(0).resting && Near(w.mech.OrbAt(0).centre.y, st::kCanalBed + dl::mech::kOrbRadius, 0.03f));
    }
    {
        // high water: the orb floats beside the edge of the floor and is in reach: it stays
        World w = hub;
        w.layer = 0;
        w.PlaceFeet({16.4f, kFloorY, 33.1f});
        w.Run(3.0f);
        CHECK(w.canal.High());
        w.PlaceFeet({-21.0f, kFloorY, 35.8f});
        CHECK(w.Use(kNorth) == dl::mech::Action::Took);
        w.PlaceFeet({21.0f, kFloorY, 30.05f});
        const int before = w.orbsReturned;
        CHECK(w.Use(kSouth) == dl::mech::Action::Put);
        w.Run(1.0f);
        CHECK(w.orbsReturned == before && w.mech.OrbAt(0).resting && w.mech.OrbAt(0).centre.y > st::MakeCanal().highTop);
        CHECK(w.Use(kSouth) == dl::mech::Action::Took);
    }
    // the rule itself: an orb on a floor is in reach; one on the top of a wall of the square is not
    World w = hub;
    w.BuildSolids();
    CHECK(!st::OrbStranded({0.0f, kFloorY + dl::mech::kOrbRadius, 23.0f}, 0, w.solids[0], w.counts[0], w.canal));
    CHECK(st::OrbStranded({st::kHubEast + 0.25f, st::kHubWallTop + dl::mech::kOrbRadius, 24.0f}, 0, w.solids[0], w.counts[0], w.canal));
    CHECK(st::OrbStranded({0.0f, st::kSquareLowY + dl::mech::kOrbRadius, 31.0f}, 0, w.solids[0], w.counts[0], w.canal)); // in the pool, A
    CHECK(!st::OrbStranded({5.6f, st::kSquareLowY + dl::mech::kOrbRadius, 31.0f}, 1, w.solids[1], w.counts[1], w.canal)); // dry, B
}


/// The cellar: the fourth link of the ring (docs/LEVEL_DESIGN.md, 14.8).
void TestCellar()
{
    bool ok = false;
    const World hub = ReachTheHub(ok);
    CHECK(ok);
    const Vec3 W{-1.0f, 0.0f, 0.0f};
    // the numbers: high enough to walk, under every floor above it, under the water of A, the grate out of reach of the lever
    CHECK(st::kCellarCeiling - st::kCellarY > Player::Settings{}.eyeHeight + 0.35f + 0.25f);
    CHECK(st::kCellarCeiling + 0.2f <= kFloorY - 0.1f + 1e-4f);
    CHECK(st::kCellarWater <= st::MakeCanal().highTop + 1e-4f && st::kCellarWater > st::kCellarY + 1.85f);
    CHECK(st::kCellarSouthX1 <= -3.0f - 1.0f && st::kCellarZ0 >= st::kCourtSouth); // never under the gap
    // B: from the canal bed into the mouth, along the cellar, down the ramp, to the grate: it holds before the lever
    World b = hub;
    b.layer = 1;
    b.PlaceFeet({20.0f, st::kCanalBed, 25.6f});
    CHECK(b.player.Grounded() && st::RoomAt(b.player.Feet()) == Room("canal bed"));
    CHECK(b.WalkPath({{20.0f, kFloorY, 23.0f}, {20.0f, kFloorY, 15.0f}}) && st::RoomAt(b.player.Feet()) == Room("cellar"));
    CHECK(b.WalkPath({{-7.0f, kFloorY, 15.0f}, {-7.0f, kFloorY, 10.0f}, {-9.0f, kFloorY, 10.0f}}) && b.At(-9.0f, st::kWestLowY, 10.0f, 0.4f));
    b.Run(1.5f, Keys(1.0f), W);
    CHECK(b.player.Feet().x > -9.5f + 0.3f && !b.mech.Active(st::kCellarLever)); // the bars hold; the lever is out of reach
    // A: the cellar is flooded (a body put in it is under the water); the sluice panels hold the water at both ends
    World a = hub;
    a.layer = 0;
    a.PlaceFeet({0.0f, st::kCellarY, 15.0f});
    CHECK(a.InWater());
    // the lever inside the lower room opens the grate for good: then the cellar is a shortcut, in B, to the canal and up
    World w = hub;
    w.layer = 1;
    w.PlaceFeet({-12.0f, st::kWestLowY, 9.0f});
    CHECK(w.WalkPath({{-11.0f, kFloorY, 8.6f}}) && w.mech.Active(st::kCellarLever));
    CHECK(!w.mech.GatePresent(st::kCellarGrate) && !w.mech.GatePresent(st::kCellarGrate + 4));
    {
        World a2 = w; // in A the iron panel still closes it
        a2.layer = 0;
        a2.PlaceFeet({-11.0f, st::kWestLowY, 10.0f});
        a2.Run(1.5f, Keys(1.0f), kEast);
        CHECK(a2.player.Feet().x < -10.0f - 0.3f);
    }
    CHECK(w.WalkPath({{-11.0f, kFloorY, 10.0f}, {-7.0f, kFloorY, 10.0f}, {-7.0f, kFloorY, 15.0f}}) && w.At(-7.0f, st::kCellarY, 15.0f, 0.4f));
    st::LightSet lights;
    CHECK(w.WalkPath({{-1.0f, kFloorY, 15.0f}}));
    for (int i = 0; i < 120; ++i)
    {
        w.Frame(Keys(1.0f), kEast);
        lights.Collect(w.player);
    }
    CHECK(lights.Taken(5)); // the light under the courtyard
    CHECK(w.WalkPath({{20.0f, kFloorY, 15.0f}, {20.0f, kFloorY, 23.5f}, {20.0f, kFloorY, 25.6f}}) && st::RoomAt(w.player.Feet()) == Room("canal bed"));
    CHECK(w.WalkPath({{23.2f, kFloorY, 25.15f}, {23.2f, kFloorY, 29.3f}, {23.2f, kFloorY, 31.0f}}) && st::RoomAt(w.player.Feet()) == Room("east hall"));
    // the mouth under the quay: in A the sluice panel holds the canal back
    World m = hub;
    m.layer = 0;
    m.PlaceFeet({20.0f, st::kCellarY, 23.0f});
    m.Run(1.0f, Keys(1.0f), kNorth);
    CHECK(m.player.Feet().z < st::kCanalSouth - 0.2f - 0.3f);
}

/// N2: one prism moves two things (the cage of the ledge out of B, the step into B).
void TestN2()
{
    bool ok = false;
    const World hub = ReachTheHub(ok);
    CHECK(ok);
    const Vec3 W{-1.0f, 0.0f, 0.0f};
    const float ledge = st::kN2LedgeTop;
    const float stepTop = kFloorY + 1.2f;
    const auto takesLight = [](World& t, float seconds, const Player::Input& in, const Vec3& look) {
        st::LightSet set;
        const int frames = static_cast<int>(seconds / kDt);
        for (int i = 0; i < frames; ++i)
        {
            t.Frame(in, look);
            set.Collect(t.player);
            if (set.Taken(4))
            {
                return true;
            }
        }
        return false;
    };
    // the numbers: the ledge is out of reach from the floor, a step from the step; the light is in the wall
    CHECK(ledge - kFloorY >= 2.3f && ledge - stepTop <= 1.2f + 1e-4f);
    // before the prism: in A the step is there but the ledge is not; from the step, no jump (with or without a switch to B in
    // the air: the cage) ends on the ledge or touches the light
    for (float runUp : {0.0f, 0.1f, 0.2f})
    {
        for (float after : {-1.0f, 0.15f, 0.3f, 0.45f})
        {
            for (float z : {57.4f, 57.7f, 58.0f})
            {
                World a = hub;
                a.layer = 0;
                a.PlaceFeet({-4.3f, stepTop, z});
                CHECK(a.player.Grounded() && Near(a.player.Feet().y, stepTop, 0.03f));
                a.Run(runUp, Keys(1.0f), W);
                a.Frame(Keys(1.0f, true), W);
                st::LightSet set;
                bool switched = false;
                bool high = false;
                for (int i = 0; i < 240; ++i)
                {
                    if (after > 0.0f && !switched && static_cast<float>(i) * kDt >= after)
                    {
                        switched = a.Switch(1);
                    }
                    a.Frame(Keys(1.0f), W);
                    set.Collect(a.player);
                    high = high || (a.player.Grounded() && a.player.Feet().y > ledge - 0.1f);
                }
                CHECK(!high && !set.Taken(4));
            }
        }
    }
    {
        World f = hub; // nor a jump from the floor, in either layer, under the light
        for (int layer = 0; layer < 2; ++layer)
        {
            f.layer = layer;
            f.PlaceFeet({-4.6f, kFloorY, 57.7f});
            CHECK(!takesLight(f, 1.2f, Keys(1.0f, true), W));
        }
    }
    // the prism, stepped on in A: the cage goes into A, the step into B, both for good
    World w = hub;
    w.layer = 0;
    w.PlaceFeet({-4.0f, kFloorY, 51.0f});
    CHECK(w.mech.Active(st::kN2Prism) && w.mech.GateBox(st::kN2Cage).layerMask == kLayerA && w.mech.GateBox(st::kN2Step).layerMask == kLayerB);
    CHECK(!w.mech.Active(st::kNorthPrism)); // (the prism of N1 is another one)
    // then in B: onto the step, onto the ledge, to the wall: the light
    CHECK(w.Switch(1));
    CHECK(w.WalkPath({{-3.0f, kFloorY, 57.7f}}));
    CHECK(w.JumpAlong(W, 3.4f, 4.6f) && Near(w.player.Feet().y, stepTop, 0.05f));
    CHECK(w.JumpAlong(W, 4.4f, 6.0f) && Near(w.player.Feet().y, ledge, 0.05f));
    CHECK(takesLight(w, 1.5f, Keys(1.0f), W));
}

/// The end: the master sluice drains every water of A (docs/LEVEL_DESIGN.md, 14.10).
void TestEnding()
{
    st::Waterworks works;
    const Vec3 pool{0.0f, st::kSquareLowY, 36.0f};
    CHECK(st::Submerged(pool, 0, works) && !st::Submerged(pool, 1, works));
    static SceneObject table[64] = {};
    CHECK(st::AddWaterDraw(works, table, 0, 64) == st::kWaterDrawCount);
    CHECK(st::AddWaterDraw(works, table, 60, 64) == 60); // no room: nothing
    works.Update(10.0f); // not closed: nothing drains
    CHECK(works.Drained() == 0.0f);
    works.Close();
    works.Update(0.5f * st::Waterworks::kDrainSeconds);
    CHECK(Near(works.Drained(), 0.5f, 1e-4f) && st::Submerged(pool, 0, works));
    works.Update(st::Waterworks::kDrainSeconds);
    CHECK(works.Drained() == 1.0f && !st::Submerged(pool, 0, works) && !st::Submerged({0.0f, st::kCellarY, 15.0f}, 0, works));
    const int n = st::AddWaterDraw(works, table, 0, 64);
    CHECK(n == 3 && st::SameRgb(table[1], st::kGold)); // only the master sluice is left, its wheel gold
    SceneObject orbWater[8] = {};
    CHECK(st::AddWaterForOrbs(orbWater, 0, 8, works) == 0);
    dl::canal::Canal canal;
    canal.Init(st::MakeCanal());
    canal.SetDrained(1.0f);
    const dl::canal::Desc d = st::MakeCanal();
    CHECK(Near(canal.WaterTop(), d.bed, 1e-5f) && Near(canal.CrateTopA(), canal.CrateTopB(), 1e-5f));
    CHECK(!canal.Submerged({20.0f, d.bed, 27.0f}, 0));
    SceneObject drawn[16] = {};
    const int canalDrawn = canal.AddDraw(drawn, 0, 16);
    bool water = false;
    for (int i = 0; i < canalDrawn; ++i)
    {
        water = water || st::SameRgb(drawn[i], st::Rgb{dl::shapes::kWaterColour.r, dl::shapes::kWaterColour.g, dl::shapes::kWaterColour.b});
    }
    CHECK(!water);
    SceneObject beacons[4] = {};
    CHECK(canal.AddBeacons(beacons, 0, 4) == 2 && beacons[0].layerMask == kLayerBoth); // the two wheels, not yet turned
}

/// A point that touches the lock wheel, as a body would (for the test of the carry, which needs the wheel turned while the
/// body stands on the crate).
struct WheelHand
{
    Vec3 at;
    Vec3 Ball(int) const
    {
        return at;
    }
    Vec3 Feet() const
    {
        return {at.x, at.y - 1.0f, at.z};
    }
    bool Grounded() const
    {
        return false;
    }
    void Carry(const Vec3&)
    {
    }
};

void TestEastCanal()
{
    bool ok = false;
    const World hub = ReachTheHub(ok);
    CHECK(ok);
    const dl::canal::Desc d = st::MakeCanal();
    const float quayTop = st::kQuayY;
    // the numbers: the canal is wider than a running jump; low, the crate is out of reach of the quay; high, it is a step
    CHECK(st::kCanalNorth - st::kCanalSouth > 3.6f + 1.0f);
    CHECK(quayTop - (d.lowTop + d.draft) >= 2.3f && quayTop - (d.highTop + d.draft) <= 1.2f + 1e-4f);
    CHECK(quayTop - (d.bed + d.crateSize) >= 2.3f && kFloorY - (d.bed + d.crateSize) >= 0.0f); // B: on the bed, under the floor
    CHECK(d.highTop < kFloorY && d.lowTop + d.draft - d.crateSize > d.bed);
    // without the wheel: no jump from the hall floor, in either layer, with or without a switch, ends on the quay
    const Vec3 S = kSouth;
    for (int layer = 0; layer < 2; ++layer)
    {
        for (float x : {16.0f, 20.0f, 23.0f})
        {
            for (float at : {29.9f, 29.7f})
            {
                for (float after : {-1.0f, 0.3f})
                {
                    World w = hub;
                    w.layer = layer;
                    w.PlaceFeet({x, kFloorY, 32.0f});
                    int guard = 0;
                    while (w.player.Feet().z > at && w.player.Grounded() && guard++ < 2000)
                    {
                        w.Frame(Keys(1.0f), S);
                    }
                    w.Frame(Keys(1.0f, true), S);
                    bool switched = false;
                    for (int i = 0; i < 240 && !w.InWater() && !w.Fallen(); ++i)
                    {
                        if (after > 0.0f && !switched && static_cast<float>(i) * kDt >= after)
                        {
                            switched = w.Switch(1 - layer);
                        }
                        w.Frame(Keys(1.0f), S);
                    }
                    CHECK(!(w.player.Grounded() && w.player.Feet().y > quayTop - 0.1f && w.player.Feet().z < st::kCanalSouth));
                }
            }
        }
    }
    {
        World w = hub; // in A, onto the low crate: from there the quay is out of reach (and the floor is a jump back)
        w.layer = 0;
        w.PlaceFeet({20.0f, kFloorY, 31.0f});
        CHECK(w.JumpAlong(S, -29.8f, -27.3f) && Near(w.player.Feet().y, d.lowTop + d.draft, 0.05f));
        const World onCrate = w;
        for (float at : {26.9f, 26.7f})
        {
            World t = onCrate;
            t.JumpAlong(S, -at, -22.0f);
            CHECK(!(t.player.Grounded() && t.player.Feet().y > quayTop - 0.1f));
        }
        World back = onCrate;
        CHECK(back.JumpAlong(kNorth, 27.5f, 31.0f) && back.At(20.0f, kFloorY, 30.5f, 1.0f));
    }
    {
        World w = hub; // the wheel is machinery of A: in B it does not turn
        w.layer = 1;
        w.PlaceFeet({16.4f, kFloorY, 33.1f});
        w.Run(1.0f);
        CHECK(!w.canal.Open() && !w.canal.High());
        w.layer = 0;
        w.Run(0.1f);
        CHECK(w.canal.Open());
        int frames = 0;
        while (!w.canal.High() && frames++ < 1200)
        {
            w.Frame({}, kNorth);
        }
        CHECK(w.canal.High() && static_cast<float>(frames) * kDt < 3.0f);
        w.canal.Reset(); // R: the water and the crate are put back
        CHECK(!w.canal.Open() && !w.canal.High() && Near(w.canal.WaterTop(), d.lowTop, 1e-5f));
    }
    {
        World w = hub; // a body on the crate while the water rises is carried up, never in the air
        w.layer = 0;
        w.PlaceFeet({20.0f, d.lowTop + d.draft, d.crateZ});
        CHECK(w.player.Grounded());
        WheelHand hand{d.wheel};
        w.canal.Update(hand, 0, kDt);
        CHECK(w.canal.Open());
        int air = 0;
        for (int i = 0; i < 600 && !w.canal.High(); ++i)
        {
            w.Frame({}, kNorth);
            air += w.player.Grounded() ? 0 : 1;
        }
        w.Run(0.2f);
        CHECK(air == 0 && Near(w.player.Feet().y, d.highTop + d.draft, 0.03f));
    }
    for (int high = 0; high < 2; ++high) // in A the canal is water at either level: a body that goes in is in the water
    {
        World w = hub;
        w.layer = 0;
        if (high == 1)
        {
            WheelHand hand{d.wheel};
            w.canal.Update(hand, 0, kDt);
            w.Run(3.0f);
        }
        w.PlaceFeet({21.0f, kFloorY, 30.4f}); // (east of the parapet at the west end of the floor)
        bool wet = false;
        for (int i = 0; i < 240 && !wet; ++i)
        {
            w.Frame(Keys(1.0f), S);
            wet = w.InWater();
        }
        CHECK(wet);
    }
    {
        World w = hub; // in B the canal is dry: down the stairs at its east end, along the bed, and back up
        w.layer = 1;
        w.PlaceFeet({23.2f, kFloorY, 31.0f});
        CHECK(w.WalkPath({{23.2f, kFloorY, 29.3f}, {23.2f, kFloorY, 25.15f}}) && w.At(23.2f, d.bed, 25.15f, 0.4f));
        CHECK(!w.InWater());
        CHECK(w.WalkPath({{16.0f, kFloorY, 25.4f}}) && w.At(16.0f, d.bed, 25.4f, 0.4f));
        CHECK(w.WalkPath({{23.2f, kFloorY, 25.15f}, {23.2f, kFloorY, 29.3f}, {23.2f, kFloorY, 31.0f}}) && w.At(23.2f, kFloorY, 31.0f, 0.4f));
        World up = w; // from the bed no jump reaches the quay
        for (float z : {25.4f, 26.4f, 27.4f})
        {
            CHECK(up.JumpFrom({17.0f, d.bed, z}, S, 0.3f) < quayTop - 1.0f);
        }
    }
}

void TestAnchors()
{
    for (int i = 0; i < st::kAnchorCount; ++i)
    {
        const st::AnchorDesc& a = st::kAnchors[i];
        const int room = st::RoomIndex(a.room);
        CHECK(room >= 0 && st::RoomAt(a.stand) == room);
        // not on the board of the courtyard: the pedestal would change the proved puzzle
        CHECK(!(a.base.x > st::kCourtWest - 0.5f && a.base.x < st::kCourtEast + 0.5f && a.base.z > st::kCourtSouth - 0.5f &&
                a.base.z < st::kCourtNorth + 0.5f));
        for (int layer = 0; layer < 2; ++layer)
        {
            World w;
            w.TakeAllSeals();
            w.layer = layer;
            w.PlaceFeet(a.stand);
            CHECK(w.player.Grounded() && Near(w.player.Feet().y, a.stand.y, 0.02f) && w.puzzle.BodyFits(w.player, layer, w.spin));
        }
    }
    // the first lamp is on the way: walking out of the guardhouse lights it; then a fall on the terrace comes back to it
    World w;
    st::AnchorSet anchors;
    const int terrace = Room("terrace");
    const int first = st::AnchorSet::AnchorOf(terrace);
    CHECK(first == 1 && Near(anchors.RespawnPoint(terrace).z, st::kRooms[terrace].entry.z, 1e-5f));
    int lit = -1;
    for (int i = 0; i < 360 && lit < 0; ++i)
    {
        w.Frame(Keys(1.0f), kNorth);
        lit = anchors.Update(w.player);
    }
    CHECK(lit == first && anchors.Lit(first) && !anchors.Lit(0) && !anchors.Lit(first + 1));
    CHECK(Near(anchors.RespawnPoint(terrace).x, st::kAnchors[first].stand.x, 1e-5f) && Near(anchors.RespawnPoint(terrace).z, st::kAnchors[first].stand.z, 1e-5f));
    CHECK(Near(anchors.RespawnPoint(Room("courtyard")).x, st::kRooms[Room("courtyard")].entry.x, 1e-5f)); // a room without a lamp
    SceneObject table[16] = {};
    CHECK(anchors.AddDraw(table, 0, 16) == st::kAnchorCount && table[first].glow > 1.0f && table[0].glow < 0.5f);
    CHECK(table[first].layerMask == kLayerBoth && st::AddRespawnRing({0, 0, 0}, 0.2f, table, 0, 8) == 1 && st::AddRespawnRing({0, 0, 0}, -1.0f, table, 0, 8) == 0);
    // a pedestal is the same in both layers (both, and a trace of light: not the stone of the layer)
    int pedestals = 0;
    for (const SceneObject& o : st::kStageStatics)
    {
        pedestals += (o.layerMask == kLayerBoth && o.glow > 0.0f && st::SameRgb(o, st::kAnchorStone)) ? 1 : 0;
    }
    CHECK(pedestals == st::kAnchorCount);
}

void TestFallLines()
{
    for (int i = 0; i < st::kRoomCount; ++i)
    {
        const st::Room& r = st::kRooms[i];
        CHECK(st::FallLine(i) <= r.y0 - 3.0f && r.entry.y > st::FallLine(i) + 3.0f);
        CHECK(st::FallLine(i) >= r.y0 - st::kFallBelowRoom - 0.01f); // and not far below it: a fall is noticed at once
    }
    CHECK(st::FallLine(Room("west lower")) < st::kWestLowY - 3.0f && st::FallLine(Room("west vault")) < st::kWestLowY - 3.0f);
    for (const st::AnchorDesc& a : st::kAnchors)
    {
        CHECK(a.stand.y > st::FallLine(st::RoomIndex(a.room)) + 3.0f);
    }
    // the drops of the map land above the fall line of the room they start from
    CHECK(kFloorY > st::FallLine(Room("gap")) && kFloorY > st::FallLine(Room("gallery")) && kFloorY > st::FallLine(Room("north hall")));
    CHECK(st::kSquareLowY > st::FallLine(Room("square")) && st::kCanalBed > st::FallLine(Room("east hall")));
    CHECK(kFloorY > st::FallLine(Room("ditch")) + 3.0f);
}

/// The top of the water called `name`, or a very low number.
float WaterTopOf(std::string_view name)
{
    if (name == "the pool")
    {
        return st::kWaters[st::kPoolWater].top;
    }
    if (name == "the trench")
    {
        return st::kWaters[st::kTrenchWater].top;
    }
    if (name == "the canal")
    {
        return st::MakeCanal().highTop;
    }
    if (name == "the cellar")
    {
        return st::kCellarWater;
    }
    if (name == "the lower lock" || name == "the middle lock" || name == "the upper lock")
    {
        const int k = name == "the lower lock" ? 0 : (name == "the middle lock" ? 1 : 2);
        return st::kLockWaters[k].top;
    }
    if (name == "the well")
    {
        return st::kWellWater;
    }
    if (name == "the aqueduct")
    {
        return st::kAqueductBottom;
    }
    return -1000.0f;
}

void TestWater()
{
    // every water of A is fed, through a chain, by the aqueduct; each feed runs downhill
    CHECK(st::kWaterFeedCount == st::kWaterCount + 2 + st::kLockCount + 1); // the static waters, the canal, the cellar, the locks, the well
    for (const st::WaterFeed& f : st::kWaterFeeds)
    {
        CHECK(Near(f.top, WaterTopOf(f.water), 1e-4f));
        std::string_view at = f.water;
        int steps = 0;
        while (at != "the aqueduct" && steps++ < st::kWaterFeedCount)
        {
            std::string_view from;
            for (const st::WaterFeed& g : st::kWaterFeeds)
            {
                from = g.water == at ? std::string_view(g.from) : from;
            }
            CHECK(!from.empty() && WaterTopOf(from) >= WaterTopOf(at));
            at = from;
        }
        CHECK(at == "the aqueduct");
    }
    // the aqueduct: whole in A from its spout to its source; in B broken by more than a running jump. No water in B anywhere
    float coveredA = 0.0f;
    float coveredB = 0.0f;
    for (const SceneObject& o : st::kDecor)
    {
        const bool channelFloor = Near(o.position[1] - 0.5f * o.scale[1], st::kAqueductBottom, 1e-4f) && Near(o.scale[1], 0.2f, 1e-4f) &&
                                  Near(o.position[2], 0.5f * (st::kAqueductZ0 + st::kAqueductZ1), 1e-3f);
        if (channelFloor)
        {
            coveredA += (o.layerMask & kLayerA) != 0u ? o.scale[0] : 0.0f;
            coveredB += (o.layerMask & kLayerB) != 0u ? o.scale[0] : 0.0f;
        }
        CHECK(!st::SameRgb(o, st::kWaterRgb)); // no water is baked: it drains at the end
    }
    static SceneObject waterTable[64] = {};
    const int nWater = st::AddWaterDraw(st::Waterworks{}, waterTable, 0, 64);
    int waterInB = 0;
    int fallsInA = 0;
    for (int i = 0; i < nWater; ++i)
    {
        const SceneObject& o = waterTable[i];
        const bool water = st::SameRgb(o, st::kWaterRgb) || st::SameRgb(o, st::Rgb{dl::shapes::kWaterColour.r, dl::shapes::kWaterColour.g, dl::shapes::kWaterColour.b});
        waterInB += (water && (o.layerMask & kLayerB) != 0u) ? 1 : 0;
        fallsInA += (water && o.layerMask == kLayerA && o.scale[1] > 5.0f) ? 1 : 0;
    }
    CHECK(Near(coveredA, st::kAqueductEast - st::kAqueductWest, 1e-3f));
    CHECK(coveredB < coveredA - 3.6f - 1.0f && st::kAqueductBreakX1 - st::kAqueductBreakX0 > 3.6f + 1.0f);
    CHECK(waterInB == 0 && fallsInA == 1);
    // the aqueduct is out of reach of every floor near it (it rests on the walls of the square)
    CHECK(st::kAqueductBottom > st::kHubWallTop && st::kAqueductBottom > st::kGalleryY + 2.3f + 1.55f && st::kLedgeEast < st::kAqueductWest - 0.8f - 5.0f);
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
    CHECK(st::CurrentRoom({-50.0f, kFloorY, 25.0f}, true, last) == last);     // standing in no room: the last one
}

} // namespace

/// A body that only touches what it is put at (the lamps lit by it, the wheel turned by it).
struct Hand
{
    Vec3 at;
    Vec3 Ball(int) const
    {
        return at;
    }
    Vec3 Feet() const
    {
        return at;
    }
    bool Grounded() const
    {
        return false;
    }
    void Carry(const Vec3&)
    {
    }
};

/// Beat 0 of the prologue (docs/LEVEL_DESIGN.md 6.1): the game starts on the quay; the stair between the harbour walls leads up
/// to the south door of the guardhouse; the sea is a fall; the harbour walls are out of reach.
void TestHarbour()
{
    World w;
    w.PlaceFeet(st::kRooms[st::kStartRoom].entry);
    CHECK(std::string_view(st::kRooms[st::RoomAt(w.player.Feet())].name) == "quay" && w.player.Grounded());
    // in both layers: along the quay, up the stair, through the south door, out of the north arch onto the terrace
    for (int layer = 0; layer < 2; ++layer)
    {
        World u = w;
        u.layer = layer;
        CHECK(u.WalkPath({{0.0f, kFloorY, -50.0f}, {0.0f, kFloorY, -45.0f}, {0.0f, kFloorY, -26.0f}}) && u.At(0.0f, st::kTerraceY, -26.0f, 0.4f));
        CHECK(st::RoomAt(u.player.Feet()) == Room("sea stair"));
        CHECK(u.WalkPath({{0.0f, kFloorY, -22.0f}, {0.0f, kFloorY, -17.0f}}) && st::RoomAt(u.player.Feet()) == Room("terrace"));
    }
    // off the edge of the quay into the sea: a fall (below the fall line of the quay), in both layers
    for (int layer = 0; layer < 2; ++layer)
    {
        World f = w;
        f.layer = layer;
        f.PlaceFeet({10.0f, st::kHarbourY, -48.0f});
        float lowest = 1000.0f;
        for (int i = 0; i < 270; ++i)
        {
            f.Frame(Keys(1.0f), kNorth);
            lowest = f.player.Feet().y < lowest ? f.player.Feet().y : lowest;
        }
        CHECK(lowest < st::FallLine(Room("quay")));
    }
    // the harbour walls beside the stair: a jump sideways from the stair or the landing stays between them (in the room of
    // the stair, on its floor)
    for (float z : {-44.0f, -36.0f, -30.0f, -26.0f})
    {
        const float y = z > st::kSeaStairTop ? st::kTerraceY : st::kHarbourY + (st::kTerraceY - st::kHarbourY) * (z - st::kQuayZ1) / (st::kSeaStairTop - st::kQuayZ1);
        for (float side : {-1.0f, 1.0f})
        {
            World j = w;
            j.JumpFrom({side, y + 0.02f, z}, {side, 0.0f, 0.0f}, 0.2f);
            CHECK(std::fabs(j.player.Feet().x) < st::kSeaStairHalf + 0.05f && j.player.Grounded() && st::RoomAt(j.player.Feet()) == Room("sea stair"));
        }
    }
    // the stair is a walk: at most 27 degrees
    CHECK(std::atan2(st::kTerraceY - st::kHarbourY, st::kSeaStairTop - st::kQuayZ1) * 180.0f / 3.14159265f <= 27.0f);
}

/// E3, the lock walk (docs/LEVEL_DESIGN.md 6.4): from the east hall up four terraces beside three locks; the basins are water in
/// A and dry rooms in B, each with a stair back up; the light on the bed of the middle lock is taken in B only.
void TestLockWalk()
{
    bool ok = false;
    const World hub = ReachTheHub(ok);
    CHECK(ok);
    const Vec3 E{1.0f, 0.0f, 0.0f};
    const float zw = 0.5f * (st::kLockWalkZ0 + st::kLockWalkZ1) - 0.5f; // the middle of the walk (south of the lamp)
    for (int layer = 0; layer < 2; ++layer)
    {
        World w = hub;
        w.layer = layer;
        CHECK(w.WalkPath({{kEastRim, kFloorY, 23.5f}, {kEastRim, kFloorY, 31.0f}, {12.0f, kFloorY, 31.0f}, {20.0f, kFloorY, 34.6f},
                          {26.0f, kFloorY, 34.6f}}));
        CHECK(st::RoomAt(w.player.Feet()) == Room("lock walk"));
        CHECK(w.WalkPath({{26.0f, kFloorY, zw}, {60.0f, kFloorY, zw}}) && w.At(60.0f, st::kTerraceFloor[3], zw, 0.4f));
        CHECK(st::RoomAt(w.player.Feet()) == Room("lock walk"));
        World back = w; // and back down
        CHECK(back.WalkPath({{27.0f, kFloorY, zw}}) && back.At(27.0f, kFloorY, zw, 0.4f));
    }
    // the walls round it hold: from the top terrace and from each terrace, a running jump north, east or south never lands
    // out of the room, and every wall top is out of reach
    for (int k = 0; k < 4; ++k)
    {
        CHECK(st::kLockWallTop > st::kTerraceFloor[k] + 2.0f + 0.6f);
    }
    for (const Vec3& d : {kNorth, kSouth, E})
    {
        World j = hub;
        j.JumpFrom({59.0f, st::kTerraceFloor[3], 30.0f}, d, 0.4f);
        CHECK(st::RoomAt(j.player.Feet()) == Room("lock walk"));
    }
    // a lock: in A water (a body that goes in is in it), in B a dry room with its stair back up
    for (int k = 0; k < st::kLockCount; ++k)
    {
        const float x = 0.5f * (st::kTerraceX0[k] + st::kTerraceX1[k]) - 1.0f;
        World a = hub;
        a.layer = 0;
        a.PlaceFeet({x, st::kTerraceFloor[k], st::kLockWalkZ0 + 0.6f});
        bool wet = false;
        for (int i = 0; i < 240 && !wet; ++i)
        {
            a.Frame(Keys(1.0f), kSouth);
            wet = a.InWater();
        }
        CHECK(wet);
        World b = hub;
        b.layer = 1;
        const float bed = st::kTerraceFloor[k] - st::kBasinDepth;
        b.PlaceFeet({x, bed, 27.0f});
        CHECK(b.player.Grounded() && !b.InWater() && st::RoomAt(b.player.Feet()) == Room("lock walk"));
        const float sx = st::kTerraceX1[k] - 0.5f * st::kBasinStairWidth;
        CHECK(b.WalkPath({{sx, kFloorY, 26.0f}, {sx, kFloorY, st::kLockWalkZ0 + 1.0f}}) && b.At(sx, st::kTerraceFloor[k], st::kLockWalkZ0 + 1.0f, 0.4f));
        World s = hub; // no jump from the bed reaches the terrace without the stair
        s.layer = 1;
        CHECK(s.JumpFrom({x, bed, 29.5f}, kNorth, 0.3f) < st::kTerraceFloor[k] - 1.0f);
    }
    // the light of the middle lock: taken on its bed in B; in A every pose that touches it is under the water
    const Vec3 c = st::kHiddenLights[6].centre;
    World t = hub;
    t.layer = 1;
    t.PlaceFeet({c.x, st::kTerraceFloor[1] - st::kBasinDepth, c.z});
    st::LightSet set;
    CHECK(set.Collect(t.player) == 6);
    bool dryTouch = false;
    for (float y = st::kTerraceFloor[1] - st::kBasinDepth; y < st::kTerraceFloor[1] + 1.0f; y += 0.05f)
    {
        World u = hub;
        u.layer = 0;
        u.player.Place({c.x, y + Player::Settings{}.eyeHeight, c.z});
        st::LightSet probe;
        const bool touches = probe.Collect(u.player) == 6;
        dryTouch = dryTouch || (touches && !st::InWater({c.x, y, c.z}, 0, st::Waterworks{}, u.canal));
    }
    CHECK(!dryTouch);
}

/// The market and the well (docs/LEVEL_DESIGN.md 6.3.1): from the west hall along the lane into the market; down the stair of
/// the well in A to the corner over the water, then in B to the dry bottom and its light; back up in B the fallen stretch
/// stops the body, in A it is walked.
void TestMarketWell()
{
    bool ok = false;
    const World hub = ReachTheHub(ok);
    CHECK(ok);
    const float xe = st::kShaftX1 - 0.9f;  // the middle of the east stretch
    const float xw = st::kShaftX0 + 0.9f;  // of the west
    const float zs = st::kShaftZ0 + 0.9f;  // of the south
    const float zn = st::kShaftZ1 - 0.9f;  // of the north
    World w = hub;
    w.layer = 0;
    w.PlaceFeet(st::kRooms[Room("west hall")].entry);
    CHECK(w.WalkPath({{-20.0f, kFloorY, 34.6f}, {-28.0f, kFloorY, 34.6f}}) && st::RoomAt(w.player.Feet()) == Room("west lane"));
    CHECK(w.WalkPath({{-36.0f, kFloorY, 34.6f}, {-36.5f, kFloorY, 51.6f}, {xe, kFloorY, 51.6f}}) && st::RoomAt(w.player.Feet()) == Room("market"));
    for (int layer = 0; layer < 2; ++layer) // the lane and the square are walked in both layers
    {
        World u = hub;
        u.layer = layer;
        u.PlaceFeet(st::kRooms[Room("west hall")].entry);
        CHECK(u.WalkPath({{-20.0f, kFloorY, 34.6f}, {-36.0f, kFloorY, 34.6f}, {-36.0f, kFloorY, 45.0f}}) &&
              st::RoomAt(u.player.Feet()) == Room("market"));
    }
    // down in A: the east stretch, the wooden south stretch, to the south-west corner, 1 m over the water
    CHECK(w.WalkPath({{xe, kFloorY, zn}, {xe, kFloorY, zs}, {xw, kFloorY, zs}}) && w.At(xw, st::kLandingSW, zs, 0.4f));
    CHECK(st::RoomAt(w.player.Feet()) == Room("well") && !w.InWater());
    {
        World a = w; // on in A: the west stretch goes under the water
        bool wet = false;
        for (int i = 0; i < 300 && !wet; ++i)
        {
            a.Frame(Keys(1.0f), kNorth);
            wet = a.InWater();
        }
        CHECK(wet);
    }
    CHECK(w.Switch(1));
    CHECK(w.WalkPath({{xw, kFloorY, zn}, {-45.0f, kFloorY, zn}, {-45.0f, kFloorY, 46.0f}}) && w.At(-45.0f, st::kWellFloor, 46.0f, 0.4f));
    st::LightSet lights;
    for (int i = 0; i < 120 && !lights.Taken(7); ++i)
    {
        w.Frame(Keys(1.0f), Vec3{-0.2f, 0.0f, -0.98f});
        lights.Collect(w.player);
    }
    CHECK(lights.Taken(7));
    // back up in B to the south-west corner; the fallen stretch stops it there
    CHECK(w.WalkPath({{-44.2f, kFloorY, 47.5f}, {-44.2f, kFloorY, zn}, {xw, kFloorY, zn}, {xw, kFloorY, zs}}) && w.At(xw, st::kLandingSW, zs, 0.4f));
    {
        World b = w;
        b.Run(2.0f, Keys(1.0f), kEast);
        CHECK(b.player.Feet().y < st::kLandingSW - 1.0f); // off the corner, down to the floor
        World j = w;
        CHECK(j.JumpFrom({xw, st::kLandingSW, zs}, kEast, 0.3f) < st::kLandingSE - 1.0f); // no jump reaches the next corner
    }
    CHECK(w.Switch(0));
    CHECK(w.WalkPath({{xe, kFloorY, zs}, {xe, kFloorY, zn}, {xe, kFloorY, 51.6f}}) && w.At(xe, kFloorY, 51.6f, 0.4f));
    // from the bottom no jump reaches a landing without the stair
    World f = hub;
    f.layer = 1;
    CHECK(f.JumpFrom({-45.0f, st::kWellFloor, 40.5f}, kSouth, 0.3f) < st::kWellFloor + 0.5f);
}

/// Step 1 of docs/LEVEL_DESIGN.md 12: a switch is refused into water; every place of return is a place to stand out of water.
void TestRecovery()
{
    bool ok = false;
    const World hub = ReachTheHub(ok);
    CHECK(ok);
    const st::Waterworks full;
    {
        // the cellar in B, dry: a switch to A would put the body under the water of the cellar: refused (it used to be sent to
        // the entry of the cellar, which is under water too, again and again)
        World w = hub;
        w.layer = 1;
        w.PlaceFeet({0.0f, st::kCellarY, 15.0f});
        w.BuildSolids();
        CHECK(w.player.Grounded() && !st::SwitchAllowed(w.player, 0, w.spin, w.solids[0], w.counts[0], full, w.canal));
        CHECK(!w.Switch(0) || true); // (World::Switch asks only for room: the program asks SwitchAllowed)
        st::Waterworks drained; // at the end the water has gone: then the switch is allowed
        drained.Close();
        drained.Update(10.0f);
        w.canal.SetDrained(1.0f);
        CHECK(st::SwitchAllowed(w.player, 0, w.spin, w.solids[0], w.counts[0], drained, w.canal));
        w.canal.SetDrained(0.0f);
        // the canal bed and the floor of the pool in B: the same
        World c = hub;
        c.layer = 1;
        c.PlaceFeet({17.0f, st::kCanalBed, 25.6f});
        c.BuildSolids();
        CHECK(c.player.Grounded() && !st::SwitchAllowed(c.player, 0, c.spin, c.solids[0], c.counts[0], full, c.canal));
        World p = hub;
        p.layer = 1;
        p.PlaceFeet({5.6f, st::kSquareLowY, 31.0f});
        p.BuildSolids();
        CHECK(p.player.Grounded() && !st::SwitchAllowed(p.player, 0, p.spin, p.solids[0], p.counts[0], full, p.canal));
        // on the causeway, over the water: allowed
        World q = hub;
        q.layer = 1;
        q.PlaceFeet({-2.5f, kFloorY, 26.0f});
        q.BuildSolids();
        CHECK(st::SwitchAllowed(q.player, 0, q.spin, q.solids[0], q.counts[0], full, q.canal));
    }
    // every room, both worlds, the lamp lit and not, the waters full, raised by the dawn bell and drained, the canal low and high:
    // the place of return is out of the water and a place to stand (with every fragment there)
    st::AnchorSet unlit;
    st::AnchorSet lit;
    for (const st::AnchorDesc& a : st::kAnchors)
    {
        lit.Update(Hand{{a.base.x, a.base.y + st::kAnchorHeight, a.base.z}});
    }
    st::Waterworks raised;
    raised.SetRaise(st::kDawnRise);
    st::Waterworks drained;
    drained.Close();
    drained.Update(10.0f);
    dl::canal::Canal low;
    low.Init(st::MakeCanal());
    dl::canal::Canal high;
    high.Init(st::MakeCanal());
    high.SetMachinery(true);
    for (int i = 0; i < 1000; ++i)
    {
        Hand away{{0.0f, -50.0f, 0.0f}};
        high.Update(away, 0, 0.01f);
    }
    CHECK(high.High());
    int checked = 0;
    int wet = 0;
    int fallbacks = 0;
    std::vector<Vec3> places;
    for (int room = 0; room < st::kRoomCount; ++room)
    {
        for (int layer = 0; layer < 2; ++layer)
        {
            for (const st::AnchorSet* anchors : {&unlit, &lit})
            {
                const st::Waterworks* const allWaters[3] = {&full, &raised, &drained};
                for (const st::Waterworks* waters : allWaters)
                {
                    for (dl::canal::Canal* canal : {&low, &high})
                    {
                        canal->SetDrained(waters->Drained());
                        const Vec3 at = st::SafeRespawn(room, layer, *anchors, *waters, *canal);
                        ++checked;
                        wet += st::InWater(at, layer, *waters, *canal) ? 1 : 0;
                        fallbacks += (at.x != anchors->RespawnPoint(room).x || at.z != anchors->RespawnPoint(room).z) ? 1 : 0;
                        places.push_back({at.x, at.y + static_cast<float>(layer) * 1000.0f, at.z}); // (the layer kept in y)
                    }
                }
            }
        }
    }
    std::printf("  recovery: %d places of return, %d under water, %d sent to a dry room\n", checked, wet, fallbacks);
    CHECK(wet == 0 && fallbacks > 0);
    // each distinct place: a place to stand in its world (every fragment and every lamp there)
    int stands = 0;
    for (std::size_t i = 0; i < places.size(); ++i)
    {
        bool seen = false;
        for (std::size_t j = 0; j < i && !seen; ++j)
        {
            seen = places[j].x == places[i].x && places[j].y == places[i].y && places[j].z == places[i].z;
        }
        if (seen)
        {
            continue;
        }
        const int layer = places[i].y > 500.0f ? 1 : 0;
        const Vec3 at{places[i].x, places[i].y - static_cast<float>(layer) * 1000.0f, places[i].z};
        World t;
        t.TakeAllSeals();
        t.TakeAllLights();
        t.layer = layer;
        t.PlaceFeet(at);
        t.BuildSolids();
        const bool good = t.player.Grounded() && Near(t.player.Feet().y, at.y, 0.03f) &&
                          dl::puzzle::Puzzle::FitsIn(t.player, layer, t.spin, t.solids[layer], t.counts[layer]);
        if (!good)
        {
            std::printf("  not a place to stand: %.2f %.2f %.2f in %c\n", static_cast<double>(at.x), static_cast<double>(at.y), static_cast<double>(at.z),
                        layer == 0 ? 'A' : 'B');
        }
        stands += good ? 1 : 0;
        CHECK(good);
    }
    CHECK(stands > 20);
}

/// Step 4 of docs/LEVEL_DESIGN.md 12: the bells as data, and the memory fragments.
void TestCityState()
{
    st::SealSet seals;
    st::CityState city;
    city.FromSeals(seals);
    CHECK(city.Count() == 0 && city.WaterRise() == 0.0f && !city.Machinery() && !city.Night());
    seals.Collect(Touch{st::kSeals[st::kSealOfBell[0]].centre}); // the east seal rings the dawn bell
    city.FromSeals(seals);
    CHECK(city.Rung(st::Bell::Dawn) && city.Count() == 1 && Near(city.WaterRise(), st::kDawnRise, 1e-6f));
    city.Update(0.5f * st::kRiseSeconds);
    CHECK(city.RiseNow() > 0.0f && city.RiseNow() < st::kDawnRise); // it rises, it does not jump
    for (int i = 0; i < 100; ++i)
    {
        city.Update(0.1f);
    }
    CHECK(Near(city.RiseNow(), st::kDawnRise, 1e-5f) && city.NightNow() == 0.0f);
    seals.Collect(Touch{st::kSeals[st::kSealOfBell[1]].centre});
    seals.Collect(Touch{st::kSeals[st::kSealOfBell[2]].centre});
    city.FromSeals(seals);
    for (int i = 0; i < 100; ++i)
    {
        city.Update(0.1f);
    }
    CHECK(city.Count() == st::kBellCount && city.Machinery() && city.Night() && Near(city.NightNow(), 1.0f, 1e-5f));
    // the dawn bell: every water of A stands higher; the rim of the square, the causeway and the places of return stay dry
    st::Waterworks raised;
    raised.SetRaise(st::kDawnRise);
    CHECK(Near(raised.Top(st::kWaters[st::kPoolWater]), st::kWaters[st::kPoolWater].top + st::kDawnRise, 1e-5f));
    CHECK(!st::Submerged({-2.5f, kFloorY, 26.0f}, 0, raised) && !st::Submerged({0.0f, kFloorY, 23.0f}, 0, raised));
    CHECK(st::Submerged({0.0f, st::kSquareLowY, 31.0f}, 0, raised));
    raised.Close(); // and at the end it drains to the same beds
    raised.Update(10.0f);
    CHECK(Near(raised.Top(st::kWaters[st::kPoolWater]), st::kWaters[st::kPoolWater].top - st::kWaters[st::kPoolWater].depth, 1e-5f));
    // the day bell: the lock opens by itself, also after R; the weir does not move
    dl::canal::Canal canal;
    canal.Init(st::MakeCanal());
    canal.SetMachinery(true);
    Hand away{{0.0f, -50.0f, 0.0f}};
    canal.Update(away, 1, 0.01f);
    CHECK(canal.Open() && !canal.WeirOpen());
    canal.Reset();
    canal.Update(away, 1, 0.01f);
    CHECK(canal.Open());
    // the fragments: none before their light; after it, in B only, in pale glowing stone
    st::LightSet lights;
    SceneObject table[64] = {};
    CHECK(st::AddFragments(lights, true, table, 0, 64) == 0);
    int pieces = 0;
    for (int f = 0; f < st::kFragmentCount; ++f)
    {
        pieces += static_cast<int>(st::Fragments().solid[f].size() + st::Fragments().drawn[f].size());
    }
    CHECK(pieces <= 16); // (the drawn table counts 16: tests/test_hints.cpp)
    lights.Collect(Touch{st::kHiddenLights[st::kFragments[0].light].centre});
    const int solid = st::AddFragments(lights, false, table, 0, 64);
    const int drawn = st::AddFragments(lights, true, table, 0, 64);
    CHECK(solid >= 1 && drawn > solid);
    for (int i = 0; i < drawn; ++i)
    {
        CHECK(table[i].layerMask == kLayerB && table[i].glow > 0.2f && st::SameRgb(table[i], st::kMemoryStone));
    }
    // the steps of the fountain: in B, after the light of the pool, a third way out of the pool, in its south-west corner
    bool ok = false;
    const World hub = ReachTheHub(ok);
    CHECK(ok);
    const float x = st::kPoolWest + 0.5f * st::kStairWidth;
    World before = hub;
    before.layer = 1;
    before.PlaceFeet({x, st::kSquareLowY, st::kPoolSouth + st::kStairRun + 1.0f});
    before.Run(2.0f, Keys(1.0f), kSouth);
    CHECK(Near(before.player.Feet().y, st::kSquareLowY, 0.05f)); // the edge of the pool: no way up here
    World after = hub;
    after.TakeAllLights();
    after.layer = 1;
    after.PlaceFeet({x, st::kSquareLowY, st::kPoolSouth + st::kStairRun + 1.0f});
    CHECK(after.WalkPath({{x, kFloorY, st::kPoolSouth - 0.8f}}) && after.At(x, kFloorY, st::kPoolSouth - 0.8f, 0.4f));
    World inA = after; // in A the fragment is not there: the stair is not solid
    inA.layer = 0;
    inA.BuildSolids();
    int inAcount = 0;
    for (int i = 0; i < inA.counts[0]; ++i)
    {
        inAcount += (st::SameRgb(inA.solids[0][i], st::kMemoryStone) && (inA.solids[0][i].layerMask & kLayerA) != 0u) ? 1 : 0;
    }
    CHECK(inAcount == 0);
    // the bench by the gate does not close the way through the breach
    World gate;
    gate.TakeAllLights();
    gate.layer = 1;
    gate.PlaceFeet({0.0f, st::kTerraceY, st::kGateZ - 2.0f});
    gate.Run(1.2f, Keys(1.0f), kNorth);
    CHECK(gate.player.Feet().z > st::kGateZ + 1.5f);
}

int main()
{
    TestTables();
    TestPrologue();
    TestCourtyard();
    TestWings();
    TestWestWing();
    TestRing();
    TestEastCanal();
    TestUndercroft();
    TestPrism();
    TestOrbReturn();
    TestCellar();
    TestN2();
    TestEnding();
    TestAnchors();
    TestFallLines();
    TestWater();
    TestSquare();
    TestTower();
    TestNoWayRound();
    TestRooms();
    TestRecovery();
    TestCityState();
    TestHarbour();
    TestLockWalk();
    TestMarketWell();
    std::printf("%d checks passed, %d failed\n", g_passed, g_failed);
    return g_failed == 0 ? 0 : 1;
}
