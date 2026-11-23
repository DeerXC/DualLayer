// test_water.cpp - tests of what water does (no window, no GPU): src/scene/WaterFx.h and the materials of the water in
// src/scene/Stage.h, Canal.h and Landscape.h.
//   g++ -std=c++20 -O2 -Wall -Wextra -I src tests/test_water.cpp src/render/MeshData.cpp -o test_water
//   MSVC (Developer PowerShell):  .\tests\run_tests.ps1 -Only water
// What is checked:
//   1. materials: every drawn water is a water material of layer A (the sea of both); running water is kMatFlow; nothing of
//      the solid map and no drawn box of the city is a water material
//   2. the sheets: one for every water and the canal, at the height the Waterworks and the lock give; the surface at a point;
//      under water or not
//   3. foam: a rim inside every sheet that holds water, just over its surface; the pad under the fall of the aqueduct
//   4. cascades: three falls between the locks while they hold water, from the upper surface down to the lower; none drained
//   5. the wet band: none while water rises; when it goes down a band from the new surface up to the old one, fading over
//      kDrySeconds, then gone; covered again when it rises
//   6. splashes: rings on the surface, drops over it that fall back, gone after kSplashSeconds; at most kMax at once
//   7. everything fits its place in the drawn table (kWaterFxDraw)
// Returns 1 when a check fails.

#include "scene/Canal.h"
#include "scene/CityState.h"
#include "scene/Landscape.h"
#include "scene/Stage.h"
#include "scene/WaterFx.h"

#include <cmath>
#include <cstdio>
#include <vector>

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
namespace st = dl::stage;

bool Near(float a, float b, float eps)
{
    return std::fabs(a - b) <= eps;
}

dl::canal::Canal MakeCanal()
{
    dl::canal::Canal c;
    c.Init(st::MakeCanal());
    return c;
}

void TestMaterials()
{
    st::Waterworks works;
    static SceneObject table[128] = {};
    const int n = st::AddWaterDraw(works, table, 0, 128);
    int water = 0;
    int flow = 0;
    for (int i = 0; i < n; ++i)
    {
        if (IsWaterMaterial(table[i].checker))
        {
            ++water;
            CHECK(table[i].layerMask == kLayerA && table[i].glow > 0.5f);
            flow += Near(table[i].checker, kMatFlow, 1e-4f) ? 1 : 0;
        }
    }
    CHECK(water >= st::kWaterCount + 3 && flow == 3);
    dl::canal::Canal canal = MakeCanal();
    const int c = canal.AddDraw(table, 0, 128);
    bool canalWater = false;
    for (int i = 0; i < c; ++i)
    {
        canalWater = canalWater || Near(table[i].checker, kMatWater, 1e-4f);
    }
    CHECK(canalWater);
    SceneObject sea[2] = {};
    CHECK(st::AddSea({0.0f, 0.0f, 0.0f}, sea, 0, 2) == 2 && Near(sea[0].checker, kMatSea, 1e-4f) && Near(sea[1].checker, kMatSea, 1e-4f));
    // nothing of the map is drawn as water by mistake (a checker of 2 or more)
    for (const SceneObject& o : st::kStageStatics)
    {
        CHECK(!IsWaterMaterial(o.checker));
    }
    int decorWater = 0;
    for (const SceneObject& o : st::kDecor)
    {
        decorWater += IsWaterMaterial(o.checker) ? 1 : 0;
    }
    CHECK(decorWater == 0);
}

void TestSheets()
{
    st::Waterworks works;
    dl::canal::Canal canal = MakeCanal();
    st::WaterSheet sheets[st::kMaxSheets] = {};
    const int n = st::CollectWaters(works, canal, sheets, st::kMaxSheets);
    int waters = 0;
    st::ForEachWater([&](const dl::shapes::Water&) { ++waters; });
    CHECK(n == waters + 1 && n <= st::kMaxSheets);
    CHECK(Near(sheets[n - 1].top, canal.WaterTop(), 1e-5f));
    // the pool: its surface at its middle, under it is under water, over it is not; in B there is no water
    const dl::shapes::Water& pool = st::kWaters[st::kPoolWater];
    const float x = 0.5f * (pool.x0 + pool.x1) + 3.0f;
    const float z = 0.5f * (pool.z0 + pool.z1);
    float top = 0.0f;
    CHECK(st::WaterTopAt(sheets, n, x, z, pool.top, top) && Near(top, pool.top, 1e-5f));
    CHECK(st::Underwater(sheets, n, {x, pool.top - 0.5f, z}, 0) && !st::Underwater(sheets, n, {x, pool.top + 0.5f, z}, 0) &&
          !st::Underwater(sheets, n, {x, pool.top - 0.5f, z}, 1));
    CHECK(!st::WaterTopAt(sheets, n, 500.0f, 500.0f, 0.0f, top));
    // raised by the dawn bell: the sheets follow
    works.SetRaise(st::kDawnRise);
    st::CollectWaters(works, canal, sheets, st::kMaxSheets);
    CHECK(st::WaterTopAt(sheets, n, x, z, pool.top, top) && Near(top, pool.top + st::kDawnRise, 1e-5f));
}

void TestFoamAndFalls()
{
    st::Waterworks works;
    dl::canal::Canal canal = MakeCanal();
    st::WaterSheet sheets[st::kMaxSheets] = {};
    const int n = st::CollectWaters(works, canal, sheets, st::kMaxSheets);
    std::vector<SceneObject> out(static_cast<std::size_t>(st::kWaterFxDraw));
    const int foam = st::AddFoam(sheets, n, true, out.data(), 0, st::kWaterFxDraw);
    bool inside = true;
    for (int i = 0; i + 1 < foam; ++i) // (the last one is the pad under the fall)
    {
        const SceneObject& f = out[static_cast<std::size_t>(i)];
        bool onSheet = false;
        for (int k = 0; k < n; ++k)
        {
            const st::WaterSheet& s = sheets[k];
            onSheet = onSheet || (f.position[0] - 0.5f * f.scale[0] >= s.x0 - 1e-4f && f.position[0] + 0.5f * f.scale[0] <= s.x1 + 1e-4f &&
                                  f.position[2] - 0.5f * f.scale[2] >= s.z0 - 1e-4f && f.position[2] + 0.5f * f.scale[2] <= s.z1 + 1e-4f &&
                                  Near(f.position[1] + 0.5f * f.scale[1], s.top + st::kFoamOver, 1e-4f));
        }
        inside = inside && onSheet && Near(f.checker, kMatFoam, 1e-4f) && f.layerMask == kLayerA;
    }
    CHECK(foam > 4 * 5 && inside);
    CHECK(st::AddFoam(sheets, n, false, out.data(), 0, st::kWaterFxDraw) == foam - 1); // no fall: no pad
    // three falls between the locks, each a sheet from the upper surface down to the lower and foam at its foot
    const int falls = st::AddCascades(works, out.data(), 0, st::kWaterFxDraw);
    CHECK(falls == 2 * st::kLockCount);
    for (int k = 0; k + 1 < st::kLockCount; ++k)
    {
        const SceneObject& sheet = out[static_cast<std::size_t>(2 * k)];
        const float top = sheet.position[1] + 0.5f * sheet.scale[1];
        const float bottom = sheet.position[1] - 0.5f * sheet.scale[1];
        CHECK(Near(sheet.checker, kMatFlow, 1e-4f) && Near(top, st::kLockWaters[k + 1].top - 0.15f, 1e-4f) &&
              Near(bottom, st::kLockWaters[k].top - 0.05f, 1e-4f) && Near(sheet.position[0] + 0.5f * sheet.scale[0], st::kTerraceX1[k], 1e-4f));
    }
    // drained at the end: no falls
    works.Close();
    works.Update(100.0f);
    CHECK(st::AddCascades(works, out.data(), 0, st::kWaterFxDraw) == 0);
}

void TestWetBand()
{
    st::Waterworks works;
    dl::canal::Canal canal = MakeCanal();
    st::WaterSheet sheets[st::kMaxSheets] = {};
    st::WetMarks wet;
    SceneObject bands[st::kMaxSheets] = {};
    int n = st::CollectWaters(works, canal, sheets, st::kMaxSheets);
    wet.Update(sheets, n, 0.1f);
    CHECK(wet.AddDraw(sheets, n, bands, 0, st::kMaxSheets) == 0);
    // rising (the dawn bell): no band
    works.SetRaise(st::kDawnRise);
    n = st::CollectWaters(works, canal, sheets, st::kMaxSheets);
    wet.Update(sheets, n, 0.1f);
    CHECK(wet.AddDraw(sheets, n, bands, 0, st::kMaxSheets) == 0);
    // draining: a band from the new surface up to the old one on every water that held some
    const float poolTop = sheets[st::kPoolWater].top;
    works.Close();
    for (int i = 0; i < 20; ++i)
    {
        works.Update(0.5f);
        n = st::CollectWaters(works, canal, sheets, st::kMaxSheets);
        wet.Update(sheets, n, 0.5f);
    }
    const int count = wet.AddDraw(sheets, n, bands, 0, st::kMaxSheets);
    CHECK(count >= st::kWaterCount);
    bool poolBand = false;
    for (int i = 0; i < count; ++i)
    {
        const SceneObject& b = bands[i];
        CHECK(Near(b.checker, kMatWetBand, 1e-4f) && b.glow > 0.0f && b.glow < 1.0f);
        poolBand = poolBand || Near(b.position[1] + 0.5f * b.scale[1], poolTop, 1e-3f);
    }
    CHECK(poolBand);
    const float early = wet.Wetness(st::kPoolWater);
    for (int i = 0; i < 20; ++i)
    {
        wet.Update(sheets, n, 0.5f);
    }
    CHECK(wet.Wetness(st::kPoolWater) < early);
    for (int i = 0; i < 100; ++i)
    {
        wet.Update(sheets, n, 0.5f);
    }
    CHECK(wet.Wetness(st::kPoolWater) == 0.0f && wet.AddDraw(sheets, n, bands, 0, st::kMaxSheets) == 0);
}

void TestSplashes()
{
    st::Splashes s;
    CHECK(s.Live() == 0);
    const Vec3 at{2.0f, -2.5f, 30.0f};
    s.Spawn(at);
    s.Update(0.3f);
    CHECK(s.Live() == 1);
    std::vector<SceneObject> out(static_cast<std::size_t>(st::kWaterFxDraw));
    const int n = s.AddDraw(out.data(), 0, st::kWaterFxDraw);
    int rings = 0;
    int drops = 0;
    for (int i = 0; i < n; ++i)
    {
        const SceneObject& o = out[static_cast<std::size_t>(i)];
        CHECK(Near(o.checker, kMatFoam, 1e-4f) && o.layerMask == kLayerA && o.glow > 0.0f);
        if (o.shape == ShapeKind::Sphere)
        {
            ++drops;
            CHECK(o.position[1] >= at.y - 0.05f);
        }
        else
        {
            ++rings;
            CHECK(Near(o.position[1] + 0.5f * o.scale[1], at.y + st::kFoamOver, 1e-4f));
        }
    }
    CHECK(rings > 0 && drops > 0 && n <= st::kSplashDraw);
    s.Update(st::kSplashSeconds);
    CHECK(s.Live() == 0 && s.AddDraw(out.data(), 0, st::kWaterFxDraw) == 0);
    for (int i = 0; i < st::Splashes::kMax + 2; ++i)
    {
        s.Spawn(at);
    }
    CHECK(s.Live() == st::Splashes::kMax);
}

void TestFits()
{
    st::Waterworks works;
    dl::canal::Canal canal = MakeCanal();
    st::WaterSheet sheets[st::kMaxSheets] = {};
    st::WetMarks wet;
    st::Splashes splashes;
    for (int i = 0; i < st::Splashes::kMax; ++i)
    {
        splashes.Spawn({0.0f, -2.5f, 30.0f});
    }
    splashes.Update(0.2f);
    const int n = st::CollectWaters(works, canal, sheets, st::kMaxSheets);
    std::vector<SceneObject> out(static_cast<std::size_t>(st::kWaterFxDraw));
    const int count = st::AddWaterFx(sheets, n, works, wet, splashes, out.data(), 0, st::kWaterFxDraw);
    std::printf("  water effects: %d drawn objects (room for %d)\n", count, st::kWaterFxDraw);
    CHECK(count > 0 && count <= st::kWaterFxDraw);
    CHECK(st::AddWaterFx(sheets, n, works, wet, splashes, out.data(), st::kWaterFxDraw, st::kWaterFxDraw) == st::kWaterFxDraw); // no room
}

} // namespace

int main()
{
    TestMaterials();
    TestSheets();
    TestFoamAndFalls();
    TestWetBand();
    TestSplashes();
    TestFits();
    std::printf("%d checks passed, %d failed\n", g_passed, g_failed);
    return g_failed == 0 ? 0 : 1;
}
