// test_mesh.cpp - are the shapes of the models right? No window, no GPU: only src/render/MeshData.cpp.
//   g++ -std=c++20 -Wall -Wextra -I src tests/test_mesh.cpp src/render/MeshData.cpp -o test_mesh
//   MSVC (Developer PowerShell):  .\tests\run_tests.ps1
// What is checked, for the cube and for a coarse and a fine number of segments of the round shapes:
//   1. the indices are in range, a triangle has three of them, and no triangle has no area
//   2. every normal has length 1
//   3. WINDING: the outward normal of a triangle, cross(v1 - v0, v2 - v0) (component formula), points the way of the
//      normals of its vertices. This is the rule "clockwise seen from outside" of docs/CONVENTIONS.md as a number: a
//      face that is wound the wrong way is invisible on the screen (back-face culling)
//   4. the surface is CLOSED and turned the same way everywhere: every edge belongs to two triangles that run in
//      opposite directions
//   5. the signed volume (the sum of v0 . (v1 x v2) / 6) is positive and close to the real volume of the shape: it is
//      negative when the shape is inside out
// Returns 1 when a check fails.
#include "render/MeshData.h"

#include <cmath>
#include <cstdio>
#include <map>
#include <tuple>
#include <utility>

using namespace dl;

static int g_Failures = 0;

static void Check(bool ok, const char* what)
{
    std::printf("  %-4s %s\n", ok ? "ok" : "FAIL", what);
    if (!ok)
    {
        ++g_Failures;
    }
}

struct V3
{
    float x, y, z;
};

static V3 Sub(V3 a, V3 b)
{
    return {a.x - b.x, a.y - b.y, a.z - b.z};
}
static V3 Cross(V3 a, V3 b)
{
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}
static float Dot(V3 a, V3 b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}
static float Length(V3 a)
{
    return std::sqrt(Dot(a, a));
}
static V3 Position(const MeshData& m, std::uint32_t i)
{
    return {m.vertices[i].position[0], m.vertices[i].position[1], m.vertices[i].position[2]};
}
static V3 Normal(const MeshData& m, std::uint32_t i)
{
    return {m.vertices[i].normal[0], m.vertices[i].normal[1], m.vertices[i].normal[2]};
}

using Key = std::tuple<long, long, long>;
static Key Weld(V3 p)
{
    // two vertices at the same place (the seam of a sphere, the corners of a cube) are the same point of the surface
    return {std::lround(p.x * 2000.0f), std::lround(p.y * 2000.0f), std::lround(p.z * 2000.0f)};
}

struct Report
{
    std::size_t vertices = 0, triangles = 0;
    float volume = 0.0f;
    bool indicesOk = true, normalsOk = true, areaOk = true, windingOk = true, closedOk = true;
    float worstWinding = 1.0f;
    int degenerate = 0;
};

static Report Analyse(const MeshData& m, bool convex)
{
    Report r;
    r.vertices = m.vertices.size();
    r.triangles = m.TriangleCount();
    r.indicesOk = m.indices.size() % 3 == 0;
    for (std::uint32_t i : m.indices)
    {
        r.indicesOk &= i < m.vertices.size();
    }
    if (!r.indicesOk)
    {
        return r;
    }
    for (std::uint32_t i = 0; i < m.vertices.size(); ++i)
    {
        r.normalsOk &= std::fabs(Length(Normal(m, i)) - 1.0f) < 1.0e-4f;
    }
    std::map<std::pair<Key, Key>, int> directed;
    for (std::size_t t = 0; t < m.TriangleCount(); ++t)
    {
        const std::uint32_t i0 = m.indices[3 * t], i1 = m.indices[3 * t + 1], i2 = m.indices[3 * t + 2];
        const V3 p0 = Position(m, i0), p1 = Position(m, i1), p2 = Position(m, i2);
        const V3 n = Cross(Sub(p1, p0), Sub(p2, p0));
        const float area2 = Length(n);
        if (area2 < 1.0e-9f)
        {
            ++r.degenerate;
            r.areaOk = false;
            continue;
        }
        const V3 unit = {n.x / area2, n.y / area2, n.z / area2};
        // the normals of its three vertices, averaged
        const V3 a = Normal(m, i0), b = Normal(m, i1), c = Normal(m, i2);
        V3 avg = {a.x + b.x + c.x, a.y + b.y + c.y, a.z + b.z + c.z};
        avg = {avg.x / Length(avg), avg.y / Length(avg), avg.z / Length(avg)};
        const float agree = Dot(unit, avg);
        r.worstWinding = std::fmin(r.worstWinding, agree);
        r.windingOk &= agree > 0.5f;
        if (convex) // a convex shape round the origin: the face must point away from it
        {
            const V3 centre = {(p0.x + p1.x + p2.x) / 3.0f, (p0.y + p1.y + p2.y) / 3.0f, (p0.z + p1.z + p2.z) / 3.0f};
            r.windingOk &= Dot(unit, centre) > 0.0f;
        }
        r.volume += Dot(p0, Cross(p1, p2)) / 6.0f;
        directed[{Weld(p0), Weld(p1)}] += 1;
        directed[{Weld(p1), Weld(p2)}] += 1;
        directed[{Weld(p2), Weld(p0)}] += 1;
    }
    for (const auto& [edge, count] : directed)
    {
        const auto reverse = directed.find({edge.second, edge.first});
        r.closedOk &= count == 1 && reverse != directed.end() && reverse->second == 1;
    }
    return r;
}

static void Show(const char* name, const Report& r, float expectedVolume, float tolerance)
{
    std::printf("%s: %zu vertices, %zu triangles, volume %.4f (the real shape: %.4f), worst facing %.3f\n", name,
                r.vertices, r.triangles, r.volume, expectedVolume, r.worstWinding);
    Check(r.indicesOk, "the indices are in range");
    Check(r.normalsOk, "every normal has length 1");
    Check(r.areaOk, "no triangle without area");
    Check(r.windingOk, "WINDING: every triangle is clockwise seen from outside (its normal points the way of the "
                       "normals of its vertices)");
    Check(r.closedOk,
          "the surface is closed and turned the same way everywhere (every edge: two triangles, opposite directions)");
    // a polygon drawn INSIDE a circle is smaller than the circle: a coarse sphere or torus has LESS volume than the
    // real shape, never more (and the more segments, the closer)
    Check(r.volume > 0.0f && r.volume < expectedVolume * 1.001f && r.volume > (1.0f - tolerance) * expectedVolume,
          "the signed volume is positive, not more than the real volume, and close to it (not inside out)");
}

int main()
{
    constexpr float kPi = 3.14159265358979323846f;

    std::printf("CUBE\n");
    {
        const MeshData cube = MakeCube();
        Check(cube.vertices.size() == 24 && cube.indices.size() == 36,
              "24 vertices (a corner belongs to three faces: three normals) and 36 indices");
        Show("cube", Analyse(cube, true), 1.0f, 1.0e-4f);
        bool inside = true;
        for (const MeshVertex& v : cube.vertices)
        {
            inside &= std::fabs(std::fabs(v.position[0]) - 0.5f) < 1.0e-6f &&
                      std::fabs(std::fabs(v.position[1]) - 0.5f) < 1.0e-6f &&
                      std::fabs(std::fabs(v.position[2]) - 0.5f) < 1.0e-6f;
        }
        Check(inside, "every corner is at (+-0.5, +-0.5, +-0.5): an edge of length 1, centred");
    }

    for (int segments : {8, 32, 64})
    {
        std::printf("SPHERE, %d segments\n", segments);
        const MeshData sphere = MakeSphere(segments);
        const float real = 4.0f / 3.0f * kPi * 0.125f;
        Show("sphere", Analyse(sphere, true), real, segments >= 32 ? 0.03f : 0.45f);
        float farthest = 0.0f;
        for (std::uint32_t i = 0; i < sphere.vertices.size(); ++i)
        {
            farthest = std::fmax(farthest, std::fabs(Length(Position(sphere, i)) - 0.5f));
        }
        Check(farthest < 1.0e-5f, "every vertex is on the sphere of radius 0.5");
    }

    for (int segments : {3, 8, 32, 64})
    {
        std::printf("CYLINDER, %d segments\n", segments);
        const MeshData cylinder = MakeCylinder(segments);
        // the volume of a polygon prism with `segments` sides of circumradius 0.5 and height 1: n/2 * r^2 * sin(2 pi /
        // n)
        const float real =
            0.5f * static_cast<float>(segments) * 0.25f * std::sin(2.0f * kPi / static_cast<float>(segments));
        Show("cylinder", Analyse(cylinder, true), real, 1.0e-3f);
    }

    for (int segments : {8, 32, 64})
    {
        std::printf("TORUS, %d segments\n", segments);
        const float major = 0.8f, minor = 0.3f;
        const MeshData torus = MakeTorus(segments, major, minor);
        const float real = 2.0f * kPi * kPi * major * minor * minor;
        Show("torus", Analyse(torus, false), real, segments >= 32 ? 0.04f : 0.45f);
    }

    std::printf("clamping\n");
    {
        Check(!MakeSphere(-5).vertices.empty() && !MakeSphere(100000).vertices.empty(),
              "a silly number of segments of a sphere is clamped, not followed");
        Check(!MakeCylinder(0).indices.empty() && !MakeCylinder(100000).indices.empty(), "... a cylinder");
        Check(!MakeTorus(1, 1.0f, 0.2f).indices.empty() && !MakeTorus(100000, 1.0f, 0.2f).indices.empty(),
              "... a torus");
        Check(MakeSphere(7).vertices.size() == MakeSphere(8).vertices.size(),
              "a sphere of an odd number of segments gets the next even one");
    }

    std::printf(g_Failures == 0 ? "all checks passed\n" : "%d CHECKS FAILED\n", g_Failures);
    return g_Failures == 0 ? 0 : 1;
}
