// StageLights.h - three hidden lights in the first stage. None of them is on the way to the goal; each one asks for a
//            mechanic of the stage used in a way the main path does not use it:
//   0 "inside the wall"  part 1: a light buried in the top of the red wall. In A it is inside the wall and cannot be
//                        touched; in B the wall is gone and a full jump at the red outline on the floor reaches it.
//   1 "the block, again" part 3: a light high above the west side of the north floor, out of reach of a jump from the floor.
//                        The purple block, pushed under it instead of onto the plate, is the step that reaches it.
//   2 "falling is free"  part 3: a light floating beside the north floor, outside the corridor. Only a jump off the edge
//                        reaches it; the fall afterwards costs nothing, because the body comes back at the checkpoint and a
//                        light, once taken, stays taken.
// A light is taken when one of the three balls of the body touches it. Taken lights stay taken for the rest of the run (R
// and a fall do not bring them back). With all three taken, the goal light changes its colour.
// Drawn:     every light not yet taken, in both layers, as a small turning sphere (a light buried in the wall of A is hidden by
//            the wall in A). Never solid.
// Plain data and plain logic, no Windows, no Direct3D: tested without a window (tests/test_lights.cpp).
// UNITS: metres.
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
    {{-2.4f, kFloorY + 3.0f, 3.0f}, "inside the wall"},
    {{-2.0f, kFloorY + 4.6f, 16.0f}, "the block, again"},
    {{4.0f, kFloorY + 2.0f, 16.0f}, "falling is free"},
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
    int AddToDrawTable(scene::SceneObject* out, int count, int capacity) const
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
            scene::SceneObject o{};
            o.shape = scene::ShapeKind::Sphere;
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
            o.layerMask = scene::kLayerBoth;
            out[count++] = o;
        }
        return count;
    }

  private:
    bool m_taken[kHiddenLightCount] = {};
    int m_count = 0;
};

/// Gives the goal light (the sphere at the goal, see BuildDrawTable) the colour of a full collection. Returns true when the
/// goal light was found in out[0 .. count - 1].
inline bool RecolourGoal(scene::SceneObject* out, int count)
{
    for (int i = 0; out != nullptr && i < count; ++i)
    {
        scene::SceneObject& o = out[i];
        if (o.shape == scene::ShapeKind::Sphere && std::fabs(o.position[2] - kGoalZ) < 0.01f &&
            std::fabs(o.position[1] - (kDoorwayLow + 0.9f)) < 0.01f)
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
