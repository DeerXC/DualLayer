// TriMesh.h - a solid made of triangles: the town of the model (scene/Town.h), one per layer.
// Grid:      the triangles are filed in a grid of square cells over the ground plane (x, z), kCell metres each; a
//            triangle is filed in every cell its box over the ground touches. A query looks only at the cells round a
//            ball.
// Once:      a triangle that is filed in several cells is met once per query, in the first of the cells that the
//            query looks at (in the order of the cells). That is found from the box of the triangle and the range of
//            the query, so a query writes nothing: any number of them may run at the same time on one mesh.
// Box:       each cell holds the boxes of its triangles in one row of memory, and a ball is tested against the box
//            before the closest point of the triangle is worked out, a ray against the box before the barycentric
//            coordinates. Most triangles of a cell are far from the query (the town is 87% of them for a ray and 99%
//            for a ball), so most of the work is skipped, with the same result: the margins below are wider than the
//            rounding of the triangle maths, and tests/test_trimesh.cpp compares with a search of every triangle.
// Collision: PushOut moves a ball out of the triangles near it, as the ground does (scene/TriMath.h): the
//            closest point on each triangle, a few rounds. A triangle has no inside: a ball is pushed out to the side
//            its centre is on, so a surface is solid from both sides. The walking body (scene/Player.h SetMeshes) uses
//            it like any other solid.
// Plain data and plain maths, no Windows, no Direct3D: tested without a window (tests/test_town.cpp).
// UNITS: metres. Space: left-handed, +Y up.
#pragma once

#include "scene/Camera.h"  // Vec3
#include "scene/TriMath.h" // ClosestOnTriangle, Sub, Dot, Cross

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
        const Vec3 n = tri::Cross(tri::Sub(b, a), tri::Sub(c, a));
        if (!(tri::Dot(n, n) > 1.0e-12f))
        {
            return;
        }
        m_tris.push_back({a, b, c});
    }

    /// Files every triangle in the grid. Call it once, after the last Add.
    void Build()
    {
        m_offsets.clear();
        m_entries.clear();
        if (m_tris.empty())
        {
            m_nx = m_nz = 0;
            return;
        }
        std::vector<Box> boxes(m_tris.size());
        float x0 = 1.0e30f, z0 = 1.0e30f, x1 = -1.0e30f, z1 = -1.0e30f;
        for (std::size_t k = 0; k < m_tris.size(); ++k)
        {
            const Tri& t = m_tris[k];
            Box& b = boxes[k];
            b.lo[0] = Min3(t.a.x, t.b.x, t.c.x);
            b.lo[1] = Min3(t.a.y, t.b.y, t.c.y);
            b.lo[2] = Min3(t.a.z, t.b.z, t.c.z);
            b.hi[0] = Max3(t.a.x, t.b.x, t.c.x);
            b.hi[1] = Max3(t.a.y, t.b.y, t.c.y);
            b.hi[2] = Max3(t.a.z, t.b.z, t.c.z);
            x0 = b.lo[0] < x0 ? b.lo[0] : x0;
            z0 = b.lo[2] < z0 ? b.lo[2] : z0;
            x1 = b.hi[0] > x1 ? b.hi[0] : x1;
            z1 = b.hi[2] > z1 ? b.hi[2] : z1;
        }
        m_x0 = x0;
        m_z0 = z0;
        m_nx = static_cast<int>(std::floor((x1 - x0) / kCell)) + 1;
        m_nz = static_cast<int>(std::floor((z1 - z0) / kCell)) + 1;
        std::vector<std::uint32_t> counts(static_cast<std::size_t>(m_nx) * static_cast<std::size_t>(m_nz) + 1u, 0u);
        const auto forCells = [&](const Box& b, auto&& f)
        {
            int i0, i1, j0, j1;
            CellRange(b.lo[0], b.hi[0], b.lo[2], b.hi[2], i0, i1, j0, j1);
            for (int j = j0; j <= j1; ++j)
            {
                for (int i = i0; i <= i1; ++i)
                {
                    f(static_cast<std::size_t>(j) * static_cast<std::size_t>(m_nx) + static_cast<std::size_t>(i), i0,
                      j0);
                }
            }
        };
        for (const Box& b : boxes)
        {
            forCells(b, [&](std::size_t cell, int, int) { ++counts[cell + 1u]; });
        }
        for (std::size_t k = 1; k < counts.size(); ++k)
        {
            counts[k] += counts[k - 1];
        }
        m_offsets = counts;
        m_entries.resize(counts.back());
        std::vector<std::uint32_t> fill(counts.begin(), counts.end() - 1);
        for (std::uint32_t k = 0; k < m_tris.size(); ++k)
        {
            forCells(boxes[k],
                     [&](std::size_t cell, int i0, int j0)
                     {
                         Entry& e = m_entries[fill[cell]++];
                         static_cast<Box&>(e) = boxes[k];
                         e.tri = k;
                         e.i0 = i0;
                         e.j0 = j0;
                     });
        }
    }

    int TriangleCount() const
    {
        return static_cast<int>(m_tris.size());
    }

    /// Calls f(a, b, c) once for every triangle that is filed in a cell within `r` of (x, z) on the ground plane.
    template <class F> void ForEachNear(float x, float z, float r, F&& f) const
    {
        ForEachFiled(x - r, x + r, z - r, z + r,
                     [&](const Entry& e)
                     {
                         const Tri& t = m_tris[e.tri];
                         f(t.a, t.b, t.c);
                     });
    }

    /// Calls f(a, b, c) once for every triangle that is filed in a cell within `r` of the ball centre `c` on the ground
    /// plane, except those that cannot touch the ball: a triangle whose box is farther from `c` than sqrt(limitD2) is
    /// left out. `limitD2` is read again for every triangle, so a caller that is looking for the closest point can pass
    /// the squared distance of the closest so far, and the search gets faster as it goes.
    template <class F> void ForEachNearBall(const Vec3& c, float r, const float& limitD2, F&& f) const
    {
        ForEachFiled(c.x - r, c.x + r, c.z - r, c.z + r,
                     [&](const Entry& e)
                     {
                         if (BoxDistance2(e, c) <= limitD2 + kBallSlack)
                         {
                             const Tri& t = m_tris[e.tri];
                             f(t.a, t.b, t.c);
                         }
                     });
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
            ForEachNearBall(c, r, bestD2,
                            [&](const Vec3& a, const Vec3& b, const Vec3& t)
                            {
                                const Vec3 q = tri::ClosestOnTriangle(c, a, b, t);
                                const Vec3 d = tri::Sub(c, q);
                                const float d2 = tri::Dot(d, d);
                                if (d2 < bestD2)
                                {
                                    bestD2 = d2;
                                    best = q;
                                    bestNormal = tri::Cross(tri::Sub(b, a), tri::Sub(t, a));
                                    hit = true;
                                }
                            });
            if (!hit)
            {
                break;
            }
            const Vec3 d = tri::Sub(c, best);
            const float dist = std::sqrt(bestD2);
            if (dist > 1.0e-6f)
            {
                c = {best.x + d.x * (r / dist), best.y + d.y * (r / dist), best.z + d.z * (r / dist)};
            }
            else
            {
                // exactly on the surface: out along its normal, to the side that faces up (or any side of a wall)
                float l = std::sqrt(tri::Dot(bestNormal, bestNormal));
                Vec3 n = l > 0.0f ? Vec3{bestNormal.x / l, bestNormal.y / l, bestNormal.z / l} : Vec3{0.0f, 1.0f, 0.0f};
                if (n.y < 0.0f)
                {
                    n = {-n.x, -n.y, -n.z};
                }
                c = {best.x + n.x * r, best.y + n.y * r, best.z + n.z * r};
            }
        }
        const Vec3 moved = tri::Sub(c, start);
        return std::sqrt(tri::Dot(moved, moved));
    }

    /// The highest surface under (x, z) between y0 and y1 (a ray down), or false.
    bool HighestBelow(float x, float z, float y1, float y0, float& y) const
    {
        bool found = false;
        ForEachFiled(x, x, z, z,
                     [&](const Entry& e)
                     {
                         if (x < e.lo[0] - kRaySlack || x > e.hi[0] + kRaySlack || z < e.lo[2] - kRaySlack ||
                             z > e.hi[2] + kRaySlack || e.hi[1] < y0 - kRaySlack || e.lo[1] > y1 + kRaySlack)
                         {
                             return; // the ray misses the box of the triangle, or the box is out of the range
                         }
                         const Tri& t = m_tris[e.tri];
                         float h = 0.0f;
                         if (HeightOn(t.a, t.b, t.c, x, z, h) && h <= y1 && h >= y0 && (!found || h > y))
                         {
                             y = h;
                             found = true;
                         }
                     });
        return found;
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

  private:
    /// WHY these margins: HeightOn accepts a point a hair outside the triangle (1e-5 of its size, in float), and
    /// ClosestOnTriangle has the rounding of float coordinates up to a few hundred metres. Both margins are far wider
    /// than that, so a triangle that the exact test would take is never left out by the box; the box only has to be
    /// right when a triangle is far away, and then it is right by a wide margin.
    static constexpr float kRaySlack = 0.05f;  ///< UNITS: metres, round the box of a triangle, for a ray down
    static constexpr float kBallSlack = 1.0e-3f; ///< UNITS: metres squared, added to the squared limit of a ball

    struct Tri
    {
        Vec3 a, b, c;
    };
    struct Box
    {
        float lo[3]; ///< x, y, z
        float hi[3];
    };
    struct Entry : Box ///< a triangle as filed in a cell: its box (to read first) and where the triangle is
    {
        std::uint32_t tri;
        std::int32_t i0, j0; ///< the first column and row in which the triangle is filed
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

    /// The squared distance from `p` to the box of the triangle (0 inside it).
    static float BoxDistance2(const Box& b, const Vec3& p)
    {
        const float dx = p.x < b.lo[0] ? b.lo[0] - p.x : (p.x > b.hi[0] ? p.x - b.hi[0] : 0.0f);
        const float dy = p.y < b.lo[1] ? b.lo[1] - p.y : (p.y > b.hi[1] ? p.y - b.hi[1] : 0.0f);
        const float dz = p.z < b.lo[2] ? b.lo[2] - p.z : (p.z > b.hi[2] ? p.z - b.hi[2] : 0.0f);
        return dx * dx + dy * dy + dz * dz;
    }

    int Column(float x) const
    {
        const int i = static_cast<int>(std::floor((x - m_x0) / kCell));
        return i < 0 ? 0 : (i >= m_nx ? m_nx - 1 : i);
    }
    int Row(float z) const
    {
        const int j = static_cast<int>(std::floor((z - m_z0) / kCell));
        return j < 0 ? 0 : (j >= m_nz ? m_nz - 1 : j);
    }
    void CellRange(float xa, float xb, float za, float zb, int& i0, int& i1, int& j0, int& j1) const
    {
        i0 = Column(xa);
        i1 = Column(xb);
        j0 = Row(za);
        j1 = Row(zb);
    }

    /// Calls f(entry) once for every triangle filed in a cell of the range (x from xa to xb, z from za to zb), cell by
    /// cell, row by row. A triangle that is filed in several cells of the range is given in the first of them: the
    /// one where the larger of the first column of its box and the first column of the range is, and the same for the
    /// row (the cells of a triangle are a whole block, and so are the cells of the range). That needs a look only when
    /// the range is more than one cell; a range of one cell, as a ray down has, never meets a triangle twice. Reads,
    /// never writes.
    template <class F> void ForEachFiled(float xa, float xb, float za, float zb, F&& f) const
    {
        if (m_nx <= 0)
        {
            return;
        }
        int i0, i1, j0, j1;
        CellRange(xa, xb, za, zb, i0, i1, j0, j1);
        const bool several = i1 > i0 || j1 > j0;
        for (int j = j0; j <= j1; ++j)
        {
            for (int i = i0; i <= i1; ++i)
            {
                const std::size_t cell =
                    static_cast<std::size_t>(j) * static_cast<std::size_t>(m_nx) + static_cast<std::size_t>(i);
                for (std::uint32_t k = m_offsets[cell]; k < m_offsets[cell + 1u]; ++k)
                {
                    const Entry& e = m_entries[k];
                    if (several && !((i == i0 || i == e.i0) && (j == j0 || j == e.j0)))
                    {
                        continue; // met in an earlier cell of the range
                    }
                    f(e);
                }
            }
        }
    }

    std::vector<Tri> m_tris;
    std::vector<std::uint32_t> m_offsets; // the triangles of cell k are m_entries[m_offsets[k] .. m_offsets[k + 1])
    std::vector<Entry> m_entries;
    float m_x0 = 0.0f;
    float m_z0 = 0.0f;
    int m_nx = 0;
    int m_nz = 0;
};

} // namespace dl::town
