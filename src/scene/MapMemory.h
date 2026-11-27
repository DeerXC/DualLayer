// MapMemory.h - what the player has seen of the town, and the markers they left (docs/DESIGN.md 10.4).
// Seen:      a cell of the plan (scene/Town.h Plan) is seen once the body has been within `revealRadius` metres of it,
//            or has stood at an overlook of the model (a high point: the bastions, the plaza, the top row of the
//            theatre, the graveyard) and so looked over the town from there. The rest of the map is blank paper: the
//            map never shows the position of what the player has not seen.
// Markers:   up to kMaxMarkers; each is a place on the ground with one of kMarkerTypes shapes and the moment (A or B)
//            it was set in. Marking a place that already has a marker takes it away; an eleventh marker takes the
//            oldest one's place. No text.
// Plain data and plain maths, no Windows, no Direct3D: tested without a window (tests/test_map.cpp).
// UNITS: metres. Space: left-handed, +Y up (the plan is seen from above: x east, z north).
#pragma once

#include "scene/Camera.h" // Vec3
#include "scene/Town.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

namespace dl::town
{

struct MapMarker
{
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    int type = 0;  ///< the shape: 0 .. kMarkerTypes - 1
    int layer = 0; ///< the moment it was set in: 0 = A, 1 = B
};

class MapMemory
{
  public:
    static constexpr int kMaxMarkers = 10;
    static constexpr int kMarkerTypes = 6;
    static constexpr float kOverlookReach = 6.0f; ///< UNITS: metres: how near an overlook the body must stand

    /// Forgets everything and takes the size of `plan`. `revealRadius`: metres round the body that are seen.
    void Init(const Plan& plan, float revealRadius)
    {
        m_x0 = plan.x0;
        m_z0 = plan.z0;
        m_cell = plan.cell > 0.0f ? plan.cell : 1.0f;
        m_nx = plan.nx;
        m_nz = plan.nz;
        m_seen.assign(static_cast<std::size_t>(std::max(m_nx, 0)) * static_cast<std::size_t>(std::max(m_nz, 0)), 0);
        m_count = 0;
        m_i0 = m_nx;
        m_i1 = -1;
        m_j0 = m_nz;
        m_j1 = -1;
        m_markers.clear();
        SetRevealRadius(revealRadius);
    }

    void SetRevealRadius(float radius)
    {
        m_radius = std::isfinite(radius) ? std::clamp(radius, 3.0f, 80.0f) : 15.0f;
    }

    float RevealRadius() const
    {
        return m_radius;
    }

    /// Marks the cells whose centre is within `radius` of (x, z) as seen.
    void Reveal(float x, float z, float radius)
    {
        if (m_seen.empty() || !std::isfinite(x) || !std::isfinite(z) || !(radius > 0.0f))
        {
            return;
        }
        const int i0 = std::max(0, static_cast<int>(std::floor((x - radius - m_x0) / m_cell)));
        const int i1 = std::min(m_nx - 1, static_cast<int>(std::floor((x + radius - m_x0) / m_cell)));
        const int j0 = std::max(0, static_cast<int>(std::floor((z - radius - m_z0) / m_cell)));
        const int j1 = std::min(m_nz - 1, static_cast<int>(std::floor((z + radius - m_z0) / m_cell)));
        for (int j = j0; j <= j1; ++j)
        {
            for (int i = i0; i <= i1; ++i)
            {
                const float dx = m_x0 + (static_cast<float>(i) + 0.5f) * m_cell - x;
                const float dz = m_z0 + (static_cast<float>(j) + 0.5f) * m_cell - z;
                std::uint8_t& cell =
                    m_seen[static_cast<std::size_t>(j) * static_cast<std::size_t>(m_nx) + static_cast<std::size_t>(i)];
                if (dx * dx + dz * dz <= radius * radius && cell == 0)
                {
                    cell = 1;
                    ++m_count;
                    m_i0 = std::min(m_i0, i);
                    m_i1 = std::max(m_i1, i);
                    m_j0 = std::min(m_j0, j);
                    m_j1 = std::max(m_j1, j);
                }
            }
        }
    }

    /// Once a frame: the body at `feet` sees what is round it, and what the overlooks it stands at look over.
    void Update(const TownData& town, const Vec3& feet)
    {
        Reveal(feet.x, feet.z, m_radius);
        for (const Overlook& o : town.overlooks)
        {
            const float dx = feet.x - o.at.x;
            const float dz = feet.z - o.at.z;
            if (dx * dx + dz * dz <= kOverlookReach * kOverlookReach && std::fabs(feet.y - o.at.y) <= kOverlookReach)
            {
                Reveal(o.at.x, o.at.z, o.radius);
            }
        }
    }

    bool Seen(int i, int j) const
    {
        return i >= 0 && j >= 0 && i < m_nx && j < m_nz &&
               m_seen[static_cast<std::size_t>(j) * static_cast<std::size_t>(m_nx) + static_cast<std::size_t>(i)] != 0;
    }

    int SeenCount() const
    {
        return m_count;
    }

    /// The box of the cells seen (inclusive); false when none has been.
    bool SeenBounds(int& i0, int& i1, int& j0, int& j1) const
    {
        if (m_count == 0)
        {
            return false;
        }
        i0 = m_i0;
        i1 = m_i1;
        j0 = m_j0;
        j1 = m_j1;
        return true;
    }

    // ---- the markers
    int MarkerCount() const
    {
        return static_cast<int>(m_markers.size());
    }

    const MapMarker& Marker(int k) const
    {
        return m_markers[static_cast<std::size_t>(k)];
    }

    /// A marker where there is none within `takeAwayWithin` metres (of m on the ground plane); else the nearest one
    /// there is taken away. True when one was placed. An eleventh marker takes the place of the oldest.
    bool ToggleMarker(const MapMarker& m, float takeAwayWithin)
    {
        if (!std::isfinite(m.x) || !std::isfinite(m.z))
        {
            return false;
        }
        int nearest = -1;
        float best = takeAwayWithin * takeAwayWithin;
        for (int k = 0; k < MarkerCount(); ++k)
        {
            const float dx = m_markers[static_cast<std::size_t>(k)].x - m.x;
            const float dz = m_markers[static_cast<std::size_t>(k)].z - m.z;
            if (dx * dx + dz * dz <= best)
            {
                best = dx * dx + dz * dz;
                nearest = k;
            }
        }
        if (nearest >= 0)
        {
            m_markers.erase(m_markers.begin() + nearest);
            return false;
        }
        if (MarkerCount() >= kMaxMarkers)
        {
            m_markers.erase(m_markers.begin());
        }
        MapMarker mark = m;
        mark.type = std::clamp(mark.type, 0, kMarkerTypes - 1);
        mark.layer = mark.layer == 1 ? 1 : 0;
        m_markers.push_back(mark);
        return true;
    }

    void ClearMarkers()
    {
        m_markers.clear();
    }

  private:
    float m_x0 = 0.0f;
    float m_z0 = 0.0f;
    float m_cell = 1.0f;
    float m_radius = 15.0f;
    int m_nx = 0;
    int m_nz = 0;
    int m_count = 0;
    int m_i0 = 0;
    int m_i1 = -1;
    int m_j0 = 0;
    int m_j1 = -1;
    std::vector<std::uint8_t> m_seen;
    std::vector<MapMarker> m_markers;
};

} // namespace dl::town
