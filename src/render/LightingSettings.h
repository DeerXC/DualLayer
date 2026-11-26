// LightingSettings.h - every number of the light of the town that is not in the model: what the config file [lighting]
// says. Why:       the model (tools/layout/city_model.py LIGHT) gives the sun, the moon, the sky and the fog of each
// moment; the game
//            adds the shadows, the point lights, the exposure and the glow, and these are the knobs of them. F5 reloads
//            them.
// Rules:     a value that makes no sense is pulled into a range that works (Sanitize); a missing key keeps the default.
// The
//            defaults are the ones the lighting was made with: A, the warm evening, as the model has it; B, the cold
//            present, a little darker so that the lamps and the lantern read in it.
// Plain data, no Windows, no Direct3D: tested without a window (tests/test_lighting.cpp).
// UNITS:     metres; texels for the shadow map; the other numbers are factors (1 = as the model says).
#pragma once

#include "core/Config.h"

#include <algorithm>
#include <cmath>

namespace dl
{

struct LightingSettings
{
    // ---- the shadow of the sun and the moon
    bool shadows = true;        ///< a shadow map of the sun or the moon (a lamp casts none)
    int shadowSize = 2048;      ///< texels along a side: 512 .. 4096, a power of two
    float shadowRadius = 60.0f; ///< UNITS: metres: the map covers this far round the camera (half its width)
    float shadowStrength =
        0.85f;                 ///< 0 .. 1: how much of the direct light a shadow takes (the sky light is never taken)
    float shadowBias = 3.0f;   ///< texels: depth bias; more takes acne away and lifts the shadow off its object
    float shadowNormal = 1.5f; ///< texels: a surface is moved this far along its normal before the shadow is looked up
    // ---- the point lights
    bool pointLights = true;
    float lampsA = 0.55f; ///< the street lamps in A: low, the sun is up (the evening)
    float lampsB = 1.0f;  ///< the street lamps in B: the only light but the moon
    float lantern = 1.0f; ///< the lantern of the traveller (and the lit lamps of the mast and the bells)
    // ---- the sun, the sky and the air
    float sunA = 1.0f;        ///< the light of the sun of A
    float sunB = 0.8f;        ///< the light of the moon of B
    float ambientA = 1.0f;    ///< the sky light of A
    float ambientB = 0.9f;    ///< the sky light of B
    float sunScatter = 0.25f; ///< how much of the light of the sun the fog scatters toward the sun: a haze round it
    // ---- the picture
    float exposure = 1.0f; ///< the light is multiplied by this before the shoulder
    float knee = 0.78f;    ///< 0.3 .. 0.98: what is brighter than this is bent toward 1 instead of cut off
    bool halos = true;     ///< a soft glow round a lit lamp
    float haloA = 0.35f;   ///< its strength in A (the sun hides it) ...
    float haloB = 1.0f;    ///< ... and in B
};

inline float LightingClamp(float v, float lo, float hi, float fallback)
{
    return std::isfinite(v) ? std::clamp(v, lo, hi) : fallback;
}

/// A size that is a power of two between 512 and 4096 (the nearest below), 2048 when it is nonsense.
inline int LightingShadowSize(int size)
{
    if (size < 512)
    {
        return size <= 0 ? 2048 : 512;
    }
    int s = 512;
    while (s * 2 <= size && s < 4096)
    {
        s *= 2;
    }
    return s;
}

/// Pulls every value into the range that works.
inline LightingSettings Sanitize(const LightingSettings& in)
{
    const LightingSettings d{};
    LightingSettings s = in;
    s.shadowSize = LightingShadowSize(in.shadowSize);
    s.shadowRadius = LightingClamp(in.shadowRadius, 15.0f, 200.0f, d.shadowRadius);
    s.shadowStrength = LightingClamp(in.shadowStrength, 0.0f, 1.0f, d.shadowStrength);
    s.shadowBias = LightingClamp(in.shadowBias, 0.0f, 20.0f, d.shadowBias);
    s.shadowNormal = LightingClamp(in.shadowNormal, 0.0f, 10.0f, d.shadowNormal);
    s.lampsA = LightingClamp(in.lampsA, 0.0f, 8.0f, d.lampsA);
    s.lampsB = LightingClamp(in.lampsB, 0.0f, 8.0f, d.lampsB);
    s.lantern = LightingClamp(in.lantern, 0.0f, 8.0f, d.lantern);
    s.sunA = LightingClamp(in.sunA, 0.0f, 4.0f, d.sunA);
    s.sunB = LightingClamp(in.sunB, 0.0f, 4.0f, d.sunB);
    s.ambientA = LightingClamp(in.ambientA, 0.0f, 4.0f, d.ambientA);
    s.ambientB = LightingClamp(in.ambientB, 0.0f, 4.0f, d.ambientB);
    s.sunScatter = LightingClamp(in.sunScatter, 0.0f, 2.0f, d.sunScatter);
    s.exposure = LightingClamp(in.exposure, 0.1f, 4.0f, d.exposure);
    s.knee = LightingClamp(in.knee, 0.3f, 0.98f, d.knee);
    s.haloA = LightingClamp(in.haloA, 0.0f, 4.0f, d.haloA);
    s.haloB = LightingClamp(in.haloB, 0.0f, 4.0f, d.haloB);
    return s;
}

/// Reads [lighting] from the config: every key is optional.
inline LightingSettings ReadLightingSettings(const Config& c)
{
    const LightingSettings d{};
    LightingSettings s;
    s.shadows = c.GetBool("lighting", "shadows", d.shadows);
    s.shadowSize = c.GetInt("lighting", "shadow_size", d.shadowSize);
    s.shadowRadius = c.GetFloat("lighting", "shadow_radius", d.shadowRadius);
    s.shadowStrength = c.GetFloat("lighting", "shadow_strength", d.shadowStrength);
    s.shadowBias = c.GetFloat("lighting", "shadow_bias", d.shadowBias);
    s.shadowNormal = c.GetFloat("lighting", "shadow_normal", d.shadowNormal);
    s.pointLights = c.GetBool("lighting", "point_lights", d.pointLights);
    s.lampsA = c.GetFloat("lighting", "lamps_a", d.lampsA);
    s.lampsB = c.GetFloat("lighting", "lamps_b", d.lampsB);
    s.lantern = c.GetFloat("lighting", "lantern", d.lantern);
    s.sunA = c.GetFloat("lighting", "sun_a", d.sunA);
    s.sunB = c.GetFloat("lighting", "sun_b", d.sunB);
    s.ambientA = c.GetFloat("lighting", "ambient_a", d.ambientA);
    s.ambientB = c.GetFloat("lighting", "ambient_b", d.ambientB);
    s.sunScatter = c.GetFloat("lighting", "sun_scatter", d.sunScatter);
    s.exposure = c.GetFloat("lighting", "exposure", d.exposure);
    s.knee = c.GetFloat("lighting", "knee", d.knee);
    s.halos = c.GetBool("lighting", "halos", d.halos);
    s.haloA = c.GetFloat("lighting", "halo_a", d.haloA);
    s.haloB = c.GetFloat("lighting", "halo_b", d.haloB);
    return Sanitize(s);
}

} // namespace dl
