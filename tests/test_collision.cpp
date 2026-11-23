// test_collision.cpp - tests of the collision maths (no window, no GPU): src/scene/Collision.h and src/scene/SceneData.h.
//   g++ -std=c++20 -Wall -Wextra -I src tests/test_collision.cpp -o test_collision
//   MSVC (Developer PowerShell):  .\tests\run_tests.ps1
// Header-only: there is no .cpp to add to the project. What is checked:
//   1. the two rotations are the ones of DirectXMath (the facts that the renderer's world matrix depends on)
//   2. the layer rule: an object is solid only in its own layers; the floor is solid in both
//   3. every shape: a ball that starts inside, or touching, is pushed out to exactly its radius from the surface
//   4. NEVER INSIDE: many points, both layers, many moments of the turning; after ResolveSphere the ball is outside every
//      solid of the layer. The check measures the distance in a DIFFERENT way than the code does (box: projection on the
//      world axes; torus: sampling the circle of the tube), so a mistake in the inverse rotation cannot hide
//   5. a settled ball is not pushed again; a walk into a solid stops at it; the same walk in the other layer goes through
//   6. bad input (NaN, a layer that does not exist) does not change anything
// Returns 1 when a check fails.

#include "scene/Collision.h"

#include <cmath>
#include <cstdio>
#include <initializer_list>
#include <limits>

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
using namespace dl::scene;
constexpr float kPi = 3.14159265358979f;
constexpr float kBall = 0.35f; // UNITS: metres, the radius of the camera body

bool Near(float a, float b, float eps = 1e-4f)
{
    return std::fabs(a - b) <= eps;
}

bool Near3(const Vec3& a, const Vec3& b, float eps = 1e-4f)
{
    return Near(a.x, b.x, eps) && Near(a.y, b.y, eps) && Near(a.z, b.z, eps);
}

float Dot(const Vec3& a, const Vec3& b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

float Len(const Vec3& a)
{
    return std::sqrt(Dot(a, a));
}

Vec3 Sub(const Vec3& a, const Vec3& b)
{
    return {a.x - b.x, a.y - b.y, a.z - b.z};
}

const SceneObject& Find(ShapeKind shape, unsigned layerMask, bool floor = false)
{
    for (const SceneObject& o : kScene)
    {
        if (o.shape == shape && o.layerMask == layerMask && (o.checker > 0.5f) == floor)
        {
            return o;
        }
    }
    std::printf("the scene has no such object: the test needs an update\n");
    return kScene[0];
}

Vec3 Centre(const SceneObject& o)
{
    return {o.position[0], o.position[1], o.position[2]};
}

/// The world axes of an object, worked out by turning the unit vectors FORWARD (tilt, then turn): the code under test
/// goes the other way (backward), so comparing the two catches a wrong sign or a wrong order.
void WorldAxes(const SceneObject& o, float spin, Vec3 axes[3])
{
    const Vec3 unit[3] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
    for (int i = 0; i < 3; ++i)
    {
        axes[i] = dl::collision_detail::RotateY(dl::collision_detail::RotateX(unit[i], o.tilt), o.spinSpeed * spin);
    }
}

/// The distance from a point to the SURFACE of the solid (positive outside, 0 inside), measured independently.
float DistanceToSolid(const SceneObject& o, const Vec3& p, float spin)
{
    Vec3 axes[3];
    WorldAxes(o, spin, axes);
    const Vec3 d = Sub(p, Centre(o));
    const Vec3 local{Dot(d, axes[0]), Dot(d, axes[1]), Dot(d, axes[2])}; // by projection, not by turning back
    switch (o.shape)
    {
    case ShapeKind::Cube:
    {
        const Vec3 h{0.5f * o.scale[0], 0.5f * o.scale[1], 0.5f * o.scale[2]};
        const Vec3 out{std::fmax(std::fabs(local.x) - h.x, 0.0f), std::fmax(std::fabs(local.y) - h.y, 0.0f),
                       std::fmax(std::fabs(local.z) - h.z, 0.0f)};
        return Len(out);
    }
    case ShapeKind::Sphere:
        return std::fmax(Len(local) - 0.5f * o.scale[0], 0.0f);
    case ShapeKind::Cylinder:
    {
        const float R = 0.5f * o.scale[0];
        const float H = 0.5f * o.scale[1];
        const float rho = std::sqrt(local.x * local.x + local.z * local.z);
        const float dr = std::fmax(rho - R, 0.0f);
        const float dy = std::fmax(std::fabs(local.y) - H, 0.0f);
        return std::sqrt(dr * dr + dy * dy);
    }
    case ShapeKind::Torus:
    {
        // sample the circle that the tube is bent round: 1440 points, the error is far below the tolerance
        const float ringRadius = kTorusMajor * o.scale[0];
        float best = 1.0e9f;
        for (int i = 0; i < 1440; ++i)
        {
            const float a = 2.0f * kPi * static_cast<float>(i) / 1440.0f;
            const Vec3 core{ringRadius * std::cos(a), 0.0f, ringRadius * std::sin(a)};
            const float dist = Len(Sub(local, core));
            best = dist < best ? dist : best;
        }
        return std::fmax(best - kTorusMinor * o.scale[0], 0.0f);
    }
    case ShapeKind::Count:
        break;
    }
    return 1.0e9f;
}

/// The smallest distance from the point to any solid of the layer.
float DistanceToLayer(const Vec3& p, int layer, float spin)
{
    float best = 1.0e9f;
    for (const SceneObject& o : kScene)
    {
        if ((o.layerMask & (1u << layer)) != 0u)
        {
            const float d = DistanceToSolid(o, p, spin);
            best = d < best ? d : best;
        }
    }
    return best;
}

void TestRotations()
{
    // the facts about DirectXMath (left-handed): RotationY(+90) turns +Z into +X; RotationX(+90) turns +Y into +Z
    CHECK(Near3(dl::collision_detail::RotateY({0, 0, 1}, kPi / 2.0f), {1, 0, 0}));
    CHECK(Near3(dl::collision_detail::RotateY({1, 0, 0}, kPi / 2.0f), {0, 0, -1}));
    CHECK(Near3(dl::collision_detail::RotateX({0, 1, 0}, kPi / 2.0f), {0, 0, 1}));
    CHECK(Near3(dl::collision_detail::RotateX({0, 0, 1}, kPi / 2.0f), {0, -1, 0}));
    // going to the local space and back is the identity, for every object and some times
    for (const SceneObject& o : kScene)
    {
        for (float spin : {0.0f, 0.7f, 3.3f})
        {
            const Vec3 p{1.5f, -0.3f, 2.2f};
            CHECK(Near3(dl::collision_detail::ToWorld(o, dl::collision_detail::ToLocal(o, p, spin), spin), p, 1e-4f));
        }
    }
}

void TestScene()
{
    CHECK(SceneIsSupported());
    CHECK(kSceneCount == 5);
    // the floor is in both layers, the others in exactly one
    int both = 0;
    int onlyA = 0;
    int onlyB = 0;
    for (const SceneObject& o : kScene)
    {
        both += o.layerMask == kLayerBoth;
        onlyA += o.layerMask == kLayerA;
        onlyB += o.layerMask == kLayerB;
    }
    CHECK(both == 1 && onlyA == 2 && onlyB == 2);
}

void TestLayerRule()
{
    const SceneObject& cube = Find(ShapeKind::Cube, kLayerA);
    const SceneObject& sphere = Find(ShapeKind::Sphere, kLayerB);
    // the middle of the red cube: solid in A, empty in B
    const dl::CollisionResult inA = dl::ResolveSphere(Centre(cube), kBall, 0, 0.0f);
    CHECK(inA.pushed);
    CHECK(Len(Sub(inA.position, Centre(cube))) > 0.8f); // pushed out to the surface (the cube has half size 0.8)
    const dl::CollisionResult inB = dl::ResolveSphere(Centre(cube), kBall, 1, 0.0f);
    CHECK(!inB.pushed && Near3(inB.position, Centre(cube)));
    // the middle of the blue sphere: the other way round
    CHECK(dl::ResolveSphere(Centre(sphere), kBall, 1, 0.0f).pushed);
    CHECK(!dl::ResolveSphere(Centre(sphere), kBall, 0, 0.0f).pushed);
    // the floor: solid in both layers, and the ball is lifted onto it
    for (int layer = 0; layer < 2; ++layer)
    {
        const dl::CollisionResult r = dl::ResolveSphere({0.0f, kFloorY - 0.05f, 0.0f}, kBall, layer, 0.0f); // inside the slab
        CHECK(r.pushed && Near(r.position.y, kFloorY + kBall) && Near(r.position.x, 0.0f));
        const dl::CollisionResult above = dl::ResolveSphere({0.0f, kFloorY + 0.2f, 0.0f}, kBall, layer, 0.0f); // too low
        CHECK(above.pushed && Near(above.position.y, kFloorY + kBall));
        CHECK(!dl::ResolveSphere({0.0f, kFloorY + 1.0f, 0.0f}, kBall, layer, 0.0f).pushed); // clear
    }
    // the start position of the camera is free in both layers
    CHECK(!dl::ResolveSphere({0.0f, 0.0f, -3.0f}, kBall, 0, 0.0f).pushed);
    CHECK(!dl::ResolveSphere({0.0f, 0.0f, -3.0f}, kBall, 1, 0.0f).pushed);
}

void TestEachShape()
{
    // a ball that is placed 0.1 m inside the surface of each shape is pushed out to exactly kBall from the surface
    struct Case
    {
        ShapeKind shape;
        unsigned mask;
        int layer;
    };
    const Case cases[] = {{ShapeKind::Cube, kLayerA, 0},
                          {ShapeKind::Sphere, kLayerB, 1},
                          {ShapeKind::Cylinder, kLayerA, 0},
                          {ShapeKind::Torus, kLayerB, 1}};
    for (const Case& c : cases)
    {
        const SceneObject& o = Find(c.shape, c.mask);
        for (float spin : {0.0f, 1.9f})
        {
            Vec3 axes[3];
            WorldAxes(o, spin, axes);
            // start inside, 0.1 m under the surface along +local X (the cylinder and the cube have a face there; the
            // torus: on the tube, 0.2 m from the middle of the tube along the ring's own X axis)
            Vec3 start{};
            if (c.shape == ShapeKind::Torus)
            {
                const float rad = kTorusMajor * o.scale[0]; // the middle of the tube on the +X axis, then 0.2 m out
                start = Centre(o);
                const float along = rad + 0.2f;
                start = {start.x + axes[0].x * along, start.y + axes[0].y * along, start.z + axes[0].z * along};
            }
            else
            {
                const float outer = 0.5f * o.scale[0]; // the face of the cube / the side of the cylinder / the sphere, at +X
                const float along = outer - 0.1f;
                const Vec3 p = Centre(o);
                start = {p.x + axes[0].x * along, p.y + axes[0].y * along, p.z + axes[0].z * along};
            }
            CHECK(DistanceToSolid(o, start, spin) < 0.01f); // really inside (or on the surface)
            const dl::CollisionResult r = dl::ResolveSphere(start, kBall, c.layer, spin);
            CHECK(r.pushed);
            const float d = DistanceToSolid(o, r.position, spin);
            CHECK(Near(d, kBall, 2e-3f)); // the surface is exactly one radius away
        }
    }
    // the torus has a hole: a ball on the axis of the ring goes through without a push (hole radius 0.5 m > 0.35 m)
    const SceneObject& torus = Find(ShapeKind::Torus, kLayerB);
    for (float spin : {0.0f, 2.4f})
    {
        Vec3 axes[3];
        WorldAxes(torus, spin, axes);
        const Vec3 centre = Centre(torus);
        for (float t : {-0.2f, 0.0f, 0.2f})
        {
            const Vec3 p{centre.x + axes[1].x * t, centre.y + axes[1].y * t, centre.z + axes[1].z * t};
            CHECK(!dl::ResolveSphere(p, kBall, 1, spin).pushed);
        }
    }
    // the exact middle of a sphere and of the tube still get out (no division by zero)
    const SceneObject& sphere = Find(ShapeKind::Sphere, kLayerB);
    const dl::CollisionResult mid = dl::ResolveSphere(Centre(sphere), kBall, 1, 0.0f);
    CHECK(mid.pushed && std::isfinite(mid.position.y) && Near(DistanceToSolid(sphere, mid.position, 0.0f), kBall, 2e-3f));
    // the axis of the cylinder (rho = 0) also gets out
    const SceneObject& cylinder = Find(ShapeKind::Cylinder, kLayerA);
    const dl::CollisionResult axis = dl::ResolveSphere(Centre(cylinder), kBall, 0, 0.0f);
    CHECK(axis.pushed && std::isfinite(axis.position.x) && DistanceToSolid(cylinder, axis.position, 0.0f) > kBall - 2e-3f);
}

void TestNeverInside()
{
    // a plain, repeatable "random" series: no <random>, so the result is the same on every compiler
    unsigned state = 12345u;
    const auto next = [&state]() {
        state = state * 1664525u + 1013904223u;
        return static_cast<float>((state >> 8) & 0xFFFFu) / 65535.0f; // 0 .. 1
    };
    int tested = 0;
    for (int i = 0; i < 4000; ++i)
    {
        const Vec3 p{-6.0f + 12.0f * next(), -1.6f + 3.2f * next(), -1.0f + 8.0f * next()}; // the area of the shapes
        const float spin = 20.0f * next();
        const int layer = i % 2;
        const dl::CollisionResult r = dl::ResolveSphere(p, kBall, layer, spin);
        // the ball may rest ON a surface (distance == radius), never inside it (a little tolerance for float error)
        CHECK(DistanceToLayer(r.position, layer, spin) >= kBall - 3e-3f);
        ++tested;
    }
    CHECK(tested == 4000);
}

void TestSettledAndWalking()
{
    // a ball that has been pushed out is settled: a second call changes nothing
    unsigned state = 777u;
    const auto next = [&state]() {
        state = state * 1664525u + 1013904223u;
        return static_cast<float>((state >> 8) & 0xFFFFu) / 65535.0f;
    };
    for (int i = 0; i < 500; ++i)
    {
        const Vec3 p{-5.0f + 10.0f * next(), -0.9f + 2.0f * next(), 0.0f + 6.0f * next()};
        const float spin = 5.0f * next();
        const int layer = i % 2;
        const dl::CollisionResult first = dl::ResolveSphere(p, kBall, layer, spin);
        const dl::CollisionResult second = dl::ResolveSphere(first.position, kBall, layer, spin);
        CHECK(!second.pushed);
    }
    // walking along +X at the height of the red cube, in small steps, as the camera does (3 m/s, 60 frames a second).
    // The ball may slide round the turning cube (a wall only pushes it away from the surface), so the walk is judged by
    // what it touches, not by where it ends: in layer A the cube pushes the ball, in layer B nothing does until the
    // blue sphere (x = -1.1) is near; and in both layers the ball is never inside a solid.
    const SceneObject& cube = Find(ShapeKind::Cube, kLayerA);
    const float sphereX = Find(ShapeKind::Sphere, kLayerB).position[0];
    for (int layer = 0; layer < 2; ++layer)
    {
        Vec3 p{-7.0f, cube.position[1], cube.position[2]};
        float spin = 0.0f;
        int pushedInCubeZone = 0;
        bool alwaysOutside = true;
        for (int frame = 0; frame < 600; ++frame)
        {
            spin += 1.0f / 60.0f;
            p.x += 3.0f / 60.0f;
            const dl::CollisionResult r = dl::ResolveSphere(p, kBall, layer, spin);
            p = r.position;
            if (r.pushed && p.x < sphereX - 1.5f) // before the blue sphere can touch the ball
            {
                ++pushedInCubeZone;
            }
            alwaysOutside = alwaysOutside && DistanceToLayer(p, layer, spin) >= kBall - 3e-3f;
        }
        CHECK(alwaysOutside);
        if (layer == 0)
        {
            CHECK(pushedInCubeZone > 5); // the cube is solid: the ball is pushed again and again while it walks into it
        }
        else
        {
            CHECK(pushedInCubeZone == 0); // the cube does not exist in B: the same walk is not touched
        }
    }
}

void TestYaw()
{
    // a box turned by a fixed yaw (a house turned to its street): local and world round trip, and a ball pushed out of it to
    // exactly its radius, measured in the frame of the box worked out forward (RotateY of the axes), not backward
    SceneObject o{};
    o.shape = ShapeKind::Cube;
    o.position[0] = 2.0f;
    o.position[1] = 0.5f;
    o.position[2] = -1.0f;
    o.scale[0] = 3.0f;
    o.scale[1] = 1.0f;
    o.scale[2] = 1.0f;
    o.yaw = 0.6f;
    o.layerMask = kLayerBoth;
    const Vec3 p{2.7f, 0.3f, -0.2f};
    CHECK(Near3(dl::collision_detail::ToWorld(o, dl::collision_detail::ToLocal(o, p, 0.0f), 0.0f), p, 1e-4f));
    const Vec3 ax = dl::collision_detail::RotateY({1.0f, 0.0f, 0.0f}, o.yaw); // the long axis of the box in the world
    const Vec3 start{2.0f + ax.x * 1.3f, 0.5f, -1.0f + ax.z * 1.3f};             // inside, 0.2 m from its end face
    const dl::CollisionResult r = dl::ResolveSphere(start, kBall, 0, 0.0f, &o, 1);
    CHECK(r.pushed);
    const Vec3 d{r.position.x - 2.0f, r.position.y - 0.5f, r.position.z + 1.0f};
    const float along = Dot(d, ax); // out through the end face: 1.5 + the radius along the long axis
    CHECK(Near(along, 1.5f + kBall, 2e-3f));
    o.yaw = 0.0f; // the same box without the yaw does not hold that point
    CHECK(!dl::ResolveSphere(start, kBall, 0, 0.0f, &o, 1).pushed || dl::ResolveSphere(start, kBall, 0, 0.0f, &o, 1).position.x != r.position.x);
}

void TestBadInput()
{
    const float notANumber = std::numeric_limits<float>::quiet_NaN();
    const Vec3 start{1.0f, 0.5f, -2.0f};
    CHECK(!dl::ResolveSphere({notANumber, 0.0f, 0.0f}, kBall, 0, 0.0f).pushed);
    CHECK(!dl::ResolveSphere(start, kBall, 0, notANumber).pushed);
    CHECK(Near3(dl::ResolveSphere(start, kBall, 2, 0.0f).position, start) && !dl::ResolveSphere(start, kBall, 2, 0.0f).pushed);
    CHECK(Near3(dl::ResolveSphere(start, kBall, -1, 0.0f).position, start));
    CHECK(Near3(dl::ResolveSphere(start, 0.0f, 0, 0.0f).position, start));
    CHECK(Near3(dl::ResolveSphere(start, -1.0f, 0, 0.0f).position, start));
}

} // namespace

int main()
{
    TestRotations();
    TestScene();
    TestLayerRule();
    TestEachShape();
    TestNeverInside();
    TestSettledAndWalking();
    TestYaw();
    TestBadInput();
    std::printf("%d checks passed, %d failed\n", g_passed, g_failed);
    return g_failed == 0 ? 0 : 1;
}
