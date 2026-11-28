// rain.hlsli - the maths of the rain of B (docs/DESIGN.md 6.11, stage R1): ink streaks in three layers, small rings where drops fall on
//              the ground, the dry circle of the lantern, and the wet look of stone and sky. No texture and no constant buffer.
// Why:         the rain is drawn as a comic draws it, in the line of 6.10: thin pale strokes and small rings, not particles. A pass over the
//              depth of the finished picture (shaders/rain.hlsl) calls these functions; the wet look is read by shaders/models.hlsl.
// Streaks:     three layers (near, middle, far), each a grid of cells in "rain space": u across the fall, v along it (pixels). A cell holds
//              at most one streak; the grid scrolls along v with the time, so a streak falls without ever popping. A layer stands at a
//              distance in the world: where the surface behind is nearer than that, the streak is behind it and is not drawn.
// Rings:       cells of the ground plane (metres); a ring grows from a point of its cell and fades as it grows.
// Plain HLSL that is also C++ through tests/hlsl_shim.h: only scalars and float3, no float2, no ddx, no texture (the tests of
//              tests/test_style.cpp, TestRain, run these functions). The names `near` and `far` are not used (windows.h defines them).
#ifndef DL_RAIN_HLSLI
#define DL_RAIN_HLSLI

// A number 0 .. 1 that differs from cell to cell, and a second one that does not follow it. Made of fractions and products only (no sine
// of a big number: its last bits are the number, and a GPU and a CPU do not agree on them), so that it is the same on any machine
// within a rounding and good for cell indices up to a thousand.
float RainHash(float a, float b)
{
    float x = frac(a * 0.1031f);
    float y = frac(b * 0.1030f);
    float z = frac(a * 0.0973f);
    const float d = x * (y + 33.33f) + y * (z + 33.33f) + z * (x + 33.33f);
    x += d;
    y += d;
    z += d;
    return frac((x + y) * z);
}

float RainHash2(float a, float b)
{
    float x = frac(a * 0.1099f);
    float y = frac(b * 0.1060f);
    float z = frac(a * 0.0937f + b * 0.0371f);
    const float d = x * (y + 41.77f) + y * (z + 41.77f) + z * (x + 41.77f);
    x += d;
    y += d;
    z += d;
    return frac((x + y) * z);
}

// A cell index kept small (0 .. 1023): the hash of a large number loses its bits.
float RainWrap(float i)
{
    return i - 1024.0f * floor(i / 1024.0f);
}

// One layer of streaks at one pixel. (u, v): the pixel in rain space, in pixels (v runs along the fall); cellW, cellH: the size of a cell;
// density 0 .. 1: the share of cells that hold a streak; speed: pixels per second along v; time: seconds; length: the part of a cell that a
// streak covers (0 .. 1); width: pixels (at most half a cell less one); seed: tells the layers apart. Returns the coverage 0 .. 1: the
// tail is faint, the head is full and ends softly.
float RainStreak(float u, float v, float cellW, float cellH, float density, float speed, float time, float length, float width,
                 float seed)
{
    const float vv = v - time * speed;
    const float col = floor(u / cellW);
    const float row = floor(vv / cellH);
    const float c = RainWrap(col) + seed * 17.0f;
    const float r = RainWrap(row);
    if (RainHash(c, r) >= density)
    {
        return 0.0f;
    }
    const float centre = (0.25f + 0.5f * RainHash2(c, r)) * cellW;
    const float across = saturate(0.5f * width + 0.5f - abs((u - col * cellW) - centre));
    const float along = (vv - row * cellH) / cellH;
    const float start = RainHash2(r + 13.0f, c) * (1.0f - length);
    const float t = (along - start) / length;
    if (t < 0.0f || t > 1.0f)
    {
        return 0.0f;
    }
    return across * (0.12f + 0.88f * t) * (1.0f - smoothstep(0.90f, 1.0f, t));
}

// The distance of the streak of the cell at (u, v), as a part of the distance of its layer: each streak stands at a distance of its own, between
// 0.4 and 1.6 of the layer's, so that the rain is in front of the near ground as well as of the far one (a layer on a plane would show
// only where the surface behind it is farther than the plane). The same for every pixel of a cell and for every moment.
float RainDepthFactor(float u, float v, float cellW, float cellH, float speed, float time, float seed)
{
    const float c = RainWrap(floor(u / cellW)) + seed * 17.0f;
    const float r = RainWrap(floor((v - time * speed) / cellH));
    return 0.4f + 1.2f * RainHash(r + 29.0f, c + 3.0f);
}

// How much of a layer at `layerDepth` metres is in front of the surface at `sceneDepth` metres (both depths of view): 1 where the surface is
// farther (the sky, the sea, a far wall), 0 where it is nearer (the layer is behind it), a short ramp between.
float RainVisible(float sceneDepth, float layerDepth)
{
    return smoothstep(layerDepth, layerDepth * 1.15f, sceneDepth);
}

// The dry circle of the lantern (3.2): 0 inside the sphere of `radius` round (cx, cy, cz), 1 outside it, a soft edge of `soft` metres.
// A radius of 0 or less is no circle.
float RainDry(float px, float py, float pz, float cx, float cy, float cz, float radius, float soft)
{
    if (radius <= 0.0f)
    {
        return 1.0f;
    }
    const float dx = px - cx;
    const float dy = py - cy;
    const float dz = pz - cz;
    return smoothstep(radius, radius + max(soft, 0.01f), sqrt(dx * dx + dy * dy + dz * dz));
}

// What one layer adds at a pixel: its streak where it is in front of the surface and out of the dry circle, with the strength of the layer.
float RainLayerCoverage(float streak, float sceneDepth, float layerDepth, float dry, float alpha)
{
    return saturate(streak * RainVisible(sceneDepth, layerDepth) * dry * alpha);
}

// A face is ground when its normal, turned to the eye, is within about 30 degrees of straight up (`cosUp`: the cosine to the up direction).
float RainGround(float cosUp)
{
    return smoothstep(0.70f, 0.90f, cosUp);
}

// How far the rings reach: all of them to 0.55 of `reach` metres, none from `reach`.
float RainRingReach(float metres, float reach)
{
    return 1.0f - smoothstep(0.55f * reach, reach, max(metres, 0.0f));
}

// The width of the line of a ring, in metres: thin near, never thinner than a pixel is far (`slope`: the size of a pixel at one metre).
float RainRingWidth(float metres, float slope)
{
    return max(0.018f, 1.1f * metres * slope);
}

// The rings on the ground plane at (x, z) metres: cells of `cell` metres, `density` 0 .. 1 of them with a ring; a ring grows from a point
// of its cell to `maxRadius` metres in 1 / `rate` seconds and fades as it grows. The cells round the pixel's are looked at as well, so
// a ring that crosses the edge of its cell is whole (`maxRadius` is less than `cell`). Returns the coverage 0 .. 1.
float RainRing(float x, float z, float time, float density, float cell, float maxRadius, float lineWidth, float rate)
{
    const float cx = floor(x / cell);
    const float cz = floor(z / cell);
    float best = 0.0f;
    for (int j = -1; j <= 1; ++j)
    {
        for (int i = -1; i <= 1; ++i)
        {
            const float gx = RainWrap(cx + float(i));
            const float gz = RainWrap(cz + float(j));
            if (RainHash(gx, gz) >= density)
            {
                continue;
            }
            const float centreX = (cx + float(i) + RainHash2(gx, gz)) * cell;
            const float centreZ = (cz + float(j) + RainHash2(gz + 7.0f, gx)) * cell;
            const float phase = frac(time * rate + RainHash(gz + 3.0f, gx + 5.0f));
            const float dx = x - centreX;
            const float dz = z - centreZ;
            const float distance = sqrt(dx * dx + dz * dz);
            const float ring = saturate(1.0f - abs(distance - phase * maxRadius) / max(lineWidth, 0.001f));
            best = max(best, ring * (1.0f - phase));
        }
    }
    return best;
}

// The layers and the rings together: each is a coverage, and they are laid one over the other (never more than 1, never less than the largest).
float RainCombine(float near0, float mid, float far0, float ring)
{
    return 1.0f - (1.0f - saturate(near0)) * (1.0f - saturate(mid)) * (1.0f - saturate(far0)) * (1.0f - saturate(ring));
}

// ---- the wet look ----------------------------------------------------------------------------------------------------------------------
// How wet a face is: `wet` (0 .. 1) on a face that looks up, 0.35 of it on a wall. (`ny`: the y of its normal.)
float RainWetness(float ny, float wet)
{
    return wet * (0.35f + 0.65f * saturate(ny));
}

// Wet stone is darker: its colour is multiplied by 0.82 at full wetness. The hue is kept.
float3 RainDarken(float3 albedo, float wetness)
{
    return albedo * lerp(1.0f, 0.82f, saturate(wetness));
}

// A wet face reflects the sky: a twentieth of the sky light is added to it, whatever its colour is, so that a shadow is not lost in the rain
// (more than this lifts the deepest dark of B out of the numbers of 6.10).
float3 RainReflect(float3 skyLight, float wetness)
{
    return skyLight * (0.05f * saturate(wetness));
}

// The highlight of a wet face: a little shine where the surface had none, and a sharper one.
float RainSheenStrength(float shine, float wetness)
{
    return max(shine, 0.45f * saturate(wetness));
}

float RainSheenPower(float power, float wetness)
{
    return lerp(power, max(power, 28.0f), saturate(wetness));
}

// The sky under cloud: its colour leans toward the grey of its own brightness, a little cool and a little darker (`overcast`: 0 .. 1).
float3 RainOvercast(float3 sky, float overcast)
{
    const float grey = dot(sky, float3(0.2126f, 0.7152f, 0.0722f));
    const float3 flat = float3(grey * 0.94f, grey * 0.98f, grey * 1.06f);
    return lerp(sky, flat, saturate(overcast) * 0.65f);
}

// The moon behind cloud is a soft glow and not a disc: the sharp core goes, the broad glow grows.
float RainMoonCore(float overcast)
{
    return 1.0f - saturate(overcast);
}

float RainMoonGlow(float overcast)
{
    return 1.0f + 0.6f * saturate(overcast);
}

#endif
