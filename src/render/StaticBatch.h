// StaticBatch.h - the vertex of the static batch and the joining of meshes. The town (scene/Town.h) bakes every polygon
// of a
//            moment into one list of vertices, so that the whole fixed town is drawn with one draw call per moment
//            instead of one per polygon (render/Models.cpp puts the lists on the GPU).
// Vertex:    the world position and the world normal (already transformed), the linear colour with the "city" flag in
//            alpha, and the parameters of the model shader (checker, shine strength, shininess, glow): what the
//            per-object constant buffer would have carried. So the model shader treats a baked polygon exactly as an
//            object drawn alone.
// Plain data, no Windows, no Direct3D: tested without a window (tests/test_batch.cpp).
#pragma once

#include <cmath>
#include <cstdint>
#include <vector>

namespace dl
{

struct BatchVertex
{
    float position[3]; ///< UNITS: metres, world space
    float normal[3];   ///< world space, unit length
    float color[4];  ///< linear rgb; a: 1 = part of the city (the tint and weathering of the layer), 0 = its own colour
    float params[4]; ///< x: checkered floor, y: shine strength, z: shininess, w: glow
};
static_assert(sizeof(BatchVertex) == 56, "the batched input layout (render/Models.cpp) assumes 56 bytes per vertex");

struct BatchMesh
{
    std::vector<BatchVertex> vertices;
    std::vector<std::uint32_t> indices;
    int objects = 0; ///< how many polygons were baked into it
};

namespace batch_detail
{

inline float SrgbToLinear(float c)
{
    return c <= 0.04045f ? c / 12.92f : std::pow((c + 0.055f) / 1.055f, 2.4f);
}

} // namespace batch_detail

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

} // namespace dl
