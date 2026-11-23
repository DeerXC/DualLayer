// test_terrain.cpp - tests of the ground of the world (no window, no GPU): src/scene/Terrain.h, src/scene/Landscape.h, the
// footings and the town of src/scene/Stage.h, with the real walking body (src/scene/Player.h).
//   g++ -std=c++20 -O2 -Wall -Wextra -I src tests/test_terrain.cpp src/render/MeshData.cpp -o test_terrain
//   MSVC (Developer PowerShell):  .\tests\run_tests.ps1 -Only terrain
// What is checked:
//   1. noise: the same numbers every time, in range
//   2. a field: exact at its vertices, continuous across every edge and diagonal, every triangle clockwise seen from above
//   3. the land: the fine field meets the coarse one without a crack; the hole of the coarse field is exactly the fine field
//   4. collision: a ball is pushed out to its radius from flat ground, from a slope, and from under the surface; the walking
//      body stands and walks up a gentle slope and slides off one too steep to stand on
//   5. under the map: at every point of every room, in both worlds, the ground is under the highest floor there: nothing of the
//      ground shows in a room (where a room has no floor, the falls are proved by 6)
//   6. no escape: from every place to stand near the edge of its box, a walk off it and a running jump off it, outward, never
//      end standing on the ground of the world (the body falls below the fall line of its room, or lands in the map)
//   7. immersion: the land reaches the far plane from every room in the north, east and west, and is under the sea at its south
//      edge; from every room entry, lamp and the balcony, rays looking down in 16 directions meet the map, the town, the land or
//      the sea within the far plane; the sea is wider than the far plane round any eye
//   8. the town: every house and tower stands on the levelled ground (its corners on kPlainY), none over a pit
//   9. the plants: each one on the ground, out of the water, off rock, away from every room and pit, apart from the others by
//      the distance of their kinds; the wild ones outside the town; the trees of the gardens inside their plots
//  10. the baked ground: one triangle pair per drawn cell, every vertex on the land, every normal up, flat-shaded
//  11. footings: none holds a place where a body can stand
//  12. the scenery (scene/Scenery.h): nothing near a room within reach
// Returns 1 when a check fails.

#include "scene/CityParts.h"
#include "scene/CityState.h"
#include "scene/Landscape.h"
#include "scene/Player.h"
#include "scene/Puzzle.h"
#include "scene/Scenery.h"
#include "scene/Stage.h"

#include <cmath>
#include <cstdio>
#include <vector>

namespace
{

int g_failed = 0;
int g_passed = 0;

#define CHECK(cond)                                                                                                    \
    do                                                                                                                 \
    {                                                                                                                  \
        if (cond)                                                                                                      \
        {                                                                                                              \
            ++g_passed;                                                                                                \
        }                                                                                                              \
        else                                                                                                           \
        {                                                                                                              \
            ++g_failed;                                                                                                \
            std::printf("FAIL line %d: %s\n", __LINE__, #cond);                                                        \
        }                                                                                                              \
    } while (0)

using dl::Player;
using dl::Vec3;
using namespace dl::scene;
namespace st = dl::stage;
namespace tr = dl::terrain;

constexpr float kDt = 1.0f / 90.0f;

bool Near(float a, float b, float eps)
{
    return std::fabs(a - b) <= eps;
}

void TestNoise()
{
    bool same = true;
    bool inRange = true;
    for (int i = 0; i < 500; ++i)
    {
        const float x = -50.0f + 0.37f * static_cast<float>(i);
        const float z = 13.0f - 0.21f * static_cast<float>(i);
        const float a = tr::Fbm(x, z, 4, 3u);
        same = same && a == tr::Fbm(x, z, 4, 3u);
        const float r = tr::Ridged(x, z, 4, 11u);
        inRange = inRange && a >= -1.0f && a <= 1.0f && r >= 0.0f && r <= 1.0f && tr::ValueNoise(x, z, 5u) >= -1.0f && tr::ValueNoise(x, z, 5u) <= 1.0f;
    }
    CHECK(same && inRange);
    CHECK(tr::SmoothStep(0.0f, 1.0f, -1.0f) == 0.0f && tr::SmoothStep(0.0f, 1.0f, 2.0f) == 1.0f && Near(tr::SmoothStep(1.0f, 0.0f, 0.25f), 0.84375f, 1e-5f));
}

tr::Field SmallField()
{
    tr::Field f;
    f.Build(-2.0f, -3.0f, 1.5f, 4, 3, [](float x, float z) { return 0.3f * x - 0.2f * z + 0.1f * x * z; });
    return f;
}

void TestField()
{
    const tr::Field f = SmallField();
    bool exact = true;
    for (int j = 0; j <= f.nz; ++j)
    {
        for (int i = 0; i <= f.nx; ++i)
        {
            exact = exact && Near(f.Height(f.VertexX(i), f.VertexZ(j)), f.At(i, j), 1e-5f);
        }
    }
    CHECK(exact);
    bool continuous = true; // across every edge and every diagonal: both sides agree
    const float e = 1.0e-4f;
    for (float x = f.x0 + 0.05f; x < f.X1(); x += 0.113f)
    {
        for (float z = f.z0 + 0.05f; z < f.Z1(); z += 0.097f)
        {
            continuous = continuous && std::fabs(f.Height(x + e, z) - f.Height(x - e, z)) < 0.01f && std::fabs(f.Height(x, z + e) - f.Height(x, z - e)) < 0.01f;
        }
    }
    CHECK(continuous);
    bool up = true;
    for (int j = 0; j < f.nz; ++j)
    {
        for (int i = 0; i < f.nx; ++i)
        {
            for (int t = 0; t < 2; ++t)
            {
                Vec3 a;
                Vec3 b;
                Vec3 c;
                f.Triangle(i, j, t, a, b, c);
                const Vec3 n = tr::Cross(tr::Sub(b, a), tr::Sub(c, a));
                up = up && n.y > 0.0f;
                // the middle of the triangle lies on the surface that Height reports
                up = up && Near(f.Height((a.x + b.x + c.x) / 3.0f, (a.z + b.z + c.z) / 3.0f), (a.y + b.y + c.y) / 3.0f, 1e-4f);
            }
        }
    }
    CHECK(up);
}

void TestLand()
{
    const tr::Land& land = st::TheLand();
    float worst = 0.0f; // the edge of the fine field against the coarse one
    for (float x = st::kNearX0; x <= st::kNearX1; x += 0.29f)
    {
        for (float z : {st::kNearZ0, st::kNearZ1})
        {
            const float d = std::fabs(land.nearField.Height(x, z) - land.farField.Height(x, z));
            worst = d > worst ? d : worst;
        }
    }
    for (float z = st::kNearZ0; z <= st::kNearZ1; z += 0.29f)
    {
        for (float x : {st::kNearX0, st::kNearX1})
        {
            const float d = std::fabs(land.nearField.Height(x, z) - land.farField.Height(x, z));
            worst = d > worst ? d : worst;
        }
    }
    std::printf("  seam between the fields: at most %.6f m\n", static_cast<double>(worst));
    CHECK(worst < 1.0e-3f);
    bool hole = true; // a coarse cell is hidden exactly when it lies in the fine field
    for (int j = 0; j < land.farField.nz; ++j)
    {
        for (int i = 0; i < land.farField.nx; ++i)
        {
            const float x0 = land.farField.VertexX(i);
            const float z0 = land.farField.VertexZ(j);
            const bool inside = x0 >= st::kNearX0 && x0 + st::kFarCell <= st::kNearX1 && z0 >= st::kNearZ0 && z0 + st::kFarCell <= st::kNearZ1;
            hole = hole && land.FarCellHidden(i, j) == inside;
        }
    }
    CHECK(hole);
    // inside the town and away from the pits the ground is level, just under the street floors
    CHECK(Near(land.Height(-40.0f, 60.0f), st::kPlainY, 1e-4f) && Near(land.Height(40.0f, -20.0f), st::kPlainY, 1e-4f));
    CHECK(land.Height(0.0f, -100.0f) < st::kSeaY && land.Height(0.0f, 250.0f) > 30.0f); // the sea, the mountain
}

/// A land of one field only, made by `height`.
template <class F>
tr::Land TestLandOf(F height)
{
    tr::Land l;
    l.nearField.Build(-20.0f, -20.0f, 1.0f, 40, 40, height);
    return l;
}

void TestCollision()
{
    const tr::Land flat = TestLandOf([](float, float) { return 0.0f; });
    Vec3 c{1.3f, 0.2f, -2.1f};
    CHECK(flat.PushOut(c, 0.35f) > 0.1f && Near(c.y, 0.35f, 1e-4f) && Near(c.x, 1.3f, 1e-4f));
    Vec3 under{0.4f, -1.0f, 0.4f}; // a centre under the surface comes up onto it, then out to its radius
    flat.PushOut(under, 0.35f);
    CHECK(Near(under.y, 0.35f, 1e-3f));
    Vec3 clear{0.0f, 0.5f, 0.0f};
    CHECK(flat.PushOut(clear, 0.35f) == 0.0f);
    const float k = std::tan(30.0f * 3.14159265f / 180.0f); // a slope of 30 degrees rising toward +x
    const tr::Land slope = TestLandOf([k](float x, float) { return k * x; });
    Vec3 s{2.0f, 2.0f * k + 0.1f, 0.5f};
    slope.PushOut(s, 0.35f);
    const float distance = (s.y - k * s.x) * std::cos(30.0f * 3.14159265f / 180.0f); // to the plane y = k x
    CHECK(Near(distance, 0.35f, 2e-3f));
    // the walking body: it stands on a gentle slope and walks up it; on a slope of 60 degrees it does not stand
    static SceneObject nothing[1] = {};
    nothing[0].shape = ShapeKind::Cube;
    nothing[0].position[1] = -500.0f;
    nothing[0].scale[0] = nothing[0].scale[1] = nothing[0].scale[2] = 1.0f;
    nothing[0].layerMask = kLayerBoth;
    const float g = std::tan(15.0f * 3.14159265f / 180.0f);
    const tr::Land gentle = TestLandOf([g](float, float z) { return g * z; });
    for (int layer = 0; layer < 2; ++layer)
    {
        Player p;
        p.SetScene(nothing, 1);
        p.SetLand(&gentle);
        p.Place({0.0f, 1.0f + p.GetSettings().eyeHeight, -10.0f});
        for (int i = 0; i < 60; ++i)
        {
            p.Update({}, {0.0f, 0.0f, 1.0f}, layer, 0.0f, kDt);
        }
        CHECK(p.Grounded() && Near(p.Feet().y, gentle.Height(0.0f, -10.0f), 0.05f));
        Player::Input walk;
        walk.moveForward = 1.0f;
        for (int i = 0; i < 270; ++i)
        {
            p.Update(walk, {0.0f, 0.0f, 1.0f}, layer, 0.0f, kDt);
        }
        CHECK(p.Grounded() && p.Feet().z > 2.0f && Near(p.Feet().y, gentle.Height(p.Feet().x, p.Feet().z), 0.05f));
    }
    const float t = std::tan(60.0f * 3.14159265f / 180.0f);
    const tr::Land steep = TestLandOf([t](float x, float) { return t * x; });
    Player p;
    p.SetScene(nothing, 1);
    p.SetLand(&steep);
    p.Place({2.0f, 2.0f * t + 0.3f + p.GetSettings().eyeHeight, 0.0f});
    int grounded = 0;
    for (int i = 0; i < 40; ++i)
    {
        p.Update({}, {0.0f, 0.0f, 1.0f}, 0, 0.0f, kDt);
        grounded += p.Grounded() ? 1 : 0;
    }
    CHECK(grounded == 0 && p.Feet().x < 1.5f); // it slid down
}

/// The boxes a body meets: the fixed map, the gates that can be there (bridges laid, the portcullis moved) and the fragments.
const std::vector<SceneObject>& MapSolids()
{
    static const std::vector<SceneObject> all = [] {
        std::vector<SceneObject> v(st::kStageStatics.begin(), st::kStageStatics.end());
        for (const dl::mech::Gate& g : st::kCityGates)
        {
            if (g.presentWhenOpen)
            {
                v.push_back(g.box);
            }
        }
        for (int f = 0; f < st::kFragmentCount; ++f)
        {
            for (const SceneObject& o : st::Fragments().solid[f])
            {
                v.push_back(o);
            }
        }
        return v;
    }();
    return all;
}

/// True when a ball at (x, top, z) starts inside something solid of `layer` (the point is in a wall).
bool InsideWall(float x, float z, float top, int layer)
{
    const Vec3 c{x, top + 0.3f, z};
    const dl::CollisionResult hit = dl::ResolveSphere(c, 0.05f, layer, 0.0f, st::kStageStatics.data(), st::kStageStaticCount);
    return hit.pushed;
}

/// The height of the highest floor at (x, z) between `top` and `bottom` in `layer` (statics only), or false.
bool FloorAt(float x, float z, float top, float bottom, int layer, float& feet)
{
    const float r = 0.3f;
    for (float y = top; y >= bottom; y -= 0.05f)
    {
        const Vec3 c{x, y + r, z};
        const dl::CollisionResult hit = dl::ResolveSphere(c, r, layer, 0.0f, st::kStageStatics.data(), st::kStageStaticCount);
        if (hit.pushed && hit.position.y - c.y > 0.0f && hit.position.y - c.y > 0.6f * std::sqrt((hit.position.x - c.x) * (hit.position.x - c.x) +
                                                                                                    (hit.position.y - c.y) * (hit.position.y - c.y) +
                                                                                                    (hit.position.z - c.z) * (hit.position.z - c.z)))
        {
            feet = hit.position.y - r;
            return true;
        }
    }
    return false;
}

void TestUnderMap()
{
    const tr::Land& land = st::TheLand();
    int points = 0;
    int open = 0;
    int bad = 0;
    for (int i = 0; i < st::kRoomCount; ++i)
    {
        const st::Room& room = st::kRooms[i];
        const float fall = st::FallLine(i);
        for (float x = room.x0 + 0.1f; x <= room.x1 - 0.1f; x += 0.75f)
        {
            for (float z = room.z0 + 0.1f; z <= room.z1 - 0.1f; z += 0.75f)
            {
                const float ground = land.Height(x, z);
                for (int layer = 0; layer < 2; ++layer)
                {
                    if (InsideWall(x, z, room.y1 + 0.5f, layer))
                    {
                        continue; // in a wall that stands in the room: not a place
                    }
                    ++points;
                    float feet = 0.0f;
                    if (FloorAt(x, z, room.y1 + 0.5f, fall, layer, feet))
                    {
                        if (ground > feet - 0.02f)
                        {
                            if (bad++ < 5)
                            {
                                std::printf("  the ground (%.2f) is not under the floor (%.2f) at %.2f %.2f in %s, layer %c\n", static_cast<double>(ground),
                                            static_cast<double>(feet), static_cast<double>(x), static_cast<double>(z), room.name, layer == 0 ? 'A' : 'B');
                            }
                        }
                    }
                    else
                    {
                        ++open; // no floor (a gap, a trench, a closed space between walls): the falls are proved by TestNoEscape
                    }
                }
            }
        }
    }
    std::printf("  under the map: %d points, %d with no floor, %d wrong\n", points, open, bad);
    CHECK(points > 5000 && open > 50 && bad == 0);
}

struct Place
{
    Vec3 feet;
    int layer;
    int room;
    Vec3 out; ///< the way off the nearest edge of its box
};

/// The places to stand near the edge of the top of a box of the map, in each layer of the box.
std::vector<Place> EdgePlaces()
{
    std::vector<Place> places;
    for (const SceneObject& o : MapSolids())
    {
        if (o.shape != ShapeKind::Cube || o.tilt != 0.0f || o.yaw != 0.0f)
        {
            continue;
        }
        const float hx = 0.5f * o.scale[0];
        const float hz = 0.5f * o.scale[2];
        const float top = o.position[1] + 0.5f * o.scale[1];
        for (int layer = 0; layer < 2; ++layer)
        {
            if ((o.layerMask & (1u << layer)) == 0u)
            {
                continue;
            }
            for (float lx = -hx + 0.25f; lx <= hx - 0.25f + 1e-3f; lx += 1.0f)
            {
                for (float lz = -hz + 0.25f; lz <= hz - 0.25f + 1e-3f; lz += 1.0f)
                {
                    const float dxMin = hx - std::fabs(lx);
                    const float dzMin = hz - std::fabs(lz);
                    if (dxMin > 0.7f && dzMin > 0.7f)
                    {
                        continue; // not near an edge
                    }
                    const Vec3 f{o.position[0] + lx, top, o.position[2] + lz};
                    const int room = st::RoomAt(f);
                    if (room < 0)
                    {
                        continue; // a wall top: out of reach (tests/test_reach.cpp)
                    }
                    const Vec3 out = dxMin <= dzMin ? Vec3{lx > 0.0f ? 1.0f : -1.0f, 0.0f, 0.0f} : Vec3{0.0f, 0.0f, lz > 0.0f ? 1.0f : -1.0f};
                    places.push_back({f, layer, room, out});
                }
            }
        }
    }
    return places;
}

std::vector<SceneObject> NearSolids(float x, float z)
{
    std::vector<SceneObject> near;
    for (const SceneObject& o : MapSolids())
    {
        float lo[3];
        float hi[3];
        st::AxisBounds(o, lo, hi);
        const float dx = x < lo[0] ? lo[0] - x : (x > hi[0] ? x - hi[0] : 0.0f);
        const float dz = z < lo[2] ? lo[2] - z : (z > hi[2] ? z - hi[2] : 0.0f);
        if (dx * dx + dz * dz < 16.0f * 16.0f) // a drop of 6 m with a run-up ends up to about 11 m away
        {
            near.push_back(o);
        }
    }
    return near;
}

/// True when a body with its feet at `f` stands on a box of `solids` (not on the ground of the world).
bool OnBox(const Vec3& f, int layer, const std::vector<SceneObject>& solids)
{
    const Vec3 c{f.x, f.y + 0.35f - 0.04f, f.z};
    const dl::CollisionResult hit = dl::ResolveSphere(c, 0.35f, layer, 0.0f, solids.data(), static_cast<int>(solids.size()));
    return hit.pushed && hit.position.y > c.y;
}

/// True when feet at `f` are in a room or right at its edge (a threshold between two rooms, where the ground lies under a
/// doorway): the body is still in the map.
bool NearARoom(const Vec3& f)
{
    for (const st::Room& r : st::kRooms)
    {
        if (f.x > r.x0 - 1.0f && f.x < r.x1 + 1.0f && f.z > r.z0 - 1.0f && f.z < r.z1 + 1.0f && f.y > r.y0 - 0.5f && f.y < r.y1 + 0.5f)
        {
            return true;
        }
    }
    return false;
}

void TestNoEscape()
{
    const tr::Land& land = st::TheLand();
    const std::vector<Place> places = EdgePlaces();
    int tried = 0;
    int escaped = 0;
    for (const Place& pl : places)
    {
        const std::vector<SceneObject> solids = NearSolids(pl.feet.x, pl.feet.z);
        const Vec3 dirs[3] = {pl.out, {pl.out.x * 0.7071f - pl.out.z * 0.7071f, 0.0f, pl.out.z * 0.7071f + pl.out.x * 0.7071f},
                              {pl.out.x * 0.7071f + pl.out.z * 0.7071f, 0.0f, pl.out.z * 0.7071f - pl.out.x * 0.7071f}};
        for (const Vec3& d : dirs)
        {
            for (int jump = 0; jump < 2; ++jump)
            {
                Player p;
                p.SetScene(solids.data(), static_cast<int>(solids.size()));
                p.SetLand(&land);
                p.Place({pl.feet.x, pl.feet.y + p.GetSettings().eyeHeight + 0.02f, pl.feet.z});
                for (int i = 0; i < 4; ++i)
                {
                    p.Update({}, d, pl.layer, 0.0f, kDt);
                }
                const Vec3 settled = p.Feet();
                const float moved = std::fabs(settled.x - pl.feet.x) + std::fabs(settled.y - pl.feet.y) + std::fabs(settled.z - pl.feet.z);
                if (!p.Grounded() || moved > 0.1f || !dl::puzzle::Puzzle::FitsIn(p, pl.layer, 0.0f, solids.data(), static_cast<int>(solids.size())))
                {
                    continue; // not a place to stand after all (inside another box, or a body would not fit there)
                }
                ++tried;
                Player::Input in;
                in.moveForward = 1.0f;
                bool jumped = false;
                const float fall = st::FallLine(pl.room);
                for (int i = 0; i < 300; ++i)
                {
                    in.jumpPressed = jump == 1 && !jumped && i == 2;
                    jumped = jumped || in.jumpPressed;
                    p.Update(in, d, pl.layer, 0.0f, kDt);
                    const Vec3 f = p.Feet();
                    if (f.y < fall)
                    {
                        break; // a fall: the game brings the body back
                    }
                    if ((f.x - pl.feet.x) * (f.x - pl.feet.x) + (f.z - pl.feet.z) * (f.z - pl.feet.z) > 100.0f)
                    {
                        break; // it walks on along a floor of the map: no edge here (and the solids given end 16 m away)
                    }
                    if (p.Grounded() && f.y - land.Height(f.x, f.z) < 0.15f && !OnBox(f, pl.layer, solids) && !NearARoom(f))
                    {
                        if (escaped++ < 40)
                        {
                            std::printf("  ESCAPE from %s (%.2f %.2f %.2f) layer %c, dir (%.2f %.2f)%s -> on the ground at %.2f %.2f %.2f\n",
                                        st::kRooms[pl.room].name, static_cast<double>(pl.feet.x), static_cast<double>(pl.feet.y),
                                        static_cast<double>(pl.feet.z), pl.layer == 0 ? 'A' : 'B', static_cast<double>(d.x), static_cast<double>(d.z),
                                        jump == 1 ? ", jump" : "", static_cast<double>(f.x), static_cast<double>(f.y), static_cast<double>(f.z));
                        }
                        break;
                    }
                }
            }
        }
    }
    std::printf("  no escape: %zu places at an edge, %d walks and jumps off them, %d onto the ground\n", places.size(), tried, escaped);
    CHECK(tried > 1000 && escaped == 0);
}

/// True when the segment p .. q meets the box `o` (its yaw taken into account; no tilt).
bool SegmentHitsBox(const Vec3& p, const Vec3& q, const SceneObject& o)
{
    // into the frame of the box: move to its centre, turn back by its yaw
    const float c = std::cos(-o.yaw);
    const float s = std::sin(-o.yaw);
    const auto local = [&](const Vec3& v) {
        const float dx = v.x - o.position[0];
        const float dz = v.z - o.position[2];
        return Vec3{dx * c + dz * s, v.y - o.position[1], -dx * s + dz * c};
    };
    const Vec3 a = local(p);
    const Vec3 b = local(q);
    const float pa[3] = {a.x, a.y, a.z};
    const float pb[3] = {b.x, b.y, b.z};
    float t0 = 0.0f;
    float t1 = 1.0f;
    for (int k = 0; k < 3; ++k)
    {
        const float h = 0.5f * o.scale[k];
        const float d = pb[k] - pa[k];
        if (std::fabs(d) < 1e-9f)
        {
            if (pa[k] < -h || pa[k] > h)
            {
                return false;
            }
            continue;
        }
        float ta = (-h - pa[k]) / d;
        float tb = (h - pa[k]) / d;
        if (ta > tb)
        {
            const float t = ta;
            ta = tb;
            tb = t;
        }
        t0 = ta > t0 ? ta : t0;
        t1 = tb < t1 ? tb : t1;
        if (t0 > t1)
        {
            return false;
        }
    }
    return true;
}

void TestImmersion()
{
    const tr::Land& land = st::TheLand();
    // the land reaches past the far plane from every room, north, east and west; its south edge lies under the sea
    for (const st::Room& r : st::kRooms)
    {
        CHECK(st::kFarX0 <= r.x0 - st::kViewDistance && st::kFarX1 >= r.x1 + st::kViewDistance && st::kFarZ1 >= r.z1 + st::kViewDistance);
    }
    bool southWet = true;
    for (float x = st::kFarX0; x <= st::kFarX1; x += 4.0f)
    {
        southWet = southWet && land.Height(x, st::kFarZ0) < st::kSeaY - 1.0f;
    }
    CHECK(southWet);
    CHECK(st::kSeaHalf - 0.5f * st::kSeaSnap >= st::kViewDistance + 30.0f); // the sea round any eye reaches past the far plane
    SceneObject sea[2] = {};
    CHECK(st::AddSea({123.0f, 4.0f, -77.0f}, sea, 0, 2) == 2 && sea[0].layerMask == kLayerA && sea[1].layerMask == kLayerB &&
          Near(sea[0].position[1] + 0.5f * sea[0].scale[1], st::kSeaY, 1e-4f) && std::fabs(sea[0].position[0] - 123.0f) <= 0.5f * st::kSeaSnap);
    // rays that look down from every room entry, every lamp and the balcony meet something within the far plane
    std::vector<Vec3> eyes;
    const float eyeHeight = Player::Settings{}.eyeHeight;
    for (const st::Room& r : st::kRooms)
    {
        eyes.push_back({r.entry.x, r.entry.y + eyeHeight, r.entry.z});
    }
    for (const st::AnchorDesc& a : st::kAnchors)
    {
        eyes.push_back({a.stand.x, a.stand.y + eyeHeight, a.stand.z});
    }
    eyes.push_back({0.0f, st::kSummitY + eyeHeight, 28.25f});
    eyes.push_back({2.9f, st::kTerraceY + eyeHeight, -12.0f}); // the edge of the terrace, where the void was
    eyes.push_back({13.5f, st::kBridgeY + eyeHeight, 59.0f});  // over the broken sky bridge
    std::vector<const SceneObject*> boxes;
    for (const SceneObject& o : st::kStageStatics)
    {
        if (o.tilt == 0.0f)
        {
            boxes.push_back(&o);
        }
    }
    for (const SceneObject& o : st::kDecor)
    {
        if (o.tilt == 0.0f)
        {
            boxes.push_back(&o);
        }
    }
    int rays = 0;
    int void_ = 0;
    for (const Vec3& eye : eyes)
    {
        for (int k = 0; k < 16; ++k)
        {
            const float a = 6.2831853f * static_cast<float>(k) / 16.0f;
            for (float degrees : {-5.0f, -12.0f, -30.0f, -60.0f})
            {
                const float e = degrees * 3.14159265f / 180.0f;
                const Vec3 d{std::sin(a) * std::cos(e), std::sin(e), std::cos(a) * std::cos(e)};
                ++rays;
                bool met = false;
                float t = 0.0f;
                for (; t <= st::kViewDistance && !met; t += 1.0f) // the ground and the sea, step by step
                {
                    const Vec3 p{eye.x + d.x * t, eye.y + d.y * t, eye.z + d.z * t};
                    met = p.y <= land.Height(p.x, p.z) || (p.y <= st::kSeaY && p.z < st::kSeaNorth);
                }
                const Vec3 far{eye.x + d.x * st::kViewDistance, eye.y + d.y * st::kViewDistance, eye.z + d.z * st::kViewDistance};
                for (std::size_t b = 0; b < boxes.size() && !met; ++b)
                {
                    met = SegmentHitsBox(eye, far, *boxes[b]);
                }
                void_ += met ? 0 : 1;
            }
        }
    }
    std::printf("  immersion: %zu eyes, %d rays looking down, %d into the void\n", eyes.size(), rays, void_);
    CHECK(void_ == 0);
}

void TestTown()
{
    const tr::Land& land = st::TheLand();
    int houses = 0;
    bool level = true;
    for (const st::Plot& p : st::SkylinePlots())
    {
        if (p.kind == st::PlotKind::Garden)
        {
            continue;
        }
        ++houses;
        for (float x : {p.x0, p.x1})
        {
            for (float z : {p.z0, p.z1})
            {
                level = level && Near(land.Height(x, z), st::kPlainY, 1e-3f);
            }
        }
        // turned, it stays on its plot
        const float h = 0.5f * p.side * (std::fabs(std::cos(p.yaw)) + std::fabs(std::sin(p.yaw)));
        level = level && h <= 0.5f * (p.x1 - p.x0) + 1e-3f && std::fabs(p.yaw) < 0.86f;
    }
    std::printf("  the town: %d houses and towers on the levelled ground\n", houses);
    CHECK(houses > 100 && level);
}

void TestPlants()
{
    const tr::Land& land = st::TheLand();
    const std::vector<st::Plant> plants = st::PlacePlants(land);
    int wild = 0;
    int garden = 0;
    bool grounded = true;
    bool dry = true;
    bool offRock = true;
    bool away = true;
    bool apart = true;
    bool inPlots = true;
    const std::vector<st::Plot> plots = st::SkylinePlots();
    for (std::size_t i = 0; i < plants.size(); ++i)
    {
        const st::Plant& p = plants[i];
        grounded = grounded && Near(p.base.y, land.Height(p.base.x, p.base.z), 1e-3f);
        dry = dry && p.base.y >= st::kSeaY + 1.5f;
        offRock = offRock && land.Slope(p.base.x, p.base.z) <= 0.97f;
        for (const st::Room& r : st::kRooms)
        {
            away = away && st::FootprintGap(p.base.x, p.base.z, p.base.x, p.base.z, r.x0, r.z0, r.x1, r.z1) >= st::kPlantFromRoom;
        }
        for (const st::Pit& pit : st::kPits)
        {
            away = away && st::FootprintGap(p.base.x, p.base.z, p.base.x, p.base.z, pit.x0, pit.z0, pit.x1, pit.z1) >= st::kPitReach;
        }
        for (std::size_t j = i + 1; j < plants.size(); ++j)
        {
            const st::Plant& q = plants[j];
            const float need = 0.5f * (st::PlantSpacing(p.kind) + st::PlantSpacing(q.kind)) - 1e-3f;
            const float dx = p.base.x - q.base.x;
            const float dz = p.base.z - q.base.z;
            apart = apart && dx * dx + dz * dz >= need * need;
        }
        // a plant in the town is in a garden plot (never on a street, never in a house); the cypresses of the ring road stand at
        // the edge of the town
        if (st::TownMask(p.base.x, p.base.z) > 0.85f)
        {
            bool inGarden = false;
            for (const st::Plot& plot : plots)
            {
                inGarden = inGarden || (plot.kind == st::PlotKind::Garden && p.base.x > plot.x0 && p.base.x < plot.x1 && p.base.z > plot.z0 && p.base.z < plot.z1);
            }
            inPlots = inPlots && inGarden;
            ++garden;
        }
        else
        {
            ++wild;
        }
    }
    std::printf("  plants: %zu (%d in the gardens of the town, %d outside it)\n", plants.size(), garden, wild);
    CHECK(plants.size() > 500 && garden > 10 && wild > 300);
    CHECK(grounded && dry && offRock && away && apart && inPlots);
    // olives grow on slopes that face the sun, in rows along the contour
    int olives = 0;
    bool sunny = true;
    for (const st::Plant& p : plants)
    {
        if (p.kind != st::PlantKind::Olive)
        {
            continue;
        }
        ++olives;
        const Vec3 n = land.Normal(p.base.x, p.base.z);
        const float gx = -n.x / n.y;
        const float gz = -n.z / n.y;
        sunny = sunny && (gz > 0.02f || gx < -0.04f) && std::fmod(p.base.y + 40.0f, 3.0f) < 0.5f + 1e-3f;
    }
    CHECK(olives > 20 && sunny);
    // the boxes: two parts (trunk, crown) per tree in A; the lemon trees have no crown in B
    std::vector<SceneObject> boxes;
    st::PlantBoxes({{0.0f, 0.0f, 0.0f}, st::PlantKind::Lemon, 1.0f, 0.3f}, boxes);
    int inA = 0;
    int inB = 0;
    for (const SceneObject& o : boxes)
    {
        inA += o.layerMask == kLayerA ? 1 : 0;
        inB += o.layerMask == kLayerB ? 1 : 0;
    }
    CHECK(inA == 2 && inB == 1);
}

void TestBakedGround()
{
    const tr::Land& land = st::TheLand();
    const dl::BatchMesh mesh = st::BakeGround(land, 0);
    int drawnCells = land.nearField.nx * land.nearField.nz;
    for (int j = 0; j < land.farField.nz; ++j)
    {
        for (int i = 0; i < land.farField.nx; ++i)
        {
            drawnCells += land.FarCellHidden(i, j) ? 0 : 1;
        }
    }
    CHECK(mesh.indices.size() == static_cast<std::size_t>(drawnCells) * 6u && mesh.vertices.size() == mesh.indices.size());
    bool onLand = true;
    bool up = true;
    for (std::size_t i = 0; i < mesh.vertices.size(); i += 7) // every seventh: the test stays fast
    {
        const dl::BatchVertex& v = mesh.vertices[i];
        onLand = onLand && Near(land.Height(v.position[0], v.position[2]), v.position[1], 2e-3f);
        up = up && v.normal[1] > 0.0f && v.color[3] == 0.0f;
    }
    CHECK(onLand && up);
    bool flat = true; // the three corners of a triangle share its normal
    for (std::size_t t = 0; t + 2 < mesh.vertices.size(); t += 3 * 97)
    {
        flat = flat && mesh.vertices[t].normal[0] == mesh.vertices[t + 2].normal[0] && mesh.vertices[t].normal[1] == mesh.vertices[t + 1].normal[1];
    }
    CHECK(flat);
    const dl::BatchMesh world = st::BakeWorld(1);
    std::printf("  baked world: %zu triangles in a layer (the ground and the plants)\n", world.indices.size() / 3);
    CHECK(world.indices.size() > mesh.indices.size());
}

void TestFootings()
{
    // a footing is drawn rock under a floor or a wall at the edge of a pit; no place to stand of the map lies in one
    int footings = 0;
    int bad = 0;
    for (const SceneObject& f : st::kDecor)
    {
        if (!st::SameRgb(f, st::kFootingRock))
        {
            continue;
        }
        ++footings;
        CHECK(Near(f.position[1] - 0.5f * f.scale[1], st::kPitY, 1e-4f));
        for (const Place& p : EdgePlaces())
        {
            const bool inside = std::fabs(p.feet.x - f.position[0]) < 0.5f * f.scale[0] - 0.05f && std::fabs(p.feet.z - f.position[2]) < 0.5f * f.scale[2] - 0.05f &&
                                p.feet.y + 0.2f > f.position[1] - 0.5f * f.scale[1] && p.feet.y + 0.2f < f.position[1] + 0.5f * f.scale[1];
            if (inside && bad < 5)
            {
                std::printf("  a place (%.2f %.2f %.2f) lies in the footing at %.2f %.2f %.2f, size %.2f %.2f %.2f\n", static_cast<double>(p.feet.x),
                            static_cast<double>(p.feet.y), static_cast<double>(p.feet.z), static_cast<double>(f.position[0]), static_cast<double>(f.position[1]),
                            static_cast<double>(f.position[2]), static_cast<double>(f.scale[0]), static_cast<double>(f.scale[1]), static_cast<double>(f.scale[2]));
            }
            bad += inside ? 1 : 0;
        }
    }
    std::printf("  footings: %d\n", footings);
    CHECK(footings > 5 && bad == 0);
}

void TestScenery()
{
    // nothing of the scenery stands where a body can land on it or walk into it: each box is 4.5 m or more from every room, or
    // beside it (not over it) with its top out of reach of a jump from the floor of the room
    std::vector<SceneObject> scenery;
    st::Scenery([](float x, float z) { return st::TheLand().Height(x, z); }, scenery);
    int near = 0;
    int bad = 0;
    for (const SceneObject& o : scenery)
    {
        float lo[3];
        float hi[3];
        st::AxisBounds(o, lo, hi);
        for (const st::Room& r : st::kRooms)
        {
            const float gap = st::FootprintGap(lo[0], lo[2], hi[0], hi[2], r.x0, r.z0, r.x1, r.z1);
            if (gap >= st::kSkylineFromRoom)
            {
                continue;
            }
            ++near;
            const bool beside = gap > 0.0f && hi[1] > r.entry.y + Player::Settings{}.jumpHeight + 0.3f;
            if (!beside)
            {
                if (bad++ < 5)
                {
                    std::printf("  scenery too near the %s: a box at %.2f %.2f %.2f, top %.2f, %.2f m away\n", r.name, static_cast<double>(o.position[0]),
                                static_cast<double>(o.position[1]), static_cast<double>(o.position[2]), static_cast<double>(hi[1]), static_cast<double>(gap));
                }
            }
        }
    }
    std::printf("  scenery: %zu boxes, %d near a room and out of reach, %d wrong\n", scenery.size(), near - bad, bad);
    CHECK(scenery.size() > 1000 && bad == 0);
}

} // namespace

int main()
{
    TestNoise();
    TestField();
    TestLand();
    TestCollision();
    TestUnderMap();
    TestImmersion();
    TestTown();
    TestPlants();
    TestBakedGround();
    TestFootings();
    TestScenery();
    TestNoEscape();
    std::printf("%d checks passed, %d failed\n", g_passed, g_failed);
    return g_failed == 0 ? 0 : 1;
}
