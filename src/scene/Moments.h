// Moments.h - the solid of the town in both moments, with the circle of the lantern (docs/DESIGN.md 3.2;
// scene/Lantern.h).
// Circle:    a sphere in which B is A: what is solid there and what is seen is A.
// PushOut:   a ball is pushed out of a triangle of A when the point of the triangle nearest to it lies inside the
//            circle, and out of a triangle of B when that point lies outside it. So a floor of A ends at the edge of the circle
//            under the centre of the ball, as the picture ends there (shaders/models.hlsl LanternClip). In A the circle
//            changes nothing.
// Extra:     the things that change (scene/Things.h) add small solids to a moment (SetExtra): the boxes of their state
//            now. They follow the same rule as the town's own triangles.
// Plain data and plain maths, no Windows, no Direct3D: tested without a window (tests/test_lantern.cpp).
// UNITS: metres. Space: left-handed, +Y up.
#pragma once

#include "scene/Camera.h"  // Vec3
#include "scene/TriMath.h" // ClosestOnTriangle
#include "scene/TriMesh.h"

#include <array>
#include <cmath>
#include <vector>

namespace dl::town
{

struct Circle
{
    Vec3 centre{};
    float radius = 0.0f; ///< 0: no circle

    bool On() const
    {
        return radius > 0.0f;
    }
    bool Contains(const Vec3& p) const
    {
        const float dx = p.x - centre.x;
        const float dy = p.y - centre.y;
        const float dz = p.z - centre.z;
        return On() && dx * dx + dy * dy + dz * dz <= radius * radius;
    }
};

/// The solid of the town in both moments, with the circle of the lantern.
class Moments
{
  public:
    Moments() = default;
    Moments(const TriMesh* a, const TriMesh* b) : m_mesh{a, b}
    {
    }

    void SetCircle(const Circle& c)
    {
        m_circle = c;
    }
    const Circle& GetCircle() const
    {
        return m_circle;
    }
    /// The solids of the things in `layer` as they are now (scene/Things.h ApplyTo). They must outlive this.
    void SetExtra(int layer, const std::vector<const TriMesh*>& meshes)
    {
        if (layer == 0 || layer == 1)
        {
            m_extra[static_cast<std::size_t>(layer)] = meshes;
        }
    }
    const TriMesh* Mesh(int layer) const
    {
        return layer == 0 || layer == 1 ? m_mesh[static_cast<std::size_t>(layer)] : nullptr;
    }

    /// Moves the centre `c` of a ball of radius `r` out of what is solid in `layer`. Returns how far it moved.
    float PushOut(Vec3& c, float r, int layer) const
    {
        if (layer != 0 && layer != 1)
        {
            return 0.0f;
        }
        const TriMesh* own = m_mesh[static_cast<std::size_t>(layer)];
        const std::vector<const TriMesh*>& extra = m_extra[static_cast<std::size_t>(layer)];
        if (layer == 0 || !m_circle.On())
        {
            if (extra.empty())
            {
                return own != nullptr ? own->PushOut(c, r) : 0.0f;
            }
            // the town and the things one after the other, twice: a ball in a corner between them is out of both
            const Vec3 start = c;
            for (int round = 0; round < 2; ++round)
            {
                if (own != nullptr)
                {
                    own->PushOut(c, r);
                }
                for (const TriMesh* m : extra)
                {
                    m->PushOut(c, r);
                }
            }
            const Vec3 moved = tri::Sub(c, start);
            return std::sqrt(tri::Dot(moved, moved));
        }
        // B with the circle: the triangles of A whose nearest point is inside it, those of B whose nearest point is
        // outside
        const Vec3 start = c;
        for (int round = 0; round < 4; ++round)
        {
            Vec3 best = c;
            float bestD2 = r * r;
            bool hit = false;
            const auto look = [&](const TriMesh* mesh, bool inside)
            {
                if (mesh == nullptr)
                {
                    return;
                }
                mesh->ForEachNear(c.x, c.z, r,
                                  [&](const Vec3& a, const Vec3& b, const Vec3& t)
                                  {
                                      const Vec3 q = tri::ClosestOnTriangle(c, a, b, t);
                                      if (m_circle.Contains(q) != inside)
                                      {
                                          return;
                                      }
                                      const Vec3 d = tri::Sub(c, q);
                                      const float d2 = tri::Dot(d, d);
                                      if (d2 < bestD2)
                                      {
                                          bestD2 = d2;
                                          best = q;
                                          hit = true;
                                      }
                                  });
            };
            look(m_mesh[0], true);
            look(m_mesh[1], false);
            for (const TriMesh* m : m_extra[0])
            {
                look(m, true);
            }
            for (const TriMesh* m : m_extra[1])
            {
                look(m, false);
            }
            if (!hit)
            {
                break;
            }
            const float dist = std::sqrt(bestD2);
            if (dist > 1.0e-6f)
            {
                const Vec3 d = tri::Sub(c, best);
                c = {best.x + d.x * (r / dist), best.y + d.y * (r / dist), best.z + d.z * (r / dist)};
            }
            else
            {
                c = {best.x, best.y + r, best.z}; // exactly on a surface: out upward
            }
        }
        const Vec3 moved = tri::Sub(c, start);
        return std::sqrt(tri::Dot(moved, moved));
    }

    /// The part of the push on a ball that comes from contacts that are not ground: for every triangle of this moment
    /// that the ball overlaps, whose push points up by less than `groundFacing` of its length (a wall, or a slope too
    /// steep to stand on), the push of that contact on the ground plane is added to (wx, wz). Each contact is asked on
    /// its own: the push of a floor that is touched in the same step must not be mixed in, or a wall that is pressed on
    /// while the body rests on a floor would be taken for ground (the sum of a push up and a push back points up when
    /// the floor is the deeper one).
    void WallPush(const Vec3& c, float r, int layer, float groundFacing, float& wx, float& wz) const
    {
        if (layer != 0 && layer != 1)
        {
            return;
        }
        const bool circle = layer == 1 && m_circle.On(); // in the present with the circle of the lantern
        enum Filter
        {
            kAny,
            kInside,
            kOutside
        };
        const auto look = [&](const TriMesh* mesh, Filter filter)
        {
            if (mesh == nullptr)
            {
                return;
            }
            mesh->ForEachNear(c.x, c.z, r,
                              [&](const Vec3& a, const Vec3& b, const Vec3& t)
                              {
                                  const Vec3 q = tri::ClosestOnTriangle(c, a, b, t);
                                  if (filter != kAny && m_circle.Contains(q) != (filter == kInside))
                                  {
                                      return;
                                  }
                                  const Vec3 d = tri::Sub(c, q);
                                  const float d2 = tri::Dot(d, d);
                                  if (!(d2 < r * r) || !(d2 > 1.0e-12f))
                                  {
                                      return;
                                  }
                                  const float dist = std::sqrt(d2);
                                  if (d.y < groundFacing * dist)
                                  {
                                      wx += d.x / dist * (r - dist);
                                      wz += d.z / dist * (r - dist);
                                  }
                              });
        };
        if (!circle)
        {
            look(m_mesh[static_cast<std::size_t>(layer)], kAny);
            for (const TriMesh* m : m_extra[static_cast<std::size_t>(layer)])
            {
                look(m, kAny);
            }
            return;
        }
        // the triangles of A inside the circle, those of B outside it (the same rule as PushOut)
        look(m_mesh[0], kInside);
        look(m_mesh[1], kOutside);
        for (const TriMesh* m : m_extra[0])
        {
            look(m, kInside);
        }
        for (const TriMesh* m : m_extra[1])
        {
            look(m, kOutside);
        }
    }

    /// True when a ball of radius `r` resting with its lowest point at `feet` has something under it that faces up (the
    /// floor it stands on), in `layer`.
    bool Supports(const Vec3& feet, float r, int layer) const
    {
        Vec3 c{feet.x, feet.y + r - 0.08f, feet.z};
        const Vec3 before = c;
        if (PushOut(c, r, layer) <= 1.0e-4f)
        {
            return false;
        }
        const float dx = c.x - before.x;
        const float dy = c.y - before.y;
        const float dz = c.z - before.z;
        return dy > 0.6f * std::sqrt(dx * dx + dy * dy + dz * dz);
    }

    /// The highest floor visible and solid in this moment, including changing things.
    bool FloorAt(float x, float z, float y1, float y0, int layer, float& y) const
    {
        bool found = false;
        const auto scan = [&](const TriMesh* mesh, int source)
        {
            float top = y1;
            float h = 0.0f;
            while (mesh != nullptr && top >= y0 && mesh->HighestBelow(x, z, top, y0, h))
            {
                if (layer == 0 || !m_circle.On() || (source == 0) == m_circle.Contains({x, h, z}))
                {
                    if (!found || h > y)
                    {
                        y = h;
                        found = true;
                    }
                    break;
                }
                top = h - 0.001f;
            }
        };
        const auto scanLayer = [&](int source)
        {
            scan(m_mesh[static_cast<std::size_t>(source)], source);
            for (const TriMesh* mesh : m_extra[static_cast<std::size_t>(source)])
            {
                scan(mesh, source);
            }
        };
        scanLayer(layer);
        if (layer == 1 && m_circle.On())
        {
            scanLayer(0);
        }
        return found;
    }

    bool FloorOfA(float x, float z, float y1, float y0, float& y) const
    {
        return FloorAt(x, z, y1, y0, 0, y);
    }

  private:
    std::array<const TriMesh*, 2> m_mesh{nullptr, nullptr};
    std::array<std::vector<const TriMesh*>, 2> m_extra;
    Circle m_circle;
};

} // namespace dl::town
