// OrbReturn.h - an orb never gets stuck (docs/LEVEL_DESIGN.md, 14.4): an orb that comes to rest where a body of its world
//            cannot take it back goes back to its start, with the same ring of light as a body that comes back.
// Rule:      E takes an orb from a body standing within mech::kReach of it, its centre between 0.6 m below and 2 m above
//            the feet (scene/Mechanisms.h, OrbInReach). So an orb at rest is reachable when there is such a place to stand:
//            on solid ground of the layer of the orb, with the whole body free there, in a room of the map
//            (scene/Stage.h kRooms: a wall top is in no room) and not under water. OrbStranded looks for one on rings round
//            the orb; finding none, the orb is stranded.
// Limits:    a local test: it asks whether there is a place to stand next to the orb, not whether the way to that place is
//            open. An orb on the socket of a closed vault counts as reachable (the vault opens). It is asked once, when the
//            orb comes to rest (MechanismSet::ConsumeRested), not every frame.
// Plain logic, no Windows, no Direct3D: tested without a window (tests/test_stage.cpp, TestOrbReturn). UNITS: metres.
#pragma once

#include "scene/Canal.h"
#include "scene/Collision.h"
#include "scene/Mechanisms.h"
#include "scene/Player.h"
#include "scene/Stage.h"

#include <cmath>

namespace dl::stage
{

inline constexpr float kOrbRings[] = {0.45f, 0.75f, 1.05f, 1.3f}; ///< UNITS: metres from the orb (all inside mech::kReach)
inline constexpr int kOrbRingSteps = 16;

/// True when a body of `layer` could stand with its feet at (x, ?, z) between `low` and `high` and take the orb: the first
/// ground under `high` (in this column) is found, and the body must be free there, in a room and not under water.
inline bool StandsToTake(float x, float z, float low, float high, int layer, const SceneObject* solids, int count, const canal::Canal& canal)
{
    const Player::Settings body{};
    const float r = body.radius;
    for (float y = high; y >= low; y -= 0.05f)
    {
        const Vec3 c{x, y + r, z};
        const CollisionResult hit = ResolveSphere(c, r, layer, 0.0f, solids, count);
        if (!hit.pushed)
        {
            continue;
        }
        const float dx = hit.position.x - c.x;
        const float dy = hit.position.y - c.y;
        const float dz = hit.position.z - c.z;
        const float length = std::sqrt(dx * dx + dy * dy + dz * dz);
        if (!(dy > 0.0f && dy > Player::kGroundFacing * length))
        {
            continue; // inside a wall at this height: look lower
        }
        const Vec3 feet{x, hit.position.y - r, z};
        if (feet.y < low - 0.05f || feet.y > high + 0.05f || RoomAt(feet) < 0 || Submerged(feet, layer) || canal.Submerged(feet, layer))
        {
            return false; // the first ground of this column is not a place to take the orb from
        }
        for (int b = 1; b < 3; ++b) // the rest of the body must be free (the feet ball rests on the ground)
        {
            const float t = static_cast<float>(b) / 2.0f;
            const Vec3 ball{feet.x, feet.y + r + (body.eyeHeight - r) * t, feet.z};
            const CollisionResult free = ResolveSphere(ball, r, layer, 0.0f, solids, count);
            if (free.pushed && std::fabs(free.position.x - ball.x) + std::fabs(free.position.y - ball.y) + std::fabs(free.position.z - ball.z) > 0.01f)
            {
                return false;
            }
        }
        return true;
    }
    return false;
}

/// True when an orb resting at `centre` in `layer` cannot be taken by any body of its world (`solids`: the solid table of
/// that layer for a BODY, without the water surfaces that hold orbs).
inline bool OrbStranded(const Vec3& centre, int layer, const SceneObject* solids, int count, const canal::Canal& canal)
{
    const float low = centre.y - 2.0f; // the feet may be at most 2 m under the orb and 0.6 m over it
    const float high = centre.y + 0.6f;
    for (float ring : kOrbRings)
    {
        for (int k = 0; k < kOrbRingSteps; ++k)
        {
            const float a = 6.2831853f * static_cast<float>(k) / static_cast<float>(kOrbRingSteps);
            if (StandsToTake(centre.x + ring * std::sin(a), centre.z + ring * std::cos(a), low, high, layer, solids, count, canal))
            {
                return false;
            }
        }
    }
    return true;
}

/// Once per frame after MechanismSet::Update: every orb that came to rest stranded goes back to its start. `solidsA`,
/// `solidsB`: the solid tables of each layer for a body. Returns the last orb sent back, or -1.
inline int ReturnStrandedOrbs(mech::MechanismSet& mech, const SceneObject* solidsA, int countA, const SceneObject* solidsB, int countB,
                              const canal::Canal& canal)
{
    int sent = -1;
    for (int i = 0; i < mech.OrbCount(); ++i)
    {
        if (!mech.ConsumeRested(i))
        {
            continue;
        }
        const mech::MechanismSet::Orb& o = mech.OrbAt(i);
        if (o.held || !o.resting)
        {
            continue;
        }
        const bool inA = o.layer == 0;
        if (OrbStranded(o.centre, o.layer, inA ? solidsA : solidsB, inA ? countA : countB, canal))
        {
            mech.SendBack(i);
            sent = i;
        }
    }
    return sent;
}

} // namespace dl::stage
