// Stage.h - the first stage: one place of about two to three minutes, walked from south to north (+Z).
//   1 the gate          on a raised terrace (2.4 m above the courtyard), a red wall of layer A closes the way; in B it is not
//                       there. A hidden light is buried in its top (scene/StageLights.h).
//   2 the gap           a 4.4 m gap in the terrace. A blue stone of B lies in the middle; the strip on the far side exists only
//                       in A. Jump from the stone in B and switch to A in the air. This is also the lesson the finale needs.
//   3 the overlook      the strip ends over the courtyard: the whole puzzle is seen from above before it is touched, with the
//                       outlines of the walls of the other layer (scene/StageHints.h) and the beacon over the goal.
//   4 the courtyard     the block puzzle of scene/Courtyard.h, built cell by cell from the same rows the solver proved: the
//                       block obeys the walls of both layers, the body those of one.
//   5 the finale        the block on the plate opens the blue door (B) of the goal and is the step up onto the low red wall
//                       (A) that ran along the puzzle. Along its top, a jump over the moat; in A there is nothing on the
//                       other side, so the body switches to B in the air and lands on the blue apron before the open door.
// Why it holds:      tests/test_stage.cpp plays every part with the real walking body and checks that each one needs the
//                    layer switch, and that the courtyard cannot be skipped: the walls are too high to climb from the terrace,
//                    from the block or from the low red wall; the apron cannot be reached from the block or the floor; the
//                    door stays closed unless the block is on the plate.
// Tables:     kStageStatics are the fixed objects (the puzzle copies them). The table that is DRAWN adds the block, the
//             closed door, the plate and the goal light (BuildDrawTable); the SOLID table adds the block and the closed door
//             (Puzzle::Solids). The plate and the goal light are not solid.
// Space:      left-handed, +Y up; the courtyard floor at scene::kFloorY (-1), the terrace at kTerraceY (1.4). UNITS: metres.
// Note:       no std::min / std::max here: windows.h may define them as macros (docs/CONVENTIONS.md).
#pragma once

#include "scene/Camera.h" // Vec3
#include "scene/Courtyard.h"
#include "scene/Puzzle.h"
#include "scene/SceneData.h"

#include <array>
#include <cstddef>

namespace dl::stage
{

using scene::kFloorY;
using scene::kLayerA;
using scene::kLayerB;
using scene::kLayerBoth;
using scene::SceneObject;
using scene::ShapeKind;

/// A box of the stage from its two corners (min, max), a colour (sRGB) and a layer mask. checker = 1 for the floors.
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

// ---- colours (sRGB): red = layer A, blue = layer B, grey = both
struct Rgb
{
    float r;
    float g;
    float b;
};
inline constexpr Rgb kGrey{0.30f, 0.33f, 0.40f};
inline constexpr Rgb kStone{0.52f, 0.54f, 0.58f};
inline constexpr Rgb kRed{0.82f, 0.24f, 0.22f};
inline constexpr Rgb kBlue{0.22f, 0.46f, 0.90f};

constexpr SceneObject Span(float x0, float y0, float z0, float x1, float y1, float z1, Rgb c, unsigned mask, float checker = 0.0f)
{
    return Span(x0, y0, z0, x1, y1, z1, c.r, c.g, c.b, mask, checker);
}

// ---- the measures (tests/test_stage.cpp checks what each one is for)
inline constexpr float kTerraceY = kFloorY + 2.4f;   // the top of the raised terrace of parts 1 .. 3
inline constexpr float kSlab = 0.2f;              // the thickness of a floor
inline constexpr float kGateZ = 3.0f;             // part 1: the red gate wall, 3.5 m high
inline constexpr float kGateHeight = 3.5f;
inline constexpr float kTerraceEnd = 6.0f;           // part 2: the terrace ends here, the gap begins
inline constexpr float kStoneNear = 7.7f;         // the blue stone: 1.7 m from each side
inline constexpr float kStoneFar = 8.7f;
inline constexpr float kStripNear = 10.4f;        // the red strip (A only), 4.4 m from the terrace
// the courtyard: grid cell (x, z) of scene/Courtyard.h is the world cell (x + kCellX0, z + kCellZ0), 1 m each
inline constexpr int kCellX0 = -3;
inline constexpr int kCellZ0 = 14;
inline constexpr float kCourtWest = static_cast<float>(kCellX0) - 0.5f;                                  // -3.5
inline constexpr float kCourtEast = static_cast<float>(kCellX0 + courtyard::kWidth) - 0.5f;              // 3.5
inline constexpr float kCourtSouth = static_cast<float>(kCellZ0) - 0.5f;                                 // 13.5: the strip ends here
inline constexpr float kCourtNorth = static_cast<float>(kCellZ0 + courtyard::kRowCount) - 0.5f;          // 21.5
inline constexpr float kWallHeight = 3.5f;        // a wall of the puzzle: too high from the top of the block (1 m + a 2 m jump)
inline constexpr float kFinaleHeight = 2.5f;      // the low red wall: too high from the floor, low enough from the block
inline constexpr float kBoundaryHeight = 5.0f;    // the walls round the courtyard: too high from the terrace (1.4 m + 2 m)
// the finale
inline constexpr float kFinaleTop = kFloorY + kFinaleHeight;  // 1.5: the top of the low red wall
inline constexpr float kApronSouth = kCourtNorth + 2.0f;     // 23.5: a 2 m moat between the wall and the blue apron
inline constexpr float kDoorSouth = 25.0f;                    // the wall with the doorway, 0.5 m thick
inline constexpr float kDoorNorth = 25.5f;
inline constexpr float kDoorwayLow = kFinaleTop;              // the sill: level with the apron and with the top of the low wall
inline constexpr float kDoorwayHigh = 3.7f;
inline constexpr float kDoorwayHalfWidth = 0.7f;
inline constexpr float kRoomNorth = 28.5f;                    // the goal room, 3 m deep

/// The world x of the column of the low red wall: the doorway, the apron and the goal are lined up with it.
inline constexpr float kFinaleX = static_cast<float>(courtyard::kFinaleWallX + kCellX0); // 2
inline constexpr Vec3 kGoal{kFinaleX, kDoorwayLow + 0.9f, 27.0f};                          // the goal light
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

/// The low red wall of the finale: the red run in its column that reaches the north edge.
constexpr bool IsFinaleRun(const Run& r)
{
    return r.kind == 'R' && r.x == courtyard::kFinaleWallX && r.z1 == courtyard::kRowCount - 1;
}

constexpr SceneObject WallOf(const Run& r)
{
    const float x0 = static_cast<float>(r.x + kCellX0) - 0.5f;
    const float z0 = static_cast<float>(r.z0 + kCellZ0) - 0.5f;
    const float z1 = static_cast<float>(r.z1 + kCellZ0) + 0.5f;
    const float height = IsFinaleRun(r) ? kFinaleHeight : kWallHeight;
    return Span(x0, kFloorY, z0, x0 + 1.0f, kFloorY + height, z1, r.kind == 'R' ? kRed : kBlue, r.kind == 'R' ? kLayerA : kLayerB);
}

// ---- the fixed objects that are not walls of the courtyard
inline constexpr SceneObject kFixed[] = {
    // parts 1 and 2: the raised terrace
    Span(-3.0f, kTerraceY - kSlab, -4.0f, 3.0f, kTerraceY, kTerraceEnd, kGrey, kLayerBoth, 1.0f),                        // the terrace
    Span(-3.0f, kTerraceY, kGateZ - 0.25f, 3.0f, kTerraceY + kGateHeight, kGateZ + 0.25f, kRed, kLayerA),             // the gate (A)
    Span(-1.0f, kTerraceY - kSlab, kStoneNear, 1.0f, kTerraceY, kStoneFar, kBlue, kLayerB),                           // the stone (B)
    Span(-3.0f, kTerraceY - kSlab, kStripNear, 3.0f, kTerraceY, kCourtSouth, kRed, kLayerA, 1.0f),                     // the strip (A)
    // part 4: the courtyard (its walls are generated from scene/Courtyard.h and follow this list)
    Span(kCourtWest, kFloorY - kSlab, kCourtSouth, kCourtEast, kFloorY, kCourtNorth, kGrey, kLayerBoth, 1.0f),     // its floor
    Span(kCourtWest, kFloorY, kCourtSouth - 0.5f, kCourtEast, kTerraceY - kSlab, kCourtSouth, kStone, kLayerBoth),  // under the strip
    Span(kCourtWest - 0.5f, kFloorY, kCourtSouth - 0.5f, kCourtWest, kFloorY + kBoundaryHeight, kRoomNorth + 0.5f, kStone,
         kLayerBoth),                                                                                            // west boundary
    Span(kCourtEast, kFloorY, kCourtSouth - 0.5f, kCourtEast + 0.5f, kFloorY + kBoundaryHeight, kRoomNorth + 0.5f, kStone,
         kLayerBoth),                                                                                            // east boundary
    // part 5: the blue apron over the moat, the wall with the doorway, the goal room
    Span(kFinaleX - 1.0f, kDoorwayLow - kSlab, kApronSouth, kFinaleX + 1.0f, kDoorwayLow, kDoorSouth, kBlue, kLayerB), // apron (B)
    Span(kCourtWest, kDoorwayLow - 1.0f, kDoorSouth, kFinaleX - kDoorwayHalfWidth, 5.0f, kDoorNorth, kStone, kLayerBoth), // west of the door
    Span(kFinaleX + kDoorwayHalfWidth, kDoorwayLow - 1.0f, kDoorSouth, kCourtEast, 5.0f, kDoorNorth, kStone, kLayerBoth), // east of it
    Span(kFinaleX - kDoorwayHalfWidth, kDoorwayHigh, kDoorSouth, kFinaleX + kDoorwayHalfWidth, 5.0f, kDoorNorth, kStone,
         kLayerBoth),                                                                                            // above it
    Span(kFinaleX - kDoorwayHalfWidth, kDoorwayLow - 1.0f, kDoorSouth, kFinaleX + kDoorwayHalfWidth, kDoorwayLow, kDoorNorth,
         kStone, kLayerBoth),                                                                                    // the sill
    Span(kCourtWest, kDoorwayLow - kSlab, kDoorNorth, kCourtEast, kDoorwayLow, kRoomNorth, kGrey, kLayerBoth, 1.0f), // goal room
    Span(kCourtWest, kDoorwayLow, kRoomNorth, kCourtEast, 5.0f, kRoomNorth + 0.5f, kStone, kLayerBoth),         // its back wall
};
inline constexpr int kFixedCount = static_cast<int>(sizeof(kFixed) / sizeof(kFixed[0]));
inline constexpr int kStageStaticCount = kFixedCount + kRunCount;

constexpr std::array<SceneObject, kStageStaticCount> BuildStatics()
{
    std::array<SceneObject, kStageStaticCount> all{};
    for (int i = 0; i < kFixedCount; ++i)
    {
        all[static_cast<std::size_t>(i)] = kFixed[i];
    }
    for (int i = 0; i < kRunCount; ++i)
    {
        all[static_cast<std::size_t>(kFixedCount + i)] = WallOf(kRuns[static_cast<std::size_t>(i)]);
    }
    return all;
}

/// THE FIXED OBJECTS (the indices are the ones that Puzzle::LastBlocker names).
inline constexpr std::array<SceneObject, kStageStaticCount> kStageStatics = BuildStatics();

/// The blue door (layer B) in the doorway.
inline constexpr SceneObject kDoor = Span(kFinaleX - kDoorwayHalfWidth, kDoorwayLow, kDoorSouth, kFinaleX + kDoorwayHalfWidth,
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

// ---- where the body comes back after a fall: the start of the part it has reached
struct Checkpoint
{
    float fromZ; ///< reached when the body stands on something at this z or beyond
    Vec3 feet;   ///< where it comes back, standing
};
inline constexpr Checkpoint kCheckpoints[] = {
    {-100.0f, {0.0f, kTerraceY, -3.0f}},               // the start
    {4.0f, {0.0f, kTerraceY, 5.0f}},                   // past the gate, in front of the gap
    {kCourtSouth, {kCourtWest + 0.5f, kFloorY, static_cast<float>(kCellZ0)}}, // the south-west corner of the courtyard: a
                                                    // cell the block can never be in (tests/test_stage.cpp)
};
inline constexpr int kCheckpointCount = static_cast<int>(sizeof(kCheckpoints) / sizeof(kCheckpoints[0]));
inline constexpr float kFallY = kFloorY - 4.0f; ///< UNITS: metres; a body whose feet are this low has fallen

/// The checkpoint for a body standing at `feet`, never below `reached` (a checkpoint, once reached, stays).
inline int CheckpointFor(const Vec3& feet, bool grounded, int reached)
{
    int best = reached < 0 ? 0 : reached;
    if (!grounded)
    {
        return best;
    }
    for (int i = 0; i < kCheckpointCount; ++i)
    {
        if (feet.z >= kCheckpoints[i].fromZ && i > best)
        {
            best = i;
        }
    }
    return best;
}

/// True when one of the three balls of `body` touches the goal light, or the body stands in the goal room. `Body` needs
/// Ball(i), Feet(), Grounded() and GetSettings().radius (scene/Player.h).
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
    const Vec3 feet = body.Feet();
    return body.Grounded() && feet.z > kDoorNorth && feet.y > kDoorwayLow - 0.2f;
}

inline constexpr int kMaxDrawObjects = 128;

/// The table that is drawn: the fixed objects (with the same indices as in the puzzle), the block, the door while it is
/// closed for a body in B, the plate (brighter while pressed) and, until it is taken, the goal light. While a refused push
/// flashes, the object that was in the way is drawn in BOTH layers and lighter, because it may be in the layer that is not
/// on the screen. Returns how many objects were written.
inline int BuildDrawTable(const puzzle::Puzzle& puzzle, bool goalTaken, SceneObject (&out)[kMaxDrawObjects])
{
    int n = 0;
    for (const SceneObject& o : kStageStatics)
    {
        out[n++] = o;
    }
    const float flash = puzzle.BlockedFlash();
    if (flash > 0.0f && puzzle.LastBlocker() >= 0 && puzzle.LastBlocker() < kStageStaticCount)
    {
        SceneObject& b = out[puzzle.LastBlocker()];
        const float t = flash / puzzle::Puzzle::kFlashSeconds; // 1 at the start of the flash, 0 at its end
        for (float& c : b.colorSrgb)
        {
            c += (1.0f - c) * 0.6f * t;
        }
        b.layerMask = kLayerBoth;
    }
    out[n++] = puzzle.BlockObject();
    if (!puzzle.DoorOpen(1))
    {
        out[n++] = kDoor;
    }
    const bool pressed = puzzle.PlatePressed(0) || puzzle.PlatePressed(1);
    const float px = static_cast<float>(kPlateCell.x);
    const float pz = static_cast<float>(kPlateCell.z);
    out[n++] = Span(px - 0.45f, kFloorY, pz - 0.45f, px + 0.45f, kFloorY + 0.04f, pz + 0.45f, pressed ? 1.0f : 0.70f,
                    pressed ? 0.55f : 0.18f, pressed ? 0.45f : 0.16f, kLayerA);
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
        out[n++] = goal;
    }
    return n;
}

/// What the collision and the puzzle can handle: every stage object is a box with sizes above 0, and every layer mask is
/// A, B or both. Checked when the program is compiled.
constexpr bool StageIsSupported()
{
    for (const SceneObject& o : kStageStatics)
    {
        if (o.shape != ShapeKind::Cube || !(o.scale[0] > 0.0f && o.scale[1] > 0.0f && o.scale[2] > 0.0f) || o.layerMask == 0u ||
            (o.layerMask & ~kLayerBoth) != 0u)
        {
            return false;
        }
    }
    return kStageStaticCount <= puzzle::Puzzle::kMaxSolids - 2;
}
static_assert(StageIsSupported(), "scene/Stage.h: every stage object must be a box with sizes above 0 and a layer mask of A, B or both");
static_assert(kPlateCell.x != 0 || kPlateCell.z != 0, "scene/Courtyard.h: the rows must hold a plate 'P'");

} // namespace dl::stage
