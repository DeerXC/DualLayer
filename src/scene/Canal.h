// Canal.h - a canal with a lock: water of layer A whose level a wheel raises, and a crate that floats on it
//            (docs/LEVEL_DESIGN.md, 14.4 and 14.8). The first piece of the east district.
// Rule:      water exists in A only (in B the aqueduct is broken and the canal is dry). Things that move exist in both
//            layers; only where they are differs: the crate floats on the water in A and lies on the bed in B. So the same
//            crate is a step up in A (once the water is high) and a stone on the floor in B.
// Wheel:     the lock wheel is machinery: it turns only for a body in A (in B it has rusted). A ball of the body within
//            kWheelReach of it in A opens the lock, and the water rises at kRiseSpeed to its high level. It stays open until
//            Reset (R in the room puts back the water and the crate, docs/LEVEL_DESIGN.md, 14.7).
// Carry:     the crate of A moves while the water rises; a body standing on it in A is carried (Player::Carry), as on a lift.
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
};

inline constexpr float kRiseSpeed = 0.5f;  ///< UNITS: metres per second
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
        m_level = m_d.lowTop;
    }

    /// Once per frame, before the solid tables are built: the wheel, the water, the crate. A body standing on the crate of A
    /// is carried with it. `Body` needs Ball(i), Feet(), Grounded() and Carry() (scene/Player.h). Returns true in the frame
    /// in which the wheel was turned.
    template <class Body>
    bool Update(Body& body, int layer, float dt)
    {
        bool turned = false;
        if (!m_open && layer == 0)
        {
            for (int b = 0; b < 3 && !m_open; ++b)
            {
                const Vec3 p = body.Ball(b);
                const float dx = p.x - m_d.wheel.x;
                const float dy = p.y - m_d.wheel.y;
                const float dz = p.z - m_d.wheel.z;
                if (dx * dx + dy * dy + dz * dz < kWheelReach * kWheelReach)
                {
                    m_open = true;
                    turned = true;
                }
            }
        }
        if (!(dt > 0.0f) || !m_open || m_level >= m_d.highTop)
        {
            return turned;
        }
        const float before = CrateTopA();
        const float step = kRiseSpeed * (dt > 0.1f ? 0.1f : dt);
        m_level = m_level + step > m_d.highTop ? m_d.highTop : m_level + step;
        const Vec3 f = body.Feet();
        if (layer == 0 && OnCrate(f, before, body.Grounded()))
        {
            body.Carry({0.0f, CrateTopA() - before, 0.0f});
        }
        return turned;
    }

    bool Open() const
    {
        return m_open;
    }
    float WaterTop() const
    {
        return m_level;
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
        return m_level + m_d.draft;
    }
    float CrateTopB() const
    {
        return m_d.bed + m_d.crateSize;
    }

    /// True when a body whose feet are at `feet` in `layer` is in the water (A only).
    bool Submerged(const Vec3& feet, int layer) const
    {
        return layer == 0 && feet.x > m_d.x0 && feet.x < m_d.x1 && feet.z > m_d.z0 && feet.z < m_d.z1 && feet.y < m_level - kSubmerge;
    }

    /// The crate of `layer`: floating in A, on the bed in B.
    SceneObject Crate(int layer) const
    {
        const float h = 0.5f * m_d.crateSize;
        const float top = layer == 0 ? CrateTopA() : CrateTopB();
        SceneObject o = shapes::Box(m_d.crateX - h, top - m_d.crateSize, m_d.crateZ - h, m_d.crateX + h, top, m_d.crateZ + h, kCrateWood,
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
        if (out == nullptr || count < 0 || capacity - count < 6)
        {
            return count;
        }
        SceneObject water = shapes::Box(m_d.x0, m_d.bed, m_d.z0, m_d.x1, m_level, m_d.z1, shapes::kWaterColour, scene::kLayerA);
        water.shineStrength = 0.9f;
        water.shininess = 90.0f;
        water.glow = 0.12f;
        out[count++] = water;
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
        return count;
    }

  private:
    bool OnCrate(const Vec3& feet, float top, bool grounded) const
    {
        const float h = 0.5f * m_d.crateSize + 0.3f;
        return grounded && std::fabs(feet.x - m_d.crateX) < h && std::fabs(feet.z - m_d.crateZ) < h && std::fabs(feet.y - top) < 0.15f;
    }

    Desc m_d{};
    float m_level = 0.0f;
    bool m_open = false;
};

} // namespace dl::canal
