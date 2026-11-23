// LampTravel.h - lamp travel (docs/LEVEL_DESIGN.md 1.3): after the first bell, at a lit anchor lamp, E opens the list of the lit
//            lamps; A and D choose, E travels, W, S or Space close it. The body arrives standing beside the chosen lamp (its
//            place of return: kAnchors stand), in the layer it is in, with the ring of a respawn. A lamp whose place is not free
//            in that layer (`free` says so) is passed over when choosing.
// While the list is open the body does not walk (main.cpp gives it no keys); the list closes by itself when a lamp is
//            chosen or the body is no longer at the lamp it opened from (a fall, R).
// Plain logic, no Windows, no Direct3D: tested without a window (tests/test_stage.cpp, TestLampTravel). UNITS: metres.
#pragma once

#include "scene/Anchors.h"
#include "scene/EndScreen.h"
#include "scene/Stage.h"

#include <cstdio>

namespace dl::stage
{

inline constexpr float kTravelReach = 1.6f; ///< UNITS: metres between the feet and the place beside a lamp, to open the list

class LampTravel
{
  public:
    /// The lit lamp the feet stand at (within kTravelReach of its place, at its height), or -1.
    static int LampAt(const AnchorSet& anchors, const Vec3& feet)
    {
        for (int i = 0; i < kAnchorCount; ++i)
        {
            const Vec3 s = kAnchors[i].stand;
            const float dx = feet.x - s.x;
            const float dz = feet.z - s.z;
            if (anchors.Lit(i) && dx * dx + dz * dz < kTravelReach * kTravelReach && feet.y > s.y - 0.5f && feet.y < s.y + 0.5f)
            {
                return i;
            }
        }
        return -1;
    }

    /// E at a lit lamp with at least one bell rung and another lit lamp to go to: the list opens on the next lit lamp after
    /// this one. Returns true when it opened.
    template <class Free>
    bool TryOpen(const AnchorSet& anchors, const Vec3& feet, int bellsRung, Free free)
    {
        if (m_open || bellsRung < 1)
        {
            return false;
        }
        const int at = LampAt(anchors, feet);
        if (at < 0)
        {
            return false;
        }
        m_from = at;
        m_selected = at;
        m_open = true;
        Step(anchors, +1, free);
        if (m_selected == at)
        {
            m_open = false; // no other lamp to go to
        }
        return m_open;
    }

    /// A or D: the next (+1) or the previous (-1) lit lamp whose place is free, never the lamp it opened from.
    template <class Free>
    void Step(const AnchorSet& anchors, int direction, Free free)
    {
        if (!m_open)
        {
            return;
        }
        int i = m_selected;
        for (int n = 0; n < kAnchorCount; ++n)
        {
            i = (i + (direction >= 0 ? 1 : kAnchorCount - 1)) % kAnchorCount;
            if (i != m_from && anchors.Lit(i) && free(kAnchors[i].stand))
            {
                m_selected = i;
                return;
            }
        }
    }

    /// E while open: the place to arrive at (and the list closes).
    Vec3 Go()
    {
        m_open = false;
        return kAnchors[m_selected].stand;
    }

    void Close()
    {
        m_open = false;
    }

    /// Closes the list when the body is no longer at the lamp it opened from.
    void Follow(const AnchorSet& anchors, const Vec3& feet)
    {
        if (m_open && LampAt(anchors, feet) != m_from)
        {
            m_open = false;
        }
    }

    bool Open() const
    {
        return m_open;
    }
    int Selected() const
    {
        return m_selected;
    }

    /// The panel of the list (render/OverlayBatch draws it like the end screen).
    endscreen::Screen Panel() const
    {
        endscreen::Screen s;
        std::snprintf(s.lines[0], endscreen::kLineLength, "LAMPS");
        std::snprintf(s.lines[1], endscreen::kLineLength, "< %.24s >", m_selected >= 0 ? kAnchors[m_selected].name : "");
        std::snprintf(s.lines[2], endscreen::kLineLength, "A D CHOOSE");
        std::snprintf(s.lines[3], endscreen::kLineLength, "E GO");
        std::snprintf(s.lines[4], endscreen::kLineLength, "W S SPACE CLOSE");
        s.count = endscreen::kLines;
        return s;
    }

  private:
    bool m_open = false;
    int m_from = -1;
    int m_selected = -1;
};

} // namespace dl::stage
