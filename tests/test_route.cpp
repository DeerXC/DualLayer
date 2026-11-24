// test_route.cpp - the whole town played through by a solver (docs/DESIGN.md 11): every puzzle shuts the way until it is
// solved, and the town can be finished. Reads data/town.dlt (python tools/layout/city_model.py export).
//   g++ -std=c++20 -O2 -Wall -Wextra -I src tests/test_route.cpp -o test_route
//   MSVC (Developer PowerShell):  .\tests\run_tests.ps1 -Only route
// The solver: a flood over cells of 0.5 m of both moments. From a cell the body walks to a neighbour (a step up of at most
// 1.2 m, any drop that lands; it fits there and halfway); it switches where it stands when it fits in the other moment (and
// falls there when there is no floor); it switches in the air: a jump of at most 1.5 m up with the moment changed at its
// top, onto a floor of the other moment within 1 m (the head must fit at the top in the moment it jumped in). The lantern
// is a circle given to a stage. The water of a moment is not walked. A handle is reached in A within its reach.
// The stages: the things in the order of the main path; for each, its handle is NOT reached before the one before it is
// done (so the order is forced), and it IS reached after. Then the end: the graveyard. Returns 1 when a check fails.

#include "scene/Lantern.h"
#include "scene/Moments.h"
#include "scene/Player.h"
#include "scene/Things.h"
#include "scene/Town.h"
#include "scene/TriMesh.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <unordered_set>
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

using dl::Vec3;
namespace town = dl::town;

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

constexpr float kCell = 0.5f;
bool g_verbose = false;
bool g_early[7] = {};
constexpr float kStep = 1.65f; // a jump onto a ledge (the full jump is 2.0 m; the margin is for the arc)
constexpr float kAirUp = 1.65f;

class Solver
{
  public:
    explicit Solver(const town::TownData& t) : m_t(t), m_a(t.BuildMesh(0)), m_b(t.BuildMesh(1)), m_moments(&m_a, &m_b)
    {
        m_things.Init(t);
        m_things.ApplyTo(m_moments);
        m_radius = dl::Player().GetSettings().radius;
        m_x0 = m_z0 = 1.0e9f;
        float x1 = -1.0e9f, z1 = -1.0e9f;
        for (const town::Street& s : t.streets)
        {
            for (const Vec3& p : s.line)
            {
                m_x0 = std::min(m_x0, p.x);
                m_z0 = std::min(m_z0, p.z);
                x1 = std::max(x1, p.x);
                z1 = std::max(z1, p.z);
            }
        }
        for (const town::Place& p : t.places)
        {
            m_x0 = std::min(m_x0, p.at.x);
            m_z0 = std::min(m_z0, p.at.z);
            x1 = std::max(x1, p.at.x);
            z1 = std::max(z1, p.at.z);
        }
        m_x0 -= 20.0f;
        m_z0 -= 20.0f;
        m_nx = static_cast<int>((x1 + 20.0f - m_x0) / kCell) + 1;
        m_nz = static_cast<int>((z1 + 20.0f - m_z0) / kCell) + 1;
    }
    Solver(const Solver&) = delete;
    Solver& operator=(const Solver&) = delete;

    void Do(int thing)
    {
        const town::ThingDef& d = m_t.things[static_cast<std::size_t>(thing)];
        const Vec3 feet{d.handle.x, d.handle.y - 1.1f, d.handle.z};
        const Vec3 none[1] = {{0.0f, -100.0f, 0.0f}};
        m_things.Use(feet, none, 1, m_radius, 0, m_moments, m_traces);
    }
    void SetLantern(const Vec3& foot)
    {
        m_moments.SetCircle({{foot.x, foot.y + town::kLanternUp, foot.z}, town::kRadius});
    }
    void NoLantern()
    {
        m_moments.SetCircle({});
    }
    int Find(const char* name) const
    {
        for (std::size_t i = 0; i < m_t.things.size(); ++i)
        {
            if (m_t.things[i].name == name)
            {
                return static_cast<int>(i);
            }
        }
        return -1;
    }

    /// Floods from `from` (in the moment `layer`).
    void Flood(const Vec3& from, int layer)
    {
        m_seen.clear();
        m_open.clear();
        m_all.clear();
        m_current = -1;
        m_how = 'w';
        float y = 0.0f;
        if (Floor(from.x, from.z, from.y + 0.5f, from.y - 3.0f, layer, y))
        {
            Visit(Ix(from.x), Iz(from.z), y, layer);
        }
        for (std::size_t head = 0; head < m_open.size(); ++head) // breadth first: the ways found are short
        {
            const Node n = m_open[head];
            m_current = n.parent;
            Expand(n);
            if (g_verbose && m_all.size() % 20000 == 0)
            {
                std::printf("    ... %zu\n", m_all.size());
                std::fflush(stdout);
            }
        }
    }

    /// True when the flood came within `r` metres (on the ground plane) and 2 m (up and down) of `p`, in `layer` (-1: any).
    bool Reached(const Vec3& p, float r = 1.0f, int layer = -1) const
    {
        for (const Node& n : m_all)
        {
            const float dx = X(n.i) - p.x;
            const float dz = Z(n.j) - p.z;
            if (dx * dx + dz * dz <= r * r && std::fabs(n.y - p.y) < 2.0f && (layer < 0 || n.layer == layer))
            {
                return true;
            }
        }
        return false;
    }
    /// A handle is used standing in A within reach of it.
    bool HandleReached(int thing) const
    {
        const town::ThingDef& d = m_t.things[static_cast<std::size_t>(thing)];
        return Reached({d.handle.x, d.handle.y - 1.1f, d.handle.z}, town::kHandleReach - 0.3f, 0);
    }
    /// Prints the way the flood found to near `p`: its switches and every 8th cell.
    void Way(const Vec3& p) const
    {
        int best = -1;
        float bd = 1.0e9f;
        for (std::size_t k = 0; k < m_all.size(); ++k)
        {
            const Node& n = m_all[k];
            const float d = std::fabs(X(n.i) - p.x) + std::fabs(Z(n.j) - p.z) + std::fabs(n.y - p.y);
            if (d < bd)
            {
                bd = d;
                best = static_cast<int>(k);
            }
        }
        std::vector<int> way;
        for (int k = best; k >= 0; k = m_all[static_cast<std::size_t>(k)].parent)
        {
            way.push_back(k);
        }
        std::reverse(way.begin(), way.end());
        for (std::size_t q = 0; q < way.size(); ++q)
        {
            const Node& n = m_all[static_cast<std::size_t>(way[q])];
            if (n.how != 'w' || q % 8 == 0 || q + 1 == way.size())
            {
                std::printf("      %c %c %.1f %.1f %.1f\n", n.how, n.layer == 0 ? 'A' : 'B', static_cast<double>(X(n.i)), static_cast<double>(n.y),
                            static_cast<double>(Z(n.j)));
            }
        }
    }
    void Probe(float x, float y, float z)
    {
        for (int layer = 0; layer < 2; ++layer)
        {
            float f = 0.0f;
            const bool has = Floor(x, z, y + 0.5f, y - 3.0f, layer, f);
            std::printf("  %c: floor %d %.2f fits %d steep? sub %d\n", layer == 0 ? 'A' : 'B', has ? 1 : 0, static_cast<double>(f), Fits(x, f + 0.05f, z, layer) ? 1 : 0,
                        m_t.Submerged({x, f + 0.05f, z}, layer) ? 1 : 0);
            for (int i = 0; i < 3; ++i)
            {
                Vec3 c{x, f + 0.05f + m_radius + 0.02f + 0.6f * static_cast<float>(i), z};
                const Vec3 c0 = c;
                const float d = m_moments.PushOut(c, m_radius - 0.03f, layer);
                std::printf("     ball %d pushed %.3f to %.2f %.2f %.2f (from %.2f)\n", i, static_cast<double>(d), static_cast<double>(c.x), static_cast<double>(c.y),
                            static_cast<double>(c.z), static_cast<double>(c0.y));
            }
        }
    }
    std::size_t Size() const
    {
        return m_all.size();
    }

  private:
    struct Node
    {
        int i, j;
        float y;
        int layer;
        int parent = -1;
        char how = 'w'; ///< w walk, s switch, a switch in the air
    };
    int m_current = -1;
    char m_how = 'w';

    const town::TownData& m_t;
    town::TriMesh m_a;
    town::TriMesh m_b;
    town::Moments m_moments;
    town::Things m_things;
    town::Traces m_traces;
    float m_radius = 0.35f;
    float m_x0 = 0.0f, m_z0 = 0.0f;
    int m_nx = 0, m_nz = 0;
    std::unordered_set<long long> m_seen;
    std::vector<Node> m_open;
    std::vector<Node> m_all;

    int Ix(float x) const
    {
        return static_cast<int>(std::lround((x - m_x0) / kCell));
    }
    int Iz(float z) const
    {
        return static_cast<int>(std::lround((z - m_z0) / kCell));
    }
    float X(int i) const
    {
        return m_x0 + static_cast<float>(i) * kCell;
    }
    float Z(int j) const
    {
        return m_z0 + static_cast<float>(j) * kCell;
    }

    /// The highest floor between y0 and y1 that the body would stand on in `layer` (with the circle, the floors of A in it).
    bool Floor(float x, float z, float y1, float y0, int layer, float& y) const
    {
        bool found = false;
        const town::Circle& c = m_moments.GetCircle();
        const auto take = [&](const town::TriMesh& mesh, bool inCircle) {
            float h = 0.0f;
            float top = y1;
            while (mesh.HighestBelow(x, z, top, y0, h))
            {
                const bool inside = c.Contains({x, h, z});
                if (inside == inCircle)
                {
                    if (!found || h > y)
                    {
                        y = h;
                        found = true;
                    }
                    return;
                }
                top = h - 0.01f;
            }
        };
        if (layer == 0)
        {
            found = m_a.HighestBelow(x, z, y1, y0, y); // in A the circle changes nothing
        }
        else if (!c.On())
        {
            found = m_b.HighestBelow(x, z, y1, y0, y);
        }
        else
        {
            take(m_b, false);
            take(m_a, true);
        }
        if (found && Steep(layer == 0 || !c.On() || !c.Contains({x, y, z}) ? (layer == 0 ? m_a : m_b) : m_a, x, z, y))
        {
            found = false; // a slope the body slides off (Player.h: about 53 degrees)
        }
        // the things are floors too (their boxes are in the moments' extra solids): a ray down through PushOut is costly, so
        // the boxes are looked at directly
        for (std::size_t k = 0; k < m_t.things.size(); ++k)
        {
            const std::uint8_t state = m_things.Open(k) ? 1 : 0;
            for (const town::ThingBox& b : m_t.things[k].boxes)
            {
                if (b.state != state || (b.worlds & (1u << layer)) == 0u)
                {
                    continue;
                }
                const float cs = std::cos(b.yaw), sn = std::sin(b.yaw);
                const float dx = x - b.centre.x, dz = z - b.centre.z;
                const float lu = dx * cs + dz * sn;
                const float lx = -dx * sn + dz * cs;
                const float top = b.centre.y + b.half.y;
                if (std::fabs(lu) <= b.half.x && std::fabs(lx) <= b.half.z && top <= y1 && top >= y0 && (!found || top > y))
                {
                    if (layer == 1 && c.On() && c.Contains({x, top, z}) && (b.worlds & 1u) == 0u)
                    {
                        continue; // a box of B only, inside the circle: not there
                    }
                    y = top;
                    found = true;
                }
            }
        }
        return found;
    }

    static bool Steep(const town::TriMesh& mesh, float x, float z, float y)
    {
        constexpr float d = 0.15f;
        float g = 0.0f;
        const float off[4][2] = {{d, 0.0f}, {-d, 0.0f}, {0.0f, d}, {0.0f, -d}};
        for (const auto& o : off)
        {
            float h = 0.0f;
            if (mesh.HighestBelow(x + o[0], z + o[1], y + 0.4f, y - 0.4f, h))
            {
                g = std::max(g, std::fabs(h - y) / d);
            }
        }
        return g > 1.33f;
    }

    bool Fits(float x, float y, float z, int layer) const
    {
        for (int i = 0; i < 3; ++i)
        {
            Vec3 c{x, y + m_radius + 0.02f + 0.6f * static_cast<float>(i), z};
            if (m_moments.PushOut(c, m_radius - 0.03f, layer) > 0.005f)
            {
                return false;
            }
        }
        return !m_t.Submerged({x, y, z}, layer, m_moments.GetCircle());
    }

    void Visit(int i, int j, float y, int layer)
    {
        if (i < 0 || j < 0 || i >= m_nx || j >= m_nz || y < m_t.sea - 3.0f)
        {
            return;
        }
        const long long key = ((static_cast<long long>(i) * 4096 + j) * 2 + layer) * 2048 + static_cast<long long>(std::lround((y + 100.0f) * 2.0f));
        // heights within 0.5 m are one node
        if (m_seen.count(key) != 0 || m_seen.count(key - 1) != 0 || m_seen.count(key + 1) != 0)
        {
            return;
        }
        if (!Fits(X(i), y + 0.05f, Z(j), layer))
        {
            return;
        }
        m_seen.insert(key);
        const Node node{i, j, y, layer, m_current, m_how};
        m_open.push_back(node);
        m_all.push_back(node);
        m_open.back().parent = static_cast<int>(m_all.size()) - 1; // (in the open list: its own index)
    }

    void Expand(const Node& n)
    {
        const float x = X(n.i), z = Z(n.j);
        // a walk to a neighbour; a jump up onto a ledge reaches 1 m (two cells), over a gap at the height of the ledge
        for (int dj = -2; dj <= 2; ++dj)
        {
            for (int di = -2; di <= 2; ++di)
            {
                const bool near = std::abs(di) + std::abs(dj) == 1;
                if (!near && (di == 0 && dj == 0))
                {
                    continue;
                }
                const float nx = x + static_cast<float>(di) * kCell;
                const float nz = z + static_cast<float>(dj) * kCell;
                float y = 0.0f;
                if (!Floor(nx, nz, n.y + kStep, n.y - 30.0f, n.layer, y))
                {
                    continue;
                }
                if (!near && y < n.y + 0.3f)
                {
                    continue; // the far cells only for a jump up
                }
                const float top = std::max(n.y, y) + 0.05f;
                if (!Fits((x + nx) * 0.5f, top, (z + nz) * 0.5f, n.layer) || (!near && !Fits(x, n.y + kStep, z, n.layer)))
                {
                    continue;
                }
                m_how = 'w';
                Visit(n.i + di, n.j + dj, y, n.layer);
            }
        }
        // the switch where it stands (and a fall in the other moment when nothing is under the feet there)
        const int other = 1 - n.layer;
        if (Fits(x, n.y + 0.05f, z, other))
        {
            float y = 0.0f;
            if (Floor(x, z, n.y + 0.3f, n.y - 30.0f, other, y))
            {
                m_how = 's';
                Visit(n.i, n.j, y, other);
            }
        }
        // the switch in the air: up to kAirUp over the floor, the moment changed at the top, onto a floor within 1 m
        if (Fits(x, n.y + kAirUp, z, n.layer) && Fits(x, n.y + kAirUp, z, other))
        {
            for (int dj2 = -2; dj2 <= 2; ++dj2)
            {
                for (int di2 = -2; di2 <= 2; ++di2)
                {
                    const float nx = x + static_cast<float>(di2) * kCell;
                    const float nz = z + static_cast<float>(dj2) * kCell;
                    float y = 0.0f;
                    if (!Floor(nx, nz, n.y + kAirUp, n.y - 0.5f, other, y))
                    {
                        continue;
                    }
                    // only where the moment jumped in has no floor like it (else walking and a switch get there)
                    float own = 0.0f;
                    if (Floor(nx, nz, y + 0.3f, y - 0.3f, n.layer, own))
                    {
                        continue;
                    }
                    if (Fits((x + nx) * 0.5f, n.y + kAirUp, (z + nz) * 0.5f, other))
                    {
                        m_how = 'a';
                        Visit(n.i + di2, n.j + dj2, y, other);
                    }
                }
            }
        }
    }
};

/// Where the lantern is set down in the west (tools/layout/city_model.py build_scaffold): its middle platform.
Vec3 g_scaffold{};

void Report(const Solver& s, const char* stage)
{
    std::printf("  %-34s %7zu cells;", stage, s.Size());
    for (const town::Place& p : Town().places)
    {
        if (s.Reached(p.at, 2.5f))
        {
            std::printf(" [%s]", p.name.c_str());
        }
    }
    std::printf("\n");
    std::fflush(stdout);
}

void TestRoute()
{
    const town::TownData& t = Town();
    Solver s(t);
    const char* order[] = {"sea gate", "canal crate", "day bell", "plank", "dawn bell", "gatehouse crate", "night bell"};
    int ids[7];
    for (int k = 0; k < 7; ++k)
    {
        ids[k] = s.Find(order[k]);
        CHECK(ids[k] >= 0);
        if (ids[k] < 0)
        {
            std::printf("  no thing %s\n", order[k]);
            return;
        }
    }
    for (int k = 0; k < 7; ++k)
    {
        // the lantern: set down on the scaffold of the west once the canal is crossed (the day bell is beyond it)
        if (k >= 2)
        {
            s.SetLantern(g_scaffold);
        }
        s.Flood(t.spawn, t.startLayer);
        Report(s, order[k]);
        const bool reached = s.HandleReached(ids[k]);
        CHECK(reached);
        if (!reached)
        {
            std::printf("  not reached: %s\n", order[k]);
            return;
        }
        // what is reached before its turn: a way round a puzzle (listed; the count is checked at the end)
        for (int later = k + 1; later < 7; ++later)
        {
            if (s.HandleReached(ids[later]) && !g_early[later])
            {
                g_early[later] = true;
                std::printf("  way round: %s reached before %s is done\n", order[later], order[k]);
            }
        }
        s.Do(ids[k]);
    }
    s.Flood(t.spawn, t.startLayer);
    Report(s, "all done");
    // without the lantern the day bell is not reached
    Solver s2(t);
    s2.Do(ids[0]);
    s2.Do(ids[1]);
    s2.Flood(t.spawn, t.startLayer);
    CHECK(!s2.HandleReached(ids[2]));
    Report(s2, "no lantern");
}

} // namespace

int main(int argc, char** argv)
{
    // the scaffold's middle platform: given by the model's numbers (city_model.py SCAFFOLD_S, build_scaffold)
    const town::TownData& t = Town();
    for (const town::Street& st : t.streets)
    {
        if (st.name != "market street")
        {
            continue;
        }
        float done = 0.0f;
        for (std::size_t i = 0; i + 1 < st.line.size(); ++i)
        {
            const Vec3& a = st.line[i];
            const Vec3& b = st.line[i + 1];
            const float l = std::sqrt((b.x - a.x) * (b.x - a.x) + (b.z - a.z) * (b.z - a.z));
            if (done + l >= 12.0f && l > 0.0f)
            {
                const float f = (12.0f - done) / l; // u -2 of the scaffold (SCAFFOLD_S 14)
                const Vec3 p{a.x + (b.x - a.x) * f, a.y + (b.y - a.y) * f, a.z + (b.z - a.z) * f};
                const float vx = (b.x - a.x) / l, vz = (b.z - a.z) / l;
                const float nx = -vz, nz = vx;
                // u -2.0, x 0.9: the middle platform (3.2 m)
                g_scaffold = {p.x + nx * 0.9f, p.y + 3.2f, p.z + nz * 0.9f};
                break;
            }
            done += l;
        }
    }
    if (argc >= 5 && std::string(argv[1]) == "probe")
    {
        Solver s(Town());
        for (int k = 5; k < argc; ++k)
        {
            s.Do(std::atoi(argv[k]));
        }
        s.Probe(static_cast<float>(std::atof(argv[2])), static_cast<float>(std::atof(argv[3])), static_cast<float>(std::atof(argv[4])));
        return 0;
    }
    if (argc >= 4)
    {
        // a way: test_route x y z [things done, by number]
        Solver s(Town());
        for (int k = 4; k < argc; ++k)
        {
            if (argv[k][0] == 'L')
            {
                s.SetLantern(g_scaffold);
            }
            else
            {
                s.Do(std::atoi(argv[k]));
            }
        }
        s.Flood(Town().spawn, Town().startLayer);
        Report(s, "way");
        for (std::size_t k = 0; k < Town().things.size(); ++k)
        {
            std::printf("  handle %s: %d\n", Town().things[k].name.c_str(), s.HandleReached(static_cast<int>(k)) ? 1 : 0);
        }
        s.Way({static_cast<float>(std::atof(argv[1])), static_cast<float>(std::atof(argv[2])), static_cast<float>(std::atof(argv[3]))});
        return 0;
    }
    (void)argc;
    (void)argv;
    TestRoute();
    std::printf("%d checks passed, %d failed\n", g_passed, g_failed);
    return g_failed == 0 ? 0 : 1;
}
