// TriMesh.h - a solid made of triangles: the town of the model (scene/Town.h), one per layer.
// Grid:      the triangles are filed in a grid of square cells over the ground plane (x, z), kCell metres each; a triangle
//            is filed in every cell its box over the ground touches. A query looks only at the cells round a ball.
// Collision: PushOut moves a ball out of the triangles near it, as the ground does (scene/Terrain.h Land::PushOut): the
//            closest point on each triangle, a few rounds. A triangle has no inside: a ball is pushed out to the side its
//            centre is on, so a surface is solid from both sides. The walking body (scene/Player.h SetMeshes) uses it like
//            any other solid.
// Plain data and plain maths, no Windows, no Direct3D: tested without a window (tests/test_town.cpp).
// UNITS: metres. Space: left-handed, +Y up.
#pragma once

#include "scene/Camera.h" // Vec3
#include "scene/Terrain.h" // ClosestOnTriangle, Sub, Dot, Cross

#include <cmath>
#include <cstdint>
#include <vector>

namespace dl::town
{

class TriMesh
{
  public:
    static constexpr float kCell = 4.0f; ///< UNITS: metres: the side of a cell of the grid

    /// Adds one triangle (before Build). A triangle with no area is left out.
    void Add(const Vec3& a, const Vec3& b, const Vec3& c)
    {
        const Vec3 n = terrain::Cross(terrain::Sub(b, a), terrain::Sub(c, a));
        if (!(terrain::Dot(n, n) > 1.0e-12f))
        {
            return;
        }
        m_tris.push_back({a, b, c});
    }

    /// Files every triangle in the grid. Call it once, after the last Add.
    void Build()
    {
        m_offsets.clear();
        m_items.clear();
        m_stamp.assign(m_tris.size(), 0u);
        if (m_tris.empty())
        {
            m_nx = m_nz = 0;
            return;
        }
        float x0 = 1.0e30f, z0 = 1.0e30f, x1 = -1.0e30f, z1 = -1.0e30f;
        for (const Tri& t : m_tris)
        {
            for (const Vec3* p : {&t.a, &t.b, &t.c})
            {
                x0 = p->x < x0 ? p->x : x0;
                z0 = p->z < z0 ? p->z : z0;
                x1 = p->x > x1 ? p->x : x1;
                z1 = p->z > z1 ? p->z : z1;
            }
        }
        m_x0 = x0;
        m_z0 = z0;
        m_nx = static_cast<int>(std::floor((x1 - x0) / kCell)) + 1;
        m_nz = static_cast<int>(std::floor((z1 - z0) / kCell)) + 1;
        std::vector<std::uint32_t> counts(static_cast<std::size_t>(m_nx) * static_cast<std::size_t>(m_nz) + 1u, 0u);
        const auto forCells = [&](const Tri& t, auto&& f) {
            int i0, i1, j0, j1;
            CellRange(Min3(t.a.x, t.b.x, t.c.x), Max3(t.a.x, t.b.x, t.c.x), Min3(t.a.z, t.b.z, t.c.z), Max3(t.a.z, t.b.z, t.c.z), i0, i1, j0, j1);
            for (int j = j0; j <= j1; ++j)
            {
                for (int i = i0; i <= i1; ++i)
                {
                    f(static_cast<std::size_t>(j) * static_cast<std::size_t>(m_nx) + static_cast<std::size_t>(i));
                }
            }
        };
        for (const Tri& t : m_tris)
        {
            forCells(t, [&](std::size_t cell) { ++counts[cell + 1u]; });
        }
        for (std::size_t k = 1; k < counts.size(); ++k)
        {
            counts[k] += counts[k - 1];
        }
        m_offsets = counts;
        m_items.resize(counts.back());
        std::vector<std::uint32_t> fill(counts.begin(), counts.end() - 1);
        for (std::uint32_t k = 0; k < m_tris.size(); ++k)
        {
            forCells(m_tris[k], [&](std::size_t cell) { m_items[fill[cell]++] = k; });
        }
    }

    int TriangleCount() const
    {
        return static_cast<int>(m_tris.size());
    }

    /// Calls f(a, b, c) once for every triangle whose cell lies within `r` of (x, z) on the ground plane.
    template <class F>
    void ForEachNear(float x, float z, float r, F&& f) const
    {
        if (m_nx <= 0)
        {
            return;
        }
        int i0, i1, j0, j1;
        CellRange(x - r, x + r, z - r, z + r, i0, i1, j0, j1);
        if (++m_visit == 0u) // the counter went round: forget every old stamp
        {
            m_stamp.assign(m_stamp.size(), 0u);
            m_visit = 1u;
        }
        for (int j = j0; j <= j1; ++j)
        {
            for (int i = i0; i <= i1; ++i)
            {
                const std::size_t cell = static_cast<std::size_t>(j) * static_cast<std::size_t>(m_nx) + static_cast<std::size_t>(i);
                for (std::uint32_t k = m_offsets[cell]; k < m_offsets[cell + 1u]; ++k)
                {
                    const std::uint32_t t = m_items[k];
                    if (m_stamp[t] == m_visit)
                    {
                        continue; // filed in more than one cell: once is enough
                    }
                    m_stamp[t] = m_visit;
                    f(m_tris[t].a, m_tris[t].b, m_tris[t].c);
                }
            }
        }
    }

    /// Moves the centre `c` of a ball of radius `r` out of every triangle it overlaps. Returns how far it moved.
    float PushOut(Vec3& c, float r) const
    {
        if (!(r > 0.0f) || !std::isfinite(c.x) || !std::isfinite(c.y) || !std::isfinite(c.z))
        {
            return 0.0f;
        }
        const Vec3 start = c;
        for (int round = 0; round < 4; ++round)
        {
            Vec3 best = c;
            Vec3 bestNormal{0.0f, 1.0f, 0.0f};
            float bestD2 = r * r;
            bool hit = false;
            ForEachNear(c.x, c.z, r, [&](const Vec3& a, const Vec3& b, const Vec3& t) {
                const Vec3 q = terrain::ClosestOnTriangle(c, a, b, t);
                const Vec3 d = terrain::Sub(c, q);
                const float d2 = terrain::Dot(d, d);
                if (d2 < bestD2)
                {
                    bestD2 = d2;
                    best = q;
                    bestNormal = terrain::Cross(terrain::Sub(b, a), terrain::Sub(t, a));
                    hit = true;
                }
            });
            if (!hit)
            {
                break;
            }
            const Vec3 d = terrain::Sub(c, best);
            const float dist = std::sqrt(bestD2);
            if (dist > 1.0e-6f)
            {
                c = {best.x + d.x * (r / dist), best.y + d.y * (r / dist), best.z + d.z * (r / dist)};
            }
            else
            {
                // exactly on the surface: out along its normal, to the side that faces up (or any side of a wall)
                float l = std::sqrt(terrain::Dot(bestNormal, bestNormal));
                Vec3 n = l > 0.0f ? Vec3{bestNormal.x / l, bestNormal.y / l, bestNormal.z / l} : Vec3{0.0f, 1.0f, 0.0f};
                if (n.y < 0.0f)
                {
                    n = {-n.x, -n.y, -n.z};
                }
                c = {best.x + n.x * r, best.y + n.y * r, best.z + n.z * r};
            }
        }
        const Vec3 moved = terrain::Sub(c, start);
        return std::sqrt(terrain::Dot(moved, moved));
    }

    /// The highest surface under (x, z) between y0 and y1 (a ray down), or false.
    bool HighestBelow(float x, float z, float y1, float y0, float& y) const
    {
        bool found = false;
        ForEachNear(x, z, 0.0f, [&](const Vec3& a, const Vec3& b, const Vec3& c) {
            float h = 0.0f;
            if (HeightOn(a, b, c, x, z, h) && h <= y1 && h >= y0 && (!found || h > y))
            {
                y = h;
                found = true;
            }
        });
        return found;
    }

  private:
    struct Tri
    {
        Vec3 a, b, c;
    };

    static float Min3(float a, float b, float c)
    {
        const float m = a < b ? a : b;
        return m < c ? m : c;
    }
    static float Max3(float a, float b, float c)
    {
        const float m = a > b ? a : b;
        return m > c ? m : c;
    }

    void CellRange(float xa, float xb, float za, float zb, int& i0, int& i1, int& j0, int& j1) const
    {
        const auto clampI = [](int v, int n) { return v < 0 ? 0 : (v >= n ? n - 1 : v); };
        i0 = clampI(static_cast<int>(std::floor((xa - m_x0) / kCell)), m_nx);
        i1 = clampI(static_cast<int>(std::floor((xb - m_x0) / kCell)), m_nx);
        j0 = clampI(static_cast<int>(std::floor((za - m_z0) / kCell)), m_nz);
        j1 = clampI(static_cast<int>(std::floor((zb - m_z0) / kCell)), m_nz);
    }

    /// The height of the triangle over (x, z), when (x, z) lies inside it seen from above (a vertical wall has none).
    static bool HeightOn(const Vec3& a, const Vec3& b, const Vec3& c, float x, float z, float& h)
    {
        const float d = (b.z - c.z) * (a.x - c.x) + (c.x - b.x) * (a.z - c.z);
        if (std::fabs(d) < 1.0e-9f)
        {
            return false;
        }
        const float u = ((b.z - c.z) * (x - c.x) + (c.x - b.x) * (z - c.z)) / d;
        const float v = ((c.z - a.z) * (x - c.x) + (a.x - c.x) * (z - c.z)) / d;
        const float w = 1.0f - u - v;
        const float e = -1.0e-5f;
        if (u < e || v < e || w < e)
        {
            return false;
        }
        h = u * a.y + v * b.y + w * c.y;
        return true;
    }

    std::vector<Tri> m_tris;
    std::vector<std::uint32_t> m_offsets; // the triangles of cell k are m_items[m_offsets[k] .. m_offsets[k + 1])
    std::vector<std::uint32_t> m_items;
    mutable std::vector<std::uint32_t> m_stamp; // the query that last met each triangle (so it is met once per query)
    mutable std::uint32_t m_visit = 0;
    float m_x0 = 0.0f;
    float m_z0 = 0.0f;
    int m_nx = 0;
    int m_nz = 0;
};

} // namespace dl::town
