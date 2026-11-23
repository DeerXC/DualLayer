// LockCrate.h - the crate of the tower lock (docs/LEVEL_DESIGN.md 6.4.2, E3: "pushed in B, lifted in A").
// Place:     one crate, in both worlds, that slides along z on the bed of the tower lock between kCrateZMax (its start, by the
//            terrace) and kCrateZMin (under the door of the water tower), a whole step at a time.
// Push:      only in B, by a body standing on the dry bed and walking into its north or its south face for kPushSeconds; it
//            then slides one step away from the body in kSlideSeconds. In A it floats and is held by its mooring.
// Height:    in B it lies on the bed; in A it floats on the water of the lock (its top kFreeboard over the surface: the
//            Waterworks of A raise and drain it), so it is the same crate at another height: under the door, a step to it.
// R:         R in the tower lock puts it back at its start.
// Plain logic, no Windows, no Direct3D: tested without a window (tests/test_stage.cpp, TestTowerLock). UNITS: metres, seconds.
#pragma once

#include "scene/Canal.h"
#include "scene/SceneData.h"
#include "scene/Shapes.h"
#include "scene/Stage.h"

#include <cmath>

namespace dl::stage
{

class LockCrate
{
  public:
    static constexpr float kSize = 1.2f;         ///< UNITS: metres
    static constexpr float kX = 59.5f;           ///< its x: under the door
    static constexpr float kZMax = 29.6f;        ///< its start (north face 30.2: a body fits between it and the terrace wall)
    static constexpr float kZMin = kBasinZ0 + 0.6f; ///< under the door (south face on the wall)
    static constexpr float kStep = 1.0f;
    static constexpr float kPushSeconds = 0.2f;
    static constexpr float kSlideSeconds = 0.3f;
    static constexpr float kFreeboard = 0.5f;   ///< UNITS: metres of the floating crate over the water (A)
    static constexpr float kTouch = 0.12f;      ///< UNITS: metres between the body and a face that counts as pushing

    void Reset()
    {
        m_z = kZMax;
        m_from = kZMax;
        m_to = kZMax;
        m_slide = -1.0f;
        m_contact = 0.0f;
        m_lastSign = 0;
        m_haveLast = false;
    }

    /// The surface of the water of A now (from Waterworks::Top of kTowerLockWaters[0]); at or under the bed: no water.
    void SetWaterTop(float y)
    {
        m_water = y;
    }

    float Z() const
    {
        return m_z;
    }
    bool Sliding() const
    {
        return m_slide >= 0.0f;
    }
    bool UnderDoor() const
    {
        return !Sliding() && std::fabs(m_z - kZMin) < 1.0e-3f;
    }
    float TopB() const
    {
        return kTowerLockBed + kSize;
    }
    float TopA() const
    {
        const float floating = m_water + kFreeboard;
        return floating > TopB() ? floating : TopB();
    }

    /// One step. `body` needs Feet(), Grounded(), and its radius; `layer` is the world of the body.
    template <class Body>
    void Update(const Body& body, int layer, float radius, float dt)
    {
        if (!(dt > 0.0f))
        {
            return;
        }
        if (Sliding())
        {
            m_slide += dt / kSlideSeconds;
            const float t = m_slide >= 1.0f ? 1.0f : m_slide;
            m_z = m_from + (m_to - m_from) * t;
            if (m_slide >= 1.0f)
            {
                m_z = m_to;
                m_slide = -1.0f;
            }
            m_contact = 0.0f;
            return;
        }
        const Vec3 f = body.Feet();
        const float h = 0.5f * kSize;
        const bool onBed = layer == 1 && body.Grounded() && std::fabs(f.y - kTowerLockBed) < 0.1f && std::fabs(f.x - kX) < h + 0.1f;
        // the sign of the push: +1 the body is north of the crate and walks south (the crate goes to -z), -1 the other way
        int sign = 0;
        if (onBed)
        {
            const float north = m_z + h + radius;
            const float south = m_z - h - radius;
            const float dz = m_haveLast ? f.z - m_lastZ : 0.0f;
            if (f.z >= north - 0.02f && f.z <= north + kTouch && dz < -0.002f)
            {
                sign = 1;
            }
            else if (f.z <= south + 0.02f && f.z >= south - kTouch && dz > 0.002f)
            {
                sign = -1;
            }
            // pressed against a face it cannot move (the body is stopped): still a push
            if (sign == 0 && m_haveLast && std::fabs(dz) <= 0.002f && m_lastSign != 0)
            {
                if ((m_lastSign > 0 && f.z >= north - 0.02f && f.z <= north + kTouch) || (m_lastSign < 0 && f.z <= south + 0.02f && f.z >= south - kTouch))
                {
                    sign = m_lastSign;
                }
            }
        }
        m_lastZ = f.z;
        m_haveLast = true;
        if (sign == 0)
        {
            m_contact = 0.0f;
            m_lastSign = 0;
            return;
        }
        m_contact = sign == m_lastSign ? m_contact + dt : dt;
        m_lastSign = sign;
        const float next = m_z - static_cast<float>(sign) * kStep;
        if (m_contact >= kPushSeconds && next >= kZMin - 1.0e-3f && next <= kZMax + 1.0e-3f)
        {
            m_from = m_z;
            m_to = next < kZMin ? kZMin : (next > kZMax ? kZMax : next);
            m_slide = 0.0f;
            m_contact = 0.0f;
        }
    }

    /// The crate as a solid of `layer`.
    SceneObject Solid(int layer) const
    {
        const float h = 0.5f * kSize;
        const float top = layer == 0 ? TopA() : TopB();
        SceneObject o = shapes::Box(kX - h, top - kSize, m_z - h, kX + h, top, m_z + h, canal::kCrateWood, layer == 0 ? scene::kLayerA : scene::kLayerB);
        o.glow = 0.04f;
        o.shineStrength = 0.08f;
        return o;
    }

    /// Adds it to a solid table of `layer`. Returns the new count.
    int AddSolids(int layer, SceneObject* out, int count, int capacity) const
    {
        if (out != nullptr && count >= 0 && count < capacity && (layer == 0 || layer == 1))
        {
            out[count++] = Solid(layer);
        }
        return count;
    }

    /// Adds it to a drawn table: its body in each world and an iron band round each. Returns the new count.
    int AddDraw(SceneObject* out, int count, int capacity) const
    {
        for (int layer = 0; layer < 2 && out != nullptr && count + 2 <= capacity; ++layer)
        {
            const SceneObject c = Solid(layer);
            out[count++] = c;
            SceneObject band = c;
            band.scale[0] += 0.04f;
            band.scale[2] += 0.04f;
            band.scale[1] = 0.08f;
            band.colorSrgb[0] = 0.24f;
            band.colorSrgb[1] = 0.22f;
            band.colorSrgb[2] = 0.21f;
            out[count++] = band;
        }
        return count;
    }

  private:
    float m_z = kZMax;
    float m_from = kZMax;
    float m_to = kZMax;
    float m_slide = -1.0f;
    float m_contact = 0.0f;
    float m_water = kTowerLockWater;
    float m_lastZ = 0.0f;
    int m_lastSign = 0;
    bool m_haveLast = false;
};

} // namespace dl::stage
