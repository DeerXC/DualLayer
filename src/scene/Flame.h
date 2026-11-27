// Flame.h - the flame of the lantern, which leans toward what the player is looking for (docs/DESIGN.md 10.3).
// Why:      help is an object of the world and not a layer on the screen: the lantern is the compass. The flame stands
//           over the lantern, in the hand and set down, and leans toward the next thing to do; the longer the player
//           goes without progress, the further it leans and the more it throws sparks.
// Drawn:    a thin glowing plume (an ellipsoid turned by `tilt` round X and then by `yaw` round Y, so that it leans
//           toward (sin yaw, cos yaw): the same bearing as the avatar turns) and, when it is restless, three sparks
//           that fly off its tip.
// Plain data, no Windows, no Direct3D: tested without a window (tests/test_guidance.cpp).
// UNITS: metres, radians, seconds. Space: left-handed, +Y up.
#pragma once

#include "scene/Camera.h" // Vec3
#include "scene/SceneData.h"

#include <cmath>

namespace dl::town
{

inline constexpr float kFlameLength = 0.34f; ///< UNITS: metres: the height of the plume
inline constexpr int kFlameObjects = 4;      ///< the plume and three sparks

struct Flame
{
    bool on = false;     ///< false: no flame is drawn (the guidance is off)
    float yaw = 0.0f;    ///< the bearing it leans toward: (sin yaw, cos yaw) on the ground plane
    float tilt = 0.0f;   ///< radians from upright
    float sparks = 0.0f; ///< 0 .. 1: how restless it is
};

/// The direction of the axis of a plume leaning by `tilt` toward `yaw`.
inline Vec3 FlameAxis(const Flame& f)
{
    const float s = std::sin(f.tilt);
    return {s * std::sin(f.yaw), std::cos(f.tilt), s * std::cos(f.yaw)};
}

/// Adds the plume (and the sparks) standing on `base`. Returns the new count; nothing for a flame that is off or a
/// table without room for kFlameObjects.
inline int AddFlame(scene::SceneObject* out, int count, int capacity, const Vec3& base, const Flame& f, float seconds)
{
    if (!f.on || out == nullptr || count < 0 || capacity - count < kFlameObjects)
    {
        return count;
    }
    const Vec3 axis = FlameAxis(f);
    scene::SceneObject plume{};
    plume.shape = scene::ShapeKind::Sphere; // a spindle: pointed at both ends, like a flame
    plume.position[0] = base.x + axis.x * 0.5f * kFlameLength;
    plume.position[1] = base.y + axis.y * 0.5f * kFlameLength;
    plume.position[2] = base.z + axis.z * 0.5f * kFlameLength;
    plume.scale[0] = plume.scale[2] = 0.085f;
    plume.scale[1] = kFlameLength;
    plume.tilt = f.tilt;
    plume.yaw = f.yaw;
    plume.colorSrgb[0] = 1.0f;
    plume.colorSrgb[1] = 0.86f;
    plume.colorSrgb[2] = 0.50f;
    plume.glow = 1.7f;
    plume.layerMask = scene::kLayerBoth;
    out[count++] = plume;
    const float sx = std::sin(f.yaw);
    const float sz = std::cos(f.yaw);
    for (int k = 0; k < 3; ++k)
    {
        scene::SceneObject spark{};
        spark.shape = scene::ShapeKind::Cube;
        float phase = seconds * 0.9f + static_cast<float>(k) / 3.0f;
        phase -= std::floor(phase);
        const bool shown = f.sparks > 0.05f;
        const float size =
            shown ? 0.055f * (1.0f - phase) * f.sparks : 0.0001f; // one that is not shown is too small to see
        const float ahead = 0.12f + 0.5f * phase;
        spark.position[0] = base.x + axis.x * kFlameLength + sx * ahead;
        spark.position[1] = base.y + axis.y * kFlameLength + 0.2f * phase - 0.22f * phase * phase;
        spark.position[2] = base.z + axis.z * kFlameLength + sz * ahead;
        spark.scale[0] = spark.scale[1] = spark.scale[2] = size;
        spark.colorSrgb[0] = 1.0f;
        spark.colorSrgb[1] = 0.90f;
        spark.colorSrgb[2] = 0.62f;
        spark.glow = 2.0f;
        spark.layerMask = scene::kLayerBoth;
        out[count++] = spark;
    }
    return count;
}

} // namespace dl::town
