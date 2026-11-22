// test_reach.cpp - searches the whole map (src/scene/Stage.h) for ways round it, so that a skip is found by a machine and
// not by a player. No window, no GPU; the real walking body (Player.h) does every jump.
//   g++ -std=c++20 -O2 -Wall -Wextra -I src tests/test_reach.cpp -o test_reach
//   MSVC (Developer PowerShell):  .\tests\run_tests.ps1 -Only reach
// Header-only: there is no .cpp to add to the project.
// The search:
//   1. every place to stand: the top of every axis-aligned box and of every ramp of the map (also the pedestals of the anchor
//      lamps, which belong to the room they stand in) is sampled on a grid of
//      kSpacing metres, in each layer in which it exists; a sample counts when a body put there stands still on it
//   2. from every sample that a jump could lift to something higher (a surface 0.3 .. 2.4 m above it within kReach metres),
//      the body jumps in 16 directions, from a stand and after a short run-up, once in its own layer and once switching to
//      the other layer at the top of the jump, holding the key the whole flight (the farthest a jump can go)
//   3. a jump that ends on solid ground more than kRise above where it started is a SKIP unless the two places are in the
//      same room or in rooms that are joined by walking (kWalkJoined), or it ends on a ramp that starts or ends in such a room:
//      such a jump only shortens a way that exists anyway
// The map is searched with every door open, every bridge laid, the portcullis moved by the prism, without the block and
// without the crate of the canal: the most permissive state (a door adds
// no way round, and what the block and the orbs allow is checked by the heights and the walks in tests/test_stage.cpp). Places that are themselves out of reach (in no
// room: wall tops) are not searched from; the jump onto them is the skip. The balcony at the top of the tower is not searched
// from either: reaching it is the end of the run.
// Every skip found is printed with where it started, the direction, the layer and where it ended. Returns 1 when a check
// fails. The numbers of the search are printed, so that a change that makes it miss whole areas is seen.

#include "scene/CityParts.h"
#include "scene/Stage.h"

#include <cmath>
#include <cstddef>
#include <cstdio>
#include <string_view>
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

constexpr float kSpacing = 0.75f; // UNITS: metres between two samples on a surface
constexpr float kRise = 0.3f;     // UNITS: metres; a landing this much higher than the take-off counts as going up
constexpr float kReach = 5.0f;    // UNITS: metres; a surface farther away than this cannot be reached by one jump
constexpr float kNear = 9.0f;     // UNITS: metres; the solids within this distance of a sample are given to the body
constexpr float kDt = 1.0f / 90.0f;
constexpr int kDirections = 16;

/// Pairs of rooms (names of Stage::kRooms) that are joined by walking: a jump from one to the other is not a way round.
const char* const kWalkJoined[][2] = {
    {"guardhouse", "terrace"},   {"terrace", "ditch"},        {"ditch", "gap"},                 {"gap", "courtyard"},        {"square", "west hall"},          {"west hall", "west lower"},
    {"west lower", "west vault"}, {"west lower", "west mezzanine"}, {"west lower", "west landing"}, {"square", "west landing"},
    {"square", "east hall"},        {"east hall", "canal bed"},  {"square", "undercroft"},     {"east hall", "east quay"},  {"east quay", "east upper"}, {"square", "north hall"},         {"square", "cathedral"},
    {"west hall", "west passage"}, {"west passage", "north hall"}, {"north hall", "sky bridge"}, {"sky bridge", "east stairs"},
    {"east stairs", "east hall"}, {"cellar", "canal bed"}, {"cellar", "west lower"},
};

int RoomIndex(const char* name)
{
    for (int i = 0; i < st::kRoomCount; ++i)
    {
        if (std::string_view(st::kRooms[i].name) == name)
        {
            return i;
        }
    }
    return -1;
}

bool Joined(int a, int b)
{
    if (a < 0 || b < 0)
    {
        return false;
    }
    if (a == b)
    {
        return true;
    }
    for (const auto& pair : kWalkJoined)
    {
        const int x = RoomIndex(pair[0]);
        const int y = RoomIndex(pair[1]);
        if ((a == x && b == y) || (a == y && b == x))
        {
            return true;
        }
    }
    return false;
}

/// The axis-aligned box that holds `o` (a turned box: its corners turned).
void Bounds(const SceneObject& o, float lo[3], float hi[3])
{
    const float hx = 0.5f * o.scale[0];
    const float hy = 0.5f * o.scale[1];
    const float hz = 0.5f * o.scale[2];
    const float c = std::cos(o.tilt);
    const float s = std::sin(o.tilt);
    const float ey = std::fabs(hy * c) + std::fabs(hz * s);
    const float ez = std::fabs(hy * s) + std::fabs(hz * c);
    const float e[3] = {hx, ey, ez};
    for (int a = 0; a < 3; ++a)
    {
        lo[a] = o.position[a] - e[a];
        hi[a] = o.position[a] + e[a];
    }
}

/// The horizontal distance from (x, z) to the footprint of `o`.
float FlatDistance(const SceneObject& o, float x, float z)
{
    float lo[3];
    float hi[3];
    Bounds(o, lo, hi);
    const float dx = x < lo[0] ? lo[0] - x : (x > hi[0] ? x - hi[0] : 0.0f);
    const float dz = z < lo[2] ? lo[2] - z : (z > hi[2] ? z - hi[2] : 0.0f);
    return std::sqrt(dx * dx + dz * dz);
}

struct Sample
{
    Vec3 feet;
    int layer;
    int room;
};

/// The places to stand on the top of `o` (for a ramp: on its slope), as feet positions.
std::vector<Vec3> TopPoints(const SceneObject& o)
{
    std::vector<Vec3> points;
    const float hx = 0.5f * o.scale[0];
    const float hz = 0.5f * o.scale[2];
    const float inset = 0.2f;
    const auto steps = [&](float half) {
        const int n = static_cast<int>(std::floor(2.0f * (half - inset) / kSpacing));
        return n < 0 ? 0 : n;
    };
    const int nx = steps(hx);
    const int nz = steps(hz);
    for (int i = 0; i <= nx; ++i)
    {
        for (int k = 0; k <= nz; ++k)
        {
            const float lx = nx == 0 ? 0.0f : -hx + inset + static_cast<float>(i) * (2.0f * (hx - inset)) / static_cast<float>(nx);
            const float lz = nz == 0 ? 0.0f : -hz + inset + static_cast<float>(k) * (2.0f * (hz - inset)) / static_cast<float>(nz);
            // the top face in the local space of the box, then into the world (turned round X by the tilt)
            const float ly = 0.5f * o.scale[1];
            const float c = std::cos(o.tilt);
            const float s = std::sin(o.tilt);
            points.push_back({o.position[0] + lx, o.position[1] + ly * c - lz * s, o.position[2] + ly * s + lz * c});
        }
    }
    return points;
}

/// Every box of the map in the most permissive state: the fixed boxes and the gates that are there while open (bridges).
const std::vector<SceneObject>& PermissiveMap()
{
    static const std::vector<SceneObject> all = [] {
        std::vector<SceneObject> v(st::kStageStatics.begin(), st::kStageStatics.end());
        for (const dl::mech::Gate& g : st::kCityGates)
        {
            if (g.presentWhenOpen)
            {
                v.push_back(g.box); // a bridge, laid
            }
            else if (g.swapsLayer)
            {
                SceneObject moved = g.box; // the portcullis, moved by the prism: the bridge under it is open in A
                moved.layerMask = moved.layerMask == kLayerA ? kLayerB : kLayerA;
                v.push_back(moved);
            }
        }
        return v;
    }();
    return all;
}

/// The solids near (x, z).
std::vector<SceneObject> NearSolids(float x, float z)
{
    std::vector<SceneObject> near;
    for (const SceneObject& o : PermissiveMap())
    {
        if (FlatDistance(o, x, z) < kNear)
        {
            near.push_back(o);
        }
    }
    return near;
}

bool BodyClear(const Player& p, int layer, const std::vector<SceneObject>& solids)
{
    for (int i = 0; i < 3; ++i)
    {
        const Vec3 c = p.Ball(i);
        const dl::CollisionResult r = dl::ResolveSphere(c, p.GetSettings().radius, layer, 0.0f, solids.data(), static_cast<int>(solids.size()));
        if (r.pushed && (std::fabs(r.position.x - c.x) + std::fabs(r.position.y - c.y) + std::fabs(r.position.z - c.z)) > 0.01f)
        {
            return false;
        }
    }
    return true;
}

/// True when a body put at `feet` in `layer` stands there and stays.
bool Stands(const Vec3& feet, int layer, const std::vector<SceneObject>& solids)
{
    Player p;
    p.SetScene(solids.data(), static_cast<int>(solids.size()));
    p.Place({feet.x, feet.y + p.GetSettings().eyeHeight + 0.02f, feet.z});
    if (!BodyClear(p, layer, solids))
    {
        return false;
    }
    for (int i = 0; i < 20; ++i)
    {
        p.Update({}, {0.0f, 0.0f, 1.0f}, layer, 0.0f, kDt);
    }
    const Vec3 f = p.Feet();
    return p.Grounded() && std::fabs(f.x - feet.x) < 0.1f && std::fabs(f.z - feet.z) < 0.1f && std::fabs(f.y - feet.y) < 0.1f;
}

/// True when a jump from this sample could end higher: some surface 0.3 .. 2.4 m above it within kReach (either layer).
bool WorthJumping(const Sample& s)
{
    for (const SceneObject& o : PermissiveMap())
    {
        float lo[3];
        float hi[3];
        Bounds(o, lo, hi);
        if (hi[1] > s.feet.y + kRise && hi[1] < s.feet.y + 2.4f && FlatDistance(o, s.feet.x, s.feet.z) < kReach)
        {
            return true;
        }
    }
    return false;
}

struct Skip
{
    Sample from;
    Vec3 dir;
    bool runUp;
    bool switched;
    Vec3 to;
    int toRoom;
};

/// One jump. Returns true and fills `end` when the body ends on solid ground (not fallen) within the time.
bool Jump(const Sample& s, const Vec3& dir, bool runUp, bool doSwitch, const std::vector<SceneObject>& solids, Vec3& end, bool& switched)
{
    Player p;
    p.SetScene(solids.data(), static_cast<int>(solids.size()));
    p.Place({s.feet.x, s.feet.y + p.GetSettings().eyeHeight, s.feet.z});
    int layer = s.layer;
    switched = false;
    for (int i = 0; i < 6; ++i)
    {
        p.Update({}, dir, layer, 0.0f, kDt);
    }
    Player::Input go;
    go.moveForward = 1.0f;
    if (runUp)
    {
        for (int i = 0; i < 11; ++i) // about 0.12 s: full speed after about 0.6 m
        {
            p.Update(go, dir, layer, 0.0f, kDt);
        }
    }
    Player::Input jump = go;
    jump.jumpPressed = true;
    p.Update(jump, dir, layer, 0.0f, kDt);
    float air = 0.0f;
    for (int i = 0; i < 260; ++i)
    {
        if (doSwitch && !switched && p.VerticalSpeed() <= 0.0f)
        {
            switched = true;
            if (BodyClear(p, 1 - layer, solids))
            {
                layer = 1 - layer;
            }
            else
            {
                return false; // the guard refuses the switch: this jump is the one without it
            }
        }
        p.Update(go, dir, layer, 0.0f, kDt);
        air += kDt;
        if (p.Feet().y < s.feet.y - 12.0f || p.JustRespawned())
        {
            return false;
        }
        if (p.Grounded() && air > 0.1f)
        {
            for (int k = 0; k < 10; ++k)
            {
                p.Update({}, dir, layer, 0.0f, kDt);
            }
            if (!p.Grounded())
            {
                return false;
            }
            end = p.Feet();
            return true;
        }
    }
    return false;
}

/// The room a body standing on a ramp (or the slope under stairs) at `f` belongs to, for the search: a ramp is walked from the
/// rooms at its two ends, so a jump onto it from one of them is a short cut of a walk, not a skip. Returns -1 when `f` is not on
/// a ramp; `other` gets the room at its other end.
int RampRooms(const Vec3& f, int& other)
{
    // a body stands on a ramp also with its feet a little beyond its side (its ball rests on the edge): up to its radius
    const float kEdge = Player::Settings{}.radius;
    for (const SceneObject& r : st::kStageStatics) // every slope: the ramps and the slopes under stairs (kPieces)
    {
        if (r.tilt == 0.0f)
        {
            continue;
        }
        const float c = std::cos(r.tilt);
        const float s = std::sin(r.tilt);
        // the feet in the local space of the ramp (turned back round X)
        const float dy = f.y - r.position[1];
        const float dz = f.z - r.position[2];
        const float ly = dy * c + dz * s;
        const float lz = -dy * s + dz * c;
        if (std::fabs(f.x - r.position[0]) > 0.5f * r.scale[0] + kEdge || std::fabs(lz) > 0.5f * r.scale[2] + 0.2f || std::fabs(ly - 0.5f * r.scale[1]) > 0.3f)
        {
            continue;
        }
        const float half = 0.5f * r.scale[2] + 0.6f; // just beyond each end, on the top
        const float top = 0.5f * r.scale[1];
        const Vec3 a{r.position[0], r.position[1] + top * c - half * s, r.position[2] + top * s + half * c};
        const Vec3 b{r.position[0], r.position[1] + top * c + half * s, r.position[2] + top * s - half * c};
        other = st::RoomAt(b);
        return st::RoomAt(a);
    }
    other = -1;
    return -1;
}

/// The room of a place to stand: the room it is in, or, on the top of the pedestal of an anchor lamp, the room of the lamp (a
/// pedestal stands inside its room; a body on it is in that room).
int PlaceRoom(const Vec3& f)
{
    const int room = st::RoomAt(f);
    if (room >= 0)
    {
        return room;
    }
    for (const st::AnchorDesc& a : st::kAnchors)
    {
        if (std::fabs(f.x - a.base.x) < st::kAnchorHalf + 0.4f && std::fabs(f.z - a.base.z) < st::kAnchorHalf + 0.4f &&
            std::fabs(f.y - (a.base.y + st::kAnchorHeight)) < 0.2f)
        {
            return st::RoomIndex(a.room);
        }
    }
    return -1;
}

bool OnBalcony(const Vec3& f)
{
    return f.y > st::kSummitY - 0.2f && f.x > st::kTowerInnerWest && f.x < st::kTowerInnerEast && f.z > st::kTowerInnerSouth &&
           f.z < st::kRampSouth;
}

void TestRoomNames()
{
    for (const auto& pair : kWalkJoined)
    {
        CHECK(RoomIndex(pair[0]) >= 0 && RoomIndex(pair[1]) >= 0); // a renamed room must not make the list silently empty
    }
}

void TestNoSkip()
{
    std::vector<Sample> samples;
    int tops = 0;
    for (const SceneObject& o : PermissiveMap())
    {
        for (int layer = 0; layer < 2; ++layer)
        {
            if ((o.layerMask & (1u << layer)) == 0u)
            {
                continue;
            }
            for (const Vec3& f : TopPoints(o))
            {
                ++tops;
                const int room = PlaceRoom(f);
                if (room < 0 || OnBalcony(f))
                {
                    continue;
                }
                const std::vector<SceneObject> solids = NearSolids(f.x, f.z);
                if (Stands(f, layer, solids))
                {
                    samples.push_back({f, layer, room});
                }
            }
        }
    }
    int searched = 0;
    int jumps = 0;
    std::vector<Skip> skips;
    for (const Sample& s : samples)
    {
        if (!WorthJumping(s))
        {
            continue;
        }
        ++searched;
        const std::vector<SceneObject> solids = NearSolids(s.feet.x, s.feet.z);
        for (int d = 0; d < kDirections; ++d)
        {
            const float a = 6.2831853f * static_cast<float>(d) / static_cast<float>(kDirections);
            const Vec3 dir{std::sin(a), 0.0f, std::cos(a)};
            for (int variant = 0; variant < 4; ++variant)
            {
                const bool runUp = (variant & 1) != 0;
                const bool doSwitch = (variant & 2) != 0;
                Vec3 end{};
                bool switched = false;
                ++jumps;
                if (!Jump(s, dir, runUp, doSwitch, solids, end, switched) || end.y <= s.feet.y + kRise)
                {
                    continue;
                }
                const int room = PlaceRoom(end);
                int rampOther = -1;
                const int rampRoom = RampRooms(end, rampOther);
                if (rampRoom >= 0 || rampOther >= 0)
                {
                    if (Joined(s.room, rampRoom) || Joined(s.room, rampOther))
                    {
                        continue; // onto a ramp of a room joined to this one: a short cut of a walk
                    }
                }
                if (!Joined(s.room, room))
                {
                    skips.push_back({s, dir, runUp, switched, end, room});
                }
            }
        }
    }
    std::printf("  %d surface points, %d places to stand, %d searched, %d jumps, %zu skips\n", tops, static_cast<int>(samples.size()),
                searched, jumps, skips.size());
    for (std::size_t i = 0; i < skips.size() && i < 20; ++i)
    {
        const Skip& k = skips[i];
        std::printf("  SKIP from %s (%.2f %.2f %.2f) in %c, dir (%.2f, %.2f)%s%s -> %s (%.2f %.2f %.2f)\n",
                    st::kRooms[k.from.room].name, static_cast<double>(k.from.feet.x), static_cast<double>(k.from.feet.y),
                    static_cast<double>(k.from.feet.z), k.from.layer == 0 ? 'A' : 'B', static_cast<double>(k.dir.x),
                    static_cast<double>(k.dir.z), k.runUp ? ", run-up" : "", k.switched ? ", switch at the top" : "",
                    k.toRoom >= 0 ? st::kRooms[k.toRoom].name : "no room", static_cast<double>(k.to.x), static_cast<double>(k.to.y),
                    static_cast<double>(k.to.z));
    }
    CHECK(samples.size() > 500); // the search covers the map, not a corner of it
    CHECK(searched > 50);
    CHECK(skips.empty());
}

} // namespace

int main()
{
    TestRoomNames();
    TestNoSkip();
    std::printf("%d checks passed, %d failed\n", g_passed, g_failed);
    return g_failed == 0 ? 0 : 1;
}
