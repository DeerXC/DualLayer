// test_trimath.cpp - tests of the maths of a ball against a triangle (no window, no GPU): src/scene/TriMath.h.
//   g++ -std=c++20 -O2 -Wall -Wextra -I src tests/test_trimath.cpp -o test_trimath
//   MSVC (Developer PowerShell):  .\tests\run_tests.ps1 -Only trimath
// What is checked: the vector operations; the closest point of a triangle to a point in each of the seven regions (over
// the face, past each of the three edges, beyond each of the three corners), and on a long series of points: it lies in
// the triangle, and no point of a fine sampling of the triangle is closer to the point. Returns 1 when a check fails.

#include "scene/TriMath.h"

#include <cmath>
#include <cstdio>

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
namespace tri = dl::tri;

bool Near(const Vec3& a, const Vec3& b, float eps = 1e-5f)
{
    return std::fabs(a.x - b.x) <= eps && std::fabs(a.y - b.y) <= eps && std::fabs(a.z - b.z) <= eps;
}

float Distance(const Vec3& a, const Vec3& b)
{
    const Vec3 d = tri::Sub(a, b);
    return std::sqrt(tri::Dot(d, d));
}

void TestVectors()
{
    CHECK(Near(tri::Sub({3, 2, 1}, {1, 1, 1}), {2, 1, 0}));
    CHECK(tri::Dot({1, 2, 3}, {4, 5, 6}) == 32.0f);
    CHECK(Near(tri::Cross({1, 0, 0}, {0, 1, 0}), {0, 0, 1}) && Near(tri::Cross({0, 1, 0}, {1, 0, 0}), {0, 0, -1}));
    CHECK(tri::Dot(tri::Cross({1, 2, 3}, {4, 5, 6}), {1, 2, 3}) == 0.0f); // at a right angle to both
}

void TestRegions()
{
    const Vec3 a{0, 0, 0};
    const Vec3 b{4, 0, 0};
    const Vec3 c{0, 0, 4};
    CHECK(Near(tri::ClosestOnTriangle({1, 2, 1}, a, b, c), {1, 0, 1}));  // over the face
    CHECK(Near(tri::ClosestOnTriangle({1, -2, 1}, a, b, c), {1, 0, 1})); // under it
    CHECK(Near(tri::ClosestOnTriangle({2, 1, -3}, a, b, c), {2, 0, 0})); // past the edge ab
    CHECK(Near(tri::ClosestOnTriangle({-3, 1, 2}, a, b, c), {0, 0, 2})); // past the edge ac
    CHECK(Near(tri::ClosestOnTriangle({4, 1, 4}, a, b, c), {2, 0, 2}));  // past the edge bc
    CHECK(Near(tri::ClosestOnTriangle({-1, 1, -1}, a, b, c), a));        // beyond the corner a
    CHECK(Near(tri::ClosestOnTriangle({7, 0, -2}, a, b, c), b));         // beyond b
    CHECK(Near(tri::ClosestOnTriangle({-2, 3, 8}, a, b, c), c));         // beyond c
    CHECK(Near(tri::ClosestOnTriangle({1, 0, 1}, a, b, c), {1, 0, 1}));  // on the face: itself
    CHECK(Near(tri::ClosestOnTriangle(b, a, b, c), b));                  // a corner: itself
}

void TestSeries()
{
    unsigned state = 99u;
    const auto next = [&state]()
    {
        state = state * 1664525u + 1013904223u;
        return static_cast<float>((state >> 8) & 0xFFFFu) / 65535.0f;
    };
    int inside = 0;
    int nearest = 0;
    const int n = 300;
    for (int i = 0; i < n; ++i)
    {
        const Vec3 a{-3.0f + 6.0f * next(), -3.0f + 6.0f * next(), -3.0f + 6.0f * next()};
        const Vec3 b{-3.0f + 6.0f * next(), -3.0f + 6.0f * next(), -3.0f + 6.0f * next()};
        const Vec3 c{-3.0f + 6.0f * next(), -3.0f + 6.0f * next(), -3.0f + 6.0f * next()};
        const Vec3 p{-6.0f + 12.0f * next(), -6.0f + 12.0f * next(), -6.0f + 12.0f * next()};
        const Vec3 q = tri::ClosestOnTriangle(p, a, b, c);
        // q is in the triangle: it is a combination of the corners with weights 0 .. 1 that add up to 1
        const Vec3 normal = tri::Cross(tri::Sub(b, a), tri::Sub(c, a));
        const float area2 = std::sqrt(tri::Dot(normal, normal));
        if (area2 < 1e-3f)
        {
            ++inside;
            ++nearest;
            continue;
        }
        const float wa = tri::Dot(tri::Cross(tri::Sub(c, b), tri::Sub(q, b)), normal) / (area2 * area2);
        const float wb = tri::Dot(tri::Cross(tri::Sub(a, c), tri::Sub(q, c)), normal) / (area2 * area2);
        const float wc = 1.0f - wa - wb;
        const float eps = -1e-3f;
        const bool in = wa >= eps && wb >= eps && wc >= eps &&
                        std::fabs(tri::Dot(tri::Sub(q, a), normal)) / area2 < 1e-3f; // in the plane, too
        inside += in ? 1 : 0;
        // no point of a fine sampling of the triangle is closer than q
        float best = 1.0e9f;
        for (int u = 0; u <= 20; ++u)
        {
            for (int v = 0; v <= 20 - u; ++v)
            {
                const float fu = static_cast<float>(u) / 20.0f;
                const float fv = static_cast<float>(v) / 20.0f;
                const Vec3 s{a.x + (b.x - a.x) * fu + (c.x - a.x) * fv, a.y + (b.y - a.y) * fu + (c.y - a.y) * fv,
                             a.z + (b.z - a.z) * fu + (c.z - a.z) * fv};
                best = Distance(p, s) < best ? Distance(p, s) : best;
            }
        }
        nearest += Distance(p, q) <= best + 1e-4f ? 1 : 0;
    }
    CHECK(inside == n && nearest == n);
}

} // namespace

int main()
{
    TestVectors();
    TestRegions();
    TestSeries();
    std::printf("%d checks passed, %d failed\n", g_passed, g_failed);
    return g_failed == 0 ? 0 : 1;
}
