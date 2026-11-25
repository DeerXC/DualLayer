// Lantern.h - the lantern and the circle of the past it keeps standing in the present (docs/DESIGN.md 3.2), and the
// traces
//            that what was done in the past leaves in the present (3.5).
// Circle:    the lantern set down in A keeps a sphere of radius kRadius round it in A in both moments. In B, inside the
//            sphere, what is solid and what is seen is A; outside it, B. (A is A everywhere: the circle changes nothing
//            in A.)
// Moments:   the solid of the town with the circle (scene/Moments.h). The water of A is water inside the circle in B as
// well. Rules:     set down in A only, by a body that stands on something; taken up in either moment from within
// kReach, unless
//            taking it would leave the body inside something or without the floor it stands on (refused, as a switch
//            is). While it is set down, the body carries nothing; a switch is checked against the moment with the
//            circle.
// Traces:    a thing changed in A is marked in B by a faint glow of the colour of the lantern (never gold: gold means
// use),
//            which comes up over kTraceRise seconds after the change and then stays (Traces). The things that change in
//            A come with the puzzles of the town (docs/DESIGN.md 12, step 4); the record is here so that every one of
//            them uses the same rule.
// Plain data and plain maths, no Windows, no Direct3D: tested without a window (tests/test_lantern.cpp).
// UNITS: metres, seconds. Space: left-handed, +Y up.
#pragma once

#include "scene/Camera.h" // Vec3
#include "scene/Moments.h"
#include "scene/SceneData.h"

#include <array>
#include <cmath>
#include <cstdint>

namespace dl::town
{

inline constexpr float kRadius = 3.5f; ///< UNITS: metres: the radius of the circle of the past
inline constexpr float kLanternUp =
    0.45f;                            ///< UNITS: metres: the centre of the circle over the floor the lantern stands on
inline constexpr float kReach = 1.8f; ///< UNITS: metres between the feet and the lantern, to take it up
inline constexpr float kPutAhead = 0.9f;   ///< UNITS: metres ahead of the feet where it is set down
inline constexpr float kRecallFrom = 8.0f; ///< UNITS: metres from the set-down lantern from which E calls it back

/// The lantern: carried, or set down with its circle.
class Lantern
{
  public:
    enum class Result
    {
        Nothing,     ///< nothing to do here
        Put,         ///< set down
        Taken,       ///< taken up
        OnlyInPast,  ///< it is set down in A only
        NoFloor,     ///< no floor of A ahead to set it on
        RefusedTake, ///< taking it would leave the body inside something or without its floor
    };

    bool Carried() const
    {
        return !m_placed;
    }
    /// Where it stands (the foot of the lantern) while it is set down.
    const Vec3& At() const
    {
        return m_at;
    }
    /// Its circle while it is set down, else none.
    Circle GetCircle() const
    {
        Circle c;
        if (m_placed)
        {
            c.centre = {m_at.x, m_at.y + kLanternUp, m_at.z};
            c.radius = kRadius;
        }
        return c;
    }

    /// True when the feet are within reach of the set-down lantern.
    bool InReach(const Vec3& feet) const
    {
        const float dx = feet.x - m_at.x;
        const float dz = feet.z - m_at.z;
        return m_placed && dx * dx + dz * dz <= kReach * kReach && std::fabs(feet.y - m_at.y) <= 1.5f;
    }

    /// E: takes it up when it is in reach, else sets it down `kPutAhead` ahead along `forward` (in A, standing).
    /// `moments` must hold the circle as it is now; on a change the caller gives it the new one (GetCircle). `radius`:
    /// the body's.
    Result Use(const Vec3& feet, bool grounded, const Vec3& forward, int layer, float radius, Moments& moments)
    {
        if (m_placed)
        {
            if (!InReach(feet))
            {
                return Result::Nothing;
            }
            // what the body would be in without the circle: it must still fit, and still stand when it stands now
            Moments without = moments;
            without.SetCircle(Circle{});
            const bool stoodWith = grounded && moments.Supports(feet, radius, layer);
            bool fits = true;
            for (int i = 0; i < 3; ++i)
            {
                Vec3 c{feet.x, feet.y + radius + 0.6f * static_cast<float>(i), feet.z};
                fits = fits && without.PushOut(c, radius - 0.03f, layer) <= 0.005f;
            }
            if (!fits || (stoodWith && !without.Supports(feet, radius, layer)))
            {
                return Result::RefusedTake;
            }
            m_placed = false;
            moments.SetCircle(Circle{});
            return Result::Taken;
        }
        if (layer != 0)
        {
            return Result::OnlyInPast;
        }
        if (!grounded)
        {
            return Result::NoFloor;
        }
        const float fl = std::sqrt(forward.x * forward.x + forward.z * forward.z);
        const float fx = fl > 1.0e-4f ? forward.x / fl : 0.0f;
        const float fz = fl > 1.0e-4f ? forward.z / fl : 1.0f;
        Vec3 at{feet.x + fx * kPutAhead, feet.y, feet.z + fz * kPutAhead};
        float floor = 0.0f;
        if (!moments.FloorOfA(at.x, at.z, feet.y + 0.5f, feet.y - 1.0f, floor))
        {
            if (!moments.FloorOfA(feet.x, feet.z, feet.y + 0.5f, feet.y - 1.0f, floor))
            {
                return Result::NoFloor;
            }
            at = feet; // nothing ahead (an edge): at the feet
        }
        at.y = floor;
        m_at = at;
        m_placed = true;
        moments.SetCircle(GetCircle());
        return Result::Put;
    }

    /// True when the lantern is set down and the feet are at least `kRecallFrom` from it (E calls it back).
    bool Far(const Vec3& feet) const
    {
        const float dx = feet.x - m_at.x;
        const float dy = feet.y - m_at.y;
        const float dz = feet.z - m_at.z;
        return m_placed && dx * dx + dy * dy + dz * dz >= kRecallFrom * kRecallFrom;
    }

    /// E far from the set-down lantern: it comes back to the hand (the circle goes), refused as a take-up is when the
    /// body would be left inside something or without its floor. A lantern left behind is never lost.
    Result Recall(const Vec3& feet, bool grounded, int layer, float radius, Moments& moments)
    {
        if (!Far(feet))
        {
            return Result::Nothing;
        }
        Moments without = moments;
        without.SetCircle(Circle{});
        const bool stoodWith = grounded && moments.Supports(feet, radius, layer);
        bool fits = true;
        for (int i = 0; i < 3; ++i)
        {
            Vec3 c{feet.x, feet.y + radius + 0.6f * static_cast<float>(i), feet.z};
            fits = fits && without.PushOut(c, radius - 0.03f, layer) <= 0.005f;
        }
        if (!fits || (stoodWith && !without.Supports(feet, radius, layer)))
        {
            return Result::RefusedTake;
        }
        m_placed = false;
        moments.SetCircle(Circle{});
        return Result::Taken;
    }

    /// Back in the hand (a restart).
    void Reset(Moments& moments)
    {
        m_placed = false;
        moments.SetCircle(Circle{});
    }

    /// The lantern as drawn while it is set down (both moments): a post and a glowing lamp. Returns the new count.
    int AddDraw(scene::SceneObject* out, int count, int capacity) const
    {
        if (!m_placed || out == nullptr || capacity - count < 2)
        {
            return count;
        }
        scene::SceneObject post{};
        post.shape = scene::ShapeKind::Cube;
        post.position[0] = m_at.x;
        post.position[1] = m_at.y + 0.2f;
        post.position[2] = m_at.z;
        post.scale[0] = 0.12f;
        post.scale[1] = 0.4f;
        post.scale[2] = 0.12f;
        post.colorSrgb[0] = 0.28f;
        post.colorSrgb[1] = 0.24f;
        post.colorSrgb[2] = 0.22f;
        post.layerMask = scene::kLayerBoth;
        out[count++] = post;
        scene::SceneObject lamp = post;
        lamp.position[1] = m_at.y + kLanternUp;
        lamp.scale[0] = lamp.scale[1] = lamp.scale[2] = 0.26f;
        lamp.colorSrgb[0] = 1.0f;
        lamp.colorSrgb[1] = 0.78f;
        lamp.colorSrgb[2] = 0.42f;
        lamp.glow = 1.4f;
        out[count++] = lamp;
        return count;
    }

  private:
    bool m_placed = false;
    Vec3 m_at{};
};

// ---- traces (docs/DESIGN.md 3.5)

inline constexpr float kTraceRise = 1.5f;  ///< UNITS: seconds a trace takes to come up after its change
inline constexpr float kTraceGlow = 0.35f; ///< the glow of a trace that has come up
inline constexpr float kTraceTint = 0.35f; ///< how far its colour goes toward the colour of the lantern

class Traces
{
  public:
    static constexpr int kMax = 64;

    /// A thing (its id, from 0 .. kMax - 1) was changed in A: its trace starts to come up in B.
    void Changed(int id)
    {
        if (id >= 0 && id < kMax && m_age[static_cast<std::size_t>(id)] < 0.0f)
        {
            m_age[static_cast<std::size_t>(id)] = 0.0f;
        }
    }
    /// The thing is as it was again (R puts a room back): its trace is gone.
    void Undo(int id)
    {
        if (id >= 0 && id < kMax)
        {
            m_age[static_cast<std::size_t>(id)] = -1.0f;
        }
    }
    void Update(float dt)
    {
        for (float& a : m_age)
        {
            if (a >= 0.0f && dt > 0.0f)
            {
                a += dt;
            }
        }
    }
    /// 0 .. 1: how far the trace of `id` has come up.
    float Amount(int id) const
    {
        if (id < 0 || id >= kMax || m_age[static_cast<std::size_t>(id)] < 0.0f)
        {
            return 0.0f;
        }
        const float t = m_age[static_cast<std::size_t>(id)] / kTraceRise;
        return t >= 1.0f ? 1.0f : t * t * (3.0f - 2.0f * t);
    }
    /// Marks the B side of a changed thing: its colour toward the colour of the lantern, a faint glow.
    void Mark(int id, scene::SceneObject& o) const
    {
        const float a = Amount(id);
        if (a <= 0.0f)
        {
            return;
        }
        const float warm[3] = {1.0f, 0.78f, 0.42f};
        for (int k = 0; k < 3; ++k)
        {
            o.colorSrgb[k] += (warm[k] - o.colorSrgb[k]) * kTraceTint * a;
        }
        o.glow = o.glow > kTraceGlow * a ? o.glow : kTraceGlow * a;
    }

  private:
    std::array<float, kMax> m_age = MakeEmpty();

    static std::array<float, kMax> MakeEmpty()
    {
        std::array<float, kMax> a{};
        a.fill(-1.0f);
        return a;
    }
};

} // namespace dl::town
