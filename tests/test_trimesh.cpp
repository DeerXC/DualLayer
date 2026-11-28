// test_trimesh.cpp - tests of the solid of triangles (no window, no GPU, no town file): src/scene/TriMesh.h.
//   g++ -std=c++20 -O2 -Wall -Wextra -I src tests/test_trimesh.cpp -o test_trimesh -pthread
//   MSVC (Developer PowerShell):  .\tests\run_tests.ps1 -Only trimesh
// The grid and the boxes of TriMesh make a query fast; here every answer is compared with a search that has neither: a
// loop over all the triangles. What is checked, on meshes of random triangles (small, large and tilted, so that many of
// them are filed in several cells) and on a floor of shared edges:
//   1. a ray down (HighestBelow): the same height as the loop, for every range, hit or miss
//   2. a ball (PushOut): the same place as the loop, to 1e-5 m; nothing moves when nothing is touched
//   3. a triangle is met once by a query that looks at several cells, and every triangle whose box is in range is met
//   4. a ball query leaves out no triangle that the ball touches (the box test never throws away a real contact)
//   5. several threads asking the same mesh at once get the answers they get alone (a query writes nothing)
//   6. an empty mesh, a mesh of one triangle, and a triangle with no area
// Returns 1 when a check fails.

#include "check.h"
#include "scene/TriMesh.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <random>
#include <thread>
#include <vector>

namespace
{

using dl::Vec3;
using dl::town::TriMesh;
namespace tri = dl::tri;

struct T
{
    Vec3 a, b, c;
};

float Near(float a, float b)
{
    return std::fabs(a - b);
}

/// A mesh of n random triangles over 120 x 120 m: most are 1 to 6 m across, one in ten is 20 to 45 m, so that they are
/// filed in one cell and in many.
std::vector<T> Soup(unsigned seed, int n)
{
    std::mt19937 rng(seed);
    const auto u = [&](float lo, float hi) { return std::uniform_real_distribution<float>(lo, hi)(rng); };
    std::vector<T> out;
    for (int i = 0; i < n; ++i)
    {
        const float size = (i % 10 == 0) ? u(20.0f, 45.0f) : u(1.0f, 6.0f);
        const Vec3 o{u(-60.0f, 60.0f), u(-5.0f, 15.0f), u(-60.0f, 60.0f)};
        const auto p = [&]
        { return Vec3{o.x + u(-size, size), o.y + u(-size * 0.4f, size * 0.4f), o.z + u(-size, size)}; };
        out.push_back({p(), p(), p()});
    }
    return out;
}

/// A flat floor of 2 x 2 m quads, each of two triangles that share edges and corners, with a small step here and there.
std::vector<T> Floor()
{
    std::vector<T> out;
    for (int j = -20; j < 20; ++j)
    {
        for (int i = -20; i < 20; ++i)
        {
            const float x = static_cast<float>(i) * 2.0f;
            const float z = static_cast<float>(j) * 2.0f;
            const float y = ((i + j) % 7 == 0) ? 0.3f : 0.0f;
            out.push_back({{x, y, z}, {x, y, z + 2.0f}, {x + 2.0f, y, z + 2.0f}});
            out.push_back({{x, y, z}, {x + 2.0f, y, z + 2.0f}, {x + 2.0f, y, z}});
        }
    }
    return out;
}

TriMesh Build(const std::vector<T>& tris)
{
    TriMesh m;
    for (const T& t : tris)
    {
        m.Add(t.a, t.b, t.c);
    }
    m.Build();
    return m;
}

/// The highest height in [y0, y1] over (x, z): the loop over every triangle.
bool RefHighest(const std::vector<T>& tris, float x, float z, float y1, float y0, float& y)
{
    bool found = false;
    for (const T& t : tris)
    {
        float h = 0.0f;
        if (TriMesh::HeightOn(t.a, t.b, t.c, x, z, h) && h <= y1 && h >= y0 && (!found || h > y))
        {
            y = h;
            found = true;
        }
    }
    return found;
}

/// PushOut over every triangle: the same rounds and the same rule as TriMesh::PushOut, with no grid and no box.
float RefPush(const std::vector<T>& tris, Vec3& c, float r)
{
    const Vec3 start = c;
    for (int round = 0; round < 4; ++round)
    {
        Vec3 best = c;
        Vec3 bestNormal{0.0f, 1.0f, 0.0f};
        float bestD2 = r * r;
        bool hit = false;
        for (const T& t : tris)
        {
            const Vec3 q = tri::ClosestOnTriangle(c, t.a, t.b, t.c);
            const Vec3 d = tri::Sub(c, q);
            const float d2 = tri::Dot(d, d);
            if (d2 < bestD2)
            {
                bestD2 = d2;
                best = q;
                bestNormal = tri::Cross(tri::Sub(t.b, t.a), tri::Sub(t.c, t.a));
                hit = true;
            }
        }
        if (!hit)
        {
            break;
        }
        const Vec3 d = tri::Sub(c, best);
        const float dist = std::sqrt(bestD2);
        if (dist > 1.0e-6f)
        {
            c = {best.x + d.x * (r / dist), best.y + d.y * (r / dist), best.z + d.z * (r / dist)};
        }
        else
        {
            const float l = std::sqrt(tri::Dot(bestNormal, bestNormal));
            Vec3 n = l > 0.0f ? Vec3{bestNormal.x / l, bestNormal.y / l, bestNormal.z / l} : Vec3{0.0f, 1.0f, 0.0f};
            if (n.y < 0.0f)
            {
                n = {-n.x, -n.y, -n.z};
            }
            c = {best.x + n.x * r, best.y + n.y * r, best.z + n.z * r};
        }
    }
    const Vec3 moved = tri::Sub(c, start);
    return std::sqrt(tri::Dot(moved, moved));
}

void TestRay(const std::vector<T>& tris, const TriMesh& mesh, unsigned seed, int count)
{
    std::mt19937 rng(seed);
    const auto u = [&](float lo, float hi) { return std::uniform_real_distribution<float>(lo, hi)(rng); };
    int mismatches = 0;
    int hits = 0;
    for (int k = 0; k < count; ++k)
    {
        const float x = u(-70.0f, 70.0f);
        const float z = u(-70.0f, 70.0f);
        const float y1 = u(-6.0f, 16.0f);
        const float y0 = y1 - u(0.2f, 12.0f);
        float a = 0.0f;
        float b = 0.0f;
        const bool fa = mesh.HighestBelow(x, z, y1, y0, a);
        const bool fb = RefHighest(tris, x, z, y1, y0, b);
        hits += fb ? 1 : 0;
        mismatches += (fa != fb || (fa && a != b)) ? 1 : 0;
    }
    std::printf("  ray: %d queries, %d hit, %d differ\n", count, hits, mismatches);
    CHECK(mismatches == 0);
    CHECK(hits > count / 20); // the test has hits to compare, not only misses
}

void TestBall(const std::vector<T>& tris, const TriMesh& mesh, unsigned seed, int count, float yLow, float yHigh)
{
    std::mt19937 rng(seed);
    const auto u = [&](float lo, float hi) { return std::uniform_real_distribution<float>(lo, hi)(rng); };
    int mismatches = 0;
    int pushed = 0;
    int untouched = 0;
    for (int k = 0; k < count; ++k)
    {
        const float r = u(0.1f, 1.2f);
        Vec3 a{u(-65.0f, 65.0f), u(yLow, yHigh), u(-65.0f, 65.0f)};
        Vec3 b = a;
        const float da = mesh.PushOut(a, r);
        const float db = RefPush(tris, b, r);
        if (db > 0.0f)
        {
            ++pushed;
            mismatches += (Near(a.x, b.x) > 1.0e-4f || Near(a.y, b.y) > 1.0e-4f || Near(a.z, b.z) > 1.0e-4f) ? 1 : 0;
        }
        else
        {
            ++untouched;
            mismatches += (da != 0.0f) ? 1 : 0; // nothing near: nothing moves
        }
    }
    std::printf("  ball: %d queries, %d pushed, %d untouched, %d differ\n", count, pushed, untouched, mismatches);
    CHECK(mismatches == 0);
    CHECK(pushed > count / 10 && untouched > count / 10);
}

/// A triangle's identity for counting: its first corner (the soup has no two triangles with the same one).
long long Key(const Vec3& a)
{
    const long long x = std::llround(a.x * 1000.0f);
    const long long y = std::llround(a.y * 1000.0f);
    const long long z = std::llround(a.z * 1000.0f);
    return (x * 1000003LL + y) * 1000003LL + z;
}

void TestOnce(const std::vector<T>& tris, const TriMesh& mesh, unsigned seed, int count)
{
    std::mt19937 rng(seed);
    const auto u = [&](float lo, float hi) { return std::uniform_real_distribution<float>(lo, hi)(rng); };
    int twice = 0;
    int missed = 0;
    int stray = 0;
    for (int k = 0; k < count; ++k)
    {
        const float x = u(-60.0f, 60.0f);
        const float z = u(-60.0f, 60.0f);
        const float r = u(0.5f, 14.0f);
        std::vector<long long> met;
        mesh.ForEachNear(x, z, r, [&](const Vec3& a, const Vec3&, const Vec3&) { met.push_back(Key(a)); });
        std::vector<long long> sorted = met;
        std::sort(sorted.begin(), sorted.end());
        twice += std::adjacent_find(sorted.begin(), sorted.end()) != sorted.end() ? 1 : 0;
        for (const T& t : tris)
        {
            const float lox = std::min({t.a.x, t.b.x, t.c.x});
            const float hix = std::max({t.a.x, t.b.x, t.c.x});
            const float loz = std::min({t.a.z, t.b.z, t.c.z});
            const float hiz = std::max({t.a.z, t.b.z, t.c.z});
            const bool inRange = hix >= x - r && lox <= x + r && hiz >= z - r && loz <= z + r;
            const bool inside = std::binary_search(sorted.begin(), sorted.end(), Key(t.a));
            missed += (inRange && !inside) ? 1 : 0; // every triangle whose box is in range is met
            // ... and no triangle is met that is more than a cell and a half away from the range
            const float far = TriMesh::kCell * 1.5f;
            const bool near = hix >= x - r - far && lox <= x + r + far && hiz >= z - r - far && loz <= z + r + far;
            stray += (inside && !near) ? 1 : 0;
        }
    }
    std::printf("  once: %d queries, %d with a triangle met twice, %d missed, %d stray\n", count, twice, missed, stray);
    CHECK(twice == 0 && missed == 0 && stray == 0);
}

void TestBallLeavesOutNothing(const std::vector<T>& tris, const TriMesh& mesh, unsigned seed, int count)
{
    std::mt19937 rng(seed);
    const auto u = [&](float lo, float hi) { return std::uniform_real_distribution<float>(lo, hi)(rng); };
    int lost = 0;
    int touching = 0;
    for (int k = 0; k < count; ++k)
    {
        const Vec3 c{u(-60.0f, 60.0f), u(-5.0f, 15.0f), u(-60.0f, 60.0f)};
        const float r = u(0.1f, 2.0f);
        const float limit = r * r;
        std::vector<long long> met;
        mesh.ForEachNearBall(c, r, limit, [&](const Vec3& a, const Vec3&, const Vec3&) { met.push_back(Key(a)); });
        std::sort(met.begin(), met.end());
        for (const T& t : tris)
        {
            const Vec3 q = tri::ClosestOnTriangle(c, t.a, t.b, t.c);
            const Vec3 d = tri::Sub(c, q);
            if (tri::Dot(d, d) < limit - 1.0e-4f) // the ball touches the triangle
            {
                ++touching;
                lost += std::binary_search(met.begin(), met.end(), Key(t.a)) ? 0 : 1;
            }
        }
    }
    std::printf("  ball query: %d contacts, %d lost\n", touching, lost);
    CHECK(lost == 0 && touching > 100);
}

void TestThreads(const std::vector<T>& tris, const TriMesh& mesh)
{
    constexpr int kThreads = 4;
    constexpr int kQueries = 6000;
    struct Answer
    {
        bool found;
        float y;
        Vec3 c;
    };
    const auto ask = [&](int thread, std::vector<Answer>& out)
    {
        std::mt19937 rng(100u + static_cast<unsigned>(thread));
        const auto u = [&](float lo, float hi) { return std::uniform_real_distribution<float>(lo, hi)(rng); };
        for (int k = 0; k < kQueries; ++k)
        {
            Answer a{};
            const float x = u(-60.0f, 60.0f);
            const float z = u(-60.0f, 60.0f);
            a.found = mesh.HighestBelow(x, z, 16.0f, -6.0f, a.y);
            a.c = {x, u(-5.0f, 15.0f), z};
            mesh.PushOut(a.c, u(0.2f, 1.0f));
            out.push_back(a);
        }
    };
    std::vector<std::vector<Answer>> alone(kThreads);
    for (int t = 0; t < kThreads; ++t)
    {
        ask(t, alone[static_cast<std::size_t>(t)]);
    }
    std::vector<std::vector<Answer>> together(kThreads);
    std::vector<std::thread> pool;
    for (int t = 0; t < kThreads; ++t)
    {
        pool.emplace_back([&, t] { ask(t, together[static_cast<std::size_t>(t)]); });
    }
    for (std::thread& th : pool)
    {
        th.join();
    }
    int differ = 0;
    for (int t = 0; t < kThreads; ++t)
    {
        const auto& x = alone[static_cast<std::size_t>(t)];
        const auto& y = together[static_cast<std::size_t>(t)];
        for (std::size_t k = 0; k < x.size(); ++k)
        {
            differ += (x[k].found != y[k].found || (x[k].found && x[k].y != y[k].y) || x[k].c.x != y[k].c.x ||
                       x[k].c.y != y[k].c.y || x[k].c.z != y[k].c.z)
                          ? 1
                          : 0;
        }
    }
    (void)tris;
    std::printf("  threads: %d x %d queries together and alone, %d differ\n", kThreads, kQueries, differ);
    CHECK(differ == 0);
}

void TestSmall()
{
    TriMesh empty;
    empty.Build();
    float y = 0.0f;
    Vec3 c{1.0f, 2.0f, 3.0f};
    CHECK(!empty.HighestBelow(0.0f, 0.0f, 10.0f, -10.0f, y) && empty.PushOut(c, 0.5f) == 0.0f);
    CHECK(empty.TriangleCount() == 0);

    TriMesh one;
    one.Add({0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 4.0f}, {4.0f, 0.0f, 0.0f});
    one.Build();
    Vec3 on{1.0f, 0.1f, 1.0f};
    CHECK(one.TriangleCount() == 1 && one.HighestBelow(1.0f, 1.0f, 1.0f, -1.0f, y) && Near(y, 0.0f) < 1.0e-6f);
    CHECK(!one.HighestBelow(3.5f, 3.5f, 1.0f, -1.0f, y)); // over the empty corner of the square
    CHECK(one.PushOut(on, 0.35f) > 0.2f && Near(on.y, 0.35f) < 1.0e-5f);
    Vec3 far{100.0f, 0.1f, 100.0f}; // far outside the grid: the cells at its edge are looked at, and nothing is touched
    CHECK(!one.HighestBelow(100.0f, 100.0f, 1.0f, -1.0f, y) && one.PushOut(far, 0.35f) == 0.0f);

    TriMesh flat; // a triangle of no area is left out
    flat.Add({0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {2.0f, 0.0f, 0.0f});
    flat.Build();
    CHECK(flat.TriangleCount() == 0);

    TriMesh bad; // a ball that is not a number or has no size moves nothing
    bad.Add({0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 4.0f}, {4.0f, 0.0f, 0.0f});
    bad.Build();
    Vec3 nan{std::nanf(""), 0.0f, 0.0f};
    Vec3 zero{1.0f, 0.0f, 1.0f};
    CHECK(bad.PushOut(nan, 0.35f) == 0.0f && bad.PushOut(zero, 0.0f) == 0.0f);
}

} // namespace

int main()
{
    TestSmall();
    const std::vector<T> soup = Soup(1u, 700);
    const TriMesh soupMesh = Build(soup);
    const std::vector<T> floor = Floor();
    const TriMesh floorMesh = Build(floor);
    std::printf("soup: %d triangles; floor: %d triangles\n", soupMesh.TriangleCount(), floorMesh.TriangleCount());
    TestRay(soup, soupMesh, 11u, 12000);
    TestRay(floor, floorMesh, 12u, 6000);
    TestBall(soup, soupMesh, 21u, 9000, -6.0f, 16.0f);
    TestBall(floor, floorMesh, 22u, 4000, -0.5f, 0.9f);
    TestOnce(soup, soupMesh, 31u, 150);
    TestBallLeavesOutNothing(soup, soupMesh, 41u, 600);
    TestThreads(soup, soupMesh);
    std::printf("%d checks passed, %d failed\n", g_passed, g_failed);
    return g_failed == 0 ? 0 : 1;
}
