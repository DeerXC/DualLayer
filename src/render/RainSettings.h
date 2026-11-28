// RainSettings.h - every number of the rain of B (docs/DESIGN.md 6.11, stage R1): what the config file [rain] says, the
// registers that the rain pass reads (shaders/rain.hlsl) and the numbers of the wet look that the model shader reads
// (shaders/models.hlsl).
// Why:       B has a fine steady rain and A has none (6.11); these are the knobs of it. F5 reloads them.
// Rules:     a value that makes no sense is pulled into a range that works (Sanitize); a missing key keeps the default.
//            The rain is for B only and never in the plain view (the key L ends its round with "plain"): MakeRain and
//            MakeWet give nothing then.
// UNITS:     pixels at a screen 1080 pixels high (the shader scales them with the height of the picture); metres;
//            seconds.
// Plain data, no Windows, no Direct3D: tested without a window (tests/test_style.cpp, TestRain).
#pragma once

#include "core/Config.h"

#include <algorithm>
#include <cmath>

namespace dl
{

struct RainSettings
{
    float amount = 1.0f;     ///< 0 .. 1: the master of all of it (0: no rain, no wet look)
    float density = 1.0f;    ///< 0 .. 2: how many streaks
    float speed = 1.0f;      ///< 0.3 .. 3: how fast they fall
    float length = 1.0f;     ///< 0.3 .. 1.8: how long a streak is
    float width = 1.0f;      ///< 0.5 .. 3: how wide a streak is
    float alpha = 1.0f;      ///< 0 .. 1: how visible the streaks and the rings are
    float rings = 1.0f;      ///< 0 .. 2: how many rings where drops fall on the ground
    float ringReach = 22.0f; ///< 5 .. 80: UNITS: metres: how far from the camera the rings are drawn
    float wet = 1.0f;        ///< 0 .. 1: the wet look of stone (darker, a sheen, the sky reflected)
    float overcast = 0.6f;   ///< 0 .. 1: the sky under cloud, and the moon as a soft glow
    float fog = 0.25f;       ///< 0 .. 1: how much thicker the fog of B is in rain
};

inline float RainClamp(float v, float lo, float hi, float fallback)
{
    return std::isfinite(v) ? std::clamp(v, lo, hi) : fallback;
}

inline RainSettings Sanitize(const RainSettings& in)
{
    const RainSettings d{};
    RainSettings s;
    s.amount = RainClamp(in.amount, 0.0f, 1.0f, d.amount);
    s.density = RainClamp(in.density, 0.0f, 2.0f, d.density);
    s.speed = RainClamp(in.speed, 0.3f, 3.0f, d.speed);
    s.length = RainClamp(in.length, 0.3f, 1.8f, d.length);
    s.width = RainClamp(in.width, 0.5f, 3.0f, d.width);
    s.alpha = RainClamp(in.alpha, 0.0f, 1.0f, d.alpha);
    s.rings = RainClamp(in.rings, 0.0f, 2.0f, d.rings);
    s.ringReach = RainClamp(in.ringReach, 5.0f, 80.0f, d.ringReach);
    s.wet = RainClamp(in.wet, 0.0f, 1.0f, d.wet);
    s.overcast = RainClamp(in.overcast, 0.0f, 1.0f, d.overcast);
    s.fog = RainClamp(in.fog, 0.0f, 1.0f, d.fog);
    return s;
}

inline RainSettings ReadRainSettings(const Config& c)
{
    const RainSettings d{};
    RainSettings s;
    s.amount = c.GetFloat("rain", "amount", d.amount);
    s.density = c.GetFloat("rain", "density", d.density);
    s.speed = c.GetFloat("rain", "speed", d.speed);
    s.length = c.GetFloat("rain", "length", d.length);
    s.width = c.GetFloat("rain", "width", d.width);
    s.alpha = c.GetFloat("rain", "alpha", d.alpha);
    s.rings = c.GetFloat("rain", "rings", d.rings);
    s.ringReach = c.GetFloat("rain", "ring_reach", d.ringReach);
    s.wet = c.GetFloat("rain", "wet", d.wet);
    s.overcast = c.GetFloat("rain", "overcast", d.overcast);
    s.fog = c.GetFloat("rain", "fog", d.fog);
    return Sanitize(s);
}

/// What the rain pass reads (the shader's constant buffer, fifteen registers of four floats; shaders/rain.hlsl says
/// what each holds).
struct RainConstants
{
    float view[4];
    float clock[4];
    float fall[4];
    float layerA[4];
    float layerB[4];
    float layerC[4];
    float depths[4];
    float look[4];
    float tint[4];
    float eye[4];
    float right[4];
    float up[4];
    float forward[4];
    float ground[4];
    float lantern[4];
};
static_assert(sizeof(RainConstants) == 240, "cbuffer Rain of shaders/rain.hlsl is 15 registers of 16 bytes");

/// What the camera and the world give the rain of one picture. All the vectors are unit length, in the world.
struct RainView
{
    float tanAcross = 1.0f; ///< tan of half the field of view across (1 / projection[0][0])
    float tanUp = 1.0f;     ///< tan of half the field of view up (1 / projection[1][1])
    float nearZ = 0.1f;     ///< UNITS: metres
    float farZ = 300.0f;    ///< UNITS: metres
    float heightPixels = 1080.0f;
    float time = 0.0f; ///< UNITS: seconds on the clock of the water
    float right[3] = {1.0f, 0.0f, 0.0f};
    float up[3] = {0.0f, 1.0f, 0.0f};
    float forward[3] = {0.0f, 0.0f, 1.0f};
    float eye[3] = {0.0f, 0.0f, 0.0f};
    float travel[3] = {0.0f, -1.0f, 0.0f}; ///< the direction that the rain falls in (the way the moonlight travels)
    float lantern[4] = {0.0f, 0.0f, 0.0f, 0.0f}; ///< the dry circle: xyz its centre, w its radius (0: none)
};

/// The clock of the rain: the time of the water, brought back every 300 s so that the cells of the pattern stay small
/// numbers (the hash of a big number loses its bits). The rain jumps to another pattern once in five minutes, which
/// nobody sees.
inline float RainClock(float seconds)
{
    return std::fmod(std::max(seconds, 0.0f), 300.0f);
}

/// The direction in which the rain falls on the screen, in pixels (x right, y DOWN), from the direction that it falls
/// in the world and the right and the up of the camera. A rain that falls straight down and a camera that looks along
/// the ground give (0, 1); a camera that looks straight up or down (no sideways in the picture) gives (0, 1) as well.
inline void RainScreenDirection(const float travel[3], const float right[3], const float up[3], float out[2])
{
    const float vx = travel[0] * right[0] + travel[1] * right[1] + travel[2] * right[2];
    const float vy = travel[0] * up[0] + travel[1] * up[1] + travel[2] * up[2];
    const float length = std::sqrt(vx * vx + vy * vy);
    if (!(length > 0.05f))
    {
        out[0] = 0.0f;
        out[1] = 1.0f;
        return;
    }
    out[0] = vx / length;
    out[1] = -vy / length;
}

inline float RainLinear(float s)
{
    return s <= 0.04045f ? s / 12.92f : std::pow((s + 0.055f) / 1.055f, 2.4f);
}

/// The registers of the rain pass for the picture of moment `layer` (0 = A, 1 = B). A gives nothing (an amount of 0),
/// and so does the plain view: the pass is not drawn then.
inline RainConstants MakeRain(const RainSettings& in, int layer, bool plain, const RainView& v)
{
    const RainSettings s = Sanitize(in);
    const float amount = (layer == 1 && !plain) ? s.amount : 0.0f;
    const float px = std::max(1.0f, v.heightPixels / 1080.0f);
    RainConstants c{};
    c.view[0] = v.tanAcross;
    c.view[1] = v.tanUp;
    c.view[2] = v.nearZ;
    c.view[3] = v.farZ;
    c.clock[0] = RainClock(v.time);
    c.clock[1] = px;
    c.clock[2] = amount;
    c.clock[3] = std::min(0.26f * s.rings, 1.0f);
    float fall[2];
    RainScreenDirection(v.travel, v.right, v.up, fall);
    c.fall[0] = fall[0];
    c.fall[1] = fall[1];
    c.fall[2] = s.ringReach;
    c.fall[3] = 2.0f * v.tanUp / std::max(v.heightPixels, 1.0f);
    // three layers, near to far: the cell (pixels of 1080 high), the share of cells with a streak, the speed (pixels a
    // second)
    c.layerA[0] = 46.0f;
    c.layerA[1] = 420.0f;
    c.layerA[2] = std::min(0.09f * s.density, 1.0f);
    c.layerA[3] = 1500.0f * s.speed;
    c.layerB[0] = 28.0f;
    c.layerB[1] = 260.0f;
    c.layerB[2] = std::min(0.24f * s.density, 1.0f);
    c.layerB[3] = 1100.0f * s.speed;
    c.layerC[0] = 16.0f;
    c.layerC[1] = 150.0f;
    c.layerC[2] = std::min(0.50f * s.density, 1.0f);
    c.layerC[3] = 800.0f * s.speed;
    c.depths[0] = 2.5f; // UNITS: metres: where the three layers stand
    c.depths[1] = 8.0f;
    c.depths[2] = 24.0f;
    c.depths[3] = 1.2f; // the cell of the rings
    c.look[0] = std::min(0.5f * s.length, 0.9f);
    c.look[1] = 2.2f * s.width;
    c.look[2] = 0.50f * s.alpha;
    c.look[3] = 0.55f * s.alpha;
    c.tint[0] = RainLinear(0.78f); // a pale grey-blue
    c.tint[1] = RainLinear(0.84f);
    c.tint[2] = RainLinear(0.94f);
    c.tint[3] = 0.32f; // the radius of a ring: UNITS: metres
    c.eye[0] = v.eye[0];
    c.eye[1] = v.eye[1];
    c.eye[2] = v.eye[2];
    c.eye[3] = 1.4f; // the rings: cycles a second
    for (int i = 0; i < 3; ++i)
    {
        c.right[i] = v.right[i];
        c.up[i] = v.up[i];
        c.forward[i] = v.forward[i];
    }
    // the world up in the view space of the camera: its components on the right, the up and the forward of the camera
    c.ground[0] = v.right[1];
    c.ground[1] = v.up[1];
    c.ground[2] = v.forward[1];
    for (int i = 0; i < 4; ++i)
    {
        c.lantern[i] = v.lantern[i];
    }
    return c;
}

/// What the model shader reads of the rain of the moment that it draws (cbuffer Frame, styleMore.z and .w), and how
/// much thicker its fog is.
struct RainWet
{
    float wet = 0.0f;      ///< 0 .. 1: the wet look of stone
    float overcast = 0.0f; ///< 0 .. 1: the sky under cloud
    float fogScale = 1.0f; ///< the fog of the moment is multiplied by this (1 or more)
};

/// A gives nothing and neither does the plain view: the lantern's circle in B is drawn with the frame of A, which is
/// dry.
inline RainWet MakeWet(const RainSettings& in, int layer, bool plain)
{
    const RainSettings s = Sanitize(in);
    RainWet w;
    if (layer == 1 && !plain)
    {
        w.wet = s.wet * s.amount;
        w.overcast = s.overcast * s.amount;
        w.fogScale = 1.0f + s.fog * s.amount;
    }
    return w;
}

} // namespace dl
