// Canal.h - a canal with a lock: water of layer A whose level a wheel raises, and a crate that floats on it
//            (docs/LEVEL_DESIGN.md, 14.4 and 14.8). The first piece of the east district.
// Rule:      water exists in A only (in B the aqueduct is broken and the canal is dry). Things that move exist in both
//            layers; only where they are differs: the crate floats on the water in A and lies on the bed in B. So the same
//            crate is a step up in A (once the water is high) and a stone on the floor in B.
// Wheel:     the lock wheel is machinery: it turns only for a body in A (in B it has rusted). A ball of the body within
//            kWheelReach of it in A opens the lock, and the water rises at kRiseSpeed to its high level. It stays open until
//            Reset (R in the room puts back the water and the crate, docs/LEVEL_DESIGN.md, 14.7).
// Weir:      a second wheel, on the quay (machinery too: A only). Turned, it lets the current through, and the crate drifts
//            west to the end of the canal (driftX). The crate of B lies under the crate of A, so it drifts with it: in A the
//            crate was moved, in B it is now a step where there was none (docs/LEVEL_DESIGN.md, 14.8, E2).
// Carry:     the crate moves while the water rises or while it drifts; a body standing on the crate of its layer is carried
//            (Player::Carry), as on a lift.
// Drain:     at the end the aqueduct is closed (scene/Stage.h, Waterworks): SetDrained moves the water toward the bed; the
//            crate of A comes down with it and rests on the bed.
// Water:     never solid for the body: feet kSubmerge under the surface in A is a fall (main.cpp brings the body back).
// Plain data and plain logic, no Windows, no Direct3D: tested without a window (tests/test_stage.cpp, TestEastCanal).
// UNITS: metres, seconds. Layers: 0 = A, 1 = B.
// Note:      no std::min / std::max here: windows.h may define them as macros (docs/CONVENTIONS.md).
#pragma once

#include "scene/Camera.h" // Vec3
#include "scene/SceneData.h"
#include "scene/Shapes.h"

#include <cmath>

namespace dl::canal
{

using scene::SceneObject;

struct Desc
{
    float x0, x1, z0, z1; ///< the water surface (the canal between its walls)
    float bed;            ///< world y of the bed
    float lowTop;         ///< world y of the water while the lock is shut
    float highTop;        ///< world y of the water once the lock is open
    Vec3 wheel;           ///< the centre of the lock wheel
    float crateX, crateZ; ///< the centre of the crate
    float crateSize;      ///< UNITS: metres: the crate is a cube
    float draft;          ///< UNITS: metres of the crate above the water
    Vec3 weir;            ///< the centre of the weir wheel
    float driftX;         ///< the x of the centre of the crate once the weir has let it drift
};

inline constexpr float kRiseSpeed = 0.5f;  ///< UNITS: metres per second
inline constexpr float kDriftSpeed = 1.2f; ///< UNITS: metres per second
inline constexpr float kWheelReach = 1.0f; ///< UNITS: metres between a ball of the body and the wheel
inline constexpr float kSubmerge = 0.3f;   ///< UNITS: metres of the feet under the surface that count as in the water
inline constexpr shapes::Rgb kCrateWood{0.55f, 0.38f, 0.22f};

class Canal
{
  public:
    void Init(const Desc& d)
    {
        m_d = d;
        Reset();
    }

    /// The lock shut, the water low, the crate on it.
    void Reset()
    {
        m_open = false;
        m_weirOpen = false;
        m_level = m_d.lowTop;
        m_x = m_d.crateX;
    }

    /// Once per frame, before the solid tables are built: the wheel, the water, the crate. A body standing on the crate of A
    /// is carried with it. `Body` needs Ball(i), Feet(), Grounded() and Carry() (scene/Player.h). Returns true in the frame
    /// in which the wheel was turned.
    template <class Body>
    bool Update(Body& body, int layer, float dt)
    {
        bool turned = false;
        if (layer == 0 && !m_open && Touches(body, m_d.wheel))
        {
            m_open = true;
            turned = true;
        }
        if (layer == 0 && !m_weirOpen && Touches(body, m_d.weir))
        {
            m_weirOpen = true;
            turned = true;
        }
        if (!(dt > 0.0f))
        {
            return turned;
        }
        const float h = dt > 0.1f ? 0.1f : dt;
        const float beforeX = m_x;
        const float beforeTop = layer == 0 ? CrateTopA() : CrateTopB();
        if (m_open && m_level < m_d.highTop)
        {
            const float step = kRiseSpeed * h;
            m_level = m_level + step > m_d.highTop ? m_d.highTop : m_level + step;
        }
        if (m_weirOpen && m_x != m_d.driftX)
        {
            const float step = kDriftSpeed * h;
            const float gap = m_d.driftX - m_x;
            m_x = std::fabs(gap) <= step ? m_d.driftX : m_x + (gap > 0.0f ? step : -step);
        }
        const float afterTop = layer == 0 ? CrateTopA() : CrateTopB();
        if ((m_x != beforeX || afterTop != beforeTop) && OnCrate(body.Feet(), beforeX, beforeTop, body.Grounded()))
        {
            body.Carry({m_x - beforeX, afterTop - beforeTop, 0.0f});
        }
        return turned;
    }

    bool Open() const
    {
        return m_open;
    }
    /// The surface now (the level of the lock, drained toward the bed at the end).
    float WaterTop() const
    {
        return m_level - m_drained * (m_level - m_d.bed);
    }
    /// 0 = as the lock says, 1 = on the bed (the end: scene/Stage.h, Waterworks).
    void SetDrained(float drained)
    {
        m_drained = drained < 0.0f ? 0.0f : (drained > 1.0f ? 1.0f : drained);
    }
    bool WeirOpen() const
    {
        return m_weirOpen;
    }
    /// The x of the centre of the crate now (the same in both layers).
    float CrateX() const
    {
        return m_x;
    }
    bool Drifted() const
    {
        return m_weirOpen && m_x == m_d.driftX;
    }
    bool High() const
    {
        return m_level >= m_d.highTop - 1.0e-4f;
    }
    const Desc& GetDesc() const
    {
        return m_d;
    }
    float CrateTopA() const
    {
        const float floating = WaterTop() + m_d.draft;
        return floating > CrateTopB() ? floating : CrateTopB(); // on the bed once the water is too low to hold it
    }
    float CrateTopB() const
    {
        return m_d.bed + m_d.crateSize;
    }

    /// True when a body whose feet are at `feet` in `layer` is in the water (A only).
    bool Submerged(const Vec3& feet, int layer) const
    {
        return layer == 0 && m_drained < 1.0f && feet.x > m_d.x0 && feet.x < m_d.x1 && feet.z > m_d.z0 && feet.z < m_d.z1 &&
               feet.y < WaterTop() - kSubmerge;
    }

    /// The crate of `layer`: floating in A, on the bed in B.
    SceneObject Crate(int layer) const
    {
        const float h = 0.5f * m_d.crateSize;
        const float top = layer == 0 ? CrateTopA() : CrateTopB();
        SceneObject o = shapes::Box(m_x - h, top - m_d.crateSize, m_d.crateZ - h, m_x + h, top, m_d.crateZ + h, kCrateWood,
                                    layer == 0 ? scene::kLayerA : scene::kLayerB);
        o.glow = 0.04f; // a thing that moves keeps its own colour (not the stone of the city)
        return o;
    }

    /// Adds the crate of `layer` to a SOLID table. Returns the new count.
    int AddSolids(int layer, SceneObject* out, int count, int capacity) const
    {
        if (out != nullptr && count >= 0 && count < capacity && (layer == 0 || layer == 1))
        {
            out[count++] = Crate(layer);
        }
        return count;
    }

    /// Adds to a DRAWN table the water (A), the two crates and the wheel: in A an iron wheel on its post (gold while open), in B
    /// a rusted stump. Returns the new count.
    int AddDraw(SceneObject* out, int count, int capacity) const
    {
        if (out == nullptr || count < 0 || capacity - count < 9)
        {
            return count;
        }
        if (WaterTop() - m_d.bed > 0.02f)
        {
            SceneObject water = shapes::Box(m_d.x0, m_d.bed, m_d.z0, m_d.x1, WaterTop(), m_d.z1, shapes::kWaterColour, scene::kLayerA);
            water.shineStrength = 0.9f;
            water.shininess = 90.0f;
            water.glow = 0.12f;
            out[count++] = water;
        }
        out[count++] = Crate(0);
        out[count++] = Crate(1);
        const Vec3 w = m_d.wheel;
        SceneObject post = shapes::Box(w.x - 0.08f, w.y - 1.1f, w.z - 0.08f, w.x + 0.08f, w.y, w.z + 0.08f, shapes::Rgb{0.30f, 0.32f, 0.36f},
                                       scene::kLayerA);
        post.glow = 0.04f;
        out[count++] = post;
        SceneObject wheel = shapes::Box(w.x - 0.05f, w.y - 0.35f, w.z - 0.35f, w.x + 0.05f, w.y + 0.35f, w.z + 0.35f,
                                        m_open ? shapes::Rgb{1.0f, 0.82f, 0.36f} : shapes::Rgb{0.42f, 0.44f, 0.48f}, scene::kLayerA);
        wheel.glow = m_open ? 0.8f : 0.25f;
        wheel.spinSpeed = 0.0f;
        out[count++] = wheel;
        SceneObject stump = shapes::Box(w.x - 0.1f, w.y - 1.1f, w.z - 0.1f, w.x + 0.1f, w.y - 0.6f, w.z + 0.1f, shapes::Rgb{0.40f, 0.26f, 0.18f},
                                        scene::kLayerB);
        stump.glow = 0.02f;
        out[count++] = stump;
        count = AddWheel(m_d.weir, m_weirOpen, out, count);
        return count;
    }

    /// Adds a thin gold column of light over each wheel not yet turned (both layers, as over a lever: scene/Mechanisms.h), so
    /// that a thing to touch is seen from across the hall. Returns the new count.
    int AddBeacons(SceneObject* out, int count, int capacity) const
    {
        const Vec3 wheels[2] = {m_d.wheel, m_d.weir};
        const bool turned[2] = {m_open, m_weirOpen};
        for (int i = 0; i < 2 && out != nullptr && count >= 0 && count < capacity; ++i)
        {
            if (turned[i])
            {
                continue;
            }
            const Vec3 w = wheels[i];
            SceneObject o = shapes::Box(w.x - 0.03f, w.y + 0.4f, w.z - 0.03f, w.x + 0.03f, w.y + 2.8f, w.z + 0.03f, shapes::Rgb{1.0f, 0.82f, 0.36f},
                                        scene::kLayerBoth);
            o.shineStrength = 0.0f;
            o.glow = 1.6f;
            out[count++] = o;
        }
        return count;
    }

    /// Adds the surface of the water (layer A) to a solid table for ORBS: they float on the canal as on the other waters.
    int AddWaterForOrbs(SceneObject* out, int count, int capacity) const
    {
        if (out != nullptr && count >= 0 && count < capacity && m_drained < 1.0f)
        {
            out[count++] = shapes::Box(m_d.x0, WaterTop() - 0.1f, m_d.z0, m_d.x1, WaterTop(), m_d.z1, shapes::kWaterColour, scene::kLayerA);
        }
        return count;
    }

  private:
    template <class Body>
    static bool Touches(const Body& body, const Vec3& at)
    {
        for (int b = 0; b < 3; ++b)
        {
            const Vec3 p = body.Ball(b);
            const float dx = p.x - at.x;
            const float dy = p.y - at.y;
            const float dz = p.z - at.z;
            if (dx * dx + dy * dy + dz * dz < kWheelReach * kWheelReach)
            {
                return true;
            }
        }
        return false;
    }

    bool OnCrate(const Vec3& feet, float x, float top, bool grounded) const
    {
        const float h = 0.5f * m_d.crateSize + 0.3f;
        return grounded && std::fabs(feet.x - x) < h && std::fabs(feet.z - m_d.crateZ) < h && std::fabs(feet.y - top) < 0.15f;
    }

    /// The weir wheel: an iron wheel on its post in A (gold once turned), a rusted stump in B. Needs 3 places.
    int AddWheel(const Vec3& w, bool open, SceneObject* out, int count) const
    {
        SceneObject post = shapes::Box(w.x - 0.08f, w.y - 1.1f, w.z - 0.08f, w.x + 0.08f, w.y, w.z + 0.08f, shapes::Rgb{0.30f, 0.32f, 0.36f},
                                       scene::kLayerA);
        post.glow = 0.04f;
        out[count++] = post;
        SceneObject wheel = shapes::Box(w.x - 0.35f, w.y - 0.35f, w.z - 0.05f, w.x + 0.35f, w.y + 0.35f, w.z + 0.05f,
                                        open ? shapes::Rgb{1.0f, 0.82f, 0.36f} : shapes::Rgb{0.42f, 0.44f, 0.48f}, scene::kLayerA);
        wheel.glow = open ? 0.8f : 0.25f;
        out[count++] = wheel;
        SceneObject stump = shapes::Box(w.x - 0.1f, w.y - 1.1f, w.z - 0.1f, w.x + 0.1f, w.y - 0.6f, w.z + 0.1f, shapes::Rgb{0.40f, 0.26f, 0.18f},
                                        scene::kLayerB);
        stump.glow = 0.02f;
        out[count++] = stump;
        return count;
    }

    Desc m_d{};
    float m_level = 0.0f;
    float m_x = 0.0f;
    float m_drained = 0.0f;
    bool m_open = false;
    bool m_weirOpen = false;
};

} // namespace dl::canal
