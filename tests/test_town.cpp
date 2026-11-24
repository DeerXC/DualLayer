// test_town.cpp - tests of the town of the model as data (no window, no GPU): src/scene/Town.h, src/scene/TriMesh.h and the
// walking body on them (src/scene/Player.h SetMeshes). Reads data/town.dlt (python tools/layout/city_model.py export).
//   g++ -std=c++20 -O2 -Wall -Wextra -I src tests/test_town.cpp -o test_town
//   MSVC (Developer PowerShell):  .\tests\run_tests.ps1 -Only town
// What is checked:
//   1. the file: a bad magic, another version, a file cut short and bytes after the end are refused
//   2. the data: polygons, materials, stations and streets; every material used exists; the water is in A only
//   3. the triangle solid: a ball resting on a floor, pushed out of a wall from either side, a surface found under a point
//   4. drawn: each polygon of a moment drawn from both sides (the terrain and a surface of water from above); the front of
//      every triangle faces along its normal
//   5. the start: the body stands on the quay in both moments, and fits there (a switch is allowed); walking north from it for
//      0.8 s keeps it on the quay (tools/check.ps1 does that in the game and reads the height of the eye)
//   6. the streets: the real body walks every street from end to end in A without a jump; in B the streets it cannot walk
//      are listed (fallen arches lie across some of them)
//   7. the water: under the moat of A is in the water, the same place in B is not
// Returns 1 when a check fails.

#include "scene/Player.h"
#include "scene/Town.h"
#include "scene/TriMesh.h"

#include <cmath>
#include <cstdio>
#include <string>
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
namespace town = dl::town;

constexpr float kDt = 1.0f / 60.0f;

bool Near(float a, float b, float eps)
{
    return std::fabs(a - b) <= eps;
}

const town::TownData& Town()
{
    static town::TownData data;
    static bool loaded = false;
    if (!loaded)
    {
        std::string error;
        loaded = data.Load(town::kPath, error);
        if (!loaded)
        {
            std::printf("  cannot load %s: %s\n", town::kPath, error.c_str());
        }
    }
    return data;
}

void TestFile()
{
    town::TownData d;
    std::string error;
    const unsigned char junk[16] = {'N', 'O', 'T', 'A', 'T', 'O', 'W', 'N', 1, 0, 0, 0, 0, 0, 0, 0};
    CHECK(!d.Parse(junk, sizeof(junk), error) && !error.empty());
    unsigned char head[12] = {'D', 'L', 'T', 'O', 'W', 'N', 0, 0, 99, 0, 0, 0};
    CHECK(!d.Parse(head, sizeof(head), error) && error.find("version") != std::string::npos);
    std::FILE* f = std::fopen(town::kPath, "rb");
    CHECK(f != nullptr);
    if (f == nullptr)
    {
        return;
    }
    std::vector<unsigned char> bytes;
    unsigned char buffer[65536];
    std::size_t n = 0;
    while ((n = std::fread(buffer, 1, sizeof(buffer), f)) > 0)
    {
        bytes.insert(bytes.end(), buffer, buffer + n);
    }
    std::fclose(f);
    CHECK(d.Parse(bytes.data(), bytes.size(), error));
    CHECK(!d.Parse(bytes.data(), bytes.size() - 7, error) && d.polygons.empty()); // cut short: nothing half-read is kept
    bytes.push_back(0);
    CHECK(!d.Parse(bytes.data(), bytes.size(), error));
}

void TestData()
{
    const town::TownData& t = Town();
    std::printf("  town: %zu polygons, %zu vertices, %zu materials, %zu places, %zu streets\n", t.polygons.size(), t.vertices.size(), t.materials.size(),
                t.places.size(), t.streets.size());
    CHECK(t.polygons.size() > 10000 && t.materials.size() >= 20 && t.places.size() >= 10 && t.streets.size() >= 8);
    int water = 0;
    bool ok = true;
    for (const town::Polygon& p : t.polygons)
    {
        ok = ok && p.material < t.materials.size() && p.first + p.count <= t.vertices.size();
        if (p.kind == town::Kind::Water)
        {
            ++water;
            ok = ok && p.worlds == 1u; // the water of the model is in A only
        }
    }
    CHECK(ok && water > 10 && static_cast<std::size_t>(water) == t.waters.size());
    CHECK(t.lights[0].fog > 0.0f && t.lights[1].fog > t.lights[0].fog); // the cold present is foggier than the warm past
    CHECK(std::string(t.NearestPlace(t.spawn.x, t.spawn.z)).find("quay") != std::string::npos);
}

void TestTriMesh()
{
    town::TriMesh m;
    // a floor of 10 x 10 m at y 0 and a wall at x 2 from z -5 to 5, 3 m high
    m.Add({-5, 0, -5}, {-5, 0, 5}, {5, 0, 5});
    m.Add({-5, 0, -5}, {5, 0, 5}, {5, 0, -5});
    m.Add({2, 0, -5}, {2, 3, -5}, {2, 3, 5});
    m.Add({2, 0, -5}, {2, 3, 5}, {2, 0, 5});
    m.Add({0, 0, 0}, {1, 0, 0}, {2, 0, 0}); // no area: left out
    m.Build();
    CHECK(m.TriangleCount() == 4);
    Vec3 c{0.0f, 0.2f, 0.0f};
    CHECK(m.PushOut(c, 0.35f) > 0.1f && Near(c.y, 0.35f, 1e-4f)); // resting on the floor
    Vec3 w{1.8f, 1.0f, 0.0f};
    m.PushOut(w, 0.35f);
    CHECK(Near(w.x, 1.65f, 1e-4f)); // out of the wall to the side it came from
    Vec3 e{2.2f, 1.0f, 0.0f};
    m.PushOut(e, 0.35f);
    CHECK(Near(e.x, 2.35f, 1e-4f)); // and from the other side
    float y = 0.0f;
    CHECK(m.HighestBelow(-1.0f, -1.0f, 1.0f, -1.0f, y) && Near(y, 0.0f, 1e-5f) && !m.HighestBelow(20.0f, 0.0f, 1.0f, -1.0f, y));
    Vec3 far{50.0f, 0.0f, 50.0f};
    CHECK(m.PushOut(far, 0.35f) == 0.0f);
}

void TestDrawn()
{
    const town::TownData& t = Town();
    for (int layer = 0; layer < 2; ++layer)
    {
        const dl::BatchMesh drawn = t.BakeLayer(layer);
        const dl::BatchMesh water = t.BakeWater(layer);
        std::printf("  drawn %c: %zu triangles, %zu vertices; water %zu triangles\n", layer == 0 ? 'A' : 'B', drawn.indices.size() / 3, drawn.vertices.size(),
                    water.indices.size() / 3);
        CHECK(drawn.indices.size() > 30000 * 3 && drawn.indices.size() % 3 == 0);
        CHECK(layer == 0 ? water.indices.size() > 0 : water.indices.empty());
        // the front of every triangle faces along its normal (the static batch culls back faces)
        int wrong = 0;
        for (std::size_t i = 0; i + 2 < drawn.indices.size(); i += 3)
        {
            const dl::BatchVertex& a = drawn.vertices[drawn.indices[i]];
            const dl::BatchVertex& b = drawn.vertices[drawn.indices[i + 1]];
            const dl::BatchVertex& c = drawn.vertices[drawn.indices[i + 2]];
            const Vec3 n = dl::terrain::Cross(Vec3{b.position[0] - a.position[0], b.position[1] - a.position[1], b.position[2] - a.position[2]},
                                              Vec3{c.position[0] - a.position[0], c.position[1] - a.position[1], c.position[2] - a.position[2]});
            const float along = n.x * a.normal[0] + n.y * a.normal[1] + n.z * a.normal[2];
            wrong += along < -1.0e-6f ? 1 : 0;
        }
        CHECK(wrong == 0);
        // every water is drawn as water; a surface of water faces up (a fall of water stands: it is seen from both sides)
        bool up = true;
        for (const dl::BatchVertex& v : water.vertices)
        {
            up = up && v.params[0] > 1.5f && (v.normal[1] > 0.0f || std::fabs(v.normal[1]) < 0.3f);
        }
        CHECK(up);
        // no colour is the stone tint of the greybox (alpha 0)
        bool own = true;
        for (std::size_t i = 0; i < drawn.vertices.size(); i += 97)
        {
            own = own && drawn.vertices[i].color[3] == 0.0f;
        }
        CHECK(own);
    }
}

struct Walker
{
    town::TriMesh a;
    town::TriMesh b;
    Player body;
    int layer = 0;

    explicit Walker(const town::TownData& t) : a(t.BuildMesh(0)), b(t.BuildMesh(1))
    {
        body.SetScene(nullptr, 0);
        body.SetMeshes(&a, &b);
    }

    void PlaceAt(const Vec3& feet)
    {
        body.SetSpawn(feet);
        body.Respawn();
        for (int i = 0; i < 30; ++i)
        {
            body.Update({}, {0.0f, 0.0f, 1.0f}, layer, 0.0f, kDt);
        }
    }

    /// Walks to (x, z) without a jump. False when it makes no headway for 1.5 s, or falls far.
    bool WalkTo(float x, float z)
    {
        float bestD = 1.0e9f;
        float sinceBetter = 0.0f;
        for (int i = 0; i < 60 * 40; ++i)
        {
            const float dx = x - body.Feet().x;
            const float dz = z - body.Feet().z;
            const float d = std::sqrt(dx * dx + dz * dz);
            if (d < 0.6f)
            {
                return true;
            }
            if (d < bestD - 0.05f)
            {
                bestD = d;
                sinceBetter = 0.0f;
            }
            else if ((sinceBetter += kDt) > 1.5f)
            {
                return false;
            }
            Player::Input in;
            in.moveForward = 1.0f;
            body.Update(in, {dx / d, 0.0f, dz / d}, layer, 0.0f, kDt);
        }
        return false;
    }
};

void TestStart()
{
    const town::TownData& t = Town();
    for (int layer = 0; layer < 2; ++layer)
    {
        Walker w(t);
        w.layer = layer;
        w.PlaceAt(t.spawn);
        CHECK(w.body.Grounded() && Near(w.body.Feet().y, t.spawn.y, 0.1f));
        // it fits in the other moment there: no ball overlaps its solid
        const town::TriMesh& other = layer == 0 ? w.b : w.a;
        bool fits = true;
        for (int i = 0; i < 3; ++i)
        {
            Vec3 c = w.body.Ball(i);
            fits = fits && other.PushOut(c, w.body.GetSettings().radius - 0.03f) <= 0.005f;
        }
        CHECK(fits && !t.Submerged(w.body.Feet(), 1 - layer));
        Player::Input in;
        in.moveForward = 1.0f;
        for (int i = 0; i < 48; ++i)
        {
            w.body.Update(in, {0.0f, 0.0f, 1.0f}, layer, 0.0f, kDt);
        }
        for (int i = 0; i < 40; ++i)
        {
            w.body.Update({}, {0.0f, 0.0f, 1.0f}, layer, 0.0f, kDt);
        }
        CHECK(w.body.Feet().z - t.spawn.z > 1.8f && Near(w.body.Feet().y, t.spawn.y, 0.2f));
    }
}

void TestStreets()
{
    const town::TownData& t = Town();
    for (int layer = 0; layer < 2; ++layer)
    {
        int walked = 0;
        std::string blocked;
        for (const town::Street& s : t.streets)
        {
            Walker w(t);
            w.layer = layer;
            const Vec3 start{s.line.front().x, s.line.front().y + 0.1f, s.line.front().z};
            w.PlaceAt(start);
            bool ok = w.body.Grounded();
            std::size_t k = 1;
            for (; ok && k < s.line.size(); k += 2)
            {
                ok = w.WalkTo(s.line[k].x, s.line[k].z);
            }
            ok = ok && w.WalkTo(s.line.back().x, s.line.back().z) && Near(w.body.Feet().y, s.line.back().y, 0.8f);
            if (ok)
            {
                ++walked;
            }
            else
            {
                char where[160];
                std::snprintf(where, sizeof(where), "%s%s (stopped at %.1f %.1f %.1f)", blocked.empty() ? "" : "; ", s.name.c_str(), static_cast<double>(w.body.Feet().x),
                              static_cast<double>(w.body.Feet().y), static_cast<double>(w.body.Feet().z));
                blocked += where;
            }
        }
        std::printf("  streets walked in %c: %d of %zu%s%s\n", layer == 0 ? 'A' : 'B', walked, t.streets.size(), blocked.empty() ? "" : "; not: ", blocked.c_str());
        if (layer == 0)
        {
            CHECK(walked == static_cast<int>(t.streets.size())); // in the past every street is open
        }
        else
        {
            CHECK(walked > 0);
        }
    }
}

void TestWater()
{
    const town::TownData& t = Town();
    // the moat round the rock: a point between the rock and the gorge wall, under its surface
    bool found = false;
    for (const std::uint32_t i : t.waters)
    {
        const town::Polygon& p = t.polygons[i];
        const Vec3& v = t.vertices[p.first];
        const Vec3& u = t.vertices[p.first + 2u];
        const Vec3 mid{(v.x + u.x) * 0.5f, v.y - 1.0f, (v.z + u.z) * 0.5f};
        if (t.Submerged(mid, 0) && !t.Submerged(mid, 1) && !t.Submerged({mid.x, v.y + 0.5f, mid.z}, 0))
        {
            found = true;
            break;
        }
    }
    CHECK(found);
}

} // namespace

int main()
{
    TestFile();
    TestData();
    TestTriMesh();
    TestDrawn();
    TestStart();
    TestStreets();
    TestWater();
    std::printf("%d checks passed, %d failed\n", g_passed, g_failed);
    return g_failed == 0 ? 0 : 1;
}
