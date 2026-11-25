// TownEffects.h - the two effects of the town loop that are drawn as objects: the sea round the eye and the ring that
// marks a
//                 return after a fall.
// Sea:       one flat plane per layer, centred under the eye and wider than the far plane, so the horizon is never a
// edge. It
//            moves in steps of kSeaSnap, so the pattern of the waves (shaders/models.hlsl, scene::kMatSea) does not
//            slide.
// Ring:      a flat torus that spreads on the floor for kRespawnRingSeconds after the body came back to the last place
// it stood on. Plain data, no Windows, no Direct3D: tested without a window (tests/test_effects.cpp). UNITS: metres,
// seconds.
#pragma once

#include "scene/Camera.h" // Vec3
#include "scene/SceneData.h"

#include <cmath>

namespace dl::town
{

inline constexpr float kViewDistance =
    300.0f; ///< UNITS: metres: the far plane (render/Projection.h) must not be beyond it
inline constexpr float kSeaHalf = kViewDistance + 60.0f; ///< UNITS: metres: half the side of the sea plane
inline constexpr float kSeaSnap = 20.0f;                 ///< UNITS: metres: the plane moves in steps of this
inline constexpr float kSinkSeconds = 0.55f;             ///< UNITS: seconds a body sinks in water before it comes back
inline constexpr float kRespawnRingSeconds = 0.8f;       ///< UNITS: seconds the ring of a return lasts

/// Adds the sea at `level` (metres), ending at `northEdge` (z) to the north, to out[count ..). Returns the new count.
/// Each layer has its own colour.
inline int AddSea(const Vec3& eye, scene::SceneObject* out, int count, int capacity, float level, float northEdge)
{
    const float cx = kSeaSnap * std::floor(eye.x / kSeaSnap + 0.5f);
    const float cz = kSeaSnap * std::floor(eye.z / kSeaSnap + 0.5f);
    const float colours[2][3] = {{0.18f, 0.36f, 0.50f}, {0.20f, 0.26f, 0.30f}};
    const unsigned masks[2] = {scene::kLayerA, scene::kLayerB};
    for (int k = 0; k < 2 && out != nullptr && count >= 0 && count < capacity; ++k)
    {
        const float north = cz + kSeaHalf < northEdge ? cz + kSeaHalf : northEdge;
        const float south = cz - kSeaHalf;
        if (north <= south)
        {
            break; // the eye is so far north that the sea is out of sight
        }
        scene::SceneObject o{};
        o.shape = scene::ShapeKind::Cube;
        o.position[0] = cx;
        o.position[1] = level - 0.1f;
        o.position[2] = 0.5f * (south + north);
        o.scale[0] = 2.0f * kSeaHalf;
        o.scale[1] = 0.2f;
        o.scale[2] = north - south;
        for (int c = 0; c < 3; ++c)
        {
            o.colorSrgb[c] = colours[k][c];
        }
        o.shineStrength = 0.6f;
        o.shininess = 60.0f;
        o.checker = scene::kMatSea; // drawn as the sea: swells, crests, the sky in it
        o.glow = 1.0f;              // (water: how opaque)
        o.layerMask = masks[k];
        out[count++] = o;
    }
    return count;
}

/// Adds, for kRespawnRingSeconds after a return (`since` >= 0), a ring that spreads on the floor from `at`.
inline int AddRespawnRing(const Vec3& at, float since, scene::SceneObject* out, int count, int capacity)
{
    if (out == nullptr || count < 0 || count >= capacity || since < 0.0f || since >= kRespawnRingSeconds)
    {
        return count;
    }
    const float t = since / kRespawnRingSeconds;
    scene::SceneObject o{};
    o.shape = scene::ShapeKind::Torus;
    o.position[0] = at.x;
    o.position[1] = at.y + 0.1f;
    o.position[2] = at.z;
    o.scale[0] = 0.6f + 4.0f * t;
    o.scale[1] = 0.15f;
    o.scale[2] = 0.6f + 4.0f * t;
    o.colorSrgb[0] = 0.85f;
    o.colorSrgb[1] = 0.80f;
    o.colorSrgb[2] = 1.00f;
    o.glow = 1.2f * (1.0f - t);
    o.layerMask = scene::kLayerBoth;
    out[count++] = o;
    return count;
}

} // namespace dl::town
