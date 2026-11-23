// CityState.h - what the bells have changed in the city (docs/LEVEL_DESIGN.md, 1 and 4), and the memory
//            fragments that the hidden lights bring back (section 8).
// Bells:     three: dawn, day and night. In the current build each one is rung by taking the seal of a wing: the east seal is
//            the dawn bell, the west seal the day bell, the north seal the night bell (kSealOfBell). The night bell is also rung
//            where it hangs, in the graveyard, the dawn bell in the water tower and the day bell at the foot of the well (RingAt). What a bell changes, it
//            changes for good and in both worlds, and never on the main path (any order of the bells finishes the game):
//              dawn   the master sluice of the aqueduct opens: every water of A stands kDawnRise higher (Waterworks::SetRaise)
//              day    the machinery of the city turns by itself: the lock of the canal opens without its wheel
//              night  night falls in A (Models::SetNight eases the theme of A to a night theme)
//            The names of the bells wait for the story (docs/LEVEL_DESIGN.md, 1); the rules do not depend on them.
// Fragments: a piece of A that appears in B, in pale glowing stone, once its hidden light is taken; solid in B. Always a
//            shortcut or a view, never a part of a puzzle: tests/test_reach.cpp searches the map with every fragment present.
//              the steps of the fountain (the light on the floor of the pool): a third stair out of the pool, south-west
//              the bench by the gate (the light inside the gate): a stone bench beside the breach, a place to look back
// Plain data and plain logic, no Windows, no Direct3D: tested without a window (tests/test_stage.cpp, TestCityState).
// UNITS: metres, seconds.
#pragma once

#include "scene/Shapes.h"
#include "scene/Stage.h"
#include "scene/StageLights.h"

#include <array>
#include <cstddef>
#include <initializer_list>
#include <vector>

namespace dl::stage
{

enum class Bell
{
    Dawn,
    Day,
    Night,
};
inline constexpr int kBellCount = 3;
/// The seal (kSeals) that rings each bell in the current build: east = dawn, west = day, north = night.
inline constexpr int kSealOfBell[kBellCount] = {1, 0, 2};
inline constexpr float kDawnRise = 0.3f;    ///< UNITS: metres: how much higher the waters of A stand after the dawn bell
inline constexpr float kRiseSeconds = 3.0f; ///< UNITS: seconds the waters take to rise
inline constexpr float kNightSeconds = 6.0f; ///< UNITS: seconds the night takes to fall

class CityState
{
  public:
    /// The bells rung, read from the seals taken (a bell rung where it hangs stays rung: Ring).
    void FromSeals(const SealSet& seals)
    {
        for (int b = 0; b < kBellCount; ++b)
        {
            m_rung[static_cast<std::size_t>(b)] = m_rung[static_cast<std::size_t>(b)] || seals.Taken(kSealOfBell[b]);
        }
    }

    /// A bell rung where it hangs (the night bell of the graveyard). True the first time.
    bool Ring(Bell b)
    {
        const bool first = !Rung(b);
        m_rung[static_cast<std::size_t>(b)] = true;
        return first;
    }

    /// The bells that hang where a body reaches them: the night bell of the graveyard and the dawn bell of the water tower. Rung
    /// by a body whose feet come within kBellReach of the place under it. True the first time a bell rings.
    template <class Body>
    bool RingAt(const Body& body)
    {
        const Vec3 f = body.Feet();
        const auto under = [&](const Vec3& p) {
            const float dx = f.x - p.x;
            const float dz = f.z - p.z;
            return dx * dx + dz * dz < kBellReach * kBellReach && f.y > p.y - 0.5f && f.y < p.y + 0.5f;
        };
        bool first = false;
        if (under(kNightBell))
        {
            first = Ring(Bell::Night) || first;
        }
        if (under(kDawnBell))
        {
            first = Ring(Bell::Dawn) || first;
        }
        if (under(kDayBell))
        {
            first = Ring(Bell::Day) || first;
        }
        return first;
    }

    bool Rung(Bell b) const
    {
        return m_rung[static_cast<std::size_t>(b)];
    }
    int Count() const
    {
        int n = 0;
        for (bool r : m_rung)
        {
            n += r ? 1 : 0;
        }
        return n;
    }

    /// What the bells ask for, once they have done it.
    float WaterRise() const
    {
        return Rung(Bell::Dawn) ? kDawnRise : 0.0f;
    }
    bool Machinery() const
    {
        return Rung(Bell::Day);
    }
    bool Night() const
    {
        return Rung(Bell::Night);
    }

    /// Eases the rise of the water and the fall of the night toward what the bells ask for.
    void Update(float dt)
    {
        if (!(dt > 0.0f))
        {
            return;
        }
        const float h = dt > 0.1f ? 0.1f : dt;
        const float rise = WaterRise();
        const float step = kDawnRise * h / kRiseSeconds;
        m_rise = m_rise < rise ? (m_rise + step > rise ? rise : m_rise + step) : rise;
        const float night = Night() ? 1.0f : 0.0f;
        const float nightStep = h / kNightSeconds;
        m_night = m_night < night ? (m_night + nightStep > night ? night : m_night + nightStep) : night;
    }
    /// UNITS: metres the waters stand higher now.
    float RiseNow() const
    {
        return m_rise;
    }
    /// 0 = the evening of A, 1 = its night.
    float NightNow() const
    {
        return m_night;
    }

  private:
    std::array<bool, kBellCount> m_rung{};
    float m_rise = 0.0f;
    float m_night = 0.0f;
};

// ---- memory fragments

inline constexpr Rgb kMemoryStone{0.82f, 0.90f, 1.00f};
inline constexpr float kMemoryGlow = 0.35f;

struct Fragment
{
    int light;        ///< the hidden light (kHiddenLights) that brings it back
    const char* name; ///< for the log
};
inline constexpr Fragment kFragments[] = {
    {1, "the steps of the fountain"},
    {0, "the bench by the gate"},
};
inline constexpr int kFragmentCount = static_cast<int>(sizeof(kFragments) / sizeof(kFragments[0]));

/// The boxes of fragment `f`: what is solid (in B) and what is only drawn (in B). Both in pale glowing stone.
inline void FragmentBoxes(int f, std::vector<SceneObject>& solid, std::vector<SceneObject>& drawn)
{
    const std::size_t solidFirst = solid.size();
    const std::size_t drawnFirst = drawn.size();
    if (f == 0) // a stair in the south-west corner of the pool, from the rim down to its floor
    {
        shapes::Stairs(kPoolWest, kPoolWest + kStairWidth, kPoolSouth, kFloorY, kPoolSouth + kStairRun, kSquareLowY, shapes::kStoneLight, solid, drawn);
    }
    else if (f == 1) // a stone bench beside the breach in the town wall, on its north side
    {
        solid.push_back(Span(1.7f, kTerraceY, kGateZ + 0.5f, 2.8f, kTerraceY + 0.45f, kGateZ + 1.1f, kMemoryStone, kLayerB));
    }
    for (std::vector<SceneObject>* v : {&solid, &drawn})
    {
        for (std::size_t i = v == &solid ? solidFirst : drawnFirst; i < v->size(); ++i)
        {
            SceneObject& o = (*v)[i];
            o.layerMask = kLayerB;
            o.colorSrgb[0] = kMemoryStone.r;
            o.colorSrgb[1] = kMemoryStone.g;
            o.colorSrgb[2] = kMemoryStone.b;
            o.glow = kMemoryGlow;
        }
    }
}

/// Every fragment, made once: its solid boxes and its drawn boxes.
struct FragmentSet
{
    std::vector<SceneObject> solid[kFragmentCount];
    std::vector<SceneObject> drawn[kFragmentCount];
};
inline const FragmentSet& Fragments()
{
    static const FragmentSet set = [] {
        FragmentSet s;
        for (int f = 0; f < kFragmentCount; ++f)
        {
            FragmentBoxes(f, s.solid[f], s.drawn[f]);
        }
        return s;
    }();
    return set;
}

/// Adds the fragments whose light is taken to a SOLID table (`drawnToo`: also their drawn steps, for a DRAWN table). Returns the
/// new count.
inline int AddFragments(const LightSet& lights, bool drawnToo, SceneObject* out, int count, int capacity)
{
    const FragmentSet& set = Fragments();
    for (int f = 0; f < kFragmentCount; ++f)
    {
        if (!lights.Taken(kFragments[f].light))
        {
            continue;
        }
        for (const SceneObject& o : set.solid[f])
        {
            if (out != nullptr && count >= 0 && count < capacity)
            {
                out[count++] = o;
            }
        }
        for (const SceneObject& o : set.drawn[f])
        {
            if (drawnToo && out != nullptr && count >= 0 && count < capacity)
            {
                out[count++] = o;
            }
        }
    }
    return count;
}

} // namespace dl::stage
