// Terrain.h - a height field: the ground the city stands on (docs/LEVEL_DESIGN.md, 5.4 and 12, step 2).
// Field:     a grid of heights, (nx + 1) x (nz + 1) vertices `cell` metres apart from (x0, z0). Each cell is two triangles
//            split along the diagonal from its (x0, z0) corner to its (x1, z1) corner. Height() is the height of those
//            triangles (exactly what is drawn and what is solid), not a smooth guess between the vertices.
// Land:      two fields: a fine one round the map (`nearField`) and a coarse one to the far plane and beyond (`farField`).
//            The coarse one has a hole where the fine one is; the outer edge of the fine one is pinned to the coarse one, so
//            the two meet without a crack (PinEdge). Height(), Normal() and PushOut() choose the field that is drawn there.
// Collision: PushOut moves a ball out of the triangles near it (the closest point on each triangle; a centre under the
//            surface is first lifted onto it). Used by the walking body (scene/Player.h SetLand) like any other solid.
// Noise:     value noise, its sum over octaves (fbm) and a ridged sum (sharp crests): the shapes of the land. Plain arithmetic
//            with a fixed hash, so the same land comes out on every machine.
// Plain data and plain maths, no Windows, no Direct3D: tested without a window (tests/test_terrain.cpp).
// UNITS: metres. Space: left-handed, +Y up.
// Note:      no std::min / std::max here: windows.h may define them as macros (docs/CONVENTIONS.md).
#pragma once

#include "scene/Camera.h" // Vec3

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace dl::terrain
{

// ---- noise

inline float Hash01(std::int64_t ix, std::int64_t iz, std::uint32_t seed)
{
    std::uint32_t h = static_cast<std::uint32_t>(ix) * 374761393u + static_cast<std::uint32_t>(iz) * 668265263u + seed * 1442695041u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return static_cast<float>(h & 0xFFFFu) / 65535.0f;
}

/// Smooth value noise in -1 .. 1, one cell per unit.
inline float ValueNoise(float x, float z, std::uint32_t seed)
{
    const float fx0 = std::floor(x);
    const float fz0 = std::floor(z);
    const auto ix = static_cast<std::int64_t>(fx0);
    const auto iz = static_cast<std::int64_t>(fz0);
    const float fx = x - fx0;
    const float fz = z - fz0;
    const float ux = fx * fx * (3.0f - 2.0f * fx);
    const float uz = fz * fz * (3.0f - 2.0f * fz);
    const float a = Hash01(ix, iz, seed);
    const float b = Hash01(ix + 1, iz, seed);
    const float c = Hash01(ix, iz + 1, seed);
    const float d = Hash01(ix + 1, iz + 1, seed);
    return 2.0f * (a + (b - a) * ux + (c - a) * uz + (a - b - c + d) * ux * uz) - 1.0f;
}

/// The sum of `octaves` layers of noise, each twice as fine and half as strong: -1 .. 1.
inline float Fbm(float x, float z, int octaves, std::uint32_t seed)
{
    float total = 0.0f;
    float amp = 1.0f;
    float freq = 1.0f;
    float norm = 0.0f;
    for (int o = 0; o < octaves; ++o)
    {
        total += amp * ValueNoise(x * freq, z * freq, seed + static_cast<std::uint32_t>(o) * 17u);
        norm += amp;
        amp *= 0.5f;
        freq *= 2.03f;
    }
    return norm > 0.0f ? total / norm : 0.0f;
}

/// Ridged noise in 0 .. 1: sharp crests, as a mountain skyline.
inline float Ridged(float x, float z, int octaves, std::uint32_t seed)
{
    float total = 0.0f;
    float amp = 1.0f;
    float freq = 1.0f;
    float norm = 0.0f;
    for (int o = 0; o < octaves; ++o)
    {
        total += amp * (1.0f - std::fabs(ValueNoise(x * freq, z * freq, seed + static_cast<std::uint32_t>(o) * 31u)));
        norm += amp;
        amp *= 0.5f;
        freq *= 2.1f;
    }
    return norm > 0.0f ? total / norm : 0.0f;
}

/// A smooth step from 0 at e0 to 1 at e1 (e0 may be larger than e1).
inline float SmoothStep(float e0, float e1, float v)
{
    float t = (v - e0) / (e1 - e0);
    t = t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);
    return t * t * (3.0f - 2.0f * t);
}

// ---- the field

struct Field
{
    float x0 = 0.0f;
    float z0 = 0.0f;
    float cell = 1.0f; ///< UNITS: metres between two vertices
    int nx = 0;        ///< cells along x
    int nz = 0;        ///< cells along z
    std::vector<float> h; ///< (nx + 1) * (nz + 1) heights, x fastest

    float X1() const
    {
        return x0 + cell * static_cast<float>(nx);
    }
    float Z1() const
    {
        return z0 + cell * static_cast<float>(nz);
    }
    bool Contains(float x, float z) const
    {
        return x >= x0 && x <= X1() && z >= z0 && z <= Z1();
    }
    float VertexX(int i) const
    {
        return x0 + cell * static_cast<float>(i);
    }
    float VertexZ(int j) const
    {
        return z0 + cell * static_cast<float>(j);
    }
    float At(int i, int j) const
    {
        return h[static_cast<std::size_t>(j) * static_cast<std::size_t>(nx + 1) + static_cast<std::size_t>(i)];
    }
    float& At(int i, int j)
    {
        return h[static_cast<std::size_t>(j) * static_cast<std::size_t>(nx + 1) + static_cast<std::size_t>(i)];
    }

    /// Sets every vertex from `height(x, z)`.
    template <class F>
    void Build(float originX, float originZ, float cellSize, int cellsX, int cellsZ, F height)
    {
        x0 = originX;
        z0 = originZ;
        cell = cellSize;
        nx = cellsX;
        nz = cellsZ;
        h.assign(static_cast<std::size_t>(nx + 1) * static_cast<std::size_t>(nz + 1), 0.0f);
        for (int j = 0; j <= nz; ++j)
        {
            for (int i = 0; i <= nx; ++i)
            {
                At(i, j) = height(VertexX(i), VertexZ(j));
            }
        }
    }

    /// The cell that holds (x, z) (clamped into the field) and the place in it, 0 .. 1 on each axis.
    void Locate(float x, float z, int& i, int& j, float& u, float& v) const
    {
        float fx = (x - x0) / cell;
        float fz = (z - z0) / cell;
        fx = fx < 0.0f ? 0.0f : (fx > static_cast<float>(nx) ? static_cast<float>(nx) : fx);
        fz = fz < 0.0f ? 0.0f : (fz > static_cast<float>(nz) ? static_cast<float>(nz) : fz);
        i = static_cast<int>(fx);
        j = static_cast<int>(fz);
        i = i >= nx ? nx - 1 : i;
        j = j >= nz ? nz - 1 : j;
        u = fx - static_cast<float>(i);
        v = fz - static_cast<float>(j);
    }

    /// The height of the drawn surface at (x, z) (outside the field: the height at its edge).
    float Height(float x, float z) const
    {
        if (nx <= 0 || nz <= 0)
        {
            return 0.0f;
        }
        int i = 0;
        int j = 0;
        float u = 0.0f;
        float v = 0.0f;
        Locate(x, z, i, j, u, v);
        const float h00 = At(i, j);
        const float h10 = At(i + 1, j);
        const float h01 = At(i, j + 1);
        const float h11 = At(i + 1, j + 1);
        // the triangle (00, 10, 11) where u >= v, (00, 11, 01) where v > u
        return u >= v ? h00 + u * (h10 - h00) + v * (h11 - h10) : h00 + v * (h01 - h00) + u * (h11 - h01);
    }

    /// The three corners of triangle `t` (0 or 1) of cell (i, j), in the order that is clockwise seen from above (their
    /// cross product, cross(b - a, c - a), points up).
    void Triangle(int i, int j, int t, Vec3& a, Vec3& b, Vec3& c) const
    {
        const float xa = VertexX(i);
        const float xb = VertexX(i + 1);
        const float za = VertexZ(j);
        const float zb = VertexZ(j + 1);
        a = {xa, At(i, j), za};
        if (t == 0)
        {
            b = {xb, At(i + 1, j + 1), zb};
            c = {xb, At(i + 1, j), za};
        }
        else
        {
            b = {xa, At(i, j + 1), zb};
            c = {xb, At(i + 1, j + 1), zb};
        }
    }
};

// ---- collision with one triangle

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

// ---- the land: a fine field round the map, a coarse one round it

class Land
{
  public:
    Field nearField; ///< the fine field round the map
    Field farField;  ///< the coarse field to the horizon; its cells under the fine one are not drawn and not solid

    /// True when (x, z) is drawn by the fine field.
    bool InNear(float x, float z) const
    {
        return nearField.nx > 0 && nearField.Contains(x, z);
    }

    /// True when cell (i, j) of the coarse field lies under the fine one (it is a hole).
    bool FarCellHidden(int i, int j) const
    {
        const float cx = farField.VertexX(i) + 0.5f * farField.cell;
        const float cz = farField.VertexZ(j) + 0.5f * farField.cell;
        return InNear(cx, cz);
    }

    float Height(float x, float z) const
    {
        return InNear(x, z) ? nearField.Height(x, z) : farField.Height(x, z);
    }

    /// The upward unit normal of the surface at (x, z) (by central differences over half a cell of the field there).
    Vec3 Normal(float x, float z) const
    {
        const float e = 0.25f * (InNear(x, z) ? nearField.cell : farField.cell);
        const float gx = (Height(x + e, z) - Height(x - e, z)) / (2.0f * e);
        const float gz = (Height(x, z + e) - Height(x, z - e)) / (2.0f * e);
        const float l = std::sqrt(gx * gx + 1.0f + gz * gz);
        return {-gx / l, 1.0f / l, -gz / l};
    }

    /// The steepness at (x, z): the rise per metre of the steepest way up.
    float Slope(float x, float z) const
    {
        const Vec3 n = Normal(x, z);
        return std::sqrt(n.x * n.x + n.z * n.z) / (n.y > 1.0e-4f ? n.y : 1.0e-4f);
    }

    /// Sets the outer edge of the fine field to the coarse field under it, so that the two meet without a crack. The fine
    /// field must lie on whole cells of the coarse one.
    void PinEdge()
    {
        Field& f = nearField;
        for (int i = 0; i <= f.nx; ++i)
        {
            f.At(i, 0) = farField.Height(f.VertexX(i), f.VertexZ(0));
            f.At(i, f.nz) = farField.Height(f.VertexX(i), f.VertexZ(f.nz));
        }
        for (int j = 0; j <= f.nz; ++j)
        {
            f.At(0, j) = farField.Height(f.VertexX(0), f.VertexZ(j));
            f.At(f.nx, j) = farField.Height(f.VertexX(f.nx), f.VertexZ(j));
        }
    }

    /// Calls f(a, b, c) for every drawn and solid triangle whose cell overlaps the square of half size `reach` round (x, z).
    template <class F>
    void ForEachTriangleNear(float x, float z, float reach, F f) const
    {
        const auto visit = [&](const Field& field, bool skipHidden) {
            if (field.nx <= 0)
            {
                return;
            }
            const int i0 = static_cast<int>(std::floor((x - reach - field.x0) / field.cell));
            const int i1 = static_cast<int>(std::floor((x + reach - field.x0) / field.cell));
            const int j0 = static_cast<int>(std::floor((z - reach - field.z0) / field.cell));
            const int j1 = static_cast<int>(std::floor((z + reach - field.z0) / field.cell));
            for (int j = j0 < 0 ? 0 : j0; j <= j1 && j < field.nz; ++j)
            {
                for (int i = i0 < 0 ? 0 : i0; i <= i1 && i < field.nx; ++i)
                {
                    if (skipHidden && FarCellHidden(i, j))
                    {
                        continue;
                    }
                    for (int t = 0; t < 2; ++t)
                    {
                        Vec3 a;
                        Vec3 b;
                        Vec3 c;
                        field.Triangle(i, j, t, a, b, c);
                        f(a, b, c);
                    }
                }
            }
        };
        visit(nearField, false);
        visit(farField, true);
    }

    /// Moves a ball (centre `c`, radius `r`) out of the ground. Returns how far it moved (0 = it did not touch).
    float PushOut(Vec3& c, float r) const
    {
        if (!(r > 0.0f) || !std::isfinite(c.x) || !std::isfinite(c.y) || !std::isfinite(c.z) || (nearField.nx <= 0 && farField.nx <= 0))
        {
            return 0.0f;
        }
        const Vec3 start = c;
        const float under = Height(c.x, c.z);
        if (c.y < under)
        {
            c.y = under; // a centre under the surface: lifted onto it first, then pushed out to its radius
        }
        for (int round = 0; round < 3; ++round)
        {
            Vec3 best = c;
            float bestD2 = r * r;
            bool hit = false;
            ForEachTriangleNear(c.x, c.z, r, [&](const Vec3& a, const Vec3& b, const Vec3& t) {
                const Vec3 q = ClosestOnTriangle(c, a, b, t);
                const Vec3 d = Sub(c, q);
                const float d2 = Dot(d, d);
                if (d2 < bestD2)
                {
                    bestD2 = d2;
                    best = q;
                    hit = true;
                }
            });
            if (!hit)
            {
                break;
            }
            const Vec3 d = Sub(c, best);
            const float dist = std::sqrt(bestD2);
            if (dist > 1.0e-6f)
            {
                c = {best.x + d.x * (r / dist), best.y + d.y * (r / dist), best.z + d.z * (r / dist)};
            }
            else
            {
                const Vec3 n = Normal(c.x, c.z); // exactly on the surface: out along its normal
                c = {best.x + n.x * r, best.y + n.y * r, best.z + n.z * r};
            }
        }
        const Vec3 moved = Sub(c, start);
        return std::sqrt(Dot(moved, moved));
    }
};

} // namespace dl::terrain
