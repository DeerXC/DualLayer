// lighting.hlsli - the maths of the light that needs no texture and no constant buffer: the fall-off of a lamp, the shading of one
//                  lamp, the shoulder that bends a light that is too bright, the fade at the edge of the shadow map, the glow round a
//                  lamp and the haze toward the sun.
// Why a file of its own: this is written in the part of HLSL that is also C++ (no swizzles, no out parameters, no textures), so
//                  tests/test_lighting.cpp includes it through a stand-in for the vector types (tests/hlsl_shim.h) and checks the
//                  numbers with the compiler of the tests: the shader file itself cannot be run without a GPU.
// Units:           metres; colours are linear (docs/CONVENTIONS.md).
// KEEP IN SYNC:    nothing in C++ depends on the numbers here except the defaults of render/LightingSettings.h (a lamp of strength 18
//                  under a street lamp, 3.4 m over the ground, lights it with 1.4).
#ifndef DL_LIGHTING_HLSLI
#define DL_LIGHTING_HLSLI

// How much of its strength a lamp gives at `dist` metres: the inverse square, softened near the lamp (+ 1.5 m^2) and multiplied by a
// smooth window that is exactly 0 at `range` (saturate(1 - (d / r)^4)^2: no hard edge to the circle of light).
float LampAttenuation(float dist, float range)
{
    const float x = saturate(dist / max(range, 0.001f));
    const float x2 = x * x;
    const float window = saturate(1.0f - x2 * x2);
    return window * window / (dist * dist + 1.5f);
}

struct LampLight
{
    float3 diffuse;  // what it adds to a surface that has an albedo (multiply by it)
    float3 specular; // what it adds as a highlight (already has the strength of the shine)
};

// One lamp at `lp` with the range `lr` and the colour `lc` (colour times strength), on a surface at `p` with the normal `n`, seen from
// the direction `v` (to the eye). `shine`: strength of the highlight, `power`: how sharp it is (params y and z of a surface).
LampLight LampShade(float3 p, float3 n, float3 v, float3 lp, float lr, float3 lc, float shine, float power)
{
    LampLight r;
    r.diffuse = float3(0.0f, 0.0f, 0.0f);
    r.specular = float3(0.0f, 0.0f, 0.0f);
    const float3 d = lp - p;
    const float dist = length(d);
    if (dist >= lr)
    {
        return r;
    }
    const float3 l = d / max(dist, 1.0e-4f);
    const float ndl = saturate(dot(n, l));
    const float3 received = lc * LampAttenuation(dist, lr);
    r.diffuse = received * ndl;
    if (shine > 0.0f)
    {
        const float3 h = normalize(l + v);
        r.specular = received * (pow(saturate(dot(n, h)), max(power, 1.0f)) * shine * step(0.0001f, ndl));
    }
    return r;
}

// A light that is brighter than `knee` is bent toward 1 (it never reaches it) instead of being cut off at 1 by the screen; one that
// is not is left as it is. The three colours are scaled together, so the hue of a bright lamp stays. The slope at the knee is 1:
// there is no visible start of the shoulder.
float3 SoftShoulder(float3 c, float knee)
{
    const float peak = max(c.x, max(c.y, c.z));
    if (peak <= knee)
    {
        return c;
    }
    const float room = 1.0f - knee;
    const float over = peak - knee;
    const float bent = knee + room * (over / (over + room));
    return c * (bent / peak);
}

// 1 inside the shadow map, falling to 0 over the last tenth of its width: the shadow ends softly at the edge of the box that follows
// the camera instead of with a line. (u, v): where the point is in the map, 0 .. 1.
float ShadowEdgeFade(float u, float v)
{
    const float edge = min(min(u, 1.0f - u), min(v, 1.0f - v));
    return saturate(edge * 10.0f);
}

// The glow round a lamp, from how squarely the soft sphere that carries it is seen (|n . v|, 0 .. 1): 1 in the middle, 0 at the rim.
float HaloFalloff(float facing)
{
    return pow(saturate(abs(facing)), 2.5f);
}

// The light of the sun scattered by the haze toward the sun: `cosine` is the cosine of the angle between the ray of sight and the
// direction to the sun; `amount` how strong the effect is.
float3 SunScatter(float3 sun, float cosine, float amount)
{
    return sun * (pow(saturate(cosine), 6.0f) * amount);
}

#endif
