// CityParts.h - every moving part of the city in one set (scene/Mechanisms.h): the west wing (scene/WestWing.h), the
//            undercroft of the east district and the prism of the north district. One set, because one E, one R and one
//            drawn table serve them all.
//   undercroft  bars of both layers close the opening of the undercroft in the east wall of the square; they are seen
//               through (the light and the lever behind them are seen from the rim). A lever inside opens them for good:
//               the first visit comes in from the canal, through the breach of B over the drifted crate (scene/Canal.h).
//   N1          a prism on the east part of landing 2 moves the portcullis of A at the far end of the bridge of A into B:
//               then a body in A walks over the bridge to the seal (docs/LEVEL_DESIGN.md, 14.9).
//   N2          a prism on the floor of the north hall moves two things at once: the cage of B over the ledge into A, and the
//               step of A at the foot of the ledge into B. Then B climbs to the hidden light in the wall (optional).
//   cellar      bars of both layers in the grate at the west end of the cellar; a lever inside the lower room of the west wing
//               opens them for good: the cellar becomes a shortcut between the west and the east districts (in B).
// The indices of the west wing (kW1Socket, kW1Bridge, ...) are kept: its triggers and gates come first.
// Plain data, no Windows, no Direct3D. UNITS: metres.
#pragma once

#include "scene/Mechanisms.h"
#include "scene/Stage.h"
#include "scene/WestWing.h"

#include <array>
#include <cstddef>

namespace dl::stage
{

inline constexpr int kWestTriggerCount = static_cast<int>(sizeof(kWestTriggers) / sizeof(kWestTriggers[0]));
inline constexpr int kWestGateCount = static_cast<int>(sizeof(kWestGates) / sizeof(kWestGates[0]));

inline constexpr mech::Trigger kOtherTriggers[] = {
    {mech::TriggerKind::Lever, {10.3f, kUndercroftY + 1.1f, kBayZ0 + 0.35f}, 0, true, "undercroft lever"},
    {mech::TriggerKind::Prism, {2.6f, kNorthHighY, 45.0f}, 0, true, "N1 prism"},
    {mech::TriggerKind::Prism, {-4.0f, kFloorY, 51.0f}, 0, true, "N2 prism"},
    {mech::TriggerKind::Lever, {-11.0f, kWestLowY + 1.1f, 8.4f}, 0, true, "cellar lever"},
};
inline constexpr int kUndercroftLever = kWestTriggerCount + 0;
inline constexpr int kNorthPrism = kWestTriggerCount + 1;
inline constexpr int kN2Prism = kWestTriggerCount + 2;
inline constexpr int kCellarLever = kWestTriggerCount + 3;

/// A bar of the undercroft opening (both layers): 0.1 m thick, so the gaps (0.3 m) are narrower than the body.
constexpr mech::Gate Bar(float z, const char* name)
{
    return {Span(kHubEast, kFloorY, z - 0.05f, kHubEast + 0.5f, kUndercroftTop, z + 0.05f, kIron, kLayerBoth), kUndercroftLever, -1, false, name};
}

/// A bar of the grate at the west end of the cellar (both layers).
constexpr mech::Gate GrateBar(float z, const char* name)
{
    return {Span(-10.0f, kWestLowY, z - 0.05f, -9.5f, kCellarGrateTop, z + 0.05f, kIron, kLayerBoth), kCellarLever, -1, false, name};
}

inline constexpr mech::Gate kOtherGates[] = {
    Bar(26.6f, "undercroft bar 1"),
    Bar(27.0f, "undercroft bar 2"),
    Bar(27.4f, "undercroft bar 3"),
    Bar(27.8f, "undercroft bar 4"),
    {Span(kPortcullisX0, kNorthHighY, kPortcullisZ0, kPortcullisX1, kNorthHighY + kParapet, kPortcullisZ1, kIron, kLayerA), kNorthPrism, -1, false,
     "N1 portcullis", true},
    {Span(-9.0f, kN2LedgeTop, kN2LedgeZ0, kN2LedgeX1, kN2CageTop, kN2LedgeZ1, kIron, kLayerB), kN2Prism, -1, false, "N2 cage", true},
    {Span(kN2LedgeX1, kFloorY, 57.2f, kN2LedgeX1 + 1.0f, kFloorY + 1.2f, 58.2f, kWood, kLayerA), kN2Prism, -1, false, "N2 step", true},
    GrateBar(9.2f, "cellar bar 1"),
    GrateBar(9.6f, "cellar bar 2"),
    GrateBar(10.0f, "cellar bar 3"),
    GrateBar(10.4f, "cellar bar 4"),
    GrateBar(10.8f, "cellar bar 5"),
};
inline constexpr int kUndercroftBars = kWestGateCount + 0; ///< the first of the four bars
inline constexpr int kPortcullis = kWestGateCount + 4;
inline constexpr int kN2Cage = kWestGateCount + 5;
inline constexpr int kN2Step = kWestGateCount + 6;
inline constexpr int kCellarGrate = kWestGateCount + 7; ///< the first of the five bars

inline constexpr int kCityTriggerCount = kWestTriggerCount + static_cast<int>(sizeof(kOtherTriggers) / sizeof(kOtherTriggers[0]));
inline constexpr int kCityGateCount = kWestGateCount + static_cast<int>(sizeof(kOtherGates) / sizeof(kOtherGates[0]));

template <class T, std::size_t N, std::size_t M>
constexpr std::array<T, N + M> Join(const T (&a)[N], const T (&b)[M])
{
    std::array<T, N + M> out{};
    for (std::size_t i = 0; i < N; ++i)
    {
        out[i] = a[i];
    }
    for (std::size_t i = 0; i < M; ++i)
    {
        out[N + i] = b[i];
    }
    return out;
}

inline constexpr auto kCityTriggers = Join(kWestTriggers, kOtherTriggers);
inline constexpr auto kCityGates = Join(kWestGates, kOtherGates);
static_assert(kCityTriggerCount <= mech::kMaxTriggers && kCityGateCount <= mech::kMaxGates, "scene/CityParts.h: too many parts for MechanismSet");

/// Every moving part of the city.
inline mech::WingDesc CityParts()
{
    mech::WingDesc d = WestWing();
    d.triggers = kCityTriggers.data();
    d.triggerCount = kCityTriggerCount;
    d.gates = kCityGates.data();
    d.gateCount = kCityGateCount;
    return d;
}

} // namespace dl::stage
