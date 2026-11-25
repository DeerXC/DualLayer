// test_map.cpp - tests of the map of two sheets of tracing paper (no window, no GPU): src/scene/MapMemory.h,
// src/render/MapView.h and the plan of the town file (src/scene/Town.h Plan). Reads data/town.dlt (python
// tools/layout/city_model.py export) for the plan, and a small plan made here for the colours.
//   g++ -std=c++20 -O2 -Wall -Wextra -I src tests/test_map.cpp -o test_map
//   MSVC (Developer PowerShell):  .\tests\run_tests.ps1 -Only map
// What is checked:
//   1. the plan of the model: the size, the classes, the sea in both moments, the moat of A that is not in B, the
//   streets of
//      the model under their middle lines, the two moments differing in a few hundred cells and not in most of them
//   2. what is seen: the body sees the cells within the radius and no others; the overlooks reveal far, but only to a
//   body that
//      stands at them; nothing outside the plan is marked; the box of what is seen
//   3. the markers: placed, taken away where they stand, never more than five (the sixth takes the oldest place), no
//   bad numbers
//   4. the paper: it is inside every screen, the window of the plan holds the player and the tower and is at least 90 m
//   across,
//      a point of the screen and a place of the plan are turned into each other both ways
//   5. the ink: in both moments the same is neutral, in one moment only it is warm (A) or cool (B), where both have
//   something
//      the sheet on top shows; what is not seen is blank paper; the lamps of the tower are lit or dark; the arrow
//      points where the camera looks; the marker flags and the lantern are drawn; nothing leaves the screen and the
//      batch is never full
// Returns 1 when a check fails.

#include "render/MapView.h"
#include "scene/MapMemory.h"
#include "scene/Town.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

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
    CHECK(plan.Valid() && plan.nx == 200 && plan.nz == 202 && Near(plan.cell, 1.0f, 1e-6f));
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
    CHECK(differ > 600 && differ < 6000); // the moments differ in a few thousand cells of 1 m and agree in most
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
    CHECK(m.SeenCount() > 600 && m.SeenCount() < 800);
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
    CHECK(low.SeenCount() < 1000);
    // nothing outside the plan is marked, a bad number is ignored
    tn::MapMemory edge;
    edge.Init(t.plan, 15.0f);
    edge.Reveal(-5000.0f, -5000.0f, 30.0f);
    edge.Reveal(std::nanf(""), 0.0f, 30.0f);
    edge.Reveal(0.0f, 0.0f, -4.0f);
    CHECK(edge.SeenCount() == 0 && !edge.Seen(-1, 0) && !edge.Seen(0, 100000));
    edge.Reveal(t.plan.x0, t.plan.z0, 10.0f); // the corner: only the cells inside the plan
    CHECK(edge.SeenCount() > 20 && edge.SeenCount() < 160);
}

void TestMarkers()
{
    const tn::TownData& t = Town();
    tn::MapMemory m;
    m.Init(t.plan, 15.0f);
    CHECK(m.ToggleMarker({10.0f, 0.0f, 10.0f}, 3.0f) && m.MarkerCount() == 1);
    CHECK(!m.ToggleMarker({11.0f, 0.0f, 10.5f}, 3.0f) && m.MarkerCount() == 0); // where one stands: taken away
    for (int k = 0; k < 5; ++k)
    {
        CHECK(m.ToggleMarker({20.0f * static_cast<float>(k), 0.0f, 0.0f}, 3.0f));
    }
    CHECK(m.MarkerCount() == tn::MapMemory::kMaxMarkers);
    CHECK(m.ToggleMarker({-50.0f, 1.0f, 3.0f}, 3.0f) && m.MarkerCount() == 5); // the sixth takes the oldest place
    CHECK(Near(m.Marker(0).x, 20.0f, 1e-6f) && Near(m.Marker(4).x, -50.0f, 1e-6f) && Near(m.Marker(4).y, 1.0f, 1e-6f));
    CHECK(!m.ToggleMarker({std::nanf(""), 0.0f, 0.0f}, 3.0f) && m.MarkerCount() == 5);
    CHECK(!m.ToggleMarker({-50.0f, 0.0f, 3.5f}, 3.0f) && m.MarkerCount() == 4); // the nearest is the one taken away
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
    // after a walk the window grows with what is seen
    tn::MapMemory far;
    far.Init(t.plan, 15.0f);
    far.Update(t, t.spawn);
    far.Update(t, {-30.0f, 0.0f, 0.0f});
    s.memory = &far;
    s.player = {-30.0f, 0.0f, 0.0f};
    const MapFrame wide = dl::ComputeMapFrame(1600, 900, s);
    s.memory = &m;
    s.player = t.spawn;
    CHECK(wide.extent > dl::ComputeMapFrame(1600, 900, s).extent - 1.0f);
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

/// A town with a plan of 8 x 8 cells of 10 m, and a mast with three emblems, for the colours.
struct Little
{
    tn::TownData town;
    tn::MapMemory memory;
    Little()
    {
        town.plan.cell = 10.0f;
        town.plan.x0 = 0.0f;
        town.plan.z0 = 0.0f;
        town.plan.nx = 8;
        town.plan.nz = 8;
        town.plan.sheet[0].assign(64, 0);
        town.plan.sheet[1].assign(64, 0);
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
        for (int j = 0; j < 8; ++j)
        {
            for (int i = 0; i < 8; ++i)
            {
                if (!(i == 6 && j == 6))
                {
                    memory.Reveal(town.plan.CentreX(i), town.plan.CentreZ(j), 0.5f);
                }
            }
        }
    }
    void Set(int i, int j, int a, int b)
    {
        town.plan.sheet[0][static_cast<std::size_t>(j * 8 + i)] = static_cast<std::uint8_t>(a);
        town.plan.sheet[1][static_cast<std::size_t>(j * 8 + i)] = static_cast<std::uint8_t>(b);
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
    { f.ToScreen(little.town.plan.CentreX(i), little.town.plan.CentreZ(j), x, y); };
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
        cellAt(f, 6, 6, x, y); // not seen: blank paper
        CHECK(ColourAt(batch, x, y, c) && Near(c.r, 0.94f, 0.01f) && Near(c.g, 0.90f, 0.01f) &&
              Near(c.b, 0.80f, 0.01f));
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
    little.memory.ToggleMarker({30.0f, 0.0f, 30.0f}, 3.0f);
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

} // namespace

int main()
{
    TestPlan();
    TestMemory();
    TestMarkers();
    TestFrame();
    TestInk();
    TestRealMap();
    std::printf("%d checks passed, %d failed\n", g_passed, g_failed);
    return g_failed == 0 ? 0 : 1;
}
