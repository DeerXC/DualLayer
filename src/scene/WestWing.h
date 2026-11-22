// WestWing.h - the moving parts of the west wing, the archive (docs/LEVEL_DESIGN.md, 6.3): what an orb does when it is carried
//            between the layers. The fixed boxes of the wing are in scene/Stage.h; the rules are in scene/Mechanisms.h.
//   W1 hand it over   the hall is cut by a trench wider than a running jump. An orb lies on the north floor in A; a socket of B
//                     stands by the trench. An orb on it in B lays a bridge of B across. "The thing in my hand went with me."
//   W2 leave it       on the south floor an orb and a plate of A; at the foot of the ramp down a door of B (a red panel fills
//                     the doorway in A). The orb on the plate in A holds the door open for a body in B - but the orb is needed
//                     beyond the door. A lever just inside keeps the door open for good; then the orb is fetched.
//   W3 let it fall    the lower room has a mezzanine with a hole too narrow for the body, closed by a red lid in A and open in
//                     B. Under it, behind bars, a socket of B. The orb put down over the hole in B falls through onto the
//                     socket, which opens the bars for good. "In A the floor was closed - the orb fell in the other world."
//   W4 the seal       inside the vault; it opens the door at the top of the ramp up, back into the hub.
// Why it holds:      tests/test_stage.cpp walks the wing with the real body and the orbs, and tries the ways round: the trench
//                    in either layer, the door in A and in B without the orb, the orb carried through the door without the
//                    lever, the orb dropped in A, the body through the hole; tests/test_reach.cpp searches the wing for skips.
// Plain data, no Windows, no Direct3D. UNITS: metres.
#pragma once

#include "scene/Mechanisms.h"
#include "scene/Stage.h"

namespace dl::stage
{

inline constexpr int kW1Socket = 0;
inline constexpr int kW2Plate = 1;
inline constexpr int kW2Lever = 2;
inline constexpr int kW3Socket = 3;

inline constexpr mech::Trigger kWestTriggers[] = {
    {mech::TriggerKind::Socket, {-17.0f, kFloorY, 31.5f}, 1, false, "W1 socket (B)"},
    {mech::TriggerKind::Plate, {-23.0f, kFloorY, 23.25f}, 0, false, "W2 plate (A)"},
    {mech::TriggerKind::Lever, {-23.4f, kWestLowY + 1.1f, 10.8f}, 0, true, "W2 lever"},
    {mech::TriggerKind::Socket, {kHoleX, kWestLowY, kHoleZ}, 1, true, "W3 socket (B)"},
};

inline constexpr int kW1Bridge = 0;
inline constexpr int kW2Door = 1;
inline constexpr int kW3Bars = 2;

inline constexpr mech::Gate kWestGates[] = {
    {Span(-21.0f, kFloorY - 0.2f, kTrenchSouth, -18.0f, kFloorY, kTrenchNorth, kDebris, kLayerB), kW1Socket, -1, true, "W1 bridge"},
    {Span(-22.0f, kWestLowY, kWestDoorSouth, -18.0f, kWestDoorTop, kWestDoorNorth, kDebris, kLayerB), kW2Plate, kW2Lever, false, "W2 door"},
    {Span(-24.0f, kWestLowY, kMezzNorth, kVaultEast, kMezzY - 0.2f, kMezzNorth + 0.5f, kDark, kLayerBoth), kW3Socket, -1, false, "W3 bars"},
};

inline constexpr mech::OrbStart kWestOrbs[] = {
    {{-21.0f, kFloorY + mech::kOrbRadius, 37.0f}, 0, RoomIndex("west hall")},  // W1's orb, on the north floor
    {{-16.5f, kFloorY + mech::kOrbRadius, 23.25f}, 0, RoomIndex("west hall")}, // W2's orb, on the south floor
};
static_assert(RoomIndex("west hall") >= 0, "scene/Stage.h: the room 'west hall' is gone");

/// The hole in the mezzanine: an orb put down near it is placed over it.
inline constexpr Vec3 kWestSpots[] = {
    {kHoleX, kMezzY, kHoleZ},
};

inline mech::WingDesc WestWing()
{
    mech::WingDesc d;
    d.triggers = kWestTriggers;
    d.triggerCount = static_cast<int>(sizeof(kWestTriggers) / sizeof(kWestTriggers[0]));
    d.gates = kWestGates;
    d.gateCount = static_cast<int>(sizeof(kWestGates) / sizeof(kWestGates[0]));
    d.orbs = kWestOrbs;
    d.orbCount = static_cast<int>(sizeof(kWestOrbs) / sizeof(kWestOrbs[0]));
    d.spots = kWestSpots;
    d.spotCount = static_cast<int>(sizeof(kWestSpots) / sizeof(kWestSpots[0]));
    d.fallY = kFallY;
    return d;
}

} // namespace dl::stage
