#pragma once

#include "scene/Flame.h"
#include "scene/LanternModel.h"
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
inline float CameraClearDistance(const Moments& moments, const Vec3& pivot, const Vec3& forward, float reach, int layer,
                                 bool both)
{
    constexpr float radius = 0.20f;
    constexpr float step = 0.09f;
    constexpr float skin = 0.02f;
    const auto blocked = [&](float distance, int world)
    {
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

inline constexpr int kAvatarBodyObjects = 45;  ///< the parts of the traveller without the lantern
inline constexpr float kAvatarStature = 1.72f; ///< UNITS: metres from the feet to the top of the hood (visual only)
inline constexpr int kAvatarHood = 19;         ///< the index of the hood (a sphere) among the parts of the body
inline constexpr int kAvatarFirstLeg = 23;  ///< the index of the first part of the legs (the left leg, then the right)
inline constexpr int kAvatarLegObjects = 6; ///< the parts of a leg: thigh, knee, shin, boot shaft, boot, toe cap

// A draw-only traveller (docs/DESIGN.md 6.7): a stylised figure of rounded forms (a bell of a coat, a round hood with a
// peak, a scarf that streams behind when he runs, big boots, a small pack with a bedroll) that stands kAvatarStature
// high. All parts, including the hand-held lantern, use the same local pose in A and B.
// Space: local x right, y up, z ahead; the lantern is in the right hand. The legs and the arms have two bones each (a
// hip and a knee, a shoulder and an elbow); the body is lowered as the legs spread, so that a foot is on the floor.
// `flame`: the flame over the lantern in the hand (scene/Flame.h), when it is carried; `raise`: 0 .. 1, how high the
// lantern is held (the arm goes up, forward: the first step of H); `seconds`: the clock of the sparks of the flame,
// the sway of the scarf and the flame inside the lantern; `lamp`: where the glass of the lantern is in the world when
// it is carried (the light of the lantern is put there: scene/Lamps.h).
inline int AddTownAvatar(scene::SceneObject* out, int count, int capacity, const Player& player, float yaw, float phase,
                         float gait, bool carried, const Flame* flame = nullptr, float raise = 0.0f,
                         float seconds = 0.0f, Vec3* lamp = nullptr)
{
    const int flameParts = carried && flame != nullptr ? kFlameObjects : 0;
    if (out == nullptr || count < 0 || count > capacity ||
        capacity - count < kAvatarBodyObjects + (carried ? kLanternModelObjects : 0) + flameParts)
    {
        return count;
    }
    using scene::ShapeKind;
    struct Mat
    {
        float r, g, b, shine, power;
    };
    // Cloth is matte, leather a little smoother, brass a controlled highlight (docs/DESIGN.md 6.7).
    constexpr Mat coat{0.17f, 0.40f, 0.44f, 0.05f, 10.0f};
    constexpr Mat wool{0.12f, 0.28f, 0.33f, 0.03f, 8.0f};
    constexpr Mat leather{0.30f, 0.20f, 0.14f, 0.12f, 20.0f};
    constexpr Mat boot{0.21f, 0.14f, 0.11f, 0.14f, 22.0f};
    constexpr Mat brass{0.62f, 0.40f, 0.20f, 0.35f, 40.0f};
    constexpr Mat skin{0.72f, 0.55f, 0.40f, 0.04f, 10.0f};
    constexpr Mat trousers{0.19f, 0.21f, 0.23f, 0.04f, 10.0f};
    constexpr Mat scarf{0.58f, 0.28f, 0.20f, 0.03f, 8.0f};
    constexpr Mat roll{0.52f, 0.44f, 0.32f, 0.03f, 8.0f};
    constexpr Mat dark{0.07f, 0.06f, 0.07f, 0.0f, 1.0f};
    constexpr float halfPi = 1.5707963f;

    const Vec3 feet = player.Feet();
    const float sy = std::sin(yaw);
    const float cy = std::cos(yaw);
    const float run = std::clamp((gait - 1.0f) / 0.6f, 0.0f, 1.0f);
    const float g = (std::min)(gait, 1.0f);
    const float swing = std::sin(phase) * (0.44f + 0.16f * run) * g;
    const float airborne = player.Grounded() ? 0.0f : 1.0f;
    const float lean = 0.16f * run + 0.08f * airborne;
    const float raised = std::clamp(raise, 0.0f, 1.0f);
    const float lc = std::cos(lean);
    const float ls = std::sin(lean);

    // ---- the legs first: the body stands on whichever foot is lowest
    constexpr float thighLen = 0.36f;
    constexpr float shinLen = 0.36f;
    constexpr float hipRest = 0.82f;
    constexpr float ankleRest = 0.10f;
    struct Leg
    {
        float side, thigh, shin, hipX;
        float kneeY, kneeZ, ankleY, ankleZ; // relative to the hip
    };
    const auto legOf = [&](float side)
    {
        Leg l{};
        l.side = side;
        l.hipX = side * 0.115f;
        // + is a foot behind. A leg that swings forward bends its knee; in the air both are tucked.
        l.thigh = side * (swing + 0.30f * airborne);
        const float forward = (std::max)(0.0f, -side * std::cos(phase)) * g;
        l.shin = l.thigh + 0.95f * forward * (1.0f - airborne) + 0.80f * airborne;
        l.kneeY = -thighLen * std::cos(l.thigh);
        l.kneeZ = -thighLen * std::sin(l.thigh);
        l.ankleY = l.kneeY - shinLen * std::cos(l.shin);
        l.ankleZ = l.kneeZ - shinLen * std::sin(l.shin);
        return l;
    };
    const Leg legs[2] = {legOf(-1.0f), legOf(1.0f)};
    // The lowest ankle is on the floor; the hips come down as the legs spread (a little less than that, so that a
    // stride is a stride and not a squat).
    const float hipsLow = ankleRest - (std::min)(legs[0].ankleY, legs[1].ankleY);
    const float up = airborne > 0.5f ? 0.0f : (std::min)(0.0f, 0.75f * (hipsLow - hipRest));

    const auto world = [&](const Vec3& p) -> Vec3
    { return {feet.x + cy * p.x + sy * p.z, feet.y + p.y, feet.z - sy * p.x + cy * p.z}; };
    const auto part =
        [&](ShapeKind shape, const Vec3& p, const Vec3& size, const Mat& m, float tilt = 0.0f, float yawOffset = 0.0f)
    {
        scene::SceneObject o{};
        const Vec3 w = world(p);
        o.shape = shape;
        o.position[0] = w.x;
        o.position[1] = w.y;
        o.position[2] = w.z;
        o.scale[0] = size.x;
        o.scale[1] = size.y;
        o.scale[2] = size.z;
        o.colorSrgb[0] = m.r;
        o.colorSrgb[1] = m.g;
        o.colorSrgb[2] = m.b;
        o.yaw = yaw + yawOffset;
        o.tilt = tilt;
        o.shineStrength = m.shine;
        o.shininess = m.power;
        o.layerMask = scene::kLayerBoth;
        out[count++] = o;
    };
    // The upper body leans about the waist (forward when he runs).
    constexpr float waistY = 0.98f;
    const auto onTorso = [&](float x, float y, float z) -> Vec3
    {
        const float dy = y - waistY;
        return {x, waistY + up + dy * lc - z * ls, dy * ls + z * lc};
    };
    // A cylinder between two points of the plane of the body (a limb): a is the upper end.
    const auto limb = [&](const Vec3& a, const Vec3& b, float diameter, const Mat& m)
    {
        const float dy = b.y - a.y;
        const float dz = b.z - a.z;
        part(ShapeKind::Cylinder, {a.x, 0.5f * (a.y + b.y), 0.5f * (a.z + b.z)},
             {diameter, std::hypot(dy, dz), diameter}, m, std::atan2(-dz, -dy));
    };
    // The centre of a strip of cloth that hangs from `hinge` and swings back by `angle`.
    const auto hung = [](const Vec3& hinge, float length, float angle) -> Vec3
    { return {hinge.x, hinge.y - 0.5f * length * std::cos(angle), hinge.z - 0.5f * length * std::sin(angle)}; };

    // ---- the body
    const float breath = 0.004f * std::sin(seconds * 1.7f) * (1.0f - g);
    part(ShapeKind::Sphere, onTorso(0.0f, 1.20f + breath, 0.005f), {0.43f, 0.44f, 0.29f}, coat, lean);
    part(ShapeKind::Cylinder, onTorso(0.0f, 1.02f, 0.0f), {0.33f, 0.20f, 0.25f}, coat, lean);
    part(ShapeKind::Cylinder, onTorso(0.0f, 0.985f, 0.0f), {0.345f, 0.045f, 0.265f}, leather, lean);
    part(ShapeKind::Cube, onTorso(0.0f, 0.985f, 0.132f), {0.065f, 0.05f, 0.02f}, brass, lean);
    part(ShapeKind::Cube, onTorso(-0.17f, 0.93f, -0.13f), {0.075f, 0.10f, 0.07f}, leather, lean);
    // the coat: a bell that hangs from the waist, and a vent behind that the stride and the wind lift
    part(ShapeKind::Sphere, {0.0f, 0.735f + up, -0.025f}, {0.44f, 0.44f, 0.32f}, coat);
    const float flap = 0.08f + 0.32f * run + 0.06f * std::sin(phase) * g;
    part(ShapeKind::Sphere, hung({0.0f, 0.86f + up, -0.12f}, 0.28f, flap), {0.22f, 0.27f, 0.075f}, coat, flap);
    // the pack, with its bedroll across the top, and the straps over the chest
    part(ShapeKind::Cube, onTorso(0.0f, 1.16f, -0.215f), {0.26f, 0.27f, 0.13f}, leather, lean);
    part(ShapeKind::Cylinder, onTorso(0.0f, 1.33f, -0.235f), {0.12f, 0.34f, 0.12f}, roll, halfPi + lean, halfPi);
    part(ShapeKind::Cube, onTorso(-0.115f, 1.21f, 0.13f), {0.045f, 0.30f, 0.02f}, leather, lean);
    part(ShapeKind::Cube, onTorso(0.115f, 1.21f, 0.13f), {0.045f, 0.30f, 0.02f}, leather, lean);
    for (int k = 0; k < 3; ++k)
    {
        const float y = 1.30f - 0.10f * static_cast<float>(k);
        const float depth = 0.15f * std::sqrt((std::max)(0.0f, 1.0f - ((y - 1.20f) / 0.22f) * ((y - 1.20f) / 0.22f)));
        part(ShapeKind::Sphere, onTorso(0.0f, y, depth + 0.006f), {0.026f, 0.026f, 0.026f}, brass);
    }
    // the scarf: a collar over the chin, a knot on the left shoulder and two tails that stream behind
    part(ShapeKind::Cylinder, onTorso(0.0f, 1.43f, 0.0f), {0.235f, 0.10f, 0.225f}, scarf, lean);
    part(ShapeKind::Sphere, onTorso(-0.13f, 1.40f, -0.06f), {0.10f, 0.10f, 0.10f}, scarf);
    for (int k = 0; k < 2; ++k)
    {
        const float length = k == 0 ? 0.30f : 0.23f;
        const float angle = 0.25f + 0.85f * run + 0.10f * std::sin(seconds * 3.2f + 1.7f * static_cast<float>(k)) +
                            0.12f * std::sin(phase) * g;
        const Vec3 hinge = onTorso(-0.15f - 0.05f * static_cast<float>(k), 1.395f, -0.09f);
        part(ShapeKind::Cube, hung(hinge, length, angle), {0.065f - 0.008f * static_cast<float>(k), length, 0.022f},
             scarf, angle);
    }
    // the head: a face in a round hood with a peak; the eyes are two dark marks
    part(ShapeKind::Sphere, onTorso(0.0f, 1.535f, 0.095f), {0.20f, 0.215f, 0.20f}, skin, lean * 0.4f);
    part(ShapeKind::Sphere, onTorso(0.0f, kAvatarStature - 0.17f, -0.015f), {0.36f, 0.34f, 0.37f}, wool, lean * 0.4f);
    part(ShapeKind::Sphere, onTorso(0.0f, 1.585f, -0.20f), {0.15f, 0.22f, 0.15f}, wool, -1.0f + lean * 0.4f);
    part(ShapeKind::Cube, onTorso(-0.042f, 1.55f, 0.187f), {0.026f, 0.040f, 0.016f}, dark, lean * 0.4f);
    part(ShapeKind::Cube, onTorso(0.042f, 1.55f, 0.187f), {0.026f, 0.040f, 0.016f}, dark, lean * 0.4f);

    // ---- the legs: a thigh, a knee, a shin, the shaft of a boot and the boot
    for (const Leg& l : legs)
    {
        const Vec3 hip{l.hipX, hipRest + up, 0.0f};
        const Vec3 knee{l.hipX, hip.y + l.kneeY, l.kneeZ};
        const Vec3 ankle{l.hipX, hip.y + l.ankleY, l.ankleZ};
        limb(hip, knee, 0.135f, trousers);
        part(ShapeKind::Sphere, knee, {0.125f, 0.125f, 0.125f}, trousers);
        limb(knee, ankle, 0.11f, trousers);
        const float up2 = 0.075f; // the shaft stands on the shin
        part(ShapeKind::Cylinder, {ankle.x, ankle.y + up2 * std::cos(l.shin), ankle.z + up2 * std::sin(l.shin)},
             {0.145f, 0.20f, 0.145f}, boot, l.shin);
        const float toe = 0.4f * l.shin; // a foot behind lifts its heel, a foot ahead its toe
        part(ShapeKind::Cube, {ankle.x, ankle.y - 0.045f, ankle.z + 0.03f}, {0.145f, 0.10f, 0.20f}, boot, toe);
        part(ShapeKind::Sphere, {ankle.x, ankle.y - 0.05f, ankle.z + 0.15f}, {0.145f, 0.095f, 0.13f}, boot, toe);
    }

    // ---- the arms: a shoulder, an upper arm, a forearm, a cuff and a glove; the right hand holds the lantern
    Vec3 hand{};
    for (int i = -1; i <= 1; i += 2)
    {
        const float side = static_cast<float>(i);
        const bool holds = i == 1 && carried;
        float shoulderAngle = -0.8f * side * swing - 0.35f * airborne;
        float flex = 0.20f + 0.45f * run + 0.15f * airborne;
        if (holds)
        {
            shoulderAngle = -0.42f - 0.20f * swing;
            flex = 1.0f;
            shoulderAngle += (-1.75f - shoulderAngle) * raised;
            flex += (0.85f - flex) * raised;
        }
        const float x = side * 0.25f;
        const Vec3 shoulder = onTorso(x, 1.34f, 0.0f);
        const float lowerAngle = shoulderAngle - flex;
        const Vec3 elbow{x, shoulder.y - 0.27f * std::cos(shoulderAngle), shoulder.z - 0.27f * std::sin(shoulderAngle)};
        const Vec3 wrist{x, elbow.y - 0.25f * std::cos(lowerAngle), elbow.z - 0.25f * std::sin(lowerAngle)};
        const Vec3 along{0.0f, -std::cos(lowerAngle), -std::sin(lowerAngle)};
        part(ShapeKind::Sphere, shoulder, {0.15f, 0.15f, 0.15f}, coat);
        limb(shoulder, elbow, 0.105f, coat);
        limb(elbow, wrist, 0.095f, coat);
        part(ShapeKind::Cylinder, {x, wrist.y - 0.03f * along.y, wrist.z - 0.03f * along.z}, {0.115f, 0.06f, 0.115f},
             leather, lowerAngle);
        const Vec3 palm{x, wrist.y + 0.045f * along.y, wrist.z + 0.045f * along.z};
        part(ShapeKind::Sphere, palm, {0.095f, 0.095f, 0.095f}, leather);
        if (holds)
        {
            hand = palm;
        }
    }

    // ---- the lantern, on the right hand; it swings a little as a lantern on a ring does
    if (carried)
    {
        const float swingBack =
            0.28f * std::cos(phase) * g * (1.0f - raised) + 0.10f * run + 0.03f * std::sin(seconds * 1.7f);
        const Vec3 hook = world({hand.x + 0.04f, hand.y - 0.02f, hand.z}); // the ring hangs a little outside the hand
        Vec3 flameBase{};
        Vec3 glass{};
        count = AddLanternModel(out, count, capacity, hook, yaw, swingBack, seconds, &flameBase, &glass);
        if (lamp != nullptr)
        {
            *lamp = glass;
        }
        if (flame != nullptr)
        {
            count = AddFlame(out, count, capacity, flameBase, *flame, seconds);
        }
    }
    return count;
}
} // namespace dl::town
