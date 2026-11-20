// Stage.h - the first stage: one room of about two minutes, walked from south to north (+Z), in three parts.
//   1 the wall that vanishes   a red wall of layer A closes the corridor from edge to edge; in layer B it is not there.
//   2 the jump between layers  a 4.4 m gap. A blue stepping stone of B lies in the middle; the landing strip on the other
//                              side exists only in A. No layer can be crossed alone: the body jumps from the stone in B and
//                              switches to A in the air, onto the strip.
//   3 one push for two worlds  a wall with a high doorway. In A a red panel fills the doorway; in B a blue door does, and it
//                              opens only while the red plate (layer A) is pressed. A body in B is not on the plate, so it
//                              cannot hold the door open: the purple block (both layers) can. Pushed onto the plate, the
//                              same block is also the step up to the blue ledge (B) in front of the doorway, 2.4 m above
//                              the floor, out of reach of a jump from the floor. Through the doorway lies the goal.
// Plain data and plain logic, no Windows, no Direct3D: tested without a window (tests/test_stage.cpp), which plays the stage
// with the real walking body and shows that each part is solved by switching and not without it.
// Tables:    the fixed objects (kStageStatics) are what the puzzle copies (scene/Puzzle.h). The table that is DRAWN adds the
//            block, the door while it is closed, the plate and the goal light (BuildDrawTable); the table that is SOLID adds
//            the block and the closed door (Puzzle::Solids). The plate and the goal light are not solid.
// Space:     left-handed, +Y up, the floor at scene::kFloorY (-1). The corridor is 6 m wide (x = -3 .. 3). UNITS: metres.
// Note:      no std::min / std::max here: windows.h may define them as macros (docs/CONVENTIONS.md).
#pragma once

#include "scene/Camera.h" // Vec3
#include "scene/Puzzle.h"
#include "scene/SceneData.h"

#include <cstddef>

namespace dl::stage
{

using scene::kFloorY;
using scene::kLayerA;
using scene::kLayerB;
using scene::kLayerBoth;
using scene::SceneObject;
using scene::ShapeKind;

/// A box of the stage: centre, size, colour (sRGB) and layer mask. checker = 1 for the floors.
constexpr SceneObject Box(float x, float y, float z, float sx, float sy, float sz, float r, float g, float b, unsigned mask,
                          float checker = 0.0f)
{
    SceneObject o{};
    o.shape = ShapeKind::Cube;
    o.position[0] = x;
    o.position[1] = y;
    o.position[2] = z;
    o.scale[0] = sx;
    o.scale[1] = sy;
    o.scale[2] = sz;
    o.colorSrgb[0] = r;
    o.colorSrgb[1] = g;
    o.colorSrgb[2] = b;
    o.checker = checker;
    o.shineStrength = checker > 0.5f ? 0.04f : 0.20f;
    o.shininess = checker > 0.5f ? 16.0f : 40.0f;
    o.layerMask = mask;
    return o;
}

// ---- the measures that the parts are built on (tests/test_stage.cpp checks every claim below)
inline constexpr float kSouthEnd = 6.0f;      // the south floor ends here; the gap begins
inline constexpr float kStoneNear = 7.7f;     // the stepping stone (B): 1.7 m from each side
inline constexpr float kStoneFar = 8.7f;
inline constexpr float kStripNear = 10.4f;    // the landing strip (A only): 4.4 m from the south floor
inline constexpr float kNorthStart = 12.6f;   // the north floor (both layers): 3.9 m from the stone
inline constexpr float kWallZ = 21.0f;        // the wall with the doorway (0.5 m thick)
inline constexpr float kDoorwayLow = 1.4f;    // the sill of the doorway: 2.4 m above the floor
inline constexpr float kDoorwayHigh = 3.6f;   // the top of the doorway
inline constexpr float kGoalZ = 23.0f;        // the goal light, on the floor behind the wall (at the height of the sill)

// colours (sRGB): red = layer A, blue = layer B, grey = both
#define DL_STAGE_GREY 0.30f, 0.33f, 0.40f
#define DL_STAGE_STONE 0.52f, 0.54f, 0.58f
#define DL_STAGE_RED 0.82f, 0.24f, 0.22f
#define DL_STAGE_BLUE 0.22f, 0.46f, 0.90f

// THE FIXED OBJECTS (the indices are the ones that Puzzle::LastBlocker names).
inline constexpr SceneObject kStageStatics[] = {
    // part 1: the south floor (both) and the red wall (A) across it
    Box(0.0f, kFloorY - 0.1f, 1.0f, 6.0f, 0.2f, 10.0f, DL_STAGE_GREY, kLayerBoth, 1.0f),           // 0 south floor, z -4 .. 6
    Box(0.0f, kFloorY + 1.75f, 3.0f, 6.0f, 3.5f, 0.5f, DL_STAGE_RED, kLayerA),                     // 1 the wall that vanishes
    // part 2: the stepping stone (B), the landing strip (A)
    Box(0.0f, kFloorY - 0.1f, 8.2f, 2.0f, 0.2f, 1.0f, DL_STAGE_BLUE, kLayerB),                     // 2 stepping stone, z 7.7 .. 8.7
    Box(0.0f, kFloorY - 0.1f, 11.5f, 6.0f, 0.2f, 2.2f, DL_STAGE_RED, kLayerA, 1.0f),               // 3 landing strip, z 10.4 .. 12.6
    // part 3: the north floor (both), the wall with the doorway, the panel (A), the ledge (B), the floor behind the wall
    Box(0.0f, kFloorY - 0.1f, 16.8f, 6.0f, 0.2f, 8.4f, DL_STAGE_GREY, kLayerBoth, 1.0f),           // 4 north floor, z 12.6 .. 21
    Box(-1.25f, kFloorY + 2.5f, kWallZ, 3.5f, 5.0f, 0.5f, DL_STAGE_STONE, kLayerBoth),             // 5 wall, left of the doorway
    Box(2.5f, kFloorY + 2.5f, kWallZ, 1.0f, 5.0f, 0.5f, DL_STAGE_STONE, kLayerBoth),               // 6 wall, right of the doorway
    Box(1.25f, 3.8f, kWallZ, 1.5f, 0.4f, 0.5f, DL_STAGE_STONE, kLayerBoth),                        // 7 wall, above the doorway
    Box(1.25f, 0.2f, kWallZ, 1.5f, 2.4f, 0.5f, DL_STAGE_STONE, kLayerBoth),                        // 8 wall, below the doorway (top = the sill)
    Box(1.25f, 2.5f, kWallZ, 1.5f, 2.2f, 0.2f, DL_STAGE_RED, kLayerA),                             // 9 the red panel in the doorway
    Box(1.25f, kDoorwayLow - 0.1f, 20.275f, 1.5f, 0.2f, 0.95f, DL_STAGE_BLUE, kLayerB),            // 10 the blue ledge, z 19.8 .. 20.75
    Box(0.0f, kDoorwayLow - 0.1f, 22.625f, 6.0f, 0.2f, 2.75f, DL_STAGE_GREY, kLayerBoth, 1.0f),    // 11 the floor behind the wall, z 21.25 .. 24
    Box(0.0f, kDoorwayLow + 1.5f, 24.25f, 6.0f, 3.0f, 0.5f, DL_STAGE_STONE, kLayerBoth),           // 12 the back wall: the room ends here
};
inline constexpr int kStageStaticCount = static_cast<int>(sizeof(kStageStatics) / sizeof(kStageStatics[0]));

/// The blue door (layer B): the same place as the red panel.
inline constexpr SceneObject kDoor = Box(1.25f, 2.5f, kWallZ, 1.5f, 2.2f, 0.2f, DL_STAGE_BLUE, kLayerB);
/// The red plate (layer A, not solid): drawn flat on the floor of the plate cell.
inline constexpr puzzle::Cell kPlateCell{1, 19};
inline constexpr puzzle::Cell kBlockStart{-1, 15};

#undef DL_STAGE_GREY
#undef DL_STAGE_STONE
#undef DL_STAGE_RED
#undef DL_STAGE_BLUE

/// The puzzle of part 3: the block may use the cells x = -2 .. 2, z = 15 .. 19 (the north floor, away from its edges and
/// from the checkpoint at z = 13.2, so that a body that comes back there is never inside the block).
inline puzzle::StageDesc MakeStage()
{
    puzzle::StageDesc d;
    d.statics = kStageStatics;
    d.staticCount = kStageStaticCount;
    d.blockStart = kBlockStart;
    d.plate = kPlateCell;
    d.door = kDoor;
    d.minCell = {-2, 15};
    d.maxCell = {2, 19};
    return d;
}

// ---- where the body comes back after a fall: the start of the part it has reached
struct Checkpoint
{
    float fromZ; ///< reached when the body stands on the floor at this z or beyond
    Vec3 feet;   ///< where it comes back, standing
};
inline constexpr Checkpoint kCheckpoints[] = {
    {-100.0f, {0.0f, kFloorY, -3.0f}},       // the start
    {4.0f, {0.0f, kFloorY, 5.0f}},           // after the red wall, in front of the gap
    {kNorthStart + 0.5f, {0.0f, kFloorY, 13.2f}}, // on the north floor, south of every cell the block can use
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

/// True when the body stands behind the wall: the stage is done.
inline bool ReachedGoal(const Vec3& feet, bool grounded)
{
    return grounded && feet.z > kWallZ + 0.5f && feet.y > kDoorwayLow - 0.2f;
}

inline constexpr int kMaxDrawObjects = puzzle::Puzzle::kMaxSolids + 4;

/// The table that is drawn: the fixed objects (with the same indices as in the puzzle), the block, the door while it is
/// closed for a body in B, the plate (brighter while pressed) and the goal light. While a refused push flashes, the object
/// that was in the way is drawn in BOTH layers and lighter, because it may be in the layer that is not on the screen.
/// Returns how many objects were written (at most kMaxDrawObjects).
inline int BuildDrawTable(const puzzle::Puzzle& puzzle, SceneObject (&out)[kMaxDrawObjects])
{
    int n = 0;
    for (int i = 0; i < kStageStaticCount; ++i)
    {
        out[n++] = kStageStatics[i];
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
    out[n++] = Box(static_cast<float>(kPlateCell.x), kFloorY + 0.02f, static_cast<float>(kPlateCell.z), 0.9f, 0.04f, 0.9f,
                   pressed ? 1.0f : 0.70f, pressed ? 0.55f : 0.18f, pressed ? 0.45f : 0.16f, kLayerA);
    SceneObject goal{};
    goal.shape = ShapeKind::Sphere;
    goal.position[0] = 1.25f;
    goal.position[1] = kDoorwayLow + 0.9f;
    goal.position[2] = kGoalZ;
    goal.scale[0] = 0.5f;
    goal.scale[1] = 0.5f;
    goal.scale[2] = 0.5f;
    goal.spinSpeed = 1.0f; // it turns with the spin time of the renderer, so the highlight moves
    goal.colorSrgb[0] = 1.0f;
    goal.colorSrgb[1] = 0.86f;
    goal.colorSrgb[2] = 0.40f;
    goal.shineStrength = 1.0f;
    goal.shininess = 120.0f;
    goal.layerMask = kLayerBoth;
    out[n++] = goal;
    return n;
}

/// What the collision can handle (see scene::SceneIsSupported): every stage object is a box with sizes above 0, and every
/// layer mask is A, B or both. Checked when the program is compiled.
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

} // namespace dl::stage
