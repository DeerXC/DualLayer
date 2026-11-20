// StageHints.h - what the stage shows without words: the outlines of the other layer, the frame of the blue door, the beacon
//            over the goal, and the rings that answer the goal.
// Outlines:  every fixed object of the stage that exists in one layer only leaves a thin outline in the other layer: four
//            narrow bars round its footprint, in its own colour made lighter. A flat object (a terrace, a stone, an apron: up
//            to kFlatHeight thick) is outlined at its TOP, where the body would stand; a tall one (a wall) at its BASE, on the
//            floor. In the courtyard this is how the player reads where the block can go: the block is stopped by the walls
//            of both layers, and the walls of the other layer are only these outlines. The list is not kept by hand: it is
//            every single-layer box of scene/Stage.h, so a new wall is outlined by itself.
// Door:      the plate is in layer A and the door it opens is in layer B. AddDoorFrame draws, in A, a thin blue frame round
//            the doorway while the door is closed for a body in A; standing on the plate makes it vanish.
// Beacon:    a thin column of light over the goal, seen from the start over every wall of both layers.
// Clear:     when the goal is reached, three rings spread from the goal light and the beacon grows (AddClearRings,
//            AddGoalBeacon with the seconds since the clear).
// Solid:     never. All of this goes into the DRAWN table only.
// Plain data and plain logic, no Windows, no Direct3D: tested without a window (tests/test_hints.cpp). UNITS: metres, seconds.
#pragma once

#include "scene/SceneData.h"
#include "scene/Stage.h"

namespace dl::stage
{

inline constexpr float kGhostBar = 0.06f;   // UNITS: metres, how wide a bar of an outline is
inline constexpr float kGhostThick = 0.03f; // UNITS: metres, how high a bar is (thin: it must not look like a step)
inline constexpr float kGhostLift = 0.02f;  // UNITS: metres above the surface, so that the bar never flickers with the floor
inline constexpr float kFlatHeight = 0.5f;  // UNITS: metres; an object up to this thick is a platform, outlined at its top
inline constexpr float kGhostLight = 0.35f; // how much of the way to white the colour goes: the outline glows a little
inline constexpr int kBarsPerGhost = 4;

/// True when `o` leaves an outline in the other layer: a box of one layer only.
constexpr bool HasGhost(const SceneObject& o)
{
    return o.shape == ShapeKind::Cube && (o.layerMask == kLayerA || o.layerMask == kLayerB);
}

constexpr int CountGhosts()
{
    int n = 0;
    for (const SceneObject& o : kStageStatics)
    {
        n += HasGhost(o) ? 1 : 0;
    }
    return n;
}
inline constexpr int kGhostCount = CountGhosts();

/// The height at which the outline of `o` is drawn: the top of a flat object, the base of a tall one (plus a small lift).
constexpr float GhostY(const SceneObject& o)
{
    const float half = 0.5f * o.scale[1];
    const float surface = o.scale[1] <= kFlatHeight ? o.position[1] + half : o.position[1] - half;
    return surface + kGhostLift + 0.5f * kGhostThick;
}

/// A colour made lighter by `amount` (0 = the same, 1 = white).
constexpr void Lighten(const SceneObject& from, float amount, float& r, float& g, float& b)
{
    r = from.colorSrgb[0] + (1.0f - from.colorSrgb[0]) * amount;
    g = from.colorSrgb[1] + (1.0f - from.colorSrgb[1]) * amount;
    b = from.colorSrgb[2] + (1.0f - from.colorSrgb[2]) * amount;
}

/// A flat, matt bar: a hint, not a thing.
constexpr SceneObject HintBar(float x0, float y0, float z0, float x1, float y1, float z1, float r, float g, float b, unsigned mask)
{
    SceneObject o = Span(x0, y0, z0, x1, y1, z1, r, g, b, mask);
    o.shineStrength = 0.0f;
    o.shininess = 1.0f;
    return o;
}

/// Writes the four bars of the outline of `o` into out[0 .. 3]: the two long sides along x, then the two short sides along
/// z between them, so that no corner is drawn twice.
inline void MakeGhost(const SceneObject& o, SceneObject* out)
{
    const unsigned other = o.layerMask == kLayerA ? kLayerB : kLayerA;
    const float y0 = GhostY(o) - 0.5f * kGhostThick;
    const float y1 = y0 + kGhostThick;
    const float x0 = o.position[0] - 0.5f * o.scale[0];
    const float x1 = o.position[0] + 0.5f * o.scale[0];
    const float z0 = o.position[2] - 0.5f * o.scale[2];
    const float z1 = o.position[2] + 0.5f * o.scale[2];
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;
    Lighten(o, kGhostLight, r, g, b);
    out[0] = HintBar(x0, y0, z0, x1, y1, z0 + kGhostBar, r, g, b, other);
    out[1] = HintBar(x0, y0, z1 - kGhostBar, x1, y1, z1, r, g, b, other);
    out[2] = HintBar(x0, y0, z0 + kGhostBar, x0 + kGhostBar, y1, z1 - kGhostBar, r, g, b, other);
    out[3] = HintBar(x1 - kGhostBar, y0, z0 + kGhostBar, x1, y1, z1 - kGhostBar, r, g, b, other);
}

/// Adds the outlines to the drawn table `out`, which holds `count` objects and has room for `capacity`. Returns the new
/// count. A table that is too full gets no outline at all (half the outlines would be a wrong hint).
inline int AddGhosts(SceneObject* out, int count, int capacity)
{
    if (out == nullptr || count < 0 || capacity - count < kGhostCount * kBarsPerGhost)
    {
        return count;
    }
    for (const SceneObject& o : kStageStatics)
    {
        if (HasGhost(o))
        {
            MakeGhost(o, out + count);
            count += kBarsPerGhost;
        }
    }
    return count;
}

// ---- the frame of the blue door, drawn in layer A while the door is closed for a body in A
inline constexpr float kFrameDepth = 0.03f; // UNITS: metres, in front of the south face of the wall
inline constexpr float kFrameGap = 0.01f;   // UNITS: metres between the frame and the wall, so that they never flicker
inline constexpr int kFrameBars = 4;

/// Adds the frame of the door to `out` when `doorOpenForA` is false (the door as it is for a body in A). Returns the new
/// count; a table without room for the whole frame gets none.
inline int AddDoorFrame(SceneObject* out, int count, int capacity, bool doorOpenForA)
{
    if (doorOpenForA || out == nullptr || count < 0 || capacity - count < kFrameBars)
    {
        return count;
    }
    const float x0 = kFinaleX - kDoorwayHalfWidth;
    const float x1 = kFinaleX + kDoorwayHalfWidth;
    const float y0 = kDoorwayLow;
    const float y1 = kDoorwayHigh;
    const float z1 = kDoorSouth - kFrameGap;
    const float z0 = z1 - kFrameDepth;
    const float w = kGhostBar;
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;
    Lighten(kDoor, kGhostLight, r, g, b);
    SceneObject* f = out + count;
    f[0] = HintBar(x0, y1 - w, z0, x1, y1, z1, r, g, b, kLayerA);         // top
    f[1] = HintBar(x0, y0, z0, x1, y0 + w, z1, r, g, b, kLayerA);         // bottom
    f[2] = HintBar(x0, y0 + w, z0, x0 + w, y1 - w, z1, r, g, b, kLayerA); // left
    f[3] = HintBar(x1 - w, y0 + w, z0, x1, y1 - w, z1, r, g, b, kLayerA); // right
    return count + kFrameBars;
}

// ---- the beacon over the goal, and what happens when the goal is reached
inline constexpr float kBeaconWidth = 0.12f;      // UNITS: metres
inline constexpr float kBeaconBottom = kGoal.y + kGoalRadius + 0.05f;
inline constexpr float kBeaconTop = 14.0f;        // UNITS: metres (world y): seen over every wall from the start
inline constexpr float kClearSeconds = 1.6f;      // UNITS: seconds that the rings spread
inline constexpr float kClearBeaconWidth = 0.6f;  // UNITS: metres: the beacon after the clear
inline constexpr int kClearRings = 3;

/// Adds the beacon (both layers, drawn only). `clearSeconds` < 0 while the goal is not reached; after it, the beacon grows
/// wide and white within kClearSeconds. Returns the new count.
inline int AddGoalBeacon(SceneObject* out, int count, int capacity, float clearSeconds)
{
    if (out == nullptr || count < 0 || capacity - count < 1)
    {
        return count;
    }
    const float t = clearSeconds < 0.0f ? 0.0f : (clearSeconds > kClearSeconds ? 1.0f : clearSeconds / kClearSeconds);
    const float half = 0.5f * (kBeaconWidth + (kClearBeaconWidth - kBeaconWidth) * t);
    out[count] = HintBar(kGoal.x - half, kBeaconBottom, kGoal.z - half, kGoal.x + half, kBeaconTop, kGoal.z + half, 1.0f,
                         0.90f + 0.10f * t, 0.55f + 0.45f * t, kLayerBoth);
    return count + 1;
}

/// Adds the rings that spread from the goal light for kClearSeconds after the clear (`clearSeconds` >= 0). Each ring is a
/// flat torus that grows and starts a little after the one before. Returns the new count.
inline int AddClearRings(SceneObject* out, int count, int capacity, float clearSeconds)
{
    if (out == nullptr || count < 0 || clearSeconds < 0.0f || clearSeconds >= kClearSeconds)
    {
        return count;
    }
    for (int i = 0; i < kClearRings && count < capacity; ++i)
    {
        const float start = 0.25f * static_cast<float>(i);
        const float t = (clearSeconds - start) / (kClearSeconds - start);
        if (t <= 0.0f || t >= 1.0f)
        {
            continue;
        }
        const float size = 0.5f + 7.5f * t; // the torus of the renderer is 1.6 m across at scale 1
        SceneObject o{};
        o.shape = ShapeKind::Torus;
        o.position[0] = kGoal.x;
        o.position[1] = kGoal.y;
        o.position[2] = kGoal.z;
        o.scale[0] = size;
        o.scale[1] = 0.3f; // flat: a ring of light on the level of the goal
        o.scale[2] = size;
        o.colorSrgb[0] = 1.0f;
        o.colorSrgb[1] = 0.95f;
        o.colorSrgb[2] = 0.80f;
        o.shineStrength = 1.0f;
        o.shininess = 8.0f;
        o.layerMask = kLayerBoth;
        out[count++] = o;
    }
    return count;
}

} // namespace dl::stage
