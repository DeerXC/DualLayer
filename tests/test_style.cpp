// test_style.cpp - tests of the style of the light (no window, no GPU): the functions of shaders/lighting.hlsli under
// "the style of the light" (through a stand-in for the vector types of HLSL: tests/hlsl_shim.h) and the numbers that
// feed them (src/render/LightingSettings.h: StyleLookOf, MakeStyle, the keys of [lighting]).
//   g++ -std=c++20 -O2 -Wall -Wextra -I src tests/test_style.cpp src/core/Config.cpp -o test_style
//   MSVC (Developer PowerShell):  .\tests\run_tests.ps1 -Only style
// What is checked:
//   1. the lamp: the rings (0 at 0, only rising, flat in the middle of a step, the amount 0 leaves the smooth light),
//   the warmth (the
//      core whiter than the edge), and that a style of 0 is exactly the smooth lamp of LampShade
//   2. the sun: the soft bands (the ends, only rising, flat inside a band), the clouds (the range, the amount, the
//   drift, the share of
//      the sky they cover, the same value along a ray of light), the rim (none on a floor, none seen squarely, more at
//      the edge)
//   3. a surface: the plain sum when the style is 0; the cool fill on the shadow side only; the rim of the stone is a
//   third of
//      the rim of the rest; the painted stone (identity at 0, a bounded change, damp darker and cooler); the haze
//      (nothing near)
//   4. the grade: identity when neutral, the contrast keeps the middle grey, the corners, no negative colour, the
//   saturation
//   5. the settings: the defaults, nonsense pulled in, the keys of the config, MakeStyle (plain = zeros, the master,
//   the shafts need
//      the shadows, A and B differ), and the looks (cool and tones keep the brightness)
// Returns 1 when a check fails.

#include "render/LightingSettings.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

// the shader maths, as C++ (the order matters: see tests/hlsl_shim.h)
#include "hlsl_shim.h"

#include "../shaders/lighting.hlsli"

#include "hlsl_shim_end.h"

namespace
{

int g_failed = 0;
int g_passed = 0;

#define CHECK(cond)                                                                                                    \
    do                                                                                                                 \
    {                                                                                                                  \
        if (cond)                                                                                                      \
        {                                                                                                              \
            ++g_passed;                                                                                                \
        }                                                                                                              \
        else                                                                                                           \
        {                                                                                                              \
            ++g_failed;                                                                                                \
            std::printf("FAIL line %d: %s\n", __LINE__, #cond);                                                        \
        }                                                                                                              \
    } while (0)

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

// a style with every part on (the looks of A at full strength)
StyleParams Full()
{
    const dl::StyleConstants c = dl::MakeStyle(dl::LightingSettings{}, 0, false);
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
    return s;
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
    CHECK(ToonRamp(0.0f, 1.0f) == 0.0f && Near(ToonRamp(0.78f, 1.0f), 1.0f, 1.0e-6f) && ToonRamp(1.0f, 1.0f) == 1.0f);
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
    // inside a band the light is flat: 0.22 .. 0.30 is one level, 0.52 .. 0.60 another
    CHECK(Near(ToonRamp(0.22f, 1.0f), ToonRamp(0.30f, 1.0f), 1.0e-6f) &&
          Near(ToonRamp(0.55f, 1.0f), ToonRamp(0.60f, 1.0f), 1.0e-6f));
    CHECK(ToonRamp(0.55f, 1.0f) > ToonRamp(0.25f, 1.0f) + 0.2f && ToonRamp(0.9f, 1.0f) > ToonRamp(0.55f, 1.0f) + 0.2f);

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
    CHECK(Near3(StyleHaze(base, 10.0f, haze), base, 0.0f) && Near3(StyleHaze(base, 40.0f, haze), base, 1.0e-6f));
    const float3 farAway = StyleHaze(base, 300.0f, haze);
    CHECK(Near3(farAway, base * haze.farTint, 1.0e-6f));
    CHECK(StyleHaze(base, 150.0f, haze).z / StyleHaze(base, 150.0f, haze).x > base.z / base.x);
    CHECK(Near3(StyleHaze(base, 300.0f, Neutral()), base, 0.0f));
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
    const dl::LightingSettings fixed = dl::Sanitize(bad);
    CHECK(fixed.style == 1.0f && fixed.toon == 0.0f && fixed.rim == d.rim && fixed.cloudsA == 0.9f &&
          fixed.shaftSteps == 32);
    CHECK(fixed.saturation == 0.0f && fixed.contrast == 2.0f && fixed.vignette == 2.0f && fixed.wash == 1.0f);
    bad.shaftSteps = -4;
    CHECK(dl::Sanitize(bad).shaftSteps == 4);

    dl::Config c;
    c.ParseString("[lighting]\nstyle = 0.5\ntoon = 0.2\nshafts = false\nshaft_steps = 8\nclouds_b = 0.1\nlamp_rings = "
                  "0\nwash = 0.3\n"
                  "haze = 0.9\ncontrast = 1.5\nrim = abc\n");
    const dl::LightingSettings r = dl::ReadLightingSettings(c);
    CHECK(r.style == 0.5f && r.toon == 0.2f && !r.shafts && r.shaftSteps == 8 && r.cloudsB == 0.1f &&
          r.lampRings == 0.0f);
    CHECK(r.wash == 0.3f && r.haze == 0.9f && r.contrast == 1.5f && r.rim == d.rim && r.cloudsA == d.cloudsA);
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
    dl::LightingSettings off;
    off.style = 0.0f;
    const dl::StyleConstants master0 = dl::MakeStyle(off, 1, false);
    CHECK(master0.grade2[2] == 1.0f && master0.misc[2] == 1.0f && master0.light[2] == 0.0f && master0.cool[3] == 0.0f);
    const dl::StyleConstants a = dl::MakeStyle(d, 0, false);
    const dl::StyleConstants b = dl::MakeStyle(d, 1, false);
    CHECK(a.grade2[0] == d.toon && a.grade2[1] == d.rim && a.grade2[2] > 1.0f && a.grade2[3] > 0.0f &&
          a.misc[2] > 1.0f);
    CHECK(a.light[0] == d.cloudsA && b.light[0] == d.cloudsB && a.light[2] == d.shaftsA && b.light[2] == d.shaftsB);
    CHECK(b.misc[1] > a.misc[1] && b.grade2[3] > a.grade2[3] &&
          b.cool[2] != a.cool[2]); // B is damper, darker in the corners, another colour
    CHECK(a.light[3] == static_cast<float>(d.shaftSteps));
    dl::LightingSettings noShadows;
    noShadows.shadows = false;
    CHECK(dl::MakeStyle(noShadows, 0, false).light[2] == 0.0f); // no shadow map, no shafts
    dl::LightingSettings noShafts;
    noShafts.shafts = false;
    CHECK(dl::MakeStyle(noShafts, 1, false).light[2] == 0.0f && dl::MakeStyle(noShafts, 1, false).light[0] > 0.0f);
    dl::LightingSettings half;
    half.style = 0.5f;
    const dl::StyleConstants h = dl::MakeStyle(half, 0, false);
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
        CHECK(Mean(look.hazeTint) > 0.85f && Mean(look.hazeTint) < 1.1f);
        CHECK(look.cool[2] > look.cool[0] && look.low[2] > look.low[0]); // the shadows lean blue in both
        CHECK(look.high[0] != look.high[2] && look.lowAmount > 0.0f && look.highAmount > 0.0f);
        CHECK(look.contrast >= 1.0f && look.contrast <= 1.4f && look.saturation >= 1.0f && look.saturation <= 1.4f);
        CHECK(look.vignette >= 0.0f && look.vignette <= 0.5f && look.dampScale > 0.0f);
    }
    CHECK(dl::StyleLookOf(0).high[0] > dl::StyleLookOf(0).high[2]);  // A: the lights are gold
    CHECK(dl::StyleLookOf(1).high[2] > dl::StyleLookOf(1).high[0]);  // B: the lights are pale cyan
    CHECK(dl::StyleLookOf(7).cool[0] == dl::StyleLookOf(1).cool[0]); // a layer that is not 0 is B
}

} // namespace

int main()
{
    TestLamp();
    TestSun();
    TestSurface();
    TestGrade();
    TestSettings();
    std::printf("%d checks passed, %d failed\n", g_passed, g_failed);
    return g_failed == 0 ? 0 : 1;
}
