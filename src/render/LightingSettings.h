// LightingSettings.h - every number of the light of the town that is not in the model: what the config file [lighting]
// says. Why:       the model (tools/layout/city_model.py LIGHT) gives the sun, the moon, the sky and the fog of each
// moment; the game
//            adds the shadows, the point lights, the exposure and the glow, and these are the knobs of them. F5 reloads
//            them.
// Rules:     a value that makes no sense is pulled into a range that works (Sanitize); a missing key keeps the default.
// The
//            defaults are the ones the lighting was made with: A, the warm evening, as the model has it; B, the cold
//            present, a little darker so that the lamps and the lantern read in it.
// Style:     since 2026-11-26 the light is stylised (shaders/lighting.hlsli, "the style of the light"): the numbers of
// it are
//            below, `style` is their master and 0 gives the plain light that was before. MakeStyle turns them into the
//            five registers that the model shader reads; the colours of the looks of A and B are in StyleLookOf.
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
    // ---- the style of the light (every amount 0 .. 1 unless said; 0 is the plain light)
    float style = 1.0f;     ///< the master of all of it: 0 is the plain light, 1 the style as set below
    float toon = 0.6f;      ///< the sun falls on a face in soft bands (three steps) instead of a smooth slope
    float cool = 0.75f;     ///< the sky light of a face that the sun does not reach leans toward a cool colour
    float rim = 0.28f;      ///< the rim of an edge that faces the light (the stone of the city has a third of it)
    float cloudsA = 0.38f;  ///< the part of the sun that a cloud takes in A: patches of shadow that drift
    float cloudsB = 0.28f;  ///< ... and in B
    float lampRings = 0.6f; ///< the pool of a lamp in rings, with a whiter core and an amber edge
    bool shafts = true;     ///< rays of light in the air (they need the shadows)
    float shaftsA = 0.55f;  ///< their strength in A ...
    float shaftsB = 0.40f;  ///< ... and in B
    int shaftSteps = 14;    ///< samples along a ray of sight: 4 .. 32 (the cost of the shafts)
    float grade =
        1.0f; ///< the grade of the picture: the darks and the lights leaning toward the colours of StyleLookOf
    float saturation =
        1.0f; ///< 0 .. 2: how much of the colour push of the look is used (0: none, 1: as set in StyleLookOf)
    float contrast = 1.0f; ///< 0 .. 2: how much of the contrast of the look is used
    float vignette = 1.0f; ///< 0 .. 2: how much of the dark corner of the look is used
    float wash = 0.9f;     ///< the painted blotches of the stone (value and hue; tall stains on a wall)
    float damp = 0.5f;     ///< the stone near the water darker and cooler (B has twice as much as A)
    float haze = 0.6f;     ///< the far haze leans toward the colour of StyleLookOf: the distance has its own hue
};

/// The colours of the grade and of the cool fill of one moment. All are multipliers (what the light is multiplied by)
/// that keep the brightness about the same: it is the hue that moves.
struct StyleLook
{
    float cool[3];     ///< the sky light of a face the sun does not reach
    float low[3];      ///< the darks of the picture
    float high[3];     ///< the lights of the picture
    float lowAmount;   ///< how far the darks go toward `low` (at full style)
    float highAmount;  ///< how far the lights go toward `high`
    float hazeTint[3]; ///< what the haze is multiplied by far away
    float dampScale;   ///< the damp of the stone near the water, as a part of `damp`
    float saturation;  ///< 1 = as it is
    float contrast;    ///< 1 = as it is (above 1: deeper darks and lighter lights)
    float vignette;    ///< 0 .. 1: how much darker the corners are
};

/// A: the warm evening: shadows blue-violet, the lights gold. B: the cold present: shadows indigo, the lights pale
/// cyan.
inline StyleLook StyleLookOf(int layer)
{
    if (layer == 0)
    {
        return StyleLook{{0.80f, 0.92f, 1.28f},
                         {0.86f, 0.93f, 1.16f},
                         {1.06f, 1.00f, 0.90f},
                         0.75f,
                         0.45f,
                         {0.84f, 0.90f, 1.22f},
                         0.5f,
                         1.12f,
                         1.08f,
                         0.20f};
    }
    return StyleLook{{0.96f, 0.92f, 1.22f},
                     {0.98f, 0.92f, 1.18f},
                     {0.94f, 1.01f, 1.10f},
                     0.75f,
                     0.45f,
                     {0.80f, 1.00f, 1.12f},
                     1.0f,
                     1.25f,
                     1.22f,
                     0.32f};
}

/// What the model shader reads for the style (cbuffer Frame, from gradeParams2 on: shaders/models.hlsl). Four floats
/// each.
struct StyleConstants
{
    float grade2[4];   ///< x: the sun in bands, y: the rim, z: saturation, w: vignette
    float cool[4];     ///< rgb: the multiplier of the sky light of an unlit face, a: how far
    float low[4];      ///< rgb: the grade of the darks, a: how much
    float high[4];     ///< rgb: the grade of the lights, a: how much
    float light[4];    ///< x: the clouds, y: the rings of a lamp, z: the light shafts, w: their samples
    float hazeTint[4]; ///< rgb: what the haze is multiplied by far away, a: how far
    float misc[4];     ///< x: the painted blotches, y: the damp near the water, z: the contrast
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
    s.style = LightingClamp(in.style, 0.0f, 1.0f, d.style);
    s.toon = LightingClamp(in.toon, 0.0f, 1.0f, d.toon);
    s.cool = LightingClamp(in.cool, 0.0f, 1.0f, d.cool);
    s.rim = LightingClamp(in.rim, 0.0f, 2.0f, d.rim);
    s.cloudsA = LightingClamp(in.cloudsA, 0.0f, 0.9f, d.cloudsA);
    s.cloudsB = LightingClamp(in.cloudsB, 0.0f, 0.9f, d.cloudsB);
    s.lampRings = LightingClamp(in.lampRings, 0.0f, 1.0f, d.lampRings);
    s.shaftsA = LightingClamp(in.shaftsA, 0.0f, 3.0f, d.shaftsA);
    s.shaftsB = LightingClamp(in.shaftsB, 0.0f, 3.0f, d.shaftsB);
    s.shaftSteps = std::clamp(in.shaftSteps, 4, 32);
    s.grade = LightingClamp(in.grade, 0.0f, 1.0f, d.grade);
    s.saturation = LightingClamp(in.saturation, 0.0f, 2.0f, d.saturation);
    s.contrast = LightingClamp(in.contrast, 0.0f, 2.0f, d.contrast);
    s.vignette = LightingClamp(in.vignette, 0.0f, 2.0f, d.vignette);
    s.wash = LightingClamp(in.wash, 0.0f, 1.0f, d.wash);
    s.damp = LightingClamp(in.damp, 0.0f, 1.0f, d.damp);
    s.haze = LightingClamp(in.haze, 0.0f, 1.0f, d.haze);
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
    s.style = c.GetFloat("lighting", "style", d.style);
    s.toon = c.GetFloat("lighting", "toon", d.toon);
    s.cool = c.GetFloat("lighting", "cool", d.cool);
    s.rim = c.GetFloat("lighting", "rim", d.rim);
    s.cloudsA = c.GetFloat("lighting", "clouds_a", d.cloudsA);
    s.cloudsB = c.GetFloat("lighting", "clouds_b", d.cloudsB);
    s.lampRings = c.GetFloat("lighting", "lamp_rings", d.lampRings);
    s.shafts = c.GetBool("lighting", "shafts", d.shafts);
    s.shaftsA = c.GetFloat("lighting", "shafts_a", d.shaftsA);
    s.shaftsB = c.GetFloat("lighting", "shafts_b", d.shaftsB);
    s.shaftSteps = c.GetInt("lighting", "shaft_steps", d.shaftSteps);
    s.grade = c.GetFloat("lighting", "grade", d.grade);
    s.saturation = c.GetFloat("lighting", "saturation", d.saturation);
    s.contrast = c.GetFloat("lighting", "contrast", d.contrast);
    s.vignette = c.GetFloat("lighting", "vignette", d.vignette);
    s.wash = c.GetFloat("lighting", "wash", d.wash);
    s.damp = c.GetFloat("lighting", "damp", d.damp);
    s.haze = c.GetFloat("lighting", "haze", d.haze);
    return Sanitize(s);
}

/// The style of moment `layer` (0 = A, 1 = B) as the registers of the model shader. `plain` (the debug view "plain" of
/// the key L) and a master `style` of 0 give all zeros and a saturation of 1: the plain light. The shafts need the
/// shadows (`shadows` false: none).
inline StyleConstants MakeStyle(const LightingSettings& in, int layer, bool plain)
{
    const LightingSettings s = Sanitize(in);
    const float k = plain ? 0.0f : s.style;
    const StyleLook look = StyleLookOf(layer == 0 ? 0 : 1);
    StyleConstants c{};
    c.grade2[0] = s.toon * k;
    c.grade2[1] = s.rim * k;
    c.grade2[2] = 1.0f + (look.saturation - 1.0f) * s.saturation * k;
    c.grade2[3] = look.vignette * s.vignette * k;
    for (int i = 0; i < 3; ++i)
    {
        c.cool[i] = look.cool[i];
        c.low[i] = look.low[i];
        c.high[i] = look.high[i];
    }
    c.cool[3] = s.cool * k;
    c.low[3] = look.lowAmount * s.grade * k;
    c.high[3] = look.highAmount * s.grade * k;
    c.light[0] = (layer == 0 ? s.cloudsA : s.cloudsB) * k;
    c.light[1] = s.lampRings * k;
    c.light[2] = (s.shafts && s.shadows) ? (layer == 0 ? s.shaftsA : s.shaftsB) * k : 0.0f;
    c.light[3] = static_cast<float>(s.shaftSteps);
    for (int i = 0; i < 3; ++i)
    {
        c.hazeTint[i] = look.hazeTint[i];
    }
    c.hazeTint[3] = s.haze * k;
    c.misc[0] = s.wash * k;
    c.misc[1] = s.damp * look.dampScale * k;
    c.misc[2] = 1.0f + (look.contrast - 1.0f) * s.contrast * k;
    return c;
}

} // namespace dl
