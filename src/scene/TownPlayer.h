#pragma once

#include "scene/Player.h"
#include "scene/Town.h"
#include <algorithm>
#include <cmath>

namespace dl::town
{
inline Vec3 Along(const Vec3& point, const Vec3& direction, float distance)
{
    return {point.x + direction.x * distance, point.y + direction.y * distance, point.z + direction.z * distance};
}

// March a camera sphere towards the desired eye. The step is smaller than its radius,
// so even a thin wall cannot be skipped. During a ripple, either visible world may block it.
inline float CameraClearDistance(const Moments& moments, const Vec3& pivot, const Vec3& forward, float reach,
                                 int layer, bool both)
{
    constexpr float radius = 0.20f;
    constexpr float step = 0.09f;
    constexpr float skin = 0.02f;
    const auto blocked = [&](float distance, int world) {
        Vec3 p = Along(pivot, forward, -distance);
        return moments.PushOut(p, radius, world) > 0.001f;
    };
    const int count = static_cast<int>(std::ceil(reach / step));
    for (int i = 0; i <= count; ++i)
    {
        const float d = (std::min)(reach, step * static_cast<float>(i));
        if (blocked(d, layer) || (both && blocked(d, 1 - layer)))
        {
            return (std::max)(0.0f, d - step - skin);
        }
    }
    return reach;
}

inline bool MantleBodyFits(const Player& player, const Moments& moments, const Vec3& feet, int layer)
{
    const Vec3 origin = player.Feet();
    for (int i = 0; i < 3; ++i)
    {
        const Vec3 ball = player.Ball(i);
        Vec3 probe{feet.x, feet.y + (ball.y - origin.y), feet.z};
        if (moments.PushOut(probe, player.GetSettings().radius - 0.015f, layer) > 0.004f)
        {
            return false;
        }
    }
    return true;
}

// Only a deliberate forward jump may climb. Check a low face, a supported top,
// then the whole lift-and-cross path, without moving the collision body until accepted.
inline bool TryTownMantle(Player& player, const Moments& moments, const TownData& town, int layer, float maxRise)
{
    const Vec3 from = player.Feet();
    const Vec3 wish = player.WishDirection();
    if (!player.Jumping() || player.VerticalSpeed() < -0.3f || maxRise <= 0.0f ||
        wish.x * wish.x + wish.z * wish.z < 0.5f)
    {
        return false;
    }
    const float r = player.GetSettings().radius;
    const float reach = r * 2.0f + 0.15f;
    Vec3 top = Along(from, wish, reach);
    const float upper = (std::min)(player.TakeoffHeight() + maxRise, from.y + 0.48f);
    const float lower = (std::max)(player.TakeoffHeight() + 0.35f, from.y + 0.06f);
    if (upper < lower || !moments.FloorAt(top.x, top.z, upper, lower, layer, top.y))
    {
        return false;
    }
    Vec3 face = Along(from, wish, r + 0.08f);
    face.y = top.y - 0.18f;
    if (moments.PushOut(face, 0.13f, layer) < 0.005f)
    {
        return false;
    }
    const Vec3 side{wish.z, 0.0f, -wish.x};
    for (const Vec3& direction : {wish, Vec3{-wish.x, 0.0f, -wish.z}, side, Vec3{-side.x, 0.0f, -side.z}})
    {
        const Vec3 p = Along(top, direction, r * 0.60f);
        float support = 0.0f;
        if (!moments.FloorAt(p.x, p.z, top.y + 0.08f, top.y - 0.10f, layer, support))
        {
            return false;
        }
    }
    top.y += 0.025f;
    if (town.Submerged(top, layer, moments.GetCircle()))
    {
        return false;
    }
    constexpr int samples = 12;
    for (int i = 0; i <= samples; ++i)
    {
        const float t = static_cast<float>(i) / static_cast<float>(samples);
        const Vec3 up{from.x, from.y + (top.y - from.y) * t, from.z};
        Vec3 across = Along(from, wish, reach * t);
        across.y = top.y;
        if (!MantleBodyFits(player, moments, up, layer) || !MantleBodyFits(player, moments, across, layer))
        {
            return false;
        }
    }
    player.CompleteMantle(top);
    return true;
}

// A draw-only traveller. All parts, including the hand-held lantern, use the same local pose in A and B.
inline int AddTownAvatar(scene::SceneObject* out, int count, int capacity, const Player& player, float yaw,
                         float phase, float gait, bool carried)
{
    constexpr int bodyParts = 15;
    constexpr int lampParts = 4;
    if (out == nullptr || count < 0 || count > capacity || capacity - count < bodyParts + (carried ? lampParts : 0))
    {
        return count;
    }
    using scene::ShapeKind;
    const Vec3 feet = player.Feet();
    const float sy = std::sin(yaw);
    const float cy = std::cos(yaw);
    const float run = std::clamp((gait - 1.0f) / 0.6f, 0.0f, 1.0f);
    const float swing = std::sin(phase) * (0.46f + 0.28f * run) * (std::min)(gait, 1.0f);
    const float airborne = player.Grounded() ? 0.0f : 1.0f;
    const float lift = 0.021f * std::fabs(std::sin(phase)) * (std::min)(gait, 1.0f);
    const float lean = 0.16f * run + 0.08f * airborne;
    const auto part = [&](ShapeKind shape, Vec3 p, Vec3 size, Vec3 tint, float tilt = 0.0f, float glow = 0.0f) {
        scene::SceneObject o{};
        o.shape = shape;
        o.position[0] = feet.x + cy * p.x + sy * p.z;
        o.position[1] = feet.y + p.y + lift;
        o.position[2] = feet.z - sy * p.x + cy * p.z;
        o.scale[0] = size.x;
        o.scale[1] = size.y;
        o.scale[2] = size.z;
        o.colorSrgb[0] = tint.x;
        o.colorSrgb[1] = tint.y;
        o.colorSrgb[2] = tint.z;
        o.yaw = yaw;
        o.tilt = tilt;
        o.shineStrength = 0.06f;
        o.shininess = 12.0f;
        o.glow = glow;
        o.layerMask = scene::kLayerBoth;
        out[count++] = o;
    };
    const Vec3 coat{0.16f, 0.34f, 0.38f};
    const Vec3 wool{0.10f, 0.22f, 0.26f};
    const Vec3 leather{0.26f, 0.18f, 0.13f};
    const Vec3 brass{0.65f, 0.42f, 0.22f};
    part(ShapeKind::Cube, {0, 1.18f, 0.04f * run}, {0.47f, 0.49f, 0.29f}, coat, lean);
    part(ShapeKind::Cube, {0, 0.91f, -0.04f}, {0.48f, 0.27f, 0.31f}, wool, -lean * 0.7f);
    part(ShapeKind::Cube, {0, 1.02f, 0.01f}, {0.49f, 0.065f, 0.32f}, leather);
    // Visual stature only. Eye height, body clearance and mantle reach are unchanged.
    constexpr float stature = 1.72f;
    constexpr float hoodHeight = 0.30f;
    part(ShapeKind::Sphere, {0, stature - hoodHeight * 0.5f, 0.08f * run},
         {0.30f, hoodHeight, 0.30f}, wool);
    part(ShapeKind::Sphere, {0, 1.57f, 0.11f + 0.08f * run}, {0.20f, 0.21f, 0.13f}, {0.63f, 0.48f, 0.34f});
    part(ShapeKind::Cylinder, {0, 1.43f, 0.035f}, {0.31f, 0.09f, 0.30f}, {0.63f, 0.37f, 0.20f});
    part(ShapeKind::Cube, {-0.03f, 1.21f, -0.205f}, {0.30f, 0.32f, 0.15f}, leather, lean);
    part(ShapeKind::Cylinder, {-0.03f, 1.36f, -0.22f}, {0.26f, 0.095f, 0.17f}, brass);
    float lampY = 0.0f;
    float lampZ = 0.0f;
    for (int i = -1; i <= 1; i += 2)
    {
        const float side = static_cast<float>(i);
        const float leg = side * (swing + 0.25f * airborne);
        const float angle = (i == 1 && carried) ? -0.46f - swing * 0.18f : -leg * 0.85f;
        const float ankleY = 0.77f - 0.67f * std::cos(leg);
        const float ankleZ = -0.67f * std::sin(leg);
        part(ShapeKind::Cube, {side * 0.13f, (0.77f + ankleY) * 0.5f, ankleZ * 0.5f},
             {0.16f, 0.68f, 0.17f}, {0.19f, 0.21f, 0.22f}, leg);
        part(ShapeKind::Cube, {side * 0.13f, ankleY - 0.025f, ankleZ + 0.045f}, {0.18f, 0.15f, 0.29f}, leather);
        const float handY = 1.36f - 0.45f * std::cos(angle);
        const float handZ = -0.45f * std::sin(angle);
        part(ShapeKind::Cylinder, {side * 0.275f, (1.36f + handY) * 0.5f, handZ * 0.5f},
             {0.15f, 0.46f, 0.15f}, coat, angle);
        if (i == 1)
        {
            lampY = handY;
            lampZ = handZ;
        }
    }
    part(ShapeKind::Cube, {0, 1.02f, 0.18f}, {0.105f, 0.084f, 0.035f}, brass);
    if (carried)
    {
        const float x = 0.31f;
        const float z = lampZ + 0.075f;
        part(ShapeKind::Cube, {x, lampY, z}, {0.055f, 0.13f, 0.05f}, leather);
        part(ShapeKind::Cylinder, {x, lampY - 0.12f, z}, {0.16f, 0.04f, 0.16f}, brass);
        part(ShapeKind::Cylinder, {x, lampY - 0.207f, z}, {0.13f, 0.14f, 0.13f}, {1.0f, 0.68f, 0.30f}, 0.0f, 0.85f);
        part(ShapeKind::Cylinder, {x, lampY - 0.29f, z}, {0.16f, 0.035f, 0.16f}, brass);
    }
    return count;
}
} // namespace dl::town
