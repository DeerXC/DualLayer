// Guidance.h - the help of the game: the lantern as compass (docs/DESIGN.md 10.3).
// Goal:      the first step of the main path (scene/Town.h `steps`, in the order it is played) that is not done: a
//            thing opened (scene/Things.h), or the lantern set down at its spot (scene/Lantern.h; once it has stood
//            there it counts as done, also when it is taken up again). When every step is done the goal is the signal
//            mast over the tower.
// Flame:     always on. The flame of the lantern leans toward the goal by leanDegrees; after idleSeconds without
//            progress it leans further (leanIdleDegrees) and throws sparks. It stands upright when the body is at the
//            goal. It is a compass, not an answer: it points at the place, never at what to do there.
// Progress:  a step done, a cell of kCell metres that the body has not stood in since the goal changed, or a new
//            closest approach to the goal by kProgressMetres. A body that walks somewhere new is never helped; one that
//            goes round and round is.
// H:         the player asks for more, in three steps that the player escalates by pressing and by time without
//            progress, never by distance to an answer. First press: the lantern is raised and a warm beam sweeps over
//            the place for sweepSeconds. Second press during the sweep, and only after ghostAfterSeconds without
//            progress (so the player has tried first): a faint frame round the thing to use, joined to the lantern by a
//            thin line, for ghostSeconds. A press while it shows (and before the second step is allowed) puts it away.
//            The third step of the design (the point of a suggested switch) is not built.
// Plain data and plain maths, no Windows, no Direct3D: tested without a window (tests/test_guidance.cpp).
// UNITS: metres, seconds, degrees in the settings, radians inside. Space: left-handed, +Y up.
#pragma once

#include "scene/Camera.h" // Vec3
#include "scene/Flame.h"
#include "scene/Lantern.h"
#include "scene/SceneData.h"
#include "scene/Things.h"
#include "scene/Town.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <unordered_set>
#include <vector>

namespace dl::town
{

struct GuidanceSettings
{
    bool enabled = true;
    float leanDegrees = 18.0f;       ///< UNITS: degrees the flame leans toward the goal
    float leanIdleDegrees = 40.0f;   ///< UNITS: degrees after idleSeconds without progress
    float idleSeconds = 30.0f;       ///< UNITS: seconds without progress before the flame grows restless
    float sweepSeconds = 6.0f;       ///< UNITS: seconds the lantern stays raised after H
    float ghostAfterSeconds = 90.0f; ///< UNITS: seconds without progress before the second press of H shows the thing
    float ghostSeconds = 12.0f;      ///< UNITS: seconds the thing stays shown
};

class Guidance
{
  public:
    enum class Tier
    {
        None,  ///< only the flame
        Sweep, ///< the lantern raised, a beam over the place
        Ghost, ///< a frame round the thing to use, joined to the lantern
    };

    static constexpr float kCell = 6.0f; ///< UNITS: metres: the side of a cell of ground (progress = a new cell)
    static constexpr float kProgressMetres = 4.0f; ///< UNITS: metres: a new closest approach counts from this much
    static constexpr float kSpotReach = 3.5f;      ///< UNITS: metres from the spot at which a set-down lantern counts
    static constexpr float kArrived = 2.0f;        ///< UNITS: metres from the goal at which the flame stands upright
    static constexpr int kBeamObjects = 8;
    static constexpr int kGhostObjects = 22;
    static constexpr int kDrawObjects = kGhostObjects + 2; ///< objects that AddDraw adds at most

    /// `town` must outlive this. Values that make no sense are pulled into a range that works.
    void Init(const TownData& town, const GuidanceSettings& settings)
    {
        m_town = &town;
        SetSettings(settings);
        m_thingOf.assign(town.steps.size(), -1);
        for (std::size_t i = 0; i < town.steps.size(); ++i)
        {
            for (std::size_t k = 0; k < town.things.size(); ++k)
            {
                if (town.steps[i].kind == StepKind::Thing && town.things[k].name == town.steps[i].name)
                {
                    m_thingOf[i] = static_cast<int>(k);
                }
            }
        }
        m_done.assign(town.steps.size(), false);
        m_lanternSet = false;
        m_goal = -1;
        m_tier = Tier::None;
        m_timer = 0.0f;
        m_raise = 0.0f;
        m_lean = 0.0f;
        m_sparks = 0.0f;
        m_idle = 0.0f;
        m_visited.clear();
        m_best = 1.0e9f;
    }

    /// New settings (F5); what has happened so far is kept.
    void SetSettings(const GuidanceSettings& settings)
    {
        m_settings = settings;
        m_settings.leanDegrees = Clamp(settings.leanDegrees, 0.0f, 60.0f);
        m_settings.leanIdleDegrees = Clamp(settings.leanIdleDegrees, m_settings.leanDegrees, 75.0f);
        m_settings.idleSeconds = Clamp(settings.idleSeconds, 5.0f, 600.0f);
        m_settings.sweepSeconds = Clamp(settings.sweepSeconds, 1.0f, 60.0f);
        m_settings.ghostAfterSeconds = Clamp(settings.ghostAfterSeconds, m_settings.idleSeconds, 900.0f);
        m_settings.ghostSeconds = Clamp(settings.ghostSeconds, 1.0f, 120.0f);
    }

    /// H.
    void Press()
    {
        if (m_town == nullptr || !m_settings.enabled || m_goal < 0)
        {
            return;
        }
        switch (m_tier)
        {
        case Tier::None:
            m_tier = Tier::Sweep;
            m_timer = m_settings.sweepSeconds;
            break;
        case Tier::Sweep:
            if (m_idle >= m_settings.ghostAfterSeconds)
            {
                m_tier = Tier::Ghost;
                m_timer = m_settings.ghostSeconds;
            }
            else
            {
                m_tier = Tier::None; // put away: the player has not been stuck long enough to be shown the thing
                m_timer = 0.0f;
            }
            break;
        case Tier::Ghost:
            m_tier = Tier::None;
            m_timer = 0.0f;
            break;
        }
    }

    /// Once per frame. `walking`: the body walks and is not sinking (time stands still otherwise).
    void Update(float dt, const Vec3& feet, const Things& things, const Lantern& lantern, bool walking)
    {
        m_feet = feet;
        if (m_town == nullptr || !m_settings.enabled)
        {
            m_goal = -1;
            m_tier = Tier::None;
            return;
        }
        UpdateDone(things, lantern);
        int goal = -1;
        for (std::size_t i = 0; i < m_done.size() && goal < 0; ++i)
        {
            goal = m_done[i] ? -1 : static_cast<int>(i);
        }
        if (goal != m_goal)
        {
            m_goal = goal; // a step was done: what was shown is over, the progress starts again
            m_tier = Tier::None;
            m_timer = 0.0f;
            m_idle = 0.0f;
            m_visited.clear();
            Vec3 at;
            m_best = Target(at) ? Distance(feet, at) : 1.0e9f;
        }
        if (!walking || !(dt > 0.0f) || !std::isfinite(dt))
        {
            return;
        }
        if (m_goal >= 0)
        {
            bool progress = m_visited.insert(CellOf(feet)).second;
            const float d = Distance(feet, m_town->steps[static_cast<std::size_t>(m_goal)].at);
            if (d < m_best - kProgressMetres)
            {
                m_best = d;
                progress = true;
            }
            m_idle = progress ? 0.0f : m_idle + dt;
            if (m_tier != Tier::None)
            {
                m_timer -= dt;
                if (m_timer <= 0.0f)
                {
                    m_tier = Tier::None;
                    m_timer = 0.0f;
                }
            }
        }
        const bool restless = m_goal >= 0 && m_idle >= m_settings.idleSeconds;
        const float leanTarget = Radians(restless ? m_settings.leanIdleDegrees : m_settings.leanDegrees);
        const float ease = 1.0f - std::exp(-3.0f * dt);
        m_lean += (leanTarget - m_lean) * ease;
        m_sparks += ((restless ? 1.0f : 0.0f) - m_sparks) * ease;
        m_raise += ((m_tier != Tier::None ? 1.0f : 0.0f) - m_raise) * (1.0f - std::exp(-8.0f * dt));
    }

    /// The index of the goal in town.steps, or -1 (every step done, or no guidance).
    int Goal() const
    {
        return m_goal;
    }

    const Step* GoalStep() const
    {
        return m_goal >= 0 ? &m_town->steps[static_cast<std::size_t>(m_goal)] : nullptr;
    }

    /// How many steps are done.
    int Done() const
    {
        int n = 0;
        for (const bool d : m_done)
        {
            n += d ? 1 : 0;
        }
        return n;
    }

    /// True when step `k` of town.steps is done (the map lights the district of a solved step).
    bool IsDone(int k) const
    {
        return k >= 0 && k < static_cast<int>(m_done.size()) && m_done[static_cast<std::size_t>(k)];
    }

    /// Seconds without progress on the goal.
    float IdleSeconds() const
    {
        return m_idle;
    }

    Tier GetTier() const
    {
        return m_tier;
    }

    /// 0 .. 1: how high the lantern is held (the avatar raises its arm).
    float Raise() const
    {
        return m_raise;
    }

    /// The point the flame leans toward: the handle or spot of the goal, else the signal mast; false when there is
    /// none.
    bool Target(Vec3& at) const
    {
        if (m_town == nullptr)
        {
            return false;
        }
        if (m_goal >= 0)
        {
            at = m_town->steps[static_cast<std::size_t>(m_goal)].at;
            return true;
        }
        return m_town->MastAt(at);
    }

    /// The flame of a lantern at `from`: it leans toward the goal; upright when `from` is at it. Off when the guidance
    /// is off.
    Flame FlameFrom(const Vec3& from) const
    {
        Flame f;
        f.on = m_town != nullptr && m_settings.enabled;
        Vec3 at;
        if (!f.on || !Target(at))
        {
            return f;
        }
        const float dx = at.x - from.x;
        const float dz = at.z - from.z;
        const float d = std::sqrt(dx * dx + dz * dz);
        f.yaw = d > 1.0e-3f ? std::atan2(dx, dz) : 0.0f;
        const float away = Smooth(kArrived, kArrived + 2.0f, d);
        f.tilt = m_lean * away;
        f.sparks = m_sparks * away;
        return f;
    }

    /// The beam of the first step and the frame of the second, from the raised lantern at `hand`. Returns the new
    /// count; nothing when only the flame shows or the table has no room for kDrawObjects.
    int AddDraw(scene::SceneObject* out, int count, int capacity, float seconds, const Vec3& hand) const
    {
        Vec3 at;
        if (m_tier == Tier::None || m_goal < 0 || out == nullptr || count < 0 || capacity - count < kDrawObjects ||
            !Target(at))
        {
            return count;
        }
        if (m_tier == Tier::Sweep)
        {
            // a beam that turns toward the place: it swings wide and settles on the bearing of the goal
            const float elapsed = m_settings.sweepSeconds - m_timer;
            const float bearing = std::atan2(at.x - hand.x, at.z - hand.z);
            const float settle = 1.0f - Clamp(elapsed / (0.8f * m_settings.sweepSeconds), 0.0f, 1.0f);
            const float yaw = bearing + 0.75f * settle * std::sin(elapsed * 4.2f);
            const float sx = std::sin(yaw);
            const float sz = std::cos(yaw);
            for (int i = 0; i < kBeamObjects; ++i)
            {
                const float run =
                    1.2f + 1.9f * static_cast<float>(i) + 0.6f * std::sin(seconds * 3.0f + static_cast<float>(i));
                scene::SceneObject o{};
                o.shape = scene::ShapeKind::Cube;
                o.position[0] = hand.x + sx * run;
                o.position[1] = hand.y + 0.12f + 0.05f * static_cast<float>(i);
                o.position[2] = hand.z + sz * run;
                const float size = 0.14f + 0.035f * static_cast<float>(i);
                o.scale[0] = o.scale[1] = o.scale[2] = size;
                Warm(o, 1.4f - 0.1f * static_cast<float>(i));
                out[count++] = o;
            }
            return count;
        }
        // Ghost: a frame of thin edges round the thing, and a thin line of motes from the lantern to it
        const Step& step = m_town->steps[static_cast<std::size_t>(m_goal)];
        const Vec3 c{at.x, step.kind == StepKind::Thing ? at.y - 0.4f : at.y + 0.6f, at.z};
        constexpr float h = 0.9f;
        constexpr float w = 0.05f;
        const auto edge = [&](float x, float y, float z, float sx, float sy, float sz)
        {
            scene::SceneObject o{};
            o.shape = scene::ShapeKind::Cube;
            o.position[0] = c.x + x;
            o.position[1] = c.y + y;
            o.position[2] = c.z + z;
            o.scale[0] = sx;
            o.scale[1] = sy;
            o.scale[2] = sz;
            Warm(o, 0.7f);
            out[count++] = o;
        };
        for (const float a : {-h, h})
        {
            for (const float b : {-h, h})
            {
                edge(0.0f, a, b, 2.0f * h, w, w); // along x
                edge(a, 0.0f, b, w, 2.0f * h, w); // along y
                edge(a, b, 0.0f, w, w, 2.0f * h); // along z
            }
        }
        for (int i = 1; i <= 10; ++i)
        {
            const float f = static_cast<float>(i) / 11.0f;
            scene::SceneObject o{};
            o.shape = scene::ShapeKind::Cube;
            o.position[0] = hand.x + (c.x - hand.x) * f;
            o.position[1] = hand.y + (c.y - hand.y) * f;
            o.position[2] = hand.z + (c.z - hand.z) * f;
            o.scale[0] = o.scale[1] = o.scale[2] = 0.07f;
            Warm(o, 0.9f);
            out[count++] = o;
        }
        return count;
    }

  private:
    const TownData* m_town = nullptr;
    GuidanceSettings m_settings;
    std::vector<int> m_thingOf; ///< step -> index of its thing in town.things, or -1
    std::vector<bool> m_done;
    bool m_lanternSet = false; ///< the lantern has stood at its spot
    int m_goal = -1;
    Tier m_tier = Tier::None;
    float m_timer = 0.0f; ///< seconds the sweep or the frame has left
    float m_raise = 0.0f;
    float m_lean = 0.0f;   ///< radians, eased
    float m_sparks = 0.0f; ///< 0 .. 1, eased
    float m_idle = 0.0f;   ///< seconds without progress
    float m_best = 1.0e9f; ///< the closest the body has been to the goal
    Vec3 m_feet{};
    std::unordered_set<long long> m_visited;

    static float Clamp(float v, float lo, float hi)
    {
        return !(v >= lo) ? lo : (v > hi ? hi : v);
    }

    static float Radians(float degrees)
    {
        return degrees * 0.017453292f;
    }

    static float Smooth(float e0, float e1, float v)
    {
        const float t = Clamp((v - e0) / (e1 - e0), 0.0f, 1.0f);
        return t * t * (3.0f - 2.0f * t);
    }

    static float Distance(const Vec3& a, const Vec3& b)
    {
        const float dx = a.x - b.x;
        const float dy = a.y - b.y;
        const float dz = a.z - b.z;
        return std::sqrt(dx * dx + dy * dy + dz * dz);
    }

    static long long CellOf(const Vec3& p)
    {
        const auto i = static_cast<long long>(std::floor(p.x / kCell));
        const auto j = static_cast<long long>(std::floor(p.z / kCell));
        return i * 100003LL + j;
    }

    static void Warm(scene::SceneObject& o, float glow)
    {
        o.colorSrgb[0] = 1.0f;
        o.colorSrgb[1] = 0.82f;
        o.colorSrgb[2] = 0.48f;
        o.shineStrength = 0.0f;
        o.shininess = 1.0f;
        o.glow = glow;
        o.layerMask = scene::kLayerBoth;
    }

    void UpdateDone(const Things& things, const Lantern& lantern)
    {
        for (std::size_t i = 0; i < m_done.size(); ++i)
        {
            const Step& step = m_town->steps[i];
            if (step.kind == StepKind::Thing)
            {
                m_done[i] = m_thingOf[i] >= 0 && things.Open(static_cast<std::size_t>(m_thingOf[i]));
            }
            else
            {
                if (!lantern.Carried() && Distance(lantern.At(), step.at) <= kSpotReach)
                {
                    m_lanternSet = true; // it has stood there: done, whatever happens to it later
                }
                m_done[i] = m_lanternSet;
            }
        }
    }
};

} // namespace dl::town
