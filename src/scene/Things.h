// Things.h - the things that change in the town (docs/DESIGN.md 3.3, 3.5 and 8): a gate raised by its winch, and the
// like.
// Data:      each thing comes from the model (data/town.dlt, scene/Town.h ThingDef): a handle and boxes, each box in
//            one or both moments and in one state (shut or open).
// Verbs:     what the prompt says: OPEN (a gate), PUSH (a crate), LAY ACROSS (a plank), RING (a bell: its only box is a
//            lamp lit in B over its rope), LOWER (the lift of the rock).
// Rule:      the causal rule. E at the handle, in A, standing, opens the thing; it opens in A and in B at once (what is
//            done in A leaves its trace in B). Nothing in B opens anything: at the handle in B, E does nothing. An
//            opening that would put a box of A where the body is, is refused.
// Solid:     the boxes of the state now are solids of their moments (Moments::SetExtra through ApplyTo), so the circle
//            of the lantern treats them as the town's own triangles.
// Drawn:     AddDraw gives each box of the state now as a turned cube of its material's colour in that moment; the
//            handle of a shut thing glows gold in A (gold means use). The boxes of B of an opened thing carry its trace
//            (Traces).
// Plain data and plain maths, no Windows, no Direct3D: tested without a window (tests/test_things.cpp).
// UNITS: metres. Space: left-handed, +Y up.
#pragma once

#include "scene/Lantern.h" // Traces
#include "scene/Moments.h"
#include "scene/SceneData.h"
#include "scene/Town.h"
#include "scene/TriMesh.h"

#include <cmath>
#include <cstddef>
#include <vector>

namespace dl::town
{

inline constexpr float kHandleReach =
    1.6f; ///< UNITS: metres between the feet and a handle (on the ground plane), for E

class Things
{
  public:
    enum class Result
    {
        Nothing,     ///< no handle in reach
        Opened,      ///< opened, in A and B
        AlreadyOpen, ///< in reach, but it is open
        OnlyInPast,  ///< in reach in B: nothing done in B changes anything
        Refused,     ///< opening would put a box of A where the body is
    };

    /// Builds the solids of every thing in every state and moment. `town` must outlive this.
    void Init(const TownData& town)
    {
        m_town = &town;
        const std::size_t n = town.things.size();
        m_open.assign(n, false);
        m_mesh.assign(n * 4, TriMesh{});
        for (std::size_t i = 0; i < n; ++i)
        {
            for (const ThingBox& b : town.things[i].boxes)
            {
                for (int layer = 0; layer < 2; ++layer)
                {
                    if ((b.worlds & (1u << layer)) != 0u)
                    {
                        AddBox(m_mesh[Slot(i, b.state, layer)], b);
                    }
                }
            }
        }
        for (TriMesh& m : m_mesh)
        {
            m.Build();
        }
    }

    std::size_t Count() const
    {
        return m_open.size();
    }
    bool Open(std::size_t i) const
    {
        return i < m_open.size() && m_open[i];
    }
    /// How many bells were rung.
    int BellsRung() const
    {
        int n = 0;
        for (std::size_t i = 0; m_town != nullptr && i < m_open.size(); ++i)
        {
            n += m_open[i] && m_town->things[i].verb == Verb::Ring ? 1 : 0;
        }
        return n;
    }
    int BellCount() const
    {
        int n = 0;
        for (std::size_t i = 0; m_town != nullptr && i < m_town->things.size(); ++i)
        {
            n += m_town->things[i].verb == Verb::Ring ? 1 : 0;
        }
        return n;
    }

    /// Gives `moments` the solids of the things as they are now.
    void ApplyTo(Moments& moments) const
    {
        for (int layer = 0; layer < 2; ++layer)
        {
            std::vector<const TriMesh*> list;
            for (std::size_t i = 0; i < m_open.size(); ++i)
            {
                const TriMesh& m = m_mesh[Slot(i, m_open[i] ? 1 : 0, layer)];
                if (m.TriangleCount() > 0)
                {
                    list.push_back(&m);
                }
            }
            moments.SetExtra(layer, list);
        }
    }

    /// The thing whose handle is in reach of the feet, or -1.
    int InReach(const Vec3& feet) const
    {
        if (m_town == nullptr)
        {
            return -1;
        }
        for (std::size_t i = 0; i < m_town->things.size(); ++i)
        {
            const Vec3& h = m_town->things[i].handle;
            const float dx = feet.x - h.x;
            const float dz = feet.z - h.z;
            if (dx * dx + dz * dz <= kHandleReach * kHandleReach && std::fabs(feet.y + 1.0f - h.y) <= 1.5f)
            {
                return static_cast<int>(i);
            }
        }
        return -1;
    }

    /// E: opens the thing in reach, in A only. `balls` are the centres of the body's balls, `radius` their radius.
    Result Use(const Vec3& feet, const Vec3* balls, int ballCount, float radius, int layer, Moments& moments,
               Traces& traces)
    {
        const int i = InReach(feet);
        if (i < 0)
        {
            return Result::Nothing;
        }
        const std::size_t k = static_cast<std::size_t>(i);
        if (m_open[k])
        {
            return Result::AlreadyOpen;
        }
        if (layer != 0)
        {
            return Result::OnlyInPast;
        }
        const TriMesh& opened = m_mesh[Slot(k, 1, 0)];
        for (int b = 0; b < ballCount; ++b)
        {
            Vec3 c = balls[b];
            if (opened.TriangleCount() > 0 && opened.PushOut(c, radius - 0.03f) > 0.005f)
            {
                return Result::Refused;
            }
        }
        m_open[k] = true;
        traces.Changed(i);
        ApplyTo(moments);
        return Result::Opened;
    }

    /// Every thing shut again (a restart).
    void Reset(Moments& moments, Traces& traces)
    {
        for (std::size_t i = 0; i < m_open.size(); ++i)
        {
            m_open[i] = false;
            traces.Undo(static_cast<int>(i));
        }
        ApplyTo(moments);
    }

    /// The things as drawn: each box of the state now, in the colour of its material in its moment; the trace on the
    /// boxes of B of an opened thing; the gold handle of a shut thing in A. Returns the new count.
    int AddDraw(scene::SceneObject* out, int count, int capacity, const Traces& traces) const
    {
        if (m_town == nullptr || out == nullptr)
        {
            return count;
        }
        for (std::size_t i = 0; i < m_town->things.size(); ++i)
        {
            const ThingDef& t = m_town->things[i];
            const std::uint8_t state = m_open[i] ? 1 : 0;
            for (const ThingBox& b : t.boxes)
            {
                if (b.state != state)
                {
                    continue;
                }
                for (int layer = 0; layer < 2; ++layer)
                {
                    if ((b.worlds & (1u << layer)) == 0u || count >= capacity)
                    {
                        continue;
                    }
                    scene::SceneObject o = Box(b, layer);
                    if (layer == 1 && m_open[i])
                    {
                        traces.Mark(static_cast<int>(i), o);
                    }
                    out[count++] = o;
                }
            }
            if (!m_open[i] && count < capacity)
            {
                scene::SceneObject h{};
                h.shape = scene::ShapeKind::Cube;
                h.position[0] = t.handle.x;
                h.position[1] = t.handle.y;
                h.position[2] = t.handle.z;
                h.scale[0] = h.scale[1] = h.scale[2] = 0.22f;
                h.colorSrgb[0] = 0.96f;
                h.colorSrgb[1] = 0.70f;
                h.colorSrgb[2] = 0.26f;
                h.glow = 1.0f;
                h.layerMask = scene::kLayerA;
                out[count++] = h;
            }
        }
        return count;
    }

  private:
    const TownData* m_town = nullptr;
    std::vector<bool> m_open;
    std::vector<TriMesh> m_mesh; ///< [thing][state][layer]

    static std::size_t Slot(std::size_t thing, int state, int layer)
    {
        return thing * 4 + static_cast<std::size_t>(state) * 2 + static_cast<std::size_t>(layer);
    }

    /// The 8 corners of a box (bottom 0..3, top 4..7), turned like the model's rect.
    static void Corners(const ThingBox& b, Vec3 out[8])
    {
        const float c = std::cos(b.yaw);
        const float s = std::sin(b.yaw);
        const float u[4][2] = {
            {-b.half.x, -b.half.z}, {b.half.x, -b.half.z}, {b.half.x, b.half.z}, {-b.half.x, b.half.z}};
        for (int k = 0; k < 4; ++k)
        {
            const float x = b.centre.x + u[k][0] * c - u[k][1] * s;
            const float z = b.centre.z + u[k][0] * s + u[k][1] * c;
            out[k] = {x, b.centre.y - b.half.y, z};
            out[k + 4] = {x, b.centre.y + b.half.y, z};
        }
    }

    static void AddBox(TriMesh& m, const ThingBox& b)
    {
        Vec3 p[8];
        Corners(b, p);
        const int quads[6][4] = {{0, 1, 2, 3}, {4, 5, 6, 7}, {0, 1, 5, 4}, {1, 2, 6, 5}, {2, 3, 7, 6}, {3, 0, 4, 7}};
        for (const auto& q : quads)
        {
            m.Add(p[q[0]], p[q[1]], p[q[2]]);
            m.Add(p[q[0]], p[q[2]], p[q[3]]);
        }
    }

    scene::SceneObject Box(const ThingBox& b, int layer) const
    {
        scene::SceneObject o{};
        o.shape = scene::ShapeKind::Cube;
        o.position[0] = b.centre.x;
        o.position[1] = b.centre.y;
        o.position[2] = b.centre.z;
        o.scale[0] = 2.0f * b.half.x;
        o.scale[1] = 2.0f * b.half.y;
        o.scale[2] = 2.0f * b.half.z;
        const Material& m = m_town->materials[b.material];
        for (int k = 0; k < 3; ++k)
        {
            o.colorSrgb[k] = layer == 0 ? m.a[k] : m.b[k];
        }
        o.shineStrength = 0.08f;
        o.shininess = 16.0f;
        o.layerMask = layer == 0 ? scene::kLayerA : scene::kLayerB;
        // WHY minus: the model turns x toward +z by its angle (rect); a SceneObject's yaw turns the other way round Y
        // (DirectX, left-handed). All the boxes of the model so far have yaw 0; tests/test_things.cpp keeps it so.
        o.yaw = -b.yaw;
        return o;
    }
};

} // namespace dl::town
