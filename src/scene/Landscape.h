// Landscape.h - the world round the map: the ground of the town, the hills, the mountain, the coast and the sea, and the
//            plants, all generated from fixed seeds (docs/LEVEL_DESIGN.md, 5.4 .. 5.6; step 2 of section 12).
// Ground:    a height field (scene/Terrain.h) in two parts: 2 m cells round the map, 8 m cells to well past the far plane.
//            The town is levelled at the street (kPlainY, just under the floors of the map), the land round it is shaped by
//            noise: hills east and west, a mountain to the north in ridged layers, rough ground outside the town, a bank that
//            falls to the sea in the south. Under the parts of the map that have no floor at street level (kPits) the
//            ground drops to kPitY, below every fall line: a fall stays a fall, and nothing of the ground shows in a room.
// Sea:       one flat plane at kSeaY, drawn round the eye each frame and wider than the far plane (AddSea): it fills the bay
//            and the pits that are deeper than it. Not solid: a fall into it is a fall.
// Plants:    placed by what grows on the ground there (5.6): the kind by height, slope, the side the slope faces and the
//            wind; never on a grid (points scattered with a distance between them); gardens in the empty plots of the town.
//            Drawn only, both worlds (B darker; the lemon trees of the gardens dead in B).
// Drawn:     BakeWorld(layer) bakes the ground and the plants of a layer into one mesh, flat-shaded (render/StaticBatch.h),
//            joined to the static batch: no draw call of its own.
// Plain data and plain logic, no Windows, no Direct3D: tested without a window (tests/test_terrain.cpp).
// UNITS: metres. Space: left-handed, +Y up.
#pragma once

#include "render/StaticBatch.h"
#include "scene/SceneData.h"
#include "scene/Scenery.h"
#include "scene/Stage.h"
#include "scene/Terrain.h"

#include <cmath>
#include <vector>

namespace dl::stage
{

// ---- the extent: the fine field lies on whole cells of the coarse one (Land::PinEdge)
inline constexpr float kNearX0 = -64.0f;
inline constexpr float kNearX1 = 72.0f;
inline constexpr float kNearZ0 = -72.0f; // (the harbour reaches to z -62)
inline constexpr float kNearZ1 = 112.0f;
inline constexpr float kNearCell = 2.0f;
inline constexpr float kFarX0 = -336.0f;
inline constexpr float kFarX1 = 344.0f;
inline constexpr float kFarZ0 = -152.0f;
inline constexpr float kFarZ1 = 368.0f;
inline constexpr float kFarCell = 8.0f;
/// UNITS: metres: how far the eye sees. KEEP IN SYNC: render/Projection.h kFarPlane (main.cpp checks it when it compiles).
inline constexpr float kViewDistance = 300.0f;

static_assert(kNearCell * 2.0f <= kPitReach, "Stage.h kPitReach must cover the slope into a pit (two cells of the fine field)");

/// The z of the water's edge along x: a bay in front of the town, the shore coming north on either side.
inline float CoastZ(float x)
{
    const float ax = x < 0.0f ? -x : x;
    return -60.0f + 4.0f * terrain::Fbm(x / 26.0f, 3.1f, 3, 7u) + 10.0f * terrain::SmoothStep(60.0f, 140.0f, ax);
}

/// 1 inside the levelled town, 0 outside it, soft at its edge.
inline float TownMask(float x, float z)
{
    const float ax = x < 0.0f ? -x : x;
    const float tx = 1.0f - terrain::SmoothStep(66.0f, 90.0f, ax);
    const float tz = terrain::SmoothStep(-60.0f, -46.0f, z) * (1.0f - terrain::SmoothStep(108.0f, 132.0f, z));
    return tx * tz;
}

/// The height of the land at (x, z), the pits left out.
inline float NaturalHeight(float x, float z)
{
    using terrain::Fbm;
    using terrain::Ridged;
    using terrain::SmoothStep;
    float n = kPlainY;
    n += 16.0f * SmoothStep(60.0f, 170.0f, x) * (0.7f + 0.3f * Fbm(x / 50.0f, z / 50.0f, 2, 3u));  // the hills of the east
    n += 13.0f * SmoothStep(-60.0f, -170.0f, x) * (0.7f + 0.3f * Fbm(x / 50.0f + 4.0f, z / 50.0f, 2, 5u)); // the hills of the west
    n += 6.0f * SmoothStep(80.0f, 150.0f, z);                                                        // the hillside under the mountain
    n += SmoothStep(110.0f, 200.0f, z) * (20.0f + 48.0f * Ridged(x / 90.0f, z / 90.0f, 4, 11u));    // the mountain, in layers
    const float town = TownMask(x, z);
    const float out = std::sqrt(x * x + (z - 30.0f) * (z - 30.0f)); // (not "far": windows.h defines far and near as macros)
    n += (1.5f + 3.0f * SmoothStep(70.0f, 200.0f, out)) * Fbm(x / 24.0f, z / 24.0f, 4, 1u); // rough ground: the town is levelled
    float y = town * kPlainY + (1.0f - town) * n;
    const float cz = CoastZ(x); // the bank down to the sea, and the sea floor
    if (z < cz + 40.0f)
    {
        const float shore = kSeaY + 0.45f * (z - cz);
        y = y < shore ? y : shore;
    }
    if (z < cz)
    {
        float bed = kSeaY - 0.5f - 0.25f * (cz - z);
        bed = bed < kSeaY - 20.0f ? kSeaY - 20.0f : bed;
        y = y < bed ? y : bed;
    }
    return y;
}

/// True when a vertex of the fine field at (x, z) is the floor of a pit: within one cell of a pit, so that every point of a
/// pit lies on triangles whose corners are all at the floor of the pit.
inline bool InPit(float x, float z)
{
    for (const Pit& p : kPits)
    {
        if (x >= p.x0 - kNearCell && x <= p.x1 + kNearCell && z >= p.z0 - kNearCell && z <= p.z1 + kNearCell)
        {
            return true;
        }
    }
    return false;
}

/// The land of the current map.
inline terrain::Land BuildLand()
{
    terrain::Land land;
    land.farField.Build(kFarX0, kFarZ0, kFarCell, static_cast<int>((kFarX1 - kFarX0) / kFarCell), static_cast<int>((kFarZ1 - kFarZ0) / kFarCell),
                        [](float x, float z) { return NaturalHeight(x, z); });
    land.nearField.Build(kNearX0, kNearZ0, kNearCell, static_cast<int>((kNearX1 - kNearX0) / kNearCell),
                         static_cast<int>((kNearZ1 - kNearZ0) / kNearCell),
                         [](float x, float z) { return InPit(x, z) ? kPitY : NaturalHeight(x, z); });
    land.PinEdge();
    return land;
}

/// The land, made once (it is the same every run).
inline const terrain::Land& TheLand()
{
    static const terrain::Land land = BuildLand();
    return land;
}

// ---- the sea

inline constexpr float kSeaHalf = kViewDistance + 60.0f; ///< UNITS: metres: half the side of the sea plane
inline constexpr float kSeaSnap = 20.0f;                  ///< the plane moves in steps of this, so its pattern does not slide

/// Adds the sea, centred under the eye (both worlds, each in its own colour). Returns the new count.
inline int AddSea(const Vec3& eye, SceneObject* out, int count, int capacity)
{
    const float cx = kSeaSnap * std::floor(eye.x / kSeaSnap + 0.5f);
    const float cz = kSeaSnap * std::floor(eye.z / kSeaSnap + 0.5f);
    const Rgb colours[2] = {{0.18f, 0.36f, 0.50f}, {0.20f, 0.26f, 0.30f}};
    const unsigned masks[2] = {kLayerA, kLayerB};
    for (int k = 0; k < 2 && out != nullptr && count >= 0 && count < capacity; ++k)
    {
        SceneObject o = Span(cx - kSeaHalf, kSeaY - 0.2f, cz - kSeaHalf, cx + kSeaHalf, kSeaY, cz + kSeaHalf, colours[k], masks[k]);
        o.shineStrength = 0.6f;
        o.shininess = 60.0f;
        o.glow = 0.03f; // its own colour, not the stone of the town
        out[count++] = o;
    }
    return count;
}

// ---- the colour of the ground

/// The colour (linear) of a triangle of the ground, from its middle and its normal.
inline void GroundColour(float x, float y, float z, float normalY, int layer, float out[4])
{
    struct Palette
    {
        Rgb grass, dry, town, sand, rock, high, pit, bed;
    };
    static constexpr Palette kA{{0.48f, 0.56f, 0.32f}, {0.66f, 0.62f, 0.42f}, {0.60f, 0.57f, 0.47f}, {0.82f, 0.76f, 0.58f},
                                {0.50f, 0.47f, 0.43f}, {0.62f, 0.60f, 0.57f}, {0.36f, 0.34f, 0.31f}, {0.55f, 0.52f, 0.42f}};
    static constexpr Palette kB{{0.30f, 0.38f, 0.28f}, {0.40f, 0.42f, 0.34f}, {0.40f, 0.40f, 0.38f}, {0.55f, 0.53f, 0.46f},
                                {0.33f, 0.34f, 0.35f}, {0.46f, 0.47f, 0.49f}, {0.25f, 0.26f, 0.27f}, {0.35f, 0.36f, 0.33f}};
    const Palette& p = layer == 0 ? kA : kB;
    Rgb c = p.grass;
    const bool steep = normalY < 0.72f; // more than about 44 degrees
    if (y < kPitY + 1.0f || (steep && y < kPlainY - 1.0f && InPit(x, z)))
    {
        c = p.pit;
    }
    else if (y < kSeaY - 0.3f)
    {
        c = p.bed;
    }
    else if (y < kSeaY + 1.5f)
    {
        c = p.sand;
    }
    else if (steep)
    {
        c = p.rock;
    }
    else if (y > 38.0f)
    {
        c = p.high;
    }
    else if (TownMask(x, z) > 0.6f)
    {
        c = p.town;
    }
    else if (terrain::Fbm(x / 30.0f, z / 30.0f, 2, 21u) > 0.12f)
    {
        c = p.dry;
    }
    out[0] = batch_detail::SrgbToLinear(c.r);
    out[1] = batch_detail::SrgbToLinear(c.g);
    out[2] = batch_detail::SrgbToLinear(c.b);
    out[3] = 0.0f; // its own colour: not the stone of the town
}

/// The ground of `layer` as flat-shaded triangles.
inline BatchMesh BakeGround(const terrain::Land& land, int layer)
{
    BatchMesh mesh;
    const auto bake = [&](const terrain::Field& f, bool coarse) {
        for (int j = 0; j < f.nz; ++j)
        {
            for (int i = 0; i < f.nx; ++i)
            {
                if (coarse && land.FarCellHidden(i, j))
                {
                    continue;
                }
                for (int t = 0; t < 2; ++t)
                {
                    Vec3 a;
                    Vec3 b;
                    Vec3 c;
                    f.Triangle(i, j, t, a, b, c);
                    const Vec3 n = terrain::Cross(terrain::Sub(b, a), terrain::Sub(c, a));
                    const float len = std::sqrt(terrain::Dot(n, n));
                    float colour[4];
                    GroundColour((a.x + b.x + c.x) / 3.0f, (a.y + b.y + c.y) / 3.0f, (a.z + b.z + c.z) / 3.0f, len > 0.0f ? n.y / len : 1.0f, layer, colour);
                    const float pa[3] = {a.x, a.y, a.z};
                    const float pb[3] = {b.x, b.y, b.z};
                    const float pc[3] = {c.x, c.y, c.z};
                    AddFlatTriangle(mesh, pa, pb, pc, colour);
                }
            }
        }
    };
    bake(land.nearField, false);
    bake(land.farField, true);
    return mesh;
}

// ---- plants (docs/LEVEL_DESIGN.md, 5.6)

enum class PlantKind
{
    Cypress,
    Olive,
    Pine,
    Oak,
    Shrub,
    Lemon,
};

struct Plant
{
    Vec3 base;      ///< the foot of the trunk, on the ground
    PlantKind kind;
    float size;     ///< 0.6 .. 1.4 of its kind
    float yaw;      ///< the turn of its crown
};

/// UNITS: metres: the least distance between two plants of this kind (the scatter keeps it).
constexpr float PlantSpacing(PlantKind k)
{
    switch (k)
    {
    case PlantKind::Cypress:
        return 3.5f;
    case PlantKind::Olive:
        return 4.5f;
    case PlantKind::Pine:
        return 6.0f;
    case PlantKind::Oak:
        return 5.0f;
    case PlantKind::Shrub:
        return 1.6f;
    case PlantKind::Lemon:
        return 2.5f;
    }
    return 3.0f;
}

inline constexpr float kPlantFromRoom = 4.5f;     ///< UNITS: metres: no plant this close to a room (a jump never lands in one)
inline constexpr float kPlantRegionX0 = -200.0f;  ///< the wild plants are placed in this area (farther, the fog hides them)
inline constexpr float kPlantRegionX1 = 220.0f;
inline constexpr float kPlantRegionZ0 = -110.0f;
inline constexpr float kPlantRegionZ1 = 240.0f;

/// The class of the ground at (x, z) and the plant that grows there, or false when nothing grows there. `roll` is 0 .. 1.
inline bool WildPlantAt(const terrain::Land& land, float x, float z, float roll, PlantKind& kind)
{
    using terrain::Fbm;
    const float y = land.Height(x, z);
    if (y < kSeaY + 1.5f || TownMask(x, z) > 0.35f)
    {
        return false; // the shore and the town: nothing wild
    }
    const Vec3 n = land.Normal(x, z);
    const float slope = std::sqrt(n.x * n.x + n.z * n.z) / (n.y > 1.0e-4f ? n.y : 1.0e-4f);
    if (slope > 0.95f)
    {
        return false; // rock
    }
    const float clumps = Fbm(x / 40.0f, z / 40.0f, 2, 41u); // woods and clearings
    const float gx = -n.x / n.y; // the rise per metre toward +x and +z
    const float gz = -n.z / n.y;
    const bool sunny = gz > 0.02f || gx < -0.04f; // the slope faces south or east
    const bool windy = y > 14.0f || z < CoastZ(x) + 30.0f;
    if (y > 40.0f) // high on the mountain: pines thinning to bare stone
    {
        kind = PlantKind::Pine;
        return roll < 0.5f * (1.0f - (y - 40.0f) / 30.0f) && clumps > -0.1f;
    }
    if (slope > 0.42f) // steep: low shrubs in drifts
    {
        kind = PlantKind::Shrub;
        return roll < 0.35f && clumps > -0.2f;
    }
    if (windy) // the heights and the coast: umbrella pines in clumps
    {
        kind = PlantKind::Pine;
        return roll < 0.6f && clumps > 0.1f;
    }
    if (sunny && slope > 0.04f) // a sunny slope: an olive grove, in rows along the contour
    {
        const float band = std::fmod(y + 40.0f, 3.0f);
        kind = PlantKind::Olive;
        return band < 0.5f && roll < 0.85f;
    }
    kind = PlantKind::Oak; // shaded or flat: holm oaks, in woods with clearings
    return roll < 0.45f && clumps > 0.05f;
}

/// Every plant of the world: the wild ones (scattered, one distance kept between any two), the cypresses along the edge of the
/// town (a line along the road round it) and the trees of the gardens.
inline std::vector<Plant> PlacePlants(const terrain::Land& land)
{
    std::vector<Plant> plants;
    // a grid of buckets for the distance test
    const float bucket = 6.0f;
    const int bx = static_cast<int>((kPlantRegionX1 - kPlantRegionX0) / bucket) + 1;
    const int bz = static_cast<int>((kPlantRegionZ1 - kPlantRegionZ0) / bucket) + 1;
    std::vector<std::vector<int>> buckets(static_cast<std::size_t>(bx) * static_cast<std::size_t>(bz));
    const auto bucketOf = [&](float x, float z) {
        int i = static_cast<int>((x - kPlantRegionX0) / bucket);
        int j = static_cast<int>((z - kPlantRegionZ0) / bucket);
        i = i < 0 ? 0 : (i >= bx ? bx - 1 : i);
        j = j < 0 ? 0 : (j >= bz ? bz - 1 : j);
        return static_cast<std::size_t>(j) * static_cast<std::size_t>(bx) + static_cast<std::size_t>(i);
    };
    const auto farFromOthers = [&](float x, float z, PlantKind k) {
        const int i0 = static_cast<int>((x - kPlantRegionX0) / bucket);
        const int j0 = static_cast<int>((z - kPlantRegionZ0) / bucket);
        for (int j = j0 - 1; j <= j0 + 1; ++j)
        {
            for (int i = i0 - 1; i <= i0 + 1; ++i)
            {
                if (i < 0 || j < 0 || i >= bx || j >= bz)
                {
                    continue;
                }
                for (int idx : buckets[static_cast<std::size_t>(j) * static_cast<std::size_t>(bx) + static_cast<std::size_t>(i)])
                {
                    const Plant& o = plants[static_cast<std::size_t>(idx)];
                    const float need = 0.5f * (PlantSpacing(k) + PlantSpacing(o.kind));
                    const float dx = o.base.x - x;
                    const float dz = o.base.z - z;
                    if (dx * dx + dz * dz < need * need)
                    {
                        return false;
                    }
                }
            }
        }
        return true;
    };
    const auto clearOfMap = [&](float x, float z, float margin) {
        for (const Room& r : kRooms)
        {
            if (FootprintGap(x, z, x, z, r.x0, r.z0, r.x1, r.z1) < margin)
            {
                return false;
            }
        }
        for (const Pit& p : kPits)
        {
            if (FootprintGap(x, z, x, z, p.x0, p.z0, p.x1, p.z1) < kPitReach + 1.0f)
            {
                return false;
            }
        }
        return true;
    };
    unsigned state = 1907u;
    const auto next = [&state]() {
        state = state * 1664525u + 1013904223u;
        return static_cast<float>((state >> 8) & 0xFFFFu) / 65535.0f;
    };
    const auto add = [&](float x, float z, PlantKind k, float size) {
        Plant p{{x, land.Height(x, z), z}, k, size, 6.2831853f * next()};
        buckets[bucketOf(x, z)].push_back(static_cast<int>(plants.size()));
        plants.push_back(p);
    };
    // the gardens of the town: a lemon tree or two and a cypress in each empty plot
    for (const Plot& plot : SkylinePlots())
    {
        if (plot.kind != PlotKind::Garden)
        {
            continue;
        }
        const float cx = 0.5f * (plot.x0 + plot.x1);
        const float cz = 0.5f * (plot.z0 + plot.z1);
        const float offsets[3][2] = {{-1.4f, -1.2f}, {1.5f, 1.3f}, {1.4f, -1.5f}};
        for (int k = 0; k < 3; ++k)
        {
            const float x = cx + offsets[k][0];
            const float z = cz + offsets[k][1];
            const PlantKind kind = k == 1 ? PlantKind::Cypress : PlantKind::Lemon;
            if (clearOfMap(x, z, kPlantFromRoom) && farFromOthers(x, z, kind))
            {
                add(x, z, kind, 0.8f + 0.4f * next());
            }
        }
    }
    // the cypresses along the road round the town: where the town meets the land
    for (float t = 0.0f; t < 6.2831853f; t += 0.035f)
    {
        const float x = 78.0f * std::sin(t);
        const float z = 30.0f + 88.0f * std::cos(t);
        const float m = TownMask(x, z);
        if (m > 0.2f && m < 0.8f && land.Height(x, z) > kSeaY + 1.5f && land.Slope(x, z) < 0.4f && clearOfMap(x, z, kPlantFromRoom) &&
            farFromOthers(x, z, PlantKind::Cypress))
        {
            add(x, z, PlantKind::Cypress, 0.9f + 0.3f * next());
        }
    }
    // the wild ones: a jittered grid of candidates, each kept by the rule of its ground and the distance to the others
    const float step = 2.6f;
    for (float z = kPlantRegionZ0; z < kPlantRegionZ1; z += step)
    {
        for (float x = kPlantRegionX0; x < kPlantRegionX1; x += step)
        {
            const float px = x + step * next();
            const float pz = z + step * next();
            const float roll = next();
            const float size = 0.6f + 0.8f * next();
            PlantKind kind = PlantKind::Oak;
            if (!WildPlantAt(land, px, pz, roll, kind) || !clearOfMap(px, pz, kPlantFromRoom) || !farFromOthers(px, pz, kind))
            {
                continue;
            }
            const bool windy = land.Height(px, pz) > 14.0f || pz < CoastZ(px) + 30.0f;
            add(px, pz, kind, windy ? 0.6f + 0.5f * (size - 0.6f) : size); // smaller where the wind is
        }
    }
    return plants;
}

/// The boxes of one plant, for both worlds (each box once per world, in its colour there). Drawn only.
inline void PlantBoxes(const Plant& p, std::vector<SceneObject>& out)
{
    struct Look
    {
        Rgb a, b;
    };
    const Rgb trunkA{0.36f, 0.27f, 0.19f};
    const Rgb trunkB{0.27f, 0.24f, 0.21f};
    const auto part = [&](float w, float y0, float h, float d, const Look& look, float yaw, float tilt, bool inB) {
        const float x = p.base.x;
        const float z = p.base.z;
        SceneObject a = Span(x - 0.5f * w, p.base.y + y0, z - 0.5f * d, x + 0.5f * w, p.base.y + y0 + h, z + 0.5f * d, look.a, kLayerA);
        a.yaw = yaw;
        a.tilt = tilt;
        a.shineStrength = 0.05f;
        out.push_back(a);
        if (inB)
        {
            SceneObject b = a;
            b.colorSrgb[0] = look.b.r;
            b.colorSrgb[1] = look.b.g;
            b.colorSrgb[2] = look.b.b;
            b.layerMask = kLayerB;
            out.push_back(b);
        }
    };
    const float s = p.size;
    const Look trunk{trunkA, trunkB};
    switch (p.kind)
    {
    case PlantKind::Cypress:
        part(0.3f, -0.2f, 1.2f * s, 0.3f, trunk, 0.0f, 0.0f, true);
        part(1.1f * s, 0.8f * s, 5.5f * s, 1.1f * s, {{0.18f, 0.30f, 0.20f}, {0.16f, 0.22f, 0.19f}}, p.yaw, 0.0f, true);
        break;
    case PlantKind::Olive:
        part(0.35f, -0.2f, 1.3f * s, 0.35f, trunk, 0.0f, 0.0f, true);
        part(2.4f * s, 1.0f * s, 1.3f * s, 2.4f * s, {{0.55f, 0.60f, 0.42f}, {0.36f, 0.40f, 0.32f}}, p.yaw, 0.0f, true);
        break;
    case PlantKind::Pine:
        part(0.35f, -0.2f, 7.0f * s, 0.35f, trunk, 0.0f, 0.05f, true); // leaning away from the sea
        part(4.0f * s, 6.4f * s, 1.2f * s, 4.0f * s, {{0.22f, 0.36f, 0.24f}, {0.18f, 0.25f, 0.21f}}, p.yaw, 0.0f, true);
        break;
    case PlantKind::Oak:
        part(0.4f, -0.2f, 1.8f * s, 0.4f, trunk, 0.0f, 0.0f, true);
        part(3.4f * s, 1.5f * s, 2.6f * s, 3.4f * s, {{0.26f, 0.40f, 0.22f}, {0.20f, 0.28f, 0.20f}}, p.yaw, 0.0f, true);
        break;
    case PlantKind::Shrub:
        part(1.2f * s, -0.2f, 0.9f * s, 1.2f * s, {{0.42f, 0.48f, 0.34f}, {0.30f, 0.34f, 0.28f}}, p.yaw, 0.0f, true);
        break;
    case PlantKind::Lemon:
        part(0.2f, -0.1f, 1.0f * s, 0.2f, trunk, 0.0f, 0.0f, true);
        part(1.4f * s, 0.8f * s, 1.3f * s, 1.4f * s, {{0.26f, 0.45f, 0.22f}, {0.26f, 0.45f, 0.22f}}, p.yaw, 0.0f, false); // dead in B
        break;
    }
}

/// The ground, the plants and the scenery of `layer`, as one mesh for the static batch.
inline BatchMesh BakeWorld(int layer)
{
    BatchMesh mesh = BakeGround(TheLand(), layer);
    std::vector<SceneObject> boxes;
    for (const Plant& p : PlacePlants(TheLand()))
    {
        PlantBoxes(p, boxes);
    }
    Scenery(boxes); // the harbour, the town wall, the bell tower, the doors and windows (scene/Scenery.h)
    AppendMesh(mesh, BakeLayer(boxes.data(), static_cast<int>(boxes.size()), layer));
    return mesh;
}

} // namespace dl::stage
