// test_batch.cpp - tests of the vertex of the static batch and the joining of meshes (no window, no GPU):
// src/render/StaticBatch.h. The baking of the town itself is tested in tests/test_town.cpp.
//   g++ -std=c++20 -O2 -Wall -Wextra -I src tests/test_batch.cpp -o test_batch
//   MSVC (Developer PowerShell):  .\tests\run_tests.ps1 -Only batch
// What is checked: the vertex is 56 bytes (what the input layout of the renderer assumes); sRGB to linear at the ends
// and in the middle and on both sides of the knee of the curve; joining two meshes moves the indices of the second past
// the vertices of the first and adds the counts; joining nothing changes nothing.
// Returns 1 when a check fails.

#include "check.h"
#include "render/StaticBatch.h"

#include <cmath>
#include <cstdio>

namespace
{

bool Near(float a, float b, float eps)
{
    return std::fabs(a - b) <= eps;
}

void TestSrgb()
{
    using dl::batch_detail::SrgbToLinear;
    CHECK(SrgbToLinear(0.0f) == 0.0f && Near(SrgbToLinear(1.0f), 1.0f, 1e-6f));
    CHECK(Near(SrgbToLinear(0.5f), 0.21404f, 1e-4f));        // the middle of the curve
    CHECK(Near(SrgbToLinear(0.04f), 0.04f / 12.92f, 1e-7f)); // below the knee: a straight line
    CHECK(Near(SrgbToLinear(0.05f), std::pow((0.05f + 0.055f) / 1.055f, 2.4f), 1e-7f)); // above it: the power curve
    CHECK(SrgbToLinear(0.2f) < SrgbToLinear(0.3f));                                     // and it only grows
}

dl::BatchMesh Quad(float x)
{
    dl::BatchMesh m;
    for (int i = 0; i < 4; ++i)
    {
        dl::BatchVertex v{};
        v.position[0] = x + static_cast<float>(i);
        m.vertices.push_back(v);
    }
    for (const std::uint32_t i : {0u, 1u, 2u, 0u, 2u, 3u})
    {
        m.indices.push_back(i);
    }
    m.objects = 1;
    return m;
}

void TestAppend()
{
    dl::BatchMesh joined = Quad(0.0f);
    dl::AppendMesh(joined, Quad(10.0f));
    CHECK(joined.vertices.size() == 8 && joined.indices.size() == 12 && joined.objects == 2);
    CHECK(joined.indices[5] == 3 && joined.indices[6] == 4 &&
          joined.indices[11] == 7); // the second one moved past the first
    CHECK(joined.vertices[4].position[0] == 10.0f);
    dl::AppendMesh(joined, dl::BatchMesh{});
    CHECK(joined.vertices.size() == 8 && joined.indices.size() == 12 && joined.objects == 2);
    dl::BatchMesh empty;
    dl::AppendMesh(empty, Quad(0.0f)); // into nothing: a copy
    CHECK(empty.vertices.size() == 4 && empty.indices[0] == 0 && empty.indices[5] == 3);
    CHECK(sizeof(dl::BatchVertex) == 56);
}

} // namespace

int main()
{
    TestSrgb();
    TestAppend();
    std::printf("%d checks passed, %d failed\n", g_passed, g_failed);
    return g_failed == 0 ? 0 : 1;
}
