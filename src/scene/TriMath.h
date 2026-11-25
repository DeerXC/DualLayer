// TriMath.h - the maths of a ball against a triangle: the vector operations and the point of a triangle closest to a
// point.
//             The solid of the town (scene/TriMesh.h, scene/Moments.h) is made of triangles and pushes a ball out of
//             them with the closest point.
// Plain maths, no Windows, no Direct3D: tested without a window (tests/test_trimath.cpp).
// UNITS: metres. Space: left-handed, +Y up.
#pragma once

#include "scene/Camera.h" // Vec3

namespace dl::tri
{

inline Vec3 Sub(const Vec3& a, const Vec3& b)
{
    return {a.x - b.x, a.y - b.y, a.z - b.z};
}

inline float Dot(const Vec3& a, const Vec3& b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline Vec3 Cross(const Vec3& a, const Vec3& b)
{
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

/// The point of triangle (a, b, c) closest to p (Ericson, Real-Time Collision Detection, 5.1.5).
inline Vec3 ClosestOnTriangle(const Vec3& p, const Vec3& a, const Vec3& b, const Vec3& c)
{
    const Vec3 ab = Sub(b, a);
    const Vec3 ac = Sub(c, a);
    const Vec3 ap = Sub(p, a);
    const float d1 = Dot(ab, ap);
    const float d2 = Dot(ac, ap);
    if (d1 <= 0.0f && d2 <= 0.0f)
    {
        return a;
    }
    const Vec3 bp = Sub(p, b);
    const float d3 = Dot(ab, bp);
    const float d4 = Dot(ac, bp);
    if (d3 >= 0.0f && d4 <= d3)
    {
        return b;
    }
    const float vc = d1 * d4 - d3 * d2;
    if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f)
    {
        const float t = d1 / (d1 - d3);
        return {a.x + t * ab.x, a.y + t * ab.y, a.z + t * ab.z};
    }
    const Vec3 cp = Sub(p, c);
    const float d5 = Dot(ab, cp);
    const float d6 = Dot(ac, cp);
    if (d6 >= 0.0f && d5 <= d6)
    {
        return c;
    }
    const float vb = d5 * d2 - d1 * d6;
    if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f)
    {
        const float t = d2 / (d2 - d6);
        return {a.x + t * ac.x, a.y + t * ac.y, a.z + t * ac.z};
    }
    const float va = d3 * d6 - d5 * d4;
    if (va <= 0.0f && (d4 - d3) >= 0.0f && (d5 - d6) >= 0.0f)
    {
        const float t = (d4 - d3) / ((d4 - d3) + (d5 - d6));
        return {b.x + t * (c.x - b.x), b.y + t * (c.y - b.y), b.z + t * (c.z - b.z)};
    }
    const float denom = 1.0f / (va + vb + vc);
    const float v = vb * denom;
    const float w = vc * denom;
    return {a.x + ab.x * v + ac.x * w, a.y + ab.y * v + ac.y * w, a.z + ab.z * v + ac.z * w};
}

} // namespace dl::tri
