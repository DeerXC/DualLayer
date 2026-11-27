// ShadowMath.h - the matrix of the sun's (or the moon's) shadow map: an orthographic box that follows the camera.
// Fit:       the box is 2 * radius wide and high (across the light) and 2 * halfDepth deep (along it), centred on a
//            point near the camera. Its centre is moved to the grid of the shadow map's texels in the light's own
//            plane, so that a shadow does not crawl or shimmer when the camera walks: it only appears at the edge of
//            the box.
// Space:     left-handed, +Y up; row vectors: clip = world * ViewProjection (DirectXMath's convention,
//            docs/CONVENTIONS.md). The result is 16 floats, row by row, what the constant buffer takes as it is.
// Depth:     0 at the light's side of the box and 1 at the far side (the plane of the light is `halfDepth` toward the
//            light from the centre).
// Plain maths, no Windows, no Direct3D: tested without a window (tests/test_lighting.cpp).
// UNITS:     metres.
#pragma once

#include <cmath>

namespace dl
{

struct ShadowFit
{
    float viewProj[16] = {}; ///< row-major, row vectors
    float texelWorld = 0.0f; ///< UNITS: metres: the size of one texel of the map in the world
    float depthRange = 0.0f; ///< UNITS: metres: 2 * halfDepth
    float centre[3] = {};    ///< the centre of the box after it was moved to the texel grid
};

namespace shadow_detail
{

struct V3
{
    float x, y, z;
};

inline V3 Cross(const V3& a, const V3& b)
{
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

inline float Dot(const V3& a, const V3& b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline V3 Unit(const V3& a)
{
    const float l = std::sqrt(Dot(a, a));
    return l > 1.0e-6f ? V3{a.x / l, a.y / l, a.z / l} : V3{0.0f, 1.0f, 0.0f};
}

} // namespace shadow_detail

/// The shadow matrix for light coming FROM `toLight` (a direction from a surface to the light; any length), a box of
/// half-width `radius` and half-depth `halfDepth` round `centre`, and a map of mapSize x mapSize texels. A zero or NaN
/// direction is taken as straight overhead; sizes below a small minimum are pulled up to it.
inline ShadowFit FitSunShadow(const float toLight[3], const float centre[3], float radius, float halfDepth, int mapSize)
{
    using namespace shadow_detail;
    ShadowFit fit;
    const float size = static_cast<float>(mapSize < 16 ? 16 : mapSize);
    const float r = std::isfinite(radius) && radius > 1.0f ? radius : 1.0f;
    const float hd = std::isfinite(halfDepth) && halfDepth > 1.0f ? halfDepth : 1.0f;
    const bool good = std::isfinite(toLight[0]) && std::isfinite(toLight[1]) && std::isfinite(toLight[2]);
    const V3 l = Unit(good ? V3{toLight[0], toLight[1], toLight[2]} : V3{0.0f, 1.0f, 0.0f});
    const V3 f{-l.x, -l.y, -l.z}; // the light looks along this
    const V3 hint = std::fabs(f.y) > 0.99f ? V3{0.0f, 0.0f, 1.0f} : V3{0.0f, 1.0f, 0.0f};
    const V3 right = Unit(Cross(hint, f));
    const V3 up = Cross(f, right);
    const float texel = 2.0f * r / size;
    V3 c{std::isfinite(centre[0]) ? centre[0] : 0.0f, std::isfinite(centre[1]) ? centre[1] : 0.0f,
         std::isfinite(centre[2]) ? centre[2] : 0.0f};
    // the centre on the texel grid of the light's plane
    const float cx = Dot(right, c);
    const float cy = Dot(up, c);
    const float sx = std::floor(cx / texel + 0.5f) * texel - cx;
    const float sy = std::floor(cy / texel + 0.5f) * texel - cy;
    c = {c.x + right.x * sx + up.x * sy, c.y + right.y * sx + up.y * sy, c.z + right.z * sx + up.z * sy};
    const V3 e{c.x + l.x * hd, c.y + l.y * hd, c.z + l.z * hd}; // the light's side of the box
    const float depth = 2.0f * hd;
    // view (look-to, left-handed) times orthographic projection (left-handed), written out
    const float sxp = 1.0f / r;
    const float szp = 1.0f / depth;
    const float tx = -Dot(right, e);
    const float ty = -Dot(up, e);
    const float tz = -Dot(f, e);
    float* m = fit.viewProj;
    m[0] = right.x * sxp;
    m[1] = up.x * sxp;
    m[2] = f.x * szp;
    m[3] = 0.0f;
    m[4] = right.y * sxp;
    m[5] = up.y * sxp;
    m[6] = f.y * szp;
    m[7] = 0.0f;
    m[8] = right.z * sxp;
    m[9] = up.z * sxp;
    m[10] = f.z * szp;
    m[11] = 0.0f;
    m[12] = tx * sxp;
    m[13] = ty * sxp;
    m[14] = tz * szp;
    m[15] = 1.0f;
    fit.texelWorld = texel;
    fit.depthRange = depth;
    fit.centre[0] = c.x;
    fit.centre[1] = c.y;
    fit.centre[2] = c.z;
    return fit;
}

/// Where a world point goes: x, y in -1 .. 1 across the box, z in 0 .. 1 along the light (0 on the light's side).
inline void ShadowClip(const ShadowFit& fit, const float p[3], float out[3])
{
    const float* m = fit.viewProj;
    out[0] = p[0] * m[0] + p[1] * m[4] + p[2] * m[8] + m[12];
    out[1] = p[0] * m[1] + p[1] * m[5] + p[2] * m[9] + m[13];
    out[2] = p[0] * m[2] + p[1] * m[6] + p[2] * m[10] + m[14];
}

} // namespace dl
