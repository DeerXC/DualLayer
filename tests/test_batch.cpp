// test_batch.cpp - tests of the static batch (no window, no GPU): src/render/StaticBatch.h on the fixed boxes of the map
// (src/scene/Stage.h), which are what the game bakes.
//   g++ -std=c++20 -O2 -Wall -Wextra -I src tests/test_batch.cpp src/render/MeshData.cpp -o test_batch
//   MSVC (Developer PowerShell):  .\tests\run_tests.ps1 -Only batch
// What is checked, for each layer:
//   1. what is baked: exactly the cubes that do not turn and are in the layer; 24 vertices and 36 indices each; every index
//      in range
//   2. where: every vertex lies on the surface of its box, measured in the local space of the box by the collision code
//      (an independent path: it turns BACK from the world, the bake turns forward); the corners of an untilted box are its
//      corners
//   3. how it faces: every normal has length 1 and every triangle is clockwise seen from outside, its cross product pointing
//      exactly along the normals of its vertices (a box has flat faces), also for the turned ramps
//   4. what it carries: the linear colour, the city flag (both layers and no glow), checker, shine and glow, as the renderer
//      would have sent them for the box alone
//   5. what is left out: a turning cube and a sphere; nothing of the fixed city of the game (it would not be drawn at all)
// Returns 1 when a check fails.

#include "render/StaticBatch.h"
#include "scene/Collision.h"
#include "scene/Stage.h"

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

using namespace dl::scene;
namespace st = dl::stage;

void TestLayer(int layer)
{
    const SceneObject* objects = st::kStageStatics.data();
    const int count = st::kStageStaticCount;
    const dl::BatchMesh mesh = dl::BakeLayer(objects, count, layer);
    int expected = 0;
    for (int i = 0; i < count; ++i)
    {
        expected += ((objects[i].layerMask & (1u << layer)) != 0u && dl::Accepts(objects[i])) ? 1 : 0;
    }
    CHECK(mesh.objects == expected && expected > 50);
    CHECK(mesh.vertices.size() == static_cast<std::size_t>(24 * expected) && mesh.indices.size() == static_cast<std::size_t>(36 * expected));
    bool inRange = true;
    for (std::uint32_t i : mesh.indices)
    {
        inRange = inRange && i < mesh.vertices.size();
    }
    CHECK(inRange);
    // walk the baked objects in order: object k owns vertices 24k .. 24k + 23
    int k = 0;
    bool onSurface = true;
    bool unitNormals = true;
    bool carried = true;
    for (int i = 0; i < count; ++i)
    {
        const SceneObject& o = objects[i];
        if ((o.layerMask & (1u << layer)) == 0u || !dl::Accepts(o))
        {
            continue;
        }
        const float city = (o.layerMask == kLayerBoth && o.glow <= 0.0f) ? 1.0f : 0.0f;
        for (int v = 0; v < 24; ++v)
        {
            const dl::BatchVertex& b = mesh.vertices[static_cast<std::size_t>(24 * k + v)];
            const dl::Vec3 local = dl::collision_detail::ToLocal(o, {b.position[0], b.position[1], b.position[2]}, 0.0f);
            const float gx = std::fabs(std::fabs(local.x) - 0.5f * o.scale[0]);
            const float gy = std::fabs(std::fabs(local.y) - 0.5f * o.scale[1]);
            const float gz = std::fabs(std::fabs(local.z) - 0.5f * o.scale[2]);
            onSurface = onSurface && gx < 1e-3f && gy < 1e-3f && gz < 1e-3f; // a vertex of a box is a corner: on all three
            unitNormals = unitNormals && std::fabs(std::sqrt(b.normal[0] * b.normal[0] + b.normal[1] * b.normal[1] + b.normal[2] * b.normal[2]) - 1.0f) < 1e-4f;
            carried = carried && b.color[3] == city && b.params[0] == o.checker && b.params[1] == o.shineStrength && b.params[2] == o.shininess &&
                      b.params[3] == o.glow && std::fabs(b.color[0] - dl::batch_detail::SrgbToLinear(o.colorSrgb[0])) < 1e-6f;
        }
        ++k;
    }
    CHECK(onSurface && unitNormals && carried);
    bool clockwise = true;
    for (std::size_t t = 0; t + 2 < mesh.indices.size(); t += 3)
    {
        const dl::BatchVertex& a = mesh.vertices[mesh.indices[t]];
        const dl::BatchVertex& b = mesh.vertices[mesh.indices[t + 1]];
        const dl::BatchVertex& c = mesh.vertices[mesh.indices[t + 2]];
        const float e1[3] = {b.position[0] - a.position[0], b.position[1] - a.position[1], b.position[2] - a.position[2]};
        const float e2[3] = {c.position[0] - a.position[0], c.position[1] - a.position[1], c.position[2] - a.position[2]};
        const float n[3] = {e1[1] * e2[2] - e1[2] * e2[1], e1[2] * e2[0] - e1[0] * e2[2], e1[0] * e2[1] - e1[1] * e2[0]};
        const float len = std::sqrt(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]);
        // the face normal of the triangle IS the vertex normal of a box (a flat face): the same direction, not only the same side
        clockwise = clockwise && len > 0.0f && (n[0] * a.normal[0] + n[1] * a.normal[1] + n[2] * a.normal[2]) / len > 0.9999f;
    }
    CHECK(clockwise);
}

void TestLeftOut()
{
    SceneObject objects[3] = {};
    objects[0].shape = ShapeKind::Cube;
    objects[0].scale[0] = objects[0].scale[1] = objects[0].scale[2] = 1.0f;
    objects[0].layerMask = kLayerBoth;
    objects[1] = objects[0];
    objects[1].spinSpeed = 0.5f; // turns: drawn alone
    objects[2] = objects[0];
    objects[2].shape = ShapeKind::Sphere;
    CHECK(dl::BakeLayer(objects, 3, 0).objects == 1 && dl::BakeLayer(objects, 3, 1).objects == 1);
    CHECK(dl::BakeLayer(nullptr, 3, 0).objects == 0 && dl::BakeLayer(objects, 3, 2).objects == 0);
    const dl::BatchMesh one = dl::BakeLayer(objects, 1, 0); // an untilted unit cube at the origin: its corners are +-0.5
    bool corners = true;
    for (const dl::BatchVertex& v : one.vertices)
    {
        for (int a = 0; a < 3; ++a)
        {
            corners = corners && std::fabs(std::fabs(v.position[a]) - 0.5f) < 1e-6f;
        }
    }
    CHECK(corners);
}

void TestYawAndAppend()
{
    // a box turned by a yaw (and a ramp-like tilt): every baked vertex on its surface (turned back by the collision code), every
    // triangle clockwise from outside along its normals
    SceneObject o{};
    o.shape = ShapeKind::Cube;
    o.position[0] = 4.0f;
    o.position[1] = 1.0f;
    o.position[2] = -2.0f;
    o.scale[0] = 2.0f;
    o.scale[1] = 0.5f;
    o.scale[2] = 3.0f;
    o.tilt = 0.3f;
    o.yaw = 0.8f;
    o.layerMask = kLayerA;
    const dl::BatchMesh m = dl::BakeLayer(&o, 1, 0);
    bool onSurface = true;
    for (const dl::BatchVertex& v : m.vertices)
    {
        const dl::Vec3 l = dl::collision_detail::ToLocal(o, {v.position[0], v.position[1], v.position[2]}, 0.0f);
        onSurface = onSurface && std::fabs(std::fabs(l.x) - 1.0f) < 1e-4f && std::fabs(std::fabs(l.y) - 0.25f) < 1e-4f && std::fabs(std::fabs(l.z) - 1.5f) < 1e-4f;
    }
    CHECK(m.objects == 1 && onSurface);
    bool clockwise = true;
    for (std::size_t t = 0; t + 2 < m.indices.size(); t += 3)
    {
        const dl::BatchVertex& a = m.vertices[m.indices[t]];
        const dl::BatchVertex& b = m.vertices[m.indices[t + 1]];
        const dl::BatchVertex& c = m.vertices[m.indices[t + 2]];
        const float e1[3] = {b.position[0] - a.position[0], b.position[1] - a.position[1], b.position[2] - a.position[2]};
        const float e2[3] = {c.position[0] - a.position[0], c.position[1] - a.position[1], c.position[2] - a.position[2]};
        const float n[3] = {e1[1] * e2[2] - e1[2] * e2[1], e1[2] * e2[0] - e1[0] * e2[2], e1[0] * e2[1] - e1[1] * e2[0]};
        const float len = std::sqrt(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]);
        clockwise = clockwise && (n[0] * a.normal[0] + n[1] * a.normal[1] + n[2] * a.normal[2]) / len > 0.9999f;
    }
    CHECK(clockwise);
    // joined: the indices of the second mesh move past the vertices of the first
    dl::BatchMesh joined = m;
    dl::AppendMesh(joined, m);
    CHECK(joined.vertices.size() == 2 * m.vertices.size() && joined.indices.size() == 2 * m.indices.size() && joined.objects == 2);
    CHECK(joined.indices[m.indices.size()] == m.indices[0] + static_cast<std::uint32_t>(m.vertices.size()));
    // one flat triangle: its normal is cross(b - a, c - a), unit; a triangle without area is left out
    dl::BatchMesh tri;
    const float a[3] = {0.0f, 0.0f, 0.0f};
    const float b[3] = {0.0f, 0.0f, 1.0f};
    const float c[3] = {1.0f, 0.0f, 0.0f};
    const float colour[4] = {0.5f, 0.4f, 0.3f, 0.0f};
    dl::AddFlatTriangle(tri, a, b, c, colour);
    dl::AddFlatTriangle(tri, a, a, c, colour);
    CHECK(tri.vertices.size() == 3 && tri.indices.size() == 3 && std::fabs(tri.vertices[0].normal[1] - 1.0f) < 1e-6f && tri.vertices[2].color[0] == 0.5f &&
          tri.vertices[1].params[1] == 0.0f);
}

void TestWholeFixedCity()
{
    // what the game bakes: the first StaticDrawCount entries of the drawn table (the fixed boxes and the decor). Every one of
    // them must be accepted: an object left out would not be drawn at all (main.cpp gives only the rest to SetObjects)
    static SceneObject table[st::kMaxDrawObjects];
    const dl::puzzle::Puzzle none;
    st::BuildDrawTable(none, false, table);
    bool all = true;
    for (int i = 0; i < st::StaticDrawCount(); ++i)
    {
        all = all && dl::Accepts(table[i]);
    }
    CHECK(all && st::StaticDrawCount() == st::kStageStaticCount + static_cast<int>(st::kDecor.size()));
    const dl::BatchMesh a = dl::BakeLayer(table, st::StaticDrawCount(), 0);
    const dl::BatchMesh b = dl::BakeLayer(table, st::StaticDrawCount(), 1);
    int inA = 0;
    int inB = 0;
    for (int i = 0; i < st::StaticDrawCount(); ++i)
    {
        inA += (table[i].layerMask & kLayerA) != 0u ? 1 : 0;
        inB += (table[i].layerMask & kLayerB) != 0u ? 1 : 0;
    }
    CHECK(a.objects == inA && b.objects == inB);
    std::printf("  fixed city: %d boxes in A, %d in B: two draw calls instead of %d\n", a.objects, b.objects, a.objects + b.objects);
}

} // namespace

int main()
{
    TestLayer(0);
    TestLayer(1);
    TestLeftOut();
    TestYawAndAppend();
    TestWholeFixedCity();
    std::printf("%d checks passed, %d failed\n", g_passed, g_failed);
    return g_failed == 0 ? 0 : 1;
}
