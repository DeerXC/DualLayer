// Stage.h - the map (docs/LEVEL_DESIGN.md, step 1: the skeleton). A walled city in two layers round a hub, walked in about
//            ten minutes when its rooms are filled; in this step the wings are grey boxes with their shapes, heights and
//            ways in and out, so that the size and the flow of the whole map can be played before any room is built.
//   prologue   (docs/LEVEL_DESIGN.md, 6.1: seven beats) a guardhouse, the start, whose arch frames the cathedral; the first
//              anchor lamp; the town wall across the terrace: in A (the whole city) its iron gate is shut; in B (the city after
//              the storm) the wall is broken open where the gate was - "in A the structure is the way, in B the breach is";
//              a heap of fallen masonry that closes B where A is clear; a shallow ditch that teaches the switch in the air
//              where a miss is a walk back up its stairs; the gap (a wide blue stone in B, then a switch to A in
//              the air onto the red strip; a blue curtain at the end of the strip keeps B from flying over it), the overlook, and
//              the courtyard with the proved crate puzzle (scene/Courtyard.h), a cloister on flagstones. The crate on the plate
//              opens the door of B in the north wall of the courtyard: the way into the hub.
//   square     (the hub; step 3b) a sunken square, walled 10 m high. A rim at street level runs along its walls and holds
//              every opening; inside it a pool 1.5 m deep surrounds the cathedral: water in A (a wall: a body that goes in
//              comes back at the entry), dry in B, entered by two stairs at opposite corners. A causeway crosses the pool to
//              the portal of the cathedral, which carries three seal lamps and opens when the three seals of the wings are
//              taken. A gallery runs along the south-east corner (2.4 m over the rim, reached only from the east wing: the
//              square seen from above). Three openings lead to the wings; a fourth, closed door in the west wall is the far
//              end of a shortcut that is opened from the other side.
//   west wing  going DOWN, the archive: an orb that is carried between the layers (scene/WestWing.h). W1, the hall: a
//              trench, bridged in B by an orb on a socket of B. W2: an orb on a plate of A opens a door of B at the foot of the
//              ramp down; a lever inside keeps it open, so that the orb can be fetched. W3, the lower room: from a mezzanine,
//              the orb is let fall through a hole that is open only in B onto a socket in the vault below, which opens its
//              bars. W4, the vault: the seal, and a ramp up to the closed door in the hub wall, which the seal opens (a
//              one-way shortcut).
//   east wing  going UP: a hall cut by a canal (scene/Canal.h: in A the lock wheel raises the water and the crate on it
//              becomes the step onto the quay 1.2 m higher; in B the canal is dry; the weir lets the crate drift west under a
//              breach of B into the undercroft, which leads to the square), a ramp up from the quay to an upper room 6 m above the hub floor, east of the terrace and seen from it,
//              and the top of the east wall of the courtyard - the wall that closed the courtyard is now a path. The seal
//              is on it; its north end drops onto the gallery of the hub, and the gallery drops into the hub (one way).
//   north wing going UP by two ramps to a landing 7 m above the hub floor, cut by the gap of N1 (a bridge of A, a portcullis
//              that the prism moves into B: scene/CityParts.h), with the seal; a ledge runs south through a window
//              in the hub wall and drops into the hub (a one-way shortcut).
//   cathedral  (the tower) the landmark of the map: walls 13 m high on a plinth in the pool, gold pinnacles, buttresses, a rose
//              window over the portal and a spire on its north wall, the highest point of the map (28.5 m); the beacon over
//              it. Inside, six ramps wind round its walls to a balcony on its south side, 12 m above the rim, which looks back
//              over the whole map; the goal is there.
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
#include "scene/Canal.h"
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
// the materials of one world (docs/LEVEL_DESIGN.md, 14.3): what exists only in A is made by people and whole (wood, iron),
// what exists only in B was left by the storm (fallen masonry). Red and blue are only the thin edges of such objects
// (scene/StageHints.h, AddWorldEdges) and the outlines in the other world.
inline constexpr Rgb kWood{0.52f, 0.36f, 0.22f};
inline constexpr Rgb kIron{0.30f, 0.32f, 0.36f};
inline constexpr Rgb kDebris{0.34f, 0.32f, 0.30f};
inline constexpr Rgb kGuardStone{0.58f, 0.53f, 0.47f}; // the guardhouse of the prologue (no battlements: it has a roof)

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
inline constexpr float kTerraceY = kFloorY + 2.4f;
inline constexpr float kParapetHeightHarbour = 2.6f; // (kParapet, which is defined later: a parapet out of reach of a jump) // the top of the raised terrace of the prologue
// the prologue (docs/LEVEL_DESIGN.md, 14.6), from the south: the guardhouse (beat 0), the first anchor lamp (1), the town wall
// with its gate (2), the rubble of B (3), the shallow ditch (4), the gap (5), the courtyard (6)
inline constexpr float kTerraceSouth = -24.5f;     // the terrace runs north from here
inline constexpr float kGuardNorth = -19.0f;       // the guardhouse: x -3 .. 3, z -24.5 .. -19, its arch in the north wall
inline constexpr float kGuardWallTop = kTerraceY + 5.0f;
// beat 0, the harbour (docs/LEVEL_DESIGN.md 6.1): the game starts on the quay of the harbour under the town wall. A stair
// climbs from the quay between two harbour walls to a landing at the south door of the guardhouse, the sea gate of the town.
// The harbour is a basin of the sea (kPits) walled on three sides; the sea is at kSeaY (scene/Landscape.h, Stage kSeaY)
inline constexpr float kHarbourY = kFloorY - 6.5f;  // -7.5: the top of the quay, 1.5 m over the sea
inline constexpr float kQuayX0 = -14.0f;            // the quay, x -14 .. 14, z -56 .. -46
inline constexpr float kQuayX1 = 14.0f;
inline constexpr float kQuayZ0 = -56.0f;
inline constexpr float kQuayZ1 = -46.0f;
inline constexpr float kSeaStairHalf = 1.5f;        // the stair, x -1.5 .. 1.5, from the quay (z -46) to the landing (z -27.5)
inline constexpr float kSeaStairTop = -27.5f;       // 8.9 m over 18.5 m: 26 degrees
inline constexpr float kSlotWallTop = kTerraceY + kParapetHeightHarbour; // the harbour walls beside the stair
inline constexpr float kSouthDoorHalf = 0.7f;       // the south door of the guardhouse, x -0.7 .. 0.7
inline constexpr float kSouthDoorTop = kTerraceY + 2.4f;
inline constexpr float kHarbourX0 = -24.0f;         // the basin, x -24 .. 24, z -62 .. -24.5
inline constexpr float kHarbourX1 = 24.0f;
inline constexpr float kHarbourZ0 = -62.0f;
inline constexpr float kArchHalf = 0.9f;           // the arch, x -0.9 .. 0.9, 4 m high: it frames the cathedral and the beacon
inline constexpr float kArchTop = kTerraceY + 4.0f;
inline constexpr float kGateZ = -14.0f;            // the town wall across the terrace, 3.5 m high, 0.5 m thick
inline constexpr float kGateHeight = 3.5f;
inline constexpr float kGateHalf = 1.5f;           // its gate (A) and its breach (B), x -1.5 .. 1.5
inline constexpr float kRubbleSouth = -9.5f;       // beat 3: a heap of fallen masonry across the terrace in B, out of reach
inline constexpr float kRubbleNorth = -8.0f;
inline constexpr float kRubbleTop = kTerraceY + 2.6f;
inline constexpr float kDitchEdge = -5.5f;         // beat 4: the terrace ends; the ditch floor is 2.4 m below (kFloorY)
inline constexpr float kDitchStoneNear = -4.5f;    // the beam of B, 1 m from the edge (as at the gap)
inline constexpr float kDitchStoneFar = -1.5f;
inline constexpr float kDitchStripNear = 0.0f;     // the planks of A, 1.5 m beyond the beam, 5.5 m from the edge
inline constexpr float kDitchStripFar = 3.0f;      // the terrace goes on from here: 4.5 m from the beam (more than a jump)
inline constexpr float kDitchStairsX0 = 3.6f;      // the stairs from the ditch floor back up to the terrace, east of it,
inline constexpr float kDitchStairsX1 = 5.4f;      // from z -5.5 (the floor) south to z -10.5 (the terrace)
inline constexpr float kDitchStairsTop = -10.5f;
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
// N1, the prism (docs/LEVEL_DESIGN.md, 14.9): landing 2 is cut by a gap of 5 m (more than a running jump) between the top of
// ramp 2 and the seal. A bridge of A spans it; at its far end a portcullis of A, 2.6 m over the landing, closes it. In B there
// is no bridge. The prism on the east part moves the portcullis into B: then A walks over
inline constexpr float kPrismGapX0 = -6.0f;
inline constexpr float kPrismGapX1 = -1.0f;
inline constexpr float kPortcullisX0 = kPrismGapX0;  // the portcullis stands on the far end of the bridge, x -6 .. -5.5
inline constexpr float kPortcullisX1 = kPrismGapX0 + 0.5f;
inline constexpr float kPortcullisZ0 = 41.8f;        // it reaches far past the landing on both sides, so that no jump goes round it
inline constexpr float kPortcullisZ1 = 48.2f;
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
// the east district, the sample of step 4 (docs/LEVEL_DESIGN.md, 14.8): the hall is cut by a canal; its far side is a quay
// 1.2 m higher, the foot of the ramp up. In A the canal holds water and a crate floats on it; a lock wheel (A: iron
// machinery; in B it has rusted) raises the water, and the crate with it, until it is the step across. In B the canal is dry:
// the crate lies on its bed, and the bed is walked (stairs at its east end) to a hidden light (scene/Canal.h)
inline constexpr float kCanalSouth = 24.8f;          // the canal, x 15 .. 24, z 24.8 .. 29.6: 4.8 m, more than a running jump
inline constexpr float kCanalNorth = 29.6f;
inline constexpr float kCanalBed = kFloorY - 2.5f;   // -3.5
inline constexpr float kQuayY = kFloorY + 1.2f;      // 0.2: the quay south of the canal, the foot of the ramp up
inline constexpr float kCanalStairsX0 = 22.4f;       // the stairs out of the dry canal, up to the hall floor (B); the bed lies
inline constexpr float kCanalStairsFoot = 25.4f;     // in front of their foot, so that they are walked onto from the bed
// E2 and the undercroft (docs/LEVEL_DESIGN.md, 14.8): the weir on the quay lets the crate drift west to the end of the canal.
// In the west wall of the canal a breach of B (in A an iron panel) opens 2.5 m over the
// bed, out of reach from it: only the crate of B, drifted under it, is the step. Behind it the undercroft runs west under
// the gallery to the east rim of the square, where bars close it; a lever inside opens them for good (a shortcut)
inline constexpr float kBayZ0 = 26.4f;               // the breach and the undercroft, z 26.4 .. 28
inline constexpr float kBayZ1 = 28.0f;
inline constexpr float kUndercroftY = kFloorY;       // the floor of the undercroft and the sill of the breach: level with the
                                                     // rim of the square (a step up is a wall), 2.5 m over the bed
inline constexpr float kUndercroftTop = 1.0f;        // the top of the opening in the hub wall (under the gallery)
inline constexpr float kBreachTop = 2.2f;            // the top of the breach and of the passage: a body jumps in from the crate
inline constexpr float kCanalParapetX1 = 18.6f;      // the parapets on the west end of the hall floor and of the quay:
                                                     // no jump from them, even one that turns in the air, ends in the breach
// the lock walk (docs/LEVEL_DESIGN.md 6.4, E3 as built): from a door in the east wall of the east hall a walk climbs east beside
// three locks to a top terrace under the water tower. Four terraces joined by stairs that climb along X; south of them the
// basins, each 2.5 m deep, its water 0.6 m under its terrace in A, dry in B with a stair back up at its east end; between two
// basins a lock head, as high as the next terrace. High walls close the walk on the north, the east and the south
inline constexpr float kLockDoorZ0 = 33.6f;          // the door in the east wall of the east hall, 2.2 m high
inline constexpr float kLockDoorZ1 = 35.6f;
inline constexpr float kLockWalkZ0 = 31.5f;          // the walk, z 31.5 .. 36
inline constexpr float kLockWalkZ1 = 36.0f;
inline constexpr float kBasinZ0 = 25.0f;             // the basins, z 25 .. 31.5
inline constexpr float kLockWallTop = 8.0f;          // the walls round the walk and the basins
inline constexpr float kLockEast = 63.5f;            // the east wall of the top terrace
inline constexpr int kLockCount = 3;
/// Terrace k (0 .. 3): x from, x to, floor. Stair k climbs from terrace k to terrace k + 1 over the 4 m between them.
inline constexpr float kTerraceX0[4] = {24.5f, 36.0f, 46.0f, 56.0f};
inline constexpr float kTerraceX1[4] = {32.0f, 42.0f, 52.0f, kLockEast};
inline constexpr float kTerraceFloor[4] = {kFloorY, kFloorY + 2.0f, kFloorY + 4.0f, kFloorY + 6.0f};
inline constexpr float kBasinDepth = 2.5f;           // the bed of basin k: 2.5 m under terrace k
inline constexpr float kLockWaterBelow = 0.6f;       // its water (A): 0.6 m under terrace k
inline constexpr float kBasinStairWidth = 1.5f;      // the stair out of a dry basin, at its east end
// the tower lock and the water tower (docs/LEVEL_DESIGN.md 6.4.2, E3 and E4): south of the top terrace a fourth basin, 3 m deep,
// against the foot of the water tower. Its door is in the south wall, 0.6 m over the top terrace but 6 m from it and 2.4 m over
// the crate of B: only the crate of A, floating, under the door is a step to it. The crate is pushed along the dry bed in B
// (scene/LockCrate.h); in A it floats where B left it. Inside the tower hangs the dawn bell
inline constexpr float kTowerLockX0 = kTerraceX0[3];  // 56 .. 63.5, z 25 .. 31.5
inline constexpr float kTowerLockBed = kFloorY + 3.0f; // 2
inline constexpr float kTowerLockWater = kFloorY + 5.0f; // 4 (A)
inline constexpr float kWaterDoorX0 = 58.6f;         // the door, x 58.6 .. 60.4, sill 5.6, 2.4 m high
inline constexpr float kWaterDoorX1 = 60.4f;
inline constexpr float kTowerDoorSill = kFloorY + 6.6f;
inline constexpr float kTowerRoomX0 = 57.5f;          // the room of the dawn bell, x 57.5 .. 61.5, z 20 .. 24.5
inline constexpr float kTowerRoomX1 = 61.5f;
inline constexpr float kTowerRoomZ0 = 20.0f;
inline constexpr float kTowerRoomTop = 10.0f;         // its vault
// the market of the west and its well (docs/LEVEL_DESIGN.md 6.3.1): a lane from a door in the west wall of the west hall to a
// walled market square; in it the great well, a shaft 12 m across and 8 m deep, with a stair down its four walls. The south
// stretch of the stair is a wooden stair of A (in B it has fallen); the water of A stands 3 m deep in the shaft, over the west
// and north stretches. So: down in A to the corner over the water, then on in B to the dry bottom, where the day bell lies
inline constexpr float kLaneZ0 = 33.6f;              // the lane, z 33.6 .. 35.6, from the west hall (x -24.5) to the market (x -34)
inline constexpr float kLaneZ1 = 35.6f;
inline constexpr float kMarketX0 = -61.0f;           // the market, x -61 .. -34, z 30 .. 60
inline constexpr float kMarketX1 = -34.0f;
inline constexpr float kMarketZ0 = 30.0f;
inline constexpr float kMarketZ1 = 60.0f;
inline constexpr float kShaftX0 = -54.0f;            // the shaft of the well, x -54 .. -42, z 38 .. 50
inline constexpr float kShaftX1 = -42.0f;
inline constexpr float kShaftZ0 = 38.0f;
inline constexpr float kShaftZ1 = 50.0f;
inline constexpr float kShaftWall = 0.6f;            // its walls, 1 m over the square: a parapet
inline constexpr float kWellFloor = kFloorY - 8.0f;  // -9: the bottom
inline constexpr float kWellWater = kFloorY - 5.0f;  // -6: the water of A
inline constexpr float kLanding = 1.8f;              // the landings in the corners and the width of the stair
inline constexpr float kLandingNE = kFloorY;          // the heights of the landings: NE -1 (the way in), SE -3, SW -5, NW -7
inline constexpr float kLandingSE = kFloorY - 2.0f;
inline constexpr float kLandingSW = kFloorY - 4.0f;
inline constexpr float kLandingNW = kFloorY - 6.0f;
// the theatre of the north (docs/LEVEL_DESIGN.md 6.5.1): a lane from a door under landing 1 of the north hall climbs to the
// stage through the stage wall; seven rows of seats rise north from the orchestra to a promenade; behind it the back wall, and
// beyond a terrace. In B the upper rows of the west half and the west half of the promenade have fallen, and the back wall
// has a breach there (in A a wooden panel): from the promenade of A, a jump at the panel with a switch to B in the air carries
// the body through the breach onto the terrace (N3)
inline constexpr float kTheatreDoorX0 = 3.0f;        // the door in the north wall of the north hall and the lane, x 3 .. 5
inline constexpr float kTheatreDoorX1 = 5.0f;
inline constexpr float kStageWallZ = 66.0f;          // the stage wall, z 66 .. 66.6
inline constexpr float kStageFront = 70.6f;          // the stage, z 66.6 .. 70.6, 1 m over the orchestra
inline constexpr float kStageY = kFloorY + 1.0f;
inline constexpr float kRowsZ0 = 76.0f;              // row k: z 76 + 1.2 k .. 76 + 1.2 (k + 1), top kFloorY + 0.55 (k + 1)
inline constexpr float kRowDepth = 1.2f;
inline constexpr float kRowRise = 0.55f;
inline constexpr int kRowCount = 7;
inline constexpr float kPromenadeY = kFloorY + kRowRise * static_cast<float>(kRowCount); // 2.85
inline constexpr float kPromenadeZ0 = kRowsZ0 + kRowDepth * static_cast<float>(kRowCount); // 84.4
inline constexpr float kBackWallZ = 86.0f;           // the back wall, z 86 .. 86.5
inline constexpr float kTheatreX = 14.0f;            // the theatre, x -14 .. 14
inline constexpr float kTheatreWallTop = kFloorY + 7.0f;
inline constexpr float kFallenX = -2.0f;             // B: west of this the upper rows (4 .. 6) and the promenade have fallen
inline constexpr float kBreachX0 = -11.0f;           // the breach of B in the back wall, x -11 .. -9 (A: a wooden panel)
inline constexpr float kBreachX1 = -9.0f;
inline constexpr float kTerraceZ1 = 90.0f;           // the terrace behind it, x -13 .. -7, z 86.5 .. 90, at the promenade
// the graveyard (docs/LEVEL_DESIGN.md 6.5.2): a walled yard on the hill north of the theatre, level with its promenade,
// reached by a lane through the back wall at the east end of the promenade; rows of graves, the night bell tower in its
// north-east corner (the night bell hangs low in its open arch, and is rung by walking up to it), a lone grave in B
inline constexpr float kGraveY = kPromenadeY;         // 2.85: its floor
inline constexpr float kGraveX0 = -12.0f;            // the yard, x -12 .. 12, z 92 .. 104
inline constexpr float kGraveX1 = 12.0f;
inline constexpr float kGraveZ0 = 92.0f;
inline constexpr float kGraveZ1 = 104.0f;
inline constexpr float kGraveLaneX0 = 8.0f;          // the lane, x 8 .. 10, from the back wall (z 86.5) to the yard
inline constexpr float kGraveLaneX1 = 10.0f;
inline constexpr float kGraveWallTop = kGraveY + 3.2f; // its parapet: the hill is seen over it, not reached (also from a pedestal)
inline constexpr Vec3 kNightBell{10.5f, kGraveY, 99.4f}; ///< the place in front of the night bell
inline constexpr Vec3 kDayBell{-48.0f, kFloorY - 8.0f, 46.1f}; ///< the place before the day bell, on the floor of the well (B)
inline constexpr Vec3 kDawnBell{59.5f, kFloorY + 6.6f, 21.2f}; ///< the place under the dawn bell, in the water tower
inline constexpr float kBellReach = 1.0f;            ///< UNITS: metres from that place to ring it
inline constexpr float kDriftX = 16.0f;              // the centre of the crate once it has drifted under the breach
// the cellar (docs/LEVEL_DESIGN.md, 14.8): the fourth link of the ring, from the canal bed of the east district west under the
// courtyard to the lower room of the west wing. It carries the water of the canal: in A it is flooded and an iron sluice panel
// closes each end; in B it is dry. Its west end is a grate opened by a lever inside the lower room (a shortcut opened from the
// far side). It never lies under the gap, where a fall has to stay a fall. Legs: east x 19 .. 21 (z 14 .. 24.8, under the
// quay), middle z 14 .. 16 (x -8 .. 21, under the courtyard), south x -8 .. -6 (z 9 .. 16, a ramp down to the floor of the
// lower room), west z 9 .. 11 (x -9.5 .. -8, to the grate)
inline constexpr float kCellarY = kCanalBed;          // -3.5: the floor of the east and the middle legs, level with the canal bed
inline constexpr float kCellarCeiling = kFloorY - 0.3f; // -1.3: the underside of its ceiling, 2.2 m over its floor
inline constexpr float kCellarWater = kFloorY - 0.6f; // -1.6 (A): the canal at its high level, under the ceiling
inline constexpr float kCellarEastX0 = 19.0f;
inline constexpr float kCellarEastX1 = 21.0f;
inline constexpr float kCellarZ0 = 14.0f;              // the middle leg
inline constexpr float kCellarZ1 = 16.0f;
inline constexpr float kCellarSouthX0 = -8.0f;         // the south leg
inline constexpr float kCellarSouthX1 = -6.0f;
inline constexpr float kCellarWestZ0 = 9.0f;           // the west leg and the grate
inline constexpr float kCellarWestZ1 = 11.0f;
inline constexpr float kCellarGrateTop = kWestLowY + 2.2f; // -2.4: the top of the opening in the east wall of the lower room
// N2 (docs/LEVEL_DESIGN.md, 14.9): one prism moves two things. A ledge of B along the west wall of the north hall (top 2.4 m
// over the floor, out of reach) is closed by a cage of B; a step of A stands at its east edge. The prism moves the cage into A
// and the step into B at once: then B climbs the step and the ledge to a hidden light buried in the wall
inline constexpr float kN2LedgeX1 = -5.0f;             // the ledge, x -9 .. -5, z 56.6 .. 58.8
inline constexpr float kN2LedgeZ0 = 56.6f;
inline constexpr float kN2LedgeZ1 = 58.8f;
inline constexpr float kN2LedgeTop = kFloorY + 2.4f;   // 1.4
inline constexpr float kN2CageTop = kN2LedgeTop + 2.6f;
// the square (step 3b): the hub is a sunken square. A rim at the level of the streets (kFloorY) runs along its walls and
// holds every opening; inside it a pool 1.5 m deep surrounds the cathedral. In A the pool is full (water: a body that goes
// in comes back at the entry of the square); in B the storm has drained it and its floor can be walked. A causeway crosses
// the pool from the south rim to the portal of the cathedral; two stairs lead down into the pool (used in B).
inline constexpr float kSquareLowY = kFloorY - 1.5f; // -2.5: the floor of the pool; a jump (2 m) climbs back onto the rim,
                                                     // so a body in B is never trapped
inline constexpr float kPoolWest = -6.5f;            // the pool, x -6.5 .. 6.5, z 24.5 .. 37.5 (the rim is 2.5 m wide)
inline constexpr float kPoolEast = 6.5f;
inline constexpr float kPoolSouth = 24.5f;
inline constexpr float kPoolNorth = 37.5f;
inline constexpr float kCausewayX0 = -4.0f; // the causeway to the portal, x -4 .. -1 (the portal is x -3.2 .. -1.8)
inline constexpr float kCausewayX1 = -1.0f;
inline constexpr float kStairRun = 3.0f;    // the stairs into the pool: 1.5 m over 3 m (27 degrees), 1.5 m wide, so that the
inline constexpr float kStairWidth = 1.5f;  // pool beside them (1 m to the cathedral) is still walked round

// the tower (the cathedral), in the middle of the square
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
inline constexpr float kFallY = kFloorY - 8.0f; // an ORB whose centre is this low has fallen (a body: FallLine of its room)
inline constexpr float kFallBelowRoom = 6.0f;   // UNITS: metres: a body this far below the bottom of its room has fallen

// ---- the ground under the map (scene/Landscape.h, docs/LEVEL_DESIGN.md 5.4): the town is levelled at the street (kPlainY);
// where the map has a floor below the street, or no floor at all (a gap, a trench, the sky bridge, the open sides of the
// prologue), the ground drops into a pit, below every fall line, so that a fall stays a fall and nothing of the ground shows
// inside a room. tests/test_terrain.cpp proves both for every room.
inline constexpr float kPlainY = kFloorY - 0.05f; ///< the levelled ground of the town, just under the street floors
inline constexpr float kPitY = kFloorY - 15.0f;   ///< -16: the floor of a pit, below every fall line (the lowest is -12)
inline constexpr float kSeaY = kFloorY - 8.0f;    ///< -9: the sea; it fills the pits that open to it
inline constexpr float kWellX = -48.0f; ///< the great well of the market
inline constexpr float kWellZ = 44.0f;
inline constexpr float kWellR = 6.0f;
struct Pit
{
    float x0, z0, x1, z1;
    const char* name;
};
inline constexpr Pit kPits[] = {
    {kHarbourX0, kHarbourZ0, kHarbourX1, kTerraceSouth, "the harbour"},  // the basin of the sea under the town wall
    {-10.0f, -30.0f, 12.0f, 12.0f, "the ravine of the prologue"}, // the terrace, the ditch, the gap: open sides
    {9.6f, 55.0f, 18.4f, 63.0f, "under the sky bridge"},           // broken in B, no parapets in B
    {-6.5f, 24.5f, 6.5f, 37.5f, "under the pool"},
    {-24.5f, 1.5f, -9.5f, 22.0f, "under the lower west"},           // the lower room, the vault, the mezzanine, the ramps
    {-24.0f, 24.5f, -15.0f, 29.5f, "the trench"},
    {-9.6f, 8.8f, 21.2f, 16.2f, "under the cellar"},
    {14.8f, 13.8f, 24.2f, 29.8f, "under the canal"},
    {24.0f, 24.5f, 64.0f, 31.6f, "the locks"},                        // the basins of the lock walk
    {kWellX - kWellR, kWellZ - kWellR, kWellX + kWellR, kWellZ + kWellR, "the great well"}, // the shaft of the market (drawn)                // the canal, its bed, the east leg of the cellar
    {5.0f, 10.0f, 17.5f, 21.5f, "east of the east path"},          // the path on the courtyard wall is open to the east
    {9.5f, 22.0f, 14.5f, 26.4f, "behind the east wall of the square"}, // a closed space between the square and the canal
    {1.5f, 40.5f, 9.5f, 43.5f, "between the square and the north hall"}, // under the south edge of landing 2
    {-24.0f, 44.0f, -9.5f, 61.0f, "west of the north hall"},       // over the wall of the passage from the ledge
};
inline constexpr int kPitCount = static_cast<int>(sizeof(kPits) / sizeof(kPits[0]));
inline constexpr float kPitReach = 4.0f; ///< UNITS: metres round a pit in which the ground slopes into it (two cells of the fine field)

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
    // A: a stack of crates and casks of the warehouse (made by people, whole); B: fallen masonry (CourtyardDecor draws their look)
    return Span(x0, kFloorY, z0, x0 + 1.0f, kFloorY + kWallHeight, z1, r.kind == 'R' ? kWood : kDebris, r.kind == 'R' ? kLayerA : kLayerB);
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

/// Row k of the seats of the theatre, its east part (x kFallenX .. 12, both layers) or its west part (x -12 .. kFallenX; rows
/// 4 .. 6 A only: fallen in B). Solid down to the floor.
constexpr SceneObject TheatreRow(int k, bool east)
{
    const float z0 = kRowsZ0 + kRowDepth * static_cast<float>(k);
    const float top = kFloorY + kRowRise * static_cast<float>(k + 1);
    return Span(east ? kFallenX : -12.0f, kFloorY, z0, east ? 12.0f : kFallenX, top, z0 + kRowDepth, Rgb{0.74f, 0.68f, 0.58f},
                (!east && k >= 4) ? kLayerA : kLayerBoth);
}

/// A stall of the market: a stone counter (both layers) over its footprint, 0.95 m high.
constexpr SceneObject MarketStall(float x, float z, bool alongZ, int)
{
    const float hx = alongZ ? 0.9f : 1.6f;
    const float hz = alongZ ? 1.6f : 0.9f;
    return Span(x - hx, kFloorY, z - hz, x + hx, kFloorY + 0.95f, z + hz, Rgb{0.66f, 0.62f, 0.55f}, kLayerBoth);
}
/// Its awning (A): cloth on a frame, 2.4 m over the square, a little larger than the counter; red or gold in turn.
constexpr SceneObject MarketAwning(float x, float z, bool alongZ, int k)
{
    const float hx = (alongZ ? 0.9f : 1.6f) + 0.2f;
    const float hz = (alongZ ? 1.6f : 0.9f) + 0.2f;
    return Span(x - hx, kFloorY + 2.4f, z - hz, x + hx, kFloorY + 2.5f, z + hz, k % 2 == 0 ? Rgb{0.78f, 0.28f, 0.22f} : Rgb{0.88f, 0.72f, 0.34f},
                kLayerA);
}
/// A rotten crate beside a stall (B), out of the way along the wall.
constexpr SceneObject MarketCrate(float x, float z)
{
    const float dz = z < 45.0f ? -1.5f : 1.5f;
    const float dx = x < -59.0f ? -0.6f : 1.9f;
    return Span(x + dx - 0.35f, kFloorY, z + dz - 0.35f, x + dx + 0.35f, kFloorY + 0.5f, z + dz + 0.35f, Rgb{0.30f, 0.25f, 0.21f}, kLayerB);
}

inline constexpr SceneObject kFixed[] = {
    // ---- the prologue (docs/LEVEL_DESIGN.md, 14.6). Index 0 is the terrace, index 1 the gate (kGateIndex)
    Span(-3.0f, kTerraceY - 0.2f, kTerraceSouth, 3.0f, kTerraceY, kDitchEdge, kGrey, kLayerBoth, 1.0f),
    Span(-kGateHalf, kTerraceY, kGateZ - 0.25f, kGateHalf, kTerraceY + kGateHeight, kGateZ + 0.25f, kIron, kLayerA),
    // beat 0, the guardhouse: walls 5 m high and an arch 4 m high in its north wall (the roof is drawn: kDecor)
    Span(-3.0f, kTerraceY, kTerraceSouth, -kSouthDoorHalf, kGuardWallTop, kTerraceSouth + 0.5f, kGuardStone, kLayerBoth), // its south
    Span(kSouthDoorHalf, kTerraceY, kTerraceSouth, 3.0f, kGuardWallTop, kTerraceSouth + 0.5f, kGuardStone, kLayerBoth),   // wall, with
    Span(-kSouthDoorHalf, kSouthDoorTop, kTerraceSouth, kSouthDoorHalf, kGuardWallTop, kTerraceSouth + 0.5f, kGuardStone, kLayerBoth), // the door
    // beat 0, the harbour: the quay, the landing at the south door, the harbour walls beside the stair (the stair: kPieces)
    Span(kQuayX0, kPitY, kQuayZ0, kQuayX1, kHarbourY, kQuayZ1, kGrey, kLayerBoth, 1.0f),
    Span(-kSeaStairHalf, kPitY, kSeaStairTop, kSeaStairHalf, kTerraceY, kTerraceSouth, kGrey, kLayerBoth, 1.0f),
    Wall(-kSeaStairHalf - 0.5f, kPitY, kQuayZ1, -kSeaStairHalf, kSlotWallTop, kTerraceSouth),
    Wall(kSeaStairHalf, kPitY, kQuayZ1, kSeaStairHalf + 0.5f, kSlotWallTop, kTerraceSouth),
    Span(-3.0f, kTerraceY, kTerraceSouth + 0.5f, -2.5f, kGuardWallTop, kGuardNorth, kGuardStone, kLayerBoth),
    Span(2.5f, kTerraceY, kTerraceSouth + 0.5f, 3.0f, kGuardWallTop, kGuardNorth, kGuardStone, kLayerBoth),
    Span(-3.0f, kTerraceY, kGuardNorth - 0.5f, -kArchHalf, kGuardWallTop, kGuardNorth, kGuardStone, kLayerBoth),
    Span(kArchHalf, kTerraceY, kGuardNorth - 0.5f, 3.0f, kGuardWallTop, kGuardNorth, kGuardStone, kLayerBoth),
    Span(-kArchHalf, kArchTop, kGuardNorth - 0.5f, kArchHalf, kGuardWallTop, kGuardNorth, kGuardStone, kLayerBoth),
    // beat 2, the town wall (both layers) beside the gate; in B the gate has fallen: a breach with rubble at its edges
    Wall(-3.0f, kTerraceY, kGateZ - 0.25f, -kGateHalf, kTerraceY + kGateHeight, kGateZ + 0.25f),
    Wall(kGateHalf, kTerraceY, kGateZ - 0.25f, 3.0f, kTerraceY + kGateHeight, kGateZ + 0.25f),
    Span(-kGateHalf, kTerraceY, kGateZ - 0.6f, -kGateHalf + 0.3f, kTerraceY + shapes::kRubble, kGateZ + 0.6f, kDebris, kLayerB),
    Span(kGateHalf - 0.3f, kTerraceY, kGateZ - 0.6f, kGateHalf, kTerraceY + shapes::kRubble, kGateZ + 0.6f, kDebris, kLayerB),
    // beat 3, the other way round: in B a heap of fallen masonry closes the terrace (wider than it, out of reach); in A the
    // way is clear. Low stones in front of it are walked over
    Span(-3.5f, kTerraceY, kRubbleSouth, 3.5f, kRubbleTop, kRubbleNorth, kDebris, kLayerB),
    Span(-2.4f, kTerraceY, kRubbleSouth - 0.7f, -1.0f, kTerraceY + 0.2f, kRubbleSouth, kDebris, kLayerB),
    // beat 4, the shallow ditch: the switch in the air taught where a miss costs a walk. Its floor (both layers), the beam of
    // B, the planks of A, the terrace beyond; the stairs back up are a piece (kPieces), walled off from the terrace so that B
    // does not climb round the heap of beat 3
    Floor(-3.0f, kDitchEdge, kDitchStairsX1, kDitchStripFar, kFloorY),
    Span(-1.5f, kTerraceY - 0.2f, kDitchStoneNear, 1.5f, kTerraceY, kDitchStoneFar, kDebris, kLayerB),
    Span(-3.0f, kTerraceY - 0.2f, kDitchStripNear, 3.0f, kTerraceY, kDitchStripFar, kWood, kLayerA, 1.0f),
    Span(-3.0f, kTerraceY - 0.2f, kDitchStripFar, 3.0f, kTerraceY, kTerraceEnd, kGrey, kLayerBoth, 1.0f),
    Floor(3.0f, kDitchStairsTop - 1.5f, kDitchStairsX1, kDitchStairsTop, kTerraceY),             // the landing at their top
    Wall(3.0f, kFloorY, kDitchStairsTop, kDitchStairsX0 - 0.1f, kTerraceY + kParapet, kDitchEdge), // between them and the terrace
    // beat 5, the gap: the beam of B (no aim needed), the planks of A, and a curtain of fallen masonry (B) at their end
    Span(-1.5f, kTerraceY - 0.2f, kStoneNear, 1.5f, kTerraceY, kStoneFar, kDebris, kLayerB),
    Span(-3.0f, kTerraceY - 0.2f, kStripNear, 3.0f, kTerraceY, kCourtSouth, kWood, kLayerA, 1.0f),
    Span(-3.5f, kTerraceY - 0.2f, kCurtainZ, 3.5f, kCurtainTop, kCourtSouth, kDebris, kLayerB),
    // ---- the courtyard: its floor, the wall under the strip, its west wall, and its east wall, 1.5 m thick: the path of
    // the east wing runs on its top from the upper room (z 10) to its north end over the gallery of the hub (z 23)
    Floor(kCourtWest, kCourtSouth, kCourtEast, kCourtNorth, kFloorY),
    Wall(kCourtWest, kFloorY, kCourtSouth - 0.5f, kCourtEast, kTerraceY - 0.2f, kCourtSouth),
    Wall(kCourtWest - 0.5f, kFloorY, kCourtSouth - 0.5f, kCourtWest, kCourtWallTop, kDoorNorth),
    Span(kCourtEast, kFloorY, kEastRoomNorth, kCourtEast + 1.5f, kCourtWallTop, 23.0f, kStone, kLayerBoth),
    // its north wall with the doorway into the hub; a wooden panel fills the doorway in A (the door of B is kDoor)
    Wall(kCourtWest, kFloorY, kDoorSouth, kDoorX - kDoorwayHalfWidth, kCourtWallTop, kDoorNorth),
    Wall(kDoorX + kDoorwayHalfWidth, kFloorY, kDoorSouth, kCourtEast, kCourtWallTop, kDoorNorth),
    Wall(kDoorX - kDoorwayHalfWidth, kDoorwayHigh, kDoorSouth, kDoorX + kDoorwayHalfWidth, kCourtWallTop, kDoorNorth),
    Span(kDoorX - kDoorwayHalfWidth, kDoorwayLow, kDoorSouth, kDoorX + kDoorwayHalfWidth, kDoorwayHigh, kDoorNorth, kWood, kLayerA),
    // ---- the square: the rim (solid down to the floor of the pool, so the edge of the pool is a wall of 1.5 m), the floor of
    // the pool, the plinth of the cathedral (its top is the floor inside it), the causeway to its portal
    Span(kHubWest, kSquareLowY - 0.2f, kHubSouth, kHubEast, kFloorY, kPoolSouth, kGrey, kLayerBoth, 1.0f),  // south rim
    Span(kHubWest, kSquareLowY - 0.2f, kPoolNorth, kHubEast, kFloorY, kHubNorth, kGrey, kLayerBoth, 1.0f),  // north rim
    Span(kHubWest, kSquareLowY - 0.2f, kPoolSouth, kPoolWest, kFloorY, kPoolNorth, kGrey, kLayerBoth, 1.0f), // west rim
    Span(kPoolEast, kSquareLowY - 0.2f, kPoolSouth, kHubEast, kFloorY, kPoolNorth, kGrey, kLayerBoth, 1.0f), // east rim
    Floor(kPoolWest, kPoolSouth, kPoolEast, kPoolNorth, kSquareLowY),                                       // the pool
    Span(kTowerWest, kSquareLowY - 0.2f, kTowerSouth, kTowerEast, kFloorY, kTowerNorth, kTowerStone, kLayerBoth, 1.0f), // plinth
    Span(kCausewayX0, kSquareLowY - 0.2f, kPoolSouth, kCausewayX1, kFloorY, kTowerSouth, kGrey, kLayerBoth, 1.0f), // causeway
    // the buttresses of the cathedral, standing in the pool on its west and east sides (out of reach: 9.5 m over the pool)
    Span(kTowerWest - 1.0f, kSquareLowY, 28.6f, kTowerWest, 7.0f, 29.4f, kTowerStone, kLayerBoth),
    Span(kTowerWest - 1.0f, kSquareLowY, 32.6f, kTowerWest, 7.0f, 33.4f, kTowerStone, kLayerBoth),
    Span(kTowerEast, kSquareLowY, 28.6f, kTowerEast + 1.0f, 7.0f, 29.4f, kTowerStone, kLayerBoth),
    Span(kTowerEast, kSquareLowY, 32.6f, kTowerEast + 1.0f, 7.0f, 33.4f, kTowerStone, kLayerBoth),
    // ---- the walls of the square, 10 m high (openings: the wings west, east and north; the closed shortcut door west; the
    // window of the ledge north)
    Wall(kHubWest - 0.5f, kFloorY, kDoorSouth, kCourtWest - 0.5f, kHubWallTop, kHubSouth),            // south, west part
    Wall(kCourtEast + 1.5f, kFloorY, kDoorSouth, kHubEast + 0.5f, kHubWallTop, kHubSouth),            // south, east part
    Wall(kHubWest - 0.5f, kFloorY, kHubSouth, kHubWest, kHubWallTop, kShortcutZ0),                    // west
    Wall(kHubWest - 0.5f, kFloorY, kShortcutZ1, kHubWest, kHubWallTop, kWingDoorZ0),
    Wall(kHubWest - 0.5f, kFloorY, kWingDoorZ1, kHubWest, kHubWallTop, kHubNorth + 0.5f),
    Wall(kHubWest - 0.5f, kOpeningTop, kShortcutZ0, kHubWest, kHubWallTop, kShortcutZ1),
    Wall(kHubWest - 0.5f, kOpeningTop, kWingDoorZ0, kHubWest, kHubWallTop, kWingDoorZ1),
    Wall(kHubEast, kFloorY, kHubSouth, kHubEast + 0.5f, kHubWallTop, kBayZ0),                         // east, with the opening
    Wall(kHubEast, kFloorY, kBayZ1, kHubEast + 0.5f, kHubWallTop, kWingDoorZ0),                       // of the undercroft
    Wall(kHubEast, kUndercroftTop, kBayZ0, kHubEast + 0.5f, kHubWallTop, kBayZ1),                     // (its bars: kCityGates)
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
    Wall(-24.5f, kFloorY, 21.5f, -24.0f, kCityWallTop, kLaneZ0),                                      // its west wall, with the
    Wall(-24.5f, kFloorY, kLaneZ1, -24.0f, kCityWallTop, 40.5f),                                      // door to the lane of the
    Wall(-24.5f, kFloorY + 2.4f, kLaneZ0, -24.0f, kCityWallTop, kLaneZ1),                             // market
    Wall(-15.0f, kFloorY, 21.5f, -14.5f, kCityWallTop, kWingDoorZ0 - 0.5f),
    Wall(-15.0f, kFloorY, kWingDoorZ1 + 0.5f, -14.5f, kCityWallTop, 40.5f),
    Wall(-24.5f, kFloorY, 21.5f, -22.0f, kCityWallTop, 22.0f),
    Wall(-18.0f, kFloorY, 21.5f, -14.5f, kCityWallTop, 22.0f),
    // the ramp down (its sides as high as the walls round it: nothing is climbed from it)
    Wall(-22.5f, kWestLowY, kWestDoorNorth, -22.0f, kCityWallTop, 21.5f),
    Wall(-18.0f, kWestLowY, kWestDoorNorth, -17.5f, kCityWallTop, 21.5f),
    // W2's door at its foot: the red panel in A (the blue door, in B, is a gate of scene/WestWing.h); a lintel over it
    Span(-22.0f, kWestLowY, kWestDoorSouth, -18.0f, kWestDoorTop, kWestDoorNorth, kWood, kLayerA),
    Wall(-22.0f, kWestDoorTop, kWestDoorSouth, -18.0f, kCityWallTop, kWestDoorNorth),
    // W3, the lower room: its floor and its walls
    Floor(-24.0f, 2.0f, -10.0f, kWestDoorSouth, kWestLowY),
    Wall(-24.5f, kWestLowY, 1.5f, -24.0f, kCityWallTop, kWestDoorNorth),
    Wall(-24.5f, kWestLowY, 1.5f, -9.5f, kCityWallTop, 2.0f),
    Wall(-10.0f, kWestLowY, 2.0f, -9.5f, kCityWallTop, kCellarWestZ0),                                // its east wall, with the
    Wall(-10.0f, kWestLowY, kCellarWestZ1, -9.5f, kCityWallTop, kWestDoorSouth),                      // grate of the cellar
    Wall(-10.0f, kCellarGrateTop, kCellarWestZ0, -9.5f, kCityWallTop, kCellarWestZ1),
    Wall(-24.0f, kWestLowY, kWestDoorSouth, -22.5f, kCityWallTop, kWestDoorNorth),                     // its north edge, beside
    Wall(-17.5f, kWestLowY, kWestDoorSouth, -12.5f, kCityWallTop, kWestDoorNorth),                     // and between the ramps
    // W3's mezzanine: a floor 3 m over the lower room, reached by a ramp along its east side; over the vault it has a hole
    // too narrow for the body (0.6 m; the body is 0.7 m across) and wide enough for an orb, open in B and closed by a red lid
    // in A
    Floor(-24.0f, 2.0f, kHoleX - kHoleHalf, kMezzNorth, kMezzY),
    Floor(kHoleX + kHoleHalf, 2.0f, kMezzEast, kMezzNorth, kMezzY),
    Floor(kHoleX - kHoleHalf, 2.0f, kHoleX + kHoleHalf, kHoleZ - kHoleHalf, kMezzY),
    Floor(kHoleX - kHoleHalf, kHoleZ + kHoleHalf, kHoleX + kHoleHalf, kMezzNorth, kMezzY),
    Span(kHoleX - kHoleHalf, kMezzY - 0.2f, kHoleZ - kHoleHalf, kHoleX + kHoleHalf, kMezzY, kHoleZ + kHoleHalf, kWood, kLayerA), // the lid (A)
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
    // the hall, cut by the canal (scene/Canal.h): the floor north of it and the quay south of it are solid down to its bed
    Span(15.0f, kCanalBed - 0.2f, kCanalNorth, 24.0f, kFloorY, 40.0f, kGrey, kLayerBoth, 1.0f),
    Span(15.0f, kCanalBed - 0.2f, 22.0f, kCellarEastX0, kQuayY, kCanalSouth, kGrey, kLayerBoth, 1.0f), // the quay, over
    Span(kCellarEastX1, kCanalBed - 0.2f, 22.0f, 24.0f, kQuayY, kCanalSouth, kGrey, kLayerBoth, 1.0f), // the east leg of the
    Span(kCellarEastX0, kCellarCeiling, 22.0f, kCellarEastX1, kQuayY, kCanalSouth, kGrey, kLayerBoth, 1.0f), // cellar
    Span(kCellarEastX0, kCanalBed - 0.2f, 22.0f, kCellarEastX1, kCanalBed, kCanalSouth, kGrey, kLayerBoth, 1.0f),
    Floor(15.0f, kCanalSouth, 24.0f, kCanalNorth, kCanalBed),
    Wall(24.0f, kCanalBed - 0.2f, 21.5f, 24.5f, kCityWallTop, kLockDoorZ0),                          // the east wall of the
    Wall(24.0f, kCanalBed - 0.2f, kLockDoorZ1, 24.5f, kCityWallTop, 40.5f),                           // hall, with the door
    Wall(24.0f, kFloorY + 2.2f, kLockDoorZ0, 24.5f, kCityWallTop, kLockDoorZ1),                       // to the lock walk
    // ---- the lane and the market (6.3.1)
    Floor(kMarketX1, kLaneZ0, -24.5f, kLaneZ1, kFloorY),
    Wall(kMarketX1, kFloorY, kLaneZ0 - 0.5f, -24.5f, kCityWallTop, kLaneZ0),
    Wall(kMarketX1, kFloorY, kLaneZ1, -24.5f, kCityWallTop, kLaneZ1 + 0.5f),
    Floor(kMarketX0, kMarketZ0, kMarketX1, kShaftZ0 - kShaftWall, kFloorY),                            // the square round the shaft
    Floor(kMarketX0, kShaftZ1 + kShaftWall, kMarketX1, kMarketZ1, kFloorY),
    Floor(kMarketX0, kShaftZ0 - kShaftWall, kShaftX0 - kShaftWall, kShaftZ1 + kShaftWall, kFloorY),
    Floor(kShaftX1 + kShaftWall, kShaftZ0 - kShaftWall, kMarketX1, kShaftZ1 + kShaftWall, kFloorY),
    Wall(kMarketX0 - 0.5f, kFloorY, kMarketZ0 - 0.5f, kMarketX0, kCityWallTop, kMarketZ1 + 0.5f),     // its walls (the backs of
    Wall(kMarketX0, kFloorY, kMarketZ1, kMarketX1 + 0.5f, kCityWallTop, kMarketZ1 + 0.5f),            // the houses round it)
    Wall(kMarketX0, kFloorY, kMarketZ0 - 0.5f, kMarketX1 + 0.5f, kCityWallTop, kMarketZ0),
    Wall(kMarketX1, kFloorY, kMarketZ0, kMarketX1 + 0.5f, kCityWallTop, kLaneZ0 - 0.5f),
    Wall(kMarketX1, kFloorY, kLaneZ1 + 0.5f, kMarketX1 + 0.5f, kCityWallTop, kMarketZ1),
    Wall(-42.5f, kFloorY, 52.5f, -37.5f, kFloorY + 12.0f, 57.5f),                                      // the clock tower (its top: decor)
    // the well: its floor, its walls (a parapet 1 m over the square, open over the landing of the way in), the landings
    Span(kShaftX0, kPitY, kShaftZ0, kShaftX1, kWellFloor, kShaftZ1, kGrey, kLayerBoth, 1.0f),
    Wall(kShaftX0 - kShaftWall, kPitY, kShaftZ0 - kShaftWall, kShaftX1 + kShaftWall, kFloorY + 1.0f, kShaftZ0),
    Wall(kShaftX0 - kShaftWall, kPitY, kShaftZ0, kShaftX0, kFloorY + 1.0f, kShaftZ1),
    Wall(kShaftX1, kPitY, kShaftZ0, kShaftX1 + kShaftWall, kFloorY + 1.0f, kShaftZ1),
    Wall(kShaftX0 - kShaftWall, kPitY, kShaftZ1, kShaftX1 - kLanding, kFloorY + 1.0f, kShaftZ1 + kShaftWall),
    Wall(kShaftX1, kPitY, kShaftZ1, kShaftX1 + kShaftWall, kFloorY + 1.0f, kShaftZ1 + kShaftWall),
    Wall(kShaftX1 - kLanding, kPitY, kShaftZ1, kShaftX1, kFloorY, kShaftZ1 + kShaftWall),              // (the sill of the way in)
    Span(kShaftX1 - kLanding, kWellFloor, kShaftZ1 - kLanding, kShaftX1, kLandingNE, kShaftZ1, kStone, kLayerBoth),
    Span(kShaftX1 - kLanding, kWellFloor, kShaftZ0, kShaftX1, kLandingSE, kShaftZ0 + kLanding, kStone, kLayerBoth),
    Span(kShaftX0, kWellFloor, kShaftZ0, kShaftX0 + kLanding, kLandingSW, kShaftZ0 + kLanding, kStone, kLayerBoth),
    Span(kShaftX0, kWellFloor, kShaftZ1 - kLanding, kShaftX0 + kLanding, kLandingNW, kShaftZ1, kStone, kLayerBoth),
    Span(-49.2f, kWellFloor, 42.8f, -46.8f, kWellFloor + 2.6f, 45.2f, Rgb{0.70f, 0.55f, 0.28f}, kLayerB),  // B: the day bell, fallen
    Span(-52.0f, kWellFloor, kShaftZ0, -46.0f, kWellFloor + 0.5f, kShaftZ0 + 1.2f, kDebris, kLayerB),  // B: the fallen stair
    // the stalls: a stone counter each (both layers), an awning over it (A: solid cloth on its frame), a rotten crate (B)
    MarketStall(-58.0f, 33.0f, false, 0), MarketStall(-52.0f, 33.0f, false, 1), MarketStall(-44.0f, 33.0f, false, 2),
    MarketStall(-38.0f, 33.0f, false, 3), MarketStall(-58.0f, 57.0f, false, 4), MarketStall(-52.0f, 57.0f, false, 5),
    MarketStall(-60.0f, 41.0f, true, 6), MarketStall(-60.0f, 47.0f, true, 7),
    MarketAwning(-58.0f, 33.0f, false, 0), MarketAwning(-52.0f, 33.0f, false, 1), MarketAwning(-44.0f, 33.0f, false, 2),
    MarketAwning(-38.0f, 33.0f, false, 3), MarketAwning(-58.0f, 57.0f, false, 4), MarketAwning(-52.0f, 57.0f, false, 5),
    MarketAwning(-60.0f, 41.0f, true, 6), MarketAwning(-60.0f, 47.0f, true, 7),
    MarketCrate(-58.0f, 33.0f), MarketCrate(-44.0f, 33.0f), MarketCrate(-52.0f, 57.0f), MarketCrate(-60.0f, 47.0f),
    // ---- the lock walk (E3): the terraces, solid down to the pit of the basins
    Span(kTerraceX0[0], kPitY, kLockWalkZ0, kTerraceX1[0], kTerraceFloor[0], kLockWalkZ1, kGrey, kLayerBoth, 1.0f),
    Span(kTerraceX0[1], kPitY, kLockWalkZ0, kTerraceX1[1], kTerraceFloor[1], kLockWalkZ1, kGrey, kLayerBoth, 1.0f),
    Span(kTerraceX0[2], kPitY, kLockWalkZ0, kTerraceX1[2], kTerraceFloor[2], kLockWalkZ1, kGrey, kLayerBoth, 1.0f),
    Span(kTerraceX0[3], kPitY, kLockWalkZ0, kTerraceX1[3], kTerraceFloor[3], kLockWalkZ1, kGrey, kLayerBoth, 1.0f), // the top terrace
    // the tower lock (E3): its bed, the parapet on the lock head west of it, the door to the water tower
    Span(kTowerLockX0, kPitY, kBasinZ0, kLockEast, kTowerLockBed, kLockWalkZ0, kGrey, kLayerBoth, 1.0f),
    Wall(kTowerLockX0 - 0.5f, kTerraceFloor[3], kBasinZ0, kTowerLockX0, kTerraceFloor[3] + kParapet, kLockWalkZ0),
    // under each stair, the stone it rests on
    Span(kTerraceX1[0], kPitY, kLockWalkZ0, kTerraceX0[1], kTerraceFloor[0], kLockWalkZ1, kStone, kLayerBoth),
    Span(kTerraceX1[1], kPitY, kLockWalkZ0, kTerraceX0[2], kTerraceFloor[1], kLockWalkZ1, kStone, kLayerBoth),
    Span(kTerraceX1[2], kPitY, kLockWalkZ0, kTerraceX0[3], kTerraceFloor[2], kLockWalkZ1, kStone, kLayerBoth),
    // the lock heads between the basins, as high as the next terrace
    Span(kTerraceX1[0], kPitY, kBasinZ0, kTerraceX0[1], kTerraceFloor[1], kLockWalkZ0, kStone, kLayerBoth),
    Span(kTerraceX1[1], kPitY, kBasinZ0, kTerraceX0[2], kTerraceFloor[2], kLockWalkZ0, kStone, kLayerBoth),
    Span(kTerraceX1[2], kPitY, kBasinZ0, kTerraceX0[3], kTerraceFloor[3], kLockWalkZ0, kStone, kLayerBoth),
    // the beds of the basins
    Span(kTerraceX0[0], kPitY, kBasinZ0, kTerraceX1[0], kTerraceFloor[0] - kBasinDepth, kLockWalkZ0, kGrey, kLayerBoth, 1.0f),
    Span(kTerraceX0[1], kPitY, kBasinZ0, kTerraceX1[1], kTerraceFloor[1] - kBasinDepth, kLockWalkZ0, kGrey, kLayerBoth, 1.0f),
    Span(kTerraceX0[2], kPitY, kBasinZ0, kTerraceX1[2], kTerraceFloor[2] - kBasinDepth, kLockWalkZ0, kGrey, kLayerBoth, 1.0f),
    // the high walls round it: north (the backs of the houses), east, south
    Span(kTerraceX0[0], kPitY, kLockWalkZ1, kLockEast + 0.5f, kLockWallTop, kLockWalkZ1 + 0.5f, kStone, kLayerBoth),
    Span(kLockEast, kPitY, kBasinZ0 - 0.5f, kLockEast + 0.5f, kLockWallTop, kLockWalkZ1 + 0.5f, kStone, kLayerBoth),
    Span(kTerraceX0[0], kPitY, kBasinZ0 - 0.5f, kWaterDoorX0, kLockWallTop, kBasinZ0, kStone, kLayerBoth),      // south, with
    Span(kWaterDoorX1, kPitY, kBasinZ0 - 0.5f, kLockEast + 0.5f, kLockWallTop, kBasinZ0, kStone, kLayerBoth),    // the door of
    Span(kWaterDoorX0, kPitY, kBasinZ0 - 0.5f, kWaterDoorX1, kTowerDoorSill, kBasinZ0, kStone, kLayerBoth),      // the water
    Span(kWaterDoorX0, kTowerDoorSill + 2.4f, kBasinZ0 - 0.5f, kWaterDoorX1, kTowerRoomTop + 0.5f, kBasinZ0, kStone, kLayerBoth), // tower
    // ---- the water tower (E4): the room of the dawn bell (its shaft above: decor)
    Span(kTowerRoomX0, kPitY, kTowerRoomZ0, kTowerRoomX1, kTowerDoorSill, kBasinZ0 - 0.5f, kGrey, kLayerBoth, 1.0f),
    Wall(kTowerRoomX0 - 0.5f, kPitY, kTowerRoomZ0 - 0.5f, kTowerRoomX0, kTowerRoomTop + 0.5f, kBasinZ0 - 0.5f),
    Wall(kTowerRoomX1, kPitY, kTowerRoomZ0 - 0.5f, kTowerRoomX1 + 0.5f, kTowerRoomTop + 0.5f, kBasinZ0 - 0.5f),
    Wall(kTowerRoomX0, kPitY, kTowerRoomZ0 - 0.5f, kTowerRoomX1, kTowerRoomTop + 0.5f, kTowerRoomZ0),
    Span(kTowerRoomX0, kTowerRoomTop, kTowerRoomZ0, kTowerRoomX1, kTowerRoomTop + 0.5f, kBasinZ0 - 0.5f, kStone, kLayerBoth),
    Wall(14.5f, kFloorY, 40.0f, kStairsX0, kCityWallTop, 40.5f),                                      // north, with the door of
    Wall(kStairsX1, kFloorY, 40.0f, 24.5f, kCityWallTop, 40.5f),                                      // the stairs from the lookout
    Wall(kStairsX0, kOpeningTop, 40.0f, kStairsX1, kCityWallTop, 40.5f),
    Wall(14.5f, kCanalBed - 0.2f, 21.5f, 15.0f, kCityWallTop, kBayZ0),                                // the west wall of the canal,
    Wall(14.5f, kCanalBed - 0.2f, kBayZ1, 15.0f, kCityWallTop, kWingDoorZ0 - 0.5f),                   // with the breach of the bay:
    Wall(14.5f, kCanalBed - 0.2f, kBayZ0, 15.0f, kUndercroftY, kBayZ1),                               // its sill,
    Wall(14.5f, kBreachTop, kBayZ0, 15.0f, kCityWallTop, kBayZ1),                                     // its lintel,
    Span(14.5f, kUndercroftY, kBayZ0, 15.0f, kBreachTop, kBayZ1, kIron, kLayerA),                      // in A an iron panel
    Wall(15.0f, kFloorY, kCanalNorth, kCanalParapetX1, kFloorY + kParapet, kCanalNorth + 0.2f),       // the parapets of the west
    Wall(15.0f, kQuayY, kCanalSouth - 0.2f, kCanalParapetX1, kQuayY + kParapet, kCanalSouth),         // end of the floor, the quay
    // the undercroft: from the breach west under the gallery to the opening in the east wall of the square
    Span(kHubEast, kUndercroftY - 0.2f, kBayZ0, 14.5f, kUndercroftY, kBayZ1, kGrey, kLayerBoth, 1.0f),
    Wall(kHubEast + 0.5f, kUndercroftY - 0.2f, kBayZ0 - 0.2f, 14.5f, kBreachTop + 0.2f, kBayZ0),
    Wall(kHubEast + 0.5f, kUndercroftY - 0.2f, kBayZ1, 14.5f, kBreachTop + 0.2f, kBayZ1 + 0.2f),
    Wall(kHubEast + 0.5f, kBreachTop, kBayZ0, 14.5f, kBreachTop + 0.2f, kBayZ1),
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
    // ---- the cellar (B: dry; A: flooded, closed by iron sluice panels): the floors, the walls, the ceilings of its legs
    Span(kCellarEastX0, kCellarY - 0.2f, kCellarZ0, kCellarEastX1, kCellarY, 22.0f, kGrey, kLayerBoth, 1.0f),           // east leg
    Span(kCellarSouthX0, kCellarY - 0.2f, kCellarZ0, kCellarEastX0, kCellarY, kCellarZ1, kGrey, kLayerBoth, 1.0f),     // middle leg
    Span(-9.5f, kWestLowY - 0.2f, kCellarWestZ0, kCellarSouthX1, kWestLowY, kCellarWestZ1, kGrey, kLayerBoth, 1.0f), // west leg
    Wall(kCellarEastX0 - 0.2f, kCellarY - 0.2f, kCellarZ1, kCellarEastX0, kCellarCeiling, 22.0f),
    Wall(kCellarEastX1, kCellarY - 0.2f, kCellarZ0 - 0.2f, kCellarEastX1 + 0.2f, kCellarCeiling, 22.0f),
    Wall(kCellarSouthX1, kCellarY - 0.2f, kCellarZ0 - 0.2f, kCellarEastX1, kCellarCeiling, kCellarZ0),                 // middle, south
    Wall(kCellarSouthX0 - 0.2f, kCellarY - 0.2f, kCellarZ1, kCellarEastX0 - 0.2f, kCellarCeiling, kCellarZ1 + 0.2f),   // middle, north
    Wall(kCellarSouthX0 - 0.2f, kWestLowY - 0.2f, kCellarWestZ1, kCellarSouthX0, kCellarCeiling, kCellarZ1 + 0.2f),   // south leg, west
    Wall(kCellarSouthX1, kWestLowY - 0.2f, kCellarWestZ0 - 0.2f, kCellarSouthX1 + 0.2f, kCellarCeiling, kCellarZ0 - 0.2f), // and east
    Wall(-9.5f, kWestLowY - 0.2f, kCellarWestZ0 - 0.2f, kCellarSouthX1, kCellarCeiling, kCellarWestZ0),               // west leg
    Wall(-9.5f, kWestLowY - 0.2f, kCellarWestZ1, kCellarSouthX0 - 0.2f, kCellarCeiling, kCellarWestZ1 + 0.2f),
    Wall(kCellarEastX0 - 0.2f, kCellarCeiling, kCellarZ1, kCellarEastX1 + 0.2f, kCellarCeiling + 0.2f, 22.0f),         // the ceilings
    Wall(kCellarSouthX0 - 0.2f, kCellarCeiling, kCellarZ0 - 0.2f, kCellarEastX1 + 0.2f, kCellarCeiling + 0.2f, kCellarZ1 + 0.2f),
    Wall(kCellarSouthX0 - 0.2f, kCellarCeiling, kCellarWestZ0 - 0.2f, kCellarSouthX1 + 0.2f, kCellarCeiling + 0.2f, kCellarZ0 - 0.2f),
    Wall(-9.5f, kCellarCeiling, kCellarWestZ0 - 0.2f, kCellarSouthX0 - 0.2f, kCellarCeiling + 0.2f, kCellarWestZ1 + 0.2f),
    Span(kCellarEastX0, kCellarY, kCanalSouth - 0.2f, kCellarEastX1, kCellarCeiling, kCanalSouth, kIron, kLayerA),    // the sluice panels (A)
    Span(-10.0f, kWestLowY, kCellarWestZ0, -9.5f, kCellarGrateTop, kCellarWestZ1, kIron, kLayerA),
    // ---- N2: the ledge of B along the west wall of the north hall (its cage and its step are gates: scene/CityParts.h)
    Span(-9.0f, kFloorY, kN2LedgeZ0, kN2LedgeX1, kN2LedgeTop, kN2LedgeZ1, kDebris, kLayerB),
    // ---- the north wing (UP): the corridor, the hall, the two landings, the ledge through the window into the hub
    Floor(-1.0f, kHubNorth + 0.5f, 1.0f, 44.0f, kFloorY),
    Wall(-1.5f, kFloorY, kHubNorth + 0.5f, -1.0f, kCityWallTop, kNorthBreachZ0),                       // its west wall, with the
    Wall(-1.5f, kFloorY, kNorthBreachZ1, -1.0f, kCityWallTop, 44.0f),                                   // breach of the passage
    Span(-1.5f, kFloorY, kNorthBreachZ0, -1.0f, kCityWallTop, kNorthBreachZ1, kWood, kLayerA),           // (A: a panel)
    Span(-1.85f, kFloorY, kNorthBreachZ0, -0.65f, kFloorY + shapes::kRubble, kNorthBreachZ0 + 0.3f, kDebris, kLayerB), // (B: rubble)
    Span(-1.85f, kFloorY, kNorthBreachZ1 - 0.3f, -0.65f, kFloorY + shapes::kRubble, kNorthBreachZ1, kDebris, kLayerB),
    Wall(1.0f, kFloorY, kHubNorth + 0.5f, 1.5f, kCityWallTop, 44.0f),
    Floor(-9.0f, 44.0f, 9.0f, 60.0f, kFloorY),
    Wall(-9.5f, kFloorY, 43.5f, -9.0f, kNorthWallTop, 60.5f),
    Wall(9.0f, kFloorY, 43.5f, 9.5f, kNorthWallTop, kBridgeSouth),                                     // east, with the door of the
    Wall(9.0f, kFloorY, kBridgeSouth, 9.5f, kBridgeY - 0.2f, kBridgeNorth),                            // sky bridge from landing 1
    Wall(9.0f, kLookoutTop, kBridgeSouth, 9.5f, kNorthWallTop, kBridgeNorth),
    Wall(9.0f, kFloorY, kBridgeNorth, 9.5f, kNorthWallTop, 60.5f),
    Wall(-9.5f, kFloorY, 60.0f, kTheatreDoorX0, kNorthWallTop, 60.5f),                                 // north, with the door of
    Wall(kTheatreDoorX1, kFloorY, 60.0f, 9.5f, kNorthWallTop, 60.5f),                                  // the theatre lane (under
    Wall(kTheatreDoorX0, kFloorY + 2.4f, 60.0f, kTheatreDoorX1, kNorthWallTop, 60.5f),                 // landing 1)
    // ---- the theatre (6.5.1): the lane (its stair is in kPieces), the stage wall and its door, the stage, the orchestra
    Floor(kTheatreDoorX0, 60.5f, kTheatreDoorX1, 62.0f, kFloorY),
    Span(kTheatreDoorX0, kFloorY, 65.0f, kTheatreDoorX1, kStageY, kStageWallZ + 0.6f, kGrey, kLayerBoth, 1.0f),
    Wall(kTheatreDoorX0 - 0.5f, kFloorY, 60.5f, kTheatreDoorX0, kStageY + 4.0f, kStageWallZ),
    Wall(kTheatreDoorX1, kFloorY, 60.5f, kTheatreDoorX1 + 0.5f, kStageY + 4.0f, kStageWallZ),
    Wall(-kTheatreX - 0.5f, kFloorY, kStageWallZ, kTheatreDoorX0, kTheatreWallTop, kStageWallZ + 0.6f),
    Wall(kTheatreDoorX1, kFloorY, kStageWallZ, kTheatreX + 0.5f, kTheatreWallTop, kStageWallZ + 0.6f),
    Wall(kTheatreDoorX0, kStageY + 2.4f, kStageWallZ, kTheatreDoorX1, kTheatreWallTop, kStageWallZ + 0.6f),
    Floor(-kTheatreX, kStageWallZ + 0.6f, kTheatreX, kBackWallZ, kFloorY),
    Span(-10.0f, kFloorY, kStageWallZ + 0.6f, 10.0f, kStageY, kStageFront, kWood, kLayerBoth),
    // the rows: west of kFallenX rows 4 .. 6 are A only (fallen in B), the rest both
    TheatreRow(0, true), TheatreRow(1, true), TheatreRow(2, true), TheatreRow(3, true), TheatreRow(4, true), TheatreRow(5, true),
    TheatreRow(6, true), TheatreRow(4, false), TheatreRow(5, false), TheatreRow(6, false), TheatreRow(0, false), TheatreRow(1, false),
    TheatreRow(2, false), TheatreRow(3, false),
    Span(kFallenX, kFloorY, kPromenadeZ0, kTheatreX, kPromenadeY, kBackWallZ, kGrey, kLayerBoth, 1.0f),      // the promenade (east)
    Span(-kTheatreX, kFloorY, kPromenadeZ0, kFallenX, kPromenadeY, kBackWallZ, kGrey, kLayerA, 1.0f),        // (west: A)
    Span(-12.0f, kFloorY, kRowsZ0 + 4.0f * kRowDepth, kFallenX, kFloorY + 0.6f, kBackWallZ, kDebris, kLayerB), // B: their rubble
    // its walls: east and west, the back wall with the breach (B) at the promenade, the terrace behind it
    Wall(-kTheatreX - 0.5f, kFloorY, kStageWallZ + 0.6f, -kTheatreX, kTheatreWallTop, kBackWallZ + 0.5f),
    Wall(kTheatreX, kFloorY, kStageWallZ + 0.6f, kTheatreX + 0.5f, kTheatreWallTop, kBackWallZ + 0.5f),
    Wall(-kTheatreX, kFloorY, kBackWallZ, kBreachX0, kTheatreWallTop, kBackWallZ + 0.5f),
    Wall(kBreachX1, kFloorY, kBackWallZ, kGraveLaneX0, kTheatreWallTop, kBackWallZ + 0.5f),                // the back wall, with
    Wall(kGraveLaneX1, kFloorY, kBackWallZ, kTheatreX, kTheatreWallTop, kBackWallZ + 0.5f),                // the door to the lane of
    Wall(kGraveLaneX0, kGraveY + 2.4f, kBackWallZ, kGraveLaneX1, kTheatreWallTop, kBackWallZ + 0.5f),     // the graveyard
    Span(kGraveLaneX0, kFloorY, kBackWallZ, kGraveLaneX1, kGraveY, kGraveZ0, kGrey, kLayerBoth, 1.0f),
    Wall(kGraveLaneX0 - 0.5f, kFloorY, kBackWallZ + 0.5f, kGraveLaneX0, kGraveWallTop + 1.0f, kGraveZ0),
    Wall(kGraveLaneX1, kFloorY, kBackWallZ + 0.5f, kGraveLaneX1 + 0.5f, kGraveWallTop + 1.0f, kGraveZ0),
    // ---- the graveyard (6.5.2): the yard, its parapet, the rows of graves, the night bell tower, the lone grave (B)
    Span(kGraveX0, kFloorY, kGraveZ0, kGraveX1, kGraveY, kGraveZ1, Rgb{0.50f, 0.55f, 0.40f}, kLayerBoth),
    Wall(kGraveX0 - 0.5f, kFloorY, kGraveZ0 - 0.5f, kGraveLaneX0 - 0.5f, kGraveWallTop, kGraveZ0),
    Wall(kGraveLaneX1 + 0.5f, kFloorY, kGraveZ0 - 0.5f, kGraveX1 + 0.5f, kGraveWallTop, kGraveZ0),
    Wall(kGraveX0 - 0.5f, kFloorY, kGraveZ0, kGraveX0, kGraveWallTop, kGraveZ1 + 0.5f),
    Wall(kGraveX1, kFloorY, kGraveZ0, kGraveX1 + 0.5f, kGraveWallTop, kGraveZ1 + 0.5f),
    Wall(kGraveX0, kFloorY, kGraveZ1, kGraveX1, kGraveWallTop, kGraveZ1 + 0.5f),
    Span(-10.0f, kGraveY, 94.2f, 4.0f, kGraveY + 0.35f, 94.8f, kGrey, kLayerBoth),
    Span(-10.0f, kGraveY, 96.7f, 4.0f, kGraveY + 0.35f, 97.3f, kGrey, kLayerBoth),
    Span(-10.0f, kGraveY, 99.2f, 4.0f, kGraveY + 0.35f, 99.8f, kGrey, kLayerBoth),
    Span(8.5f, kGraveY, 100.5f, 12.0f, kGraveY + 9.0f, 104.0f, kStone, kLayerBoth),                      // the night bell tower
    Span(-11.0f, kGraveY, 101.5f, -9.0f, kGraveY + 0.3f, 103.0f, Rgb{0.40f, 0.36f, 0.30f}, kLayerB),      // B: the lone grave
    Wall(kBreachX0, kFloorY, kBackWallZ, kBreachX1, kPromenadeY, kBackWallZ + 0.5f),                     // the sill of the breach
    Span(kBreachX0, kPromenadeY, kBackWallZ, kBreachX1, kTheatreWallTop, kBackWallZ + 0.5f, kWood, kLayerA), // A: the panel
    Span(-13.0f, kFloorY, kBackWallZ + 0.5f, -7.0f, kPromenadeY, kTerraceZ1, kGrey, kLayerBoth, 1.0f),       // the terrace
    Wall(-13.5f, kFloorY, kBackWallZ + 0.5f, -13.0f, kTheatreWallTop, kTerraceZ1 + 0.5f),
    Wall(-7.0f, kFloorY, kBackWallZ + 0.5f, -6.5f, kTheatreWallTop, kTerraceZ1 + 0.5f),
    Wall(-13.0f, kFloorY, kTerraceZ1, -7.0f, kTheatreWallTop, kTerraceZ1 + 0.5f),
    Wall(-9.0f, kFloorY, 43.5f, -1.5f, kCityWallTop, 44.0f),
    Wall(1.5f, kFloorY, 43.5f, 9.0f, kCityWallTop, 44.0f),
    Floor(-1.0f, 58.0f, 9.0f, 60.0f, kFloorY + 3.5f),                                                 // landing 1 (north)
    Floor(-9.0f, 44.0f, kPrismGapX0, 46.0f, kNorthHighY),                                             // landing 2 (south): its
    Floor(kPrismGapX1, 44.0f, 4.0f, 46.0f, kNorthHighY),                                              // two parts, the gap of N1,
    Span(kPrismGapX0, kNorthHighY - 0.2f, 44.0f, kPrismGapX1, kNorthHighY, 46.0f, kWood, kLayerA, 1.0f), // and its bridge (A)
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
    Span(kBridgeGapX0, kBridgeY - 0.3f, kBridgeSouth, kBridgeGapX1, kBridgeY, kBridgeNorth, kWood, kLayerA),
    Span(kBridgeGapX1, kBridgeY - 0.3f, kBridgeSouth, kBridgeEast, kBridgeY, kBridgeNorth, kDark, kLayerBoth),
    Span(9.5f, kBridgeY, kBridgeSouth, kBridgeEast, kLookoutTop, kBridgeSouth + 0.15f, kWood, kLayerA),  // its parapets (A: in B they fell with it)
    Span(9.5f, kBridgeY, kBridgeNorth - 0.15f, kBridgeEast, kLookoutTop, kBridgeNorth, kWood, kLayerA),
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
    Ramp(18.0f, 22.0f, 22.0f, kQuayY, kEastRoomNorth, kEastHighY),             // east: up, from the quay
    Ramp(kCellarSouthX0, kCellarSouthX1, kCellarZ0, kCellarY, kCellarWestZ1, kWestLowY), // the cellar: down to the grate
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

/// Pieces of scene/Shapes.h: the breach in the north wall of the west hall (5 boxes), the slope of the stairs from the lookout
/// (1), and the slopes of the two stairs down into the pool of the square (2). Their steps are drawn in kDecor.
inline constexpr int kPieceCount = 5 + 1 + 2 + 1 + 1 + 1 + 3 + kLockCount + 1 + 4 + 3;
/// The two stairs down into the pool: south-east (from the south rim, beside the east rim) and north-west (from the north rim,
/// beside the west rim), so that the pool is entered at its two opposite corners.
inline void PoolStairs(std::vector<SceneObject>& solid, std::vector<SceneObject>& drawn)
{
    shapes::Stairs(kPoolEast - kStairWidth, kPoolEast, kPoolSouth, kFloorY, kPoolSouth + kStairRun, kSquareLowY, shapes::kStoneLight, solid, drawn);
    shapes::Stairs(kPoolWest, kPoolWest + kStairWidth, kPoolNorth, kFloorY, kPoolNorth - kStairRun, kSquareLowY, shapes::kStoneLight, solid, drawn);
}
/// The stairs from the floor of the ditch of the prologue back up to the terrace (beat 4: a miss costs a short walk).
inline void DitchStairs(std::vector<SceneObject>& solid, std::vector<SceneObject>& drawn)
{
    shapes::Stairs(kDitchStairsX0, kDitchStairsX1, kDitchEdge, kFloorY, kDitchStairsTop, kTerraceY, shapes::kStoneLight, solid, drawn);
}
/// The stairs at the east end of the canal: from its bed up to the floor of the hall (walked in B; in A they go into the water).
inline void CanalStairs(std::vector<SceneObject>& solid, std::vector<SceneObject>& drawn)
{
    shapes::Stairs(kCanalStairsX0, 24.0f, kCanalStairsFoot, kCanalBed, kCanalNorth, kFloorY, shapes::kStoneLight, solid, drawn);
}
/// The stair of the harbour: from the quay up to the landing at the south door of the guardhouse.
inline void SeaStair(std::vector<SceneObject>& solid, std::vector<SceneObject>& drawn)
{
    shapes::Stairs(-kSeaStairHalf, kSeaStairHalf, kQuayZ1, kHarbourY, kSeaStairTop, kTerraceY, shapes::kStoneLight, solid, drawn);
}
/// The three stairs of the lock walk (along X, from terrace k to terrace k + 1) and the stair out of each dry basin (along Z,
/// from its bed up to its terrace, at its east end).
inline void LockStairs(std::vector<SceneObject>& solid, std::vector<SceneObject>& drawn)
{
    for (int k = 0; k < 3; ++k)
    {
        shapes::StairsX(kLockWalkZ0, kLockWalkZ1, kTerraceX1[k], kTerraceFloor[k], kTerraceX0[k + 1], kTerraceFloor[k + 1], shapes::kStoneLight, solid, drawn);
    }
    for (int k = 0; k < kLockCount; ++k)
    {
        shapes::Stairs(kTerraceX1[k] - kBasinStairWidth, kTerraceX1[k], kBasinZ0 + 0.5f, kTerraceFloor[k] - kBasinDepth, kLockWalkZ0, kTerraceFloor[k],
                       shapes::kStoneLight, solid, drawn);
    }
    shapes::Stairs(kLockEast - kBasinStairWidth, kLockEast, kBasinZ0 + 0.5f, kTowerLockBed, kLockWalkZ0, kTerraceFloor[3], shapes::kStoneLight, solid, drawn);
}
/// The stair down the walls of the well: east (both layers), south (A: a wooden stair; in B it has fallen), west and north
/// (both layers), from landing to landing, 2 m down over 8.4 m each.
inline void WellStairs(std::vector<SceneObject>& solid, std::vector<SceneObject>& drawn)
{
    const float a0 = kShaftZ0 + kLanding;
    const float a1 = kShaftZ1 - kLanding;
    const float b0 = kShaftX0 + kLanding;
    const float b1 = kShaftX1 - kLanding;
    shapes::Stairs(kShaftX1 - kLanding, kShaftX1, a1, kLandingNE, a0, kLandingSE, shapes::kStoneLight, solid, drawn);
    const std::size_t solidFirst = solid.size();
    const std::size_t drawnFirst = drawn.size();
    shapes::StairsX(kShaftZ0, kShaftZ0 + kLanding, b1, kLandingSE, b0, kLandingSW, shapes::Rgb{kWood.r, kWood.g, kWood.b}, solid, drawn);
    for (std::size_t i = solidFirst; i < solid.size(); ++i)
    {
        solid[i].layerMask = kLayerA;
    }
    for (std::size_t i = drawnFirst; i < drawn.size(); ++i)
    {
        drawn[i].layerMask = kLayerA;
    }
    shapes::Stairs(kShaftX0, kShaftX0 + kLanding, a0, kLandingSW, a1, kLandingNW, shapes::kStoneLight, solid, drawn);
    shapes::StairsX(kShaftZ1 - kLanding, kShaftZ1, b0, kLandingNW, b1, kWellFloor, shapes::kStoneLight, solid, drawn);
}
/// The stairs of the theatre: up the lane to the stage door, from the stage down to the orchestra, and the aisle up the east
/// end of the rows to the promenade.
inline void TheatreStairs(std::vector<SceneObject>& solid, std::vector<SceneObject>& drawn)
{
    shapes::Stairs(kTheatreDoorX0, kTheatreDoorX1, 62.0f, kFloorY, 65.0f, kStageY, shapes::kStoneLight, solid, drawn);
    shapes::Stairs(-1.0f, 1.0f, kStageFront, kStageY, kStageFront + 3.0f, kFloorY, shapes::kStoneLight, solid, drawn);
    shapes::Stairs(12.0f, kTheatreX, kRowsZ0, kFloorY, kPromenadeZ0, kPromenadeY, shapes::kStoneLight, solid, drawn);
}
inline std::array<SceneObject, kPieceCount> BuildPieces()
{
    std::vector<SceneObject> v;
    shapes::BreachWall(-24.5f, -14.5f, 40.25f, 0.5f, kFloorY, kCityWallTop - kFloorY, kWestBreachX0, kWestBreachX1, v);
    std::vector<SceneObject> steps;
    shapes::Stairs(kStairsX0, kStairsX1, 56.0f, kBridgeY, 42.0f, kFloorY, shapes::kStoneLight, v, steps);
    PoolStairs(v, steps);
    DitchStairs(v, steps);
    CanalStairs(v, steps);
    SeaStair(v, steps);
    LockStairs(v, steps);
    WellStairs(v, steps);
    TheatreStairs(v, steps);
    std::array<SceneObject, kPieceCount> out{};
    for (std::size_t i = 0; i < out.size() && i < v.size(); ++i)
    {
        out[i] = v[i];
    }
    return out;
}
inline const std::array<SceneObject, kPieceCount> kPieces = BuildPieces();

// ---- the anchor lamps (docs/LEVEL_DESIGN.md, 14.5): a stone lamp that looks exactly the same in both layers (the one thing
// in the city that the switch does not change). It is lit the first time a body comes near it; from then on a fall or R in
// its room brings the body back beside it. Its pedestal is solid; its light is drawn (scene/Anchors.h).
struct AnchorDesc
{
    Vec3 base;        ///< the centre of the foot of the pedestal, on the floor
    Vec3 stand;       ///< where a body comes back, standing (a place to stand in both layers)
    const char* room; ///< the room (kRooms) whose falls and R come back here
    const char* name; ///< for the log
};
inline constexpr AnchorDesc kAnchors[] = {
    {{-6.0f, kHarbourY, -54.6f}, {-6.0f, kHarbourY, -53.3f}, "quay", "the lantern of the quay"},
    {{2.0f, kTerraceY, -16.6f}, {0.8f, kTerraceY, -16.6f}, "terrace", "the first lamp"},
    {{2.4f, kTerraceY, 4.6f}, {1.1f, kTerraceY, 4.6f}, "gap", "before the gap"},
    {{-5.0f, kFloorY, 22.6f}, {-5.0f, kFloorY, 23.7f}, "square", "the square"},
    {{-16.0f, kFloorY, 38.6f}, {-16.0f, kFloorY, 37.3f}, "west hall", "the archive"},
    {{23.2f, kFloorY, 33.0f}, {21.9f, kFloorY, 33.0f}, "east hall", "the canal"},
    {{29.0f, kFloorY, 35.5f}, {29.0f, kFloorY, 34.2f}, "lock walk", "the locks"},
    {{-35.2f, kFloorY, 58.6f}, {-35.2f, kFloorY, 57.3f}, "market", "the market"},
    {{13.0f, kFloorY, 72.5f}, {11.7f, kFloorY, 72.5f}, "theatre", "the theatre"},
    {{6.5f, kGraveY, 93.0f}, {6.5f, kGraveY, 94.3f}, "graveyard", "the graveyard"},
    {{-7.6f, kFloorY, 52.0f}, {-6.3f, kFloorY, 52.0f}, "north hall", "the north hall"},
};
inline constexpr int kAnchorCount = static_cast<int>(sizeof(kAnchors) / sizeof(kAnchors[0]));
inline constexpr float kAnchorHalf = 0.25f;   ///< UNITS: metres: the pedestal is 0.5 m square
inline constexpr float kAnchorHeight = 0.9f;  ///< UNITS: metres: the pedestal (its lamp is drawn on top)
inline constexpr Rgb kAnchorStone{0.70f, 0.66f, 0.58f};
/// The pedestal of an anchor: both layers, with a trace of light of its own, so that the model shader does not give it the
/// stone of the layer: it looks the same in A and in B.
constexpr SceneObject AnchorPedestal(const AnchorDesc& a)
{
    SceneObject o = Span(a.base.x - kAnchorHalf, a.base.y, a.base.z - kAnchorHalf, a.base.x + kAnchorHalf, a.base.y + kAnchorHeight,
                         a.base.z + kAnchorHalf, kAnchorStone, kLayerBoth);
    o.glow = 0.02f;
    return o;
}

inline constexpr int kStageStaticCount = kFixedCount + kRampCount + kRunCount + kArcadeCount + kPieceCount + kAnchorCount;

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
    for (const AnchorDesc& a : kAnchors)
    {
        all[static_cast<std::size_t>(n++)] = AnchorPedestal(a);
    }
    return all;
}
inline const std::array<SceneObject, kStageStaticCount> kStageStatics = BuildStatics();
inline constexpr int kGateIndex = 1; ///< the gate (A) in kStageStatics (scene/StageLights.h hides a light in it)

/// The door of fallen masonry (layer B) in the doorway from the courtyard into the hub; the plate (A) opens it.
inline constexpr SceneObject kDoor = Span(kDoorX - kDoorwayHalfWidth, kDoorwayLow, kDoorSouth, kDoorX + kDoorwayHalfWidth,
                                          kDoorwayHigh, kDoorNorth, kDebris, kLayerB);

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
    {"cathedral", kTowerInnerWest, -2.0f, kTowerInnerSouth, kTowerInnerEast, 14.0f, kTowerInnerNorth, {-2.5f, kFloorY, 28.0f}, false},
    {"guardhouse", -2.5f, 0.0f, kTerraceSouth + 0.5f, 2.5f, 6.0f, kGuardNorth, {0.0f, kTerraceY, -23.2f}, false},
    {"terrace", -3.2f, 0.0f, kGuardNorth, kDitchStairsX1 + 0.2f, 6.0f, kDitchEdge, {0.0f, kTerraceY, -18.2f}, false},
    {"ditch", -3.2f, -2.0f, kDitchStairsTop, kDitchStairsX1 + 0.2f, 2.5f, kDitchStripFar, {4.4f, kFloorY, -4.6f}, false},
    {"gap", -3.2f, 0.0f, kDitchStripFar, 3.2f, 6.0f, kCourtSouth, {0.0f, kTerraceY, 4.0f}, false},
    {"courtyard", kCourtWest, -2.0f, kCourtSouth, kCourtEast, 3.0f, kCourtNorth, {kCourtWest + 0.5f, kFloorY, static_cast<float>(kCellZ0)}, true},
    {"undercroft", kHubEast, -2.0f, kBayZ0, 15.0f, -0.5f, kBayZ1, {11.0f, kUndercroftY, 27.2f}, false},
    {"square", kHubWest, kSquareLowY - 0.5f, kHubSouth, kHubEast, -0.5f, kHubNorth, {0.0f, kFloorY, 23.0f}, false},
    {"west landing", -12.0f, -2.0f, 22.0f, kHubWest, -0.5f, 25.0f, {-11.0f, kFloorY, 23.5f}, false},
    {"west passage", kWestBreachX0, -2.0f, kPassageSouth, -1.5f, -0.5f, kPassageNorth, {-12.0f, kFloorY, 42.0f}, false},
    {"sky bridge", 9.0f, 1.5f, kBridgeSouth, kBridgeEast, 3.5f, kBridgeNorth, {10.3f, kBridgeY, 59.0f}, false},
    {"east stairs", kBridgeEast, -2.0f, kPassageSouth, 21.5f, 3.5f, kBridgeNorth, {20.0f, kBridgeY, 58.5f}, false},
    {"west hall", -24.0f, -2.0f, 22.0f, kHubWest, -0.5f, 40.0f, {-12.0f, kFloorY, 31.0f}, true},
    {"west vault", -24.0f, -6.0f, 2.0f, kVaultEast, -4.0f, kMezzNorth, {-21.5f, kWestLowY, 4.3f}, false},
    {"west mezzanine", -24.0f, -2.5f, 2.0f, kMezzEast, -1.0f, kMezzNorth, {-15.0f, kMezzY, 3.5f}, false},
    {"west lower", -24.0f, -6.0f, 2.0f, kHubWest, -4.0f, 22.0f, {-20.0f, kWestLowY, 10.0f}, true},
    {"east hall", kHubEast, -2.0f, kCanalNorth, 24.0f, -0.5f, 40.0f, {12.0f, kFloorY, 31.0f}, true},
    {"canal bed", 15.0f, kCanalBed - 0.5f, kCanalSouth, 24.0f, kCanalBed + 0.7f, kCanalNorth, {23.2f, kCanalBed, 25.15f}, false},
    {"cellar", -9.5f, kWestLowY - 0.5f, kCellarWestZ0, kCellarEastX1, kCellarY + 0.3f, kCanalSouth, {0.0f, kCellarY, 15.0f}, false},
    {"east quay", 15.0f, -0.5f, 22.0f, 24.0f, 1.5f, kCanalSouth, {16.5f, kQuayY, 23.4f}, false},
    {"east upper", kCourtEast, 2.0f, 2.0f, 24.0f, 7.0f, 23.0f, {20.0f, kEastHighY, 6.0f}, false},
    {"north hall", -9.0f, -2.0f, kHubNorth, 9.0f, 8.0f, 60.0f, {0.0f, kFloorY, 42.0f}, false},
    {"gallery", kCourtEast, 0.5f, 23.0f, kHubEast, 2.5f, 34.0f, {8.0f, kGalleryY, 28.0f}, false},
    {"graveyard", kGraveX0, kGraveY - 0.5f, kGraveZ0, kGraveX1, kGraveY + 1.0f, kGraveZ1, {9.0f, kGraveY, 93.5f}, false},
    {"graveyard lane", kGraveLaneX0, kGraveY - 0.5f, kBackWallZ, kGraveLaneX1, kGraveY + 1.0f, kGraveZ0, {9.0f, kGraveY, 89.0f}, false},
    {"theatre terrace", -13.0f, kPromenadeY - 0.5f, kBackWallZ + 0.5f, -7.0f, kPromenadeY + 0.5f, kTerraceZ1, {-10.0f, kPromenadeY, 88.8f}, false},
    {"theatre", -kTheatreX, kFloorY - 0.5f, kStageWallZ, kTheatreX, kPromenadeY + 0.5f, kBackWallZ, {11.0f, kFloorY, 73.0f}, false},
    {"theatre lane", kTheatreDoorX0, kFloorY - 0.5f, 60.5f, kTheatreDoorX1, kStageY + 0.5f, kStageWallZ, {4.0f, kFloorY, 61.2f}, false},
    {"well", kShaftX0, kWellFloor - 0.5f, kShaftZ0, kShaftX1, kFloorY - 0.3f, kShaftZ1, {kShaftX0 + 0.9f, kLandingSW, kShaftZ0 + 0.9f}, false},
    {"market", kMarketX0, kFloorY - 0.5f, kMarketZ0, kMarketX1, kFloorY + 3.0f, kMarketZ1, {-36.0f, kFloorY, 34.6f}, false},
    {"west lane", kMarketX1, kFloorY - 0.5f, kLaneZ0, -24.5f, kFloorY + 1.5f, kLaneZ1, {-28.0f, kFloorY, 34.6f}, false},
    {"water tower", kTowerRoomX0, kTowerDoorSill - 0.5f, kTowerRoomZ0, kTowerRoomX1, kTowerDoorSill + 0.5f, kBasinZ0, {59.5f, kTowerDoorSill, 23.6f}, false},
    {"tower lock", kTowerLockX0, kTowerLockBed - 0.5f, kBasinZ0, kLockEast, kTowerLockBed + 0.5f, kLockWalkZ0, {57.0f, kTowerLockBed, 26.0f}, true},
    {"lock walk", 24.0f, kTerraceFloor[0] - kBasinDepth - 0.5f, kBasinZ0, kLockEast, kTerraceFloor[3] + 1.5f, kLockWalkZ1, {26.5f, kFloorY, 34.5f}, false},
    {"quay", kQuayX0, kHarbourY - 0.5f, kQuayZ0, kQuayX1, kHarbourY + 0.5f, kQuayZ1, {6.0f, kHarbourY, -52.5f}, false},
    {"sea stair", -kSeaStairHalf, kHarbourY - 0.5f, kQuayZ1, kSeaStairHalf, kTerraceY + 0.5f, kTerraceSouth, {0.0f, kTerraceY, -26.0f}, false},
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
inline constexpr int kStartRoom = RoomIndex("quay"); ///< beat 0: the quay of the harbour

/// The fall line of a room: a body of that room whose feet are below it has fallen (it goes back to the room entry or to the
/// lit anchor lamp of the room). Each room has its own, so that a deep room is never taken for a fall (docs/LEVEL_DESIGN.md,
/// 14.7). Below every floor of the room by kFallBelowRoom, because the bottom of a room is kept close to its floors.
constexpr float FallLine(int room)
{
    return room >= 0 && room < kRoomCount ? kRooms[room].y0 - kFallBelowRoom : kFallY;
}

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
    Span(57.0f, 31.0f, 19.75f, 62.0f, 35.0f, 24.75f, Rgb{0.72f, 0.66f, 0.56f}, kLayerA),             // east: the tank of the water tower (A)
};
inline constexpr int kLandmarkCount = static_cast<int>(sizeof(kLandmarks) / sizeof(kLandmarks[0]));

// ---- what is drawn and never solid and never moves: the steps over the ramp down of the west wing, the water of the trench,
// the roofs over the wing corridors (scene/Shapes.h). Static: baked with kStageStatics (render/StaticBatch.h).
/// The water in the trench of the west hall (W1): layer A only. A body that goes under it comes back at the entry of the room;
/// an orb floats on it.
inline constexpr shapes::Water kWaters[] = {
    {-24.0f, kTrenchSouth, -15.0f, kTrenchNorth, kFloorY - 0.5f, 3.0f},                              // the trench (W1)
    {kPoolWest, kPoolSouth, kPoolEast, kPoolNorth, kFloorY - 0.45f, kFloorY - 0.45f - kSquareLowY}, // the pool of the square
};
inline constexpr int kTrenchWater = 0;
inline constexpr int kPoolWater = 1;
/// The water of the cellar (A): the canal at its high level, in its four legs. In B the cellar is dry.
inline constexpr shapes::Water kCellarWaters[] = {
    {kCellarEastX0, kCellarZ0, kCellarEastX1, kCanalSouth, kCellarWater, kCellarWater - kCellarY},    // east leg
    {kCellarSouthX0, kCellarZ0, kCellarEastX0, kCellarZ1, kCellarWater, kCellarWater - kCellarY},     // middle leg
    {kCellarSouthX0, kCellarWestZ0, kCellarSouthX1, kCellarZ0, kCellarWater, kCellarWater - kWestLowY}, // south leg
    {-9.5f, kCellarWestZ0, kCellarSouthX0, kCellarWestZ1, kCellarWater, kCellarWater - kWestLowY},     // west leg
};

/// The water of the three locks of the lock walk (A): 0.6 m under each terrace, down to its bed.
inline constexpr shapes::Water kLockWaters[kLockCount] = {
    {kTerraceX0[0], kBasinZ0, kTerraceX1[0], kLockWalkZ0, kTerraceFloor[0] - kLockWaterBelow, kBasinDepth - kLockWaterBelow},
    {kTerraceX0[1], kBasinZ0, kTerraceX1[1], kLockWalkZ0, kTerraceFloor[1] - kLockWaterBelow, kBasinDepth - kLockWaterBelow},
    {kTerraceX0[2], kBasinZ0, kTerraceX1[2], kLockWalkZ0, kTerraceFloor[2] - kLockWaterBelow, kBasinDepth - kLockWaterBelow},
};

/// The water of the great well (A): 3 m deep over its floor.
inline constexpr shapes::Water kWellWaters[] = {
    {kShaftX0, kShaftZ0, kShaftX1, kShaftZ1, kWellWater, kWellWater - kWellFloor},
};

/// The water of the tower lock (A), 2 m deep over its bed.
inline constexpr shapes::Water kTowerLockWaters[] = {
    {kTowerLockX0, kBasinZ0, kLockEast, kLockWalkZ0, kTowerLockWater, kTowerLockWater - kTowerLockBed},
};

/// The canal of the east district (scene/Canal.h): the crate floats 0.5 m out of the water; low, its top is 2.4 m under the
/// quay (out of reach), high, 1.2 m under it (the step of the main path) and level with the hall floor.
inline canal::Desc MakeCanal()
{
    canal::Desc d;
    d.x0 = 15.0f;
    d.x1 = 24.0f;
    d.z0 = kCanalSouth;
    d.z1 = kCanalNorth;
    d.bed = kCanalBed;
    d.lowTop = kFloorY - 1.7f;  // -2.7
    d.highTop = kFloorY - 0.5f; // -1.5
    d.wheel = {16.0f, kFloorY + 1.1f, 33.5f};
    d.crateX = 20.0f;
    d.crateZ = 0.5f * (kCanalSouth + kCanalNorth);
    d.crateSize = 1.2f;
    d.draft = 0.5f;
    d.weir = {15.7f, kQuayY + 1.1f, 23.2f}; // on the quay, at its west end
    d.driftX = kDriftX;
    return d;
}

// ---- where the water comes from (docs/LEVEL_DESIGN.md, 14.4): every water of A is fed, through a chain of feeds, by the
// aqueduct that comes into the city from the east. In B the storm has broken the aqueduct, so all of it is dry. A feed runs
// downhill: the water it fills lies lower than what it comes from. tests/test_stage.cpp checks the chain and the heights.
inline constexpr float kAqueductZ0 = 36.4f;   ///< the channel, along x, over the hall of the east district and the east rim
inline constexpr float kAqueductZ1 = 37.2f;
inline constexpr float kAqueductBottom = kHubWallTop + 0.2f; ///< 9.2: it rests on the walls, out of reach of every floor
inline constexpr float kAqueductTop = kAqueductBottom + 0.8f;
inline constexpr float kAqueductWest = 6.4f;  ///< its spout, over the pool
inline constexpr float kAqueductEast = 44.0f; ///< where it comes in from the hills
inline constexpr float kAqueductBreakX0 = 30.5f; ///< in B this part has fallen: 5 m, more than a running jump
inline constexpr float kAqueductBreakX1 = 35.5f;
inline constexpr float kAqueductPiers[] = {30.0f, 36.0f, 42.0f};
inline constexpr Rgb kAqueductStone{0.60f, 0.57f, 0.52f};
inline constexpr Rgb kWaterRgb{shapes::kWaterColour.r, shapes::kWaterColour.g, shapes::kWaterColour.b};
struct WaterFeed
{
    const char* water; ///< "the trench", "the pool", "the canal"
    const char* from;  ///< "the aqueduct" or another water
    float top;         ///< world y of the highest surface of the water it fills
};
inline constexpr WaterFeed kWaterFeeds[] = {
    {"the pool", "the aqueduct", kFloorY - 0.45f},   // the spout over the east rim falls into the pool
    {"the trench", "the pool", kFloorY - 0.5f},      // a drain under the arcade, out through a culvert into the west trench
    {"the canal", "the aqueduct", kFloorY - 0.5f},   // the sluice of the lock, opened by the wheel
    {"the cellar", "the canal", kCellarWater},        // through the sluice panel under the quay
    {"the tower lock", "the aqueduct", kFloorY + 5.0f},   // the outflow of the water tower
    {"the upper lock", "the tower lock", kFloorY + 4.0f - 0.6f}, // through the lock head
    {"the middle lock", "the upper lock", kFloorY + 2.0f - 0.6f}, // each lock fills the one below it
    {"the lower lock", "the middle lock", kFloorY - 0.6f},
    {"the well", "the trench", kWellWater},          // a drain from the trench of the west hall, under the lane
};
inline constexpr int kWaterFeedCount = static_cast<int>(sizeof(kWaterFeeds) / sizeof(kWaterFeeds[0]));

/// The aqueduct as drawn boxes that never change: the channel in three parts (the middle one in A only), the piers, and the
/// mouth of the culvert in the west trench. The water in them is drawn by AddWaterDraw (it drains at the end).
inline void AqueductDecor(std::vector<SceneObject>& drawn)
{
    const float b = kAqueductBottom;
    const float t = kAqueductTop;
    const float z0 = kAqueductZ0;
    const float z1 = kAqueductZ1;
    const float parts[3][2] = {{kAqueductWest, kAqueductBreakX0}, {kAqueductBreakX0, kAqueductBreakX1}, {kAqueductBreakX1, kAqueductEast}};
    for (int i = 0; i < 3; ++i)
    {
        const unsigned mask = i == 1 ? kLayerA : kLayerBoth;
        const Rgb c = i == 1 ? kWood : kAqueductStone; // (the part of A: a timber trough that the storm took)
        drawn.push_back(Span(parts[i][0], b, z0, parts[i][1], b + 0.2f, z1, c, mask));
        drawn.push_back(Span(parts[i][0], b + 0.2f, z0, parts[i][1], t, z0 + 0.15f, c, mask));
        drawn.push_back(Span(parts[i][0], b + 0.2f, z1 - 0.15f, parts[i][1], t, z1, c, mask));
    }
    for (float x : kAqueductPiers)
    {
        drawn.push_back(Span(x - 0.4f, kFloorY, z0, x + 0.4f, b, z1, kAqueductStone, kLayerBoth));
    }
    drawn.push_back(Span(kAqueductEast - 1.0f, kFloorY, z0 - 0.6f, kAqueductEast + 1.0f, t + 1.0f, z1 + 0.6f, kAqueductStone, kLayerBoth));
    // the culvert in the east wall of the west trench (its drain runs under the arcade from the pool)
    drawn.push_back(Span(-15.02f, kFloorY - 1.0f, 26.4f, -15.0f, kFloorY - 0.3f, 27.6f, kDark, kLayerBoth));
}

// ---- the cathedral, drawn only: a rose window over the portal and a spire on the north wall, the highest point of the
// map, seen from every district. Above every place a body can reach inside (the spire starts 13 m up), so the camera never
// enters it.
inline constexpr float kBelfryY = 17.0f;  ///< UNITS: metres (world y): the floor of the belfry (A) and the broken top (B)
inline constexpr float kSpireTop = 28.5f; ///< UNITS: metres (world y): over the bell tower (24.5) and under the beacon (30)
inline constexpr Rgb kRose{1.00f, 0.72f, 0.40f};
inline void CathedralDecor(std::vector<SceneObject>& drawn)
{
    // the rose window: a square frame, a cross and a centre, glowing, just in front of the south wall
    const float cx = 0.0f;
    const float cy = 7.0f;
    const float z0 = kTowerSouth - 0.06f;
    const float z1 = kTowerSouth;
    const float h = 1.5f;  // half of its size
    const float b = 0.18f; // the width of a bar
    const SceneObject bars[] = {
        Span(cx - h, cy + h - b, z0, cx + h, cy + h, z1, kRose, kLayerBoth),         Span(cx - h, cy - h, z0, cx + h, cy - h + b, z1, kRose, kLayerBoth),
        Span(cx - h, cy - h + b, z0, cx - h + b, cy + h - b, z1, kRose, kLayerBoth), Span(cx + h - b, cy - h + b, z0, cx + h, cy + h - b, z1, kRose, kLayerBoth),
        Span(cx - h + b, cy - 0.5f * b, z0, cx + h - b, cy + 0.5f * b, z1, kRose, kLayerBoth),
        Span(cx - 0.5f * b, cy - h + b, z0, cx + 0.5f * b, cy + h - b, z1, kRose, kLayerBoth),
        Span(cx - 0.45f, cy - 0.45f, z0 - 0.02f, cx + 0.45f, cy + 0.45f, z1, kGold, kLayerBoth),
    };
    for (SceneObject o : bars)
    {
        o.glow = 0.7f;
        drawn.push_back(o);
    }
    // the bell tower of the city (docs/LEVEL_DESIGN.md 6.6): on the north wall of the cathedral, a collar and a shaft (both
    // worlds); in A a belfry of four piers with the great bell, then three storeys, each narrower, and a gold tip; in B the
    // shaft ends at the belfry in broken teeth, and the great bell lies cracked on top of it. The ring of the memory lights
    // round the shaft is drawn each frame (StageLights AddLightRing)
    const float sz = 0.5f * (kTowerInnerNorth + kTowerNorth); // over the middle of the north wall
    drawn.push_back(Span(-1.5f, kTowerTop, kTowerInnerNorth, 1.5f, 13.0f, kTowerNorth, kTowerStone, kLayerBoth));
    drawn.push_back(Span(-1.6f, 13.0f, sz - 1.6f, 1.6f, kBelfryY, sz + 1.6f, kTowerStone, kLayerBoth));
    for (int k = 0; k < 4; ++k) // A: the belfry
    {
        const float px = k % 2 == 0 ? -1.6f : 1.2f;
        const float pz = sz + (k < 2 ? -1.6f : 1.2f);
        drawn.push_back(Span(px, kBelfryY, pz, px + 0.4f, kBelfryY + 4.0f, pz + 0.4f, kTowerStone, kLayerA));
    }
    SceneObject bell = Span(-0.8f, kBelfryY + 0.8f, sz - 0.8f, 0.8f, kBelfryY + 2.8f, sz + 0.8f, kGold, kLayerA);
    bell.glow = 0.5f;
    drawn.push_back(bell);
    const float halves[] = {1.8f, 1.2f, 0.8f, 0.45f};
    const float tops[] = {kBelfryY + 4.5f, 24.5f, 26.5f, 27.5f};
    float bottom = kBelfryY + 4.0f;
    for (int i = 0; i < 4; ++i)
    {
        drawn.push_back(Span(-halves[i], bottom, sz - halves[i], halves[i], tops[i], sz + halves[i], kTowerStone, kLayerA));
        bottom = tops[i];
    }
    SceneObject tip = Span(-0.2f, bottom, sz - 0.2f, 0.2f, kSpireTop, sz + 0.2f, kGold, kLayerA);
    tip.glow = 0.4f;
    drawn.push_back(tip);
    for (int k = 0; k < 4; ++k) // B: broken teeth and the cracked bell lying on the stump
    {
        const float px = k % 2 == 0 ? -1.6f : 0.6f;
        const float pz = sz + (k < 2 ? -1.6f : 0.6f);
        drawn.push_back(Span(px, kBelfryY, pz, px + 1.0f, kBelfryY + 0.6f + 0.5f * static_cast<float>(k), pz + 1.0f, Rgb{0.47f, 0.48f, 0.49f}, kLayerB));
    }
    SceneObject cracked = Span(-0.9f, kBelfryY, sz - 0.6f, 0.7f, kBelfryY + 1.2f, sz + 0.9f, Rgb{0.62f, 0.50f, 0.30f}, kLayerB);
    cracked.tilt = 0.5f;
    drawn.push_back(cracked);
}
inline constexpr int kWaterCount = static_cast<int>(sizeof(kWaters) / sizeof(kWaters[0]));

// ---- the silhouette of the city, drawn only (a greybox pass: scale and outline, not detail)
// Battlements: merlons on the top of every thin stone wall of both layers that ends at the height of the city walls or above,
// generated from kStageStatics (no list kept by hand). A merlon that would stand in or on any other box of the map (a floor
// laid on the wall, a lintel, the wall next to it) is left out, so none pokes through a walkway. They sit on wall tops that
// no body can reach (tests/test_reach.cpp), so being drawn only changes nothing a body can do.
inline constexpr float kMerlonWidth = 0.5f;   ///< UNITS: metres along the wall
inline constexpr float kMerlonSpacing = 1.1f; ///< UNITS: metres from one merlon to the next
inline constexpr float kMerlonHeight = 0.6f;  ///< UNITS: metres
inline constexpr float kMerlonWallThin = 0.6f; ///< walls up to this thick get merlons (a thicker one is a walkway: the east path)

constexpr bool SameRgb(const SceneObject& o, Rgb c)
{
    return o.colorSrgb[0] == c.r && o.colorSrgb[1] == c.g && o.colorSrgb[2] == c.b;
}
/// True when the axis-aligned boxes overlap with a positive volume (a touch is not an overlap).
inline bool BoxesOverlap(const SceneObject& a, const SceneObject& b)
{
    for (int k = 0; k < 3; ++k)
    {
        if (std::fabs(a.position[k] - b.position[k]) >= 0.5f * (a.scale[k] + b.scale[k]) - 1.0e-4f)
        {
            return false;
        }
    }
    return true;
}
/// True when `o` gets merlons.
inline bool HasBattlements(const SceneObject& o)
{
    const float top = o.position[1] + 0.5f * o.scale[1];
    const float thin = o.scale[0] < o.scale[2] ? o.scale[0] : o.scale[2];
    const float along = o.scale[0] < o.scale[2] ? o.scale[2] : o.scale[0];
    return o.shape == ShapeKind::Cube && o.tilt == 0.0f && o.layerMask == kLayerBoth && o.checker < 0.5f && SameRgb(o, kStone) &&
           o.scale[1] >= 2.0f && top >= kCityWallTop - 0.1f && thin <= kMerlonWallThin && along >= 1.5f;
}
inline void Battlements(std::vector<SceneObject>& drawn)
{
    for (const SceneObject& w : kStageStatics)
    {
        if (!HasBattlements(w))
        {
            continue;
        }
        const bool alongX = w.scale[0] >= w.scale[2];
        const float length = alongX ? w.scale[0] : w.scale[2];
        const int n = static_cast<int>(std::floor((length - kMerlonWidth) / kMerlonSpacing)) + 1;
        const float first = -0.5f * static_cast<float>(n - 1) * kMerlonSpacing; // centred on the wall
        const float top = w.position[1] + 0.5f * w.scale[1];
        for (int i = 0; i < n; ++i)
        {
            const float c = first + static_cast<float>(i) * kMerlonSpacing;
            const float x = alongX ? w.position[0] + c : w.position[0];
            const float z = alongX ? w.position[2] : w.position[2] + c;
            const float hx = alongX ? 0.5f * kMerlonWidth : 0.5f * w.scale[0];
            const float hz = alongX ? 0.5f * w.scale[2] : 0.5f * kMerlonWidth;
            const SceneObject m = Span(x - hx, top, z - hz, x + hx, top + kMerlonHeight, z + hz, kStone, kLayerBoth);
            // the test box reaches a little below the wall top (a floor laid exactly on it counts as in the way) and 2 m over
            // it (a merlon under a ceiling, such as the ledge over the north hall, would look like a mistake)
            SceneObject probe = Span(x - hx, top - 0.05f, z - hz, x + hx, top + 2.0f, z + hz, kStone, kLayerBoth);
            bool clear = true;
            for (const SceneObject& o : kStageStatics)
            {
                if (&o != &w && (o.layerMask & kLayerBoth) != 0u && BoxesOverlap(probe, o))
                {
                    clear = false;
                    break;
                }
            }
            if (clear)
            {
                SceneObject placed = m;
                // B: the storm threw down about one merlon in four (the same ones every run)
                const unsigned hash = (static_cast<unsigned>(std::lround(x * 10.0f)) * 73856093u) ^ (static_cast<unsigned>(std::lround(z * 10.0f)) * 19349663u);
                placed.layerMask = (hash >> 4) % 4u == 0u ? kLayerA : kLayerBoth;
                drawn.push_back(placed);
            }
        }
    }
}

// ---- walls in three parts (docs/LEVEL_DESIGN.md, 5.7): every city wall that has battlements gets a plinth at its foot and a
// cornice under its top, drawn only. In B the cornice is broken: one piece in three is gone.
inline constexpr float kPlinthHeight = 0.5f;  ///< UNITS: metres
inline constexpr float kPlinthProud = 0.06f;  ///< UNITS: metres out of each face
inline constexpr float kCorniceHeight = 0.25f;
inline constexpr float kCorniceProud = 0.12f;
inline constexpr float kCorniceBelowTop = 0.1f; ///< UNITS: metres from the top of the wall to the top of the cornice
inline constexpr float kCornicePiece = 1.5f;    ///< UNITS: metres: the cornice of B is laid in pieces this long
inline constexpr Rgb kCorniceStone{0.60f, 0.61f, 0.64f};

inline void WallParts(std::vector<SceneObject>& drawn)
{
    for (const SceneObject& w : kStageStatics)
    {
        if (!HasBattlements(w))
        {
            continue;
        }
        const bool alongX = w.scale[0] >= w.scale[2];
        const float x0 = w.position[0] - 0.5f * w.scale[0];
        const float x1 = w.position[0] + 0.5f * w.scale[0];
        const float z0 = w.position[2] - 0.5f * w.scale[2];
        const float z1 = w.position[2] + 0.5f * w.scale[2];
        const float bottom = w.position[1] - 0.5f * w.scale[1];
        const float top = w.position[1] + 0.5f * w.scale[1];
        const float px = alongX ? 0.0f : kPlinthProud;
        const float pz = alongX ? kPlinthProud : 0.0f;
        drawn.push_back(Span(x0 - px, bottom, z0 - pz, x1 + px, bottom + kPlinthHeight, z1 + pz, kCorniceStone, kLayerBoth));
        const float cx = alongX ? 0.0f : kCorniceProud;
        const float cz = alongX ? kCorniceProud : 0.0f;
        const float cy1 = top - kCorniceBelowTop;
        const float cy0 = cy1 - kCorniceHeight;
        drawn.push_back(Span(x0 - cx, cy0, z0 - cz, x1 + cx, cy1, z1 + cz, kCorniceStone, kLayerA)); // A: whole
        const float length = alongX ? x1 - x0 : z1 - z0;
        const int pieces = static_cast<int>(std::ceil(length / kCornicePiece));
        for (int k = 0; k < pieces; ++k)
        {
            const unsigned hash = static_cast<unsigned>(std::lround((x0 + z0) * 7.0f)) + static_cast<unsigned>(k) * 2654435761u;
            if ((hash >> 7) % 3u == 0u)
            {
                continue; // B: this piece has fallen
            }
            const float a = static_cast<float>(k) * length / static_cast<float>(pieces);
            const float b = static_cast<float>(k + 1) * length / static_cast<float>(pieces);
            if (alongX)
            {
                drawn.push_back(Span(x0 + a, cy0, z0 - cz, x0 + b, cy1, z1 + cz, kCorniceStone, kLayerB));
            }
            else
            {
                drawn.push_back(Span(x0 - cx, cy0, z0 + a, x1 + cx, cy1, z0 + b, kCorniceStone, kLayerB));
            }
        }
    }
}

// The skyline: the rest of the city round and between the parts that are played, as blocks of houses with roofs and a few
// towers, drawn only. It fills every plot that is far enough from the map, so the map reads as a quarter of a city and not as
// boxes in the void; the fog of B swallows most of it, which is the storm. A plot is left empty when it comes within
// kSkylineFromRoom of a room (a running jump is 3.6 m: a body that jumps off an open edge never lands in a house it would
// fall through) or within kSkylineFromBox of any box of the map or a landmark. A fixed seed: the same city every run.
inline constexpr float kSkylineFromRoom = 4.5f; ///< UNITS: metres
inline constexpr float kSkylineFromBox = 1.0f;  ///< UNITS: metres
inline constexpr float kSkylineX0 = -62.0f;     ///< the area that is filled
inline constexpr float kSkylineX1 = 62.0f;
inline constexpr float kSkylineZ0 = -40.0f;
inline constexpr float kSkylineZ1 = 100.0f;
inline constexpr float kSkylineCell = 7.0f;     ///< UNITS: metres: one plot, a street of kSkylineStreet included
inline constexpr float kSkylineStreet = 1.2f;
inline constexpr Rgb kHouse{0.50f, 0.47f, 0.44f};

// the places of the districts that are drawn round the map (scene/Scenery.h, docs/LEVEL_DESIGN.md 5.8): no house stands there
struct Zone
{
    float x0, z0, x1, z1;
    const char* name;
};
inline constexpr float kWaterTowerX = 59.5f;
inline constexpr float kWaterTowerZ = 19.5f;
inline constexpr Zone kZones[] = {
    {-62.0f, 30.0f, -34.0f, 60.0f, "the market"},
    {-16.0f, 60.0f, 16.0f, 106.0f, "the theatre and the graveyard"},
    {53.0f, 12.0f, 64.0f, 25.0f, "the water tower"},
};

/// The horizontal gap between two footprints (0 when they overlap).
inline float FootprintGap(float ax0, float az0, float ax1, float az1, float bx0, float bz0, float bx1, float bz1)
{
    const float dx = ax1 < bx0 ? bx0 - ax1 : (bx1 < ax0 ? ax0 - bx1 : 0.0f);
    const float dz = az1 < bz0 ? bz0 - az1 : (bz1 < az0 ? az0 - bz1 : 0.0f);
    return std::sqrt(dx * dx + dz * dz);
}

/// True when a house may stand on the footprint (x0 .. x1, z0 .. z1).
inline bool PlotIsFree(float x0, float z0, float x1, float z1)
{
    for (const Room& r : kRooms)
    {
        if (FootprintGap(x0, z0, x1, z1, r.x0, r.z0, r.x1, r.z1) < kSkylineFromRoom)
        {
            return false;
        }
    }
    const auto clearOf = [&](const SceneObject& o, float margin) {
        // a turned box (a ramp) is measured by the box round it
        const float c = std::fabs(std::cos(o.tilt));
        const float sn = std::fabs(std::sin(o.tilt));
        const float hx = o.shape == ShapeKind::Torus ? (scene::kTorusMajor + scene::kTorusMinor) * o.scale[0] : 0.5f * o.scale[0];
        const float hz = o.shape == ShapeKind::Torus ? hx : 0.5f * (o.scale[1] * sn + o.scale[2] * c);
        return FootprintGap(x0, z0, x1, z1, o.position[0] - hx, o.position[2] - hz, o.position[0] + hx, o.position[2] + hz) >= margin;
    };
    for (const SceneObject& o : kStageStatics)
    {
        if (!clearOf(o, kSkylineFromBox))
        {
            return false;
        }
    }
    for (const SceneObject& o : kLandmarks)
    {
        if (!clearOf(o, kSkylineFromBox))
        {
            return false;
        }
    }
    for (const Zone& zone : kZones) // the districts drawn round the map
    {
        if (FootprintGap(x0, z0, x1, z1, zone.x0, zone.z0, zone.x1, zone.z1) < 0.5f)
        {
            return false;
        }
    }
    // nothing stands where the ground drops into a pit (scene/Landscape.h): it would float over it
    for (const Pit& p : kPits)
    {
        if (FootprintGap(x0, z0, x1, z1, p.x0, p.z0, p.x1, p.z1) < kPitReach + 0.5f)
        {
            return false;
        }
    }
    // the aqueduct crosses the roofs: no house stands under it
    return FootprintGap(x0, z0, x1, z1, kAqueductWest, kAqueductZ0 - 0.6f, kAqueductEast + 1.0f, kAqueductZ1 + 0.6f) >= kSkylineFromBox;
}
/// Turns objects v[from ..] about the vertical line through (cx, cz) by `yaw` (RotationY on a row vector: x' = x c + z s,
/// z' = -x s + z c), each one turning about its own centre as well: the group turns as one piece.
inline void TurnAbout(std::vector<SceneObject>& v, std::size_t from, float cx, float cz, float yaw)
{
    const float c = std::cos(yaw);
    const float sn = std::sin(yaw);
    for (std::size_t i = from; i < v.size(); ++i)
    {
        SceneObject& o = v[i];
        const float dx = o.position[0] - cx;
        const float dz = o.position[2] - cz;
        o.position[0] = cx + dx * c + dz * sn;
        o.position[2] = cz - dx * sn + dz * c;
        o.yaw += yaw;
    }
}

/// The axis-aligned box round `o` (its tilt round X and its yaw round Y taken into account).
inline void AxisBounds(const SceneObject& o, float lo[3], float hi[3])
{
    const float hx = 0.5f * o.scale[0];
    const float hy = 0.5f * o.scale[1];
    const float hz = 0.5f * o.scale[2];
    const float ct = std::fabs(std::cos(o.tilt));
    const float st = std::fabs(std::sin(o.tilt));
    const float ey = hy * ct + hz * st; // after the tilt
    const float ez0 = hy * st + hz * ct;
    const float cy = std::fabs(std::cos(o.yaw));
    const float sy = std::fabs(std::sin(o.yaw));
    const float ex = hx * cy + ez0 * sy; // after the yaw
    const float ez = hx * sy + ez0 * cy;
    const float e[3] = {ex, ey, ez};
    for (int k = 0; k < 3; ++k)
    {
        lo[k] = o.position[k] - e[k];
        hi[k] = o.position[k] + e[k];
    }
}

// The houses of the town (docs/LEVEL_DESIGN.md, 5.3 and 5.5): the town grows round its cathedral, so every house is turned to
// the street that runs round it (its walls face the cathedral or run round it, within a few degrees), and its colours follow
// the quarter it is in: ochre plaster in the west, whitewash in the east, grey stone in the north, sandstone in the south,
// blended by distance so that no quarter has a border. In B one house in four is a shell: no roof, the walls broken lower.
// A plot that is not built is a garden (trees: scene/Landscape.h).
enum class PlotKind
{
    House,
    Tower,
    Garden,
};

struct Plot
{
    float x0, z0, x1, z1; ///< the plot, without its street
    PlotKind kind;
    float height;      ///< UNITS: metres of the walls
    float towerSize;   ///< half the width of a tower
    float towerHeight; ///< over the roofs
    float yaw;         ///< radians: the turn of the house on its plot
    float side;        ///< UNITS: metres: the side of the house, so that it fits its plot when turned
    bool ruin;         ///< B: a shell
};

inline constexpr float kCathedralX = 0.0f; ///< the middle of the cathedral, which the streets run round
inline constexpr float kCathedralZ = 31.0f;

/// Every plot of the town (fixed seed: the same town every run).
inline std::vector<Plot> SkylinePlots()
{
    std::vector<Plot> plots;
    unsigned state = 2026u;
    const auto next = [&state]() {
        state = state * 1664525u + 1013904223u;
        return static_cast<float>((state >> 8) & 0xFFFFu) / 65535.0f;
    };
    constexpr float kQuarter = 1.5707963f;
    for (float x = kSkylineX0; x < kSkylineX1; x += kSkylineCell)
    {
        for (float z = kSkylineZ0; z < kSkylineZ1; z += kSkylineCell)
        {
            Plot p{};
            p.x0 = x + 0.5f * kSkylineStreet;
            p.z0 = z + 0.5f * kSkylineStreet;
            p.x1 = x + kSkylineCell - 0.5f * kSkylineStreet;
            p.z1 = z + kSkylineCell - 0.5f * kSkylineStreet;
            const float roll = next(); // drawn for every plot, so that one change of the map does not reshuffle the whole city
            p.height = 3.0f + 6.0f * next();
            const float towerRoll = next();
            p.towerSize = 1.2f + 0.6f * next();
            p.towerHeight = 5.0f + 6.0f * next();
            const float jitter = next();
            p.ruin = next() < 0.25f;
            if (!PlotIsFree(p.x0, p.z0, p.x1, p.z1))
            {
                continue;
            }
            const float cx = 0.5f * (p.x0 + p.x1);
            const float cz = 0.5f * (p.z0 + p.z1);
            // the direction to the cathedral, folded into -45 .. 45 degrees (a square house looks the same turned by 90)
            float yaw = std::atan2(cx - kCathedralX, cz - kCathedralZ);
            yaw = yaw - kQuarter * std::floor(yaw / kQuarter + 0.5f);
            p.yaw = yaw + (jitter - 0.5f) * 0.14f;
            const float inner = p.x1 - p.x0;
            p.side = inner / (std::fabs(std::cos(p.yaw)) + std::fabs(std::sin(p.yaw))); // its turned corners stay on the plot
            p.kind = roll < 0.12f ? PlotKind::Garden : (towerRoll < 0.15f ? PlotKind::Tower : PlotKind::House);
            plots.push_back(p);
        }
    }
    return plots;
}

/// The colour of the walls and of the roof of a house at (x, z): the four quarters blended by distance.
inline void QuarterColours(float x, float z, Rgb& wall, Rgb& roof)
{
    struct Quarter
    {
        float x, z;
        Rgb wall, roof;
    };
    static constexpr Quarter kQuarters[] = {
        {-36.0f, 31.0f, {0.66f, 0.55f, 0.42f}, {0.62f, 0.27f, 0.19f}}, // west: ochre plaster, deep red tiles
        {38.0f, 31.0f, {0.70f, 0.69f, 0.65f}, {0.70f, 0.42f, 0.28f}},  // east: whitewash, light tiles
        {0.0f, 75.0f, {0.52f, 0.52f, 0.52f}, {0.38f, 0.40f, 0.45f}},   // north: grey stone, slate
        {0.0f, -25.0f, {0.62f, 0.56f, 0.47f}, {0.58f, 0.29f, 0.21f}},  // south: sandstone, red tiles
    };
    float w = 0.0f;
    wall = {0.0f, 0.0f, 0.0f};
    roof = {0.0f, 0.0f, 0.0f};
    for (const Quarter& q : kQuarters)
    {
        const float d2 = (x - q.x) * (x - q.x) + (z - q.z) * (z - q.z);
        const float k = std::exp(-d2 / (2.0f * 26.0f * 26.0f)) + 1.0e-6f;
        w += k;
        wall = {wall.r + k * q.wall.r, wall.g + k * q.wall.g, wall.b + k * q.wall.b};
        roof = {roof.r + k * q.roof.r, roof.g + k * q.roof.g, roof.b + k * q.roof.b};
    }
    wall = {wall.r / w, wall.g / w, wall.b / w};
    roof = {roof.r / w, roof.g / w, roof.b / w};
}

inline void Skyline(std::vector<SceneObject>& drawn)
{
    for (const Plot& p : SkylinePlots())
    {
        if (p.kind == PlotKind::Garden)
        {
            continue;
        }
        const float cx = 0.5f * (p.x0 + p.x1);
        const float cz = 0.5f * (p.z0 + p.z1);
        const float h = 0.5f * p.side;
        Rgb wall{};
        Rgb roof{};
        QuarterColours(cx, cz, wall, roof);
        const std::size_t first = drawn.size();
        if (p.ruin)
        {
            drawn.push_back(Span(cx - h, kFloorY, cz - h, cx + h, kFloorY + p.height, cz + h, wall, kLayerA));
            drawn.push_back(Span(cx - h, kFloorY, cz - h, cx + h, kFloorY + 0.55f * p.height, cz + h, wall, kLayerB)); // a shell in B
        }
        else
        {
            drawn.push_back(Span(cx - h, kFloorY, cz - h, cx + h, kFloorY + p.height, cz + h, wall, kLayerBoth));
        }
        if (p.kind == PlotKind::Tower) // now and then a tower over the roofs
        {
            drawn.push_back(Span(cx - p.towerSize, kFloorY, cz - p.towerSize, cx + p.towerSize, kFloorY + p.height + p.towerHeight, cz + p.towerSize, kStone,
                                 kLayerBoth));
        }
        else
        {
            const std::size_t roofFirst = drawn.size();
            shapes::Roof(cx - h, cx + h, cz - h, cz + h, kFloorY + p.height, kFloorY + p.height + 0.3f * p.side + 0.4f, drawn);
            for (std::size_t i = roofFirst; i < drawn.size(); ++i)
            {
                drawn[i].colorSrgb[0] = roof.r;
                drawn[i].colorSrgb[1] = roof.g;
                drawn[i].colorSrgb[2] = roof.b;
                drawn[i].layerMask = p.ruin ? kLayerA : kLayerBoth;
            }
        }
        TurnAbout(drawn, first, cx, cz, p.yaw);
    }
}

// ---- footings (docs/LEVEL_DESIGN.md, 5.4): where the ground drops into a pit, a floor or a wall of the city that stands at the
// edge of it gets a footing of rock down to the floor of the pit, drawn only, so that nothing of the city floats: a wall that
// stands at street level and a deck (a floor, the end of a bridge). Not under a box that stands on another box, never through
// another box (a floor under a landing), and never through a room that lies below it (the cellar under the courtyard).
inline constexpr Rgb kFootingRock{0.42f, 0.40f, 0.37f};

inline void Footings(std::vector<SceneObject>& drawn)
{
    for (const SceneObject& o : kStageStatics)
    {
        if (o.shape != ShapeKind::Cube || o.tilt != 0.0f || o.yaw != 0.0f || o.layerMask != kLayerBoth || o.glow > 0.0f)
        {
            continue;
        }
        const float x0 = o.position[0] - 0.5f * o.scale[0];
        const float x1 = o.position[0] + 0.5f * o.scale[0];
        const float z0 = o.position[2] - 0.5f * o.scale[2];
        const float z1 = o.position[2] + 0.5f * o.scale[2];
        const float bottom = o.position[1] - 0.5f * o.scale[1];
        if (bottom < kPitY + 1.0f || (bottom > kFloorY + 0.05f && o.scale[1] > 0.5f))
        {
            continue; // only what stands at street level, and decks (a lintel over an arch is carried by its walls)
        }
        bool atPit = false;
        for (const Pit& p : kPits)
        {
            atPit = atPit || FootprintGap(x0, z0, x1, z1, p.x0, p.z0, p.x1, p.z1) < kPitReach;
        }
        if (!atPit)
        {
            continue;
        }
        bool standsOnBox = false; // on another box of both layers: that one carries it
        for (const SceneObject& t : kStageStatics)
        {
            if (&t == &o || t.tilt != 0.0f || (t.layerMask & kLayerBoth) != kLayerBoth)
            {
                continue;
            }
            const float top = t.position[1] + 0.5f * t.scale[1];
            const bool under = std::fabs(top - bottom) < 0.05f && FootprintGap(x0, z0, x1, z1, t.position[0] - 0.5f * t.scale[0], t.position[2] - 0.5f * t.scale[2],
                                                                               t.position[0] + 0.5f * t.scale[0], t.position[2] + 0.5f * t.scale[2]) <= 0.0f &&
                               std::fabs(o.position[0] - t.position[0]) < 0.5f * (o.scale[0] + t.scale[0]) - 1.0e-3f &&
                               std::fabs(o.position[2] - t.position[2]) < 0.5f * (o.scale[2] + t.scale[2]) - 1.0e-3f;
            standsOnBox = standsOnBox || under;
        }
        if (standsOnBox)
        {
            continue;
        }
        bool roomBelow = false;
        for (const Room& r : kRooms)
        {
            roomBelow = roomBelow || (r.y1 < bottom - 0.3f && x0 < r.x1 && x1 > r.x0 && z0 < r.z1 && z1 > r.z0);
        }
        const SceneObject footing = Span(x0, kPitY, z0, x1, bottom, z1, kFootingRock, kLayerBoth);
        bool throughBox = false; // a footing never passes through another box of the map (a floor under a landing)
        for (const SceneObject& t : kStageStatics)
        {
            throughBox = throughBox || (&t != &o && t.tilt == 0.0f && BoxesOverlap(footing, t));
        }
        if (!roomBelow && !throughBox)
        {
            drawn.push_back(footing);
        }
    }
}

/// A carved tile at the entry of every room that has no anchor lamp: the place where a body comes back is a place, not a
/// point on an empty floor. Both layers, drawn only, flush on the floor.
inline void EntryTiles(std::vector<SceneObject>& drawn)
{
    for (int i = 0; i < kRoomCount; ++i)
    {
        bool anchored = false;
        for (const AnchorDesc& a : kAnchors)
        {
            anchored = anchored || RoomIndex(a.room) == i;
        }
        if (anchored)
        {
            continue;
        }
        const Vec3 e = kRooms[i].entry;
        SceneObject tile = Span(e.x - 0.6f, e.y, e.z - 0.6f, e.x + 0.6f, e.y + 0.02f, e.z + 0.6f, kAnchorStone, kLayerBoth);
        tile.glow = 0.05f;
        drawn.push_back(tile);
    }
}

// ---- the courtyard as a cloister (docs/LEVEL_DESIGN.md, 6.1): the yard of the old customs house. The grid of the puzzle is
// its flagstones; the walls of A are the goods of the warehouse, stacked crates (whole: made by people); the walls of B are the
// masonry of the arcade that fell into the yard. Its own walls carry a blind arcade: pilasters, arched heads and a string
// course, a few centimetres proud of the wall (drawn only, never in the way of a body or the crate). In B the string course is
// broken and moss grows in some joints of the flagstones.
inline constexpr float kJointWidth = 0.035f;  ///< UNITS: metres: the joints of the flagstones
inline constexpr float kJointHeight = 0.008f; ///< over the floor
inline constexpr float kArcadeProud = 0.06f;  ///< UNITS: metres the blind arcade stands out of the wall
inline constexpr float kArcadeTop = kFloorY + 3.6f; ///< the top of the arched heads (under the string course)
inline constexpr float kArcadeBay = 2.0f;      ///< UNITS: metres between two pilasters
inline constexpr Rgb kJoint{0.36f, 0.35f, 0.33f};
inline constexpr Rgb kMoss{0.30f, 0.38f, 0.24f};
inline constexpr Rgb kCrateEdge{0.36f, 0.25f, 0.15f};
inline constexpr Rgb kArcadeStone{shapes::kStoneLight.r, shapes::kStoneLight.g, shapes::kStoneLight.b};

/// One wall face of the arcade: along Z at x = `face` (`inward` +1 when the yard is toward +x) or along X at z = `face`, from a0
/// to a1, leaving out the stretch skip0 .. skip1 (a doorway).
inline void BlindArcade(bool alongX, float face, float inward, float a0, float a1, float skip0, float skip1, std::vector<SceneObject>& drawn)
{
    const float t0 = inward > 0.0f ? face : face - kArcadeProud;
    const float t1 = inward > 0.0f ? face + kArcadeProud : face;
    const auto box = [&](float lo, float y0, float hi, float y1, float proud, const Rgb& c, unsigned mask) {
        const float u0 = inward > 0.0f ? face : face - proud;
        const float u1 = inward > 0.0f ? face + proud : face;
        drawn.push_back(alongX ? Span(lo, y0, u0, hi, y1, u1, c, mask) : Span(u0, y0, lo, u1, y1, hi, c, mask));
    };
    const int bays = static_cast<int>(std::lround((a1 - a0) / kArcadeBay));
    const float bay = (a1 - a0) / static_cast<float>(bays > 0 ? bays : 1);
    for (int k = 0; k <= bays; ++k)
    {
        const float at = a0 + bay * static_cast<float>(k);
        if (at + 0.15f > skip0 && at - 0.15f < skip1)
        {
            continue;
        }
        box(at - 0.15f, kFloorY, at + 0.15f, kArcadeTop, kArcadeProud, kArcadeStone, kLayerBoth); // a pilaster
    }
    for (int k = 0; k < bays; ++k)
    {
        const float lo = a0 + bay * static_cast<float>(k) + 0.15f;
        const float hi = a0 + bay * static_cast<float>(k + 1) - 0.15f;
        if (hi > skip0 && lo < skip1)
        {
            continue;
        }
        shapes::ArchHead(alongX, lo, hi, t0, t1, kArcadeTop, 0.7f, drawn);
    }
    // the string course over the arcade: whole in A; in B laid in pieces, one in three fallen
    box(a0, kArcadeTop, a1, kArcadeTop + 0.2f, kArcadeProud + 0.04f, kArcadeStone, kLayerA);
    const int pieces = static_cast<int>(std::ceil((a1 - a0) / 1.0f));
    for (int k = 0; k < pieces; ++k)
    {
        if ((k * 7 + static_cast<int>(face * 3.0f)) % 3 == 0)
        {
            continue;
        }
        const float lo = a0 + (a1 - a0) * static_cast<float>(k) / static_cast<float>(pieces);
        const float hi = a0 + (a1 - a0) * static_cast<float>(k + 1) / static_cast<float>(pieces);
        box(lo, kArcadeTop, hi, kArcadeTop + 0.2f, kArcadeProud + 0.04f, kArcadeStone, kLayerB);
    }
}

inline void CourtyardDecor(std::vector<SceneObject>& drawn)
{
    // the flagstones: a joint on every line of the grid of the puzzle
    for (int i = 0; i <= courtyard::kWidth; ++i)
    {
        const float x = kCourtWest + static_cast<float>(i);
        drawn.push_back(Span(x - 0.5f * kJointWidth, kFloorY, kCourtSouth, x + 0.5f * kJointWidth, kFloorY + kJointHeight, kCourtNorth, kJoint, kLayerBoth));
    }
    for (int j = 0; j <= courtyard::kRowCount; ++j)
    {
        const float z = kCourtSouth + static_cast<float>(j);
        drawn.push_back(Span(kCourtWest, kFloorY, z - 0.5f * kJointWidth, kCourtEast, kFloorY + kJointHeight, z + 0.5f * kJointWidth, kJoint, kLayerBoth));
    }
    // B: moss in the joints round a few flagstones that no wall stands on
    for (int z = 0; z < courtyard::kRowCount; ++z)
    {
        for (int x = 0; x < courtyard::kWidth; ++x)
        {
            if (RowCell(x, z) != '.' || (x * 5 + z * 3) % 7 != 0)
            {
                continue;
            }
            const float cx = static_cast<float>(x + kCellX0);
            const float cz = static_cast<float>(z + kCellZ0);
            drawn.push_back(Span(cx - 0.5f, kFloorY, cz - 0.5f, cx + 0.5f, kFloorY + kJointHeight + 0.002f, cz - 0.5f + 0.08f, kMoss, kLayerB));
            drawn.push_back(Span(cx - 0.5f, kFloorY, cz - 0.5f, cx - 0.5f + 0.08f, kFloorY + kJointHeight + 0.002f, cz + 0.5f, kMoss, kLayerB));
        }
    }
    // the walls of the puzzle: A, stacked crates (a joint at every crate, 1 m apart, round each face); B, a broken top
    for (const Run& r : kRuns)
    {
        const SceneObject w = WallOf(r);
        const float x0 = w.position[0] - 0.5f * w.scale[0];
        const float x1 = w.position[0] + 0.5f * w.scale[0];
        const float z0 = w.position[2] - 0.5f * w.scale[2];
        const float z1 = w.position[2] + 0.5f * w.scale[2];
        const float p = 0.015f;
        if (r.kind == 'R')
        {
            for (float y = kFloorY + 1.0f; y < kFloorY + kWallHeight - 0.1f; y += 1.0f)
            {
                drawn.push_back(Span(x0 - p, y - 0.03f, z0 - p, x1 + p, y + 0.03f, z1 + p, kCrateEdge, kLayerA));
            }
            for (float z = z0 + 1.0f; z < z1 - 0.1f; z += 1.0f)
            {
                drawn.push_back(Span(x0 - p, kFloorY, z - 0.03f, x1 + p, kFloorY + kWallHeight + p, z + 0.03f, kCrateEdge, kLayerA));
            }
        }
        else
        {
            for (float z = z0; z < z1 - 0.1f; z += 1.0f)
            {
                const float h = 0.25f + 0.2f * static_cast<float>((static_cast<int>(z * 10.0f) + r.x * 3) % 3);
                drawn.push_back(Span(x0 + 0.15f, kFloorY + kWallHeight, z + 0.1f, x1 - 0.2f, kFloorY + kWallHeight + h, z + 0.8f, kDebris, kLayerB));
            }
        }
    }
    // the blind arcade on the west, east and north walls of the yard
    BlindArcade(false, kCourtWest, 1.0f, kCourtSouth, kCourtNorth, 0.0f, 0.0f, drawn);
    BlindArcade(false, kCourtEast, -1.0f, kCourtSouth, kCourtNorth, 0.0f, 0.0f, drawn);
    BlindArcade(true, kCourtNorth, -1.0f, kCourtWest, kCourtEast, kDoorX - kDoorwayHalfWidth - 0.2f, kDoorX + kDoorwayHalfWidth + 0.2f, drawn);
}

// ---- the market (6.3.1): what is drawn only, all of it out of reach or flat: the top of the clock tower (A: piers, a roof
// slab, the day bell, its faces; B: broken) and the posts of the stalls (A: wood; B: grey, bare)
inline void MarketDecor(std::vector<SceneObject>& drawn)
{
    const float y = kFloorY;
    const float tx = -40.0f;
    const float tz = 55.0f;
    drawn.push_back(Span(tx - 2.5f, y + 12.0f, tz - 2.5f, tx + 2.5f, y + 18.0f, tz + 2.5f, kStone, kLayerA));
    for (int k = 0; k < 4; ++k)
    {
        const float px = tx + (k % 2 == 0 ? -2.2f : 1.6f);
        const float pz = tz + (k < 2 ? -2.2f : 1.6f);
        drawn.push_back(Span(px, y + 18.0f, pz, px + 0.6f, y + 21.0f, pz + 0.6f, kStone, kLayerA));
    }
    drawn.push_back(Span(tx - 2.8f, y + 21.0f, tz - 2.8f, tx + 2.8f, y + 21.6f, tz + 2.8f, kStone, kLayerA));
    SceneObject bell = Span(tx - 1.1f, y + 18.6f, tz - 1.1f, tx + 1.1f, y + 20.6f, tz + 1.1f, Rgb{0.70f, 0.55f, 0.28f}, kLayerA);
    bell.glow = 0.2f;
    drawn.push_back(bell);
    const Rgb face{0.86f, 0.74f, 0.42f};
    drawn.push_back(Span(tx - 1.1f, y + 13.5f, tz - 2.56f, tx + 1.1f, y + 15.7f, tz - 2.5f, face, kLayerA));
    drawn.push_back(Span(tx - 1.1f, y + 13.5f, tz + 2.5f, tx + 1.1f, y + 15.7f, tz + 2.56f, face, kLayerA));
    drawn.push_back(Span(tx - 2.56f, y + 13.5f, tz - 1.1f, tx - 2.5f, y + 15.7f, tz + 1.1f, face, kLayerA));
    drawn.push_back(Span(tx + 2.5f, y + 13.5f, tz - 1.1f, tx + 2.56f, y + 15.7f, tz + 1.1f, face, kLayerA));
    drawn.push_back(Span(tx - 2.5f, y + 12.0f, tz - 2.5f, tx + 2.5f, y + 13.2f, tz + 2.5f, Rgb{0.47f, 0.48f, 0.49f}, kLayerB)); // B: broken
    const float stalls[][3] = {{-58.0f, 33.0f, 0.0f}, {-52.0f, 33.0f, 0.0f}, {-44.0f, 33.0f, 0.0f}, {-38.0f, 33.0f, 0.0f},
                               {-58.0f, 57.0f, 0.0f}, {-52.0f, 57.0f, 0.0f}, {-60.0f, 41.0f, 1.0f}, {-60.0f, 47.0f, 1.0f}};
    for (const auto& st : stalls)
    {
        const bool alongZ = st[2] > 0.5f;
        const float hx = alongZ ? 0.9f : 1.6f;
        const float hz = alongZ ? 1.6f : 0.9f;
        for (int k = 0; k < 4; ++k) // a post on each corner of the counter, from its top to the awning
        {
            const float px = st[0] + (k % 2 == 0 ? -hx : hx - 0.12f);
            const float pz = st[1] + (k < 2 ? -hz : hz - 0.12f);
            drawn.push_back(Span(px, y + 0.95f, pz, px + 0.12f, y + 2.4f, pz + 0.12f, kWood, kLayerA));
            drawn.push_back(Span(px, y + 0.95f, pz, px + 0.12f, y + 2.0f, pz + 0.12f, Rgb{0.30f, 0.25f, 0.21f}, kLayerB));
        }
    }
}

// ---- the theatre (6.5.1), drawn only: the upper stage wall with its columns (A; broken in B), the painted scenery of the
// evening's play against the stage wall (A), grass in the orchestra (B)
inline void TheatreDecor(std::vector<SceneObject>& drawn)
{
    drawn.push_back(Span(-kTheatreX - 0.5f, kTheatreWallTop, kStageWallZ, kTheatreX + 0.5f, kTheatreWallTop + 4.0f, kStageWallZ + 0.6f, kStone, kLayerA));
    for (float x = -12.0f; x <= 12.1f; x += 3.0f)
    {
        drawn.push_back(Span(x - 0.25f, kStageY, kStageWallZ + 0.6f, x + 0.25f, kTheatreWallTop + 3.5f, kStageWallZ + 0.9f, kArcadeStone, kLayerA));
    }
    drawn.push_back(Span(-kTheatreX - 0.5f, kTheatreWallTop, kStageWallZ, -6.0f, kTheatreWallTop + 1.5f, kStageWallZ + 0.6f, Rgb{0.47f, 0.48f, 0.49f},
                         kLayerB));
    const Rgb paint[3] = {{0.56f, 0.44f, 0.62f}, {0.40f, 0.52f, 0.66f}, {0.72f, 0.58f, 0.36f}};
    for (int k = 0; k < 3; ++k)
    {
        const float x = -9.0f + 3.0f * static_cast<float>(k);
        drawn.push_back(Span(x, kStageY, kStageWallZ + 0.6f, x + 2.4f, kStageY + 3.2f - 0.4f * static_cast<float>(k), kStageWallZ + 0.66f, paint[k], kLayerA));
    }
    drawn.push_back(Span(-5.0f, kFloorY, kStageFront + 1.0f, 4.0f, kFloorY + 0.02f, kRowsZ0 - 0.5f, Rgb{0.36f, 0.44f, 0.28f}, kLayerB));
}

// ---- the graveyard (6.5.2), drawn only: the headstones on the rows (A: with fresh flowers), the top of the night bell tower
// (its open arch, the bell, a roof in A), the stone lantern on the lone grave (B)
inline void GraveyardDecor(std::vector<SceneObject>& drawn)
{
    for (float z : {94.5f, 97.0f, 99.5f})
    {
        for (float x = -9.4f; x < 4.0f; x += 1.7f)
        {
            const int k = static_cast<int>((x + 10.0f) * 3.0f + z);
            drawn.push_back(Span(x - 0.35f, kGraveY + 0.35f, z - 0.08f, x + 0.35f, kGraveY + 1.2f + 0.1f * static_cast<float>(k % 3), z + 0.08f, kGrey,
                                 k % 7 == 0 ? kLayerA : kLayerBoth));
            if (k % 3 == 1)
            {
                drawn.push_back(Span(x - 0.2f, kGraveY + 0.35f, z - 0.3f, x + 0.2f, kGraveY + 0.5f, z - 0.1f, Rgb{0.86f, 0.38f, 0.42f}, kLayerA));
            }
        }
    }
    drawn.push_back(Span(8.5f, kGraveY + 9.0f, 100.5f, 12.0f, kGraveY + 12.0f, 104.0f, kStone, kLayerBoth));
    const std::size_t roof = drawn.size();
    shapes::Roof(8.2f, 12.3f, 100.2f, 104.3f, kGraveY + 12.0f, kGraveY + 14.5f, drawn);
    for (std::size_t i = roof; i < drawn.size(); ++i)
    {
        drawn[i].layerMask = kLayerA;
    }
    SceneObject bell = Span(10.0f, kGraveY + 1.4f, 100.1f, 11.0f, kGraveY + 2.3f, 100.5f, kGold, kLayerBoth); // the night bell, in its arch
    bell.glow = 0.4f;
    drawn.push_back(bell);
    drawn.push_back(Span(-10.2f, kGraveY + 0.3f, 103.1f, -9.8f, kGraveY + 1.2f, 103.4f, kGrey, kLayerB));
    SceneObject lantern = Span(-10.15f, kGraveY + 1.2f, 103.1f, -9.85f, kGraveY + 1.5f, 103.4f, Rgb{1.0f, 0.80f, 0.40f}, kLayerB);
    lantern.glow = 0.6f;
    drawn.push_back(lantern);
}

// ---- the water tower (6.4.2), drawn only: its shaft over the room of the bell (A: to the bell stage and the tank, a
// landmark; B: broken at 20 m), the dawn bell hanging low in the room
inline void WaterTowerDecor(std::vector<SceneObject>& drawn)
{
    const float x = 0.5f * (kTowerRoomX0 + kTowerRoomX1);
    const float z = 0.5f * (kTowerRoomZ0 + kBasinZ0 - 0.5f);
    drawn.push_back(Span(x - 2.6f, kTowerRoomTop + 0.5f, z - 2.6f, x + 2.6f, 27.0f, z + 2.6f, kStone, kLayerA));
    for (int k = 0; k < 4; ++k)
    {
        const float px = x + (k % 2 == 0 ? -2.4f : 1.8f);
        const float pz = z + (k < 2 ? -2.4f : 1.8f);
        drawn.push_back(Span(px, 27.0f, pz, px + 0.6f, 31.0f, pz + 0.6f, kStone, kLayerA));
    }
    drawn.push_back(Span(x - 2.6f, kTowerRoomTop + 0.5f, z - 2.6f, x + 2.6f, 20.0f, z + 2.6f, Rgb{0.47f, 0.48f, 0.49f}, kLayerB));
    for (int k = 0; k < 4; ++k) // B: broken teeth
    {
        const float px = x + (k % 2 == 0 ? -2.6f : 1.2f);
        const float pz = z + (k < 2 ? -2.6f : 1.2f);
        drawn.push_back(Span(px, 20.0f, pz, px + 1.4f, 20.6f + static_cast<float>(k), pz + 1.4f, Rgb{0.47f, 0.48f, 0.49f}, kLayerB));
    }
    SceneObject bell = Span(kDawnBell.x - 0.7f, kDawnBell.y + 2.2f, kDawnBell.z - 0.7f, kDawnBell.x + 0.7f, kDawnBell.y + 3.4f, kDawnBell.z + 0.7f, kGold,
                            kLayerBoth);
    bell.glow = 0.4f;
    drawn.push_back(bell);
    drawn.push_back(Span(kDawnBell.x - 0.04f, kDawnBell.y + 3.4f, kDawnBell.z - 0.04f, kDawnBell.x + 0.04f, kTowerRoomTop, kDawnBell.z + 0.04f, kIron, kLayerBoth));
}

inline std::vector<SceneObject> BuildDecor()
{
    std::vector<SceneObject> solid;
    std::vector<SceneObject> drawn;
    shapes::Stairs(-22.0f, -18.0f, 22.0f, kFloorY, 12.0f, kWestLowY, shapes::kStoneLight, solid, drawn); // the ramp itself is in kRamps
    shapes::Stairs(kStairsX0, kStairsX1, 56.0f, kBridgeY, 42.0f, kFloorY, shapes::kStoneLight, solid, drawn); // (the slope: kPieces)
    shapes::Roof(-15.0f, kHubWest - 0.5f, kWingDoorZ0 - 0.5f, kWingDoorZ1 + 0.5f, kCityWallTop, kCityWallTop + 1.0f, drawn);
    shapes::Roof(kHubEast + 0.5f, 15.0f, kWingDoorZ0 - 0.5f, kWingDoorZ1 + 0.5f, kCityWallTop, kCityWallTop + 1.0f, drawn);
    PoolStairs(solid, drawn); // (the slopes: kPieces)
    DitchStairs(solid, drawn);
    CanalStairs(solid, drawn);
    shapes::Roof(-3.0f, 3.0f, kTerraceSouth, kGuardNorth, kGuardWallTop, kGuardWallTop + 1.6f, drawn); // the guardhouse
    EntryTiles(drawn);
    AqueductDecor(drawn);
    CathedralDecor(drawn);
    CourtyardDecor(drawn);
    WellStairs(solid, drawn); // (the slopes: kPieces)
    TheatreStairs(solid, drawn);
    TheatreDecor(drawn);
    GraveyardDecor(drawn);
    WaterTowerDecor(drawn);
    MarketDecor(drawn);
    Battlements(drawn);
    WallParts(drawn);
    // arched heads in the openings of the square into the wings (drawn only, above every head)
    shapes::ArchHead(false, kWingDoorZ0, kWingDoorZ1, kHubWest - 0.5f, kHubWest, kOpeningTop, 0.6f, drawn);
    shapes::ArchHead(false, kWingDoorZ0, kWingDoorZ1, kHubEast, kHubEast + 0.5f, kOpeningTop, 0.6f, drawn);
    shapes::ArchHead(true, -1.0f, 1.0f, kHubNorth, kHubNorth + 0.5f, kOpeningTop, 0.6f, drawn);
    Footings(drawn);
    Skyline(drawn);
    return drawn;
}
inline const std::vector<SceneObject> kDecor = BuildDecor();

/// The static part of the drawn table: kStageStatics, then kDecor. BuildDrawTable writes them first, in this order.
inline int StaticDrawCount()
{
    return kStageStaticCount + static_cast<int>(kDecor.size());
}

// ---- the water of A, and the end (docs/LEVEL_DESIGN.md, 14.10): on the balcony at the top of the cathedral stands the master
// sluice of the aqueduct. Reaching the goal closes it: the aqueduct runs dry, the waterfall stops, and every water of A drains
// to its bed in kDrainSeconds - the two faces of the city meet. Until then every water is full.
class Waterworks
{
  public:
    static constexpr float kDrainSeconds = 4.0f; ///< UNITS: seconds from full to the bed

    void Close()
    {
        m_closed = true;
    }
    bool Closed() const
    {
        return m_closed;
    }
    /// The master sluice opened again (the last moment played once more): every water full.
    void Reopen()
    {
        m_closed = false;
        m_drained = 0.0f;
    }
    void Update(float dt)
    {
        if (m_closed && dt > 0.0f && m_drained < 1.0f)
        {
            m_drained = m_drained + dt / kDrainSeconds > 1.0f ? 1.0f : m_drained + dt / kDrainSeconds;
        }
    }
    /// 0 = full, 1 = every water on its bed.
    float Drained() const
    {
        return m_drained;
    }
    /// UNITS: metres the waters stand over their usual surface (the dawn bell opens the master sluice: scene/CityState.h).
    void SetRaise(float metres)
    {
        m_raise = metres < 0.0f ? 0.0f : metres;
    }
    float Raise() const
    {
        return m_raise;
    }
    /// The surface of `w` now: raised by the dawn bell, drained toward its bed at the end.
    float Top(const shapes::Water& w) const
    {
        const float top = w.top + m_raise;
        const float bed = w.top - w.depth;
        return top - m_drained * (top - bed);
    }

  private:
    float m_drained = 0.0f;
    float m_raise = 0.0f;
    bool m_closed = false;
};

/// Calls `f(water)` for every static water of A (the trench, the pool, the legs of the cellar).
template <class F>
void ForEachWater(F f)
{
    for (const shapes::Water& w : kWaters)
    {
        f(w);
    }
    for (const shapes::Water& w : kCellarWaters)
    {
        f(w);
    }
    for (const shapes::Water& w : kLockWaters)
    {
        f(w);
    }
    for (const shapes::Water& w : kWellWaters)
    {
        f(w);
    }
    for (const shapes::Water& w : kTowerLockWaters)
    {
        f(w);
    }
}

/// True when a body whose feet are at `feet` in `layer` is under water.
inline bool Submerged(const Vec3& feet, int layer, const Waterworks& works)
{
    if (works.Drained() >= 1.0f)
    {
        return false; // drained: what is left is a bed
    }
    bool under = false;
    ForEachWater([&](const shapes::Water& w) {
        shapes::Water now = w;
        now.top = works.Top(w);
        under = under || shapes::Submerged(now, feet, layer);
    });
    return under;
}

/// The same with every water full (before the end).
inline bool Submerged(const Vec3& feet, int layer)
{
    return Submerged(feet, layer, Waterworks{});
}

/// Adds the water surfaces (layer A) to a solid table for ORBS (they float; the body is never held by water). Returns the new
/// count.
inline int AddWaterForOrbs(SceneObject* out, int count, int capacity, const Waterworks& works = Waterworks{})
{
    ForEachWater([&](const shapes::Water& w) {
        if (out != nullptr && count >= 0 && count < capacity && works.Drained() < 1.0f)
        {
            shapes::Water now = w;
            now.top = works.Top(w);
            out[count++] = shapes::WaterSurface(now);
        }
    });
    return count;
}

inline constexpr Vec3 kMasterSluice{2.7f, kSummitY + 1.1f, 28.25f}; ///< the wheel on the balcony, beside the goal light
inline constexpr int kWaterDrawCount = static_cast<int>(sizeof(kWaters) / sizeof(kWaters[0])) +
                                       static_cast<int>(sizeof(kCellarWaters) / sizeof(kCellarWaters[0])) + kLockCount + 1 + 1 + 3 + 3;

/// Adds to a DRAWN table the water of A as it is now (the trench, the pool, the cellar; while the aqueduct runs, the water in
/// its channel, the waterfall into the pool and the spill of the culvert) and the master sluice on the balcony (an iron wheel
/// on its post in A, gold once closed; a rusted stump in B). Returns the new count; needs at most kWaterDrawCount places.
inline int AddWaterDraw(const Waterworks& works, SceneObject* out, int count, int capacity)
{
    if (out == nullptr || count < 0 || capacity - count < kWaterDrawCount)
    {
        return count;
    }
    ForEachWater([&](const shapes::Water& w) {
        const float top = works.Top(w);
        if (top - (w.top - w.depth) > 0.02f)
        {
            shapes::Water now = w;
            now.top = top;
            now.depth = top - (w.top - w.depth);
            out[count++] = shapes::WaterBox(now);
        }
    });
    if (!works.Closed())
    {
        const float b = kAqueductBottom;
        SceneObject channel = Span(kAqueductWest, b + 0.2f, kAqueductZ0 + 0.15f, kAqueductEast, kAqueductTop - 0.15f, kAqueductZ1 - 0.15f, kWaterRgb, kLayerA);
        channel.glow = 0.12f;
        out[count++] = channel;
        SceneObject fall = Span(kAqueductWest - 0.8f, kFloorY - 0.45f, kAqueductZ0 + 0.15f, kAqueductWest, b, kAqueductZ1 - 0.15f, kWaterRgb, kLayerA);
        fall.glow = 0.2f;
        out[count++] = fall;
        SceneObject spill = Span(-15.4f, kFloorY - 0.55f, 26.5f, -15.0f, kFloorY - 0.45f, 27.5f, kWaterRgb, kLayerA);
        spill.glow = 0.12f;
        out[count++] = spill;
    }
    const Vec3 m = kMasterSluice;
    SceneObject post = Span(m.x - 0.08f, kSummitY, m.z - 0.08f, m.x + 0.08f, m.y, m.z + 0.08f, kIron, kLayerA);
    post.glow = 0.04f;
    out[count++] = post;
    SceneObject wheel = Span(m.x - 0.05f, m.y - 0.4f, m.z - 0.4f, m.x + 0.05f, m.y + 0.4f, m.z + 0.4f, works.Closed() ? kGold : Rgb{0.42f, 0.44f, 0.48f},
                             kLayerA);
    wheel.glow = works.Closed() ? 0.9f : 0.25f;
    out[count++] = wheel;
    SceneObject stump = Span(m.x - 0.1f, kSummitY, m.z - 0.1f, m.x + 0.1f, kSummitY + 0.5f, m.z + 0.1f, Rgb{0.40f, 0.26f, 0.18f}, kLayerB);
    stump.glow = 0.02f;
    out[count++] = stump;
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

inline constexpr int kMaxDrawObjects = 4000; ///< most of it is the static city (baked into one call per layer)

/// The table that is drawn: the fixed objects (with the same indices as in the puzzle), the crate and its bands, the door while it is
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
    out[n++] = puzzle.BlockObject(); // the crate, with its iron bands
    puzzle.CrateBands(out + n);
    n += puzzle::Puzzle::kCrateBands;
    if (!puzzle.DoorOpen(1))
    {
        out[n++] = kDoor;
    }
    const bool pressed = puzzle.PlatePressed(0) || puzzle.PlatePressed(1);
    const float px = static_cast<float>(kPlateCell.x);
    const float pz = static_cast<float>(kPlateCell.z);
    // the plate: a brass slab let into the flagstones (gold: a thing to use), bright while pressed
    SceneObject plate = Span(px - 0.45f, kFloorY, pz - 0.45f, px + 0.45f, kFloorY + 0.04f, pz + 0.45f, pressed ? 1.0f : 0.72f,
                             pressed ? 0.82f : 0.56f, pressed ? 0.36f : 0.26f, kLayerA);
    plate.shineStrength = 0.8f;
    plate.shininess = 60.0f;
    plate.glow = pressed ? 0.6f : 0.1f;
    out[n++] = plate;
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
