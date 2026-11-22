// Stage.h - the map (docs/LEVEL_DESIGN.md, step 1: the skeleton). A walled city in two layers round a hub, walked in about
//            ten minutes when its rooms are filled; in this step the wings are grey boxes with their shapes, heights and
//            ways in and out, so that the size and the flow of the whole map can be played before any room is built.
//   prologue   the terrace with the town wall across it: in A (the whole city) its gate is shut; in B (the city after the
//              storm) the wall is broken open where the gate was, with rubble at the edges of the breach - the first lesson,
//              "in A the structure is the way, in B the breach is"; the gap (a wide blue stone in B, then a switch to A in
//              the air onto the red strip; a blue curtain at the end of the strip keeps B from flying over it), the overlook, and
//              the courtyard with the proved block puzzle (scene/Courtyard.h). The block on the plate opens the blue door
//              in the north wall of the courtyard: the way into the hub.
//   hub        a square place, walled 10 m high, with the tower in the middle and a gallery along its south-east corner
//              (2.4 m up, reached only from the east wing: the hub seen from above). The tower door carries three seal lamps; it
//              opens when the three seals of the wings are taken. Three openings lead to the wings; a fourth, closed door in
//              the west wall is the far end of a shortcut that is opened from the other side.
//   west wing  going DOWN, the archive: an orb that is carried between the layers (scene/WestWing.h). W1, the hall: a
//              trench, bridged in B by an orb on a socket of B. W2: an orb on a plate of A opens a door of B at the foot of the
//              ramp down; a lever inside keeps it open, so that the orb can be fetched. W3, the lower room: from a mezzanine,
//              the orb is let fall through a hole that is open only in B onto a socket in the vault below, which opens its
//              bars. W4, the vault: the seal, and a ramp up to the closed door in the hub wall, which the seal opens (a
//              one-way shortcut).
//   east wing  going UP: a hall, a ramp up to an upper room 6 m above the hub floor, east of the terrace and seen from it,
//              and the top of the east wall of the courtyard - the wall that closed the courtyard is now a path. The seal
//              is on it; its north end drops onto the gallery of the hub, and the gallery drops into the hub (one way).
//   north wing going UP by two ramps to a landing 7 m above the hub floor, with the seal; a ledge runs south through a window
//              in the hub wall and drops into the hub (a one-way shortcut).
//   tower      the landmark of the map: 13 m high, gold pinnacles on its corners, the beacon over it. Inside, six ramps wind
//              round its walls to a balcony on its south side, 12 m above the hub, which looks back over the whole map; the
//              goal is there.
// Landmarks:  one per wing (an archive of shelves, a turning water ring, a bell tower), outside the walls and drawn only.
// Ring:       (step 3) the districts are joined without the hub, one way in each layer. B: a passage under the ledge from the
//             west hall to the north corridor, with a breach at each end (in A both are shut). A: a sky bridge from landing 1
//             of the north hall east over the city to a lookout, and stairs down into the east hall (in B the middle of the
//             bridge is gone). A lift in the north hall goes from its floor to the landing of the seal once that seal is taken.
// Vocabulary: pieces of scene/Shapes.h used in the map so far: the stairs over the ramp down of the west wing (the ramp is
//             walked, the steps are drawn), the water of layer A in the trench of the west hall (in B it has gone), an arcade
//             along the west wall of the hub, roofs over the corridors of the west and east wings. Drawn-only pieces (steps,
//             water, roofs) are kDecor; the arcade is solid and part of kStageStatics.
// Heights:    every wall that a body must not climb is out of reach of a jump (2 m) from every place a body can stand next
//             to it: the hub walls (9 m) from the ledge (6 m), the tower walls (12.2 m) from the ledge, the courtyard walls
//             (5 m) from the terrace (1.4 m) and from the tops of the puzzle walls (2.5 m: they can be stood on from the
//             strip), the parapets (2.6 m) from the floors they close. tests/test_reach.cpp searches the whole map for a jump
//             that ends higher in another room; tests/test_stage.cpp checks the numbers and walks the map.
// Rooms:      kRooms says where each room is and where a body comes back in it after a fall or R. R also puts back the
//             puzzle of a room that has one (docs/LEVEL_DESIGN.md, section 9).
// Ramps:      boxes turned round X (SceneObject::tilt); they run along Z, at most 27 degrees (the body stands up to about
//             53). Every other box is axis-aligned.
// Tables:     kStageStatics are the fixed objects (the puzzle copies them). The DRAWN table adds the block, the closed
//             courtyard door, the plate, the goal light and the state of the map (seals, seal lamps, closed doors:
//             AddMapDraw); the SOLID table adds the block, the closed courtyard door (Puzzle::Solids) and the closed doors of
//             the map (AddMapSolids). Plates, seals, lamps and lights are never solid.
// Space:      left-handed, +Y up, the hub floor at scene::kFloorY (-1). UNITS: metres.
// Proof:      tests/test_stage.cpp walks the whole map with the real walking body: every room reached, every shortcut
//             taken, every closed door holding, every ramp climbed, every room entry standable in both layers, and the jumps
//             that would skip a part tried.
// Note:       no std::min / std::max here: windows.h may define them as macros (docs/CONVENTIONS.md).
#pragma once

#include "scene/Camera.h" // Vec3
#include "scene/Courtyard.h"
#include "scene/Puzzle.h"
#include "scene/SceneData.h"
#include "scene/Shapes.h"
#include <array>
#include <cmath>
#include <cstddef>
#include <vector>
namespace dl::stage
{

using scene::kFloorY;
using scene::kLayerA;
using scene::kLayerB;
using scene::kLayerBoth;
using scene::SceneObject;
using scene::ShapeKind;

// ---- colours (sRGB): red = layer A, blue = layer B, grey = both, gold = goal and seals
struct Rgb
{
    float r;
    float g;
    float b;
};
inline constexpr Rgb kGrey{0.30f, 0.33f, 0.40f};
inline constexpr Rgb kStone{0.52f, 0.54f, 0.58f};
inline constexpr Rgb kDark{0.40f, 0.42f, 0.47f};
inline constexpr Rgb kTowerStone{0.62f, 0.60f, 0.56f};
inline constexpr Rgb kRed{0.82f, 0.24f, 0.22f};
inline constexpr Rgb kBlue{0.22f, 0.46f, 0.90f};
inline constexpr Rgb kGold{1.00f, 0.82f, 0.36f};

/// A box from its two corners (min, max), a colour (sRGB) and a layer mask. checker = 1 for the floors.
constexpr SceneObject Span(float x0, float y0, float z0, float x1, float y1, float z1, float r, float g, float b, unsigned mask,
                           float checker = 0.0f)
{
    SceneObject o{};
    o.shape = ShapeKind::Cube;
    o.position[0] = 0.5f * (x0 + x1);
    o.position[1] = 0.5f * (y0 + y1);
    o.position[2] = 0.5f * (z0 + z1);
    o.scale[0] = x1 - x0;
    o.scale[1] = y1 - y0;
    o.scale[2] = z1 - z0;
    o.colorSrgb[0] = r;
    o.colorSrgb[1] = g;
    o.colorSrgb[2] = b;
    o.checker = checker;
    o.shineStrength = checker > 0.5f ? 0.04f : 0.20f;
    o.shininess = checker > 0.5f ? 16.0f : 40.0f;
    o.layerMask = mask;
    return o;
}

constexpr SceneObject Span(float x0, float y0, float z0, float x1, float y1, float z1, Rgb c, unsigned mask, float checker = 0.0f)
{
    return Span(x0, y0, z0, x1, y1, z1, c.r, c.g, c.b, mask, checker);
}

/// A floor slab of both layers whose TOP is at `top`.
constexpr SceneObject Floor(float x0, float z0, float x1, float z1, float top)
{
    return Span(x0, top - 0.2f, z0, x1, top, z1, kGrey, kLayerBoth, 1.0f);
}

/// A wall of both layers.
constexpr SceneObject Wall(float x0, float y0, float z0, float x1, float y1, float z1)
{
    return Span(x0, y0, z0, x1, y1, z1, kStone, kLayerBoth);
}

/// A wall of the tower (both layers, a lighter stone: the landmark).
constexpr SceneObject TowerWall(float x0, float y0, float z0, float x1, float y1, float z1)
{
    return Span(x0, y0, z0, x1, y1, z1, kTowerStone, kLayerBoth);
}

inline constexpr float kRampThickness = 0.2f;

/// A ramp of both layers between x0 and x1, whose top surface runs from (z0, y0) to (z1, y1) in a straight line. It is a box
/// turned round X: the top face of a box of size (x1 - x0, thickness, length), turned by -atan2(y1 - y0, z1 - z0), has
/// exactly that slope. NOT constexpr: it needs std::atan2 (the table that holds ramps is built at start-up, see kRamps).
inline SceneObject Ramp(float x0, float x1, float z0, float y0, float z1, float y1)
{
    if (z1 < z0) // the same ramp, described from its other end: the angle must stay within +-90 degrees, so that the top face
    {            // of the box is the one on the line and not its bottom face
        const float tz = z0;
        const float ty = y0;
        z0 = z1;
        y0 = y1;
        z1 = tz;
        y1 = ty;
    }
    const float dz = z1 - z0;
    const float dy = y1 - y0;
    const float length = std::sqrt(dz * dz + dy * dy);
    const float a = std::atan2(dy, dz);
    // after RotationX(-a), local +Y (the normal of the top face) is (0, cos a, -sin a) in the world
    const float ny = std::cos(a);
    const float nz = -std::sin(a);
    SceneObject o = Span(x0, 0.0f, 0.0f, x1, kRampThickness, length, kDark, kLayerBoth);
    o.tilt = -a;
    o.position[1] = 0.5f * (y0 + y1) - 0.5f * kRampThickness * ny;
    o.position[2] = 0.5f * (z0 + z1) - 0.5f * kRampThickness * nz;
    return o;
}

// ---- the measures (tests/test_stage.cpp checks what each one is for)
inline constexpr float kTerraceY = kFloorY + 2.4f; // the top of the raised terrace of the prologue
inline constexpr float kGateZ = 3.0f;              // the town wall across the terrace, 3.5 m high, 0.5 m thick
inline constexpr float kGateHeight = 3.5f;
inline constexpr float kGateHalf = 1.5f;           // its gate (A) and its breach (B), x -1.5 .. 1.5
inline constexpr float kTerraceEnd = 6.0f;         // the terrace ends here, the gap begins
inline constexpr float kStoneNear = 7.0f;          // the blue stone (B): 1 m from the terrace, 3 m deep, so that a jump
inline constexpr float kStoneFar = 10.0f;          // onto it needs no aim
inline constexpr float kStripNear = 11.5f;         // the red strip (A only): 1.5 m from the stone, 5.5 m from the terrace (more
                                                   // than a running jump, so A alone never crosses)
inline constexpr float kCurtainZ = 13.0f;          // a blue curtain (B) at the end of the strip: in B, a jump from the stone
                                                   // cannot fly over the strip into the courtyard
// the courtyard: grid cell (x, z) of scene/Courtyard.h is the world cell (x + kCellX0, z + kCellZ0), 1 m each
inline constexpr int kCellX0 = -3;
inline constexpr int kCellZ0 = 14;
inline constexpr float kCourtWest = static_cast<float>(kCellX0) - 0.5f;                         // -3.5
inline constexpr float kCourtEast = static_cast<float>(kCellX0 + courtyard::kWidth) - 0.5f;     // 3.5
inline constexpr float kCourtSouth = static_cast<float>(kCellZ0) - 0.5f;                        // 13.5: the strip ends here
inline constexpr float kCourtNorth = static_cast<float>(kCellZ0 + courtyard::kRowCount) - 0.5f; // 21.5
inline constexpr float kWallHeight = 3.5f;  // a wall of the puzzle: out of reach from the top of the block (1 m + a 2 m jump)
inline constexpr float kCityWallTop = 4.0f; // world y of the top of the walls of the wings
inline constexpr float kCourtWallTop = kFloorY + 6.0f; // 5: the walls round the courtyard; out of reach from the tops of the
                                                       // puzzle walls (a player can stand on those: 2.5 + a 2 m jump)
inline constexpr float kCurtainTop = kTerraceY + 6.4f; // 7.8: out of reach from the strip and from the top of the east wall
inline constexpr float kParapet = 2.6f;    // UNITS: metres: a parapet round a high floor, out of reach of a jump from it
inline constexpr float kHubWallTop = 9.0f;  // world y of the top of the hub walls: out of reach from the ledge (6 m + 2 m)
// the door from the courtyard into the hub (blue, layer B; a red panel fills it in A)
inline constexpr float kDoorX = 0.0f;
inline constexpr float kDoorwayHalfWidth = 0.7f;
inline constexpr float kDoorSouth = kCourtNorth;        // 21.5
inline constexpr float kDoorNorth = kCourtNorth + 0.5f; // 22.0
inline constexpr float kDoorwayLow = kFloorY;
inline constexpr float kDoorwayHigh = kFloorY + 2.2f;
// the hub
inline constexpr float kHubWest = -9.0f;
inline constexpr float kHubEast = 9.0f;
inline constexpr float kHubSouth = kDoorNorth; // 22
inline constexpr float kHubNorth = 40.0f;
inline constexpr float kOpeningTop = kFloorY + 2.4f; // the openings into the wings, 2.4 m high
inline constexpr float kWingDoorZ0 = 30.0f;          // the west and east openings, z 30 .. 32
inline constexpr float kWingDoorZ1 = 32.0f;
inline constexpr float kShortcutZ0 = 22.5f;          // the closed door at the end of the west shortcut, z 22.5 .. 24.5
inline constexpr float kShortcutZ1 = 24.5f;
// the wings
inline constexpr float kWestLowY = kFloorY - 3.6f;   // -4.6: the lower room of the west wing
inline constexpr float kTrenchSouth = 24.5f;         // W1: the trench across the hall, z 24.5 .. 29.5: wider than a running jump
inline constexpr float kTrenchNorth = 29.5f;
inline constexpr float kWestDoorSouth = 12.0f;       // W2: the door at the foot of the ramp down, z 12 .. 12.5
inline constexpr float kWestDoorNorth = 12.5f;
inline constexpr float kWestDoorTop = kWestLowY + 2.6f;
inline constexpr float kMezzY = kWestLowY + 3.0f;    // -1.6, W3: the mezzanine, 3 m over the lower room (out of reach of a jump)
inline constexpr float kMezzNorth = 5.0f;            // it covers x -24 .. -14, z 2 .. 5
inline constexpr float kMezzEast = -14.0f;
inline constexpr float kHoleX = -20.0f;              // the hole in it, over the socket in the vault
inline constexpr float kHoleZ = 3.5f;
inline constexpr float kHoleHalf = 0.3f;             // UNITS: metres: 0.6 m across, narrower than the body (2 x 0.35 m)
inline constexpr float kVaultEast = -18.0f;          // W4: the vault under the mezzanine, x -24 .. -18, z 2 .. 5
inline constexpr float kEastHighY = kCourtWallTop;  // 5: the upper room of the east wing = the top of the east wall of the courtyard
inline constexpr float kEastRoomNorth = 10.0f;      // the upper room ends here; the ramp up runs from z 22 to here (6 m over 12 m)
inline constexpr float kGalleryY = kTerraceY;       // 1.4: the gallery in the hub, 2.4 m over its floor (out of reach from it)
inline constexpr float kNorthHighY = kFloorY + 7.0f; // 6: the landing of the north wing and its ledge
inline constexpr float kNorthWallTop = 9.0f;         // the walls of the north hall: out of reach from the landing
inline constexpr float kLedgeWest = -9.0f;           // the ledge, x -9 .. -5, z 37 .. 44, passes through a window of the hub
inline constexpr float kLedgeEast = -5.0f;           // north wall
inline constexpr float kLedgeSouth = 37.0f;
inline constexpr float kWindowTop = kNorthHighY + 2.5f; // the top of that window
// the ring (step 3): the districts are joined without the hub, one way per layer
inline constexpr float kPassageSouth = 40.5f;        // B: the passage under the ledge, west hall -> north corridor, z 40.5 .. 43.5
inline constexpr float kPassageNorth = 43.5f;
inline constexpr float kWestBreachX0 = -21.0f;       // its breach in the north wall of the west hall
inline constexpr float kWestBreachX1 = -19.0f;
inline constexpr float kNorthBreachZ0 = 41.5f;       // its breach in the west wall of the north corridor
inline constexpr float kNorthBreachZ1 = 43.5f;
inline constexpr float kBridgeY = kFloorY + 3.5f;    // 2.5, A: the sky bridge from landing 1 of the north hall, east
inline constexpr float kBridgeSouth = 58.0f;         // z 58 .. 60, x 9 .. 18.5
inline constexpr float kBridgeNorth = 60.0f;
inline constexpr float kBridgeEast = 18.5f;
inline constexpr float kBridgeGapX0 = 11.25f;        // its middle (A only): 4.5 m, more than a running jump
inline constexpr float kBridgeGapX1 = 15.75f;
inline constexpr float kLookoutTop = kBridgeY + kParapet; // the parapets of the lookout and the stairs
inline constexpr float kStairsX0 = 19.0f;            // the stairs from the lookout (z 56) down into the east hall (z 42)
inline constexpr float kStairsX1 = 21.0f;
// the tower, in the middle of the hub
inline constexpr float kTowerWest = -4.0f;
inline constexpr float kTowerEast = 4.0f;
inline constexpr float kTowerSouth = 27.0f;
inline constexpr float kTowerNorth = 35.0f;
inline constexpr float kTowerWall = 0.5f;
inline constexpr float kTowerDoorX0 = -3.2f; // its door, in the south wall, in front of the first ramp
inline constexpr float kTowerDoorX1 = -1.8f;
inline constexpr float kRampRise = 2.0f;     // each of its six ramps rises 2 m over 4 m (27 degrees)
inline constexpr int kTowerRamps = 6;
inline constexpr float kSummitY = kFloorY + static_cast<float>(kTowerRamps) * kRampRise; // 11: the balcony at the top
inline constexpr float kTowerTop = kSummitY + 1.2f;                                       // 12.2: its parapet
inline constexpr float kPinnacleTop = kTowerTop + 2.0f;                                   // the gold pinnacles on the corners
inline constexpr float kFallY = kFloorY - 8.0f; // a body whose feet are this low has fallen (below the west wing)

/// The goal light, over the middle of the balcony at the top of the tower.
inline constexpr Vec3 kGoal{0.0f, kSummitY + 0.9f, 28.25f};
inline constexpr float kGoalRadius = 0.25f;

// ---- the walls of the courtyard, generated from the rows of scene/Courtyard.h: one box per run of the same wall in a column
struct Run
{
    int x;
    int z0;
    int z1;
    char kind; // 'R' or 'B'
};

constexpr char RowCell(int gx, int gz)
{
    return courtyard::kRows[courtyard::kRowCount - 1 - gz][gx]; // the first row is the north one
}

constexpr bool IsWall(char c)
{
    return c == 'R' || c == 'B';
}

/// Every row of scene/Courtyard.h is kWidth cells long (RowCell reads them without a bound).
constexpr bool RowsAreWellFormed()
{
    for (const char* row : courtyard::kRows)
    {
        int n = 0;
        while (row[n] != '\0')
        {
            ++n;
        }
        if (n != courtyard::kWidth)
        {
            return false;
        }
    }
    return true;
}
static_assert(RowsAreWellFormed(), "scene/Courtyard.h: every row must be kWidth cells long");
static_assert(puzzle::Puzzle::kBlockSize == 1.0f, "the cells of the courtyard are 1 m: the block must be as big as a cell");

constexpr int CountRuns()
{
    int n = 0;
    for (int x = 0; x < courtyard::kWidth; ++x)
    {
        for (int z = 0; z < courtyard::kRowCount; ++z)
        {
            const char c = RowCell(x, z);
            if (IsWall(c) && (z == 0 || RowCell(x, z - 1) != c))
            {
                ++n;
            }
        }
    }
    return n;
}

inline constexpr int kRunCount = CountRuns();

constexpr std::array<Run, kRunCount> FindRuns()
{
    std::array<Run, kRunCount> runs{};
    int n = 0;
    for (int x = 0; x < courtyard::kWidth; ++x)
    {
        for (int z = 0; z < courtyard::kRowCount; ++z)
        {
            const char c = RowCell(x, z);
            if (IsWall(c) && (z == 0 || RowCell(x, z - 1) != c))
            {
                int end = z;
                while (end + 1 < courtyard::kRowCount && RowCell(x, end + 1) == c)
                {
                    ++end;
                }
                runs[static_cast<std::size_t>(n++)] = Run{x, z, end, c};
            }
        }
    }
    return runs;
}

inline constexpr std::array<Run, kRunCount> kRuns = FindRuns();

constexpr SceneObject WallOf(const Run& r)
{
    const float x0 = static_cast<float>(r.x + kCellX0) - 0.5f;
    const float z0 = static_cast<float>(r.z0 + kCellZ0) - 0.5f;
    const float z1 = static_cast<float>(r.z1 + kCellZ0) + 0.5f;
    return Span(x0, kFloorY, z0, x0 + 1.0f, kFloorY + kWallHeight, z1, r.kind == 'R' ? kRed : kBlue, r.kind == 'R' ? kLayerA : kLayerB);
}

// ---- the inside of the tower: three lanes along x, the ramps run between z 29 and z 33, the landings north and south of them.
// Level k (0 .. 6) is at y = kFloorY + 2k. Ramps 1 .. 3 wind up lanes 1, 2, 3; ramps 4 .. 6 wind up the same lanes again, 6 m
// higher (a body on a ramp has at least 3.8 m over its head).
inline constexpr float kTowerInnerWest = kTowerWest + kTowerWall;   // -3.5
inline constexpr float kTowerInnerEast = kTowerEast - kTowerWall;   // 3.5
inline constexpr float kTowerInnerSouth = kTowerSouth + kTowerWall; // 27.5
inline constexpr float kTowerInnerNorth = kTowerNorth - kTowerWall; // 34.5
inline constexpr float kLane1 = kTowerInnerWest + (kTowerInnerEast - kTowerInnerWest) / 3.0f;       // -1.17
inline constexpr float kLane2 = kTowerInnerWest + 2.0f * (kTowerInnerEast - kTowerInnerWest) / 3.0f; // 1.17
inline constexpr float kRampSouth = 29.0f;
inline constexpr float kRampNorth = 33.0f;

constexpr float Level(int k)
{
    return kFloorY + static_cast<float>(k) * kRampRise;
}

inline constexpr SceneObject kFixed[] = {
    // ---- the prologue: the terrace, the gate (A, shut), the stone (B), the strip (A), the curtain (B); the town wall and the
    // rubble of its breach (B) follow the curtain
    Span(-3.0f, kTerraceY - 0.2f, -4.0f, 3.0f, kTerraceY, kTerraceEnd, kGrey, kLayerBoth, 1.0f),
    Span(-kGateHalf, kTerraceY, kGateZ - 0.25f, kGateHalf, kTerraceY + kGateHeight, kGateZ + 0.25f, kRed, kLayerA),
    Span(-1.5f, kTerraceY - 0.2f, kStoneNear, 1.5f, kTerraceY, kStoneFar, kBlue, kLayerB),
    Span(-3.0f, kTerraceY - 0.2f, kStripNear, 3.0f, kTerraceY, kCourtSouth, kRed, kLayerA, 1.0f),
    Span(-3.5f, kTerraceY - 0.2f, kCurtainZ, 3.5f, kCurtainTop, kCourtSouth, kBlue, kLayerB),
    Wall(-3.0f, kTerraceY, kGateZ - 0.25f, -kGateHalf, kTerraceY + kGateHeight, kGateZ + 0.25f), // the town wall, both layers
    Wall(kGateHalf, kTerraceY, kGateZ - 0.25f, 3.0f, kTerraceY + kGateHeight, kGateZ + 0.25f),
    Span(-kGateHalf, kTerraceY, kGateZ - 0.6f, -kGateHalf + 0.3f, kTerraceY + 0.4f, kGateZ + 0.6f, kDark, kLayerB), // the rubble of
    Span(kGateHalf - 0.3f, kTerraceY, kGateZ - 0.6f, kGateHalf, kTerraceY + 0.4f, kGateZ + 0.6f, kDark, kLayerB),   // the breach (B)
    // ---- the courtyard: its floor, the wall under the strip, its west wall, and its east wall, 1.5 m thick: the path of
    // the east wing runs on its top from the upper room (z 10) to its north end over the gallery of the hub (z 23)
    Floor(kCourtWest, kCourtSouth, kCourtEast, kCourtNorth, kFloorY),
    Wall(kCourtWest, kFloorY, kCourtSouth - 0.5f, kCourtEast, kTerraceY - 0.2f, kCourtSouth),
    Wall(kCourtWest - 0.5f, kFloorY, kCourtSouth - 0.5f, kCourtWest, kCourtWallTop, kDoorNorth),
    Span(kCourtEast, kFloorY, kEastRoomNorth, kCourtEast + 1.5f, kCourtWallTop, 23.0f, kStone, kLayerBoth),
    // its north wall with the doorway into the hub; the red panel fills the doorway in A (the blue door, in B, is kDoor)
    Wall(kCourtWest, kFloorY, kDoorSouth, kDoorX - kDoorwayHalfWidth, kCourtWallTop, kDoorNorth),
    Wall(kDoorX + kDoorwayHalfWidth, kFloorY, kDoorSouth, kCourtEast, kCourtWallTop, kDoorNorth),
    Wall(kDoorX - kDoorwayHalfWidth, kDoorwayHigh, kDoorSouth, kDoorX + kDoorwayHalfWidth, kCourtWallTop, kDoorNorth),
    Span(kDoorX - kDoorwayHalfWidth, kDoorwayLow, kDoorSouth, kDoorX + kDoorwayHalfWidth, kDoorwayHigh, kDoorNorth, kRed, kLayerA),
    // ---- the hub: its floor and its walls, 10 m high (openings: the wings west, east and north; the closed shortcut door
    // west; the window of the ledge north)
    Floor(kHubWest, kHubSouth, kHubEast, kHubNorth, kFloorY),
    Wall(kHubWest - 0.5f, kFloorY, kDoorSouth, kCourtWest - 0.5f, kHubWallTop, kHubSouth),            // south, west part
    Wall(kCourtEast + 1.5f, kFloorY, kDoorSouth, kHubEast + 0.5f, kHubWallTop, kHubSouth),            // south, east part
    Wall(kHubWest - 0.5f, kFloorY, kHubSouth, kHubWest, kHubWallTop, kShortcutZ0),                    // west
    Wall(kHubWest - 0.5f, kFloorY, kShortcutZ1, kHubWest, kHubWallTop, kWingDoorZ0),
    Wall(kHubWest - 0.5f, kFloorY, kWingDoorZ1, kHubWest, kHubWallTop, kHubNorth + 0.5f),
    Wall(kHubWest - 0.5f, kOpeningTop, kShortcutZ0, kHubWest, kHubWallTop, kShortcutZ1),
    Wall(kHubWest - 0.5f, kOpeningTop, kWingDoorZ0, kHubWest, kHubWallTop, kWingDoorZ1),
    Wall(kHubEast, kFloorY, kHubSouth, kHubEast + 0.5f, kHubWallTop, kWingDoorZ0),                    // east
    Wall(kHubEast, kFloorY, kWingDoorZ1, kHubEast + 0.5f, kHubWallTop, kHubNorth + 0.5f),
    Wall(kHubEast, kOpeningTop, kWingDoorZ0, kHubEast + 0.5f, kHubWallTop, kWingDoorZ1),
    Wall(kHubWest, kFloorY, kHubNorth, -1.0f, kNorthHighY - 0.2f, kHubNorth + 0.5f),                  // north, under the window
    Wall(kLedgeEast, kNorthHighY - 0.2f, kHubNorth, -1.0f, kHubWallTop, kHubNorth + 0.5f),            // north, east of the window
    Wall(kHubWest, kWindowTop, kHubNorth, kLedgeEast, kHubWallTop, kHubNorth + 0.5f),                 // north, over the window
    Wall(1.0f, kFloorY, kHubNorth, kHubEast + 0.5f, kHubWallTop, kHubNorth + 0.5f),
    Wall(-1.0f, kOpeningTop, kHubNorth, 1.0f, kHubWallTop, kHubNorth + 0.5f),
    // ---- the gallery: a walk along the south-east corner of the hub, 2.4 m over its floor, reached only by the drop from the
    // north end of the east path; from it the hub is seen from above, and a drop leads down into it
    Floor(kCourtEast, 23.0f, kHubEast, 25.0f, kGalleryY),
    Floor(kHubEast - 2.0f, 25.0f, kHubEast, 34.0f, kGalleryY),
    // ---- the tower: its walls (the door in the south wall), the pinnacles, the landings of its ramps, the balcony
    TowerWall(kTowerWest, kFloorY, kTowerSouth, kTowerDoorX0, kTowerTop, kTowerInnerSouth),
    TowerWall(kTowerDoorX1, kFloorY, kTowerSouth, kTowerEast, kTowerTop, kTowerInnerSouth),
    TowerWall(kTowerDoorX0, kDoorwayHigh, kTowerSouth, kTowerDoorX1, kTowerTop, kTowerInnerSouth),
    TowerWall(kTowerWest, kFloorY, kTowerInnerNorth, kTowerEast, kTowerTop, kTowerNorth),
    TowerWall(kTowerWest, kFloorY, kTowerInnerSouth, kTowerInnerWest, kTowerTop, kTowerInnerNorth),
    TowerWall(kTowerInnerEast, kFloorY, kTowerInnerSouth, kTowerEast, kTowerTop, kTowerInnerNorth),
    Span(kTowerWest, kTowerTop, kTowerSouth, kTowerWest + 0.8f, kPinnacleTop, kTowerSouth + 0.8f, kGold, kLayerBoth),
    Span(kTowerEast - 0.8f, kTowerTop, kTowerSouth, kTowerEast, kPinnacleTop, kTowerSouth + 0.8f, kGold, kLayerBoth),
    Span(kTowerWest, kTowerTop, kTowerNorth - 0.8f, kTowerWest + 0.8f, kPinnacleTop, kTowerNorth, kGold, kLayerBoth),
    Span(kTowerEast - 0.8f, kTowerTop, kTowerNorth - 0.8f, kTowerEast, kPinnacleTop, kTowerNorth, kGold, kLayerBoth),
    Floor(kTowerInnerWest, kRampNorth, kLane2, kTowerInnerNorth, Level(1)),            // north, lanes 1-2
    Floor(kLane1, kTowerInnerSouth, kTowerInnerEast, kRampSouth, Level(2)),            // south, lanes 2-3
    Floor(kTowerInnerWest, kRampNorth, kTowerInnerEast, kTowerInnerNorth, Level(3)),   // north, lanes 1-3
    Floor(kTowerInnerWest, kTowerInnerSouth, kLane2, kRampSouth, Level(4)),            // south, lanes 1-2
    Floor(kLane1, kRampNorth, kTowerInnerEast, kTowerInnerNorth, Level(5)),            // north, lanes 2-3
    Span(kTowerInnerWest, kSummitY - 0.2f, kTowerInnerSouth, kTowerInnerEast, kSummitY, kRampSouth, kGold, kLayerBoth, 1.0f), // the balcony
    // ---- the west wing (DOWN), the archive (docs/LEVEL_DESIGN.md, 6.3). Its moving parts (orbs, sockets, the plate, the lever,
    // the bridge, the door, the bars) are in scene/WestWing.h. The corridor from the hub:
    Floor(-15.0f, kWingDoorZ0, kHubWest - 0.5f, kWingDoorZ1, kFloorY),
    Wall(-15.0f, kFloorY, kWingDoorZ0 - 0.5f, kHubWest - 0.5f, kCityWallTop, kWingDoorZ0),
    Wall(-15.0f, kFloorY, kWingDoorZ1, kHubWest - 0.5f, kCityWallTop, kWingDoorZ1 + 0.5f),
    // W1 and W2, the hall: the north floor (the orb of W1, the socket), the trench (the bridge of W1 spans it in B), the south
    // floor (the orb and the plate of W2, the top of the ramp down)
    Floor(-24.0f, kTrenchNorth, -15.0f, 40.0f, kFloorY),
    Floor(-24.0f, 22.0f, -15.0f, kTrenchSouth, kFloorY),
    Wall(-24.5f, kFloorY, 21.5f, -24.0f, kCityWallTop, 40.5f),
    Wall(-15.0f, kFloorY, 21.5f, -14.5f, kCityWallTop, kWingDoorZ0 - 0.5f),
    Wall(-15.0f, kFloorY, kWingDoorZ1 + 0.5f, -14.5f, kCityWallTop, 40.5f),
    Wall(-24.5f, kFloorY, 21.5f, -22.0f, kCityWallTop, 22.0f),
    Wall(-18.0f, kFloorY, 21.5f, -14.5f, kCityWallTop, 22.0f),
    // the ramp down (its sides as high as the walls round it: nothing is climbed from it)
    Wall(-22.5f, kWestLowY, kWestDoorNorth, -22.0f, kCityWallTop, 21.5f),
    Wall(-18.0f, kWestLowY, kWestDoorNorth, -17.5f, kCityWallTop, 21.5f),
    // W2's door at its foot: the red panel in A (the blue door, in B, is a gate of scene/WestWing.h); a lintel over it
    Span(-22.0f, kWestLowY, kWestDoorSouth, -18.0f, kWestDoorTop, kWestDoorNorth, kRed, kLayerA),
    Wall(-22.0f, kWestDoorTop, kWestDoorSouth, -18.0f, kCityWallTop, kWestDoorNorth),
    // W3, the lower room: its floor and its walls
    Floor(-24.0f, 2.0f, -10.0f, kWestDoorSouth, kWestLowY),
    Wall(-24.5f, kWestLowY, 1.5f, -24.0f, kCityWallTop, kWestDoorNorth),
    Wall(-24.5f, kWestLowY, 1.5f, -9.5f, kCityWallTop, 2.0f),
    Wall(-10.0f, kWestLowY, 2.0f, -9.5f, kCityWallTop, kWestDoorSouth),
    Wall(-24.0f, kWestLowY, kWestDoorSouth, -22.5f, kCityWallTop, kWestDoorNorth),                     // its north edge, beside
    Wall(-17.5f, kWestLowY, kWestDoorSouth, -12.5f, kCityWallTop, kWestDoorNorth),                     // and between the ramps
    // W3's mezzanine: a floor 3 m over the lower room, reached by a ramp along its east side; over the vault it has a hole
    // too narrow for the body (0.6 m; the body is 0.7 m across) and wide enough for an orb, open in B and closed by a red lid
    // in A
    Floor(-24.0f, 2.0f, kHoleX - kHoleHalf, kMezzNorth, kMezzY),
    Floor(kHoleX + kHoleHalf, 2.0f, kMezzEast, kMezzNorth, kMezzY),
    Floor(kHoleX - kHoleHalf, 2.0f, kHoleX + kHoleHalf, kHoleZ - kHoleHalf, kMezzY),
    Floor(kHoleX - kHoleHalf, kHoleZ + kHoleHalf, kHoleX + kHoleHalf, kMezzNorth, kMezzY),
    Span(kHoleX - kHoleHalf, kMezzY - 0.2f, kHoleZ - kHoleHalf, kHoleX + kHoleHalf, kMezzY, kHoleZ + kHoleHalf, kRed, kLayerA), // the lid (A)
    // W4, the vault under the mezzanine: the seal; its east wall (the bars on its north side are a gate of scene/WestWing.h)
    Wall(kVaultEast, kWestLowY, 2.0f, kVaultEast + 0.5f, kMezzY - 0.2f, kMezzNorth + 0.5f),
    // the shortcut up from the lower room to the door in the hub wall, which the seal opens
    Wall(-12.5f, kWestLowY, kWestDoorSouth, -12.0f, kCityWallTop, 25.5f),
    Wall(-10.0f, kWestLowY, kWestDoorSouth, -9.5f, kCityWallTop, 22.0f),
    Floor(-12.0f, 22.0f, kHubWest - 0.5f, 25.0f, kFloorY),                                            // the landing at its top
    Wall(-12.0f, kFloorY, 25.0f, kHubWest - 0.5f, kCityWallTop, 25.5f),
    // ---- the east wing (UP): the corridor, the hall, the walls of the ramp up, the upper room
    Floor(kHubEast + 0.5f, kWingDoorZ0, 15.0f, kWingDoorZ1, kFloorY),
    Wall(kHubEast + 0.5f, kFloorY, kWingDoorZ0 - 0.5f, 15.0f, kCityWallTop, kWingDoorZ0),
    Wall(kHubEast + 0.5f, kFloorY, kWingDoorZ1, 15.0f, kCityWallTop, kWingDoorZ1 + 0.5f),
    Floor(15.0f, 22.0f, 24.0f, 40.0f, kFloorY),
    Wall(24.0f, kFloorY, 21.5f, 24.5f, kCityWallTop, 40.5f),
    Wall(14.5f, kFloorY, 40.0f, kStairsX0, kCityWallTop, 40.5f),                                      // north, with the door of
    Wall(kStairsX1, kFloorY, 40.0f, 24.5f, kCityWallTop, 40.5f),                                      // the stairs from the lookout
    Wall(kStairsX0, kOpeningTop, 40.0f, kStairsX1, kCityWallTop, 40.5f),
    Wall(14.5f, kFloorY, 21.5f, 15.0f, kCityWallTop, kWingDoorZ0 - 0.5f),
    Wall(14.5f, kFloorY, kWingDoorZ1 + 0.5f, 15.0f, kCityWallTop, 40.5f),
    Wall(14.5f, kFloorY, 21.5f, 18.0f, kCityWallTop, 22.0f),
    Wall(22.0f, kFloorY, 21.5f, 24.5f, kCityWallTop, 22.0f),
    Wall(17.5f, kFloorY, kEastRoomNorth, 18.0f, kEastHighY + kParapet, 21.5f),                        // the sides of the ramp up
    Wall(22.0f, kFloorY, kEastRoomNorth, 22.5f, kEastHighY + kParapet, 21.5f),
    Floor(kCourtEast, 2.0f, 24.0f, kEastRoomNorth, kEastHighY),                                       // the upper room
    Wall(kCourtEast, kEastHighY, 1.5f, 24.5f, kEastHighY + kParapet, 2.0f),                           // its parapet
    Wall(24.0f, kEastHighY, 2.0f, 24.5f, kEastHighY + kParapet, kEastRoomNorth),
    Wall(kCourtEast - 0.5f, kEastHighY, 1.5f, kCourtEast, kEastHighY + kParapet, kEastRoomNorth),
    Wall(kCourtEast + 1.5f, kEastHighY, kEastRoomNorth, 17.5f, kEastHighY + kParapet, kEastRoomNorth + 0.5f),
    Wall(22.5f, kEastHighY, kEastRoomNorth, 24.5f, kEastHighY + kParapet, kEastRoomNorth + 0.5f),
    // ---- the north wing (UP): the corridor, the hall, the two landings, the ledge through the window into the hub
    Floor(-1.0f, kHubNorth + 0.5f, 1.0f, 44.0f, kFloorY),
    Wall(-1.5f, kFloorY, kHubNorth + 0.5f, -1.0f, kCityWallTop, kNorthBreachZ0),                       // its west wall, with the
    Wall(-1.5f, kFloorY, kNorthBreachZ1, -1.0f, kCityWallTop, 44.0f),                                   // breach of the passage
    Span(-1.5f, kFloorY, kNorthBreachZ0, -1.0f, kCityWallTop, kNorthBreachZ1, kRed, kLayerA),           // (A: a panel)
    Span(-1.85f, kFloorY, kNorthBreachZ0, -0.65f, kFloorY + shapes::kRubble, kNorthBreachZ0 + 0.3f, kDark, kLayerB), // (B: rubble)
    Span(-1.85f, kFloorY, kNorthBreachZ1 - 0.3f, -0.65f, kFloorY + shapes::kRubble, kNorthBreachZ1, kDark, kLayerB),
    Wall(1.0f, kFloorY, kHubNorth + 0.5f, 1.5f, kCityWallTop, 44.0f),
    Floor(-9.0f, 44.0f, 9.0f, 60.0f, kFloorY),
    Wall(-9.5f, kFloorY, 43.5f, -9.0f, kNorthWallTop, 60.5f),
    Wall(9.0f, kFloorY, 43.5f, 9.5f, kNorthWallTop, kBridgeSouth),                                     // east, with the door of the
    Wall(9.0f, kFloorY, kBridgeSouth, 9.5f, kBridgeY - 0.2f, kBridgeNorth),                            // sky bridge from landing 1
    Wall(9.0f, kLookoutTop, kBridgeSouth, 9.5f, kNorthWallTop, kBridgeNorth),
    Wall(9.0f, kFloorY, kBridgeNorth, 9.5f, kNorthWallTop, 60.5f),
    Wall(-9.5f, kFloorY, 60.0f, 9.5f, kNorthWallTop, 60.5f),
    Wall(-9.0f, kFloorY, 43.5f, -1.5f, kCityWallTop, 44.0f),
    Wall(1.5f, kFloorY, 43.5f, 9.0f, kCityWallTop, 44.0f),
    Floor(-1.0f, 58.0f, 9.0f, 60.0f, kFloorY + 3.5f),                                                 // landing 1 (north)
    Floor(-9.0f, 44.0f, 4.0f, 46.0f, kNorthHighY),                                                    // landing 2 (south)
    Floor(kLedgeWest, kLedgeSouth, kLedgeEast, 44.0f, kNorthHighY),                                   // the ledge into the hub
    // ---- the ring, B: the passage under the ledge from the west hall to the north corridor (both of its ends are breaches:
    // closed in A, open in B; the breach in the west hall is a piece of scene/Shapes.h, kPieces)
    Floor(kWestBreachX0, kPassageSouth, -1.5f, kPassageNorth, kFloorY),
    Wall(kWestBreachX0 - 0.5f, kFloorY, kPassageSouth, kWestBreachX0, kCityWallTop, kPassageNorth),
    Wall(kWestBreachX0 - 0.5f, kFloorY, kPassageNorth, -9.0f, kCityWallTop, kPassageNorth + 0.5f),
    Wall(-14.5f, kFloorY, 40.0f, -9.5f, kCityWallTop, kPassageSouth),
    // ---- the ring, A: the sky bridge from landing 1 of the north hall, east over the city to a lookout, and the stairs down
    // into the east hall. Its ends are in both layers, its middle and its rails only in A: in B the bridge is broken.
    Span(9.0f, kBridgeY - 0.3f, kBridgeSouth, kBridgeGapX0, kBridgeY, kBridgeNorth, kDark, kLayerBoth),
    Span(kBridgeGapX0, kBridgeY - 0.3f, kBridgeSouth, kBridgeGapX1, kBridgeY, kBridgeNorth, kRed, kLayerA),
    Span(kBridgeGapX1, kBridgeY - 0.3f, kBridgeSouth, kBridgeEast, kBridgeY, kBridgeNorth, kDark, kLayerBoth),
    Span(9.5f, kBridgeY, kBridgeSouth, kBridgeEast, kLookoutTop, kBridgeSouth + 0.15f, kRed, kLayerA),  // its parapets (A: in B they fell with it)
    Span(9.5f, kBridgeY, kBridgeNorth - 0.15f, kBridgeEast, kLookoutTop, kBridgeNorth, kRed, kLayerA),
    Floor(kBridgeEast, 56.0f, 21.5f, kBridgeNorth, kBridgeY),                                           // the lookout
    Wall(kBridgeEast - 0.5f, kBridgeY - 0.2f, 56.0f, kBridgeEast, kLookoutTop, kBridgeSouth),          // its parapets
    Wall(kBridgeEast - 0.5f, kBridgeY - 0.2f, kBridgeNorth, 22.0f, kLookoutTop, kBridgeNorth + 0.5f),
    Wall(21.5f, kBridgeY - 0.2f, 56.0f, 22.0f, kLookoutTop, kBridgeNorth),
    Wall(kBridgeEast, kFloorY, kPassageSouth, kStairsX0, kLookoutTop, 56.0f),                           // the walls of the stairs
    Wall(kStairsX1, kFloorY, kPassageSouth, 21.5f, kLookoutTop, 56.0f),
    Floor(kStairsX0, kPassageSouth, kStairsX1, 42.0f, kFloorY),                                         // their foot
};
inline constexpr int kFixedCount = static_cast<int>(sizeof(kFixed) / sizeof(kFixed[0]));

/// The ramps (not constexpr: Ramp needs std::atan2).
inline const SceneObject kRamps[] = {
    Ramp(kTowerInnerWest, kLane1, kRampSouth, Level(0), kRampNorth, Level(1)), // tower 1: lane 1 north
    Ramp(kLane1, kLane2, kRampNorth, Level(1), kRampSouth, Level(2)),          // tower 2: lane 2 south
    Ramp(kLane2, kTowerInnerEast, kRampSouth, Level(2), kRampNorth, Level(3)), // tower 3: lane 3 north
    Ramp(kTowerInnerWest, kLane1, kRampNorth, Level(3), kRampSouth, Level(4)), // tower 4: lane 1 south
    Ramp(kLane1, kLane2, kRampSouth, Level(4), kRampNorth, Level(5)),          // tower 5: lane 2 north
    Ramp(kLane2, kTowerInnerEast, kRampNorth, Level(5), kRampSouth, Level(6)), // tower 6: lane 3 south, to the balcony
    Ramp(-22.0f, -18.0f, 22.0f, kFloorY, 12.0f, kWestLowY),                    // west: down
    Ramp(-12.0f, -10.0f, 12.0f, kWestLowY, 22.0f, kFloorY),                    // west: the shortcut up
    Ramp(-16.0f, kMezzEast, 11.0f, kWestLowY, kMezzNorth, kMezzY),             // west: up to the mezzanine (W3)
    Ramp(18.0f, 22.0f, 22.0f, kFloorY, kEastRoomNorth, kEastHighY),            // east: up
    Ramp(5.0f, 9.0f, 46.0f, kFloorY, 58.0f, kFloorY + 3.5f),                   // north: 1
    Ramp(0.0f, 4.0f, 58.0f, kFloorY + 3.5f, 46.0f, kNorthHighY),               // north: 2
};
inline constexpr int kRampCount = static_cast<int>(sizeof(kRamps) / sizeof(kRamps[0]));
/// The arcade along the inside of the west wall of the hub: columns at z 26, 28 and 34, 36, 38 (the openings at z 22.5 .. 24.5
/// and 30 .. 32 stay free), lintels 2.6 m over the floor (out of reach of a jump from it).
inline constexpr int kArcadeCount = (2 + 1) + (3 + 1);
inline std::array<SceneObject, kArcadeCount> BuildArcade()
{
    std::vector<SceneObject> v;
    shapes::Arcade(kHubWest + 0.3f, kHubWest + 0.9f, 26.0f, 28.0f, kFloorY, 2.0f, 2.6f, v);
    shapes::Arcade(kHubWest + 0.3f, kHubWest + 0.9f, 34.0f, 38.0f, kFloorY, 2.0f, 2.6f, v);
    std::array<SceneObject, kArcadeCount> out{};
    for (std::size_t i = 0; i < out.size() && i < v.size(); ++i)
    {
        out[i] = v[i];
    }
    return out;
}
inline const std::array<SceneObject, kArcadeCount> kArcade = BuildArcade();

/// Pieces of scene/Shapes.h in the ring: the breach in the north wall of the west hall (5 boxes) and the slope of the stairs
/// from the lookout (1; its steps are drawn in kDecor).
inline constexpr int kPieceCount = 5 + 1;
inline std::array<SceneObject, kPieceCount> BuildPieces()
{
    std::vector<SceneObject> v;
    shapes::BreachWall(-24.5f, -14.5f, 40.25f, 0.5f, kFloorY, kCityWallTop - kFloorY, kWestBreachX0, kWestBreachX1, v);
    std::vector<SceneObject> steps;
    shapes::Stairs(kStairsX0, kStairsX1, 56.0f, kBridgeY, 42.0f, kFloorY, shapes::kStoneLight, v, steps);
    std::array<SceneObject, kPieceCount> out{};
    for (std::size_t i = 0; i < out.size() && i < v.size(); ++i)
    {
        out[i] = v[i];
    }
    return out;
}
inline const std::array<SceneObject, kPieceCount> kPieces = BuildPieces();

inline constexpr int kStageStaticCount = kFixedCount + kRampCount + kRunCount + kArcadeCount + kPieceCount;

/// Every box of the map in one table: the fixed boxes, the ramps, the walls of the courtyard (the indices are the ones that
/// Puzzle::LastBlocker names).
inline std::array<SceneObject, kStageStaticCount> BuildStatics()
{
    std::array<SceneObject, kStageStaticCount> all{};
    int n = 0;
    for (const SceneObject& o : kFixed)
    {
        all[static_cast<std::size_t>(n++)] = o;
    }
    for (const SceneObject& o : kRamps)
    {
        all[static_cast<std::size_t>(n++)] = o;
    }
    for (const Run& r : kRuns)
    {
        all[static_cast<std::size_t>(n++)] = WallOf(r);
    }
    for (const SceneObject& o : kArcade)
    {
        all[static_cast<std::size_t>(n++)] = o;
    }
    for (const SceneObject& o : kPieces)
    {
        all[static_cast<std::size_t>(n++)] = o;
    }
    return all;
}
inline const std::array<SceneObject, kStageStaticCount> kStageStatics = BuildStatics();
inline constexpr int kGateIndex = 1; ///< the gate (A) in kStageStatics (scene/StageLights.h hides a light in it)

/// The blue door (layer B) in the doorway from the courtyard into the hub.
inline constexpr SceneObject kDoor = Span(kDoorX - kDoorwayHalfWidth, kDoorwayLow, kDoorSouth, kDoorX + kDoorwayHalfWidth,
                                          kDoorwayHigh, kDoorNorth, kBlue, kLayerB);

// ---- the cells of the puzzle in the world
constexpr puzzle::Cell FindCell(char wanted)
{
    for (int z = 0; z < courtyard::kRowCount; ++z)
    {
        for (int x = 0; x < courtyard::kWidth; ++x)
        {
            if (RowCell(x, z) == wanted)
            {
                return {x + kCellX0, z + kCellZ0};
            }
        }
    }
    return {0, 0};
}

inline constexpr puzzle::Cell kPlateCell = FindCell('P');
inline constexpr puzzle::Cell kBlockStart = FindCell('K');

/// The puzzle of the courtyard: the block may use every cell of the grid.
inline puzzle::StageDesc MakeStage()
{
    puzzle::StageDesc d;
    d.statics = kStageStatics.data();
    d.staticCount = kStageStaticCount;
    d.blockStart = kBlockStart;
    d.plate = kPlateCell;
    d.door = kDoor;
    d.minCell = {kCellX0, kCellZ0};
    d.maxCell = {kCellX0 + courtyard::kWidth - 1, kCellZ0 + courtyard::kRowCount - 1};
    return d;
}

// ---- the rooms: where a body is, and where it comes back after a fall or R
struct Room
{
    const char* name;
    float x0, y0, z0; // the box in which the FEET of a body standing on the ground are "in" the room: its heights are kept
                      // close to the floors of the room, because tests/test_reach.cpp allows a jump up inside one room
    float x1, y1, z1;
    Vec3 entry;       // where it comes back, standing; a place to stand in both layers
    bool puzzle;      // R also puts back the puzzle of the room: the block of the courtyard, the orbs of the west wing
};

/// The rooms, the most specific first (the tower stands inside the hub).
inline constexpr Room kRooms[] = {
    {"tower", kTowerInnerWest, -2.0f, kTowerInnerSouth, kTowerInnerEast, 14.0f, kTowerInnerNorth, {-2.5f, kFloorY, 28.0f}, false},
    {"terrace", -3.2f, 0.0f, -4.0f, 3.2f, 6.0f, 4.2f, {0.0f, kTerraceY, -3.0f}, false},
    {"gap", -3.2f, 0.0f, 4.2f, 3.2f, 6.0f, kCourtSouth, {0.0f, kTerraceY, 5.0f}, false},
    {"courtyard", kCourtWest, -2.0f, kCourtSouth, kCourtEast, 3.0f, kCourtNorth, {kCourtWest + 0.5f, kFloorY, static_cast<float>(kCellZ0)}, true},
    {"hub", kHubWest, -2.0f, kHubSouth, kHubEast, -0.5f, kHubNorth, {0.0f, kFloorY, 23.0f}, false},
    {"west landing", -12.0f, -2.0f, 22.0f, kHubWest, -0.5f, 25.0f, {-11.0f, kFloorY, 23.5f}, false},
    {"west passage", kWestBreachX0, -2.0f, kPassageSouth, -1.5f, -0.5f, kPassageNorth, {-12.0f, kFloorY, 42.0f}, false},
    {"sky bridge", 9.0f, 1.5f, kBridgeSouth, kBridgeEast, 3.5f, kBridgeNorth, {10.3f, kBridgeY, 59.0f}, false},
    {"east stairs", kBridgeEast, -2.0f, kPassageSouth, 21.5f, 3.5f, kBridgeNorth, {20.0f, kBridgeY, 58.5f}, false},
    {"west hall", -24.0f, -2.0f, 22.0f, kHubWest, -0.5f, 40.0f, {-12.0f, kFloorY, 31.0f}, true},
    {"west vault", -24.0f, -6.0f, 2.0f, kVaultEast, -4.0f, kMezzNorth, {-21.5f, kWestLowY, 4.3f}, false},
    {"west mezzanine", -24.0f, -2.5f, 2.0f, kMezzEast, -1.0f, kMezzNorth, {-15.0f, kMezzY, 3.5f}, false},
    {"west lower", -24.0f, -6.0f, 2.0f, kHubWest, -4.0f, 22.0f, {-20.0f, kWestLowY, 10.0f}, true},
    {"east hall", kHubEast, -2.0f, 22.0f, 24.0f, -0.5f, 40.0f, {12.0f, kFloorY, 31.0f}, false},
    {"east upper", kCourtEast, 2.0f, 2.0f, 24.0f, 7.0f, 23.0f, {20.0f, kEastHighY, 6.0f}, false},
    {"north hall", -9.0f, -2.0f, kHubNorth, 9.0f, 8.0f, 60.0f, {0.0f, kFloorY, 42.0f}, false},
    {"gallery", kCourtEast, 0.5f, 23.0f, kHubEast, 2.5f, 34.0f, {8.0f, kGalleryY, 28.0f}, false},
};
inline constexpr int kRoomCount = static_cast<int>(sizeof(kRooms) / sizeof(kRooms[0]));

/// The index of the room called `name` in kRooms, or -1 (constexpr: a room that is renamed stops the build where it is used).
constexpr int RoomIndex(const char* name)
{
    for (int i = 0; i < kRoomCount; ++i)
    {
        const char* a = kRooms[i].name;
        const char* b = name;
        while (*a != '\0' && *a == *b)
        {
            ++a;
            ++b;
        }
        if (*a == *b)
        {
            return i;
        }
    }
    return -1;
}
inline constexpr int kStartRoom = 1; ///< the terrace

/// The room a body standing at `feet` is in, or -1.
inline int RoomAt(const Vec3& feet)
{
    for (int i = 0; i < kRoomCount; ++i)
    {
        const Room& r = kRooms[i];
        if (feet.x >= r.x0 && feet.x <= r.x1 && feet.y >= r.y0 && feet.y <= r.y1 && feet.z >= r.z0 && feet.z <= r.z1)
        {
            return i;
        }
    }
    return -1;
}

/// The room a body is in now: the room it stands in, or the last one when it stands in none or is in the air.
inline int CurrentRoom(const Vec3& feet, bool grounded, int last)
{
    if (!grounded)
    {
        return last;
    }
    const int room = RoomAt(feet);
    return room >= 0 ? room : last;
}

// ---- the seals: one at the far end of each wing; the three of them open the tower
struct Seal
{
    Vec3 centre;
    const char* name; ///< for the log
};
inline constexpr Seal kSeals[] = {
    {{-23.0f, kWestLowY + 1.0f, 3.5f}, "west"},
    {{kCourtEast + 0.75f, kEastHighY + 1.0f, 20.0f}, "east"},
    {{-7.0f, kNorthHighY + 1.0f, 45.0f}, "north"},
};
inline constexpr int kSealCount = static_cast<int>(sizeof(kSeals) / sizeof(kSeals[0]));
inline constexpr int kWestSeal = 0; ///< it also opens the door at the end of the west shortcut
inline constexpr float kSealRadius = 0.35f;

/// Which seals are taken. A seal is taken when one of the three balls of the body touches it; it stays taken.
class SealSet
{
  public:
    /// `Body` needs Ball(i) for i = 0, 1, 2 and GetSettings().radius (scene/Player.h). Returns a seal taken now, or -1.
    template <class Body>
    int Collect(const Body& body)
    {
        const float reach = body.GetSettings().radius + kSealRadius;
        for (int i = 0; i < kSealCount; ++i)
        {
            if (m_taken[i])
            {
                continue;
            }
            for (int b = 0; b < 3; ++b)
            {
                const Vec3 p = body.Ball(b);
                const float dx = p.x - kSeals[i].centre.x;
                const float dy = p.y - kSeals[i].centre.y;
                const float dz = p.z - kSeals[i].centre.z;
                if (dx * dx + dy * dy + dz * dz < reach * reach)
                {
                    m_taken[i] = true;
                    ++m_count;
                    return i;
                }
            }
        }
        return -1;
    }

    bool Taken(int i) const
    {
        return i >= 0 && i < kSealCount && m_taken[i];
    }

    int Count() const
    {
        return m_count;
    }

    bool TowerOpen() const
    {
        return m_count == kSealCount;
    }

    bool ShortcutOpen() const
    {
        return m_taken[kWestSeal];
    }

  private:
    bool m_taken[kSealCount] = {};
    int m_count = 0;
};

/// The door of the tower (both layers) and the door at the end of the west shortcut (both layers), while they are closed.
inline constexpr SceneObject kTowerDoor = Span(kTowerDoorX0, kFloorY, kTowerSouth, kTowerDoorX1, kDoorwayHigh, kTowerInnerSouth, kGold.r * 0.6f,
                                               kGold.g * 0.6f, kGold.b * 0.6f, kLayerBoth);
inline constexpr SceneObject kShortcutDoor = Span(kHubWest - 0.5f, kFloorY, kShortcutZ0, kHubWest, kOpeningTop, kShortcutZ1, kDark, kLayerBoth);

/// Adds the closed doors of the map to a SOLID table. Returns the new count.
inline int AddMapSolids(const SealSet& seals, SceneObject* out, int count, int capacity)
{
    if (out == nullptr || count < 0)
    {
        return count;
    }
    if (!seals.TowerOpen() && count < capacity)
    {
        out[count++] = kTowerDoor;
    }
    if (!seals.ShortcutOpen() && count < capacity)
    {
        out[count++] = kShortcutDoor;
    }
    return count;
}

/// Adds the state of the map to a DRAWN table: the closed doors, the seals not yet taken, and the three seal lamps over the
/// door of the tower (gold when their seal is taken). Returns the new count.
inline int AddMapDraw(const SealSet& seals, SceneObject* out, int count, int capacity)
{
    count = AddMapSolids(seals, out, count, capacity);
    for (int i = 0; i < kSealCount && count < capacity; ++i)
    {
        if (seals.Taken(i))
        {
            continue;
        }
        const Vec3 c = kSeals[i].centre;
        SceneObject s = Span(c.x - 0.25f, c.y - 0.25f, c.z - 0.25f, c.x + 0.25f, c.y + 0.25f, c.z + 0.25f, kGold, kLayerBoth);
        s.spinSpeed = 1.2f; // it turns with the spin time of the renderer
        s.shineStrength = 1.0f;
        s.shininess = 100.0f;
        s.glow = 0.9f;
        out[count++] = s;
    }
    for (int i = 0; i < kSealCount && count < capacity; ++i)
    {
        const float x = kTowerDoorX0 + 0.2f + 0.5f * static_cast<float>(i);
        const Rgb c = seals.Taken(i) ? kGold : kDark;
        out[count] = Span(x - 0.15f, kDoorwayHigh + 0.3f, kTowerSouth - 0.1f, x + 0.15f, kDoorwayHigh + 0.6f, kTowerSouth, c, kLayerBoth);
        out[count++].glow = seals.Taken(i) ? 1.2f : 0.0f; // a lit lamp shines
    }
    return count;
}

/// True when the body touches the goal light, or stands on the balcony at the top of the tower. `Body` needs Ball(i), Feet(),
/// Grounded() and GetSettings().radius (scene/Player.h).
template <class Body>
bool ReachedGoal(const Body& body)
{
    const float reach = body.GetSettings().radius + kGoalRadius;
    for (int i = 0; i < 3; ++i)
    {
        const Vec3 p = body.Ball(i);
        const float dx = p.x - kGoal.x;
        const float dy = p.y - kGoal.y;
        const float dz = p.z - kGoal.z;
        if (dx * dx + dy * dy + dz * dz < reach * reach)
        {
            return true;
        }
    }
    const Vec3 f = body.Feet();
    return body.Grounded() && f.y > kSummitY - 0.2f && f.y < kSummitY + 0.5f && f.x > kTowerInnerWest && f.x < kTowerInnerEast &&
           f.z > kTowerInnerSouth && f.z < kRampSouth;
}

// ---- the landmarks: one per wing, tall enough to be seen over the hub walls, so that a player in the hub knows where each
// wing is before entering it (docs/LEVEL_DESIGN.md, section 9). Drawn only, never solid: each stands outside every room,
// behind walls that are out of reach (tests/test_hints.cpp checks both).
inline constexpr Rgb kArchive{0.36f, 0.29f, 0.25f};
constexpr SceneObject Landmark(ShapeKind shape, float x, float y, float z, float size, float tilt, float spin, Rgb c)
{
    SceneObject o = Span(x - 0.5f * size, y - 0.5f * size, z - 0.5f * size, x + 0.5f * size, y + 0.5f * size, z + 0.5f * size, c, kLayerBoth);
    o.shape = shape;
    o.tilt = tilt;
    o.spinSpeed = spin;
    o.shineStrength = 0.6f;
    o.shininess = 60.0f;
    return o;
}
inline constexpr SceneObject kLandmarks[] = {
    Span(-29.0f, kFloorY, 25.0f, -26.0f, 20.0f, 37.0f, kArchive, kLayerBoth),                       // west: the archive, a wall of shelves
    Span(-29.5f, 20.0f, 30.0f, -25.5f, 21.0f, 32.0f, kGold, kLayerBoth),                            // its gold crown
    Landmark(ShapeKind::Torus, 29.0f, 15.0f, 31.0f, 6.0f, 1.5708f, 0.15f, Rgb{0.70f, 0.86f, 0.95f}), // east: the water ring, turning
    Span(-2.5f, kFloorY, 63.0f, 2.5f, 21.0f, 67.0f, kStone, kLayerBoth),                            // north: the bell tower
    Landmark(ShapeKind::Sphere, 0.0f, 23.0f, 65.0f, 3.0f, 0.0f, 0.0f, kGold),                       // its bell
};
inline constexpr int kLandmarkCount = static_cast<int>(sizeof(kLandmarks) / sizeof(kLandmarks[0]));

// ---- what is drawn and never solid and never moves: the steps over the ramp down of the west wing, the water of the trench,
// the roofs over the wing corridors (scene/Shapes.h). Static: baked with kStageStatics (render/StaticBatch.h).
/// The water in the trench of the west hall (W1): layer A only. A body that goes under it comes back at the entry of the room;
/// an orb floats on it.
inline constexpr shapes::Water kWaters[] = {
    {-24.0f, kTrenchSouth, -15.0f, kTrenchNorth, kFloorY - 0.5f, 3.0f},
};
inline constexpr int kWaterCount = static_cast<int>(sizeof(kWaters) / sizeof(kWaters[0]));

inline std::vector<SceneObject> BuildDecor()
{
    std::vector<SceneObject> solid;
    std::vector<SceneObject> drawn;
    shapes::Stairs(-22.0f, -18.0f, 22.0f, kFloorY, 12.0f, kWestLowY, shapes::kStoneLight, solid, drawn); // the ramp itself is in kRamps
    shapes::Stairs(kStairsX0, kStairsX1, 56.0f, kBridgeY, 42.0f, kFloorY, shapes::kStoneLight, solid, drawn); // (the slope: kPieces)
    for (const shapes::Water& w : kWaters)
    {
        drawn.push_back(shapes::WaterBox(w));
    }
    shapes::Roof(-15.0f, kHubWest - 0.5f, kWingDoorZ0 - 0.5f, kWingDoorZ1 + 0.5f, kCityWallTop, kCityWallTop + 1.0f, drawn);
    shapes::Roof(kHubEast + 0.5f, 15.0f, kWingDoorZ0 - 0.5f, kWingDoorZ1 + 0.5f, kCityWallTop, kCityWallTop + 1.0f, drawn);
    return drawn;
}
inline const std::vector<SceneObject> kDecor = BuildDecor();

/// The static part of the drawn table: kStageStatics, then kDecor. BuildDrawTable writes them first, in this order.
inline int StaticDrawCount()
{
    return kStageStaticCount + static_cast<int>(kDecor.size());
}

/// True when a body whose feet are at `feet` in `layer` is under water.
inline bool Submerged(const Vec3& feet, int layer)
{
    for (const shapes::Water& w : kWaters)
    {
        if (shapes::Submerged(w, feet, layer))
        {
            return true;
        }
    }
    return false;
}

/// Adds the water surfaces (layer A) to a solid table for ORBS (they float; the body is never held by water). Returns the new
/// count.
inline int AddWaterForOrbs(SceneObject* out, int count, int capacity)
{
    for (int i = 0; i < kWaterCount && out != nullptr && count >= 0 && count < capacity; ++i)
    {
        out[count++] = shapes::WaterSurface(kWaters[i]);
    }
    return count;
}

// ---- the lifts: shortcuts, opened from their far end
/// The lift of the north hall: from its floor up to landing 2 and the seal (x -9 .. -7, z 46 .. 48). It starts once the
/// north seal is taken: the way up has been walked once, and from then on it is a shortcut.
struct LiftDesc
{
    float x0, x1, z0, z1, bottom, top;
    int openedBySeal; ///< the seal that opens it
};
inline constexpr LiftDesc kLifts[] = {
    {-9.0f, -7.0f, 46.0f, 48.0f, kFloorY, kNorthHighY, 2},
};
inline constexpr int kLiftCount = static_cast<int>(sizeof(kLifts) / sizeof(kLifts[0]));

inline std::array<shapes::Lift, kLiftCount> MakeLifts()
{
    std::array<shapes::Lift, kLiftCount> out{};
    for (int i = 0; i < kLiftCount; ++i)
    {
        const LiftDesc& d = kLifts[i];
        out[static_cast<std::size_t>(i)] = shapes::Lift(d.x0, d.x1, d.z0, d.z1, d.bottom, d.top);
    }
    return out;
}

/// Moves the lifts for one frame and carries a body that stands on one (call it before the body moves). `Body` needs Feet(),
/// Grounded() and Carry() (scene/Player.h); `opened(seal)` says whether a seal is taken.
template <class Body, class Opened>
void UpdateLifts(std::array<shapes::Lift, kLiftCount>& lifts, Body& body, Opened opened, float dt)
{
    for (int i = 0; i < kLiftCount; ++i)
    {
        shapes::Lift& lift = lifts[static_cast<std::size_t>(i)];
        const Vec3 f = body.Feet();
        const float moved = lift.Update(f, body.Grounded(), dt, opened(kLifts[i].openedBySeal));
        if (moved != 0.0f && lift.OnIt({f.x, f.y + moved, f.z}, body.Grounded()))
        {
            body.Carry({0.0f, moved, 0.0f});
        }
    }
}

/// Adds the platforms of the lifts to a solid or drawn table. Returns the new count.
inline int AddLifts(const std::array<shapes::Lift, kLiftCount>& lifts, SceneObject* out, int count, int capacity)
{
    for (const shapes::Lift& lift : lifts)
    {
        if (out != nullptr && count >= 0 && count < capacity)
        {
            out[count++] = lift.Platform();
        }
    }
    return count;
}

inline constexpr int kMaxDrawObjects = 400;

/// The table that is drawn: the fixed objects (with the same indices as in the puzzle), the block, the door while it is
/// closed for a body in B, the plate (brighter while pressed), the landmarks and, until it is taken, the goal light. A refused push is
/// shown by StageHints::AddBlockedFlash (the outline of what was in the way), not by drawing it in the wrong layer. Returns
/// how many objects were written.
inline int BuildDrawTable(const puzzle::Puzzle& puzzle, bool goalTaken, SceneObject (&out)[kMaxDrawObjects])
{
    int n = 0;
    for (const SceneObject& o : kStageStatics)
    {
        out[n++] = o;
    }
    for (const SceneObject& o : kDecor)
    {
        out[n++] = o;
    }
    SceneObject block = puzzle.BlockObject();
    block.colorSrgb[0] = 0.62f;
    block.colorSrgb[1] = 0.32f;
    block.colorSrgb[2] = 0.86f;
    block.shineStrength = 0.30f;
    block.shininess = 40.0f;
    out[n++] = block;
    if (!puzzle.DoorOpen(1))
    {
        out[n++] = kDoor;
    }
    const bool pressed = puzzle.PlatePressed(0) || puzzle.PlatePressed(1);
    const float px = static_cast<float>(kPlateCell.x);
    const float pz = static_cast<float>(kPlateCell.z);
    out[n++] = Span(px - 0.45f, kFloorY, pz - 0.45f, px + 0.45f, kFloorY + 0.04f, pz + 0.45f, pressed ? 1.0f : 0.70f,
                    pressed ? 0.55f : 0.18f, pressed ? 0.45f : 0.16f, kLayerA);
    for (const SceneObject& o : kLandmarks)
    {
        out[n++] = o;
    }
    if (!goalTaken)
    {
        SceneObject goal{};
        goal.shape = ShapeKind::Sphere;
        goal.position[0] = kGoal.x;
        goal.position[1] = kGoal.y;
        goal.position[2] = kGoal.z;
        goal.scale[0] = 2.0f * kGoalRadius;
        goal.scale[1] = 2.0f * kGoalRadius;
        goal.scale[2] = 2.0f * kGoalRadius;
        goal.spinSpeed = 1.0f; // it turns with the spin time of the renderer, so the highlight moves
        goal.colorSrgb[0] = 1.0f;
        goal.colorSrgb[1] = 0.86f;
        goal.colorSrgb[2] = 0.40f;
        goal.shineStrength = 1.0f;
        goal.shininess = 120.0f;
        goal.layerMask = kLayerBoth;
        goal.glow = 1.2f;
        out[n++] = goal;
    }
    return n;
}

/// What the collision and the puzzle can handle: every fixed box has sizes above 0 and a layer mask of A, B or both; the
/// map fits the puzzle. Checked when the program is compiled (the ramps are checked by tests/test_stage.cpp).
constexpr bool FixedIsSupported()
{
    for (const SceneObject& o : kFixed)
    {
        if (o.shape != ShapeKind::Cube || !(o.scale[0] > 0.0f && o.scale[1] > 0.0f && o.scale[2] > 0.0f) || o.layerMask == 0u ||
            (o.layerMask & ~kLayerBoth) != 0u)
        {
            return false;
        }
    }
    return kStageStaticCount <= puzzle::Puzzle::kMaxSolids - 2;
}
static_assert(FixedIsSupported(), "scene/Stage.h: every box must have sizes above 0 and a layer mask of A, B or both, and the map must fit Puzzle::kMaxSolids");
static_assert(kPlateCell.x != 0 || kPlateCell.z != 0, "scene/Courtyard.h: the rows must hold a plate 'P'");

} // namespace dl::stage



