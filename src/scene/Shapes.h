// Shapes.h - the vocabulary of the city: the pieces that a map is built from, beyond plain floors and walls
//            (docs/LEVEL_DESIGN.md, 13). Each piece is a function that writes boxes into a list, so that a map says "stairs
//            from here to there" and not twelve boxes by hand.
//   Stairs        drawn as steps, walked as the ramp under them: the body slides up a slope smoothly, and the eye sees steps.
//                 The steps are DRAW-ONLY (their front corners touch the slope, so the body never sinks more than half a
//                 step into them); the ramp is the SOLID part.
//   Railing       posts and a rail, 1.1 m: they stop a walk and mark an edge; a jump clears them. A parapet (2.6 m) is what
//                 seals a floor (scene/Stage.h kParapet).
//   Arcade        columns with a lintel over them: seen through, walked between, not walked through.
//   Bridge        a deck of both layers, or a deck of A that is broken in B (two halves and a gap): in A the structure is the
//                 way, in B it is not.
//   BreachWall    a wall of both layers with an opening: in A a panel closes it, in B the opening is a breach with rubble at
//                 its sides: in B the breach is the way.
//   Roof          two slopes and a ridge along X, draw-only (roofs are scenery over places no body can reach).
//   Water         a surface of layer A only: never solid for the body (a body whose feet go under it goes back to the room
//                 entry), solid for orbs (they float). In B the water has gone.
//   Lift          a platform that rises while a body stands on it and comes back down when it is left: the shortcut of a
//                 district (entered standing still, no jump needed). It moves itself; what stands on it is carried
//                 (Player::Carry).
// Plain data and plain logic, no Windows, no Direct3D: tested without a window (tests/test_shapes.cpp). UNITS: metres, seconds.
// Space:     left-handed, +Y up; slopes run along Z (a box turned round X), as every ramp of the project.
// Note:      no std::min / std::max here: windows.h may define them as macros (docs/CONVENTIONS.md).
#pragma once

#include "scene/Camera.h" // Vec3
#include "scene/SceneData.h"

#include <cmath>
#include <vector>

namespace dl::shapes
{

using scene::kLayerA;
using scene::kLayerB;
using scene::kLayerBoth;
using scene::SceneObject;
using scene::ShapeKind;

struct Rgb
{
    float r;
    float g;
    float b;
};

inline constexpr Rgb kStoneLight{0.55f, 0.54f, 0.52f};
inline constexpr Rgb kStoneDark{0.40f, 0.41f, 0.44f};
inline constexpr Rgb kWood{0.42f, 0.30f, 0.20f};
inline constexpr Rgb kRoofTile{0.55f, 0.24f, 0.18f};
inline constexpr Rgb kWaterColour{0.16f, 0.42f, 0.62f};
inline constexpr Rgb kPanelRed{0.82f, 0.24f, 0.22f};

/// A box from its two corners.
inline SceneObject Box(float x0, float y0, float z0, float x1, float y1, float z1, Rgb c, unsigned mask)
{
    SceneObject o{};
    o.shape = ShapeKind::Cube;
    o.position[0] = 0.5f * (x0 + x1);
    o.position[1] = 0.5f * (y0 + y1);
    o.position[2] = 0.5f * (z0 + z1);
    o.scale[0] = x1 - x0;
    o.scale[1] = y1 - y0;
    o.scale[2] = z1 - z0;
    o.colorSrgb[0] = c.r;
    o.colorSrgb[1] = c.g;
    o.colorSrgb[2] = c.b;
    o.shineStrength = 0.15f;
    o.shininess = 30.0f;
    o.layerMask = mask;
    return o;
}

/// A slab between x0 and x1 whose TOP runs in a straight line from (z0, y0) to (z1, y1): a box turned round X. `thickness` is
/// measured at right angles to the top.
inline SceneObject Slope(float x0, float x1, float z0, float y0, float z1, float y1, float thickness, Rgb c, unsigned mask)
{
    if (z1 < z0) // described from the other end: keep the angle within +-90 degrees, so that the top face is the one on the line
    {
        const float tz = z0;
        const float ty = y0;
        z0 = z1;
        y0 = y1;
        z1 = tz;
        y1 = ty;
    }
    const float dz = z1 - z0;
    const float dy = y1 - y0;
    const float a = std::atan2(dy, dz);
    SceneObject o = Box(x0, 0.0f, 0.0f, x1, thickness, std::sqrt(dz * dz + dy * dy), c, mask);
    o.tilt = -a;
    o.position[1] = 0.5f * (y0 + y1) - 0.5f * thickness * std::cos(a);
    o.position[2] = 0.5f * (z0 + z1) + 0.5f * thickness * std::sin(a);
    return o;
}

// ---- stairs

inline constexpr float kStepRise = 0.3f; ///< UNITS: metres: at most this high (the walking body snaps down 0.35 m)

/// Stairs between x0 and x1 from (z0, y0) to (z1, y1). The ramp goes to `solid`, the steps to `drawn`. Returns the number
/// of steps (rises of at most kStepRise).
inline int Stairs(float x0, float x1, float z0, float y0, float z1, float y1, Rgb c, std::vector<SceneObject>& solid,
                  std::vector<SceneObject>& drawn)
{
    solid.push_back(Slope(x0, x1, z0, y0, z1, y1, 0.2f, c, kLayerBoth));
    const float rise = std::fabs(y1 - y0);
    int n = static_cast<int>(std::ceil(rise / kStepRise - 1.0e-4f));
    n = n < 1 ? 1 : n;
    const float low = y0 < y1 ? y0 : y1;
    const float zLow = y0 < y1 ? z0 : z1;  // the foot of the stairs
    const float zHigh = y0 < y1 ? z1 : z0; // the head
    const float run = (zHigh - zLow) / static_cast<float>(n); // signed: the stairs may climb toward -Z
    const float stepRise = rise / static_cast<float>(n);
    for (int i = 0; i < n; ++i)
    {
        // step i: its top at low + (i + 1) rise, from its front edge (on the slope) back to the foot side of the stairs. The
        // tread of step i lies between the front edges of steps i and i + 1; under it the step reaches down to `low`.
        const float top = low + static_cast<float>(i + 1) * stepRise;
        const float zFront = zLow + static_cast<float>(i) * run;
        const float zBack = zLow + static_cast<float>(i + 1) * run;
        const float za = zFront < zBack ? zFront : zBack;
        const float zb = zFront < zBack ? zBack : zFront;
        drawn.push_back(Box(x0, low - 0.2f, za, x1, top, zb, c, kLayerBoth));
    }
    return n;
}

// ---- railings, arcades

inline constexpr float kRailHeight = 1.1f; ///< UNITS: metres
inline constexpr float kPostSpacing = 2.0f;

/// A railing along a straight line on the floor at `y`, from (x0, z0) to (x1, z1) (one of the two must be the same: along X or
/// along Z). Posts every kPostSpacing and a rail on top; all solid. Returns the number of boxes.
inline int Railing(float x0, float z0, float x1, float z1, float y, unsigned mask, std::vector<SceneObject>& solid)
{
    const bool alongX = std::fabs(z1 - z0) < 1.0e-4f;
    const float length = alongX ? std::fabs(x1 - x0) : std::fabs(z1 - z0);
    const int posts = static_cast<int>(std::floor(length / kPostSpacing)) + 1;
    const float half = 0.08f;
    int n = 0;
    for (int i = 0; i < posts; ++i)
    {
        const float t = posts == 1 ? 0.0f : static_cast<float>(i) / static_cast<float>(posts - 1);
        const float px = x0 + (x1 - x0) * t;
        const float pz = z0 + (z1 - z0) * t;
        solid.push_back(Box(px - half, y, pz - half, px + half, y + kRailHeight - 0.1f, pz + half, kWood, mask));
        ++n;
    }
    const float xa = (x0 < x1 ? x0 : x1) - half;
    const float xb = (x0 < x1 ? x1 : x0) + half;
    const float za = (z0 < z1 ? z0 : z1) - half;
    const float zb = (z0 < z1 ? z1 : z0) + half;
    solid.push_back(Box(xa, y + kRailHeight - 0.1f, za, xb, y + kRailHeight, zb, kWood, mask));
    return n + 1;
}

/// An arcade along Z at x0 .. x1 (the column width), from z0 to z1, on a floor at `y`: columns of `width` every `spacing`,
/// and a lintel from `clear` to `clear` + 0.4 m above the floor. Columns and lintel are solid. Returns the number of boxes.
inline int Arcade(float x0, float x1, float z0, float z1, float y, float spacing, float clear, std::vector<SceneObject>& solid)
{
    const float w = x1 - x0;
    int n = 0;
    for (float z = z0; z <= z1 + 1.0e-3f; z += spacing)
    {
        solid.push_back(Box(x0, y, z - 0.5f * w, x1, y + clear, z + 0.5f * w, kStoneLight, kLayerBoth));
        ++n;
    }
    solid.push_back(Box(x0, y + clear, z0 - 0.5f * w, x1, y + clear + 0.4f, z1 + 0.5f * w, kStoneLight, kLayerBoth));
    return n + 1;
}

// ---- bridges and breaches

/// A bridge deck along Z (x0 .. x1, z0 .. z1, its top at `y`). `brokenInB`: in A the deck is whole; in B only its two ends are
/// there, with a gap of `gap` metres in the middle. Returns the number of boxes.
inline int Bridge(float x0, float x1, float z0, float z1, float y, bool brokenInB, float gap, std::vector<SceneObject>& solid)
{
    if (!brokenInB)
    {
        solid.push_back(Box(x0, y - 0.3f, z0, x1, y, z1, kStoneDark, kLayerBoth));
        return 1;
    }
    const float mid = 0.5f * (z0 + z1);
    solid.push_back(Box(x0, y - 0.3f, z0, x1, y, mid - 0.5f * gap, kStoneDark, kLayerBoth)); // the two ends: both layers
    solid.push_back(Box(x0, y - 0.3f, mid + 0.5f * gap, x1, y, z1, kStoneDark, kLayerBoth));
    solid.push_back(Box(x0, y - 0.3f, mid - 0.5f * gap, x1, y, mid + 0.5f * gap, kStoneDark, kLayerA)); // the middle: A only
    return 3;
}

/// A wall across Z at z (thickness `t`) from x0 to x1, `height` high on a floor at `y`, with an opening from bx0 to bx1. In A a
/// panel closes the opening; in B it is a breach with rubble at both sides. Returns the number of boxes.
inline int BreachWall(float x0, float x1, float z, float t, float y, float height, float bx0, float bx1, std::vector<SceneObject>& solid)
{
    const float za = z - 0.5f * t;
    const float zb = z + 0.5f * t;
    solid.push_back(Box(x0, y, za, bx0, y + height, zb, kStoneLight, kLayerBoth));
    solid.push_back(Box(bx1, y, za, x1, y + height, zb, kStoneLight, kLayerBoth));
    solid.push_back(Box(bx0, y, za, bx1, y + height, zb, kPanelRed, kLayerA));
    solid.push_back(Box(bx0, y, za - 0.35f, bx0 + 0.3f, y + 0.4f, zb + 0.35f, kStoneDark, kLayerB));
    solid.push_back(Box(bx1 - 0.3f, y, za - 0.35f, bx1, y + 0.4f, zb + 0.35f, kStoneDark, kLayerB));
    return 5;
}

// ---- roofs

/// A roof over x0 .. x1, z0 .. z1: two slopes from the eaves (at `eave`) up to a ridge along X (at `ridge`) in the middle of Z,
/// and a ridge cap. Draw-only. Returns the number of boxes.
inline int Roof(float x0, float x1, float z0, float z1, float eave, float ridge, std::vector<SceneObject>& drawn)
{
    const float mid = 0.5f * (z0 + z1);
    drawn.push_back(Slope(x0, x1, z0, eave, mid, ridge, 0.15f, kRoofTile, kLayerBoth));
    drawn.push_back(Slope(x0, x1, mid, ridge, z1, eave, 0.15f, kRoofTile, kLayerBoth));
    drawn.push_back(Box(x0, ridge - 0.05f, mid - 0.12f, x1, ridge + 0.15f, mid + 0.12f, kStoneDark, kLayerBoth));
    return 3;
}

// ---- water

struct Water
{
    float x0;
    float z0;
    float x1;
    float z1;
    float top;   ///< world y of the surface
    float depth; ///< UNITS: metres the drawn body of water reaches below the surface
};

/// True when a body whose feet are at `feet` is under the water (in layer A only: in B the water has gone).
inline bool Submerged(const Water& w, const Vec3& feet, int layer)
{
    return layer == 0 && feet.x > w.x0 && feet.x < w.x1 && feet.z > w.z0 && feet.z < w.z1 && feet.y < w.top - 0.3f;
}

/// The drawn water: a box of layer A with a little light of its own (it catches the evening sky).
inline SceneObject WaterBox(const Water& w)
{
    SceneObject o = Box(w.x0, w.top - w.depth, w.z0, w.x1, w.top, w.z1, kWaterColour, kLayerA);
    o.shineStrength = 0.9f;
    o.shininess = 90.0f;
    o.glow = 0.12f;
    return o;
}

/// The surface as a solid for ORBS only (they float): a thin slab of layer A.
inline SceneObject WaterSurface(const Water& w)
{
    return Box(w.x0, w.top - 0.1f, w.z0, w.x1, w.top, w.z1, kWaterColour, kLayerA);
}

// ---- lifts

/// A platform (a box of both layers) between a bottom and a top stop. It rises while a body stands on it, waits at the top,
/// and goes back down once it has been left for kReturnSeconds. Update returns how far it moved in this frame; a body standing
/// on it is carried by the same amount (Player::Carry) before the body moves.
class Lift
{
  public:
    static constexpr float kSpeed = 2.5f;          ///< UNITS: metres per second
    static constexpr float kBoardSeconds = 0.3f;   ///< standing still on it this long starts it
    static constexpr float kReturnSeconds = 1.5f;  ///< left alone at the top this long sends it back down

    Lift() = default;

    /// x0 .. x1, z0 .. z1: the footprint; bottom / top: the world y of its TOP surface at the two stops.
    Lift(float x0, float x1, float z0, float z1, float bottom, float top) : m_x0(x0), m_x1(x1), m_z0(z0), m_z1(z1), m_bottom(bottom), m_top(top), m_y(bottom)
    {
    }

    /// `feet`, `grounded`: the body as it is now. Returns the move of the platform in y.
    float Update(const Vec3& feet, bool grounded, float dt)
    {
        if (!(dt > 0.0f))
        {
            return 0.0f;
        }
        const bool on = OnIt(feet, grounded);
        float target = m_y;
        if (on)
        {
            m_waitOn += dt;
            m_waitOff = 0.0f;
            if (m_waitOn >= kBoardSeconds || m_y > m_bottom + 1.0e-3f)
            {
                target = m_top; // boarded: up (and keep going up while it stays on)
            }
        }
        else
        {
            m_waitOn = 0.0f;
            m_waitOff += dt;
            if (m_waitOff >= kReturnSeconds)
            {
                target = m_bottom;
            }
        }
        const float step = kSpeed * dt;
        float next = m_y;
        if (target > m_y)
        {
            next = m_y + step > target ? target : m_y + step;
        }
        else if (target < m_y)
        {
            next = m_y - step < target ? target : m_y - step;
        }
        const float moved = next - m_y;
        m_y = next;
        return moved;
    }

    /// True when a body standing at `feet` is on the platform.
    bool OnIt(const Vec3& feet, bool grounded) const
    {
        return grounded && feet.x > m_x0 && feet.x < m_x1 && feet.z > m_z0 && feet.z < m_z1 && std::fabs(feet.y - m_y) < 0.15f;
    }

    float Height() const
    {
        return m_y;
    }

    bool AtTop() const
    {
        return std::fabs(m_y - m_top) < 1.0e-3f;
    }

    bool AtBottom() const
    {
        return std::fabs(m_y - m_bottom) < 1.0e-3f;
    }

    /// The platform as a solid and drawn box (0.3 m thick, both layers).
    SceneObject Platform() const
    {
        SceneObject o = Box(m_x0, m_y - 0.3f, m_z0, m_x1, m_y, m_z1, kWood, kLayerBoth);
        o.glow = 0.05f; // not the city: it is a thing that moves, so it keeps its own colour
        return o;
    }

  private:
    float m_x0 = 0.0f;
    float m_x1 = 0.0f;
    float m_z0 = 0.0f;
    float m_z1 = 0.0f;
    float m_bottom = 0.0f;
    float m_top = 0.0f;
    float m_y = 0.0f;
    float m_waitOn = 0.0f;
    float m_waitOff = 0.0f;
};

} // namespace dl::shapes
