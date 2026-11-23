// Ending.h - the last moment and the three endings (docs/LEVEL_DESIGN.md 1.5). Reaching the goal light at the top of the
//            cathedral starts the last moment: the great bell strikes three times, kStrokeSeconds apart. The body may still
//            switch worlds between the strokes. At the third stroke the ending is decided:
//              Past     standing in A: the held moment is kept (the waters of A stay)
//              Present  standing in B: time moves on (the master sluice drains A: Waterworks::Close)
//              Both     the three bells rung and Tab held through the third stroke: as Present, and what the memory lights
//                       brought back stays (the fragments: scene/CityState.h)
//            After the end screen, R plays the last moment again from the balcony (Reset), so every ending is seen without
//            playing the map again.
// The hidden floor of the tower (T4) that the plan asks for the third ending is not built: the three bells are the condition.
// Plain logic, no Windows, no Direct3D: tested without a window (tests/test_stage.cpp, TestLastMoment). UNITS: seconds.
#pragma once

namespace dl::stage
{

enum class EndingKind
{
    None,
    Past,
    Present,
    Both,
};

class Ending
{
  public:
    static constexpr int kStrokes = 3;
    static constexpr float kStrokeSeconds = 1.2f; ///< UNITS: seconds between two strokes (the first comes after one)

    /// The goal is reached: the last moment begins (once; Reset to play it again).
    void Start()
    {
        if (!m_running && m_kind == EndingKind::None)
        {
            m_running = true;
            m_seconds = 0.0f;
            m_strokes = 0;
        }
    }

    /// Advances the last moment. Returns the stroke that sounds in this step (1 .. 3), or 0. At the third the ending is
    /// decided from `layer` (0 A, 1 B), `tabHeld` and the bells rung.
    int Update(float dt, int layer, bool tabHeld, int bellsRung)
    {
        if (!m_running || !(dt > 0.0f))
        {
            return 0;
        }
        m_seconds += dt;
        const int due = static_cast<int>(m_seconds / kStrokeSeconds);
        if (due <= m_strokes)
        {
            return 0;
        }
        m_strokes = due > kStrokes ? kStrokes : due;
        if (m_strokes == kStrokes)
        {
            m_running = false;
            if (tabHeld && bellsRung >= 3)
            {
                m_kind = EndingKind::Both;
            }
            else
            {
                m_kind = layer == 0 ? EndingKind::Past : EndingKind::Present;
            }
        }
        return m_strokes;
    }

    void Reset()
    {
        m_running = false;
        m_kind = EndingKind::None;
        m_seconds = 0.0f;
        m_strokes = 0;
    }

    bool Running() const
    {
        return m_running;
    }
    bool Done() const
    {
        return m_kind != EndingKind::None;
    }
    EndingKind Kind() const
    {
        return m_kind;
    }
    int Strokes() const
    {
        return m_strokes;
    }

    /// The first line of the end screen.
    static const char* Title(EndingKind k)
    {
        switch (k)
        {
        case EndingKind::Past:
            return "THE PAST";
        case EndingKind::Present:
            return "THE PRESENT";
        case EndingKind::Both:
            return "BOTH";
        case EndingKind::None:
            break;
        }
        return "CLEAR";
    }

    /// True when the ending lets the waters of A drain (time moves on).
    static bool Drains(EndingKind k)
    {
        return k == EndingKind::Present || k == EndingKind::Both;
    }

  private:
    bool m_running = false;
    EndingKind m_kind = EndingKind::None;
    float m_seconds = 0.0f;
    int m_strokes = 0;
};

} // namespace dl::stage
