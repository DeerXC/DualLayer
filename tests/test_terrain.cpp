// test_terrain.cpp - tests of the height field (no window, no GPU): src/scene/Terrain.h, with the real walking body
// (src/scene/Player.h).
//   g++ -std=c++20 -O2 -Wall -Wextra -I src tests/test_terrain.cpp -o test_terrain
//   MSVC (Developer PowerShell):  .\tests\run_tests.ps1 -Only terrain
// What is checked:
//   1. noise: the same numbers every time, in range
//   2. a field: exact at its vertices, continuous across every edge and diagonal, every triangle clockwise seen from
//   above
//   3. collision: a ball is pushed out to its radius from flat ground, from a slope, and from under the surface; the
//   walking
//      body stands and walks up a gentle slope and slides off one too steep to stand on
// Returns 1 when a check fails.

#include "scene/Player.h"
#include "scene/Terrain.h"

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

using dl::Player;
using dl::Vec3;
using namespace dl::scene;
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
        inRange = inRange && a >= -1.0f && a <= 1.0f && r >= 0.0f && r <= 1.0f && tr::ValueNoise(x, z, 5u) >= -1.0f &&
                  tr::ValueNoise(x, z, 5u) <= 1.0f;
    }
    CHECK(same && inRange);
    CHECK(tr::SmoothStep(0.0f, 1.0f, -1.0f) == 0.0f && tr::SmoothStep(0.0f, 1.0f, 2.0f) == 1.0f &&
          Near(tr::SmoothStep(1.0f, 0.0f, 0.25f), 0.84375f, 1e-5f));
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
            continuous = continuous && std::fabs(f.Height(x + e, z) - f.Height(x - e, z)) < 0.01f &&
                         std::fabs(f.Height(x, z + e) - f.Height(x, z - e)) < 0.01f;
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
                up = up && Near(f.Height((a.x + b.x + c.x) / 3.0f, (a.z + b.z + c.z) / 3.0f), (a.y + b.y + c.y) / 3.0f,
                                1e-4f);
            }
        }
    }
    CHECK(up);
}

/// A land of one field only, made by `height`.
template <class F> tr::Land TestLandOf(F height)
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

} // namespace

int main()
{
    TestNoise();
    TestField();
    TestCollision();
    std::printf("%d checks passed, %d failed\n", g_passed, g_failed);
    return g_failed == 0 ? 0 : 1;
}
