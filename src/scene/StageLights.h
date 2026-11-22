// StageLights.h - the hidden lights of the map. None is on the way to the goal; each asks for something the map teaches,
//            used in a way the main path does not use it (docs/LEVEL_DESIGN.md, section 8; the others come with the rooms
//            of the wings):
//   0 "inside the gate"   buried in the top of the gate. In A it is inside the shut gate and cannot be touched; in B the
//                         gate is gone (a breach), and a full jump under it, beside the main path, reaches it.
//   1 "the floor of the pool"  on the floor of the pool of the square, behind the cathedral. In A it lies under the water:
//                         a body that goes in comes back at the entry before it gets there (main.cpp takes the lights only
//                         after that check). In B the pool is dry and the stairs lead down to it: the water of the west
//                         trench, used a second time.
//   2 "the bed of the canal"   in the west corner of the canal of the east district, under the quay. In A it lies under the
//                         water at either level; in B the canal is dry, and its bed is walked from the stairs at its east
//                         end: the lesson of the pool, again in the district of water.
//   3 "behind the bars"   in the undercroft, seen through its bars from the east rim of the square. The bars open only
//                         from inside; the way in is the breach of B at the west end of the canal, over the crate that the weir
//                         let drift there (E2): moved in A, used in B.
//   4 "in the wall of the north hall"  buried in the west wall over the ledge of N2: only a body standing on the ledge in B
//                         reaches it, after the prism has moved the cage out of B and the step into B.
//   5 "under the courtyard"  in the middle of the cellar, under the puzzle that the player solved at the start: the place is
//                         known, the side is new.
// A light is taken when one of the three balls of the body touches it. Taken lights stay taken for the rest of the run (R
// and a fall do not bring them back). With every light taken, the goal light changes its colour.
// Drawn:     every light not yet taken, in both layers, as a small turning sphere. Never solid.
// Plain data and plain logic, no Windows, no Direct3D: tested without a window (tests/test_lights.cpp). UNITS: metres.
#pragma once

#include "scene/Camera.h" // Vec3
#include "scene/SceneData.h"
#include "scene/Stage.h"

#include <cmath>

namespace dl::stage
{

struct HiddenLight
{
    Vec3 centre;
    const char* name; ///< for the log
};

inline constexpr float kLightRadius = 0.25f; ///< UNITS: metres, the drawn size
inline constexpr float kLightReach = 0.20f;  ///< UNITS: metres; a ball of the body must come this close to the centre (plus
                                             ///< its own radius): smaller than the drawn size, so a light inside a wall cannot
                                             ///< be touched through the wall

inline constexpr HiddenLight kHiddenLights[] = {
    {{-0.9f, kTerraceY + 3.0f, kGateZ}, "inside the gate"},
    {{0.0f, kSquareLowY + 0.3f, 36.25f}, "the floor of the pool"},
    {{15.7f, kCanalBed + 0.25f, kCanalSouth + 0.6f}, "the bed of the canal"},
    {{12.5f, kUndercroftY + 0.6f, 0.5f * (kBayZ0 + kBayZ1)}, "behind the bars"},
    {{-9.15f, kN2LedgeTop + 1.6f, 57.7f}, "in the wall of the north hall"},
    {{0.0f, kCellarY + 0.6f, 0.5f * (kCellarZ0 + kCellarZ1)}, "under the courtyard"},
};
inline constexpr int kHiddenLightCount = static_cast<int>(sizeof(kHiddenLights) / sizeof(kHiddenLights[0]));

/// The colour of the goal light once every hidden light is taken (sRGB): from gold to a cool white.
inline constexpr float kGoalAllColour[3] = {0.80f, 0.95f, 1.00f};

class LightSet
{
  public:
    /// Takes every light that one of the three balls of `body` touches. `Body` needs Ball(i) for i = 0, 1, 2 and
    /// GetSettings().radius (scene/Player.h). Returns the index of a light taken now, or -1 (two at once: the first).
    template <class Body>
    int Collect(const Body& body)
    {
        int taken = -1;
        const float reach = body.GetSettings().radius + kLightReach;
        for (int i = 0; i < kHiddenLightCount; ++i)
        {
            if (m_taken[i])
            {
                continue;
            }
            for (int b = 0; b < 3; ++b)
            {
                const Vec3 p = body.Ball(b);
                const float dx = p.x - kHiddenLights[i].centre.x;
                const float dy = p.y - kHiddenLights[i].centre.y;
                const float dz = p.z - kHiddenLights[i].centre.z;
                if (dx * dx + dy * dy + dz * dz < reach * reach)
                {
                    m_taken[i] = true;
                    ++m_count;
                    if (taken < 0)
                    {
                        taken = i;
                    }
                    break;
                }
            }
        }
        return taken;
    }

    bool Taken(int i) const
    {
        return i >= 0 && i < kHiddenLightCount && m_taken[i];
    }

    int Count() const
    {
        return m_count;
    }

    bool All() const
    {
        return m_count == kHiddenLightCount;
    }

    /// Adds every light not yet taken to the drawn table `out` (`count` used, room for `capacity`). Returns the new count.
    int AddToDrawTable(SceneObject* out, int count, int capacity) const
    {
        if (out == nullptr || count < 0)
        {
            return count;
        }
        for (int i = 0; i < kHiddenLightCount && count < capacity; ++i)
        {
            if (m_taken[i])
            {
                continue;
            }
            SceneObject o{};
            o.shape = ShapeKind::Sphere;
            o.position[0] = kHiddenLights[i].centre.x;
            o.position[1] = kHiddenLights[i].centre.y;
            o.position[2] = kHiddenLights[i].centre.z;
            o.scale[0] = 2.0f * kLightRadius;
            o.scale[1] = 2.0f * kLightRadius;
            o.scale[2] = 2.0f * kLightRadius;
            o.spinSpeed = 1.5f;
            o.colorSrgb[0] = 0.75f;
            o.colorSrgb[1] = 0.95f;
            o.colorSrgb[2] = 1.00f;
            o.shineStrength = 1.0f;
            o.shininess = 120.0f;
            o.layerMask = kLayerBoth;
            o.glow = 0.9f;
            out[count++] = o;
        }
        return count;
    }

  private:
    bool m_taken[kHiddenLightCount] = {};
    int m_count = 0;
};

/// Gives the goal light (the sphere at kGoal, see BuildDrawTable) the colour of a full collection. Returns true when the
/// goal light was found in out[0 .. count - 1].
inline bool RecolourGoal(SceneObject* out, int count)
{
    for (int i = 0; out != nullptr && i < count; ++i)
    {
        SceneObject& o = out[i];
        if (o.shape == ShapeKind::Sphere && std::fabs(o.position[0] - kGoal.x) < 0.01f && std::fabs(o.position[1] - kGoal.y) < 0.01f &&
            std::fabs(o.position[2] - kGoal.z) < 0.01f)
        {
            o.colorSrgb[0] = kGoalAllColour[0];
            o.colorSrgb[1] = kGoalAllColour[1];
            o.colorSrgb[2] = kGoalAllColour[2];
            return true;
        }
    }
    return false;
}

} // namespace dl::stage
