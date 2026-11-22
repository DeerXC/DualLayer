// StageHints.h - what the map shows without words: the outlines of the other layer, the outline of what stopped a push, the
//            frame of the blue door of the courtyard, the beacon over the goal, and the rings that answer the goal.
// Outlines:  every fixed box of the map that exists in one layer only leaves a thin outline in the other layer: four narrow
//            bars round its footprint, in the colour of its world (red A, blue B) made lighter. A flat object (a floor, a stone, a strip: up to
//            kFlatHeight thick) is outlined at its TOP, where the body would stand; a tall one (a wall) at its BASE. In the
//            courtyard this is how the player reads where the block can go. The list is not kept by hand: it is every
//            single-layer box of scene/Stage.h.
// Worlds:    a single-layer box is drawn in its material (wood or iron in A, fallen masonry in B); red and blue are only
//            its outlines: in the other world (the hint above, MakeGhost) and a thin edge in its own world (AddWorldEdges).
// Blocked:   a refused push names what was in the way (Puzzle::LastBlocker). For kFlashSeconds its outline is drawn in BOTH
//            layers, wide and bright, fading out. Only the outline: the thing itself stays in its own layer, so a blue wall is
//            never seen as a solid box in layer A (the first version drew the whole box in both layers, which looked like a
//            wall that should not be there).
// Previews: a gate that appears when something is done (a bridge) shows where it will be: its outline in both layers, a
//            little brighter than a hint (AddGatePreviews). The player sees "something goes here" before knowing how.
// Glow:      every hint has a little light of its own (SceneObject::glow), so it reads in the darker corners as well.
// Door:      the plate is in layer A and the door it opens is in layer B. AddDoorFrame draws, in A, a thin blue frame round
//            the doorway while the door is closed for a body in A; standing on the plate makes it vanish.
// Beacon:    a thin column of light over the goal on the top of the tower, seen from the start over every wall of both
//            layers: the goal is the first thing seen and the last thing reached.
// Clear:     when the goal is reached, three rings spread from the goal light and the beacon grows (AddClearRings,
//            AddGoalBeacon with the seconds since the clear).
// Solid:     never. All of this goes into the DRAWN table only.
// Plain data and plain logic, no Windows, no Direct3D: tested without a window (tests/test_hints.cpp). UNITS: metres, seconds.
#pragma once

#include "scene/Mechanisms.h"
#include "scene/Puzzle.h"
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
inline constexpr float kHintGlow = 0.35f;  // the light of its own of a hint bar

/// True when `o` leaves an outline in the other layer: an axis-aligned box of one layer only.
constexpr bool HasGhost(const SceneObject& o)
{
    return o.shape == ShapeKind::Cube && o.tilt == 0.0f && (o.layerMask == kLayerA || o.layerMask == kLayerB);
}

inline int CountGhosts()
{
    int n = 0;
    for (const SceneObject& o : kStageStatics)
    {
        n += HasGhost(o) ? 1 : 0;
    }
    return n;
}
inline const int kGhostCount = CountGhosts();

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
    o.glow = kHintGlow;
    return o;
}

/// Writes the four bars of an outline of the footprint of `o` into out[0 .. 3]: the two long sides along x, then the two short
/// sides along z between them, so that no corner is drawn twice. `bar` is the width of a bar, `lighten` how close to white
/// the colour of `o` goes, `mask` the layers.
inline void MakeOutline(const SceneObject& o, float bar, float lighten, unsigned mask, SceneObject* out)
{
    const float y0 = GhostY(o) - 0.5f * kGhostThick;
    const float y1 = y0 + kGhostThick;
    const float x0 = o.position[0] - 0.5f * o.scale[0];
    const float x1 = o.position[0] + 0.5f * o.scale[0];
    const float z0 = o.position[2] - 0.5f * o.scale[2];
    const float z1 = o.position[2] + 0.5f * o.scale[2];
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;
    Lighten(o, lighten, r, g, b);
    out[0] = HintBar(x0, y0, z0, x1, y1, z0 + bar, r, g, b, mask);
    out[1] = HintBar(x0, y0, z1 - bar, x1, y1, z1, r, g, b, mask);
    out[2] = HintBar(x0, y0, z0 + bar, x0 + bar, y1, z1 - bar, r, g, b, mask);
    out[3] = HintBar(x1 - bar, y0, z0 + bar, x1, y1, z1 - bar, r, g, b, mask);
}

/// `o` in the colour of its world: red for a box of A, blue for B (both layers: unchanged). Red and blue mean only "this
/// exists in one world"; the box itself is drawn in its material (docs/LEVEL_DESIGN.md, 14.3).
inline SceneObject WorldTinted(const SceneObject& o)
{
    SceneObject t = o;
    if (o.layerMask == kLayerA || o.layerMask == kLayerB)
    {
        const Rgb c = o.layerMask == kLayerA ? kRed : kBlue;
        t.colorSrgb[0] = c.r;
        t.colorSrgb[1] = c.g;
        t.colorSrgb[2] = c.b;
    }
    return t;
}

/// The outline of `o` in the other layer (the hint that is always there), in the colour of the world of `o`.
inline void MakeGhost(const SceneObject& o, SceneObject* out)
{
    MakeOutline(WorldTinted(o), kGhostBar, kGhostLight, o.layerMask == kLayerA ? kLayerB : kLayerA, out);
}

inline constexpr float kEdgeBar = 0.05f;   ///< UNITS: metres: the edge of a box of one world, in its own world
inline constexpr float kEdgeLight = 0.1f;

/// The thin edge of `o` in its OWN world, in the colour of that world: the box is wood or fallen masonry, its edge says which
/// world it belongs to.
inline void MakeEdge(const SceneObject& o, SceneObject* out)
{
    MakeOutline(WorldTinted(o), kEdgeBar, kEdgeLight, o.layerMask, out);
}

/// Adds the edge of every single-layer box of the map, in its own world. Returns the new count; a table without room for all
/// of them gets none.
inline int AddWorldEdges(SceneObject* out, int count, int capacity)
{
    if (out == nullptr || count < 0 || capacity - count < kGhostCount * kBarsPerGhost)
    {
        return count;
    }
    for (const SceneObject& o : kStageStatics)
    {
        if (HasGhost(o))
        {
            MakeEdge(o, out + count);
            count += kBarsPerGhost;
        }
    }
    return count;
}

/// The same for objects[0 .. n - 1] (the moving parts of a wing). `objects` may point into `out` before `count`.
inline int AddOwnEdges(const SceneObject* objects, int n, SceneObject* out, int count, int capacity)
{
    if (objects == nullptr || out == nullptr || n < 0 || count < 0)
    {
        return count;
    }
    int needed = 0;
    for (int i = 0; i < n; ++i)
    {
        needed += HasGhost(objects[i]) ? kBarsPerGhost : 0;
    }
    if (capacity - count < needed)
    {
        return count;
    }
    for (int i = 0; i < n; ++i)
    {
        if (HasGhost(objects[i]))
        {
            const SceneObject o = objects[i];
            MakeEdge(o, out + count);
            count += kBarsPerGhost;
        }
    }
    return count;
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

/// Adds the outline, in the other layer, of every single-layer axis-aligned box in objects[0 .. n - 1] (the moving parts of a
/// wing: a bridge, a door, the pad of a socket). `objects` may point into `out` before `count`. Returns the new count; a table
/// without room for all of them gets none.
inline int AddOutlines(const SceneObject* objects, int n, SceneObject* out, int count, int capacity)
{
    if (objects == nullptr || out == nullptr || n < 0 || count < 0)
    {
        return count;
    }
    int needed = 0;
    for (int i = 0; i < n; ++i)
    {
        needed += HasGhost(objects[i]) ? kBarsPerGhost : 0;
    }
    if (capacity - count < needed)
    {
        return count;
    }
    for (int i = 0; i < n; ++i)
    {
        if (HasGhost(objects[i]))
        {
            const SceneObject o = objects[i]; // a copy: `objects` may lie in `out`
            MakeGhost(o, out + count);
            count += kBarsPerGhost;
        }
    }
    return count;
}

// ---- where a gate will appear
inline constexpr float kPreviewBar = 0.09f;   // UNITS: metres
inline constexpr float kPreviewLight = 0.6f;  // closer to white than a hint: it is an invitation

/// Adds the outline, in both layers, of every gate of `mech` that appears when its trigger is worked (a bridge) and is not
/// there yet. Returns the new count; a table without room for all of them gets none.
inline int AddGatePreviews(const mech::MechanismSet& mech, SceneObject* out, int count, int capacity)
{
    if (out == nullptr || count < 0)
    {
        return count;
    }
    int needed = 0;
    for (int i = 0; i < mech.GateCount(); ++i)
    {
        needed += (mech.GateAt(i).presentWhenOpen && !mech.GatePresent(i)) ? kBarsPerGhost : 0;
    }
    if (capacity - count < needed)
    {
        return count;
    }
    for (int i = 0; i < mech.GateCount(); ++i)
    {
        if (mech.GateAt(i).presentWhenOpen && !mech.GatePresent(i))
        {
            MakeOutline(WorldTinted(mech.GateAt(i).box), kPreviewBar, kPreviewLight, kLayerBoth, out + count);
            for (int b = 0; b < kBarsPerGhost; ++b)
            {
                out[count + b].glow = 0.7f;
            }
            count += kBarsPerGhost;
        }
    }
    return count;
}

// ---- the outline of what stopped a push
inline constexpr float kBlockedBar = 0.14f; // UNITS: metres: wider than the hint, so that it is seen as an answer to the push

/// Adds, while a refused push flashes, the outline of the fixed object that was in the way, in both layers, from almost white
/// fading back to its colour. Nothing for a refusal by the door or the edge of the board (they have no single footprint to
/// show). Returns the new count.
inline int AddBlockedFlash(const puzzle::Puzzle& puzzle, SceneObject* out, int count, int capacity)
{
    const int blocker = puzzle.LastBlocker();
    const float flash = puzzle.BlockedFlash();
    if (out == nullptr || count < 0 || capacity - count < kBarsPerGhost || flash <= 0.0f || blocker < 0 || blocker >= kStageStaticCount)
    {
        return count;
    }
    const SceneObject& o = kStageStatics[static_cast<std::size_t>(blocker)];
    if (o.tilt != 0.0f)
    {
        return count;
    }
    const float t = flash / puzzle::Puzzle::kFlashSeconds; // 1 at the start of the flash, 0 at its end
    MakeOutline(WorldTinted(o), kBlockedBar, kGhostLight + (1.0f - kGhostLight) * 0.8f * t, kLayerBoth, out + count);
    return count + kBarsPerGhost;
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
    const float x0 = kDoorX - kDoorwayHalfWidth;
    const float x1 = kDoorX + kDoorwayHalfWidth;
    const float y0 = kDoorwayLow;
    const float y1 = kDoorwayHigh;
    const float z1 = kDoorSouth - kFrameGap;
    const float z0 = z1 - kFrameDepth;
    const float w = kGhostBar;
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;
    Lighten(WorldTinted(kDoor), kGhostLight, r, g, b);
    SceneObject* f = out + count;
    f[0] = HintBar(x0, y1 - w, z0, x1, y1, z1, r, g, b, kLayerA);         // top
    f[1] = HintBar(x0, y0, z0, x1, y0 + w, z1, r, g, b, kLayerA);         // bottom
    f[2] = HintBar(x0, y0 + w, z0, x0 + w, y1 - w, z1, r, g, b, kLayerA); // left
    f[3] = HintBar(x1 - w, y0 + w, z0, x1, y1 - w, z1, r, g, b, kLayerA); // right
    return count + kFrameBars;
}

// ---- the beacon over the goal, and what happens when the goal is reached
inline constexpr float kBeaconWidth = 0.12f;     // UNITS: metres
inline constexpr float kBeaconBottom = kGoal.y + kGoalRadius + 0.05f;
inline constexpr float kBeaconTop = 30.0f;       // UNITS: metres (world y): seen over every wall from the start
inline constexpr float kClearSeconds = 1.6f;     // UNITS: seconds that the rings spread
inline constexpr float kClearBeaconWidth = 0.6f; // UNITS: metres: the beacon after the clear
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
    out[count].glow = 1.5f; // a column of light: seen in the dark of either layer
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
