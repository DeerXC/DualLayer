#include "render/MeshData.h"

#include <algorithm>
#include <cmath>

namespace dl
{
namespace
{
constexpr float kPi = 3.14159265358979323846f;

void AddVertex(MeshData& mesh, float px, float py, float pz, float nx, float ny, float nz)
{
    MeshVertex v{};
    v.position[0] = px;
    v.position[1] = py;
    v.position[2] = pz;
    v.normal[0] = nx;
    v.normal[1] = ny;
    v.normal[2] = nz;
    mesh.vertices.push_back(v);
}

void AddTriangle(MeshData& mesh, std::uint32_t a, std::uint32_t b, std::uint32_t c)
{
    mesh.indices.push_back(a);
    mesh.indices.push_back(b);
    mesh.indices.push_back(c);
}
} // namespace

// -----------------------------------------------------------------------------
// CUBE
// Every face is a square with the outward normal n and two edge directions u and v chosen so that cross(u, v) = n. The
// corners are taken in the order
//   (-u -v), (+u -v), (+u +v), (-u +v)
// and the two triangles are (0, 1, 2) and (0, 2, 3). For both of them cross(v1 - v0, v2 - v0) = n: clockwise seen from
// outside.
// -----------------------------------------------------------------------------
MeshData MakeCube()
{
    struct Face
    {
        float n[3];
        float u[3];
        float v[3];
    };
    // cross(u, v) = n for every row (the usual component formula)
    static const Face kFaces[6] = {
        {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}},  // +X
        {{-1, 0, 0}, {0, 0, 1}, {0, 1, 0}}, // -X
        {{0, 1, 0}, {0, 0, 1}, {1, 0, 0}},  // +Y
        {{0, -1, 0}, {1, 0, 0}, {0, 0, 1}}, // -Y
        {{0, 0, 1}, {1, 0, 0}, {0, 1, 0}},  // +Z
        {{0, 0, -1}, {0, 1, 0}, {1, 0, 0}}, // -Z
    };
    constexpr float kSigns[4][2] = {{-1, -1}, {1, -1}, {1, 1}, {-1, 1}};

    MeshData mesh;
    mesh.vertices.reserve(24);
    mesh.indices.reserve(36);
    for (const Face& face : kFaces)
    {
        const std::uint32_t first = static_cast<std::uint32_t>(mesh.vertices.size());
        for (const auto& s : kSigns)
        {
            const float px = 0.5f * face.n[0] + 0.5f * s[0] * face.u[0] + 0.5f * s[1] * face.v[0];
            const float py = 0.5f * face.n[1] + 0.5f * s[0] * face.u[1] + 0.5f * s[1] * face.v[1];
            const float pz = 0.5f * face.n[2] + 0.5f * s[0] * face.u[2] + 0.5f * s[1] * face.v[2];
            AddVertex(mesh, px, py, pz, face.n[0], face.n[1], face.n[2]);
        }
        AddTriangle(mesh, first, first + 1, first + 2);
        AddTriangle(mesh, first, first + 2, first + 3);
    }
    return mesh;
}

// -----------------------------------------------------------------------------
// SPHERE
// A grid of (slices + 1) x (stacks + 1) vertices: i goes round the Y axis (theta), j from the north pole (+Y) to the
// south pole (phi).
//   position = 0.5 * (sin(phi) cos(theta), cos(phi), sin(phi) sin(theta)),   normal = position / 0.5
// Seen from outside on the equator, theta grows to the right and j grows downwards, so a square (A top left, B top
// right, C bottom left, D bottom right) is clockwise as (A, B, D) and (A, D, C). At the poles one of the two triangles
// of a square has no area (two of its corners are the same point): it is left out.
// -----------------------------------------------------------------------------
MeshData MakeSphere(int segments)
{
    segments = std::clamp(segments, 6, 128);
    segments += segments % 2; // an even number: the stacks are half of it
    const int slices = segments;
    const int stacks = segments / 2;

    MeshData mesh;
    mesh.vertices.reserve(static_cast<std::size_t>(slices + 1) * static_cast<std::size_t>(stacks + 1));
    for (int j = 0; j <= stacks; ++j)
    {
        const float phi = kPi * static_cast<float>(j) / static_cast<float>(stacks);
        for (int i = 0; i <= slices; ++i)
        {
            const float theta = 2.0f * kPi * static_cast<float>(i) / static_cast<float>(slices);
            const float nx = std::sin(phi) * std::cos(theta);
            const float ny = std::cos(phi);
            const float nz = std::sin(phi) * std::sin(theta);
            AddVertex(mesh, 0.5f * nx, 0.5f * ny, 0.5f * nz, nx, ny, nz);
        }
    }
    for (int j = 0; j < stacks; ++j)
    {
        for (int i = 0; i < slices; ++i)
        {
            const std::uint32_t a = static_cast<std::uint32_t>(j * (slices + 1) + i);
            const std::uint32_t b = a + 1;
            const std::uint32_t c = a + static_cast<std::uint32_t>(slices + 1);
            const std::uint32_t d = c + 1;
            if (j != 0)
            {
                AddTriangle(mesh, a, b, d);
            }
            if (j != stacks - 1)
            {
                AddTriangle(mesh, a, d, c);
            }
        }
    }
    return mesh;
}

// -----------------------------------------------------------------------------
// CYLINDER
// The side: a ring of vertices at the top (y = +0.5) and one at the bottom, with the normal pointing away from the
// axis. Seen from outside, theta grows to the right: the square (top i = A, top i+1 = B, bottom i = C, bottom i+1 = D)
// is clockwise as (A, B, D) and (A, D, C). The caps: a centre and a ring of their own (the normal of a cap is flat, not
// the one of the side). Seen from above, theta grows COUNTER-clockwise, so the top cap is (centre, i + 1, i); seen from
// below it grows clockwise, so the bottom cap is (centre, i, i + 1).
// -----------------------------------------------------------------------------
MeshData MakeCylinder(int segments)
{
    segments = std::clamp(segments, 3, 128);

    MeshData mesh;
    // the side
    for (int ring = 0; ring < 2; ++ring)
    {
        const float y = ring == 0 ? 0.5f : -0.5f;
        for (int i = 0; i <= segments; ++i)
        {
            const float theta = 2.0f * kPi * static_cast<float>(i) / static_cast<float>(segments);
            const float cx = std::cos(theta), cz = std::sin(theta);
            AddVertex(mesh, 0.5f * cx, y, 0.5f * cz, cx, 0.0f, cz);
        }
    }
    for (int i = 0; i < segments; ++i)
    {
        const std::uint32_t a = static_cast<std::uint32_t>(i);
        const std::uint32_t b = a + 1;
        const std::uint32_t c = a + static_cast<std::uint32_t>(segments + 1);
        const std::uint32_t d = c + 1;
        AddTriangle(mesh, a, b, d);
        AddTriangle(mesh, a, d, c);
    }
    // the two caps
    for (int cap = 0; cap < 2; ++cap)
    {
        const bool top = cap == 0;
        const float y = top ? 0.5f : -0.5f;
        const float ny = top ? 1.0f : -1.0f;
        const std::uint32_t centre = static_cast<std::uint32_t>(mesh.vertices.size());
        AddVertex(mesh, 0.0f, y, 0.0f, 0.0f, ny, 0.0f);
        for (int i = 0; i <= segments; ++i)
        {
            const float theta = 2.0f * kPi * static_cast<float>(i) / static_cast<float>(segments);
            AddVertex(mesh, 0.5f * std::cos(theta), y, 0.5f * std::sin(theta), 0.0f, ny, 0.0f);
        }
        for (int i = 0; i < segments; ++i)
        {
            const std::uint32_t r0 = centre + 1 + static_cast<std::uint32_t>(i);
            const std::uint32_t r1 = r0 + 1;
            if (top)
            {
                AddTriangle(mesh, centre, r1, r0);
            }
            else
            {
                AddTriangle(mesh, centre, r0, r1);
            }
        }
    }
    return mesh;
}

// -----------------------------------------------------------------------------
// TORUS
// A tube of radius r bent round the Y axis in a circle of radius R. The vertex (i, j): the angle u = i / segments round
// the ring, v = j / tubeSegments round the tube.
//   radial(u) = (cos u, 0, sin u),   normal = cos(v) * radial(u) + sin(v) * (0, 1, 0),   position = R * radial(u) + r *
//   normal
// Seen from outside at u = 0, u grows to the right and v grows upwards, so a square (A bottom left, B bottom right, C
// top left, D top right) is clockwise as (C, D, B) and (C, B, A).
// -----------------------------------------------------------------------------
MeshData MakeTorus(int segments, float majorRadius, float minorRadius)
{
    segments = std::clamp(segments, 3, 128);
    const int ringSteps = segments;
    const int tubeSteps = std::max(4, segments / 2);

    MeshData mesh;
    mesh.vertices.reserve(static_cast<std::size_t>(ringSteps + 1) * static_cast<std::size_t>(tubeSteps + 1));
    for (int i = 0; i <= ringSteps; ++i)
    {
        const float u = 2.0f * kPi * static_cast<float>(i) / static_cast<float>(ringSteps);
        const float rx = std::cos(u), rz = std::sin(u);
        for (int j = 0; j <= tubeSteps; ++j)
        {
            const float v = 2.0f * kPi * static_cast<float>(j) / static_cast<float>(tubeSteps);
            const float nx = std::cos(v) * rx;
            const float ny = std::sin(v);
            const float nz = std::cos(v) * rz;
            AddVertex(mesh, majorRadius * rx + minorRadius * nx, minorRadius * ny, majorRadius * rz + minorRadius * nz,
                      nx, ny, nz);
        }
    }
    for (int i = 0; i < ringSteps; ++i)
    {
        for (int j = 0; j < tubeSteps; ++j)
        {
            const std::uint32_t a = static_cast<std::uint32_t>(i * (tubeSteps + 1) + j);
            const std::uint32_t c = a + 1;
            const std::uint32_t b = a + static_cast<std::uint32_t>(tubeSteps + 1);
            const std::uint32_t d = b + 1;
            AddTriangle(mesh, c, d, b);
            AddTriangle(mesh, c, b, a);
        }
    }
    return mesh;
}
} // namespace dl
