// test_lighting.cpp - tests of the light of the town (no window, no GPU): shaders/lighting.hlsli (through a stand-in
// for the vector types of HLSL: tests/hlsl_shim.h), src/render/Lights.h, src/render/ShadowMath.h,
// src/render/LightingSettings.h, src/scene/Lamps.h and the lamps of the town file (data/town.dlt: python
// tools/layout/city_model.py export).
//   g++ -std=c++20 -O2 -Wall -Wextra -I src tests/test_lighting.cpp src/core/Config.cpp -o test_lighting
//   MSVC (Developer PowerShell):  .\tests\run_tests.ps1 -Only lighting
// What is checked:
//   1. the maths of the shader: the fall-off of a lamp (1.4 under a street lamp, 0 at its range, only falling), one
//   lamp on a surface (not
//      behind it, not beyond its range, the highlight where the half vector says), the soft shoulder (nothing below the
//      knee, only rising, never 1, the hue kept, no kink), the fade at the edge of the shadow map, the halo and the
//      haze of the sun
//   2. the choice of lamps: the moment, the cap of kMaxLamps, the nearest and strongest stay, a far lamp fades out, the
//   scales, the flicker
//   3. the shadow box: what is inside it is inside the map, depth runs from the light, the box moves on the texel grid,
//   a light overhead,
//      nonsense in; and the whole chain on a toy scene (a software depth map drawn through the matrix, looked up with
//      the formulas of ShadowAt and the default bias): no speckle on a plane, also at a grazing angle; a box leaves the
//      shadow that a ray says it should
//   4. the settings: defaults, a nonsense value pulled in, the size a power of two, the config file
//   5. the town: its lamps (kinds, moments, positions), their conversion to point lights, the lantern, the lit lamps of
//   the mast, the bell
//      lamps, the halos (a weaker one in A, a cap, a distance)
// Returns 1 when a check fails.

#include "render/LightingSettings.h"
#include "render/Lights.h"
#include "render/ShadowMath.h"
#include "scene/Lamps.h"
#include "scene/Things.h"
#include "scene/Town.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

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

// ---- 1. the maths of the shader

void TestAttenuation()
{
    CHECK(Near(LampAttenuation(3.4f, 14.0f) * 18.0f, 1.36f, 0.05f)); // a street lamp (18), 3.4 m over the ground
    CHECK(LampAttenuation(14.0f, 14.0f) == 0.0f && LampAttenuation(20.0f, 14.0f) == 0.0f);
    CHECK(LampAttenuation(13.9f, 14.0f) < 1.0e-4f); // no hard edge to the circle of light
    float last = 1.0e9f;
    bool falling = true;
    for (float d = 0.0f; d <= 14.0f; d += 0.1f)
    {
        const float a = LampAttenuation(d, 14.0f);
        falling = falling && a <= last + 1.0e-7f && a >= 0.0f;
        last = a;
    }
    CHECK(falling);
    CHECK(LampAttenuation(0.0f, 14.0f) > 0.0f && LampAttenuation(0.0f, 14.0f) < 1.0f); // softened, never infinite
    CHECK(LampAttenuation(2.0f, 0.0f) == 0.0f);                                        // a range of 0 is no light
}

void TestLampShade()
{
    const float3 up(0.0f, 1.0f, 0.0f);
    const float3 eye(0.0f, 1.0f, 0.0f);
    const float3 lamp(0.0f, 3.4f, 0.0f);
    const float3 colour(1.0f, 0.5f, 0.25f);
    // straight under the lamp: the diffuse is the colour times the fall-off
    LampLight l = LampShade(float3(0.0f, 0.0f, 0.0f), up, eye, lamp, 14.0f, colour, 0.0f, 16.0f);
    const float a = LampAttenuation(3.4f, 14.0f);
    CHECK(Near(l.diffuse.x, a, 1.0e-6f) && Near(l.diffuse.y, 0.5f * a, 1.0e-6f) &&
          Near(l.diffuse.z, 0.25f * a, 1.0e-6f));
    CHECK(l.specular.x == 0.0f && l.specular.y == 0.0f); // a shine of 0: no highlight
    // the lamp behind the surface: nothing
    l = LampShade(float3(0.0f, 0.0f, 0.0f), float3(0.0f, -1.0f, 0.0f), eye, lamp, 14.0f, colour, 1.0f, 16.0f);
    CHECK(l.diffuse.x == 0.0f && l.specular.x == 0.0f);
    // beyond its range: nothing
    l = LampShade(float3(0.0f, -20.0f, 0.0f), up, eye, lamp, 14.0f, colour, 1.0f, 16.0f);
    CHECK(l.diffuse.x == 0.0f && l.specular.z == 0.0f);
    // at a slant the diffuse is the cosine of the angle (and the fall-off of the longer way)
    l = LampShade(float3(3.4f, 0.0f, 0.0f), up, eye, lamp, 14.0f, colour, 0.0f, 16.0f);
    const float dist = std::sqrt(3.4f * 3.4f * 2.0f);
    CHECK(Near(l.diffuse.x, LampAttenuation(dist, 14.0f) * std::sqrt(0.5f), 1.0e-5f));
    // the highlight: the eye in the mirror direction sees it, an eye far off does not
    const float3 surface(0.0f, 0.0f, 0.0f);
    const float3 lampAt(1.0f, 1.0f, 0.0f); // 45 degrees up on the right
    l = LampShade(surface, up, normalize(float3(-1.0f, 1.0f, 0.0f)), lampAt, 14.0f, colour, 1.0f,
                  32.0f); // the eye on the mirror side
    const LampLight off = LampShade(surface, up, normalize(float3(1.0f, 1.0f, 0.0f)), lampAt, 14.0f, colour, 1.0f,
                                    32.0f); // the eye beside the lamp
    CHECK(l.specular.x > 100.0f * off.specular.x && l.specular.x > 0.0f);
    CHECK(Near(l.specular.x, l.diffuse.x / std::sqrt(0.5f),
               1.0e-5f)); // half vector = normal: the whole lit colour, the diffuse without the cosine
    // twice the colour, twice the light
    const LampLight twice = LampShade(float3(0.0f, 0.0f, 0.0f), up, eye, lamp, 14.0f, colour * 2.0f, 0.0f, 16.0f);
    CHECK(Near(twice.diffuse.x, 2.0f * a, 1.0e-6f));
}

void TestShoulder()
{
    const float knee = 0.78f;
    // below the knee: as it is
    const float3 low = SoftShoulder(float3(0.7f, 0.4f, 0.1f), knee);
    CHECK(low.x == 0.7f && low.y == 0.4f && low.z == 0.1f);
    // continuous at the knee, and no kink: the slope above is 1
    const float3 at = SoftShoulder(float3(knee, 0.2f, 0.1f), knee);
    const float3 just = SoftShoulder(float3(knee + 0.001f, 0.2f, 0.1f), knee);
    CHECK(Near(at.x, knee, 1.0e-6f) && Near((just.x - at.x) / 0.001f, 1.0f, 0.01f));
    // only rising, never 1
    float last = 0.0f;
    bool rising = true;
    bool bounded = true;
    for (float v = 0.0f; v <= 50.0f; v += 0.05f)
    {
        const float s = SoftShoulder(float3(v, 0.0f, 0.0f), knee).x;
        rising = rising && s >= last - 1.0e-7f;
        bounded = bounded && s < 1.0f;
        last = s;
    }
    CHECK(rising && bounded);
    CHECK(SoftShoulder(float3(3.0f, 0.0f, 0.0f), knee).x > 0.97f); // a very bright light is nearly white, not cut
    // the three colours are scaled together: the hue stays
    const float3 hot = SoftShoulder(float3(3.0f, 1.5f, 0.6f), knee);
    CHECK(Near(hot.y / hot.x, 0.5f, 1.0e-5f) && Near(hot.z / hot.x, 0.2f, 1.0e-5f));
    // the highest colour decides (a blue light too)
    const float3 blue = SoftShoulder(float3(0.1f, 0.2f, 2.0f), knee);
    CHECK(blue.z < 1.0f && blue.z > 0.9f && Near(blue.x / blue.z, 0.05f, 1.0e-5f));
}

void TestSmallThings()
{
    CHECK(ShadowEdgeFade(0.5f, 0.5f) == 1.0f && ShadowEdgeFade(0.0f, 0.5f) == 0.0f &&
          ShadowEdgeFade(0.5f, 1.0f) == 0.0f);
    CHECK(Near(ShadowEdgeFade(0.05f, 0.5f), 0.5f, 1.0e-6f) && ShadowEdgeFade(0.2f, 0.2f) == 1.0f);
    CHECK(ShadowEdgeFade(-0.2f, 0.5f) == 0.0f && ShadowEdgeFade(0.5f, 1.4f) == 0.0f); // outside the map
    CHECK(HaloFalloff(1.0f) == 1.0f && HaloFalloff(0.0f) == 0.0f && HaloFalloff(-1.0f) == 1.0f &&
          HaloFalloff(0.5f) < HaloFalloff(0.8f));
    const float3 sun(1.0f, 0.8f, 0.5f);
    const float3 none = SunScatter(sun, -0.3f, 0.25f);
    const float3 full = SunScatter(sun, 1.0f, 0.25f);
    CHECK(none.x == 0.0f && Near(full.x, 0.25f, 1.0e-6f) && Near(full.y, 0.2f, 1.0e-6f));
    CHECK(SunScatter(sun, 0.5f, 0.25f).x < full.x && SunScatter(sun, 1.0f, 0.0f).x == 0.0f);
}

// ---- 2. the choice of lamps

dl::PointLight Lamp(float x, float z, float strength = 10.0f, unsigned layers = 3u, float range = 14.0f)
{
    dl::PointLight l;
    l.pos[0] = x;
    l.pos[1] = 3.4f;
    l.pos[2] = z;
    l.range = range;
    l.color[0] = l.color[1] = l.color[2] = strength;
    l.layers = layers;
    return l;
}

void TestLampFadeAndFlicker()
{
    CHECK(dl::LampFade(0.0f, 14.0f) == 1.0f && dl::LampFade(47.0f, 14.0f) == 1.0f &&
          dl::LampFade(72.0f, 14.0f) == 0.0f);
    float last = 1.0f;
    bool falling = true;
    for (float d = 0.0f; d < 100.0f; d += 1.0f)
    {
        const float f = dl::LampFade(d, 14.0f);
        falling = falling && f <= last + 1.0e-6f && f >= 0.0f && f <= 1.0f;
        last = f;
    }
    CHECK(falling);
    CHECK(dl::LampFade(59.0f, 38.0f) == 1.0f &&
          dl::LampFade(70.0f, 38.0f) > dl::LampFade(70.0f, 14.0f)); // a lighthouse is seen from farther
    // a flame is never still, never beyond its amount, and no two lamps flicker together
    float lo = 10.0f, hi = -10.0f;
    for (float t = 0.0f; t < 60.0f; t += 0.05f)
    {
        const float f = dl::LampFlicker(0.1f, 1.0f, t);
        lo = std::min(lo, f);
        hi = std::max(hi, f);
    }
    CHECK(lo >= 0.9f - 1.0e-5f && hi <= 1.1f + 1.0e-5f && hi - lo > 0.12f);
    CHECK(dl::LampFlicker(0.0f, 2.0f, 5.0f) == 1.0f &&
          std::fabs(dl::LampFlicker(0.1f, 0.0f, 3.0f) - dl::LampFlicker(0.1f, 2.0f, 3.0f)) > 1.0e-3f);
    CHECK(dl::LampFlicker(5.0f, 0.0f, 1.0f) <= 1.4f + 1.0e-6f); // the amount is limited
}

void TestSelect()
{
    const float eye[3] = {0.0f, 1.7f, 0.0f};
    dl::LampBatch out;
    // the moment: a lamp of A only is not in B
    std::vector<dl::PointLight> fixed = {Lamp(2.0f, 0.0f, 10.0f, 1u), Lamp(4.0f, 0.0f, 10.0f, 2u),
                                         Lamp(6.0f, 0.0f, 10.0f, 3u)};
    const std::vector<dl::PointLight> none;
    dl::SelectLamps(fixed, none, 0, eye, 1.0f, 1.0f, 0.0f, out);
    CHECK(out.count == 2 && out.pos[0][3] == 14.0f);
    dl::SelectLamps(fixed, none, 1, eye, 1.0f, 1.0f, 0.0f, out);
    CHECK(out.count == 2);
    // the scale of a group: 0 turns it off, 2 doubles it; the moving group has its own
    const std::vector<dl::PointLight> moving = {Lamp(1.0f, 1.0f, 4.0f)};
    dl::SelectLamps(fixed, moving, 1, eye, 0.0f, 1.0f, 0.0f, out);
    CHECK(out.count == 1 && Near(out.color[0][0], 4.0f, 1.0e-4f));
    dl::SelectLamps(fixed, moving, 1, eye, 2.0f, 0.5f, 0.0f, out);
    CHECK(out.count == 3);
    float biggest = 0.0f;
    float smallest = 1.0e9f;
    for (int i = 0; i < out.count; ++i)
    {
        biggest = std::max(biggest, out.color[i][0]);
        smallest = std::min(smallest, out.color[i][0]);
    }
    CHECK(Near(biggest, 20.0f, 1.0e-3f) && Near(smallest, 2.0f, 1.0e-3f));
    // a far lamp is gone, a near one stays
    fixed = {Lamp(300.0f, 0.0f), Lamp(3.0f, 0.0f)};
    dl::SelectLamps(fixed, none, 0, eye, 1.0f, 1.0f, 0.0f, out);
    CHECK(out.count == 1 && out.pos[0][0] == 3.0f);
    // too many: the cap, and the ones kept are the nearest of equal ones
    fixed.clear();
    for (int i = 0; i < 100; ++i)
    {
        fixed.push_back(Lamp(1.0f + static_cast<float>(i), 0.0f));
    }
    dl::SelectLamps(fixed, none, 0, eye, 1.0f, 1.0f, 0.0f, out);
    CHECK(out.count == dl::kMaxLamps);
    float farthest = 0.0f;
    for (int i = 0; i < out.count; ++i)
    {
        farthest = std::max(farthest, out.pos[i][0]);
    }
    CHECK(farthest <= 33.0f); // the 32 nearest of 100 in a row (x 1 .. 100)
    // the same twice gives the same (no blinking from a tie)
    dl::LampBatch again;
    dl::SelectLamps(fixed, none, 0, eye, 1.0f, 1.0f, 0.0f, again);
    bool same = again.count == out.count;
    for (int i = 0; i < out.count && same; ++i)
    {
        same = same && again.pos[i][0] == out.pos[i][0] && again.color[i][0] == out.color[i][0];
    }
    CHECK(same);
    // a strong lamp a little farther beats a weak one nearer
    fixed = {Lamp(2.0f, 0.0f, 1.0f)};
    const std::vector<dl::PointLight> strong = {Lamp(8.0f, 0.0f, 50.0f)};
    for (int i = 0; i < 40; ++i)
    {
        fixed.push_back(Lamp(2.0f + 0.01f * static_cast<float>(i), 1.0f, 1.0f));
    }
    dl::SelectLamps(fixed, strong, 0, eye, 1.0f, 1.0f, 0.0f, out);
    bool kept = false;
    for (int i = 0; i < out.count; ++i)
    {
        kept = kept || out.pos[i][0] == 8.0f;
    }
    CHECK(kept && out.count == dl::kMaxLamps);
    // a flicker is a factor of its amount, a bad phase does not matter
    dl::PointLight flame = Lamp(2.0f, 0.0f, 10.0f);
    flame.flicker = 0.2f;
    flame.phase = 1.3f;
    const std::vector<dl::PointLight> one = {flame};
    float lo = 100.0f, hi = 0.0f;
    for (float t = 0.0f; t < 30.0f; t += 0.1f)
    {
        dl::SelectLamps(one, none, 0, eye, 1.0f, 1.0f, t, out);
        lo = std::min(lo, out.color[0][0]);
        hi = std::max(hi, out.color[0][0]);
    }
    CHECK(lo >= 8.0f - 1.0e-3f && hi <= 12.0f + 1.0e-3f && hi - lo > 2.0f);
    // nothing to choose from; a lamp with no range
    dl::SelectLamps(none, none, 0, eye, 1.0f, 1.0f, 0.0f, out);
    CHECK(out.count == 0);
    const std::vector<dl::PointLight> dead = {Lamp(1.0f, 0.0f, 10.0f, 3u, 0.0f)};
    dl::SelectLamps(dead, none, 0, eye, 1.0f, 1.0f, 0.0f, out);
    CHECK(out.count == 0);
}

// ---- 3. the shadow box

void TestShadowFit()
{
    const float lightLength = std::sqrt(0.6f * 0.6f + 0.45f * 0.45f + 0.65f * 0.65f);
    const float toLight[3] = {-0.6f / lightLength, 0.45f / lightLength, -0.65f / lightLength};
    const float centre[3] = {10.0f, 2.0f, -30.0f};
    const dl::ShadowFit fit = dl::FitSunShadow(toLight, centre, 60.0f, 160.0f, 2048);
    CHECK(Near(fit.texelWorld, 120.0f / 2048.0f, 1.0e-6f) && Near(fit.depthRange, 320.0f, 1.0e-4f));
    // the centre of the box (moved to the grid by less than a texel) is in the middle of the map, half way along the
    // light
    float c[3];
    dl::ShadowClip(fit, fit.centre, c);
    CHECK(Near(c[0], 0.0f, 1.0e-4f) && Near(c[1], 0.0f, 1.0e-4f) && Near(c[2], 0.5f, 1.0e-4f));
    CHECK(std::fabs(fit.centre[0] - centre[0]) < 0.07f && std::fabs(fit.centre[2] - centre[2]) < 0.07f);
    // points inside the box are inside the map; a point toward the light is nearer to it (smaller depth)
    const float inside[3] = {centre[0] + 20.0f, centre[1] + 5.0f, centre[2] - 25.0f};
    float p[3];
    dl::ShadowClip(fit, inside, p);
    CHECK(std::fabs(p[0]) < 1.0f && std::fabs(p[1]) < 1.0f && p[2] > 0.0f && p[2] < 1.0f);
    const float towards[3] = {inside[0] + toLight[0] * 10.0f, inside[1] + toLight[1] * 10.0f,
                              inside[2] + toLight[2] * 10.0f};
    float q[3];
    dl::ShadowClip(fit, towards, q);
    CHECK(q[2] < p[2] && Near(p[2] - q[2], 10.0f / 320.0f, 1.0e-4f) && Near(q[0], p[0], 1.0e-4f) &&
          Near(q[1], p[1], 1.0e-4f));
    // the radius across the light is the edge of the map (the first column of the matrix is the right of the light over
    // the radius)
    const float rightEdge[3] = {
        fit.centre[0] + fit.viewProj[0] * 3600.0f, fit.centre[1] + fit.viewProj[4] * 3600.0f,
        fit.centre[2] + fit.viewProj[8] * 3600.0f}; // 3600 = 60 * 60: right = column * radius, then 60 m of it
    float e[3];
    dl::ShadowClip(fit, rightEdge, e);
    CHECK(Near(e[0], 1.0f, 1.0e-3f) && Near(e[1], 0.0f, 1.0e-3f));
    // the box moves on the texel grid: a walk of any length moves the map by whole texels (no crawling)
    bool onGrid = true;
    for (int i = 0; i < 40; ++i)
    {
        const float moved[3] = {centre[0] + 0.137f * static_cast<float>(i), centre[1],
                                centre[2] - 0.211f * static_cast<float>(i)};
        const dl::ShadowFit f = dl::FitSunShadow(toLight, moved, 60.0f, 160.0f, 2048);
        float a[3], b[3];
        dl::ShadowClip(f, centre, a);
        dl::ShadowClip(fit, centre, b);
        const float dx = (a[0] - b[0]) * 1024.0f; // texels (the map is 2048 across 2 clip units)
        const float dy = (a[1] - b[1]) * 1024.0f;
        onGrid = onGrid && Near(dx, std::round(dx), 0.02f) && Near(dy, std::round(dy), 0.02f);
    }
    CHECK(onGrid);
    // a light straight overhead, from straight below, with no direction or a bad one: a box that still holds the centre
    const float overhead[3] = {0.0f, 1.0f, 0.0f};
    const float zero[3] = {0.0f, 0.0f, 0.0f};
    const float nan[3] = {std::nanf(""), 1.0f, 0.0f};
    for (const float* dir : {overhead, zero, nan})
    {
        const dl::ShadowFit f = dl::FitSunShadow(dir, centre, 60.0f, 160.0f, 2048);
        float k[3];
        dl::ShadowClip(f, f.centre, k);
        bool finite = true;
        for (const float v : f.viewProj)
        {
            finite = finite && std::isfinite(v);
        }
        CHECK(finite && Near(k[0], 0.0f, 1.0e-4f) && Near(k[2], 0.5f, 1.0e-4f));
    }
    const dl::ShadowFit silly = dl::FitSunShadow(toLight, centre, -5.0f, 0.0f, 3);
    CHECK(silly.texelWorld > 0.0f && std::isfinite(silly.depthRange) && silly.depthRange >= 2.0f);
}

// ---- 3b. the shadow chain on a toy scene: the matrix, a depth map drawn by a small software rasteriser, the lookup of
// shaders/models.hlsl ShadowAt

struct Tri
{
    float a[3], b[3], c[3];
};

void AddQuad(std::vector<Tri>& tris, const float p0[3], const float p1[3], const float p2[3], const float p3[3])
{
    tris.push_back({{p0[0], p0[1], p0[2]}, {p1[0], p1[1], p1[2]}, {p2[0], p2[1], p2[2]}});
    tris.push_back({{p0[0], p0[1], p0[2]}, {p2[0], p2[1], p2[2]}, {p3[0], p3[1], p3[2]}});
}

void AddBox(std::vector<Tri>& tris, float x0, float x1, float y0, float y1, float z0, float z1)
{
    const float v[8][3] = {{x0, y0, z0}, {x1, y0, z0}, {x1, y0, z1}, {x0, y0, z1},
                           {x0, y1, z0}, {x1, y1, z0}, {x1, y1, z1}, {x0, y1, z1}};
    const int q[6][4] = {{0, 1, 2, 3}, {4, 5, 6, 7}, {0, 1, 5, 4}, {1, 2, 6, 5}, {2, 3, 7, 6}, {3, 0, 4, 7}};
    for (const auto& f : q)
    {
        AddQuad(tris, v[f[0]], v[f[1]], v[f[2]], v[f[3]]);
    }
}

/// The depth map of `tris` seen through `fit`: what the shadow pass makes (one depth per texel, nearest to the light
/// wins).
std::vector<float> DrawDepth(const dl::ShadowFit& fit, const std::vector<Tri>& tris, int size)
{
    std::vector<float> depth(static_cast<std::size_t>(size) * static_cast<std::size_t>(size), 1.0f);
    for (const Tri& t : tris)
    {
        float c[3][3];
        dl::ShadowClip(fit, t.a, c[0]);
        dl::ShadowClip(fit, t.b, c[1]);
        dl::ShadowClip(fit, t.c, c[2]);
        float px[3], py[3];
        for (int k = 0; k < 3; ++k)
        {
            px[k] = (c[k][0] * 0.5f + 0.5f) *
                    static_cast<float>(size); // the same map of the clip to the texture as ShadowAt
            py[k] = (-c[k][1] * 0.5f + 0.5f) * static_cast<float>(size);
        }
        const float area = (px[1] - px[0]) * (py[2] - py[0]) - (px[2] - px[0]) * (py[1] - py[0]);
        if (std::fabs(area) < 1.0e-9f)
        {
            continue;
        }
        const int x0 = std::max(0, static_cast<int>(std::floor(std::min({px[0], px[1], px[2]}))));
        const int x1 = std::min(size - 1, static_cast<int>(std::ceil(std::max({px[0], px[1], px[2]}))));
        const int y0 = std::max(0, static_cast<int>(std::floor(std::min({py[0], py[1], py[2]}))));
        const int y1 = std::min(size - 1, static_cast<int>(std::ceil(std::max({py[0], py[1], py[2]}))));
        for (int y = y0; y <= y1; ++y)
        {
            for (int x = x0; x <= x1; ++x)
            {
                const float fx = static_cast<float>(x) + 0.5f;
                const float fy = static_cast<float>(y) + 0.5f;
                const float w1 = ((fx - px[0]) * (py[2] - py[0]) - (px[2] - px[0]) * (fy - py[0])) / area;
                const float w2 = ((px[1] - px[0]) * (fy - py[0]) - (fx - px[0]) * (py[1] - py[0])) / area;
                const float w0 = 1.0f - w1 - w2;
                if (w0 < 0.0f || w1 < 0.0f || w2 < 0.0f)
                {
                    continue;
                }
                const float z = w0 * c[0][2] + w1 * c[1][2] + w2 * c[2][2];
                float& d =
                    depth[static_cast<std::size_t>(y) * static_cast<std::size_t>(size) + static_cast<std::size_t>(x)];
                d = std::min(d, std::max(z, 0.0f)); // a caster in front of the near plane is clamped to 0, as the pass
                                                    // does (no depth clip)
            }
        }
    }
    return depth;
}

/// shaders/models.hlsl ShadowAt for a surface at `pos` with the unit normal `n` and the unit direction to the light
/// `l`: 1 lit, 0 in the shadow (strength 1; 3 x 3 taps of the nearest texel instead of the bilinear comparison: a
/// little harder than the GPU).
float ShadowLookup(const dl::ShadowFit& fit, const std::vector<float>& depth, int size, const dl::LightingSettings& set,
                   const float pos[3], const float n[3], const float l[3])
{
    const float ndl = std::max(0.0f, n[0] * l[0] + n[1] * l[1] + n[2] * l[2]);
    const float offset = set.shadowNormal * fit.texelWorld * (1.5f - ndl);
    const float p[3] = {pos[0] + n[0] * offset, pos[1] + n[1] * offset, pos[2] + n[2] * offset};
    float c[3];
    dl::ShadowClip(fit, p, c);
    const float u = c[0] * 0.5f + 0.5f;
    const float v = -c[1] * 0.5f + 0.5f;
    if (c[2] >= 1.0f || c[2] <= 0.0f || ShadowEdgeFade(u, v) <= 0.0f)
    {
        return 1.0f;
    }
    const float z = c[2] - set.shadowBias * fit.texelWorld / fit.depthRange;
    float sum = 0.0f;
    for (int j = -1; j <= 1; ++j)
    {
        for (int i = -1; i <= 1; ++i)
        {
            const int tx = static_cast<int>(std::floor(u * static_cast<float>(size))) + i;
            const int ty = static_cast<int>(std::floor(v * static_cast<float>(size))) + j;
            const float stored = (tx < 0 || ty < 0 || tx >= size || ty >= size)
                                     ? 1.0f
                                     : depth[static_cast<std::size_t>(ty) * static_cast<std::size_t>(size) +
                                             static_cast<std::size_t>(tx)];
            sum += z <= stored ? 1.0f : 0.0f;
        }
    }
    return sum / 9.0f;
}

/// True when the ray from `p` toward the light `l` meets the box (a slab test).
bool RayHitsBox(const float p[3], const float l[3], const float lo[3], const float hi[3])
{
    float t0 = 0.0f;
    float t1 = 1.0e9f;
    for (int k = 0; k < 3; ++k)
    {
        if (std::fabs(l[k]) < 1.0e-9f)
        {
            if (p[k] < lo[k] || p[k] > hi[k])
            {
                return false;
            }
            continue;
        }
        float a = (lo[k] - p[k]) / l[k];
        float b = (hi[k] - p[k]) / l[k];
        if (a > b)
        {
            std::swap(a, b);
        }
        t0 = std::max(t0, a);
        t1 = std::min(t1, b);
    }
    return t0 < t1 - 1.0e-4f;
}

void TestShadowChain()
{
    const int size = 1024;
    const dl::LightingSettings set; // the defaults: what the game uses
    const float len = std::sqrt(0.6f * 0.6f + 0.45f * 0.45f + 0.65f * 0.65f);
    const float l[3] = {-0.6f / len, 0.45f / len, -0.65f / len}; // the sun of A: low, in the south-west
    const float centre[3] = {0.0f, 0.0f, 0.0f};
    const dl::ShadowFit fit = dl::FitSunShadow(l, centre, 30.0f, 160.0f, size);
    // ground and a box of 4 m
    std::vector<Tri> scene;
    const float g0[3] = {-45.0f, 0.0f, -45.0f}, g1[3] = {45.0f, 0.0f, -45.0f}, g2[3] = {45.0f, 0.0f, 45.0f},
                g3[3] = {-45.0f, 0.0f, 45.0f};
    AddQuad(scene, g0, g1, g2, g3);
    AddBox(scene, -2.0f, 2.0f, 0.0f, 4.0f, -2.0f, 2.0f);
    const std::vector<float> depth = DrawDepth(fit, scene, size);
    const float lo[3] = {-2.0f, 0.0f, -2.0f};
    const float hi[3] = {2.0f, 4.0f, 2.0f};
    const float up[3] = {0.0f, 1.0f, 0.0f};
    int checked = 0, wrongLit = 0, wrongShadow = 0, shadowed = 0;
    for (float x = -22.0f; x <= 22.0f; x += 0.37f)
    {
        for (float z = -22.0f; z <= 22.0f; z += 0.37f)
        {
            const float p[3] = {x, 0.0f, z};
            // only where a small step in any direction does not change the answer (the edge of a shadow is soft)
            bool agree = true;
            const bool hit = RayHitsBox(p, l, lo, hi);
            for (const float d : {0.35f, -0.35f})
            {
                const float a[3] = {x + d, 0.0f, z};
                const float b[3] = {x, 0.0f, z + d};
                agree = agree && RayHitsBox(a, l, lo, hi) == hit && RayHitsBox(b, l, lo, hi) == hit;
            }
            const bool onBox =
                std::fabs(x) < 2.4f && std::fabs(z) < 2.4f; // under or beside the box: its own contact edge
            if (!agree || onBox)
            {
                continue;
            }
            ++checked;
            const float lit = ShadowLookup(fit, depth, size, set, p, up, l);
            shadowed += hit ? 1 : 0;
            wrongLit += (!hit && lit < 0.99f) ? 1 : 0;  // acne: a lit point that reads shadowed
            wrongShadow += (hit && lit > 0.2f) ? 1 : 0; // a shadow that is missing, or eaten by the bias
        }
    }
    std::printf("  shadow chain: %d ground points checked, %d in the shadow of the box; speckled %d, missing %d\n",
                checked, shadowed, wrongLit, wrongShadow);
    CHECK(checked > 1000 && shadowed > 40 && wrongLit == 0 && wrongShadow == 0);
    // the top of the box is lit (it faces the light), and a wall of it that faces away is in its own shadow
    const float top[3] = {0.5f, 4.0f, 0.3f};
    CHECK(ShadowLookup(fit, depth, size, set, top, up, l) > 0.99f);
    const float away[3] = {2.0f, 2.0f, 0.5f}; // the east face: the sun is in the west
    const float east[3] = {1.0f, 0.0f, 0.0f};
    const float faceLit = ShadowLookup(fit, depth, size, set, away, east, l);
    CHECK(faceLit >= 0.0f && faceLit <= 1.0f);
}

void TestShadowGrazing()
{
    // a plane that the light grazes (the cosine 0.25) with nothing above it: the whole plane must be lit, no speckle at
    // any place
    const int size = 1024;
    const dl::LightingSettings set;
    const float len = std::sqrt(0.6f * 0.6f + 0.45f * 0.45f + 0.65f * 0.65f);
    const float l[3] = {-0.6f / len, 0.45f / len, -0.65f / len};
    float w[3] = {l[2] * 1.0f, 0.0f, -l[0]}; // perpendicular to l and horizontal
    const float wl = std::sqrt(w[0] * w[0] + w[2] * w[2]);
    w[0] /= wl;
    w[2] /= wl;
    const float k = std::sqrt(1.0f - 0.25f * 0.25f);
    const float n[3] = {l[0] * 0.25f + w[0] * k, l[1] * 0.25f + w[1] * k, l[2] * 0.25f + w[2] * k};
    const float s[3] = {n[1] * w[2] - n[2] * w[1], n[2] * w[0] - n[0] * w[2],
                        n[0] * w[1] - n[1] * w[0]}; // n x w: the other direction in the plane
    const float centre[3] = {0.0f, 0.0f, 0.0f};
    const dl::ShadowFit fit = dl::FitSunShadow(l, centre, 30.0f, 160.0f, size);
    std::vector<Tri> scene;
    const float big = 60.0f;
    const float p0[3] = {-big * w[0] - big * s[0], -big * w[1] - big * s[1], -big * w[2] - big * s[2]};
    const float p1[3] = {big * w[0] - big * s[0], big * w[1] - big * s[1], big * w[2] - big * s[2]};
    const float p2[3] = {big * w[0] + big * s[0], big * w[1] + big * s[1], big * w[2] + big * s[2]};
    const float p3[3] = {-big * w[0] + big * s[0], -big * w[1] + big * s[1], -big * w[2] + big * s[2]};
    AddQuad(scene, p0, p1, p2, p3);
    const std::vector<float> depth = DrawDepth(fit, scene, size);
    int bad = 0, count = 0;
    for (float a = -18.0f; a <= 18.0f; a += 0.41f)
    {
        for (float b = -18.0f; b <= 18.0f; b += 0.41f)
        {
            const float p[3] = {a * w[0] + b * s[0], a * w[1] + b * s[1], a * w[2] + b * s[2]};
            ++count;
            bad += ShadowLookup(fit, depth, size, set, p, n, l) < 0.99f ? 1 : 0;
        }
    }
    std::printf("  shadow at a grazing angle (cosine 0.25): %d of %d points speckled\n", bad, count);
    CHECK(count > 1000 && bad == 0);
}

// ---- 4. the settings

void TestSettings()
{
    const dl::LightingSettings d;
    CHECK(d.shadows && d.shadowSize == 2048 && d.pointLights && d.halos && d.sunB < d.sunA + 1.0f);
    dl::LightingSettings bad;
    bad.shadowSize = 3000;
    bad.shadowRadius = std::nanf("");
    bad.shadowStrength = 7.0f;
    bad.exposure = -3.0f;
    bad.knee = 1.5f;
    bad.lampsB = 1.0e9f;
    bad.haloA = -2.0f;
    const dl::LightingSettings fixed = dl::Sanitize(bad);
    CHECK(fixed.shadowSize == 2048 && fixed.shadowRadius == d.shadowRadius && fixed.shadowStrength == 1.0f &&
          fixed.exposure == 0.1f);
    CHECK(fixed.knee == 0.98f && fixed.lampsB == 8.0f && fixed.haloA == 0.0f);
    CHECK(dl::LightingShadowSize(0) == 2048 && dl::LightingShadowSize(-5) == 2048 &&
          dl::LightingShadowSize(100) == 512 && dl::LightingShadowSize(512) == 512 &&
          dl::LightingShadowSize(1500) == 1024 && dl::LightingShadowSize(4096) == 4096 &&
          dl::LightingShadowSize(99999) == 4096);
    dl::Config c;
    c.ParseString("[lighting]\nshadows = false\nshadow_size = 1024\nlamps_b = 2.5   # a brighter night\nexposure = "
                  "1.2\nhalos = off\nknee = abc\n");
    const dl::LightingSettings r = dl::ReadLightingSettings(c);
    CHECK(!r.shadows && r.shadowSize == 1024 && r.lampsB == 2.5f && r.exposure == 1.2f && !r.halos &&
          r.knee == d.knee && r.lampsA == d.lampsA && r.pointLights);
    dl::Config empty;
    const dl::LightingSettings none = dl::ReadLightingSettings(empty);
    CHECK(none.shadows == d.shadows && none.exposure == d.exposure && none.shadowSize == d.shadowSize);
}

// ---- 5. the town

const dl::town::TownData& Town()
{
    static dl::town::TownData data;
    static bool loaded = false;
    if (!loaded)
    {
        std::string error;
        loaded = data.Load(dl::town::kPath, error);
        if (!loaded)
        {
            std::printf("  cannot load %s: %s\n", dl::town::kPath, error.c_str());
        }
    }
    return data;
}

void TestTownLamps()
{
    namespace tn = dl::town;
    const tn::TownData& t = Town();
    int kinds[5] = {};
    int inA = 0, inB = 0, both = 0;
    bool sane = true;
    for (const tn::Lamp& l : t.lamps)
    {
        kinds[static_cast<int>(l.kind)]++;
        inA += (l.worlds & 1) != 0 ? 1 : 0;
        inB += (l.worlds & 2) != 0 ? 1 : 0;
        both += l.worlds == 3 ? 1 : 0;
        sane = sane && l.range >= 5.0f && l.range <= 60.0f && l.intensity > 0.0f && l.flicker >= 0.0f &&
               l.flicker <= 0.4f && l.at.y > -10.0f && l.at.y < 80.0f && l.color[0] >= 0.0f && l.color[0] <= 1.0f;
    }
    std::printf("  lamps: %zu (street %d, post %d, light %d, memory %d, grave %d); in A %d, in B %d, in both %d\n",
                t.lamps.size(), kinds[0], kinds[1], kinds[2], kinds[3], kinds[4], inA, inB, both);
    CHECK(t.lamps.size() > 40 && kinds[0] > 30 && kinds[1] == 2 && kinds[2] == 2 && kinds[3] == 9 && kinds[4] == 1 &&
          sane);
    CHECK(inA > inB && inB >= 10 && both >= 10); // the evening has every lamp lit, the ruin only some
    // to point lights: colour linear times strength, the moments, a halo by kind, a phase of its own
    const std::vector<dl::PointLight> lights = tn::TownLamps(t);
    CHECK(lights.size() == t.lamps.size());
    bool same = true;
    bool phases = false;
    for (std::size_t i = 0; i < lights.size(); ++i)
    {
        const tn::Lamp& l = t.lamps[i];
        const float lin = dl::batch_detail::SrgbToLinear(l.color[1]) * l.intensity;
        same = same && Near(lights[i].color[1], lin, 1.0e-3f * (1.0f + lin)) && lights[i].layers == l.worlds &&
               lights[i].range == l.range && lights[i].pos[1] == l.at.y && lights[i].halo == tn::HaloOfKind(l.kind) &&
               lights[i].halo > 0.0f;
        phases = phases || (i > 0 && lights[i].phase != lights[i - 1].phase);
    }
    CHECK(same && phases);
    // what a street lamp does on the ground under it, through the shader maths: a warm pool that the shoulder keeps
    // under 1
    const tn::Lamp* street = nullptr;
    for (const tn::Lamp& l : t.lamps)
    {
        street = street == nullptr && l.kind == tn::LampKind::Street && l.worlds == 3 ? &l : street;
    }
    CHECK(street != nullptr);
    if (street != nullptr)
    {
        const dl::PointLight pl = tn::TownLamps(t)[static_cast<std::size_t>(street - t.lamps.data())];
        const float3 under(street->at.x, street->at.y - 3.4f, street->at.z);
        const LampLight l = LampShade(under, float3(0.0f, 1.0f, 0.0f), float3(0.0f, 1.0f, 0.0f),
                                      float3(street->at.x, street->at.y, street->at.z), pl.range,
                                      float3(pl.color[0], pl.color[1], pl.color[2]), 0.0f, 16.0f);
        const float3 stone(0.3f, 0.3f, 0.3f);
        const float3 shown =
            SoftShoulder(float3(stone.x * l.diffuse.x, stone.y * l.diffuse.y, stone.z * l.diffuse.z), 0.78f);
        CHECK(l.diffuse.x > 1.0f && l.diffuse.x < 2.0f && l.diffuse.x > l.diffuse.y &&
              l.diffuse.y > l.diffuse.z); // warm: red over green over blue
        CHECK(shown.x < 1.0f && shown.x > 0.2f);
    }
}

void TestMovingLamps()
{
    namespace tn = dl::town;
    const tn::TownData& t = Town();
    tn::Things things;
    things.Init(t);
    tn::Lantern lantern;
    std::vector<dl::PointLight> out;
    const dl::Vec3 hand{5.0f, 1.2f, 7.0f};
    tn::MovingLamps(t, things, lantern, hand, out);
    CHECK(out.size() == 1 && out[0].pos[0] == 5.0f && out[0].pos[1] == 1.2f && out[0].pos[2] == 7.0f &&
          out[0].layers == 3u);
    CHECK(out[0].range == tn::kLanternLightRange && out[0].flicker > 0.0f && out[0].halo > 0.0f);
    // set down: the light is where the lantern stands, a little over its foot
    dl::town::Moments moments;
    dl::town::TriMesh a, b;
    a.Add({-5.0f, 0.0f, -5.0f}, {-5.0f, 0.0f, 5.0f}, {5.0f, 0.0f, 5.0f});
    a.Add({-5.0f, 0.0f, -5.0f}, {5.0f, 0.0f, 5.0f}, {5.0f, 0.0f, -5.0f});
    a.Build();
    b.Build();
    moments = dl::town::Moments(&a, &b);
    CHECK(lantern.Use({0.0f, 0.0f, 0.0f}, true, {0.0f, 0.0f, 1.0f}, 0, 0.35f, moments) == tn::Lantern::Result::Put);
    tn::MovingLamps(t, things, lantern, hand, out);
    CHECK(out.size() == 1 && Near(out[0].pos[0], lantern.At().x, 1.0e-5f) && out[0].pos[1] > lantern.At().y + 0.3f);
    // a bell rung lights a lamp of the mast and the lamp over its rope (B)
    const std::size_t lamps = out.size();
    int rung = 0;
    for (const tn::Emblem& e : t.emblems)
    {
        std::size_t bell = 0;
        for (std::size_t i = 0; i < t.things.size(); ++i)
        {
            bell = t.things[i].name == e.bell ? i : bell;
        }
        const tn::ThingDef& def = t.things[bell];
        const dl::Vec3 feet{def.handle.x, def.handle.y - 1.1f, def.handle.z};
        const dl::Vec3 balls[1] = {{0.0f, -100.0f, 0.0f}};
        tn::Traces traces;
        CHECK(things.Use(feet, balls, 1, 0.35f, 0, moments, traces) == tn::Things::Result::Opened);
        ++rung;
        tn::MovingLamps(t, things, lantern, hand, out);
        CHECK(out.size() == lamps + 2 * static_cast<std::size_t>(rung)); // the mast and the rope
    }
    bool mast = false;
    bool rope = false;
    for (const dl::PointLight& l : out)
    {
        mast = mast || (l.range == tn::kEmblemLightRange && l.pos[1] > 50.0f);
        rope = rope || (l.range == tn::kBellLightRange && l.layers == 2u);
    }
    CHECK(mast && rope);
}

void TestHalos()
{
    namespace tn = dl::town;
    const tn::TownData& t = Town();
    const std::vector<dl::PointLight> lamps = tn::TownLamps(t);
    const std::vector<dl::PointLight> none;
    dl::scene::SceneObject table[200] = {};
    // near the first lamp of the quay: halos for the lamps within reach, once for each moment they shine in
    const dl::PointLight& first = lamps.front();
    const float eye[3] = {first.pos[0], first.pos[1], first.pos[2]};
    const int n = tn::AddHalos(lamps, none, eye, 0.35f, 1.0f, 0.0f, table, 0, 200);
    CHECK(n > 4 && n <= 2 * tn::kMaxHalos);
    float inA = 0.0f, inB = 0.0f;
    int a = 0, b = 0;
    bool right = true;
    for (int i = 0; i < n; ++i)
    {
        const dl::scene::SceneObject& o = table[i];
        right = right && o.shape == dl::scene::ShapeKind::Sphere && dl::scene::IsHalo(o.checker) &&
                !dl::scene::IsWaterMaterial(o.checker) && o.glow > 0.0f && o.scale[0] > 1.0f &&
                (o.layerMask == dl::scene::kLayerA || o.layerMask == dl::scene::kLayerB) && o.colorSrgb[0] <= 1.0f &&
                o.colorSrgb[0] >= 0.0f;
        if (o.layerMask == dl::scene::kLayerA)
        {
            inA = std::max(inA, o.glow);
            ++a;
        }
        else
        {
            inB = std::max(inB, o.glow);
            ++b;
        }
    }
    CHECK(right && a > 0 && b > 0 && inA < inB);                  // weaker in A, where the sun hides a lamp
    CHECK(std::fabs(table[0].position[0] - first.pos[0]) < 1.0f); // the nearest first
    // the strength of a moment of 0 turns its halos off; a full table keeps what it has; far away: none
    const int onlyB = tn::AddHalos(lamps, none, eye, 0.0f, 1.0f, 0.0f, table, 0, 200);
    CHECK(onlyB > 0 && onlyB < n);
    CHECK(tn::AddHalos(lamps, none, eye, 0.35f, 1.0f, 0.0f, table, 0, 3) == 3);
    CHECK(tn::AddHalos(lamps, none, eye, 0.35f, 1.0f, 0.0f, table, 5, 5) == 5 &&
          tn::AddHalos(lamps, none, eye, 0.35f, 1.0f, 0.0f, nullptr, 0, 5) == 0);
    const float far[3] = {1000.0f, 0.0f, 1000.0f};
    CHECK(tn::AddHalos(lamps, none, far, 0.35f, 1.0f, 0.0f, table, 0, 200) == 0);
    // the halo of a lamp that flickers follows it
    dl::PointLight flame = lamps.front();
    flame.flicker = 0.2f;
    flame.layers = 3u;
    const std::vector<dl::PointLight> one = {flame};
    float lo = 100.0f, hi = 0.0f;
    for (float s = 0.0f; s < 20.0f; s += 0.1f)
    {
        tn::AddHalos(one, none, eye, 0.0f, 1.0f, s, table, 0, 200);
        lo = std::min(lo, table[0].glow);
        hi = std::max(hi, table[0].glow);
    }
    CHECK(hi - lo > 0.1f);
}

} // namespace

int main()
{
    TestAttenuation();
    TestLampShade();
    TestShoulder();
    TestSmallThings();
    TestLampFadeAndFlicker();
    TestSelect();
    TestShadowFit();
    TestShadowChain();
    TestShadowGrazing();
    TestSettings();
    TestTownLamps();
    TestMovingLamps();
    TestHalos();
    std::printf("%d checks passed, %d failed\n", g_passed, g_failed);
    return g_failed == 0 ? 0 : 1;
}
