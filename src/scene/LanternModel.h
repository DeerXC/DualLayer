// LanternModel.h - the lantern as an object: the model that hangs from the hand of the traveller and, set down, from
// the
//                  arm of a small iron post (docs/DESIGN.md 6.7).
// Why:             one model for both states, so that the thing the player sets down is plainly the thing the player
//                  carried. It is made of the shapes of scene/SceneData.h and is drawn like any other object of the
//                  town.
// Parts:           a bail ring, a chimney, a brass cap and its rim, four iron posts, a glass globe that glows, the
// flame
//                  inside it, a rim and a foot. The post of a set-down lantern adds a foot plate, a pole, an arm and a
//                  finial.
// Hung:            every part is placed from one point, the hook (the top of the ring), and swings about it with `tilt`
//                  (radians, round X, before the turn: + puts the foot of the lantern behind). The glass is
//                  kLanternGlassBelowHook under the hook, which is where the light of the lantern is put.
// Flame:           the flame of the guidance (scene/Flame.h) stands over the chimney: AddLanternModel gives its base.
// Plain data and plain maths, no Windows, no Direct3D: tested without a window (tests/test_lantern.cpp).
// UNITS:           metres, radians, seconds. Space: left-handed, +Y up.
#pragma once

#include "scene/Camera.h" // Vec3
#include "scene/SceneData.h"

#include <cmath>

namespace dl::town
{

inline constexpr int kLanternModelObjects = 12;         ///< the parts of the lantern itself
inline constexpr int kLanternStandObjects = 4;          ///< the post a set-down lantern hangs from
inline constexpr float kLanternGlassBelowHook = 0.275f; ///< UNITS: metres from the hook to the middle of the glass
inline constexpr float kLanternFootBelowHook = 0.401f;  ///< UNITS: metres from the hook to the underside of the foot
inline constexpr float kLanternStandArm = 0.16f;        ///< UNITS: metres from the pole of the post to the lantern

namespace lantern_model_detail
{

struct Rig
{
    Vec3 hook;
    float sy;
    float cy;
    float st;
    float ct;

    /// A point of the lantern (x right, y up, z ahead, from the hook) in the world: swung by the tilt, then turned.
    Vec3 At(float x, float y, float z) const
    {
        const float ry = y * ct - z * st;
        const float rz = y * st + z * ct;
        return {hook.x + cy * x + sy * rz, hook.y + ry, hook.z - sy * x + cy * rz};
    }
};

inline scene::SceneObject Make(scene::ShapeKind shape, const Vec3& at, float sx, float sy, float sz, float r, float g,
                               float b, float tilt, float yaw, float glow, float shine, float shininess)
{
    scene::SceneObject o{};
    o.shape = shape;
    o.position[0] = at.x;
    o.position[1] = at.y;
    o.position[2] = at.z;
    o.scale[0] = sx;
    o.scale[1] = sy;
    o.scale[2] = sz;
    o.colorSrgb[0] = r;
    o.colorSrgb[1] = g;
    o.colorSrgb[2] = b;
    o.tilt = tilt;
    o.yaw = yaw;
    o.glow = glow;
    o.shineStrength = shine;
    o.shininess = shininess;
    o.layerMask = scene::kLayerBoth;
    return o;
}

} // namespace lantern_model_detail

/// Adds the lantern hanging from `hook`, turned by `yaw`, swung by `tilt`. `seconds` moves the flame inside. Returns
/// the new count; nothing when there is no room for kLanternModelObjects. `flameBase`: the top of the chimney (where
/// the flame of the guidance stands); `glass`: the middle of the glass (where its light is).
inline int AddLanternModel(scene::SceneObject* out, int count, int capacity, const Vec3& hook, float yaw, float tilt,
                           float seconds, Vec3* flameBase = nullptr, Vec3* glass = nullptr)
{
    using lantern_model_detail::Make;
    using scene::ShapeKind;
    if (out == nullptr || count < 0 || capacity - count < kLanternModelObjects)
    {
        return count;
    }
    const lantern_model_detail::Rig rig{hook, std::sin(yaw), std::cos(yaw), std::sin(tilt), std::cos(tilt)};
    constexpr float halfPi = 1.5707963f;
    const float flicker = 1.0f + 0.07f * std::sin(seconds * 9.1f) + 0.04f * std::sin(seconds * 15.7f + 1.1f);
    const float glassY = -kLanternGlassBelowHook;
    // The ring stands in the plane of the arm and the body (the torus is turned onto its edge by half a pi).
    out[count++] = Make(ShapeKind::Torus, rig.At(0.0f, -0.075f, 0.0f), 0.07f, 0.05f, 0.07f, 0.50f, 0.33f, 0.17f,
                        halfPi + tilt, yaw, 0.0f, 0.35f, 40.0f);
    out[count++] = Make(ShapeKind::Cylinder, rig.At(0.0f, -0.112f, 0.0f), 0.048f, 0.04f, 0.048f, 0.17f, 0.15f, 0.14f,
                        tilt, yaw, 0.0f, 0.15f, 20.0f);
    out[count++] = Make(ShapeKind::Sphere, rig.At(0.0f, -0.148f, 0.0f), 0.17f, 0.075f, 0.17f, 0.52f, 0.34f, 0.17f, tilt,
                        yaw, 0.0f, 0.40f, 45.0f);
    out[count++] = Make(ShapeKind::Cylinder, rig.At(0.0f, -0.194f, 0.0f), 0.15f, 0.022f, 0.15f, 0.17f, 0.15f, 0.14f,
                        tilt, yaw, 0.0f, 0.15f, 20.0f);
    for (int k = 0; k < 4; ++k)
    {
        const float px = (k & 1) != 0 ? 0.054f : -0.054f;
        const float pz = (k & 2) != 0 ? 0.054f : -0.054f;
        out[count++] = Make(ShapeKind::Cube, rig.At(px, glassY, pz), 0.016f, 0.165f, 0.016f, 0.17f, 0.15f, 0.14f, tilt,
                            yaw, 0.0f, 0.15f, 20.0f);
    }
    out[count++] = Make(ShapeKind::Sphere, rig.At(0.0f, glassY, 0.0f), 0.125f, 0.165f, 0.125f, 1.0f, 0.70f, 0.32f, tilt,
                        yaw, 1.05f * flicker, 0.0f, 12.0f);
    out[count++] =
        Make(ShapeKind::Sphere, rig.At(0.0f, glassY + 0.005f, 0.0f), 0.055f, 0.095f * (1.0f + 0.12f * (flicker - 1.0f)),
             0.055f, 1.0f, 0.94f, 0.72f, tilt, yaw, 2.6f, 0.0f, 12.0f);
    out[count++] = Make(ShapeKind::Cylinder, rig.At(0.0f, -0.370f, 0.0f), 0.14f, 0.026f, 0.14f, 0.17f, 0.15f, 0.14f,
                        tilt, yaw, 0.0f, 0.15f, 20.0f);
    out[count++] = Make(ShapeKind::Cylinder, rig.At(0.0f, -0.392f, 0.0f), 0.115f, 0.018f, 0.115f, 0.50f, 0.33f, 0.17f,
                        tilt, yaw, 0.0f, 0.35f, 40.0f);
    if (flameBase != nullptr)
    {
        *flameBase = rig.At(0.0f, -0.092f, 0.0f);
    }
    if (glass != nullptr)
    {
        *glass = rig.At(0.0f, glassY, 0.0f);
    }
    return count;
}

/// Adds the post a set-down lantern hangs from: a foot plate on the floor at `base`, a pole kLanternStandArm to the +X
/// side, an arm over the lantern and a finial. `hookHeight`: the height of the arm over the floor (where the hook of
/// AddLanternModel is). Returns the new count; nothing when there is no room for kLanternStandObjects.
inline int AddLanternStand(scene::SceneObject* out, int count, int capacity, const Vec3& base, float hookHeight)
{
    using lantern_model_detail::Make;
    using scene::ShapeKind;
    if (out == nullptr || count < 0 || capacity - count < kLanternStandObjects)
    {
        return count;
    }
    const float poleX = base.x + kLanternStandArm;
    const float top = hookHeight + 0.03f;
    out[count++] = Make(ShapeKind::Cylinder, {poleX, base.y + 0.015f, base.z}, 0.17f, 0.03f, 0.17f, 0.17f, 0.15f, 0.14f,
                        0.0f, 0.0f, 0.0f, 0.15f, 20.0f);
    out[count++] = Make(ShapeKind::Cylinder, {poleX, base.y + 0.5f * top, base.z}, 0.034f, top, 0.034f, 0.17f, 0.15f,
                        0.14f, 0.0f, 0.0f, 0.0f, 0.15f, 20.0f);
    out[count++] = Make(ShapeKind::Cube, {base.x + 0.5f * kLanternStandArm, base.y + hookHeight, base.z},
                        kLanternStandArm + 0.034f, 0.028f, 0.028f, 0.17f, 0.15f, 0.14f, 0.0f, 0.0f, 0.0f, 0.15f, 20.0f);
    out[count++] = Make(ShapeKind::Sphere, {poleX, base.y + top + 0.02f, base.z}, 0.058f, 0.058f, 0.058f, 0.50f, 0.33f,
                        0.17f, 0.0f, 0.0f, 0.0f, 0.40f, 45.0f);
    return count;
}

} // namespace dl::town
