// LineSettings.h - every number of the ink lines (docs/DESIGN.md 6.10, S2): what the config file [lines] says, and the
// registers that the line pass reads (shaders/lines.hlsli).
// Why:       the lines are found in the picture from its depth (shaders/lines.hlsli); these are the knobs of them. F5
//            reloads them.
// Rules:     a value that makes no sense is pulled into a range that works (Sanitize); a missing key keeps the default.
//            The master of the style (`style` of [lighting]) is also the master of the lines: the key L ends its round
//            with "plain" and the plain view has no line.
// UNITS:     pixels at a screen 1080 pixels high (the pass scales them with the height of the picture); metres for the
//            distances; degrees for the turn of a surface. The registers carry pixels of the real picture and the turn
//            as 1 - cos.
// Plain data, no Windows, no Direct3D: tested without a window (tests/test_style.cpp, TestLines).
#pragma once

#include "core/Config.h"

#include <algorithm>
#include <cmath>

namespace dl
{

struct LineSettings
{
    float amount = 1.0f;      ///< 0 .. 1: how much of the line is laid over the picture (0: no line)
    float width = 2.0f;       ///< 0.5 .. 6: pixels (at 1080 high): the outline
    float creaseWidth = 1.2f; ///< 0.5 .. 6: pixels (at 1080 high): the inner edge
    float silhouette = 6.0f;  ///< 2 .. 40: how far a pixel must stand in front of its neighbours to be on an outline
    float crease = 30.0f;     ///< 10 .. 80: degrees: an inner edge starts here and is full 15 more (a 50 turn draws)
    float fadeNear = 40.0f;   ///< UNITS: metres: the inner edges start to fade here
    float fadeFar = 80.0f;    ///< UNITS: metres: and are gone here; the outlines thin from here to five times it
    float farAlpha = 0.5f;    ///< 0 .. 1: the strength of an outline at five times fadeFar
    float local = 0.35f;      ///< 0 .. 1: how much of the picture's own colour is in the line (0: the moment's)
};

inline float LineClamp(float v, float lo, float hi, float fallback)
{
    return std::isfinite(v) ? std::clamp(v, lo, hi) : fallback;
}

inline LineSettings Sanitize(const LineSettings& in)
{
    const LineSettings d{};
    LineSettings s;
    s.amount = LineClamp(in.amount, 0.0f, 1.0f, d.amount);
    s.width = LineClamp(in.width, 0.5f, 6.0f, d.width);
    s.creaseWidth = LineClamp(in.creaseWidth, 0.5f, 6.0f, d.creaseWidth);
    s.silhouette = LineClamp(in.silhouette, 2.0f, 40.0f, d.silhouette);
    s.crease = LineClamp(in.crease, 10.0f, 80.0f, d.crease);
    s.fadeNear = LineClamp(in.fadeNear, 5.0f, 400.0f, d.fadeNear);
    s.fadeFar = LineClamp(in.fadeFar, 10.0f, 600.0f, d.fadeFar);
    s.fadeFar = std::max(s.fadeFar, s.fadeNear + 5.0f); // the fade has a length
    s.farAlpha = LineClamp(in.farAlpha, 0.0f, 1.0f, d.farAlpha);
    s.local = LineClamp(in.local, 0.0f, 1.0f, d.local);
    return s;
}

inline LineSettings ReadLineSettings(const Config& c)
{
    const LineSettings d{};
    LineSettings s;
    s.amount = c.GetFloat("lines", "amount", d.amount);
    s.width = c.GetFloat("lines", "width", d.width);
    s.creaseWidth = c.GetFloat("lines", "crease_width", d.creaseWidth);
    s.silhouette = c.GetFloat("lines", "silhouette", d.silhouette);
    s.crease = c.GetFloat("lines", "crease", d.crease);
    s.fadeNear = c.GetFloat("lines", "fade_near", d.fadeNear);
    s.fadeFar = c.GetFloat("lines", "fade_far", d.fadeFar);
    s.farAlpha = c.GetFloat("lines", "far_alpha", d.farAlpha);
    s.local = c.GetFloat("lines", "local", d.local);
    return Sanitize(s);
}

/// What the line pass reads (the shader's constant buffer, four registers of four floats each and a colour).
struct LineConstants
{
    float depth[4];  ///< x: near plane, y: far plane (metres), z: PixelSlope, w: jump (relative inverse depth)
    float levels[4]; ///< x: silhouette level, y, z: crease start and full (1 - cos of the turn), w: master amount
    float fade[4];   ///< x: crease fade near, y: crease fade far (metres), z: far alpha, w: local mix
    float width[4];  ///< x: outline width, y: inner edge width (pixels of the real picture), z, w: reserved
    float ink[4];    ///< x, y, z: the colour of the line (linear), w: reserved
};

/// The colour of the line of a moment as the design sets it (6.10), sRGB: A a warm brown-black, B an indigo-black.
inline void LineColorOf(int layer, float srgb[3])
{
    srgb[0] = layer == 0 ? 0x2D / 255.0f : 0x0F / 255.0f;
    srgb[1] = layer == 0 ? 0x22 / 255.0f : 0x0F / 255.0f;
    srgb[2] = layer == 0 ? 0x1B / 255.0f : 0x14 / 255.0f;
}

inline float LineLinear(float s)
{
    return s <= 0.04045f ? s / 12.92f : std::pow((s + 0.055f) / 1.055f, 2.4f);
}

/// The registers for a picture of `heightPixels`, drawn with a projection whose element [1][1] is `projection11`, of
/// the moment `layer` (0 = A, 1 = B). `master` is the style master of [lighting] (0: the plain view, no line).
inline LineConstants MakeLines(const LineSettings& in, int layer, float master, float projection11, float heightPixels,
                               float nearZ, float farZ)
{
    const LineSettings s = Sanitize(in);
    const float px = std::max(1.0f, heightPixels / 1080.0f);
    const float pi = 3.14159265f;
    LineConstants c{};
    c.depth[0] = nearZ;
    c.depth[1] = farZ;
    c.depth[2] = 2.0f / (projection11 * std::max(heightPixels, 1.0f));
    c.depth[3] = 0.08f;
    c.levels[0] = s.silhouette;
    c.levels[1] = 1.0f - std::cos(s.crease * pi / 180.0f);
    c.levels[2] = 1.0f - std::cos((s.crease + 15.0f) * pi / 180.0f);
    c.levels[3] = LineClamp(master, 0.0f, 1.0f, 1.0f) * s.amount;
    c.fade[0] = s.fadeNear;
    c.fade[1] = s.fadeFar;
    c.fade[2] = s.farAlpha;
    c.fade[3] = s.local;
    c.width[0] = s.width * px;
    c.width[1] = s.creaseWidth * px;
    float rgb[3];
    LineColorOf(layer == 0 ? 0 : 1, rgb);
    c.ink[0] = LineLinear(rgb[0]);
    c.ink[1] = LineLinear(rgb[1]);
    c.ink[2] = LineLinear(rgb[2]);
    return c;
}

} // namespace dl
