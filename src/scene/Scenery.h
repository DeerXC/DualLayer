// Scenery.h - the town round the map that no body reaches, drawn only and baked into the static batch with the ground
//            (scene/Landscape.h BakeWorld): the harbour of beat 0, the town wall round the town, the bell tower, and the doors
//            and windows of the houses (docs/LEVEL_DESIGN.md 5.5, 6.1 and 6.6; step 5 and part of step 6 of section 12).
// Harbour:   the basin under the town wall: its side walls, the sea wall beside the guardhouse (the sea gate of the town), a
//            mole with a lighthouse to the south-east, boats moored on the west side (A); in B the mole is broken, the
//            lighthouse a stump, a wreck lies on the west side, and the boats are gone.
// Wall:      the town wall round the levelled town, with merlons (a quarter fallen in B) and towers (one in three a stump in B).
// Tower:     the bell tower stands west of the west wing at (kBellTowerX, kBellTowerZ) until the square is rebuilt (step 6). A:
//            an octagonal shaft, a ring of twelve lamps (the memory lights taken), a belfry with its bell, a stepped spire to
//            about 64 m. B: the shaft broken at about 24 m; its top fell south and lies on the ground in three pieces along
//            the line of the fall (the shaft, the belfry, the spire), as a tall tower falls; the bell cracked at its foot. The
//            houses under the line of the fall stand in A only; in B a low heap of rubble is where they were.
// Houses:    each house gets a door on the side that faces the cathedral and windows on its upper floor; in A some windows
//            are lit (the evening); in B the shells have none above the height they were broken at.
// Districts: the theatre of the north (its seats, the
//            stage), the graveyard outside the north wall with the night bell tower, the water tower over the locks of the east;
//            and a lamp at every other crossing of the streets (docs/LEVEL_DESIGN.md 5.8).
// Rule:      nothing here is solid, and nothing stands where a body can land on it: every box is 4.5 m or more from every room,
//            or out of reach above it (tests/test_terrain.cpp, TestScenery).
// Plain data, no Windows, no Direct3D. UNITS: metres. Space: left-handed, +Y up.
#pragma once

#include "scene/SceneData.h"
#include "scene/Stage.h"

#include <cmath>
#include <vector>

namespace dl::stage
{

inline constexpr float kTownWallX = 64.0f;   ///< the town wall: x -64 .. 64, z -44 .. 106 (outside the plots of the town)
inline constexpr float kTownWallZ0 = -44.0f;
inline constexpr float kTownWallZ1 = 106.0f;
inline constexpr float kTownWallTop = kPlainY + 9.0f;
inline constexpr float kTownWallThick = 1.2f;

inline constexpr Rgb kBellStone{0.84f, 0.80f, 0.71f};
inline constexpr Rgb kBellStoneB{0.47f, 0.48f, 0.49f};
inline constexpr Rgb kHull{0.36f, 0.25f, 0.17f};
inline constexpr Rgb kSail{0.93f, 0.90f, 0.82f};
inline constexpr Rgb kDoorWood{0.30f, 0.20f, 0.13f};
inline constexpr Rgb kWindowDark{0.13f, 0.13f, 0.15f};
inline constexpr Rgb kWindowLit{1.0f, 0.78f, 0.45f};
inline constexpr Rgb kBellBronze{0.70f, 0.55f, 0.28f};

/// An octagonal prism standing from y0 to y1 round (x, z): two boxes, the second turned by 45 degrees.
inline void Octagon(float x, float z, float r, float y0, float y1, const Rgb& c, unsigned mask, std::vector<SceneObject>& out)
{
    const float w = 2.0f * r * 0.9238795f; // across the flats
    for (int k = 0; k < 2; ++k)
    {
        SceneObject o = Span(x - 0.5f * w, y0, z - 0.5f * w, x + 0.5f * w, y1, z + 0.5f * w, c, mask);
        o.yaw = 0.3926991f + 0.7853982f * static_cast<float>(k);
        out.push_back(o);
    }
}

/// An octagonal prism lying on its side along the line of the fall (toward -z), its middle at (x, y, z), `length` long.
inline void LyingOctagon(float x, float y, float z, float r, float length, const Rgb& c, unsigned mask, std::vector<SceneObject>& out)
{
    const float w = 2.0f * r * 0.9238795f;
    for (int k = 0; k < 2; ++k)
    {
        // a box along X turned about X (the octagon), then turned by 90 degrees about Y: its long axis runs along -Z
        SceneObject o = Span(x - 0.5f * length, y - 0.5f * w, z - 0.5f * w, x + 0.5f * length, y + 0.5f * w, z + 0.5f * w, c, mask);
        o.tilt = 0.3926991f + 0.7853982f * static_cast<float>(k);
        o.yaw = 1.5707963f;
        out.push_back(o);
    }
}

// ---- the harbour

inline void HarbourScenery(std::vector<SceneObject>& out)
{
    const float quayEdge = kPlainY + 0.6f;
    // the side walls of the basin and the sea wall beside the guardhouse (the sea gate)
    out.push_back(Span(kHarbourX0 - 1.0f, kPitY, kHarbourZ0, kHarbourX0, quayEdge, kTerraceSouth, kStone, kLayerBoth));
    out.push_back(Span(kHarbourX1, kPitY, kHarbourZ0, kHarbourX1 + 1.0f, quayEdge, kTerraceSouth, kStone, kLayerBoth));
    out.push_back(Span(kHarbourX0 - 1.0f, kPitY, kTerraceSouth - 0.5f, -3.0f, kGuardWallTop - 1.0f, kTerraceSouth, kStone, kLayerBoth));
    out.push_back(Span(3.0f, kPitY, kTerraceSouth - 0.5f, kHarbourX1 + 1.0f, kGuardWallTop - 1.0f, kTerraceSouth, kStone, kLayerBoth));
    for (float x = kHarbourX0; x < -3.5f; x += 2.0f) // merlons on the sea wall
    {
        out.push_back(Span(x, kGuardWallTop - 1.0f, kTerraceSouth - 0.5f, x + 1.0f, kGuardWallTop - 0.2f, kTerraceSouth, kStone, kLayerBoth));
    }
    for (float x = 4.0f; x < kHarbourX1; x += 2.0f)
    {
        out.push_back(Span(x, kGuardWallTop - 1.0f, kTerraceSouth - 0.5f, x + 1.0f, kGuardWallTop - 0.2f, kTerraceSouth, kStone, kLayerBoth));
    }
    // the mole to the south-east, its middle broken in B, the lighthouse at its end
    const float moleY = kHarbourY + 0.3f;
    out.push_back(Span(19.0f, kPitY, -60.0f, 22.5f, moleY, -54.0f, kGrey, kLayerBoth));
    out.push_back(Span(19.0f, kPitY, -54.0f, 22.5f, moleY, -51.0f, kGrey, kLayerA));
    out.push_back(Span(19.0f, kPitY, -54.0f, 22.5f, kSeaY + 0.4f, -51.0f, kDebris, kLayerB));
    out.push_back(Span(19.0f, kPitY, -51.0f, 22.5f, moleY, -47.0f, kGrey, kLayerBoth));
    Octagon(20.75f, -58.0f, 1.6f, moleY, moleY + 11.0f, kBellStone, kLayerA, out);
    Octagon(20.75f, -58.0f, 1.9f, moleY + 11.0f, moleY + 12.4f, Rgb{1.0f, 0.82f, 0.36f}, kLayerA, out);
    out.back().glow = 0.8f;
    out[out.size() - 2].glow = 0.8f;
    Octagon(20.75f, -58.0f, 1.2f, moleY + 12.4f, moleY + 13.6f, kStone, kLayerA, out);
    Octagon(20.75f, -58.0f, 1.6f, moleY, moleY + 4.5f, kBellStoneB, kLayerB, out); // a stump in B
    // A: boats moored on the west side of the basin, their masts; B: a wreck there, half sunk
    const float boatZ[3] = {-50.0f, -42.0f, -34.0f};
    for (int k = 0; k < 3; ++k)
    {
        const float z = boatZ[k];
        out.push_back(Span(-22.4f, kSeaY - 0.6f, z - 2.6f, -19.8f, kSeaY + 0.8f, z + 2.6f, kHull, kLayerA));
        out.push_back(Span(-21.2f, kSeaY + 0.8f, z - 0.1f, -21.0f, kSeaY + 6.0f - static_cast<float>(k), z + 0.1f, kHull, kLayerA));
    }
    SceneObject wreck = Span(-22.6f, kSeaY - 1.5f, -46.0f, -19.6f, kSeaY + 0.6f, -38.0f, Rgb{0.27f, 0.22f, 0.19f}, kLayerB);
    wreck.tilt = 0.25f;
    out.push_back(wreck);
    // A: ships far out at sea, under sail (nothing out there is reached)
    const float ships[3][3] = {{60.0f, -170.0f, 16.0f}, {-40.0f, -230.0f, 18.0f}, {110.0f, -260.0f, 14.0f}};
    for (const auto& sh : ships)
    {
        const float l = sh[2];
        out.push_back(Span(sh[0] - 0.14f * l, kSeaY - 0.6f, sh[1] - 0.5f * l, sh[0] + 0.14f * l, kSeaY + 1.4f, sh[1] + 0.5f * l, kHull, kLayerA));
        out.push_back(Span(sh[0] - 0.05f * l, kSeaY + 1.6f, sh[1] - 0.25f * l, sh[0] + 0.05f * l, kSeaY + 0.9f * l, sh[1] + 0.25f * l, kSail, kLayerA));
    }
}

// ---- the town wall

inline void TownWall(std::vector<SceneObject>& out)
{
    const float y0 = kPlainY - 1.0f;
    const float t = kTownWallThick;
    const float h = kTownWallTop;
    // west, east, north; the south runs along the shore on either side of the harbour
    out.push_back(Span(-kTownWallX - t, y0, kTownWallZ0, -kTownWallX, h, kTownWallZ1 + t, kStone, kLayerBoth));
    out.push_back(Span(kTownWallX, y0, kTownWallZ0, kTownWallX + t, h, kTownWallZ1 + t, kStone, kLayerBoth));
    out.push_back(Span(-kTownWallX - t, y0, kTownWallZ1, kTownWallX + t, h, kTownWallZ1 + t, kStone, kLayerBoth));
    out.push_back(Span(-kTownWallX - t, y0, kTownWallZ0 - t, kHarbourX0 - 1.0f, h, kTownWallZ0, kStone, kLayerBoth));
    out.push_back(Span(kHarbourX1 + 1.0f, y0, kTownWallZ0 - t, kTownWallX + t, h, kTownWallZ0, kStone, kLayerBoth));
    // merlons on the outer edge, a quarter fallen in B
    int k = 0;
    const auto merlon = [&](float x0, float z0, float x1, float z1) {
        ++k;
        out.push_back(Span(x0, h, z0, x1, h + 0.9f, z1, kStone, (k * 7) % 4 == 0 ? kLayerA : kLayerBoth));
    };
    for (float z = kTownWallZ0; z < kTownWallZ1; z += 2.0f)
    {
        merlon(-kTownWallX - t, z, -kTownWallX - t + 0.45f, z + 1.0f);
        merlon(kTownWallX + t - 0.45f, z, kTownWallX + t, z + 1.0f);
    }
    for (float x = -kTownWallX; x < kTownWallX; x += 2.0f)
    {
        merlon(x, kTownWallZ1 + t - 0.45f, x + 1.0f, kTownWallZ1 + t);
        if (x < kHarbourX0 - 1.5f || x > kHarbourX1 + 1.0f)
        {
            merlon(x, kTownWallZ0 - t, x + 1.0f, kTownWallZ0 - t + 0.45f);
        }
    }
    // towers: at the corners and along the sides, one in three a stump in B
    int n = 0;
    const auto tower = [&](float x, float z) {
        ++n;
        const bool stump = n % 3 == 0;
        out.push_back(Span(x - 2.6f, y0, z - 2.6f, x + 2.6f, h + 4.0f, z + 2.6f, kStone, stump ? kLayerA : kLayerBoth));
        if (stump)
        {
            out.push_back(Span(x - 2.6f, y0, z - 2.6f, x + 2.6f, h - 2.0f, z + 2.6f, kBellStoneB, kLayerB));
        }
        const std::size_t roof = out.size();
        shapes::Roof(x - 2.9f, x + 2.9f, z - 2.9f, z + 2.9f, h + 4.0f, h + 6.2f, out);
        for (std::size_t i = roof; i < out.size(); ++i)
        {
            out[i].layerMask = kLayerA;
        }
    };
    for (float z = kTownWallZ0; z <= kTownWallZ1 + 0.1f; z += 30.0f)
    {
        tower(-kTownWallX - 0.5f * t, z);
        tower(kTownWallX + 0.5f * t, z);
    }
    for (float x = -kTownWallX + 32.0f; x < kTownWallX - 10.0f; x += 32.0f)
    {
        tower(x, kTownWallZ1 + 0.5f * t);
    }
}

// ---- the bell tower

inline void BellTower(std::vector<SceneObject>& out)
{
    const float x = kBellTowerX;
    const float z = kBellTowerZ;
    const float r = kBellTowerR;
    const float base = kPlainY - 1.0f;
    // A: the shaft, a cornice, the ring of twelve lamps, the belfry, the bell, the spire in steps
    Octagon(x, z, r + 0.6f, base, kFloorY + 3.0f, kBellStone, kLayerBoth, out); // the plinth (both: it stands in B as well)
    Octagon(x, z, r, kFloorY + 3.0f, 40.0f, kBellStone, kLayerA, out);
    Octagon(x, z, r + 0.4f, 40.0f, 40.8f, kBellStone, kLayerA, out);
    for (int k = 0; k < 12; ++k)
    {
        const float a = 6.2831853f * static_cast<float>(k) / 12.0f;
        SceneObject lamp = Span(x + (r + 0.7f) * std::cos(a) - 0.25f, 30.0f, z + (r + 0.7f) * std::sin(a) - 0.25f, x + (r + 0.7f) * std::cos(a) + 0.25f,
                                30.6f, z + (r + 0.7f) * std::sin(a) + 0.25f, Rgb{0.82f, 0.92f, 1.0f}, kLayerA);
        lamp.glow = 0.5f;
        out.push_back(lamp);
    }
    for (int k = 0; k < 8; ++k) // the piers of the belfry
    {
        const float a = 0.3926991f + 0.7853982f * static_cast<float>(k);
        const float px = x + (r - 0.4f) * std::cos(a);
        const float pz = z + (r - 0.4f) * std::sin(a);
        SceneObject pier = Span(px - 0.45f, 40.8f, pz - 0.45f, px + 0.45f, 48.0f, pz + 0.45f, kBellStone, kLayerA);
        pier.yaw = -a;
        out.push_back(pier);
    }
    Octagon(x, z, 1.9f, 42.0f, 45.5f, kBellBronze, kLayerA, out);
    out.back().glow = 0.25f;
    out[out.size() - 2].glow = 0.25f;
    Octagon(x, z, r + 0.3f, 48.0f, 49.0f, kBellStone, kLayerA, out);
    for (int k = 0; k < 8; ++k) // the spire: eight tiers, each smaller
    {
        const float f = 1.0f - static_cast<float>(k) / 8.0f;
        Octagon(x, z, 0.3f + (r - 0.3f) * f, 49.0f + 1.875f * static_cast<float>(k), 49.0f + 1.875f * static_cast<float>(k + 1), kBellStone, kLayerA, out);
    }
    Octagon(x, z, 0.25f, kBellTowerTop - 1.0f, kBellTowerTop, Rgb{1.0f, 0.82f, 0.36f}, kLayerA, out);
    // B: the stump, broken teeth, the fallen top in three pieces along the line of the fall, the cracked bell at its foot
    Octagon(x, z, r, kFloorY + 3.0f, kBrokenAt, kBellStoneB, kLayerB, out);
    for (int k = 0; k < 6; ++k)
    {
        const float a = 6.2831853f * static_cast<float>(k) / 6.0f;
        const float tx = x + (r - 0.8f) * std::cos(a);
        const float tz = z + (r - 0.8f) * std::sin(a);
        SceneObject tooth = Span(tx - 0.9f, kBrokenAt, tz - 0.5f, tx + 0.9f, kBrokenAt + 1.0f + static_cast<float>((k * 5) % 4), tz + 0.5f, kBellStoneB, kLayerB);
        tooth.yaw = -a;
        out.push_back(tooth);
    }
    const float z1 = kFallZ1; // the fall runs south from here (z1 - 16: the end of the shaft)
    LyingOctagon(x, kPlainY + r - 0.4f, z1 - 8.0f, r, 16.0f, kBellStoneB, kLayerB, out);                                      // the shaft, 16 m
    SceneObject belfry = Span(x - 4.0f, kPlainY - 0.5f, z1 - 25.0f, x + 4.0f, kPlainY + 6.5f, z1 - 17.5f, kBellStoneB, kLayerB); // the belfry
    belfry.yaw = 0.35f;
    out.push_back(belfry);
    for (int k = 0; k < 4; ++k) // the spire, tier after tier along the fall
    {
        const float rr = 2.8f - 0.6f * static_cast<float>(k);
        LyingOctagon(x + 0.4f * static_cast<float>(k), kPlainY + rr - 0.3f, z1 - 28.0f - 3.0f * static_cast<float>(k), rr, 3.0f, kBellStoneB, kLayerB, out);
    }
    SceneObject bellA = Span(x + 5.0f, kPlainY - 0.4f, z - 2.0f, x + 8.6f, kPlainY + 2.4f, z + 1.6f, kBellBronze, kLayerB); // the bell, cracked
    bellA.tilt = 0.5f;
    bellA.yaw = 0.4f;
    out.push_back(bellA);
    // the rubble over the plots under the line of the fall (their houses stand in A only: Skyline)
    out.push_back(Span(x - 4.5f, kPlainY - 0.2f, kFallZ0, x + 4.5f, kPlainY + 1.2f, kFallZ1, kDebris, kLayerB));
}

// ---- doors and windows of the houses

inline void HouseDetails(std::vector<SceneObject>& out)
{
    unsigned state = 707u;
    const auto next = [&state]() {
        state = state * 1664525u + 1013904223u;
        return static_cast<float>((state >> 8) & 0xFFFFu) / 65535.0f;
    };
    for (const Plot& p : SkylinePlots())
    {
        if (p.kind != PlotKind::House)
        {
            continue;
        }
        const float cx = 0.5f * (p.x0 + p.x1);
        const float cz = 0.5f * (p.z0 + p.z1);
        const float h = 0.5f * p.side;
        const unsigned inB = UnderTheFall(p.x0, p.z0, p.x1, p.z1) ? 0u : kLayerB;
        // the face toward the cathedral, in the frame of the house (local +x: world (c, -s); local +z: world (s, c))
        const float c = std::cos(p.yaw);
        const float s = std::sin(p.yaw);
        const float dx = kCathedralX - cx;
        const float dz = kCathedralZ - cz;
        const float alongX = dx * c - dz * s;
        const float alongZ = dx * s + dz * c;
        const bool doorOnX = std::fabs(alongX) > std::fabs(alongZ);
        const float doorSign = doorOnX ? (alongX > 0.0f ? 1.0f : -1.0f) : (alongZ > 0.0f ? 1.0f : -1.0f);
        const std::size_t first = out.size();
        const float proud = 0.05f;
        const auto onFace = [&](bool faceX, float sign, float along, float y0, float y1, float halfWidth, const Rgb& col, unsigned mask, float glow) {
            SceneObject o = faceX ? Span(cx + sign * h - (sign > 0.0f ? 0.0f : proud), y0, cz + along - halfWidth, cx + sign * h + (sign > 0.0f ? proud : 0.0f), y1,
                                         cz + along + halfWidth, col, mask)
                                  : Span(cx + along - halfWidth, y0, cz + sign * h - (sign > 0.0f ? 0.0f : proud), cx + along + halfWidth, y1,
                                         cz + sign * h + (sign > 0.0f ? proud : 0.0f), col, mask);
            o.glow = glow;
            out.push_back(o);
        };
        onFace(doorOnX, doorSign, 0.0f, kFloorY, kFloorY + 2.0f, 0.5f, kDoorWood, kLayerA | inB, 0.0f);
        if (p.height >= 4.5f)
        {
            const float wy0 = kFloorY + 0.62f * p.height;
            const float wy1 = wy0 + 1.0f;
            for (int face = 0; face < 4; ++face)
            {
                const bool faceX = face < 2;
                const float sign = face % 2 == 0 ? 1.0f : -1.0f;
                for (float along : {-0.28f * p.side, 0.28f * p.side})
                {
                    const bool lit = next() < 0.3f;
                    onFace(faceX, sign, along, wy0, wy1, 0.32f, lit ? kWindowLit : kWindowDark, kLayerA, lit ? 0.7f : 0.0f);
                    if (!p.ruin && inB != 0u && wy1 < kFloorY + p.height - 0.2f)
                    {
                        onFace(faceX, sign, along, wy0, wy1, 0.32f, kWindowDark, kLayerB, 0.0f); // B: dark, empty
                    }
                }
            }
        }
        TurnAbout(out, first, cx, cz, p.yaw);
    }
}

// ---- the market of the west is part of the map (scene/Stage.h, docs/LEVEL_DESIGN.md 6.3.1): its square, its well, its stalls
// and its clock tower are walked, so they are solid boxes of the map and its drawn decor, not scenery.

inline constexpr Rgb kPaving{0.64f, 0.60f, 0.52f};

// ---- the theatre of the north (docs/LEVEL_DESIGN.md 5.8): an open bowl north of the north hall: the orchestra, the stage and
// its front wall against the stage tower (kLandmarks), the seats in rows rising north. In B the stage front stands lower, the
// upper rows on the west have fallen, and grass grows in the orchestra.

inline constexpr Rgb kSeatStone{0.78f, 0.71f, 0.60f};

inline void TheatreScenery(std::vector<SceneObject>& out)
{
    const float y = kPlainY;
    const float cx = kTheatreX;
    const float cz = kTheatreZ;
    Octagon(cx, cz, 6.0f, y - 0.2f, y + 0.08f, kPaving, kLayerBoth, out); // the orchestra
    SceneObject grass = Span(cx - 3.0f, y + 0.08f, cz - 1.0f, cx + 2.0f, y + 0.1f, cz + 3.0f, Rgb{0.36f, 0.44f, 0.28f}, kLayerB);
    out.push_back(grass);
    out.push_back(Span(cx - 9.0f, y - 0.2f, 67.5f, cx + 9.0f, y + 1.0f, 70.5f, kWood, kLayerBoth)); // the stage
    out.push_back(Span(cx - 9.0f, y - 0.2f, 67.0f, cx - 2.5f, y + 7.0f, 67.5f, kStone, kLayerA));  // its front wall (A)
    out.push_back(Span(cx + 2.5f, y - 0.2f, 67.0f, cx + 9.0f, y + 7.0f, 67.5f, kStone, kLayerA));
    out.push_back(Span(cx - 9.0f, y - 0.2f, 67.0f, cx - 2.5f, y + 3.0f, 67.5f, kBellStoneB, kLayerB)); // (B: lower)
    out.push_back(Span(cx + 2.5f, y - 0.2f, 67.0f, cx + 9.0f, y + 2.2f, 67.5f, kBellStoneB, kLayerB));
    // the scenery of the evening's play (A): a painted stair against the stage front
    SceneObject scenery = Span(cx - 7.5f, y + 1.0f, 68.0f, cx - 5.5f, y + 5.5f, 68.6f, Rgb{0.56f, 0.44f, 0.62f}, kLayerA);
    scenery.tilt = -0.35f;
    out.push_back(scenery);
    // the seats: seven rows on a half circle north of the orchestra, each 0.55 m higher
    for (int row = 0; row < 7; ++row)
    {
        const float rad = 7.5f + 1.3f * static_cast<float>(row);
        const float top = y + 0.55f * static_cast<float>(row + 1);
        const int segments = 10 + 2 * row;
        for (int k = 0; k < segments; ++k)
        {
            const float a = 3.14159265f * (static_cast<float>(k) + 0.5f) / static_cast<float>(segments);
            const float px = cx + rad * std::cos(a);
            const float pz = cz + rad * std::sin(a);
            const float len = 3.14159265f * rad / static_cast<float>(segments) + 0.15f;
            const bool fallen = row >= 4 && px < cx - 2.0f; // B: the upper rows on the west have fallen
            SceneObject seat = Span(px - 0.5f * len, y - 0.2f, pz - 0.65f, px + 0.5f * len, top, pz + 0.65f, kSeatStone, fallen ? kLayerA : kLayerBoth);
            seat.yaw = 1.5707963f - a;
            out.push_back(seat);
        }
    }
    for (int k = 0; k < 5; ++k) // B: where the upper west rows were, their stones lie in a heap
    {
        out.push_back(Span(cx - 15.0f + 2.0f * static_cast<float>(k), y - 0.2f, cz + 9.0f + static_cast<float>(k % 2), cx - 13.4f + 2.0f * static_cast<float>(k),
                           y + 0.6f + 0.3f * static_cast<float>(k % 3), cz + 10.6f + static_cast<float>(k % 2), kBellStoneB, kLayerB));
    }
}

// ---- the graveyard on the hillside outside the north wall, with the night bell tower; in B a grave apart from the others,
// with a stone lantern (docs/LEVEL_DESIGN.md 9.2)

inline constexpr float kGraveyardZ0 = 112.0f;
inline constexpr float kGraveyardZ1 = 128.0f;

template <class HeightAt>
void GraveyardScenery(HeightAt heightAt, std::vector<SceneObject>& out)
{
    for (float z = kGraveyardZ0 + 1.0f; z < kGraveyardZ1 - 1.0f; z += 2.6f)
    {
        for (float x = -16.0f; x <= 16.0f; x += 2.2f)
        {
            const int k = static_cast<int>((x + 20.0f) * 3.0f + z);
            if (k % 5 == 0)
            {
                continue;
            }
            const float gy = heightAt(x, z);
            SceneObject stone = Span(x - 0.4f, gy - 0.3f, z - 0.12f, x + 0.4f, gy + 0.9f + 0.1f * static_cast<float>(k % 3), z + 0.12f, kGrey,
                                     k % 7 == 0 ? kLayerA : kLayerBoth);
            stone.yaw = 0.05f * static_cast<float>(k % 5 - 2);
            out.push_back(stone);
            if (k % 4 == 1) // A: fresh flowers on the grave
            {
                SceneObject flowers = Span(x - 0.25f, gy - 0.05f, z + 0.2f, x + 0.25f, gy + 0.15f, z + 0.5f, Rgb{0.86f, 0.38f, 0.42f}, kLayerA);
                out.push_back(flowers);
            }
        }
    }
    const float bx = 12.0f; // the night bell tower
    const float bz = 124.0f;
    const float by = heightAt(bx, bz);
    out.push_back(Span(bx - 2.2f, by - 1.0f, bz - 2.2f, bx + 2.2f, by + 12.0f, bz + 2.2f, kStone, kLayerBoth));
    for (int k = 0; k < 4; ++k)
    {
        const float px = bx + (k % 2 == 0 ? -2.0f : 1.4f);
        const float pz = bz + (k < 2 ? -2.0f : 1.4f);
        out.push_back(Span(px, by + 12.0f, pz, px + 0.6f, by + 15.0f, pz + 0.6f, kStone, kLayerBoth));
    }
    const std::size_t roof = out.size();
    shapes::Roof(bx - 2.5f, bx + 2.5f, bz - 2.5f, bz + 2.5f, by + 15.0f, by + 17.5f, out);
    for (std::size_t i = roof; i < out.size(); ++i)
    {
        out[i].layerMask = kLayerA;
    }
    Octagon(bx, bz, 1.0f, by + 12.6f, by + 14.4f, kBellBronze, kLayerBoth, out);
    // B: the grave apart, with its stone lantern
    const float gx = -18.5f;
    const float gz = 116.0f;
    const float gy = heightAt(gx, gz);
    out.push_back(Span(gx - 0.9f, gy - 0.2f, gz - 1.5f, gx + 0.9f, gy + 0.3f, gz + 1.5f, Rgb{0.40f, 0.36f, 0.30f}, kLayerB));
    out.push_back(Span(gx - 0.2f, gy, gz - 2.2f, gx + 0.2f, gy + 1.1f, gz - 1.8f, kGrey, kLayerB));
    SceneObject lantern = Span(gx - 0.15f, gy + 1.1f, gz - 2.15f, gx + 0.15f, gy + 1.4f, gz - 1.85f, Rgb{1.0f, 0.80f, 0.40f}, kLayerB);
    lantern.glow = 0.6f;
    out.push_back(lantern);
}

// ---- the water tower over the locks (east): in A its shaft, an open stage with the dawn bell and the tank (kLandmarks); in B
// the shaft broken at 20 m and its top fallen beside it

inline void WaterTowerScenery(std::vector<SceneObject>& out)
{
    const float x = kWaterTowerX;
    const float z = kWaterTowerZ;
    Octagon(x, z, 2.8f, kPlainY - 1.0f, 27.0f, kStone, kLayerA, out);
    for (int k = 0; k < 4; ++k)
    {
        const float px = x + (k % 2 == 0 ? -2.4f : 1.8f);
        const float pz = z + (k < 2 ? -2.4f : 1.8f);
        out.push_back(Span(px, 27.0f, pz, px + 0.6f, 31.0f, pz + 0.6f, kStone, kLayerA));
    }
    Octagon(x, z, 1.1f, 28.0f, 30.0f, kBellBronze, kLayerA, out); // the dawn bell
    Octagon(x, z, 2.8f, kPlainY - 1.0f, 20.0f, kBellStoneB, kLayerB, out);
    for (int k = 0; k < 3; ++k) // B: the tank and the bell stage lie south of it
    {
        SceneObject piece = Span(x - 2.5f + 1.5f * static_cast<float>(k), kPlainY - 0.3f, z - 7.5f - 1.2f * static_cast<float>(k), x - 0.5f + 1.5f * static_cast<float>(k),
                                 kPlainY + 1.6f - 0.3f * static_cast<float>(k), z - 5.0f - 1.2f * static_cast<float>(k), kBellStoneB, kLayerB);
        piece.yaw = 0.3f * static_cast<float>(k);
        out.push_back(piece);
    }
}

// ---- the streets: a lamp at every other crossing of the town (a lit lantern in A; in B the post only, its lantern gone)

inline void StreetLamps(std::vector<SceneObject>& out)
{
    int k = 0;
    for (float x = kSkylineX0 + kSkylineCell; x < kSkylineX1 - 1.0f; x += kSkylineCell)
    {
        for (float z = kSkylineZ0 + kSkylineCell; z < kSkylineZ1 - 1.0f; z += kSkylineCell)
        {
            ++k;
            if (k % 2 != 0 || !PlotIsFree(x - 0.3f, z - 0.3f, x + 0.3f, z + 0.3f))
            {
                continue;
            }
            out.push_back(Span(x - 0.07f, kPlainY, z - 0.07f, x + 0.07f, kPlainY + 3.2f, z + 0.07f, kIron, kLayerBoth));
            SceneObject lantern = Span(x - 0.18f, kPlainY + 3.2f, z - 0.18f, x + 0.18f, kPlainY + 3.6f, z + 0.18f, Rgb{1.0f, 0.82f, 0.46f}, kLayerA);
            lantern.glow = 0.7f;
            out.push_back(lantern);
        }
    }
}

/// Everything of this file, for the bake of the world. `heightAt(x, z)`: the height of the ground (for the graveyard on the
/// hillside).
template <class HeightAt>
void Scenery(HeightAt heightAt, std::vector<SceneObject>& out)
{
    HarbourScenery(out);
    TownWall(out);
    BellTower(out);
    HouseDetails(out);
    TheatreScenery(out);
    GraveyardScenery(heightAt, out);
    WaterTowerScenery(out);
    StreetLamps(out);
}

} // namespace dl::stage
