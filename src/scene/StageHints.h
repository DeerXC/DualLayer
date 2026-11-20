// StageHints.h - the outlines of the other layer: a hint drawn into the stage, so that the player can read what is on the
//            other side without switching. An object that exists in one layer only leaves a thin outline in the other
//            layer: four narrow bars round its footprint, in its own colour. Looking at the gap in A, the player sees the
//            blue outline of the stepping stone of B hanging in the air; looking at the corridor in B, the red outline of
//            the wall on the floor. No text, no arrow: the hint is the other world itself.
// Where:     a flat object (a platform, a ledge: up to kFlatHeight thick) is outlined at its TOP, where the body would stand;
//            a tall one (a wall) at its BASE, on the floor in front of the player.
// Which:     the objects listed in kGhostIndices, not every single-layer object: the red panel in the doorway is left out,
//            because the blue door already shows the doorway in B.
// Solid:     never. The outlines go into the DRAWN table only (AddGhosts); the solid table (Puzzle::Solids) does not hold
//            them, so the body falls through an outline exactly as through the empty air it marks.
// Plain data and plain logic, no Windows, no Direct3D: tested without a window (tests/test_hints.cpp).
// UNITS: metres.
#pragma once

#include "scene/SceneData.h"
#include "scene/Stage.h"

namespace dl::stage
{

inline constexpr float kGhostBar = 0.06f;    // UNITS: metres, how wide a bar of an outline is
inline constexpr float kGhostThick = 0.03f;  // UNITS: metres, how high a bar is (thin: it must not look like a step)
inline constexpr float kGhostLift = 0.02f;   // UNITS: metres above the surface, so that the bar never flickers with the floor
inline constexpr float kFlatHeight = 0.5f;   // UNITS: metres; an object up to this thick is a platform, outlined at its top
inline constexpr float kGhostLight = 0.35f;  // how much of the way to white the colour goes: the outline glows a little

/// The fixed objects of the stage (indices into kStageStatics) that leave an outline in the other layer.
inline constexpr int kGhostIndices[] = {
    1,  // the red wall of part 1 (A): its base on the floor of B
    2,  // the blue stepping stone of part 2 (B): its top, over the gap of A
    3,  // the red landing strip of part 2 (A): its top, over the gap of B
    10, // the blue ledge of part 3 (B): its top, in front of the doorway of A
};
inline constexpr int kGhostCount = static_cast<int>(sizeof(kGhostIndices) / sizeof(kGhostIndices[0]));
inline constexpr int kBarsPerGhost = 4;

/// Every listed object exists in exactly one layer and is a box: an object of both layers has no "other side".
constexpr bool GhostsAreValid()
{
    for (int index : kGhostIndices)
    {
        if (index < 0 || index >= kStageStaticCount)
        {
            return false;
        }
        const scene::SceneObject& o = kStageStatics[index];
        if (o.shape != scene::ShapeKind::Cube || (o.layerMask != scene::kLayerA && o.layerMask != scene::kLayerB))
        {
            return false;
        }
    }
    return true;
}
static_assert(GhostsAreValid(), "scene/StageHints.h: every ghost must be a box of the stage that exists in exactly one layer");

/// The height at which the outline of `o` is drawn: the top of a flat object, the base of a tall one (plus a small lift).
constexpr float GhostY(const scene::SceneObject& o)
{
    const float half = 0.5f * o.scale[1];
    const float surface = o.scale[1] <= kFlatHeight ? o.position[1] + half : o.position[1] - half;
    return surface + kGhostLift + 0.5f * kGhostThick;
}

/// Writes the four bars of the outline of `o` into out[0 .. 3].
inline void MakeGhost(const scene::SceneObject& o, scene::SceneObject* out)
{
    const unsigned other = o.layerMask == scene::kLayerA ? scene::kLayerB : scene::kLayerA;
    const float y = GhostY(o);
    const float hx = 0.5f * o.scale[0];
    const float hz = 0.5f * o.scale[2];
    const float r = o.colorSrgb[0] + (1.0f - o.colorSrgb[0]) * kGhostLight;
    const float g = o.colorSrgb[1] + (1.0f - o.colorSrgb[1]) * kGhostLight;
    const float b = o.colorSrgb[2] + (1.0f - o.colorSrgb[2]) * kGhostLight;
    const float x = o.position[0];
    const float z = o.position[2];
    // the two long sides along x, then the two short sides along z (between them, so the corners are not drawn twice)
    out[0] = Box(x, y, z - hz + 0.5f * kGhostBar, 2.0f * hx, kGhostThick, kGhostBar, r, g, b, other);
    out[1] = Box(x, y, z + hz - 0.5f * kGhostBar, 2.0f * hx, kGhostThick, kGhostBar, r, g, b, other);
    out[2] = Box(x - hx + 0.5f * kGhostBar, y, z, kGhostBar, kGhostThick, 2.0f * hz - 2.0f * kGhostBar, r, g, b, other);
    out[3] = Box(x + hx - 0.5f * kGhostBar, y, z, kGhostBar, kGhostThick, 2.0f * hz - 2.0f * kGhostBar, r, g, b, other);
    for (int i = 0; i < kBarsPerGhost; ++i)
    {
        out[i].shineStrength = 0.0f; // matt: a hint, not a thing
        out[i].shininess = 1.0f;
    }
}

/// Adds the outlines to the drawn table `out`, which holds `count` objects and has room for `capacity`. Returns the new
/// count. A table that is too full gets no outline at all (half an outline would be a wrong hint).
inline int AddGhosts(scene::SceneObject* out, int count, int capacity)
{
    if (out == nullptr || count < 0 || capacity - count < kGhostCount * kBarsPerGhost)
    {
        return count;
    }
    for (int index : kGhostIndices)
    {
        MakeGhost(kStageStatics[index], out + count);
        count += kBarsPerGhost;
    }
    return count;
}

} // namespace dl::stage
