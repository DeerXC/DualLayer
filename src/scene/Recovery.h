// Recovery.h - nobody gets stuck (docs/LEVEL_DESIGN.md, 11 "Recovery"; step 1 of section 12).
// Water:     a switch of worlds is refused into the water of the other world, as it is refused into a wall (SwitchAllowed):
//            the body would only come up again at the entry of its room. Before this rule a body of B in the dry cellar could
//            switch to A, where the cellar is flooded, and was sent to the entry of the cellar, which is under the water of A
//            as well: again and again.
// Respawn:   a fall or R brings the body back beside the lit lamp of its room, else to the entry of the room (scene/Anchors.h).
//            When that place is under the water of the world the body is in, it comes back at the entry of a dry room next to
//            it (kWetFallbacks) instead (SafeRespawn). tests/test_stage.cpp (TestRecovery) checks every room, in both worlds,
//            with the lamp lit and not, with the waters full, raised by the dawn bell and drained, the canal low and high: the
//            place is always a place to stand, out of the water.
// Plain logic, no Windows, no Direct3D. UNITS: metres.
#pragma once

#include "scene/Anchors.h"
#include "scene/Canal.h"
#include "scene/Player.h"
#include "scene/Puzzle.h"
#include "scene/Stage.h"

namespace dl::stage
{

/// True when a body whose feet are at `feet` in `layer` is under water (the static waters and the canal).
inline bool InWater(const Vec3& feet, int layer, const Waterworks& waters, const canal::Canal& canal)
{
    return Submerged(feet, layer, waters) || canal.Submerged(feet, layer);
}

/// True when a body may switch to `target`: it fits there (nothing solid of that world in the way: `solids` is the whole solid
/// table of `target`) and it would not be under the water of that world.
inline bool SwitchAllowed(const Player& body, int target, float spinSeconds, const SceneObject* solids, int count, const Waterworks& waters,
                          const canal::Canal& canal)
{
    return puzzle::Puzzle::FitsIn(body, target, spinSeconds, solids, count) && !InWater(body.Feet(), target, waters, canal);
}

/// The rooms whose place of return can lie under water (in A), and the dry room next to each one where the body comes back then.
struct WetFallback
{
    const char* room;
    const char* dryRoom;
};
inline constexpr WetFallback kWetFallbacks[] = {
    {"cellar", "east hall"},
    {"canal bed", "east hall"},
    {"tower lock", "lock walk"},
};

/// Where a body of `room` in `layer` comes back after a fall or R: beside the lit lamp of the room, else at its entry; when that
/// place is under water in `layer`, at the place of return of the dry room next to it.
inline Vec3 SafeRespawn(int room, int layer, const AnchorSet& anchors, const Waterworks& waters, const canal::Canal& canal)
{
    const Vec3 here = anchors.RespawnPoint(room);
    if (!InWater(here, layer, waters, canal))
    {
        return here;
    }
    for (const WetFallback& f : kWetFallbacks)
    {
        if (room >= 0 && room < kRoomCount && RoomIndex(f.room) == room)
        {
            return anchors.RespawnPoint(RoomIndex(f.dryRoom));
        }
    }
    return kRooms[kStartRoom].entry;
}

} // namespace dl::stage
