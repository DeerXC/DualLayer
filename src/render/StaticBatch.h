// StaticBatch.h - bakes the boxes of the map that never move into one list of vertices per layer, so that the whole fixed city
//            is drawn with one draw call per layer instead of one per box (render/Models.cpp puts the lists on the GPU).
// What:      every object of the table that is a cube and does not turn (spinSpeed 0; a fixed yaw is fine), in each layer of
//            its mask. Anything else is left out and has to be drawn one by one (Accepts). Other fixed geometry (the terrain,
//            the plants: scene/Landscape.h) is baked by its owner into a BatchMesh and joined with AppendMesh.
// Vertex:    the world position and the world normal (already transformed: the world matrix is Scale * RotationX(tilt) *
//            RotationY(yaw) * Translation, the same as the renderer, docs/CONVENTIONS.md), the linear colour with the "city" flag in alpha,
//            and the parameters of the model shader (checker, shine strength, shininess, glow): what the per-object constant
//            buffer would have carried. So the model shader treats a baked box exactly as a box drawn alone.
// Winding:   the cube of render/MeshData.h, turned and moved: still clockwise seen from outside (a scale does not flip it,
//            all scales are above 0). tests/test_batch.cpp checks every triangle.
// Plain data, no Windows, no Direct3D: tested without a window (tests/test_batch.cpp).
#pragma once

#include "render/MeshData.h"
#include "scene/SceneData.h"

#include <cmath>
#include <cstdint>
#include <vector>

namespace dl
{

struct BatchVertex
{
    float position[3]; ///< UNITS: metres, world space
    float normal[3];   ///< world space, unit length
    float color[4];    ///< linear rgb; a: 1 = part of the city (the tint and weathering of the layer), 0 = its own colour
    float params[4];   ///< x: checkered floor, y: shine strength, z: shininess, w: glow
};
static_assert(sizeof(BatchVertex) == 56, "the batched input layout (render/Models.cpp) assumes 56 bytes per vertex");

struct BatchMesh
{
    std::vector<BatchVertex> vertices;
    std::vector<std::uint32_t> indices;
    int objects = 0; ///< how many boxes were baked into it
};

namespace batch_detail
{

inline float SrgbToLinear(float c)
{
    return c <= 0.04045f ? c / 12.92f : std::pow((c + 0.055f) / 1.055f, 2.4f);
}

} // namespace batch_detail

/// True when `o` can be baked: a cube that does not turn, with sizes above 0.
inline bool Accepts(const scene::SceneObject& o)
{
    return o.shape == scene::ShapeKind::Cube && o.spinSpeed == 0.0f && o.scale[0] > 0.0f && o.scale[1] > 0.0f && o.scale[2] > 0.0f;
}

/// The baked mesh of the objects of `layer` (0 = A, 1 = B) among objects[0 .. count - 1] that Accepts.
inline BatchMesh BakeLayer(const scene::SceneObject* objects, int count, int layer)
{
    BatchMesh out;
    if (objects == nullptr || count <= 0 || (layer != 0 && layer != 1))
    {
        return out;
    }
    static const MeshData cube = MakeCube();
    const unsigned bit = 1u << layer;
    for (int i = 0; i < count; ++i)
    {
        const scene::SceneObject& o = objects[i];
        if ((o.layerMask & bit) == 0u || !Accepts(o))
        {
            continue;
        }
        const float c = std::cos(o.tilt);
        const float s = std::sin(o.tilt);
        const float cy = std::cos(o.yaw);
        const float sy = std::sin(o.yaw);
        const float city = (o.layerMask == scene::kLayerBoth && o.glow <= 0.0f) ? 1.0f : 0.0f;
        const float colour[4] = {batch_detail::SrgbToLinear(o.colorSrgb[0]), batch_detail::SrgbToLinear(o.colorSrgb[1]),
                                 batch_detail::SrgbToLinear(o.colorSrgb[2]), city};
        const std::uint32_t first = static_cast<std::uint32_t>(out.vertices.size());
        for (const MeshVertex& v : cube.vertices)
        {
            BatchVertex b{};
            // scale, then RotationX(tilt) on a row vector (y' = y c - z s, z' = y s + z c), then RotationY(yaw) (x' = x cy + z sy,
            // z' = -x sy + z cy), then the move
            const float x = v.position[0] * o.scale[0];
            const float y = v.position[1] * o.scale[1];
            const float z = v.position[2] * o.scale[2];
            const float ty = y * c - z * s;
            const float tz = y * s + z * c;
            b.position[0] = x * cy + tz * sy + o.position[0];
            b.position[1] = ty + o.position[1];
            b.position[2] = -x * sy + tz * cy + o.position[2];
            // the normal of a face of a box keeps its axis under an axis-aligned scale; then it turns with the tilt and the yaw
            const float ny = v.normal[1];
            const float nz = v.normal[2];
            const float my = ny * c - nz * s;
            const float mz = ny * s + nz * c;
            b.normal[0] = v.normal[0] * cy + mz * sy;
            b.normal[1] = my;
            b.normal[2] = -v.normal[0] * sy + mz * cy;
            for (int k = 0; k < 4; ++k)
            {
                b.color[k] = colour[k];
            }
            b.params[0] = o.checker;
            b.params[1] = o.shineStrength;
            b.params[2] = o.shininess;
            b.params[3] = o.glow;
            out.vertices.push_back(b);
        }
        for (std::uint32_t index : cube.indices)
        {
            out.indices.push_back(first + index);
        }
        ++out.objects;
    }
    return out;
}

/// Adds `more` at the end of `into` (its indices moved past the vertices already there).
inline void AppendMesh(BatchMesh& into, const BatchMesh& more)
{
    const std::uint32_t first = static_cast<std::uint32_t>(into.vertices.size());
    into.vertices.insert(into.vertices.end(), more.vertices.begin(), more.vertices.end());
    for (std::uint32_t i : more.indices)
    {
        into.indices.push_back(first + i);
    }
    into.objects += more.objects;
}

/// Adds one flat, matt triangle (a, b, c) in the linear colour `colour` (alpha: the city flag). Its normal is cross(b - a, c - a)
/// made unit length, so (a, b, c) is clockwise seen from the side the normal points to. A triangle without area is left out.
inline void AddFlatTriangle(BatchMesh& mesh, const float a[3], const float b[3], const float c[3], const float colour[4])
{
    const float e1[3] = {b[0] - a[0], b[1] - a[1], b[2] - a[2]};
    const float e2[3] = {c[0] - a[0], c[1] - a[1], c[2] - a[2]};
    float n[3] = {e1[1] * e2[2] - e1[2] * e2[1], e1[2] * e2[0] - e1[0] * e2[2], e1[0] * e2[1] - e1[1] * e2[0]};
    const float length = std::sqrt(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]);
    if (!(length > 1.0e-9f))
    {
        return;
    }
    for (float& v : n)
    {
        v /= length;
    }
    const std::uint32_t first = static_cast<std::uint32_t>(mesh.vertices.size());
    const float* corners[3] = {a, b, c};
    for (const float* p : corners)
    {
        BatchVertex v{};
        for (int k = 0; k < 3; ++k)
        {
            v.position[k] = p[k];
            v.normal[k] = n[k];
        }
        for (int k = 0; k < 4; ++k)
        {
            v.color[k] = colour[k];
        }
        v.params[2] = 1.0f; // shininess 1 with a strength of 0: matt
        mesh.vertices.push_back(v);
    }
    mesh.indices.push_back(first);
    mesh.indices.push_back(first + 1);
    mesh.indices.push_back(first + 2);
}

} // namespace dl
