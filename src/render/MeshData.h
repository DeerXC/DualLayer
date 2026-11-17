#pragma once

// MeshData.h - the shapes of the models as plain lists of vertices and indices. No Windows, no Direct3D: the shapes are
// made on the CPU and tested without a window (tests/test_mesh.cpp); render/Models.cpp copies them into GPU buffers.
// Owns:      nothing (every Make* function returns a new value).
// UNITS:     metres. Every shape is centred on its own origin; scaling and moving it is the job of the world matrix.
// Rules of every shape (docs/CONVENTIONS.md, "Mesh" rows):
//   - left-handed, +Y up;
//   - a triangle is CLOCKWISE seen from OUTSIDE (the Direct3D default front face). The outward normal of such a
//   triangle
//     is cross(v1 - v0, v2 - v0) with the usual component formula; the test checks this for every triangle;
//   - every vertex carries the unit normal of the surface at that vertex, pointing outwards: the light is worked out
//     from it. A cube has 24 vertices, not 8: a corner belongs to three faces and each face needs its own normal (a
//     "hard edge"). A sphere shares its vertices between triangles (a "smooth" surface);
//   - indices are 32 bit.
#include <cstdint>
#include <vector>

namespace dl
{
struct MeshVertex
{
    float position[3];
    float normal[3];
};

static_assert(sizeof(MeshVertex) == 24,
              "the input layout of the models (render/Models.cpp) assumes 24 bytes per vertex");

struct MeshData
{
    std::vector<MeshVertex> vertices;
    std::vector<std::uint32_t> indices;

    std::size_t TriangleCount() const
    {
        return indices.size() / 3;
    }
};

// A cube with edges of length 1 (from -0.5 to +0.5 on every axis): 24 vertices, 12 triangles.
MeshData MakeCube();

// A sphere of radius 0.5 (a diameter of 1): `segments` slices round the Y axis and segments / 2 stacks from the pole to
// the pole. segments is clamped to 6 .. 128 and made even.
MeshData MakeSphere(int segments);

// A cylinder of radius 0.5 and height 1 (from y = -0.5 to y = +0.5) with two caps: `segments` sides, clamped to 3 ..
// 128. The side has smooth normals (they point away from the axis), each cap has its own vertices with a flat normal
// (+Y or -Y).
MeshData MakeCylinder(int segments);

// A torus (a ring, a doughnut) lying flat in the XZ plane round the Y axis. majorRadius: from the centre of the ring to
// the middle of the tube; minorRadius: the radius of the tube. `segments` steps round the ring (3 .. 128) and segments
// / 2 steps round the tube (at least 4). It is exactly a tube of radius minorRadius, bent into a circle.
MeshData MakeTorus(int segments, float majorRadius, float minorRadius);
} // namespace dl
