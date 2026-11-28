// test_style.cpp - tests of the style of the light (no window, no GPU): the functions of shaders/lighting.hlsli under
// "the style of the light" (through a stand-in for the vector types of HLSL: tests/hlsl_shim.h) and the numbers that
// feed them (src/render/LightingSettings.h: StyleLookOf, MakeStyle, the keys of [lighting]).
//   g++ -std=c++20 -O2 -Wall -Wextra -I src tests/test_style.cpp src/core/Config.cpp -o test_style
//   MSVC (Developer PowerShell):  .\tests\run_tests.ps1 -Only style
// What is checked:
//   1. the lamp: the rings (0 at 0, only rising, flat in the middle of a step, the amount 0 leaves the smooth light),
//      the warmth (the core whiter than the edge), and that a style of 0 is exactly the smooth lamp of LampShade
//   2. the sun: the hard steps (three tones, the ends, only rising, flat inside a tone, the width of an edge), the
//      clouds (the range, the amount, the
//      drift, the share of the sky they cover, the same value along a ray of light), the rim (none on a floor, none
//      seen squarely, more at the edge)
//   3. a surface: the plain sum when the style is 0; the cool fill on the shadow side only; the rim of the stone is a
//      third of the rim of the rest; the painted stone (identity at 0, a bounded change, damp darker and cooler); the
//      haze (nothing near)
//   4. the grade: identity when neutral, the contrast keeps the middle grey, the corners, no negative colour, the
//      saturation
//   5. the settings: the defaults, nonsense pulled in, the keys of the config, MakeStyle (plain = zeros, the master,
//      the shafts need the shadows, A and B differ), and the looks (cool and tones keep the brightness)
//   7. the ink lines of docs/DESIGN.md 6.10 (S2): shaders/lines.hlsli on a depth buffer that the test makes (a house, a
//      crate, a pole, a ramp): no line on a plane or inside a face, the outline on the near side only, an inner edge
//      for a turn of 50 degrees and not for 20, the fade, the width, the colour; and the settings, the registers and
//      the shipped file
//   6. the look of docs/DESIGN.md 6.10, with the settings that config/default.ini really has: A in three tones (sunny
//      wall, ground, shadow) with a cool shadow; B with its open ground not pale, its route readable in the moon alone
//      and its deepest dark not below L* 12; the distance of A lavender and pale (the haze and the far fog), that of B
//      as it was; the horizon of the sky of A and of B is the colour of their far land (no band of the old horizon)
//   8. the sky: its horizon leans to the hue that the far land fades to (LeanToHue, StyleSky), none overhead, none in
//      the plain view
// Returns 1 when a check fails.

#include "check.h"
#include "render/LightingSettings.h"
#include "render/LineSettings.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

// the shader maths, as C++ (the order matters: see tests/hlsl_shim.h)
#include "hlsl_shim.h"

#include "../shaders/lighting.hlsli"
#include "../shaders/lines.hlsli"
#include "../shaders/lines_blend.hlsli"

#include "hlsl_shim_end.h"

namespace
{

bool Near(float a, float b, float eps)
{
    return std::fabs(a - b) <= eps;
}

bool Near3(const float3& a, const float3& b, float eps)
{
    return Near(a.x, b.x, eps) && Near(a.y, b.y, eps) && Near(a.z, b.z, eps);
}

// a style that changes nothing
StyleParams Neutral()
{
    StyleParams s{};
    s.saturation = 1.0f;
    s.contrast = 1.0f;
    s.cool = float3(1.0f, 1.0f, 1.0f);
    s.low = float3(1.0f, 1.0f, 1.0f);
    s.high = float3(1.0f, 1.0f, 1.0f);
    s.farTint = float3(1.0f, 1.0f, 1.0f);
    s.shaftSteps = 8.0f;
    return s;
}

// the style of moment `layer` (0 = A, 1 = B) with the settings `set`, as the shader reads it for a flat face
StyleParams StyleOf(const dl::LightingSettings& set, int layer)
{
    const dl::StyleConstants c = dl::MakeStyle(set, layer, false);
    StyleParams s{};
    s.toon = c.grade2[0];
    s.rim = c.grade2[1];
    s.saturation = c.grade2[2];
    s.vignette = c.grade2[3];
    s.contrast = c.misc[2];
    s.cool = float3(c.cool[0], c.cool[1], c.cool[2]);
    s.coolAmount = c.cool[3];
    s.low = float3(c.low[0], c.low[1], c.low[2]);
    s.lowAmount = c.low[3];
    s.high = float3(c.high[0], c.high[1], c.high[2]);
    s.highAmount = c.high[3];
    s.cloud = c.light[0];
    s.lampRings = c.light[1];
    s.shaft = c.light[2];
    s.shaftSteps = c.light[3];
    s.wash = c.misc[0];
    s.damp = c.misc[1];
    s.farTint = float3(c.hazeTint[0], c.hazeTint[1], c.hazeTint[2]);
    s.farAmount = c.hazeTint[3];
    s.farFog = c.more[0];
    s.toonEdge = 0.0f; // a flat face has no change of the cosine over a pixel: its steps are hard
    return s;
}

// a style with every part on (the looks of A at full strength)
StyleParams Full()
{
    return StyleOf(dl::LightingSettings{}, 0);
}

// ---- 1. the lamp

void TestLamp()
{
    CHECK(LampRings(0.0f, 1.0f) == 0.0f && LampRings(0.0f, 0.0f) == 0.0f);
    bool smoothKept = true;
    bool rising = true;
    bool bounded = true;
    float last = -1.0f;
    for (int i = 0; i <= 2000; ++i)
    {
        const float att = 0.7f * static_cast<float>(i) / 2000.0f;
        smoothKept = smoothKept && LampRings(att, 0.0f) == att; // an amount of 0 is the smooth light, exactly
        const float r = LampRings(att, 1.0f);
        rising = rising && r >= last - 1.0e-6f;
        bounded = bounded && r >= 0.0f && r <= 0.667f + 1.0e-4f;
        last = r;
    }
    CHECK(smoothKept && rising && bounded);
    // flat in the middle of a step: between two samples a hair apart the rings change less than the smooth light, and
    // between steps more (find the largest and the smallest slope over the range)
    float lo = 1.0e9f;
    float hi = 0.0f;
    for (int i = 40; i < 600; ++i)
    {
        const float a0 = 0.001f * static_cast<float>(i);
        const float slope = (LampRings(a0 + 0.0005f, 1.0f) - LampRings(a0, 1.0f)) / 0.0005f;
        lo = std::min(lo, slope);
        hi = std::max(hi, slope);
    }
    CHECK(lo < 0.15f * hi); // the plateaus of the rings and their risers are plain to see
    CHECK(Near(LampRings(0.2f, 0.5f), 0.5f * (0.2f + LampRings(0.2f, 1.0f)), 1.0e-6f)); // the amount is a mix

    const float3 amber(1.0f, 0.7f, 0.4f);
    CHECK(Near3(LampWarmth(amber, 0.1f, 0.0f), amber, 0.0f)); // an amount of 0: the colour as it is
    const float3 core = LampWarmth(amber, 0.5f, 1.0f);
    const float3 edge = LampWarmth(amber, 0.01f, 1.0f);
    CHECK(core.y / core.x > edge.y / edge.x && core.z / core.x > edge.z / edge.x); // the core is whiter than the edge
    CHECK(edge.x > 0.0f && edge.y > 0.0f && edge.z > 0.0f && core.x <= amber.x + 1.0e-6f);

    // a style of 0 is the smooth lamp
    const float3 p(0, 0, 0);
    const float3 n(0, 1, 0);
    const float3 v(0, 1, 0);
    const float3 lp(1.0f, 3.0f, 0.5f);
    const LampLight a = LampShade(p, n, v, lp, 14.0f, amber * 18.0f, 0.5f, 16.0f);
    const LampLight b = LampShadeStyled(p, n, v, lp, 14.0f, amber * 18.0f, 0.5f, 16.0f, 0.0f);
    CHECK(a.diffuse.x == b.diffuse.x && a.diffuse.y == b.diffuse.y && a.specular.z == b.specular.z);
    const LampLight c = LampShadeStyled(p, n, v, lp, 14.0f, amber * 18.0f, 0.5f, 16.0f, 1.0f);
    CHECK(c.diffuse.x > 0.0f && c.diffuse.x < 2.0f * a.diffuse.x && c.diffuse.y < c.diffuse.x);
    const LampLight beyond = LampShadeStyled(p, n, v, float3(0, 20.0f, 0), 14.0f, amber * 18.0f, 0.5f, 16.0f, 1.0f);
    CHECK(beyond.diffuse.x == 0.0f && beyond.specular.x == 0.0f); // still no light beyond the range
}

// ---- 2. the sun

void TestSun()
{
    CHECK(ToonRamp(0.0f, 1.0f) == 0.0f && Near(ToonRamp(1.0f, 1.0f), 1.0f, 1.0e-6f));
    bool plain = true;
    bool rising = true;
    float last = -1.0f;
    for (int i = 0; i <= 1000; ++i)
    {
        const float x = static_cast<float>(i) / 1000.0f;
        plain = plain && ToonRamp(x, 0.0f) == x;
        const float r = ToonRamp(x, 1.0f);
        rising = rising && r >= last - 1.0e-6f && r <= 1.0f + 1.0e-6f;
        last = r;
    }
    CHECK(plain && rising);
    // three tones with hard edges (6.10): none of the sun below a cosine of 0.14, 0.46 of it from there, all of it
    // from 0.52
    bool none = true;
    bool part = true;
    bool all = true;
    for (int i = 0; i <= 100; ++i)
    {
        const float x = static_cast<float>(i) / 100.0f;
        none = none && (x > 0.1399f || ToonRamp(x, 1.0f) == 0.0f);
        part = part && (x < 0.1401f || x > 0.5199f || Near(ToonRamp(x, 1.0f), 0.46f, 1.0e-6f));
        all = all && (x < 0.5201f || Near(ToonRamp(x, 1.0f), 1.0f, 1.0e-6f));
    }
    CHECK(none && part && all);
    CHECK(ToonRamp(0.1399f, 1.0f) == 0.0f && Near(ToonRamp(0.1401f, 1.0f), 0.46f, 1.0e-6f)); // the edge is a step
    CHECK(ToonRamp(0.55f, 1.0f) > ToonRamp(0.25f, 1.0f) + 0.4f && ToonRamp(0.25f, 1.0f) > ToonRamp(0.05f, 1.0f) + 0.4f);
    // the width of an edge: `edge` wide, centred on the step, half way at its middle, only rising
    CHECK(ToonRampEdge(0.09f, 1.0f, 0.1f) == 0.0f && Near(ToonRampEdge(0.19f, 1.0f, 0.1f), 0.46f, 1.0e-6f));
    CHECK(Near(ToonRampEdge(0.14f, 1.0f, 0.1f), 0.23f, 1.0e-5f) &&
          Near(ToonRampEdge(0.52f, 1.0f, 0.1f), 0.73f, 1.0e-5f));
    bool softRising = true;
    float prev = -1.0f;
    for (int i = 0; i <= 1000; ++i)
    {
        const float r = ToonRampEdge(static_cast<float>(i) / 1000.0f, 1.0f, 0.1f);
        softRising = softRising && r >= prev - 1.0e-6f && r <= 1.0f + 1.0e-6f;
        prev = r;
    }
    CHECK(softRising);
    CHECK(ToonRampEdge(0.3f, 0.0f, 0.1f) == 0.3f); // an amount of 0 leaves the cosine as it is, whatever the edge
    CHECK(Near(ToonRampEdge(0.3f, 0.5f, 0.0f), 0.5f * (0.3f + 0.46f), 1.0e-6f)); // the amount is a mix
    // an edge of 0 is hard
    CHECK(ToonRampEdge(0.1399f, 1.0f, 0.0f) == 0.0f && Near(ToonRampEdge(0.1401f, 1.0f, 0.0f), 0.46f, 1.0e-6f));

    // the clouds
    const float3 up(-0.3f, 0.8f, -0.5f); // the moon of B: steep, the light never below 0.3 of vertical
    CHECK(CloudShade(float3(3, 4, 5), up, 7.0f, 0.0f) == 1.0f); // an amount of 0: no cloud
    int covered = 0;
    int total = 0;
    bool inRange = true;
    for (int i = 0; i < 80; ++i)
    {
        for (int j = 0; j < 80; ++j)
        {
            const float s =
                CloudShade(float3(5.0f * static_cast<float>(i), 0.0f, 5.0f * static_cast<float>(j)), up, 3.0f, 0.4f);
            inRange = inRange && s >= 0.6f - 1.0e-5f && s <= 1.0f;
            covered += s < 0.8f ? 1 : 0;
            ++total;
        }
    }
    const float share = static_cast<float>(covered) / static_cast<float>(total);
    CHECK(inRange && share > 0.15f && share < 0.65f); // a fair share of the ground is under a cloud, never all or none
    // a cloud has a soft but short edge: two points 10 cm apart never differ by much
    bool soft = true;
    for (int i = 0; i < 4000; ++i)
    {
        const float x = 0.37f * static_cast<float>(i);
        soft = soft && Near(CloudShade(float3(x, 0, 11.0f), up, 3.0f, 0.5f),
                            CloudShade(float3(x + 0.1f, 0, 11.0f), up, 3.0f, 0.5f), 0.12f);
    }
    CHECK(soft);
    // they drift: after a minute a point is not under the same cloud everywhere
    int changed = 0;
    for (int i = 0; i < 200; ++i)
    {
        const float3 p(7.0f * static_cast<float>(i), 0.0f, 13.0f);
        changed += std::fabs(CloudShade(p, up, 0.0f, 0.5f) - CloudShade(p, up, 60.0f, 0.5f)) > 0.1f ? 1 : 0;
    }
    CHECK(changed > 10);
    // the shadow lies along the light: two points on the same ray to the sun are under the same cloud
    bool along = true;
    for (int i = 0; i < 100; ++i)
    {
        const float3 p(9.0f * static_cast<float>(i), 2.0f * static_cast<float>(i % 7),
                       4.0f * static_cast<float>(i % 11));
        const float3 q = p + up * 5.0f;
        along = along && Near(CloudShade(p, up, 5.0f, 0.5f), CloudShade(q, up, 5.0f, 0.5f), 1.0e-3f);
    }
    CHECK(along);
    // a low sun does not stretch the clouds without end: the light is taken as no lower than 0.3 of vertical
    const float3 low(-0.9f, 0.05f, 0.0f);
    const float s1 = CloudShade(float3(100.0f, 30.0f, 0.0f), low, 1.0f, 0.5f);
    CHECK(s1 >= 0.5f - 1.0e-5f && s1 <= 1.0f);

    // the rim
    CHECK(RimTerm(1.0f, 1.0f, 0.0f) == 0.0f);                                       // seen squarely: none
    CHECK(RimTerm(0.0f, 1.0f, 1.0f) == 0.0f && RimTerm(0.0f, 1.0f, -1.0f) == 0.0f); // a floor or a ceiling: none
    CHECK(RimTerm(0.1f, 1.0f, 0.0f) > RimTerm(0.5f, 1.0f, 0.0f) &&
          RimTerm(0.5f, 1.0f, 0.0f) > RimTerm(0.9f, 1.0f, 0.0f));
    CHECK(RimTerm(0.1f, 1.0f, 0.0f) > RimTerm(0.1f, 0.0f, 0.0f) &&
          RimTerm(0.1f, 0.0f, 0.0f) > 0.0f); // more toward the light, some away
    CHECK(RimTerm(0.0f, 1.0f, 0.0f) <= 1.0f);
}

// ---- 3. a surface

void TestSurface()
{
    const float3 albedo(0.6f, 0.4f, 0.3f);
    const float3 sky(0.2f, 0.18f, 0.3f);
    const float3 sunColour(1.0f, 0.8f, 0.5f);
    const float3 none(0, 0, 0);
    const float3 n(0, 1, 0);
    const float3 v = normalize(float3(0.3f, 0.5f, 0.8f));
    const float3 toSun = normalize(float3(0.2f, 1.0f, 0.1f));
    // style 0: the sum of the plain light
    const float diffuse = saturate(dot(n, toSun));
    const float3 plainSum = albedo * sky + albedo * sunColour * diffuse * 0.8f + sunColour * (0.1f * 0.8f) +
                            albedo * 0.05f + albedo * float3(0.1f, 0.1f, 0.0f) + float3(0.0f, 0.0f, 0.02f);
    const float3 got = StyleShade(albedo, n, toSun, v, sky, sunColour, 0.8f, 0.1f, 0.05f, float3(0.1f, 0.1f, 0.0f),
                                  float3(0.0f, 0.0f, 0.02f), 1.0f, Neutral());
    CHECK(Near3(got, plainSum, 1.0e-5f));

    // the cool fill: only where the sun does not reach
    StyleParams cool = Neutral();
    cool.cool = float3(0.8f, 0.92f, 1.28f);
    cool.coolAmount = 1.0f;
    const float3 lit =
        StyleShade(albedo, n, n, v, sky, none, 1.0f, 0.0f, 0.0f, none, none, 1.0f, cool); // the sun squarely on it
    const float3 litPlain = StyleShade(albedo, n, n, v, sky, none, 1.0f, 0.0f, 0.0f, none, none, 1.0f, Neutral());
    CHECK(Near3(lit, litPlain, 1.0e-6f)); // a face in the sun is untouched
    const float3 shade = StyleShade(albedo, n, toSun, v, sky, none, 0.0f, 0.0f, 0.0f, none, none, 1.0f, cool);
    const float3 shadePlain = StyleShade(albedo, n, toSun, v, sky, none, 0.0f, 0.0f, 0.0f, none, none, 1.0f, Neutral());
    CHECK(shade.z / shade.x > 1.4f * shadePlain.z / shadePlain.x); // the shadow leans blue
    const float3 back =
        StyleShade(albedo, float3(0, -1, 0), toSun, v, sky, none, 1.0f, 0.0f, 0.0f, none, none, 1.0f, cool);
    CHECK(back.z / back.x >
          1.4f * (albedo.z * sky.z) / (albedo.x * sky.x)); // so does a face that looks away from the sun
    // the bands change what a tilted face gets of the sun, not a face squarely in it
    StyleParams bands = Neutral();
    bands.toon = 1.0f;
    const float3 up = StyleShade(albedo, float3(0, 1, 0), float3(0, 1, 0), float3(0, 1, 0), none, sunColour, 1.0f, 0.0f,
                                 0.0f, none, none, 1.0f, bands);
    CHECK(Near3(up, albedo * sunColour, 1.0e-5f));
    // a flat face is a flat fill (6.10): inside a tone its colour does not depend on the cosine to the sun
    const float tilts[] = {0.16f, 0.30f, 0.45f, 0.50f};
    bool flatFill = true;
    float3 firstFill;
    for (int i = 0; i < 4; ++i)
    {
        const float3 tilt(std::sqrt(1.0f - tilts[i] * tilts[i]), tilts[i], 0.0f);
        const float3 fill =
            StyleShade(albedo, tilt, float3(0, 1, 0), tilt, none, sunColour, 1.0f, 0.0f, 0.0f, none, none, 1.0f, bands);
        if (i == 0)
        {
            firstFill = fill;
        }
        flatFill = flatFill && Near3(fill, firstFill, 1.0e-6f);
    }
    CHECK(flatFill && Near3(firstFill, albedo * sunColour * 0.46f, 1.0e-5f)); // the middle tone: 0.46 of the sun
    // a soft edge (a round thing) is only between the tones: the same face with an edge of 0.1 differs only near a step
    StyleParams softEdge = bands;
    softEdge.toonEdge = 0.1f;
    const float3 farFromStep(std::sqrt(1.0f - 0.3f * 0.3f), 0.3f, 0.0f);
    const float3 softFill = StyleShade(albedo, farFromStep, float3(0, 1, 0), farFromStep, none, sunColour, 1.0f, 0.0f,
                                       0.0f, none, none, 1.0f, softEdge);
    CHECK(Near3(softFill, firstFill, 1.0e-6f));
    // the rim: it adds light, never takes it, and the stone has a third of it
    StyleParams rim = Neutral();
    rim.rim = 1.0f;
    const float3 side = normalize(float3(1.0f, 0.0f, 0.2f)); // a wall
    const float3 eye = normalize(float3(0.1f, 0.0f, 1.0f));  // seen almost edge-on
    const float3 noRim =
        StyleShade(albedo, side, side, eye, sky, sunColour, 1.0f, 0.0f, 0.0f, none, none, 1.0f, Neutral());
    const float3 thing = StyleShade(albedo, side, side, eye, sky, sunColour, 1.0f, 0.0f, 0.0f, none, none, 1.0f, rim);
    const float3 stone = StyleShade(albedo, side, side, eye, sky, sunColour, 1.0f, 0.0f, 0.0f, none, none, 0.35f, rim);
    CHECK(thing.x > noRim.x && thing.y > noRim.y && thing.z > noRim.z);
    CHECK(Near((stone.x - noRim.x) / (thing.x - noRim.x), 0.35f, 1.0e-4f));

    // the painted stone
    const float3 wallN(1, 0, 0);
    CHECK(Near3(PaintAlbedo(albedo, float3(3, 4, 5), wallN, 0.7f, Neutral()), albedo,
                1.0e-6f)); // wash 0, damp 0: as it was
    StyleParams paint = Neutral();
    paint.wash = 1.0f;
    float minV = 10.0f;
    float maxV = 0.0f;
    bool deterministic = true;
    for (int i = 0; i < 3000; ++i)
    {
        const float3 p(1.7f * static_cast<float>(i), 0.9f * static_cast<float>(i % 40),
                       2.3f * static_cast<float>(i % 97));
        const float3 a = PaintAlbedo(float3(0.5f, 0.5f, 0.5f), p, (i % 2) != 0 ? float3(0, 1, 0) : wallN, 0.0f, paint);
        const float3 b = PaintAlbedo(float3(0.5f, 0.5f, 0.5f), p, (i % 2) != 0 ? float3(0, 1, 0) : wallN, 0.0f, paint);
        deterministic = deterministic && a.x == b.x && a.y == b.y && a.z == b.z;
        minV = std::min(minV, a.y);
        maxV = std::max(maxV, a.y);
    }
    CHECK(deterministic && minV >= 0.5f * 0.80f - 1.0e-5f && maxV <= 0.5f * 1.14f + 1.0e-5f);
    CHECK(maxV - minV > 0.04f); // the blotches can be seen
    StyleParams damp = Neutral();
    damp.damp = 1.0f;
    const float3 wet = PaintAlbedo(albedo, float3(3, 4, 5), wallN, 1.0f, damp);
    CHECK(wet.x < albedo.x && wet.y < albedo.y && wet.z < albedo.z &&
          wet.z / wet.x > albedo.z / albedo.x);                                             // darker and cooler
    CHECK(Near3(PaintAlbedo(albedo, float3(3, 4, 5), wallN, 0.0f, damp), albedo, 1.0e-6f)); // dry: as it was

    // the haze
    StyleParams haze = Neutral();
    haze.farTint = float3(0.8f, 0.9f, 1.2f);
    haze.farAmount = 1.0f;
    const float3 base(0.8f, 0.7f, 0.6f);
    CHECK(Near3(StyleHaze(base, 10.0f, haze), base, 0.0f) && Near3(StyleHaze(base, 24.0f, haze), base, 1.0e-6f));
    // far away the haze has the hue of farTint and the brightness that it had
    const float3 grey(0.2126f, 0.7152f, 0.0722f);
    const float3 farAway = StyleHaze(base, 300.0f, haze);
    CHECK(Near(farAway.x / farAway.z, haze.farTint.x / haze.farTint.z, 1.0e-5f) &&
          Near(farAway.y / farAway.z, haze.farTint.y / haze.farTint.z, 1.0e-5f));
    CHECK(Near(dot(farAway, grey), dot(base, grey), 1.0e-5f));
    // on the way (to 200 m, where it has all leaned) it leans more and more toward blue; the brightness never changes
    float lastBlue = base.z / base.x;
    bool leaning = true;
    bool bright = true;
    for (float metres = 30.0f; metres <= 180.0f; metres += 30.0f)
    {
        const float3 h = StyleHaze(base, metres, haze);
        leaning = leaning && h.z / h.x > lastBlue;
        bright = bright && Near(dot(h, grey), dot(base, grey), 1.0e-5f);
        lastBlue = h.z / h.x;
    }
    CHECK(leaning && bright);
    CHECK(Near3(StyleHaze(base, 300.0f, Neutral()), base, 0.0f));
    // the far fog: none within 100 m, all of it from 280 m, rising in between; the fog of the model is kept
    StyleParams fog = Neutral();
    fog.farFog = 0.5f;
    CHECK(StyleFarFog(0.0f, fog) == 0.0f && StyleFarFog(100.0f, fog) == 0.0f);
    CHECK(Near(StyleFarFog(280.0f, fog), 0.5f, 1.0e-6f) && Near(StyleFarFog(900.0f, fog), 0.5f, 1.0e-6f));
    bool fogRising = true;
    float lastFog = -1.0f;
    for (int i = 0; i <= 300; ++i)
    {
        const float f = StyleFarFog(static_cast<float>(i), fog);
        fogRising = fogRising && f >= lastFog && f <= 0.5f + 1.0e-6f;
        lastFog = f;
    }
    CHECK(fogRising && StyleFarFog(300.0f, Neutral()) == 0.0f);
    CHECK(Near(StyleFog(0.3f, 50.0f, fog), 0.3f, 1.0e-6f));                // near: the fog of the model alone
    CHECK(Near(StyleFog(0.3f, 300.0f, fog), 1.0f - 0.7f * 0.5f, 1.0e-6f)); // far: both, one over the other
    CHECK(Near(StyleFog(0.0f, 300.0f, fog), 0.5f, 1.0e-6f) && StyleFog(1.0f, 300.0f, fog) == 1.0f);
    CHECK(Near(StyleFog(0.4f, 900.0f, Neutral()), 0.4f, 1.0e-6f)); // no far fog: the fog of the model
    // the sky (6.10, S3c): the lean of the far distance, strongest at the horizon, none overhead, none when the
    // style is off
    CHECK(Near3(LeanToHue(base, haze.farTint, 0.0f), base, 0.0f));
    const float3 leant = LeanToHue(base, haze.farTint, 1.0f);
    CHECK(Near(dot(leant, grey), dot(base, grey), 1.0e-5f) &&
          Near(leant.z / leant.x, haze.farTint.z / haze.farTint.x, 1.0e-5f));
    // the haze is the lean by the distance, whichever way it is written
    const float byDistance = smoothstep(24.0f, 200.0f, 120.0f) * haze.farAmount;
    CHECK(Near3(StyleHaze(base, 120.0f, haze), LeanToHue(base, haze.farTint, byDistance), 1.0e-6f));
    // the weight: all of it at the horizon and below, none from 0.55 up, falling in between and never out of 0 .. 1
    CHECK(SkyHorizonWeight(-1.0f) == 1.0f && SkyHorizonWeight(0.0f) == 1.0f && SkyHorizonWeight(0.08f) == 1.0f);
    CHECK(SkyHorizonWeight(0.55f) == 0.0f && SkyHorizonWeight(1.0f) == 0.0f);
    bool weightFalls = true;
    float lastWeight = 2.0f;
    for (int i = -20; i <= 100; ++i)
    {
        const float w = SkyHorizonWeight(0.01f * static_cast<float>(i));
        weightFalls = weightFalls && w <= lastWeight + 1.0e-6f && w >= 0.0f && w <= 1.0f;
        lastWeight = w;
    }
    CHECK(weightFalls && SkyHorizonWeight(0.3f) > 0.0f && SkyHorizonWeight(0.3f) < 1.0f);
    // the horizon of the sky is the haze of the far land (200 m and more): there is no band between them
    CHECK(Near3(StyleSky(base, 0.0f, haze), StyleHaze(base, 300.0f, haze), 1.0e-6f));
    CHECK(Near3(StyleSky(base, -0.5f, haze), StyleHaze(base, 300.0f, haze), 1.0e-6f)); // and below it
    CHECK(Near3(StyleSky(base, 0.9f, haze), base, 0.0f));                              // overhead: as it was
    // toward the horizon the sky leans more and more to blue, and keeps its brightness all the way
    float skyBlue = base.z / base.x;
    bool skyLeans = true;
    bool skyBright = true;
    for (float altitude = 0.60f; altitude >= -0.01f; altitude -= 0.06f)
    {
        const float3 s = StyleSky(base, altitude, haze);
        skyLeans = skyLeans && s.z / s.x >= skyBlue - 1.0e-6f;
        skyBright = skyBright && Near(dot(s, grey), dot(base, grey), 1.0e-5f);
        skyBlue = s.z / s.x;
    }
    CHECK(skyLeans && skyBright && skyBlue > 1.4f * (base.z / base.x));
    // the plain view (a farAmount of 0) and a neutral style: the sky is as it was
    CHECK(Near3(StyleSky(base, 0.0f, Neutral()), base, 0.0f));
    StyleParams plainSky = haze;
    plainSky.farAmount = 0.0f;
    CHECK(Near3(StyleSky(base, 0.0f, plainSky), base, 0.0f));
}

// ---- 4. the grade

void TestGrade()
{
    const StyleParams neutral = Neutral();
    const float3 samples[] = {float3(0.0f, 0.0f, 0.0f), float3(0.02f, 0.03f, 0.05f), float3(0.18f, 0.18f, 0.18f),
                              float3(0.9f, 0.5f, 0.1f), float3(2.5f, 2.0f, 1.0f)};
    bool identity = true;
    for (const float3& c : samples)
    {
        const float3 g = StyleGrade(c, neutral, 0.9f, -0.7f);
        identity = identity && Near3(g, c, 1.0e-4f * (1.0f + c.x));
    }
    CHECK(identity);

    StyleParams contrast = Neutral();
    contrast.contrast = 1.25f;
    const float3 mid = StyleGrade(float3(0.18f, 0.18f, 0.18f), contrast, 0.0f, 0.0f);
    CHECK(Near3(mid, float3(0.18f, 0.18f, 0.18f), 1.0e-5f)); // the middle grey stays
    CHECK(StyleGrade(float3(0.05f, 0.05f, 0.05f), contrast, 0, 0).x < 0.05f &&
          StyleGrade(float3(0.5f, 0.5f, 0.5f), contrast, 0, 0).x > 0.5f);
    CHECK(StyleContrast(float3(0, 0, 0), 1.25f).x == 0.0f &&
          StyleContrast(float3(-1.0f, 0.5f, 0.2f), 1.25f).x == 0.0f); // no negative light

    StyleParams vig = Neutral();
    vig.vignette = 0.3f;
    const float3 grey(0.4f, 0.4f, 0.4f);
    CHECK(Near3(StyleGrade(grey, vig, 0.0f, 0.0f), grey, 1.0e-6f)); // none at the centre
    const float3 corner = StyleGrade(grey, vig, 1.2f, 1.2f); // the corner of the screen and beyond: the full dark
    CHECK(Near(corner.x, 0.4f * 0.7f, 1.0e-4f));
    CHECK(StyleGrade(grey, vig, 0.9f, 0.0f).x > StyleGrade(grey, vig, 0.9f, 0.9f).x); // darker toward the corner

    StyleParams sat = Neutral();
    sat.saturation = 0.0f;
    const float3 flat = StyleGrade(float3(0.8f, 0.4f, 0.1f), sat, 0, 0);
    CHECK(Near(flat.x, flat.y, 1.0e-5f) && Near(flat.y, flat.z, 1.0e-5f));
    sat.saturation = 1.6f;
    bool nonNegative = true;
    for (int i = 0; i < 200; ++i)
    {
        const float3 c(0.9f * static_cast<float>(i % 5) / 4.0f, 0.5f * static_cast<float>(i % 3) / 2.0f,
                       0.02f * static_cast<float>(i % 7));
        const float3 g = StyleGrade(c, sat, 0.2f, 0.2f);
        nonNegative = nonNegative && g.x >= 0.0f && g.y >= 0.0f && g.z >= 0.0f;
    }
    CHECK(nonNegative);

    // the tones: the darks lean toward `low`, the lights toward `high`
    StyleParams tones = Neutral();
    tones.low = float3(0.8f, 0.9f, 1.3f);
    tones.lowAmount = 1.0f;
    tones.high = float3(1.2f, 1.0f, 0.7f);
    tones.highAmount = 1.0f;
    const float3 dark = StyleGrade(float3(0.03f, 0.03f, 0.03f), tones, 0, 0);
    const float3 bright = StyleGrade(float3(0.9f, 0.9f, 0.9f), tones, 0, 0);
    CHECK(dark.z > dark.x && bright.x > bright.z);
    const float3 middle = StyleGrade(float3(0.70f, 0.70f, 0.70f), tones, 0, 0);
    CHECK(Near(middle.x / middle.z, (1.0f + 0.5f * 0.2f) / (1.0f - 0.5f * 0.3f),
               0.001f)); // at 0.7 half the lights, none of the darks
    const float3 between = StyleGrade(float3(0.30f, 0.30f, 0.30f), tones, 0, 0);
    CHECK(Near(between.x, between.z, 1.0e-6f)); // at 0.30 the darks have ended and the lights not begun

    CHECK(Near(ShaftPhase(-1.0f), 0.04f, 1.0e-6f) && Near(ShaftPhase(0.0f), 0.04f, 1.0e-6f) &&
          Near(ShaftPhase(1.0f), 1.0f, 1.0e-6f));
    CHECK(ShaftPhase(0.9f) > 5.0f * ShaftPhase(0.3f)); // the shafts are a thing of the view toward the light
    CHECK(ShaftEdge(0.0f) == 0.0f && ShaftEdge(1.0f) == 0.0f &&
          ShaftEdge(0.5f) == 1.0f); // a shaft is seen at the edge of a shadow
    CHECK(Near(ShaftEdge(0.2f), ShaftEdge(0.8f), 1.0e-6f) && ShaftEdge(0.25f) > ShaftEdge(0.1f) &&
          ShaftEdge(-3.0f) == 0.0f && ShaftEdge(7.0f) == 0.0f);
    bool rising = true;
    for (int i = 0; i < 100; ++i)
    {
        rising = rising && ShaftPhase(0.01f * static_cast<float>(i + 1)) >= ShaftPhase(0.01f * static_cast<float>(i));
    }
    CHECK(rising);

    // the full style of A on a mid-tone: every number finite and the picture not blown out or black
    const StyleParams full = Full();
    const float3 shaded = StyleGrade(float3(0.4f, 0.3f, 0.2f), full, 0.3f, 0.2f);
    CHECK(std::isfinite(shaded.x) && shaded.x > 0.05f && shaded.x < 2.0f && shaded.z > 0.0f);
}

// ---- 5. the settings

float Mean(const float c[3])
{
    return (c[0] + c[1] + c[2]) / 3.0f;
}

void TestSettings()
{
    const dl::LightingSettings d;
    CHECK(d.style == 1.0f && d.shafts && d.shaftSteps >= 4 && d.shaftSteps <= 32 && d.toon > 0.0f && d.cool > 0.0f);
    CHECK(d.toon == 1.0f && d.toonEdge == 1.5f); // 6.10: hard steps, an edge of a pixel and a half
    CHECK(d.farFog == 1.0f && d.haze == 0.85f);  // S3: the far fog of the look in full, the haze leaning far
    // S3c: the dusk of A: lamps lit and a darker shade (ambient_a is held at 0.8 or more by TestLook)
    CHECK(d.lampsA == 0.9f && d.ambientA == 0.85f && d.ambientA >= 0.8f && d.lampsA < d.lampsB);

    dl::LightingSettings bad;
    bad.style = 5.0f;
    bad.toon = -1.0f;
    bad.rim = std::nanf("");
    bad.cloudsA = 9.0f;
    bad.shaftSteps = 1000;
    bad.saturation = -3.0f;
    bad.contrast = 99.0f;
    bad.vignette = 99.0f;
    bad.wash = 4.0f;
    bad.toonEdge = 99.0f;
    bad.farFog = 9.0f;
    const dl::LightingSettings fixed = dl::Sanitize(bad);
    CHECK(fixed.style == 1.0f && fixed.toon == 0.0f && fixed.rim == d.rim && fixed.cloudsA == 0.9f &&
          fixed.shaftSteps == 32);
    CHECK(fixed.saturation == 0.0f && fixed.contrast == 2.0f && fixed.vignette == 2.0f && fixed.wash == 1.0f);
    CHECK(fixed.toonEdge == 6.0f && fixed.farFog == 4.0f);
    bad.toonEdge = 0.0f;
    CHECK(dl::Sanitize(bad).toonEdge == 0.25f); // never 0: the shader reads 0 as 'not set'
    bad.toonEdge = std::nanf("");
    CHECK(dl::Sanitize(bad).toonEdge == d.toonEdge);
    bad.shaftSteps = -4;
    CHECK(dl::Sanitize(bad).shaftSteps == 4);

    dl::Config c;
    c.ParseString("[lighting]\nstyle = 0.5\ntoon = 0.2\nshafts = false\nshaft_steps = 8\nclouds_b = 0.1\nlamp_rings = "
                  "0\nwash = 0.3\n"
                  "haze = 0.9\ncontrast = 1.5\nrim = abc\ntoon_edge = 2.5\nfar_fog = 0.4\n");
    const dl::LightingSettings r = dl::ReadLightingSettings(c);
    CHECK(r.style == 0.5f && r.toon == 0.2f && !r.shafts && r.shaftSteps == 8 && r.cloudsB == 0.1f &&
          r.lampRings == 0.0f);
    CHECK(r.wash == 0.3f && r.haze == 0.9f && r.contrast == 1.5f && r.rim == d.rim && r.cloudsA == d.cloudsA);
    CHECK(r.toonEdge == 2.5f && r.farFog == 0.4f);
    dl::Config empty;
    const dl::LightingSettings none = dl::ReadLightingSettings(empty);
    CHECK(none.style == d.style && none.toon == d.toon && none.shafts == d.shafts && none.shaftsA == d.shaftsA);

    // MakeStyle
    const dl::StyleConstants plain = dl::MakeStyle(d, 0, true);
    CHECK(plain.grade2[0] == 0.0f && plain.grade2[1] == 0.0f && plain.grade2[2] == 1.0f && plain.grade2[3] == 0.0f);
    CHECK(plain.cool[3] == 0.0f && plain.low[3] == 0.0f && plain.high[3] == 0.0f && plain.light[0] == 0.0f &&
          plain.light[1] == 0.0f);
    CHECK(plain.light[2] == 0.0f && plain.hazeTint[3] == 0.0f && plain.misc[0] == 0.0f && plain.misc[1] == 0.0f &&
          plain.misc[2] == 1.0f);
    CHECK(plain.misc[3] == 0.0f && plain.more[0] == 0.0f);
    dl::LightingSettings off;
    off.style = 0.0f;
    const dl::StyleConstants master0 = dl::MakeStyle(off, 1, false);
    CHECK(master0.grade2[2] == 1.0f && master0.misc[2] == 1.0f && master0.light[2] == 0.0f && master0.cool[3] == 0.0f);
    CHECK(master0.misc[3] == 0.0f && master0.more[0] == 0.0f);
    const dl::StyleConstants a = dl::MakeStyle(d, 0, false);
    const dl::StyleConstants b = dl::MakeStyle(d, 1, false);
    CHECK(a.grade2[0] == d.toon && a.grade2[1] == d.rim && a.grade2[2] > 1.0f && a.grade2[3] > 0.0f &&
          a.misc[2] > 1.0f);
    CHECK(a.light[0] == d.cloudsA && b.light[0] == d.cloudsB && a.light[2] == d.shaftsA && b.light[2] == d.shaftsB);
    CHECK(b.misc[1] > a.misc[1] && b.grade2[3] > a.grade2[3] &&
          b.cool[2] != a.cool[2]); // B is damper, darker in the corners, another colour
    CHECK(a.light[3] == static_cast<float>(d.shaftSteps));
    // the distance of A is paler, that of B is as it was
    CHECK(Near(a.more[0], 0.5f * d.farFog, 1.0e-6f) && b.more[0] == 0.0f);
    dl::LightingSettings noShadows;
    noShadows.shadows = false;
    CHECK(dl::MakeStyle(noShadows, 0, false).light[2] == 0.0f); // no shadow map, no shafts
    dl::LightingSettings noShafts;
    noShafts.shafts = false;
    CHECK(dl::MakeStyle(noShafts, 1, false).light[2] == 0.0f && dl::MakeStyle(noShafts, 1, false).light[0] > 0.0f);
    dl::LightingSettings half;
    half.style = 0.5f;
    const dl::StyleConstants h = dl::MakeStyle(half, 0, false);
    // the edge is in pixels: the master does not scale it
    CHECK(a.misc[3] == d.toonEdge && b.misc[3] == d.toonEdge && h.misc[3] == d.toonEdge);
    CHECK(Near(h.more[0], 0.5f * a.more[0], 1.0e-6f)); // the master scales the far fog
    dl::LightingSettings strongFog;
    strongFog.farFog = 4.0f;
    CHECK(dl::MakeStyle(strongFog, 0, false).more[0] == 1.0f); // 0.5 * 4 is 2: never more than all of the light
    CHECK(Near(h.grade2[0], 0.5f * d.toon, 1.0e-6f) && Near(h.misc[2], 1.0f + 0.5f * (a.misc[2] - 1.0f), 1.0e-6f));
    dl::LightingSettings nonsense;
    nonsense.style = std::nanf("");
    nonsense.shaftSteps = 0;
    const dl::StyleConstants s = dl::MakeStyle(nonsense, 0, false);
    CHECK(std::isfinite(s.grade2[0]) && s.light[3] >= 4.0f && s.light[3] <= 32.0f);

    // the looks: the cool fill and the tones move the hue and keep the brightness about the same
    for (int layer = 0; layer < 2; ++layer)
    {
        const dl::StyleLook look = dl::StyleLookOf(layer);
        CHECK(Mean(look.cool) > 0.9f && Mean(look.cool) < 1.1f);
        CHECK(Mean(look.low) > 0.9f && Mean(look.low) < 1.1f && Mean(look.high) > 0.9f && Mean(look.high) < 1.1f);
        // a cool hue to lean to (only the hue counts)
        CHECK(look.hazeTint[2] > look.hazeTint[0] && look.hazeTint[0] > 0.0f);
        CHECK(look.farFog >= 0.0f && look.farFog <= 1.0f);
        CHECK(look.cool[2] > look.cool[0] && look.low[2] > look.low[0]); // the shadows lean blue in both
        CHECK(look.high[0] != look.high[2] && look.lowAmount > 0.0f && look.highAmount > 0.0f);
        CHECK(look.contrast >= 1.0f && look.contrast <= 1.4f && look.saturation >= 1.0f && look.saturation <= 1.4f);
        CHECK(look.vignette >= 0.0f && look.vignette <= 0.5f && look.dampScale > 0.0f);
    }
    CHECK(dl::StyleLookOf(0).high[0] > dl::StyleLookOf(0).high[2]);  // A: the lights are gold
    CHECK(dl::StyleLookOf(1).high[2] > dl::StyleLookOf(1).high[0]);  // B: the lights are pale cyan
    CHECK(dl::StyleLookOf(7).cool[0] == dl::StyleLookOf(1).cool[0]); // a layer that is not 0 is B
}

// ---- 6. the look of docs/DESIGN.md 6.10
// What a flat, unshadowed face looks like on the screen: the path of PSMain for one pixel without the shadow map, the
// clouds, the lamps, the fog and the painted blotches. The light of the moment is the LIGHT of the model
// (tools/layout/city_model.py: the direction to the light, its colour, the shade) and the albedos are the palette
// (tools/layout/town_style.py build_mat(), B_AGE 0.65); KEEP IN SYNC with both. The ground light under a face is taken
// as 0.55 of the shade (the game takes it from the theme): it only matters for the faces that look sideways.
// Returns the linear colour.
struct Moment
{
    float3 toLight;
    float3 lightColor;
    float3 shade;
};

float Linear(float s)
{
    return s <= 0.04045f ? s / 12.92f : std::pow((s + 0.055f) / 1.055f, 2.4f);
}

// a float for printf (which takes a double)
double Show(float v)
{
    return static_cast<double>(v);
}

float LStar(const float3& c)
{
    const float y = 0.2126f * c.x + 0.7152f * c.y + 0.0722f * c.z;
    const float f = y > 0.008856f ? std::cbrt(y) : 7.787f * y + 16.0f / 116.0f;
    return 116.0f * f - 16.0f;
}

// `sun`: what the shadow leaves of the sun (1 lit, 0.15 in a cast shadow, 0 for a face that no sun can reach)
float3 Seen(const dl::LightingSettings& set, int layer, const Moment& m, const float3& albedoSrgb, const float3& n,
            float sun)
{
    const StyleParams style = StyleOf(set, layer);
    const float3 albedo(Linear(albedoSrgb.x), Linear(albedoSrgb.y), Linear(albedoSrgb.z));
    const float sunScale = layer == 0 ? set.sunA : set.sunB;
    const float ambient = layer == 0 ? set.ambientA : set.ambientB;
    const float3 skyDown = m.shade * 0.55f;
    const float3 skyLight = lerp(skyDown, m.shade, n.y * 0.5f + 0.5f) * ambient;
    const float3 l = normalize(m.toLight);
    const float3 none(0, 0, 0);
    const float3 c =
        StyleShade(albedo, n, l, n, skyLight, m.lightColor * sunScale, sun, 0.0f, 0.0f, none, none, 1.0f, style);
    return SoftShoulder(StyleGrade(c * set.exposure, style, 0.0f, 0.0f), set.knee);
}

// What a lit hillside looks like at `metres` in a moment: the fog of the model (density, colour of the horizon in
// sRGB), the far fog and the haze of the style, then the grade and the shoulder. The surface is given in linear light.
float3 SeenFar(const dl::LightingSettings& set, int layer, const float3& horizonSrgb, float density, float metres,
               const float3& surface)
{
    const StyleParams style = StyleOf(set, layer);
    const float3 horizon(Linear(horizonSrgb.x), Linear(horizonSrgb.y), Linear(horizonSrgb.z));
    const float3 haze = StyleHaze(horizon, metres, style);
    const float fog = StyleFog(1.0f - std::exp(-metres * density), metres, style);
    return SoftShoulder(StyleGrade(lerp(surface, haze, fog) * set.exposure, style, 0.0f, 0.0f), set.knee);
}

// how far a colour is from grey: (largest - smallest) / largest of the three
float Chroma(const float3& c)
{
    const float hi = std::max(c.x, std::max(c.y, c.z));
    const float lo = std::min(c.x, std::min(c.y, c.z));
    return hi > 0.0f ? (hi - lo) / hi : 0.0f;
}

// the config the game runs with: config/default.ini, read the way the game reads it
bool ShippedConfig(dl::Config& out)
{
    std::ifstream file("config/default.ini", std::ios::binary);
    if (!file)
    {
        return false;
    }
    std::stringstream text;
    text << file.rdbuf();
    out.ParseString(text.str());
    return true;
}

// The height of the sun of A over the horizon, as tools/layout/city_model.py has it (A_SUN_ELEVATION, degrees). The
// copy of the sun in TestLook says KEEP IN SYNC with the model; this is what keeps it so, and a test that cannot find
// the line fails and does not pass in silence.
bool ModelSunElevation(float& degrees)
{
    std::ifstream file("tools/layout/city_model.py", std::ios::binary);
    std::string line;
    while (std::getline(file, line))
    {
        if (line.rfind("A_SUN_ELEVATION", 0) != 0)
        {
            continue;
        }
        const size_t eq = line.find('=');
        if (eq == std::string::npos)
        {
            return false;
        }
        char* end = nullptr;
        const float v = std::strtof(line.c_str() + eq + 1, &end);
        if (end == line.c_str() + eq + 1)
        {
            return false;
        }
        degrees = v;
        return true;
    }
    return false;
}

bool ShippedSettings(dl::LightingSettings& out)
{
    dl::Config c;
    if (!ShippedConfig(c))
    {
        return false;
    }
    out = dl::ReadLightingSettings(c);
    return true;
}

void TestLook()
{
    dl::LightingSettings set;
    const bool read = ShippedSettings(set);
    CHECK(read); // run from the root of the repository (tests/run_tests.ps1 and tools/run_tests.py do)
    if (!read)
    {
        std::printf("config/default.ini was not found: run this test from the root of the repository\n");
        return;
    }
    CHECK(set.toon == 1.0f && set.style == 1.0f && set.toonEdge >= 0.25f && set.toonEdge <= 3.0f); // hard steps are on
    const Moment a{float3(-0.7471f, 0.2419f, -0.6191f), float3(1.06f, 0.86f, 0.64f), float3(0.36f, 0.27f, 0.52f)};
    const Moment b{float3(-0.30f, 0.80f, -0.52f), float3(0.92f, 0.98f, 1.10f), float3(0.30f, 0.33f, 0.43f)};
    // the copy of the sun of A above is the sun of the model: 14 degrees over the horizon (6.10, S3b)
    float modelElevation = 0.0f;
    const bool haveModel = ModelSunElevation(modelElevation);
    CHECK(haveModel);
    if (haveModel)
    {
        const float copy = std::asin(a.toLight.y / length(a.toLight)) * 180.0f / 3.14159265f;
        std::printf("  sun of A: the model %.1f degrees, the copy in this test %.1f\n", Show(modelElevation),
                    Show(copy));
        CHECK(Near(copy, modelElevation, 0.1f));
    }
    const float3 up(0, 1, 0);
    const float3 stone(0.86f, 0.76f, 0.60f); // A: limestone
    const float3 roofA(0.70f, 0.31f, 0.19f);
    const float3 paving(0.553f, 0.533f, 0.498f); // B: the paving of the present
    const float3 roofB(0.361f, 0.235f, 0.224f);
    // the wall that faces the sun, and a wall that the sun does not reach
    const float3 toSun = normalize(a.toLight);
    const float3 sunWall = normalize(float3(toSun.x, 0.0f, toSun.z));
    const float3 darkWall = normalize(float3(-toSun.z, 0.0f, toSun.x)); // at a right angle to the sun
    // ---- A: three tones, and the shadow is cooler than the light
    const float3 wallLit = Seen(set, 0, a, stone, sunWall, 1.0f);
    const float3 groundLit = Seen(set, 0, a, stone, up, 1.0f); // a low sun: the middle tone
    const float3 wallShade = Seen(set, 0, a, stone, darkWall, 0.0f);
    const float3 roofShade = Seen(set, 0, a, roofA, darkWall, 0.0f);
    std::printf("  look A: sunny wall L* %.1f, ground L* %.1f, shadow wall L* %.1f, roof in shadow L* %.1f\n",
                Show(LStar(wallLit)), Show(LStar(groundLit)), Show(LStar(wallShade)), Show(LStar(roofShade)));
    std::printf("  look A: blue over red: lit %.2f, shadow %.2f\n", Show(wallLit.z / wallLit.x),
                Show(wallShade.z / wallShade.x));
    // three values, in that order
    CHECK(LStar(wallLit) > LStar(groundLit) + 5.0f && LStar(groundLit) > LStar(wallShade) + 15.0f);
    // the light is warm and the shadow leans cool
    CHECK(wallLit.x > wallLit.z && wallShade.z / wallShade.x > 1.6f * (wallLit.z / wallLit.x));
    // and the shadow is not black
    CHECK(LStar(wallShade) >= 36.0f && LStar(roofShade) >= 15.0f);
    // ---- B: the open ground is not pale, the route reads in the moon alone, the deepest dark is not below L* 12
    const float3 groundB = Seen(set, 1, b, paving, up, 1.0f);
    const float3 groundCast = Seen(set, 1, b, paving, up, 1.0f - set.shadowStrength); // in the shadow of a house
    const float3 roofCast = Seen(set, 1, b, roofB, up, 1.0f - set.shadowStrength);
    const float3 roofWall = Seen(set, 1, b, roofB, darkWall, 0.0f);
    std::printf("  look B: ground L* %.1f, ground in a shadow L* %.1f, roof in a shadow L* %.1f, roof wall in the dark "
                "L* %.1f\n",
                Show(LStar(groundB)), Show(LStar(groundCast)), Show(LStar(roofCast)), Show(LStar(roofWall)));
    // lit by the moon: a value of its own, not pale
    CHECK(LStar(groundB) >= 50.0f && LStar(groundB) <= 64.0f);
    // the route reads in a shadow, with lamps_b = 0, and the shadow still shows
    CHECK(LStar(groundCast) >= 33.0f && LStar(groundB) > LStar(groundCast) + 10.0f);
    // 6.10: the deepest dark of B
    CHECK(LStar(roofCast) >= 14.0f && LStar(roofWall) >= 11.0f);
    // ---- the distance of A: lavender and pale. The horizon of the model (LIGHT of tools/layout/city_model.py: 0.99,
    // 0.80, 0.62, fog 0.0020) over a lit hillside; what is seen at 250 m is bluer and paler than the same hillside near
    const float3 hill(0.55f, 0.38f, 0.22f);
    const float3 horizonA(0.99f, 0.80f, 0.62f);
    const float3 hillNear = SeenFar(set, 0, horizonA, 0.0020f, 0.0f, hill);
    const float3 hillMiddle = SeenFar(set, 0, horizonA, 0.0020f, 130.0f, hill);
    const float3 hillFar = SeenFar(set, 0, horizonA, 0.0020f, 250.0f, hill);
    std::printf("  far A: hillside near blue/red %.2f chroma %.2f, at 130 m %.2f %.2f, at 250 m %.2f %.2f (L* %.0f)\n",
                Show(hillNear.z / hillNear.x), Show(Chroma(hillNear)), Show(hillMiddle.z / hillMiddle.x),
                Show(Chroma(hillMiddle)), Show(hillFar.z / hillFar.x), Show(Chroma(hillFar)), Show(LStar(hillFar)));
    CHECK(hillFar.z / hillFar.x > 1.8f * (hillNear.z / hillNear.x));   // bluer
    CHECK(Chroma(hillFar) < 0.6f * Chroma(hillNear));                  // paler
    CHECK(hillMiddle.z / hillMiddle.x > hillNear.z / hillNear.x);      // the middle distance has begun
    CHECK(hillMiddle.z / hillMiddle.x < hillFar.z / hillFar.x);        // and goes on
    CHECK(LStar(hillFar) > LStar(hillNear) && LStar(hillFar) < 95.0f); // lighter, not white
    // ---- the horizon of the sky of A is the colour of the far hills: the sky at the horizon (the horizon colour of
    // the model, leant and graded as the picture is) against the far land at 3 km, where the fog has taken all but
    // a thousandth of it
    const StyleParams styleA = StyleOf(set, 0);
    const float3 horizonLin(Linear(horizonA.x), Linear(horizonA.y), Linear(horizonA.z));
    const float3 skyHorizon =
        SoftShoulder(StyleGrade(StyleSky(horizonLin, 0.0f, styleA) * set.exposure, styleA, 0.0f, 0.0f), set.knee);
    const float3 oldSky = SoftShoulder(StyleGrade(horizonLin * set.exposure, styleA, 0.0f, 0.0f), set.knee);
    const float3 hillsLimit = SeenFar(set, 0, horizonA, 0.0020f, 3000.0f, hill);
    std::printf("  sky A at the horizon blue/red %.2f (it was %.2f), the far land at 3 km %.2f\n",
                Show(skyHorizon.z / skyHorizon.x), Show(oldSky.z / oldSky.x), Show(hillsLimit.z / hillsLimit.x));
    CHECK(Near3(skyHorizon, hillsLimit, 0.01f));                       // no band between the sky and the hills
    CHECK(skyHorizon.z / skyHorizon.x > 1.5f * (oldSky.z / oldSky.x)); // and it is not the old orange
    // not darker than it was and not blown out. (The lean keeps the brightness before the grade; after it the horizon
    // is brighter, L* 80 to 87, because an orange with a red channel far over the knee lost light to the shoulder and a
    // lavender, whose light is spread over the three, does not.)
    CHECK(LStar(skyHorizon) > LStar(oldSky) - 1.5f && LStar(skyHorizon) < 95.0f);
    // the same for B, which has its own horizon and a thicker fog
    const StyleParams styleB = StyleOf(set, 1);
    const float3 horizonB(0.45f, 0.49f, 0.56f);
    const float3 horizonBLin(Linear(horizonB.x), Linear(horizonB.y), Linear(horizonB.z));
    const float3 skyHorizonB =
        SoftShoulder(StyleGrade(StyleSky(horizonBLin, 0.0f, styleB) * set.exposure, styleB, 0.0f, 0.0f), set.knee);
    CHECK(Near3(skyHorizonB, SeenFar(set, 1, horizonB, 0.0036f, 3000.0f, float3(0.3f, 0.3f, 0.3f)), 0.01f));
    // the distance of B has no far fog of its own
    CHECK(StyleFarFog(250.0f, StyleOf(set, 1)) == 0.0f && StyleFarFog(250.0f, StyleOf(set, 0)) > 0.2f);
}

// ---- 7. the ink lines of docs/DESIGN.md 6.10 (S2)
// A scene of boxes, a ground, a ramp and a far wall is cast ray by ray into a depth buffer the way the game's
// projection makes it (D3D depth, near 0.1 m, far 600 m, 60 degrees of height, a camera 1.7 m over the ground looking
// along +z), and the functions of shaders/lines.hlsli run over it as the line pass will: one pixel and its four
// neighbours. What is checked is what the lines must be and not what they happen to be: a plane has no line at any
// distance or slant, a face has none inside, an outline is on the near side only, a turn of 50 degrees or more has an
// inner edge and a turn of 20 degrees has none.
struct LineBox
{
    double x0, x1, y0, y1, z0, z1;
};

struct LineScene
{
    static constexpr int kW = 640;
    static constexpr int kH = 360;
    static constexpr double kNear = 0.1;
    static constexpr double kFar = 600.0;
    static constexpr double kEye = 1.7;
    static constexpr double kPi = 3.14159265358979;
    double tanY = std::tan(30.0 * kPi / 180.0);
    double tanX = tanY * kW / kH;
    std::vector<LineBox> boxes;
    double wallZ = 300.0; ///< a wall across the whole view at this depth (it stands 50 m high)
    double rampZ = -1.0;  ///< the ground is a ramp from this depth on, rising at rampDegrees (< 0: flat)
    double rampDegrees = 0.0;
    std::vector<float> depth;
    std::vector<float3> edge; ///< x: outline, y: inner edge, z: depth of view, as LineEdge returns them

    // the depth of view of what the ray (sx, sy, 1) meets first, or -1 for the sky
    double Cast(double sx, double sy) const
    {
        double best = 1.0e30;
        const double ramp = std::tan(rampDegrees * kPi / 180.0);
        if (sy < 0.0)
        {
            const double t = -kEye / sy;
            if (rampZ < 0.0 || t <= rampZ)
            {
                best = std::min(best, t);
            }
        }
        if (rampZ > 0.0 && std::fabs(sy - ramp) > 1.0e-12)
        {
            const double t = (-rampZ * ramp - kEye) / (sy - ramp); // the ray meets y = (z - rampZ) * ramp
            if (t > rampZ)
            {
                best = std::min(best, t);
            }
        }
        for (const LineBox& b : boxes)
        {
            const double o[3] = {0.0, kEye, 0.0};
            const double d[3] = {sx, sy, 1.0};
            const double lo[3] = {b.x0, b.y0, b.z0};
            const double hi[3] = {b.x1, b.y1, b.z1};
            double tmin = 0.0;
            double tmax = 1.0e30;
            bool hit = true;
            for (int a = 0; a < 3; ++a)
            {
                if (std::fabs(d[a]) < 1.0e-12)
                {
                    hit = hit && o[a] >= lo[a] && o[a] <= hi[a];
                    continue;
                }
                double t1 = (lo[a] - o[a]) / d[a];
                double t2 = (hi[a] - o[a]) / d[a];
                if (t1 > t2)
                {
                    std::swap(t1, t2);
                }
                tmin = std::max(tmin, t1);
                tmax = std::min(tmax, t2);
            }
            if (hit && tmin <= tmax && tmin > 0.0)
            {
                best = std::min(best, tmin);
            }
        }
        if (kEye + sy * wallZ < 50.0)
        {
            best = std::min(best, wallZ);
        }
        return best >= 1.0e29 ? -1.0 : best;
    }

    // the depth buffer, then the edge at every pixel but the ones on the border of the picture
    void Run(float silhouetteLevel, float creaseDegrees)
    {
        const std::size_t count = static_cast<std::size_t>(kW) * static_cast<std::size_t>(kH);
        depth.assign(count, 1.0f);
        edge.assign(count, float3());
        for (int y = 0; y < kH; ++y)
        {
            for (int x = 0; x < kW; ++x)
            {
                const double t = Cast(Sx(x), Sy(y));
                if (t > 0.0)
                {
                    // the depth buffer value of a point at depth of view t, as the projection makes it
                    depth[Index(x, y)] = static_cast<float>(kFar * (t - kNear) / ((kFar - kNear) * t));
                }
            }
        }
        const float slope = PixelSlope(static_cast<float>(1.0 / tanY), static_cast<float>(kH));
        const float turnLo = 1.0f - std::cos(creaseDegrees * 3.14159265f / 180.0f);
        const float turnHi = 1.0f - std::cos((creaseDegrees + 15.0f) * 3.14159265f / 180.0f);
        for (int y = 1; y < kH - 1; ++y)
        {
            for (int x = 1; x < kW - 1; ++x)
            {
                edge[Index(x, y)] =
                    LineEdge(static_cast<float>(Sx(x)), static_cast<float>(Sy(y)), slope, W(x, y), W(x - 1, y),
                             W(x + 1, y), W(x, y - 1), W(x, y + 1), silhouetteLevel, turnLo, turnHi, 0.08f);
            }
        }
    }

    double Sx(int x) const
    {
        return (2.0 * (x + 0.5) / kW - 1.0) * tanX;
    }
    double Sy(int y) const
    {
        return (1.0 - 2.0 * (y + 0.5) / kH) * tanY;
    }
    static std::size_t Index(int x, int y)
    {
        return static_cast<std::size_t>(y) * static_cast<std::size_t>(kW) + static_cast<std::size_t>(x);
    }
    float W(int x, int y) const
    {
        return InverseViewDepth(depth[Index(x, y)], static_cast<float>(kNear), static_cast<float>(kFar));
    }
    const float3& At(int x, int y) const
    {
        return edge[Index(x, y)];
    }
    // the pixel column and row of the point (X, Y) of the wall of boxes at depth Z
    int Col(double X, double Z) const
    {
        return static_cast<int>((X / Z / tanX + 1.0) * 0.5 * kW);
    }
    int Row(double Y, double Z) const
    {
        return static_cast<int>((1.0 - (Y - kEye) / Z / tanY) * 0.5 * kH);
    }
    // the largest outline and inner edge over a block of pixels (inclusive)
    void Largest(int x0, int y0, int x1, int y1, float& outline, float& crease) const
    {
        outline = 0.0f;
        crease = 0.0f;
        for (int y = y0; y <= y1; ++y)
        {
            for (int x = x0; x <= x1; ++x)
            {
                outline = std::max(outline, At(x, y).x);
                crease = std::max(crease, At(x, y).y);
            }
        }
    }
};

float LuminanceOf(const float3& c)
{
    return 0.2126f * c.x + 0.7152f * c.y + 0.0722f * c.z;
}

// The pass of shaders/lines.hlsl for one pixel, written again in C++: the registers that MakeLines gives, the depth of
// a scene, the square of pixels round the pixel and the largest coverage in it. KEEP IN SYNC with the pixel shader.
float PassCoverage(const LineScene& scene, const dl::LineConstants& c, int px, int py)
{
    const float widest = std::max(c.width[0], c.width[1]);
    const int reach = static_cast<int>(std::min(std::max(std::ceil(0.5f * widest + 0.5f) - 1.0f, 1.0f), 4.0f));
    const auto inverseDepth = [&](int x, int y)
    {
        const int cx = std::clamp(x, 0, LineScene::kW - 1);
        const int cy = std::clamp(y, 0, LineScene::kH - 1);
        return InverseViewDepth(scene.depth[LineScene::Index(cx, cy)], c.depth[0], c.depth[1]);
    };
    float coverage = 0.0f;
    for (int j = -reach; j <= reach; ++j)
    {
        for (int i = -reach; i <= reach; ++i)
        {
            const int x = px + i;
            const int y = py + j;
            if (x < 1 || y < 1 || x > LineScene::kW - 2 || y > LineScene::kH - 2)
            {
                continue; // no line on the outermost row and column
            }
            const float sx =
                ((static_cast<float>(x) + 0.5f) / static_cast<float>(LineScene::kW) * 2.0f - 1.0f) * c.width[2];
            const float sy =
                (1.0f - (static_cast<float>(y) + 0.5f) / static_cast<float>(LineScene::kH) * 2.0f) * c.width[3];
            const float3 e = LineEdge(sx, sy, c.depth[2], inverseDepth(x, y), inverseDepth(x - 1, y),
                                      inverseDepth(x + 1, y), inverseDepth(x, y - 1), inverseDepth(x, y + 1),
                                      c.levels[0], c.levels[1], c.levels[2], c.depth[3]);
            if (e.x > 0.0f || e.y > 0.0f)
            {
                const float dist = std::sqrt(static_cast<float>(i * i + j * j));
                coverage =
                    std::max(coverage, LineCoverage(e, dist, c.width[0], c.width[1], c.fade[0], c.fade[1], c.fade[2]));
            }
        }
    }
    return coverage;
}

void TestLines()
{
    // ---- the depth and the pixel
    const float nearZ = 0.1f;
    const float farZ = 600.0f;
    CHECK(Near(InverseViewDepth(0.0f, nearZ, farZ), 10.0f, 1.0e-4f) &&
          Near(InverseViewDepth(1.0f, nearZ, farZ), 1.0f / 600.0f, 1.0e-6f));
    bool matches = true;
    bool falling = true;
    float last = 1.0e9f;
    for (int z = 1; z <= 400; z += 3)
    {
        const double zd = static_cast<double>(z);
        const float d = static_cast<float>(600.0 * (zd - 0.1) / (599.9 * zd)); // the projection of a point at depth z
        const float w = InverseViewDepth(d, nearZ, farZ);
        matches = matches && std::fabs(w * static_cast<float>(zd) - 1.0f) < 2.0e-3f;
        falling = falling && w < last;
        last = w;
    }
    CHECK(matches && falling); // 1 / z from the depth buffer, to 0.2%, up to 400 m; it falls with the distance
    CHECK(Near(PixelSlope(2.0f, 1000.0f), 0.001f, 1.0e-9f));

    // ---- a scene of boxes: a house, a crate, a pole, a far wall
    static LineScene a;
    a.boxes = {{-2, 2, 0, 3, 18, 22}, {4, 6, 0, 1.2, 10, 12}, {-6, -5.9, 0, 4, 14, 14.1}};
    a.Run(6.0f, 30.0f);
    float outline = 0.0f;
    float crease = 0.0f;
    // a plane has no line, at any distance or slant: the ground from 3 m to the far wall, on the left of the view
    a.Largest(1, 182, 120, 358, outline, crease);
    CHECK(outline < 0.01f && crease < 0.01f);
    // the face of the house and of the crate have none inside
    a.Largest(a.Col(-1.5, 18), a.Row(2.6, 18), a.Col(1.5, 18), a.Row(0.4, 18), outline, crease);
    CHECK(outline < 0.01f && crease < 0.01f);
    a.Largest(a.Col(4.2, 10), a.Row(1.1, 10) + 1, a.Col(5.8, 10), a.Row(0.1, 10) - 1, outline, crease);
    CHECK(outline < 0.01f && crease < 0.01f);
    // the outline of the house against the far wall: on the pixel of the house, never on the one of the wall beside it
    bool leftNear = true;
    bool leftOnce = true;
    bool rightNear = true;
    bool rightOnce = true;
    const int xl = a.Col(-2.0, 18);
    const int xr = a.Col(2.0, 18);
    for (int y = a.Row(2.8, 18); y <= a.Row(0.4, 18); ++y)
    {
        leftNear = leftNear && a.At(xl, y).x > 0.99f && a.At(xl, y).z < 20.0f;
        leftOnce = leftOnce && a.At(xl - 1, y).x < 0.01f && a.At(xl + 1, y).x < 0.01f && a.At(xl - 1, y).y < 0.01f;
        rightNear = rightNear && a.At(xr, y).x > 0.99f && a.At(xr, y).z < 20.0f;
        rightOnce = rightOnce && a.At(xr - 1, y).x < 0.01f && a.At(xr + 1, y).x < 0.01f && a.At(xr + 1, y).y < 0.01f;
    }
    CHECK(leftNear && leftOnce && rightNear && rightOnce);
    const int xm = a.Col(0.0, 18);
    const int yTop = a.Row(3.0, 18);
    // the roof line, one pixel
    CHECK(a.At(xm, yTop).x > 0.99f && a.At(xm, yTop - 1).x < 0.01f && a.At(xm, yTop + 1).x < 0.01f);
    // the foot of the house, where the wall meets the ground (a turn of 90 degrees), has an inner edge within a pixel
    const int yFoot = a.Row(0.0, 18);
    float footEdge = 0.0f;
    for (int y = yFoot - 1; y <= yFoot + 1; ++y)
    {
        footEdge = std::max(footEdge, a.At(xm, y).y);
    }
    CHECK(footEdge > 0.99f);
    // the crate seen from above at a slant: the edge of its top and the foot of its face, and nothing on its face
    const int xc = a.Col(5.0, 10);
    float topEdge = 0.0f;
    float crateFoot = 0.0f;
    for (int y = a.Row(1.2, 10) - 4; y <= a.Row(1.2, 10) + 4; ++y)
    {
        topEdge = std::max(topEdge, std::max(a.At(xc, y).x, a.At(xc, y).y));
    }
    for (int y = a.Row(0.0, 10) - 1; y <= a.Row(0.0, 10) + 1; ++y)
    {
        crateFoot = std::max(crateFoot, a.At(xc, y).y);
    }
    CHECK(topEdge > 0.99f && crateFoot > 0.99f);
    // a pole of 10 cm at 14 m is two pixels wide: both of its sides are outlines
    const int yp = a.Row(2.5, 14);
    const int xp = a.Col(-5.95, 14);
    float pole = 0.0f;
    for (int x = xp - 4; x <= xp + 4; ++x)
    {
        pole = std::max(pole, a.At(x, yp).x);
    }
    CHECK(pole > 0.99f);
    // the depth to which the lines are made: the pixel of the outline knows its depth of view (for the fade)
    CHECK(Near(a.At(xl, a.Row(1.5, 18)).z, 18.0f, 0.5f));

    // ---- a change of slope in the ground: 12 and 20 degrees (the facets of the terrain) draw nothing, 50 and more do
    const double turns[] = {12.0, 20.0, 50.0, 60.0, 80.0};
    const bool wanted[] = {false, false, true, true, true};
    bool turnsRight = true;
    float half = 0.0f;
    for (int i = 0; i < 5; ++i)
    {
        static LineScene r;
        r.boxes.clear();
        r.wallZ = 1.0e6;
        r.rampZ = 25.0;
        r.rampDegrees = turns[i];
        r.Run(6.0f, 30.0f);
        const int join = r.Row(0.0, 25.0);
        float atJoin = 0.0f;
        for (int y = join - 3; y <= join + 3; ++y)
        {
            for (int x = 300; x < 340; ++x)
            {
                atJoin = std::max(atJoin, r.At(x, y).y);
            }
        }
        float onRamp = 0.0f;
        float unused = 0.0f;
        r.Largest(300, std::max(1, join - 40), 339, join - 6, unused, onRamp); // the ramp itself is a plane
        turnsRight = turnsRight && onRamp < 0.01f && (wanted[i] ? atJoin > 0.85f : atJoin < 0.01f);
        if (i == 2)
        {
            half = atJoin;
        }
    }
    CHECK(turnsRight);
    // 50 degrees is (nearly) full: the start of 30 degrees is for a turn that is seen across two pixels
    CHECK(half > 0.85f && half <= 1.0f);

    // ---- the distance: the inner edges fade out, the outlines thin
    CHECK(LineCreaseFade(10.0f, 40.0f, 80.0f) == 1.0f && LineCreaseFade(80.0f, 40.0f, 80.0f) == 0.0f);
    CHECK(LineCreaseFade(60.0f, 40.0f, 80.0f) > 0.0f && LineCreaseFade(60.0f, 40.0f, 80.0f) < 1.0f);
    CHECK(LineSilhouetteFade(50.0f, 80.0f, 0.5f) == 1.0f &&
          Near(LineSilhouetteFade(400.0f, 80.0f, 0.5f), 0.5f, 1.0e-6f));
    // never thinner than far_alpha
    CHECK(LineSilhouetteFade(1000.0f, 80.0f, 0.5f) == LineSilhouetteFade(400.0f, 80.0f, 0.5f));

    // ---- the width of a line: a pixel in the middle of a line is full, the ones beside it have what its width leaves
    CHECK(InkReach(0.0f, 1.0f) == 1.0f && InkReach(1.0f, 1.0f) == 0.0f && Near(InkReach(1.0f, 2.0f), 0.5f, 1.0e-6f));
    CHECK(InkReach(1.0f, 3.0f) == 1.0f && InkReach(2.0f, 3.0f) == 0.0f); // a width of 3: the pixel and one on each side
    const float3 anOutline(1.0f, 0.0f, 18.0f);
    const float3 anInner(0.0f, 1.0f, 18.0f);
    // the outline is wider than the inner edge
    CHECK(Near(LineCoverage(anOutline, 1.0f, 2.0f, 1.2f, 40.0f, 80.0f, 0.5f), 0.5f, 1.0e-6f));
    CHECK(Near(LineCoverage(anInner, 1.0f, 2.0f, 1.2f, 40.0f, 80.0f, 0.5f), 0.1f, 1.0e-6f));
    CHECK(LineCoverage(float3(0.0f, 0.0f, 18.0f), 0.0f, 2.0f, 1.2f, 40.0f, 80.0f, 0.5f) == 0.0f);
    // far away the outline is thin and the inner edge is gone
    CHECK(Near(LineCoverage(float3(1.0f, 0.0f, 3000.0f), 0.0f, 2.0f, 1.2f, 40.0f, 80.0f, 0.5f), 0.5f, 1.0e-6f));
    CHECK(LineCoverage(float3(0.0f, 1.0f, 3000.0f), 0.0f, 2.0f, 1.2f, 40.0f, 80.0f, 0.5f) == 0.0f);

    // ---- the colour: the line is a dark of the local colour and not black
    const float3 warmLine(0.027f, 0.016f, 0.011f); // A, linear
    const float3 sunnyWall(0.90f, 0.60f, 0.30f);
    const float3 ink = InkColor(warmLine, sunnyWall, 0.35f);
    CHECK(LuminanceOf(ink) < 0.5f * LuminanceOf(sunnyWall) && ink.x > ink.z);
    CHECK(LuminanceOf(ink) > LuminanceOf(warmLine)); // a dark of the local colour, not the colour of the line itself
    CHECK(Near3(InkColor(warmLine, sunnyWall, 0.0f), warmLine, 1.0e-6f));
    CHECK(Near3(LineComposite(sunnyWall, 0.0f, warmLine, 0.35f, 1.0f), sunnyWall, 1.0e-6f));
    CHECK(Near3(LineComposite(sunnyWall, 1.0f, warmLine, 0.35f, 1.0f), ink, 1.0e-6f));
    CHECK(Near3(LineComposite(sunnyWall, 1.0f, warmLine, 0.35f, 0.0f), sunnyWall, 1.0e-6f)); // amount 0: no line
    const float3 half50 = LineComposite(sunnyWall, 0.5f, warmLine, 0.35f, 1.0f);
    CHECK(Near3(half50, lerp(sunnyWall, ink, 0.5f), 1.0e-6f));
    // the blend of the pass (source.rgb + picture * (1 - source.a)) is the same line, for any picture, coverage, master
    const float3 pictures[] = {float3(0.9f, 0.6f, 0.3f), float3(0.05f, 0.08f, 0.2f), float3(0.0f, 0.0f, 0.0f),
                               float3(1.0f, 1.0f, 1.0f)};
    const float coverages[] = {0.0f, 0.1f, 0.5f, 1.0f};
    const float masters[] = {0.0f, 0.6f, 1.0f};
    bool blendIsLine = true;
    bool blendBounded = true;
    for (const float3& picture : pictures)
    {
        for (const float cover : coverages)
        {
            for (const float master : masters)
            {
                const float3 got = LineBlendColor(cover, warmLine, 0.35f, master) +
                                   picture * (1.0f - LineBlendAlpha(cover, 0.35f, master));
                const float3 want = LineComposite(picture, cover, warmLine, 0.35f, master);
                blendIsLine = blendIsLine && Near3(got, want, 1.0e-6f);
                const float alpha = LineBlendAlpha(cover, 0.35f, master);
                const float3 drawn = LineBlendColor(cover, warmLine, 0.35f, master);
                blendBounded = blendBounded && alpha >= 0.0f && alpha <= 1.0f && drawn.x >= 0.0f;
            }
        }
    }
    CHECK(blendIsLine && blendBounded);
    // no line: no change
    CHECK(LineBlendAlpha(0.0f, 0.35f, 1.0f) == 0.0f && LineBlendColor(0.0f, warmLine, 0.35f, 1.0f).x == 0.0f);

    // ---- the settings
    const dl::LineSettings d{};
    CHECK(d.amount == 1.0f && d.width == 2.0f && d.creaseWidth == 1.2f && d.silhouette == 6.0f && d.crease == 30.0f);
    CHECK(d.fadeNear == 40.0f && d.fadeFar == 70.0f && d.farAlpha == 0.3f && d.local == 0.35f);
    dl::LineSettings bad;
    bad.amount = 7.0f;
    bad.width = 0.0f;
    bad.creaseWidth = std::nanf("");
    bad.silhouette = -3.0f;
    bad.crease = 500.0f;
    bad.fadeNear = 300.0f;
    bad.fadeFar = 100.0f;
    bad.farAlpha = -1.0f;
    bad.local = 2.0f;
    const dl::LineSettings fixed = dl::Sanitize(bad);
    CHECK(fixed.amount == 1.0f && fixed.width == 0.5f && fixed.creaseWidth == d.creaseWidth);
    CHECK(fixed.silhouette == 2.0f);
    CHECK(fixed.crease == 80.0f && fixed.fadeNear == 300.0f && fixed.fadeFar >= fixed.fadeNear + 5.0f);
    CHECK(fixed.farAlpha == 0.0f && fixed.local == 1.0f);
    dl::Config c;
    c.ParseString("[lines]\nwidth = 3\ncrease = 42\nfade_far = 120\nlocal = abc\n");
    const dl::LineSettings r = dl::ReadLineSettings(c);
    CHECK(r.width == 3.0f && r.crease == 42.0f && r.fadeFar == 120.0f && r.local == d.local && r.amount == d.amount);
    // the shipped file says what the code defaults to: a number changed in one and not in the other would be a surprise
    dl::Config shipped;
    if (ShippedConfig(shipped))
    {
        const dl::LineSettings f = dl::ReadLineSettings(shipped);
        const float e = 1.0e-6f;
        CHECK(Near(f.amount, d.amount, e) && Near(f.width, d.width, e) && Near(f.creaseWidth, d.creaseWidth, e));
        CHECK(Near(f.silhouette, d.silhouette, e) && Near(f.crease, d.crease, e) && Near(f.fadeNear, d.fadeNear, e));
        CHECK(Near(f.fadeFar, d.fadeFar, e) && Near(f.farAlpha, d.farAlpha, e) && Near(f.local, d.local, e));
    }
    // the registers
    const dl::LineConstants a1 = dl::MakeLines(d, 0, 1.0f, 1.0f / static_cast<float>(a.tanY), 1080.0f, 0.1f, 600.0f);
    const dl::LineConstants b1 = dl::MakeLines(d, 1, 1.0f, 1.0f / static_cast<float>(a.tanY), 1080.0f, 0.1f, 600.0f);
    CHECK(a1.ink[0] > a1.ink[2] && b1.ink[2] > b1.ink[0]);              // A warm brown-black, B indigo-black
    CHECK(a1.ink[0] < 0.05f && a1.ink[1] < 0.05f && b1.ink[2] < 0.05f); // and dark, in linear light
    CHECK(Near(a1.depth[2], 2.0f * static_cast<float>(a.tanY) / 1080.0f, 1.0e-9f));
    CHECK(a1.depth[0] == 0.1f && a1.depth[1] == 600.0f);
    // the view ray of a pixel: tan of half the field of view up (from the projection) and across (wider on a wide one)
    CHECK(Near(a1.width[3], static_cast<float>(a.tanY), 1.0e-6f) && Near(a1.width[2], a1.width[3], 1.0e-6f));
    const float p11 = 1.0f / static_cast<float>(a.tanY);
    const dl::LineConstants wide = dl::MakeLines(d, 0, 1.0f, p11, 900.0f, 0.1f, 300.0f, p11 * 900.0f / 1600.0f);
    CHECK(Near(wide.width[2], static_cast<float>(a.tanY) * 1600.0f / 900.0f, 1.0e-5f) && wide.depth[1] == 300.0f);
    CHECK(Near(wide.width[3], static_cast<float>(a.tanY), 1.0e-6f)); // up is the same on any screen
    CHECK(Near(a1.levels[1], 1.0f - std::cos(30.0f * 3.14159265f / 180.0f), 1.0e-6f) && a1.levels[2] > a1.levels[1]);
    CHECK(a1.levels[0] == 6.0f && a1.levels[3] == 1.0f && a1.width[0] == 2.0f && Near(a1.width[1], 1.2f, 1.0e-6f));
    // the plain view (style master 0): no line
    CHECK(dl::MakeLines(d, 0, 0.0f, 1.0f, 1080.0f, 0.1f, 600.0f).levels[3] == 0.0f);
    const dl::LineConstants tall = dl::MakeLines(d, 0, 1.0f, 1.0f, 2160.0f, 0.1f, 600.0f);
    const dl::LineConstants small = dl::MakeLines(d, 0, 1.0f, 1.0f, 540.0f, 0.1f, 600.0f);
    // pixels scale with the height of the picture, and never go below the setting
    CHECK(Near(tall.width[0], 4.0f, 1.0e-6f) && Near(small.width[0], 2.0f, 1.0e-6f));
    const dl::LineConstants shy = dl::MakeLines(d, 0, 1.0f, 1.0f, 1600.0f, 0.1f, 600.0f);
    CHECK(Near(shy.width[0], 2.0f * 1600.0f / 1080.0f, 1.0e-5f)); // 2560 x 1600 of the laptop
    // ---- the pass, with the registers of MakeLines, on the scene of the house: the line is on the pixel of the edge
    // and one pixel to each side of it (a width of 2), nowhere else, and nothing on a face or on the ground
    const float tanX = static_cast<float>(a.tanX);
    const dl::LineConstants pass =
        dl::MakeLines(d, 0, 1.0f, p11, static_cast<float>(LineScene::kH), static_cast<float>(LineScene::kNear),
                      static_cast<float>(LineScene::kFar), 1.0f / tanX);
    const int yMid = a.Row(1.5, 18);
    CHECK(Near(PassCoverage(a, pass, xl, yMid), 1.0f, 1.0e-6f));
    CHECK(Near(PassCoverage(a, pass, xl - 1, yMid), 0.5f, 1.0e-6f) &&
          Near(PassCoverage(a, pass, xl + 1, yMid), 0.5f, 1.0e-6f));
    CHECK(PassCoverage(a, pass, xl - 2, yMid) == 0.0f && PassCoverage(a, pass, xl + 2, yMid) == 0.0f);
    CHECK(PassCoverage(a, pass, a.Col(0.0, 18), yMid) == 0.0f); // the face of the house
    CHECK(PassCoverage(a, pass, 40, 300) == 0.0f);              // the ground
    // a screen of another height and shape gives the same lines, in its own pixels
    const dl::LineConstants other = dl::MakeLines(d, 0, 1.0f, p11, 1440.0f, 0.1f, 600.0f, 1.0f / tanX);
    CHECK(Near(other.depth[2], 2.0f / (p11 * 1440.0f), 1.0e-9f));
    CHECK(Near(other.width[0], 2.0f * 1440.0f / 1080.0f, 1.0e-5f));
}

} // namespace

int main()
{
    TestLamp();
    TestSun();
    TestSurface();
    TestGrade();
    TestSettings();
    TestLook();
    TestLines();
    std::printf("%d checks passed, %d failed\n", g_passed, g_failed);
    return g_failed == 0 ? 0 : 1;
}
