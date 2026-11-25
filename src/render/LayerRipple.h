// LayerRipple.h - the timing of the switch between layer A and layer B: a circle that opens from one point of the
// screen. Owns:      which layer is outside the circle, which one is inside, and how far the circle has opened (0 ..
// 1). It knows
//            nothing about Direct3D, so it is tested without a window; Models turns the numbers into pixels.
// Rules:     idle          = the whole screen shows one layer (Outer), the circle is closed.
//            Toggle        = the other layer starts to open as a circle from the centre (the ripple).
//            Toggle again while it moves = the circle turns round and closes again; the picture never jumps.
//            When the circle has covered the whole screen, the inner layer becomes the outer one and the ripple is
//            over.
// Layers:    0 = A, 1 = B.
// Threading: main thread only.
#pragma once

#include <algorithm>
#include <cmath>

namespace dl
{

class LayerRipple
{
  public:
    /// UNITS: seconds for the circle to open completely. 0 = no ripple, the layers change at once.
    void SetSeconds(float seconds)
    {
        m_seconds = std::clamp(seconds, 0.0f, 10.0f);
    }

    float Seconds() const
    {
        return m_seconds;
    }

    /// Goes to the other layer. While a ripple runs, it turns the ripple round instead.
    void Toggle()
    {
        if (m_seconds <= 0.0f)
        {
            m_outer = 1 - Target(); // a cut
            Stop();
            return;
        }
        if (!m_moving)
        {
            m_inner = 1 - m_outer;
            m_direction = 1;
            m_progress = 0.0f;
            m_moving = true;
        }
        else
        {
            m_direction = -m_direction;
        }
    }

    /// deltaSeconds: the time of the last frame.
    void Update(float deltaSeconds)
    {
        if (!m_moving)
        {
            return;
        }
        if (m_seconds <= 0.0f) // the time was set to 0 while the ripple ran: finish at once
        {
            m_outer = Target();
            Stop();
            return;
        }
        m_progress += static_cast<float>(m_direction) * deltaSeconds / m_seconds;
        if (m_progress >= 1.0f)
        {
            m_outer = m_inner; // the new layer has covered the screen
            Stop();
        }
        else if (m_progress <= 0.0f)
        {
            Stop(); // the circle has closed again: the old layer is still the outer one
        }
    }

    bool Active() const
    {
        return m_moving;
    }

    /// The layer outside the circle (when idle: the layer on the screen).
    int Outer() const
    {
        return m_outer;
    }

    /// The layer inside the circle. Only meaningful while Active().
    int Inner() const
    {
        return m_inner;
    }

    /// The layer the screen is going to show: the one the player asked for last.
    int Target() const
    {
        return (m_moving && m_direction > 0) ? m_inner : m_outer;
    }

    /// How far the circle has opened, 0 .. 1, with an easing that is fast at the start and slow at the end (like a
    /// wave).
    float Opened() const
    {
        const float rest = 1.0f - std::clamp(m_progress, 0.0f, 1.0f);
        return 1.0f - rest * rest * rest;
    }

  private:
    void Stop()
    {
        m_moving = false;
        m_direction = 0;
        m_progress = 0.0f;
    }

    int m_outer = 0;
    int m_inner = 1;
    int m_direction = 0; // +1 opening, -1 closing, 0 idle
    float m_progress = 0.0f;
    float m_seconds = 0.9f;
    bool m_moving = false;
};

/// Distance (in screen-height units) from the centre of the ripple to the farthest corner of the screen. `centerX` and
/// `centerY` are 0 .. 1 over the screen, `aspect` is width / height. A circle with this radius covers the whole screen.
inline float FarthestCornerDistance(float centerX, float centerY, float aspect)
{
    // (no std::max here: windows.h may define max as a macro before this header is read, see docs/CONVENTIONS.md)
    const float farX = centerX > 1.0f - centerX ? centerX : 1.0f - centerX;
    const float farY = centerY > 1.0f - centerY ? centerY : 1.0f - centerY;
    const float dx = farX * aspect;
    return std::sqrt(dx * dx + farY * farY);
}

} // namespace dl
