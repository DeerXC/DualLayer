// LayerGuard.h - the guard of the layer switch.
// A request to switch the layer is turned into what the program should do. A switch into a place where the body does
// not fit is not done: the ripple opens a little and turns back (Refuse), and while it does, the old layer stays the
// solid one. Plain logic, no Windows, no Direct3D: tested without a window (tests/test_layer_guard.cpp). UNITS:
// seconds.
#pragma once

#include <cmath>

namespace dl
{

class LayerGuard
{
  public:
    enum class Action
    {
        Switch, ///< switch as usual (ToggleLayer)
        Refuse, ///< start the ripple (ToggleLayer) and turn it back when Update returns true; keep HeldLayer solid
                ///< meanwhile
        Ignore  ///< do nothing
    };

    static constexpr float kRefuseSeconds = 0.15f; ///< UNITS: seconds before a refused ripple turns back

    /// currentLayer: the layer that is solid now. rippleActive: a ripple is running. fits: the body fits in the layer
    /// the switch would go to.
    Action OnSwitchRequest(int currentLayer, bool rippleActive, bool fits)
    {
        if (m_refuse > 0.0f)
        {
            return Action::Ignore; // a refusal is playing
        }
        if (fits)
        {
            return Action::Switch;
        }
        if (rippleActive)
        {
            return Action::Ignore; // no refusal on top of a ripple: the request is dropped
        }
        m_refuse = kRefuseSeconds;
        m_held = currentLayer;
        ++m_refusals;
        return Action::Refuse;
    }

    /// True once, at the moment the refused ripple has to be turned back.
    bool Update(float dtSeconds)
    {
        if (m_refuse <= 0.0f || !(dtSeconds > 0.0f) || !std::isfinite(dtSeconds))
        {
            return false;
        }
        m_refuse -= dtSeconds;
        if (m_refuse <= 0.0f)
        {
            m_refuse = 0.0f;
            return true;
        }
        return false;
    }

    bool Refusing() const
    {
        return m_refuse > 0.0f;
    }

    /// The layer that stays solid while a refusal plays.
    int HeldLayer() const
    {
        return m_held;
    }

    /// How many switches were refused (for the log and the tests).
    int Refusals() const
    {
        return m_refusals;
    }

  private:
    float m_refuse = 0.0f;
    int m_held = 0;
    int m_refusals = 0;
};

} // namespace dl
