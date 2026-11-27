// test_map.cpp - tests of the map of two sheets of tracing paper (no window, no GPU): src/scene/MapMemory.h,
// src/render/MapView.h and the plan of the town file (src/scene/Town.h Plan). Reads data/town.dlt (python
// tools/layout/city_model.py export) for the plan, and a small plan made here for the colours.
//   g++ -std=c++20 -O2 -Wall -Wextra -I src tests/test_map.cpp -o test_map
//   MSVC (Developer PowerShell):  .\tests\run_tests.ps1 -Only map
// What is checked:
//   1. the plan of the model: the size, the classes, the sea in both moments, the moat of A that is not in B, the
//      streets of the model under their middle lines, the two moments differing in a few hundred cells and not in most
//      of them
//   2. what is seen: the body sees the cells within the radius and no others; the overlooks reveal far, but only to a
//      body that stands at them; nothing outside the plan is marked; the box of what is seen
//   3. the markers: placed, taken away where they stand, never more than ten (the eleventh takes the oldest place), the
//      six shapes and the moment each was set in are kept, no bad numbers
//   4. the paper: it is inside every screen; the whole plan is in the window from the first frame (and the player and
//      the tower with it); zoom and pan stay inside the plan; a point of the screen and a place of the plan are turned
//      into each other both ways; the buttons are on the screen and are found by a point
//   5. the ink: in both moments the same is neutral, in one moment only it is warm (A) or cool (B), where both have
//      something the sheet on top shows; what is not seen is blank paper; the lamps of the tower are lit or dark; the
//      arrow points where the camera looks; the marker shapes and the lantern are drawn; nothing leaves the screen and
//      the batch is never full
//   6. the fog and the shapes: the whole town is under fog at the start and nothing of the unseen is inked; the edge of
//      an ink is smooth (a round building has the area of a circle, not of a staircase); the rings of the puzzles to
//      solve show in the fog and go when solved; the cached layer is made again only when what it shows changes
// Returns 1 when a check fails.

#include "check.h"
#include "render/MapView.h"
#include "scene/MapMemory.h"
#include "scene/Town.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace
{

using dl::MapFrame;
using dl::MapScene;
using dl::OverlayBatch;
using dl::Vec3;
namespace tn = dl::town;

bool Near(float a, float b, float eps)
{
    return std::fabs(a - b) <= eps;
}

const tn::TownData& Town()
{
    static tn::TownData data;
    static bool loaded = false;
    if (!loaded)
    {
        std::string error;
        loaded = data.Load(tn::kPath, error);
        if (!loaded)
        {
            std::printf("  cannot load %s: %s\n", tn::kPath, error.c_str());
        }
    }
    return data;
}

int ClassAt(const tn::Plan& plan, int world, float x, float z)
{
    return plan.At(world, plan.CellX(x), plan.CellZ(z));
}

void TestPlan()
{
    const tn::TownData& t = Town();
    const tn::Plan& plan = t.plan;
    CHECK(plan.Valid() && plan.nx == 400 && plan.nz == 404 && Near(plan.cell, 0.5f, 1e-6f));
    int counts[2][tn::kPlanClassCount] = {};
    int differ = 0;
    for (int j = 0; j < plan.nz; ++j)
    {
        for (int i = 0; i < plan.nx; ++i)
        {
            counts[0][plan.At(0, i, j)]++;
            counts[1][plan.At(1, i, j)]++;
            differ += plan.At(0, i, j) != plan.At(1, i, j) ? 1 : 0;
        }
    }
    std::printf("  plan: A water %d cliff %d building %d street %d; B water %d cliff %d building %d street %d; %d "
                "cells differ\n",
                counts[0][1], counts[0][2], counts[0][3], counts[0][4], counts[1][1], counts[1][2], counts[1][3],
                counts[1][4], differ);
    for (int w = 0; w < 2; ++w)
    {
        CHECK(counts[w][tn::kPlanWater] > 2000 && counts[w][tn::kPlanBuilding] > 6000 &&
              counts[w][tn::kPlanStreet] > 4000 && counts[w][tn::kPlanCliff] > 800);
    }
    // the moments differ in a few thousand cells and agree in most (3.5% of the plan at pass 6). The lower bound was
    // 6000 and the town of pass 5 had 6109; the smooth ground, the court and the rooms of pass 6 move the shore and
    // the footprints, and it has 5726. So the bound is 5000: what is checked is that the sheets are not the same,
    // and that they are not different everywhere.
    CHECK(differ > 5000 && differ < 8000);
    // the sea, in the basin of the harbour, is water in both moments
    CHECK(ClassAt(plan, 0, 4.0f, -86.0f) == tn::kPlanWater && ClassAt(plan, 1, 4.0f, -86.0f) == tn::kPlanWater);
    // the moat round the rock (south side, clear of the lift): water in A, not in B
    CHECK(ClassAt(plan, 0, 2.0f, -19.0f) == tn::kPlanWater && ClassAt(plan, 1, 2.0f, -19.0f) != tn::kPlanWater);
    // the plaza of the rock and the quay are streets (a floor to walk) in both moments
    CHECK(ClassAt(plan, 0, 8.0f, -3.0f) == tn::kPlanStreet && ClassAt(plan, 1, 8.0f, -3.0f) == tn::kPlanStreet);
    CHECK(ClassAt(plan, 0, t.spawn.x, t.spawn.z + 2.0f) == tn::kPlanStreet);
    // the streets of the model lie on streets (or under what is built over them) along their middle lines, in A
    int onStreet = 0;
    int total = 0;
    for (const tn::Street& s : t.streets)
    {
        if (s.kind == 1 || s.name == "headland path")
        {
            continue; // a stair is a run of treads: not asked here
        }
        for (const Vec3& p : s.line)
        {
            ++total;
            const int c = ClassAt(plan, 0, p.x, p.z);
            onStreet += (c == tn::kPlanStreet || c == tn::kPlanBuilding) ? 1 : 0;
        }
    }
    CHECK(total > 40 && onStreet >= total * 9 / 10);
    // the town wall is drawn, and what is far outside it is blank
    CHECK(ClassAt(plan, 0, 0.0f, 80.0f) != tn::kPlanBuilding);
    CHECK(t.overlooks.size() >= 4 && t.emblems.size() == 3);
    for (const tn::Overlook& o : t.overlooks)
    {
        CHECK(o.radius > 20.0f && o.at.x > plan.x0 && o.at.x < plan.x0 + static_cast<float>(plan.nx) * plan.cell);
    }
}

void TestMemory()
{
    const tn::TownData& t = Town();
    tn::MapMemory m;
    m.Init(t.plan, 15.0f);
    CHECK(m.SeenCount() == 0 && !m.Seen(10, 10));
    int i0 = 0, i1 = 0, j0 = 0, j1 = 0;
    CHECK(!m.SeenBounds(i0, i1, j0, j1));
    // the spawn: a disk of 15 m: about pi r^2 / cell^2 = 707 cells, and every one within the radius
    m.Update(t, {t.spawn.x, t.spawn.y, t.spawn.z});
    CHECK(m.SeenCount() > 2500 && m.SeenCount() < 3000);
    CHECK(m.Seen(t.plan.CellX(t.spawn.x), t.plan.CellZ(t.spawn.z)));
    CHECK(!m.Seen(t.plan.CellX(t.spawn.x), t.plan.CellZ(t.spawn.z + 25.0f)));
    bool near = true;
    for (int j = 0; j < t.plan.nz; ++j)
    {
        for (int i = 0; i < t.plan.nx; ++i)
        {
            if (m.Seen(i, j))
            {
                const float dx = t.plan.CentreX(i) - t.spawn.x;
                const float dz = t.plan.CentreZ(j) - t.spawn.z;
                near = near && dx * dx + dz * dz <= 15.0f * 15.0f + 1e-3f;
            }
        }
    }
    CHECK(near);
    CHECK(m.SeenBounds(i0, i1, j0, j1) && i0 <= t.plan.CellX(t.spawn.x) && i1 >= t.plan.CellX(t.spawn.x) && j0 < j1);
    // the same place again adds nothing; the radius can be changed
    const int before = m.SeenCount();
    m.Update(t, {t.spawn.x, t.spawn.y, t.spawn.z});
    CHECK(m.SeenCount() == before);
    m.SetRevealRadius(1000.0f);
    CHECK(Near(m.RevealRadius(), 80.0f, 1e-6f));
    m.SetRevealRadius(0.0f);
    CHECK(Near(m.RevealRadius(), 3.0f, 1e-6f));
    // an overlook reveals far, to a body that stands at it and to no other
    tn::MapMemory v;
    v.Init(t.plan, 15.0f);
    const tn::Overlook& o = t.overlooks.front();
    v.Update(t, {o.at.x + 30.0f, o.at.y, o.at.z}); // near but not at it
    const int away = v.SeenCount();
    v.Update(t, {o.at.x + 2.0f, o.at.y, o.at.z});
    CHECK(v.SeenCount() > away + 4000);
    tn::MapMemory low;
    low.Init(t.plan, 15.0f);
    low.Update(t, {o.at.x, o.at.y - 20.0f, o.at.z}); // under it (the foot of the bastion): not looking over
    CHECK(low.SeenCount() < 4000);
    // nothing outside the plan is marked, a bad number is ignored
    tn::MapMemory edge;
    edge.Init(t.plan, 15.0f);
    edge.Reveal(-5000.0f, -5000.0f, 30.0f);
    edge.Reveal(std::nanf(""), 0.0f, 30.0f);
    edge.Reveal(0.0f, 0.0f, -4.0f);
    CHECK(edge.SeenCount() == 0 && !edge.Seen(-1, 0) && !edge.Seen(0, 100000));
    edge.Reveal(t.plan.x0, t.plan.z0, 10.0f); // the corner: only the cells inside the plan
    CHECK(edge.SeenCount() > 80 && edge.SeenCount() < 640);
}

void TestMarkers()
{
    const tn::TownData& t = Town();
    constexpr int kMax = tn::MapMemory::kMaxMarkers;
    CHECK(kMax == 10 && tn::MapMemory::kMarkerTypes == 6);
    tn::MapMemory m;
    m.Init(t.plan, 15.0f);
    CHECK(m.ToggleMarker({10.0f, 0.0f, 10.0f}, 3.0f) && m.MarkerCount() == 1);
    CHECK(!m.ToggleMarker({11.0f, 0.0f, 10.5f}, 3.0f) && m.MarkerCount() == 0); // where one stands: taken away
    for (int k = 0; k < kMax; ++k)
    {
        CHECK(m.ToggleMarker({20.0f * static_cast<float>(k), 0.0f, 0.0f, k % 6, k % 2}, 3.0f));
    }
    CHECK(m.MarkerCount() == kMax);
    // the shape and the moment are kept
    CHECK(m.Marker(0).type == 0 && m.Marker(0).layer == 0 && m.Marker(3).type == 3 && m.Marker(3).layer == 1 &&
          m.Marker(5).type == 5);
    CHECK(m.ToggleMarker({-50.0f, 1.0f, 3.0f, 2, 1}, 3.0f) &&
          m.MarkerCount() == kMax); // the eleventh takes the oldest place
    CHECK(Near(m.Marker(0).x, 20.0f, 1e-6f) && Near(m.Marker(kMax - 1).x, -50.0f, 1e-6f) &&
          Near(m.Marker(kMax - 1).y, 1.0f, 1e-6f) && m.Marker(kMax - 1).type == 2 && m.Marker(kMax - 1).layer == 1);
    CHECK(!m.ToggleMarker({std::nanf(""), 0.0f, 0.0f}, 3.0f) && m.MarkerCount() == kMax);
    CHECK(!m.ToggleMarker({-50.0f, 0.0f, 3.5f}, 3.0f) &&
          m.MarkerCount() == kMax - 1); // the nearest is the one taken away
    // a shape or a moment out of range is pulled into range
    CHECK(m.ToggleMarker({-90.0f, 0.0f, 90.0f, 99, 7}, 3.0f) && m.Marker(m.MarkerCount() - 1).type == 5 &&
          m.Marker(m.MarkerCount() - 1).layer == 0);
    CHECK(m.ToggleMarker({-95.0f, 0.0f, 95.0f, -4, -1}, 3.0f) && m.Marker(m.MarkerCount() - 1).type == 0 &&
          m.Marker(m.MarkerCount() - 1).layer == 0);
    m.ClearMarkers();
    CHECK(m.MarkerCount() == 0);
}

void TestFrame()
{
    const tn::TownData& t = Town();
    tn::MapMemory m;
    m.Init(t.plan, 15.0f);
    m.Update(t, t.spawn);
    MapScene s;
    s.town = &t;
    s.memory = &m;
    s.player = t.spawn;
    Vec3 mast;
    CHECK(t.MastAt(mast));
    for (const auto& [w, h] : {std::pair<int, int>{1600, 900}, {1280, 720}, {1024, 768}, {800, 800}, {2560, 1600}})
    {
        const MapFrame f = dl::ComputeMapFrame(w, h, s);
        CHECK(f.valid && f.px >= 0.0f && f.py >= 0.0f && f.px + f.size <= static_cast<float>(w) &&
              f.py + f.size <= static_cast<float>(h) && f.size >= 0.7f * static_cast<float>(std::min(w, h)));
        CHECK(f.extent >= dl::map_detail::kMinExtent - 1e-3f);
        // the player and the tower are on the paper
        float sx = 0.0f;
        float sy = 0.0f;
        f.ToScreen(s.player.x, s.player.z, sx, sy);
        CHECK(f.Inside(sx, sy));
        f.ToScreen(mast.x, mast.z, sx, sy);
        CHECK(f.Inside(sx, sy));
        // north is up: a place further north is higher on the screen; east is to the right
        float ax = 0.0f, ay = 0.0f, bx = 0.0f, by = 0.0f;
        f.ToScreen(0.0f, 0.0f, ax, ay);
        f.ToScreen(10.0f, 10.0f, bx, by);
        CHECK(by < ay && bx > ax);
        // a point of the paper and a place of the plan are each other's
        float wx = 0.0f;
        float wz = 0.0f;
        CHECK(f.ToWorld(f.px + 0.5f * f.size, f.py + 0.25f * f.size, wx, wz));
        f.ToScreen(wx, wz, sx, sy);
        CHECK(Near(sx, f.px + 0.5f * f.size, 1e-2f) && Near(sy, f.py + 0.25f * f.size, 1e-2f));
        CHECK(!f.ToWorld(f.px - 5.0f, f.py, wx, wz) && !f.ToWorld(f.px, f.py + f.size + 5.0f, wx, wz));
    }
    CHECK(!dl::ComputeMapFrame(0, 900, s).valid && !dl::ComputeMapFrame(1600, 900, MapScene{}).valid);
    // the whole plan is in the window from the first frame, and it does not grow with a walk
    const tn::Plan& plan = t.plan;
    const float west = plan.x0;
    const float east = plan.x0 + static_cast<float>(plan.nx) * plan.cell;
    const float south = plan.z0;
    const float north = plan.z0 + static_cast<float>(plan.nz) * plan.cell;
    const MapFrame whole = dl::ComputeMapFrame(1600, 900, s);
    CHECK(whole.wx0 <= west && whole.wx0 + whole.extent >= east && whole.wz1 >= north &&
          whole.wz1 - whole.extent <= south);
    tn::MapMemory far;
    far.Init(t.plan, 15.0f);
    far.Update(t, t.spawn);
    far.Update(t, {-30.0f, 0.0f, 0.0f});
    s.memory = &far;
    s.player = {-30.0f, 0.0f, 0.0f};
    const MapFrame after = dl::ComputeMapFrame(1600, 900, s);
    CHECK(Near(after.extent, whole.extent, 1e-3f) && Near(after.wx0, whole.wx0, 1e-3f) &&
          Near(after.wz1, whole.wz1, 1e-3f));
    s.memory = &m;
    s.player = t.spawn;
    // zoom: smaller window, never below the closest, never beyond the plan; a bad number is no zoom
    float lastExtent = whole.extent;
    for (const float z : {1.5f, 2.0f, 3.0f, 4.0f, 9.0f})
    {
        s.zoom = z;
        const MapFrame zoomed = dl::ComputeMapFrame(1600, 900, s);
        CHECK(zoomed.extent < lastExtent + 1e-3f && zoomed.extent >= dl::map_detail::kMinZoomExtent - 1e-3f);
        CHECK(Near(zoomed.extent, whole.extent / std::min(z, 4.0f), 1.0f));
        lastExtent = zoomed.extent;
        // pans far to every side keep the window inside the plan
        for (const auto& [px, pz] :
             {std::pair<float, float>{500.0f, 0.0f}, {-500.0f, 0.0f}, {0.0f, 500.0f}, {0.0f, -500.0f}})
        {
            s.panX = px;
            s.panZ = pz;
            const MapFrame moved = dl::ComputeMapFrame(1600, 900, s);
            CHECK(moved.wx0 >= west - 2.1f && moved.wx0 + moved.extent <= east + 2.1f && moved.wz1 <= north + 2.1f &&
                  moved.wz1 - moved.extent >= south - 2.1f);
        }
        s.panX = 0.0f;
        s.panZ = 0.0f;
    }
    s.zoom = std::nanf("");
    CHECK(Near(dl::ComputeMapFrame(1600, 900, s).extent, whole.extent, 1e-3f));
    s.zoom = 0.2f;
    CHECK(Near(dl::ComputeMapFrame(1600, 900, s).extent, whole.extent, 1e-3f));
    s.zoom = 1.0f;
    // the buttons are on the screen, apart from each other, and are found by a point
    for (const auto& [w, h] : {std::pair<int, int>{1600, 900}, {1280, 720}, {1024, 768}, {800, 800}, {2560, 1600}})
    {
        const MapFrame f = dl::ComputeMapFrame(w, h, s);
        for (int k = 0; k < dl::kOptCount; ++k)
        {
            float x = 0.0f, y = 0.0f, bw = 0.0f, bh = 0.0f;
            dl::MapOptionRect(f, k, x, y, bw, bh);
            CHECK(x >= 0.0f && y >= 0.0f && x + bw <= static_cast<float>(w) && y + bh <= static_cast<float>(h));
            CHECK(dl::MapOptionAt(f, x + 0.5f * bw, y + 0.5f * bh) == k);
            if (k > 0)
            {
                float x0 = 0.0f, y0 = 0.0f, w0 = 0.0f, h0 = 0.0f;
                dl::MapOptionRect(f, k - 1, x0, y0, w0, h0);
                CHECK(y >= y0 + h0);
            }
        }
        CHECK(dl::MapOptionAt(f, 2.0f, 2.0f) == -1);
    }
}

/// The colour that shows at a point of the screen: the last triangle of the batch that covers it.
bool ColourAt(const OverlayBatch& b, float x, float y, dl::OverlayColor& out)
{
    bool found = false;
    const dl::OverlayVertex* v = b.Data();
    for (int i = 0; i + 2 < b.Count(); i += 3)
    {
        const float d =
            (v[i + 1].y - v[i + 2].y) * (v[i].x - v[i + 2].x) + (v[i + 2].x - v[i + 1].x) * (v[i].y - v[i + 2].y);
        if (std::fabs(d) < 1.0e-6f)
        {
            continue;
        }
        const float a =
            ((v[i + 1].y - v[i + 2].y) * (x - v[i + 2].x) + (v[i + 2].x - v[i + 1].x) * (y - v[i + 2].y)) / d;
        const float c = ((v[i + 2].y - v[i].y) * (x - v[i + 2].x) + (v[i].x - v[i + 2].x) * (y - v[i + 2].y)) / d;
        if (a >= 0.0f && c >= 0.0f && a + c <= 1.0f)
        {
            out = {v[i].r, v[i].g, v[i].b, v[i].a};
            found = true;
        }
    }
    return found;
}

/// A town with a plan of 80 x 80 cells of 1 m, made of blocks of 10 x 10 cells (the block (i, j) is the cells 10 i + 1
/// .. 10 i + 8 of 10 j + 1 .. 10 j + 8), and a mast with three emblems, for the colours.
struct Little
{
    tn::TownData town;
    tn::MapMemory memory;
    static constexpr int kN = 80;
    static float BlockX(int i)
    {
        return 10.0f * static_cast<float>(i) + 5.0f;
    }
    Little()
    {
        town.plan.cell = 1.0f;
        town.plan.x0 = 0.0f;
        town.plan.z0 = 0.0f;
        town.plan.nx = kN;
        town.plan.nz = kN;
        town.plan.sheet[0].assign(static_cast<std::size_t>(kN * kN), 0);
        town.plan.sheet[1].assign(static_cast<std::size_t>(kN * kN), 0);
        Set(1, 1, tn::kPlanBuilding, tn::kPlanBuilding); // the same in both moments: neutral
        Set(2, 1, tn::kPlanBuilding, 0);                 // in A only: warm
        Set(3, 1, 0, tn::kPlanBuilding);                 // in B only: cool
        Set(4, 1, tn::kPlanStreet, tn::kPlanBuilding);   // both, and different: the sheet on top shows
        Set(5, 1, tn::kPlanWater, tn::kPlanWater);       // neutral water
        Set(6, 6, tn::kPlanBuilding, tn::kPlanBuilding); // not seen
        for (int k = 0; k < 3; ++k)
        {
            tn::Emblem e;
            e.at = {40.0f + 4.0f * static_cast<float>(k), 60.0f, 60.0f};
            town.emblems.push_back(e);
        }
        memory.Init(town.plan, 15.0f);
        for (int j = 0; j < kN; ++j)
        {
            for (int i = 0; i < kN; ++i)
            {
                if (!(i >= 60 && i < 70 && j >= 60 && j < 70)) // the block (6, 6) stays unseen
                {
                    memory.Reveal(town.plan.CentreX(i), town.plan.CentreZ(j), 0.5f);
                }
            }
        }
    }
    void Set(int bi, int bj, int a, int b)
    {
        for (int j = 10 * bj + 1; j <= 10 * bj + 8; ++j)
        {
            for (int i = 10 * bi + 1; i <= 10 * bi + 8; ++i)
            {
                town.plan.sheet[0][static_cast<std::size_t>(j * kN + i)] = static_cast<std::uint8_t>(a);
                town.plan.sheet[1][static_cast<std::size_t>(j * kN + i)] = static_cast<std::uint8_t>(b);
            }
        }
    }
};

void TestInk()
{
    static Little little;
    static OverlayBatch batch;
    MapScene s;
    s.town = &little.town;
    s.memory = &little.memory;
    s.player = {20.0f, 0.0f, 20.0f};
    s.yaw = 0.0f;
    const int w = 1600;
    const int h = 900;
    const auto cellAt = [&](const MapFrame& f, int i, int j, float& x, float& y)
    { f.ToScreen(Little::BlockX(i), Little::BlockX(j), x, y); };
    for (int top = 0; top < 2; ++top)
    {
        s.top = top;
        const MapFrame f = dl::ComputeMapFrame(w, h, s);
        batch.Clear();
        dl::BuildMap(batch, w, h, s, f);
        CHECK(!batch.Full() && batch.Count() > 30 && batch.Count() < OverlayBatch::kMaxVertices);
        bool inside = true;
        for (int i = 0; i < batch.Count(); ++i)
        {
            inside = inside && batch.Data()[i].x >= -1.0f && batch.Data()[i].x <= static_cast<float>(w) + 1.0f &&
                     batch.Data()[i].y >= -1.0f && batch.Data()[i].y <= static_cast<float>(h) + 1.0f;
        }
        CHECK(inside);
        float x = 0.0f;
        float y = 0.0f;
        dl::OverlayColor c;
        cellAt(f, 1, 1, x, y); // the same in both: a neutral ink (no colour of a moment)
        CHECK(ColourAt(batch, x, y, c) && std::fabs(c.r - c.b) < 0.12f && c.r < 0.4f);
        cellAt(f, 2, 1, x, y); // A only: warm
        CHECK(ColourAt(batch, x, y, c) && c.r > c.b + 0.25f && c.r < 0.75f);
        cellAt(f, 3, 1, x, y); // B only: cool
        CHECK(ColourAt(batch, x, y, c) && c.b > c.r + 0.25f);
        cellAt(f, 5, 1, x, y); // water in both: neutral, bluish grey
        CHECK(ColourAt(batch, x, y, c) && std::fabs(c.r - c.b) < 0.35f && c.b >= c.r);
        cellAt(f, 4, 1, x, y); // both, different: the sheet on top shows
        CHECK(ColourAt(batch, x, y, c));
        if (top == 0)
        {
            CHECK(c.r > c.b + 0.25f); // A on top: the warm street
        }
        else
        {
            CHECK(c.b > c.r + 0.25f); // B on top: the cool building
        }
        cellAt(f, 6, 6, x, y); // not seen: coloured fog, not blank paper and not the ink of the building under it
        CHECK(ColourAt(batch, x, y, c));
        CHECK(top == 0 ? c.b > c.g : c.g > c.r);
        cellAt(f, 0, 0, x, y); // seen and empty: blank paper too
        CHECK(ColourAt(batch, x, y, c) && Near(c.g, 0.90f, 0.01f));
        // the frame is in the colour of the sheet on top
        CHECK(ColourAt(batch, f.px - 1.5f, f.py + 0.5f * f.size, c));
        CHECK(top == 0 ? c.r > c.b + 0.3f : c.b > c.r + 0.3f);
    }
    // the arrow points where the camera looks: north (yaw 0) is up, east (yaw pi/2) is to the right
    s.top = 0;
    const MapFrame f = dl::ComputeMapFrame(w, h, s);
    float px = 0.0f;
    float py = 0.0f;
    f.ToScreen(s.player.x, s.player.z, px, py);
    const auto arrowReach = [&](float yaw, float& dx, float& dy)
    {
        s.yaw = yaw;
        batch.Clear();
        dl::BuildMap(batch, w, h, s, f);
        float far = 0.0f;
        dx = dy = 0.0f;
        const dl::OverlayVertex* v = batch.Data();
        for (int i = 0; i < batch.Count(); ++i)
        {
            if (Near(v[i].r, 0.60f, 0.001f) && Near(v[i].g, 0.10f, 0.001f) && Near(v[i].b, 0.08f, 0.001f))
            {
                const float ex = v[i].x - px;
                const float ey = v[i].y - py;
                if (ex * ex + ey * ey > far)
                {
                    far = ex * ex + ey * ey;
                    dx = ex;
                    dy = ey;
                }
            }
        }
        return far > 0.0f;
    };
    float dx = 0.0f;
    float dy = 0.0f;
    CHECK(arrowReach(0.0f, dx, dy) && dy < -10.0f && std::fabs(dx) < 2.0f);
    CHECK(arrowReach(1.5707963f, dx, dy) && dx > 10.0f && std::fabs(dy) < 2.0f);
    CHECK(arrowReach(3.1415927f, dx, dy) && dy > 10.0f);
    // the lamps of the tower: dark at first, one lit after (a warm gold square beside two dark ones)
    const auto gold = [&](int& count)
    {
        count = 0;
        const dl::OverlayVertex* v = batch.Data();
        for (int i = 0; i + 5 < batch.Count(); i += 3)
        {
            count += (Near(v[i].r, 1.0f, 0.001f) && Near(v[i].g, 0.80f, 0.001f) && Near(v[i].b, 0.40f, 0.001f)) ? 1 : 0;
        }
    };
    int litCount = 0;
    s.yaw = 0.0f;
    batch.Clear();
    dl::BuildMap(batch, w, h, s, f);
    gold(litCount);
    CHECK(litCount == 0);
    s.lit[1] = true;
    batch.Clear();
    dl::BuildMap(batch, w, h, s, f);
    gold(litCount);
    CHECK(litCount == 2); // one lamp: two triangles of its square
    // the lantern and a marker are drawn
    s.hasLantern = true;
    s.lantern = {50.0f, 0.0f, 20.0f};
    little.memory.ToggleMarker({30.0f, 0.0f, 30.0f, 0, 1}, 3.0f); // a flag set in B
    batch.Clear();
    dl::BuildMap(batch, w, h, s, f);
    int flags = 0;
    int lantern = 0;
    for (int i = 0; i < batch.Count(); ++i)
    {
        const dl::OverlayVertex& vtx = batch.Data()[i];
        flags += (Near(vtx.r, 0.08f, 0.001f) && Near(vtx.g, 0.56f, 0.001f) && Near(vtx.b, 0.88f, 0.001f)) ? 1 : 0;
        lantern += (Near(vtx.r, 1.0f, 0.001f) && Near(vtx.g, 0.78f, 0.001f) && Near(vtx.b, 0.38f, 0.001f)) ? 1 : 0;
    }
    CHECK(flags == 3 && lantern == 6);
    // the six shapes are all drawn, each in the colour of its moment (A warm, B cool)
    little.memory.ClearMarkers();
    for (int type = 0; type < tn::MapMemory::kMarkerTypes; ++type)
    {
        little.memory.ToggleMarker({8.0f + 10.0f * static_cast<float>(type), 0.0f, 30.0f, type, type % 2}, 3.0f);
    }
    batch.Clear();
    dl::BuildMap(batch, w, h, s, f);
    for (int type = 0; type < tn::MapMemory::kMarkerTypes; ++type)
    {
        float mx = 0.0f;
        float my = 0.0f;
        f.ToScreen(8.0f + 10.0f * static_cast<float>(type), 30.0f, mx, my);
        dl::OverlayColor c;
        const float un = f.size / 180.0f;
        CHECK(type == 0 ? ColourAt(batch, mx + 1.5f * un, my - 6.8f * un, c)
                        : ColourAt(batch, mx, my, c)); // a flag's cloth is above the point
        const bool cool = c.b > c.r;
        CHECK(cool == (type % 2 == 1));
    }
    little.memory.ClearMarkers();
    // without a plan or a town: nothing breaks, nothing is drawn
    batch.Clear();
    dl::BuildMap(batch, w, h, MapScene{}, f);
    CHECK(batch.Count() == 0);
}

void TestRealMap()
{
    const tn::TownData& t = Town();
    static OverlayBatch batch;
    tn::MapMemory m;
    m.Init(t.plan, 15.0f);
    // the whole town seen at once: the largest batch the map can make
    for (const tn::Overlook& o : t.overlooks)
    {
        m.Reveal(o.at.x, o.at.z, o.radius);
    }
    m.Reveal(0.0f, 0.0f, 200.0f);
    MapScene s;
    s.town = &t;
    s.memory = &m;
    s.player = {0.0f, 14.0f, -8.0f};
    s.hasLantern = true;
    s.lantern = {-20.0f, 14.0f, 0.0f};
    int most = 0;
    for (const auto& [w, h] : {std::pair<int, int>{1600, 900}, {2560, 1600}, {1024, 768}})
    {
        for (int top = 0; top < 2; ++top)
        {
            s.top = top;
            batch.Clear();
            const MapFrame f = dl::ComputeMapFrame(w, h, s);
            dl::BuildMap(batch, w, h, s, f);
            most = std::max(most, batch.Count());
            CHECK(!batch.Full());
        }
    }
    std::printf("  the whole town on the map: %d of %d vertices\n", most, OverlayBatch::kMaxVertices);
    CHECK(most < OverlayBatch::kMaxVertices * 2 / 3);
}

/// How many vertices have this colour (`a` < 0: whatever the alpha).
int CountColour(const OverlayBatch& b, float r, float g, float bl, float a = -1.0f, float eps = 0.002f)
{
    int n = 0;
    for (int i = 0; i < b.Count(); ++i)
    {
        const dl::OverlayVertex& v = b.Data()[i];
        n += (Near(v.r, r, eps) && Near(v.g, g, eps) && Near(v.b, bl, eps) && (a < 0.0f || Near(v.a, a, eps))) ? 1 : 0;
    }
    return n;
}

/// The area, in square metres of the plan, of the triangles whose three corners all have this colour (r, g, b, a).
double InkArea(const OverlayBatch& b, const MapFrame& f, const dl::OverlayColor& c)
{
    double area = 0.0;
    const dl::OverlayVertex* v = b.Data();
    const auto same = [&](const dl::OverlayVertex& x)
    { return Near(x.r, c.r, 1e-4f) && Near(x.g, c.g, 1e-4f) && Near(x.b, c.b, 1e-4f) && Near(x.a, c.a, 1e-4f); };
    for (int i = 0; i + 2 < b.Count(); i += 3)
    {
        if (same(v[i]) && same(v[i + 1]) && same(v[i + 2]))
        {
            area +=
                0.5 * std::fabs(static_cast<double>(v[i + 1].x - v[i].x) * static_cast<double>(v[i + 2].y - v[i].y) -
                                static_cast<double>(v[i + 2].x - v[i].x) * static_cast<double>(v[i + 1].y - v[i].y));
        }
    }
    const double scale = static_cast<double>(f.Scale());
    return area / (scale * scale);
}

/// A town whose plan is `cells` x `cells` cells of 0.5 m with a round building (radius `radius` metres) in both
/// moments.
struct RoundTown
{
    tn::TownData town;
    tn::MapMemory memory;
    RoundTown(int cells, float radius)
    {
        town.plan.cell = 0.5f;
        town.plan.x0 = 0.0f;
        town.plan.z0 = 0.0f;
        town.plan.nx = cells;
        town.plan.nz = cells;
        town.plan.sheet[0].assign(static_cast<std::size_t>(cells) * static_cast<std::size_t>(cells), 0);
        town.plan.sheet[1] = town.plan.sheet[0];
        const float mid = 0.25f * static_cast<float>(cells);
        for (int j = 0; j < cells; ++j)
        {
            for (int i = 0; i < cells; ++i)
            {
                const float dx = town.plan.CentreX(i) - mid;
                const float dz = town.plan.CentreZ(j) - mid;
                if (dx * dx + dz * dz <= radius * radius)
                {
                    town.plan.sheet[0][static_cast<std::size_t>(j * cells + i)] = tn::kPlanBuilding;
                    town.plan.sheet[1][static_cast<std::size_t>(j * cells + i)] = tn::kPlanBuilding;
                }
            }
        }
        memory.Init(town.plan, 15.0f);
        memory.Reveal(mid, mid, 1000.0f);
    }
};

void TestSmooth()
{
    static OverlayBatch batch;
    const dl::OverlayColor building = dl::map_detail::Neutral(tn::kPlanBuilding);
    // a round building has the area of its circle, and its edge is made of slopes, not of steps of cells; the same with
    // many cells in the window (the ink is then cut on a coarser grid)
    for (const auto& [cells, radius] : {std::pair<int, float>{120, 20.0f}, {600, 60.0f}})
    {
        RoundTown rt(cells, radius);
        MapScene s;
        s.town = &rt.town;
        s.memory = &rt.memory;
        s.player = {1.0f, 0.0f, 1.0f};
        const MapFrame f = dl::ComputeMapFrame(1600, 900, s);
        batch.Clear();
        dl::BuildMap(batch, 1600, 900, s, f);
        const double truth = 3.14159265358979 * static_cast<double>(radius) * static_cast<double>(radius);
        const double area = InkArea(batch, f, building);
        CHECK(std::fabs(area - truth) < 0.03 * truth);
        int slanted = 0;
        int triangles = 0;
        const dl::OverlayVertex* v = batch.Data();
        for (int i = 0; i + 2 < batch.Count(); i += 3)
        {
            if (!(Near(v[i].r, building.r, 1e-4f) && Near(v[i].a, building.a, 1e-4f)))
            {
                continue;
            }
            ++triangles;
            bool slope = false;
            for (int e = 0; e < 3; ++e)
            {
                const float dx = v[i + (e + 1) % 3].x - v[i + e].x;
                const float dy = v[i + (e + 1) % 3].y - v[i + e].y;
                slope = slope || (std::fabs(dx) > 0.3f && std::fabs(dy) > 0.3f);
            }
            slanted += slope ? 1 : 0;
        }
        CHECK(triangles > 40 && slanted >= 40);
        CHECK(!batch.Full());
    }
}

void TestFogStart()
{
    const tn::TownData& t = Town();
    static OverlayBatch batch;
    tn::MapMemory m;
    m.Init(t.plan, 15.0f);
    MapScene s;
    s.town = &t;
    s.memory = &m;
    s.player = t.spawn;
    // nothing seen yet: no ink at all, only fog (the neutral inks and the building of either moment are not in the
    // batch)
    const MapFrame f = dl::ComputeMapFrame(1600, 900, s);
    for (int top = 0; top < 2; ++top)
    {
        s.top = top;
        batch.Clear();
        dl::BuildMap(batch, 1600, 900, s, f);
        CHECK(CountColour(batch, 0.25f, 0.22f, 0.20f, 0.88f) == 0 &&
              CountColour(batch, 0.60f, 0.48f, 0.30f, 0.72f) == 0 &&
              CountColour(batch, 0.38f, 0.52f, 0.62f, 0.62f) == 0 &&
              CountColour(batch, 0.52f, 0.44f, 0.34f, 0.50f) == 0);
        // the fog is over the whole town, corner to corner: every point of the plan we try is fog (its hue), and the
        // paper has no hole
        for (const auto& [x, z] : {std::pair<float, float>{-80.0f, -80.0f},
                                   {80.0f, 80.0f},
                                   {-80.0f, 55.0f},
                                   {80.0f, -80.0f},
                                   {0.0f, 40.0f},
                                   {-60.0f, 0.0f},
                                   {60.0f, 10.0f}})
        {
            float sx = 0.0f, sy = 0.0f;
            f.ToScreen(x, z, sx, sy);
            dl::OverlayColor c;
            CHECK(ColourAt(batch, sx, sy, c));
            const bool fogHue = top == 0 ? (c.b > c.g && c.r < 0.95f) : (c.g > c.r && c.b > c.r);
            if (!fogHue)
            {
                std::printf("  not fog at (%.0f, %.0f) top %d: %.2f %.2f %.2f %.2f\n", static_cast<double>(x),
                            static_cast<double>(z), top, static_cast<double>(c.r), static_cast<double>(c.g),
                            static_cast<double>(c.b), static_cast<double>(c.a));
            }
            CHECK(fogHue);
        }
    }
    // a step on the ground clears a disc: near the feet the fog is thin or gone, far from them it stays
    m.Update(t, t.spawn);
    s.top = 0;
    batch.Clear();
    dl::BuildMap(batch, 1600, 900, s, f);
    float sx = 0.0f, sy = 0.0f;
    dl::OverlayColor c;
    f.ToScreen(t.spawn.x, t.spawn.z + 2.0f, sx, sy);
    CHECK(ColourAt(batch, sx, sy, c));
    CHECK(CountColour(batch, 0.60f, 0.48f, 0.30f, 0.72f) + CountColour(batch, 0.25f, 0.22f, 0.20f, 0.88f) > 30);
    // the whole town seen: no fog colour is left
    m.Reveal(0.0f, 0.0f, 400.0f);
    batch.Clear();
    dl::BuildMap(batch, 1600, 900, s, f);
    CHECK(CountColour(batch, 0.62f, 0.54f, 0.68f) == 0);
}

void TestHalo()
{
    const tn::TownData& t = Town();
    static OverlayBatch batch;
    tn::MapMemory m;
    m.Init(t.plan, 15.0f);
    MapScene s;
    s.town = &t;
    s.memory = &m;
    s.player = t.spawn;
    CHECK(t.steps.size() == 9 && t.steps.size() <= s.done.size());
    const MapFrame f = dl::ComputeMapFrame(1600, 900, s);
    for (int top = 0; top < 2; ++top)
    {
        s.top = top;
        const float r = top == 0 ? 0.98f : 0.42f;
        const float g = top == 0 ? 0.66f : 0.80f;
        const float b = top == 0 ? 0.16f : 1.0f;
        s.goal = 0;
        s.done.fill(false);
        batch.Clear();
        dl::BuildMap(batch, 1600, 900, s, f);
        const int all = CountColour(batch, r, g, b);
        CHECK(all > 200); // nine rings and nine question marks, the first the strongest
        s.done[0] = true;
        s.goal = 1;
        batch.Clear();
        dl::BuildMap(batch, 1600, 900, s, f);
        CHECK(CountColour(batch, r, g, b) < all && CountColour(batch, r, g, b) > 0);
        for (std::size_t k = 0; k < t.steps.size(); ++k)
        {
            s.done[k] = true;
        }
        s.goal = -1;
        batch.Clear();
        dl::BuildMap(batch, 1600, 900, s, f);
        CHECK(CountColour(batch, r, g, b) == 0); // every puzzle solved: no ring left
    }
    // the ring is a broad hint: its centre is not the place itself, but near it (within the jitter)
    s.top = 0;
    s.goal = 0;
    s.done.fill(false);
    batch.Clear();
    dl::BuildMap(batch, 1600, 900, s, f);
    const tn::Step& first = t.steps.front();
    float sx = 0.0f, sy = 0.0f;
    f.ToScreen(first.at.x, first.at.z, sx, sy);
    float far = 0.0f;
    float near = 1.0e9f;
    for (int i = 0; i < batch.Count(); ++i)
    {
        const dl::OverlayVertex& v = batch.Data()[i];
        if (Near(v.r, 0.98f, 0.002f) && Near(v.g, 0.66f, 0.002f) && Near(v.b, 0.16f, 0.002f) &&
            Near(v.a, 0.95f, 0.002f))
        {
            const float d = std::hypot(v.x - sx, v.y - sy);
            far = std::max(far, d);
            near = std::min(near, d);
        }
    }
    CHECK(far > 8.0f * f.Scale() &&
          far < 20.0f * f.Scale()); // a ring of about 12 m round a point within a few metres of the place
}

void TestThings()
{
    static Little little;
    static OverlayBatch batch;
    const auto box = [](int state, int worlds, float x, float z, float hx, float hz, float yaw)
    {
        tn::ThingBox b;
        b.state = static_cast<std::uint8_t>(state);
        b.worlds = static_cast<std::uint8_t>(worlds);
        b.centre = {x, 1.0f, z};
        b.half = {hx, 1.0f, hz};
        b.yaw = yaw;
        return b;
    };
    const auto thing = [](const char* name, tn::Verb verb)
    {
        tn::ThingDef t;
        t.name = name;
        t.verb = verb;
        return t;
    };
    tn::ThingDef gate = thing("gate", tn::Verb::Open); // in both moments: wide when shut, narrow when open
    gate.boxes = {box(0, 3, 15.0f, 45.0f, 4.0f, 1.0f, 0.0f), box(1, 3, 15.0f, 45.0f, 1.0f, 1.0f, 0.0f)};
    tn::ThingDef crate = thing("crate", tn::Verb::Push); // in A only: moves from x 30 to x 40
    crate.boxes = {box(0, 1, 30.0f, 45.0f, 1.5f, 1.5f, 0.6f), box(1, 1, 40.0f, 45.0f, 1.5f, 1.5f, 0.6f)};
    tn::ThingDef bell = thing("bell", tn::Verb::Ring); // not drawn: the lamps of the mast show it
    bell.boxes = {box(1, 2, 50.0f, 45.0f, 1.5f, 1.5f, 0.0f)};
    tn::ThingDef hidden = thing("hidden", tn::Verb::Open); // in the block that is not seen
    hidden.boxes = {box(0, 3, 65.0f, 65.0f, 2.0f, 2.0f, 0.0f)};
    little.town.things = {gate, crate, bell, hidden};
    MapScene s;
    s.town = &little.town;
    s.memory = &little.memory;
    s.player = {20.0f, 0.0f, 20.0f};
    const MapFrame f = dl::ComputeMapFrame(1600, 900, s);
    const auto warm = [&](float x, float z, float& alpha)
    {
        float sx = 0.0f, sy = 0.0f;
        f.ToScreen(x, z, sx, sy);
        dl::OverlayColor c;
        alpha = 0.0f;
        const bool found = ColourAt(batch, sx, sy, c) && Near(c.r, 0.55f, 0.002f) && Near(c.g, 0.22f, 0.002f);
        alpha = c.a;
        return found;
    };
    const auto cool = [&](float x, float z)
    {
        float sx = 0.0f, sy = 0.0f;
        f.ToScreen(x, z, sx, sy);
        dl::OverlayColor c;
        return ColourAt(batch, sx, sy, c) && Near(c.r, 0.04f, 0.002f) && Near(c.b, 0.50f, 0.002f);
    };
    const auto build = [&]()
    {
        batch.Clear();
        dl::BuildMap(batch, 1600, 900, s, f);
    };
    float alpha = 0.0f;
    // everything shut, A on top: the wide gate and the crate where it stands, in A's colour and strong
    s.top = 0;
    build();
    CHECK(warm(15.0f, 45.0f, alpha) && alpha > 0.9f && warm(18.0f, 45.0f, alpha)); // 3 m from the middle: the wide box
    CHECK(warm(30.0f, 45.0f, alpha) && alpha > 0.9f && !warm(40.0f, 45.0f, alpha));
    CHECK(CountColour(batch, 0.98f, 0.94f, 0.80f, 0.95f) == 0); // nothing is in its second state: no outline
    CHECK(!warm(50.0f, 45.0f, alpha) && !cool(50.0f, 45.0f));   // the bell is not drawn
    CHECK(!warm(65.0f, 65.0f, alpha));                          // what is not seen is not shown
    // B on top: the gate is B's too (strong, cool) and the crate, which is A's only, is faint
    s.top = 1;
    build();
    CHECK(cool(15.0f, 45.0f) && cool(18.0f, 45.0f));
    CHECK(warm(30.0f, 45.0f, alpha) && alpha < 0.4f);
    // the gate opened and the crate pushed: the gate is narrow, the crate is where it was pushed to, both have an
    // outline
    s.top = 0;
    s.thingOpen[0] = true;
    s.thingOpen[1] = true;
    build();
    CHECK(warm(15.0f, 45.0f, alpha) && !warm(18.0f, 45.0f, alpha));
    CHECK(!warm(30.0f, 45.0f, alpha) && warm(40.0f, 45.0f, alpha));
    CHECK(CountColour(batch, 0.98f, 0.94f, 0.80f, 0.95f) >=
          2 * 4 * 6); // two things, each seen in A: four edges of a quad
    // a place that is seen later shows its thing
    s.thingOpen.fill(false);
    little.memory.Reveal(65.0f, 65.0f, 6.0f);
    build();
    CHECK(warm(65.0f, 65.0f, alpha) && alpha > 0.9f);
    // a thing that is small is still a few pixels wide, and every corner stays on the paper
    little.town.things[0].boxes[0].half = {0.05f, 0.05f, 0.0f};
    build();
    CHECK(warm(15.0f, 45.0f, alpha));
    bool inside = true;
    for (int i = 0; i < batch.Count(); ++i)
    {
        inside = inside && batch.Data()[i].x >= -1.0f && batch.Data()[i].x <= 1601.0f && batch.Data()[i].y >= -1.0f &&
                 batch.Data()[i].y <= 901.0f;
    }
    CHECK(inside);
    little.town.things.clear();
}

void TestCache()
{
    const tn::TownData& t = Town();
    static OverlayBatch plain;
    static OverlayBatch cached;
    tn::MapMemory m;
    m.Init(t.plan, 15.0f);
    m.Update(t, t.spawn);
    MapScene s;
    s.town = &t;
    s.memory = &m;
    s.player = t.spawn;
    dl::MapCache cache;
    const MapFrame f = dl::ComputeMapFrame(1600, 900, s);
    plain.Clear();
    dl::BuildMap(plain, 1600, 900, s, f);
    cached.Clear();
    dl::BuildMap(cached, 1600, 900, s, f, &cache);
    CHECK(cache.rebuilds == 1 && plain.Count() == cached.Count());
    bool same = plain.Count() == cached.Count();
    for (int i = 0; same && i < plain.Count(); ++i)
    {
        same = plain.Data()[i].x == cached.Data()[i].x && plain.Data()[i].y == cached.Data()[i].y &&
               plain.Data()[i].r == cached.Data()[i].r && plain.Data()[i].a == cached.Data()[i].a;
    }
    CHECK(same);
    // the same again, a marker, the pointer, the lantern, the player moving a little: the layer is kept
    m.ToggleMarker({10.0f, 0.0f, 10.0f, 2, 1}, 3.0f);
    s.player.x += 0.3f;
    s.yaw = 1.0f;
    s.hasLantern = true;
    s.lantern = {5.0f, 0.0f, 5.0f};
    s.lit[0] = true;
    cached.Clear();
    dl::BuildMap(cached, 1600, 900, s, f, &cache);
    CHECK(cache.rebuilds == 1);
    // what it shows changes: a new cell seen, the other sheet, the other mode, the window moves, the screen changes
    m.Update(t, {t.spawn.x + 20.0f, t.spawn.y, t.spawn.z});
    cached.Clear();
    dl::BuildMap(cached, 1600, 900, s, f, &cache);
    CHECK(cache.rebuilds == 2);
    s.top = 1;
    cached.Clear();
    dl::BuildMap(cached, 1600, 900, s, f, &cache);
    CHECK(cache.rebuilds == 3);
    s.mode = 1;
    cached.Clear();
    dl::BuildMap(cached, 1600, 900, s, f, &cache);
    CHECK(cache.rebuilds == 4);
    s.zoom = 2.0f;
    const MapFrame zoomed = dl::ComputeMapFrame(1600, 900, s);
    cached.Clear();
    dl::BuildMap(cached, 1600, 900, s, zoomed, &cache);
    CHECK(cache.rebuilds == 5);
    const MapFrame small = dl::ComputeMapFrame(1024, 768, s);
    cached.Clear();
    dl::BuildMap(cached, 1024, 768, s, small, &cache);
    CHECK(cache.rebuilds == 6);
    cached.Clear();
    dl::BuildMap(cached, 1024, 768, s, small, &cache);
    CHECK(cache.rebuilds == 6);
    cache.Invalidate();
    cached.Clear();
    dl::BuildMap(cached, 1024, 768, s, small, &cache);
    CHECK(cache.rebuilds == 7);
}

void TestEveryView()
{
    // every zoom, every corner, small and large screens, both sheets, both modes: nothing leaves the screen, nothing
    // overflows
    const tn::TownData& t = Town();
    static OverlayBatch batch;
    tn::MapMemory m;
    m.Init(t.plan, 15.0f);
    for (const tn::Overlook& o : t.overlooks)
    {
        m.Reveal(o.at.x, o.at.z, o.radius);
    }
    m.Update(t, t.spawn);
    for (int k = 0; k < 10; ++k)
    {
        m.ToggleMarker(
            {-80.0f + 16.0f * static_cast<float>(k), 0.0f, -70.0f + 14.0f * static_cast<float>(k), k % 6, k % 2}, 2.0f);
    }
    MapScene s;
    s.town = &t;
    s.memory = &m;
    s.player = t.spawn;
    s.hasLantern = true;
    s.lantern = {t.spawn.x + 4.0f, 0.0f, t.spawn.z};
    s.goal = 0;
    int most = 0;
    for (const auto& [w, h] : {std::pair<int, int>{1600, 900}, {800, 800}, {2560, 1600}})
    {
        for (const float zoom : {1.0f, 2.0f, 4.0f})
        {
            for (const auto& [px, pz] : {std::pair<float, float>{0.0f, 0.0f}, {-500.0f, -500.0f}, {500.0f, 500.0f}})
            {
                for (int top = 0; top < 2; ++top)
                {
                    s.zoom = zoom;
                    s.panX = px;
                    s.panZ = pz;
                    s.top = top;
                    s.mode = top;
                    s.markerType = top * 3;
                    const MapFrame f = dl::ComputeMapFrame(w, h, s);
                    batch.Clear();
                    dl::BuildMap(batch, w, h, s, f);
                    CHECK(!batch.Full());
                    bool inside = true;
                    for (int i = 0; i < batch.Count(); ++i)
                    {
                        const dl::OverlayVertex& v = batch.Data()[i];
                        inside = inside && std::isfinite(v.x) && std::isfinite(v.y) && v.x >= -1.0f &&
                                 v.x <= static_cast<float>(w) + 1.0f && v.y >= -1.0f &&
                                 v.y <= static_cast<float>(h) + 1.0f;
                    }
                    CHECK(inside);
                    most = std::max(most, batch.Count());
                }
            }
        }
    }
    std::printf("  the largest map of all views: %d of %d vertices\n", most, OverlayBatch::kMaxVertices);
}

} // namespace

int main()
{
    TestPlan();
    TestMemory();
    TestMarkers();
    TestFrame();
    TestInk();
    TestRealMap();
    TestSmooth();
    TestFogStart();
    TestHalo();
    TestThings();
    TestCache();
    TestEveryView();
    std::printf("%d checks passed, %d failed\n", g_passed, g_failed);
    return g_failed == 0 ? 0 : 1;
}
