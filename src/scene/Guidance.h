// Guidance.h - guidance that grows with time and with the bells (docs/LEVEL_DESIGN.md 1.3 and 9.3).
// Resonance: after the second bell, the lantern pulses while a hidden light not yet taken is within kResonanceRange, faster
//            as it comes nearer; when that light is not ahead (more than 45 degrees off the view), a mark at the edge of the
//            screen shows its side (ResonanceAt; main.cpp draws it).
// Stuck:     the hint bars (outlines and edges of the other world, glow kHintGlow) inside the room the body has been in for
//            kBrightenAfter seconds grow brighter; after kPulseAfter they pulse (BrightenRoom). The guide figure of the plan,
//            which shows the key step after 60 s, is not built: the pulse stands in for it.
// Plain logic, no Windows, no Direct3D: tested without a window (tests/test_stage.cpp, TestGuidance). UNITS: metres, seconds.
#pragma once

#include "scene/Stage.h"
#include "scene/StageHints.h"
#include "scene/StageLights.h"

#include <cmath>

namespace dl::stage
{

inline constexpr float kResonanceRange = 25.0f; ///< UNITS: metres
inline constexpr int kResonanceBells = 2;        ///< bells rung before the lantern resonates
inline constexpr float kBrightenAfter = 30.0f;   ///< UNITS: seconds in one room
inline constexpr float kPulseAfter = 60.0f;

struct Resonance
{
    float strength = 0.0f; ///< 0 = nothing near; 1 = at the light
    float rate = 0.0f;     ///< UNITS: pulses per second
    int side = 0;          ///< -1 the light is to the left, +1 to the right, 0 ahead (or nothing)
    int light = -1;        ///< the nearest light not taken, within range
};

/// The resonance for a body with its feet at `feet`, looking along `look`, with `bellsRung` bells rung.
inline Resonance ResonanceAt(const LightSet& lights, int bellsRung, const Vec3& feet, const Vec3& look)
{
    Resonance r;
    if (bellsRung < kResonanceBells)
    {
        return r;
    }
    float best = kResonanceRange;
    for (int i = 0; i < kHiddenLightCount; ++i)
    {
        if (lights.Taken(i))
        {
            continue;
        }
        const Vec3 c = kHiddenLights[i].centre;
        const float dx = c.x - feet.x;
        const float dy = c.y - feet.y;
        const float dz = c.z - feet.z;
        const float d = std::sqrt(dx * dx + dy * dy + dz * dz);
        if (d < best)
        {
            best = d;
            r.light = i;
        }
    }
    if (r.light < 0)
    {
        return r;
    }
    r.strength = 1.0f - best / kResonanceRange;
    r.rate = 0.8f + 3.2f * r.strength;
    const Vec3 c = kHiddenLights[r.light].centre;
    float tx = c.x - feet.x;
    float tz = c.z - feet.z;
    float lx = look.x;
    float lz = look.z;
    const float tl = std::sqrt(tx * tx + tz * tz);
    const float ll = std::sqrt(lx * lx + lz * lz);
    if (tl > 1.0e-4f && ll > 1.0e-4f)
    {
        tx /= tl;
        tz /= tl;
        lx /= ll;
        lz /= ll;
        const float dot = tx * lx + tz * lz;
        const float cross = lz * tx - lx * tz; // > 0: the light is to the right (left-handed, y up, looking along +z)
        if (dot < 0.7071f)
        {
            r.side = cross > 0.0f ? 1 : -1;
        }
    }
    return r;
}

/// Brightens the hint bars of `table[from .. count)` that lie in `room` (its box, 1 m round it), when the body has been in it
/// for `seconds`. Returns how many bars were brightened.
inline int BrightenRoom(SceneObject* table, int from, int count, int room, float seconds, float clock)
{
    if (table == nullptr || room < 0 || room >= kRoomCount || seconds < kBrightenAfter)
    {
        return 0;
    }
    float amount = (seconds - kBrightenAfter) / 5.0f;
    amount = amount > 1.0f ? 1.0f : amount;
    if (seconds >= kPulseAfter)
    {
        amount *= 0.6f + 0.4f * (0.5f + 0.5f * std::sin(6.2831853f * 1.2f * clock));
    }
    const Room& r = kRooms[room];
    int n = 0;
    for (int i = from; i < count; ++i)
    {
        SceneObject& o = table[i];
        if (std::fabs(o.glow - kHintGlow) > 1.0e-4f)
        {
            continue;
        }
        if (o.position[0] < r.x0 - 1.0f || o.position[0] > r.x1 + 1.0f || o.position[2] < r.z0 - 1.0f || o.position[2] > r.z1 + 1.0f ||
            o.position[1] < r.y0 - 1.0f || o.position[1] > r.y1 + 6.0f)
        {
            continue;
        }
        o.glow = kHintGlow + 0.9f * amount;
        for (float& c : o.colorSrgb)
        {
            c = c + (1.0f - c) * 0.4f * amount;
        }
        ++n;
    }
    return n;
}

} // namespace dl::stage
