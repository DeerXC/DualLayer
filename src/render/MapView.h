// MapView.h - the map: two sheets of tracing paper (docs/DESIGN.md 10.4). Drawn into an OverlayBatch, so it needs no
//            new pass.
// Sheets:    the plan of the past (A) and of the present (B) are laid one over the other. What stands in both is
//            drawn once, in a neutral ink; what stands in one moment only is warm (A) or cool (B), the colours of the
//            two moments; where both moments have something and it differs, both are drawn and the sheet on top shows
//            over the other. The player turns the other sheet to the top with Tab.
// Shapes:    the plan is a grid of 0.5 m cells, but the map never shows the cells: every ink is drawn as a smooth
//            shape (the grid is averaged over 2 x 2 cells and cut along the half level, so a staircase of cells
//            becomes a straight edge and a corner is cut), and the fog is a smooth cloud with soft layers.
// Fog:       the whole town is under fog from the first frame, and the paper shows all of it. Only the cells the player
//            has seen are inked (scene/MapMemory.h); a solved puzzle lights its district. The tower, with its three
//            emblems, is always drawn: it is the goal and it was in sight from the first frame. The puzzles still to be
//            solved show through the fog as rings, broad ones, not exact places.
// Holds:     the player (an arrow that points the way the camera looks), the lantern, the tower with the lit and dark
//            lamps of its mast, the markers the player left (six shapes, in the colour of the moment they were set in).
//            Never: a corner minimap, a list, or what to do. No words but the keys and the buttons.
// View:      north is up. Zoom 1 shows the whole plan; the player zooms in (up to 4 x) and moves the window.
// Layers:    the ink and the fog (the part that costs) are kept in a MapCache and drawn again only when what they show
//            changes; the marks over them are made every frame.
// Plain data and plain maths, no Windows, no Direct3D: tested without a window (tests/test_map.cpp).
// UNITS: screen pixels for the frame, metres for the world. Plan: x east, z north.
#pragma once

#include "render/OverlayBatch.h"
#include "scene/Camera.h"
#include "scene/MapMemory.h"
#include "scene/Town.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <memory>
#include <vector>

namespace dl
{

/// What the map shows now.
struct MapScene
{
    const town::TownData* town = nullptr;
    const town::MapMemory* memory = nullptr;
    int top = 0;        ///< the sheet on top: 0 = A, 1 = B
    int mode = 0;       ///< 0 normal, 1 differences only
    float zoom = 1.0f;  ///< 1 = the whole plan; up to 4 (MapFrame clamps it)
    float panX = 0.0f;  ///< UNITS: metres: the window's centre east of the plan's centre
    float panZ = 0.0f;  ///< UNITS: metres: the window's centre north of the plan's centre
    int markerType = 0; ///< the shape the next marker will have (shown on its button)
    Vec3 player{};
    float yaw = 0.0f;        ///< where the camera looks (0 = north, + toward east)
    bool hasLantern = false; ///< the lantern is set down
    Vec3 lantern{};
    std::array<bool, 16> lit{};       ///< which emblems are lit (the order of town.emblems)
    int goal = -1;                    ///< the index in town.steps of the next puzzle, or -1
    std::array<bool, 32> done{};      ///< which steps of town.steps are solved
    std::array<bool, 32> thingOpen{}; ///< which things of town.things are in their second state (opened, moved, laid)
};

/// Where the paper is on the screen, and which part of the plan it shows.
struct MapFrame
{
    bool valid = false;
    int screenW = 0;
    int screenH = 0;
    float px = 0.0f; ///< the paper, in pixels
    float py = 0.0f;
    float size = 0.0f;
    float wx0 = 0.0f;    ///< the west edge of the window on the plan, metres
    float wz1 = 0.0f;    ///< the north edge
    float extent = 1.0f; ///< metres across (the window is square)

    float Scale() const
    {
        return size / extent;
    }
    void ToScreen(float x, float z, float& sx, float& sy) const
    {
        sx = px + (x - wx0) * Scale();
        sy = py + (wz1 - z) * Scale();
    }
    bool Inside(float sx, float sy) const
    {
        return valid && sx >= px && sx <= px + size && sy >= py && sy <= py + size;
    }
    /// The place of the plan under a point of the screen; false when the point is not on the paper.
    bool ToWorld(float sx, float sy, float& x, float& z) const
    {
        if (!Inside(sx, sy))
        {
            return false;
        }
        x = wx0 + (sx - px) / Scale();
        z = wz1 - (sy - py) / Scale();
        return true;
    }
};

/// The buttons beside the paper, top to bottom.
enum MapOption : int
{
    kOptMap = 0,
    kOptDifference = 1,
    kOptZoomIn = 2,
    kOptZoomOut = 3,
    kOptCentre = 4,
    kOptMarker = 5,
    kOptCount = 6,
};

namespace map_detail
{

inline constexpr float kMinExtent = 90.0f;     ///< UNITS: metres: an unzoomed window is never smaller than this
inline constexpr float kMinZoomExtent = 24.0f; ///< UNITS: metres: the closest zoom
inline constexpr float kPlanMargin = 2.0f;     ///< UNITS: metres of paper round the plan
inline constexpr float kMargin = 14.0f;        ///< UNITS: metres round what is seen (a town without a plan)
inline constexpr float kMaxZoom = 4.0f;
inline constexpr float kFeatherMetres = 2.5f; ///< UNITS: metres: the soft edge of the fog
inline constexpr float kVisibleBelow = 0.55f; ///< ink is drawn where the fog field is below this
inline constexpr float kInkLevel = 0.46f;     ///< the ink is cut where the averaged coverage is at least this
inline constexpr float kCliffLevel =
    0.64f; ///< the cliff is cut higher: the thin streaks of steep ground the plan holds vanish
inline constexpr float kHaloMetres = 12.0f; ///< UNITS: metres: the ring of the next puzzle
inline constexpr float kHaloJitter = 6.0f;  ///< UNITS: metres: how far the ring's centre is from the true place

inline OverlayColor Warm(float a)
{
    return {0.86f, 0.38f, 0.10f, a};
}
inline OverlayColor Cool(float a)
{
    return {0.10f, 0.40f, 0.74f, a};
}

/// The ink of a class of the plan, in the neutral colour (the same in both moments).
inline OverlayColor Neutral(int cls)
{
    switch (cls)
    {
    case town::kPlanWater:
        return {0.38f, 0.52f, 0.62f, 0.62f};
    case town::kPlanCliff:
        return {0.52f, 0.44f, 0.34f, 0.50f};
    case town::kPlanBuilding:
        return {0.25f, 0.22f, 0.20f, 0.88f};
    case town::kPlanStreet:
        return {0.60f, 0.48f, 0.30f, 0.72f};
    default:
        return {0.0f, 0.0f, 0.0f, 0.0f};
    }
}

/// The ink of a class that stands in one moment only: its weight in the neutral ink, in the colour of its moment.
inline OverlayColor OfMoment(int cls, int world, float alpha)
{
    const OverlayColor n = Neutral(cls);
    OverlayColor c = world == 0 ? Warm(1.0f) : Cool(1.0f);
    // a building is the darkest, a street the lightest: the same weights as the neutral ink, in the colour of the
    // moment
    const float dark = cls == town::kPlanBuilding ? 0.62f : (cls == town::kPlanStreet ? 1.0f : 0.85f);
    c.r *= dark;
    c.g *= dark;
    c.b *= dark;
    c.a = n.a * alpha;
    return c;
}

/// The colour of the shapes of a moment's markers: A warm, B cool.
inline OverlayColor MarkerInk(int layer, float alpha)
{
    return layer == 1 ? OverlayColor{0.08f, 0.56f, 0.88f, alpha} : OverlayColor{0.92f, 0.46f, 0.08f, alpha};
}

inline float Hash01(int x, int y, unsigned seed)
{
    unsigned h = static_cast<unsigned>(x) * 73856093u ^ static_cast<unsigned>(y) * 19349663u ^ seed * 83492791u;
    h ^= h >> 13;
    h *= 0x5bd1e995u;
    h ^= h >> 15;
    return static_cast<float>(h & 0xFFFFu) / 65535.0f;
}

/// Smooth noise 0 .. 1 (bilinear between hashed lattice points, eased).
inline float ValueNoise(float x, float y, unsigned seed)
{
    const float fx = std::floor(x);
    const float fy = std::floor(y);
    const int ix = static_cast<int>(fx);
    const int iy = static_cast<int>(fy);
    float tx = x - fx;
    float ty = y - fy;
    tx = tx * tx * (3.0f - 2.0f * tx);
    ty = ty * ty * (3.0f - 2.0f * ty);
    const float a = Hash01(ix, iy, seed);
    const float b = Hash01(ix + 1, iy, seed);
    const float c = Hash01(ix, iy + 1, seed);
    const float d = Hash01(ix + 1, iy + 1, seed);
    return (a + (b - a) * tx) + ((c + (d - c) * tx) - (a + (b - a) * tx)) * ty;
}

/// Cell coordinates to screen pixels, kept on the paper.
struct Painter
{
    OverlayBatch* b = nullptr;
    const MapFrame* f = nullptr;
    const town::Plan* plan = nullptr;
    float X(float ci) const
    {
        const float x = f->px + (plan->x0 + ci * plan->cell - f->wx0) * f->Scale();
        return std::clamp(x, f->px, f->px + f->size);
    }
    float Y(float cj) const
    {
        const float y = f->py + (f->wz1 - (plan->z0 + cj * plan->cell)) * f->Scale();
        return std::clamp(y, f->py, f->py + f->size);
    }
};

/// Fills where a field of cells is at least `thr`. `cells` is W x H values (row by row from the south); cell (x, y)
/// covers the plan cells from (ci0 + x * step, cj0 + y * step) on, `step` of them on a side; `outside` is the value
/// beyond the field. The value at a corner is the mean of the 4 cells that meet there, and the edge is cut along `thr`
/// between corners: a straight edge of cells stays where it is, a staircase becomes a slope and a corner is cut. Whole
/// cells are joined into rectangles (along a row, and then up the rows that have the same run), the cut ones are small
/// polygons. Every shared edge is computed from the same numbers, so there are no cracks.
inline void FillField(const Painter& p, const float* cells, int W, int H, int ci0, int cj0, int step, float outside,
                      float thr, const OverlayColor& colour, std::vector<float>& corner)
{
    if (W <= 0 || H <= 0 || step <= 0)
    {
        return;
    }
    const int VW = W + 1;
    const float fs = static_cast<float>(step);
    const auto cell = [&](int x, int y) -> float
    {
        return (x < 0 || y < 0 || x >= W || y >= H)
                   ? outside
                   : cells[static_cast<std::size_t>(y) * static_cast<std::size_t>(W) + static_cast<std::size_t>(x)];
    };
    corner.resize(static_cast<std::size_t>(VW) * static_cast<std::size_t>(H + 1));
    for (int b = 0; b <= H; ++b)
    {
        for (int a = 0; a <= W; ++a)
        {
            corner[static_cast<std::size_t>(b) * static_cast<std::size_t>(VW) + static_cast<std::size_t>(a)] =
                0.25f * (cell(a - 1, b - 1) + cell(a, b - 1) + cell(a - 1, b) + cell(a, b));
        }
    }
    const auto vert = [&](int a, int b)
    { return corner[static_cast<std::size_t>(b) * static_cast<std::size_t>(VW) + static_cast<std::size_t>(a)]; };
    struct Run
    {
        int a0;
        int a1;
        int b0;
    };
    std::vector<Run> open;
    std::vector<Run> runs;
    std::vector<Run> next;
    const auto emitRect = [&](const Run& r, int b1)
    {
        const float xa = p.X(static_cast<float>(ci0) + static_cast<float>(r.a0) * fs);
        const float xb = p.X(static_cast<float>(ci0) + static_cast<float>(r.a1) * fs);
        const float yt = p.Y(static_cast<float>(cj0) + static_cast<float>(b1) * fs);
        const float yb = p.Y(static_cast<float>(cj0) + static_cast<float>(r.b0) * fs);
        p.b->AddRect(xa, yt, xb - xa, yb - yt, colour);
    };
    for (int b = 0; b < H; ++b)
    {
        runs.clear();
        int runStart = -1;
        for (int a = 0; a < W; ++a)
        {
            const float v[4] = {vert(a, b), vert(a + 1, b), vert(a + 1, b + 1), vert(a, b + 1)}; // sw se ne nw
            const bool in[4] = {v[0] >= thr, v[1] >= thr, v[2] >= thr, v[3] >= thr};
            const int count = (in[0] ? 1 : 0) + (in[1] ? 1 : 0) + (in[2] ? 1 : 0) + (in[3] ? 1 : 0);
            if (count == 4)
            {
                if (runStart < 0)
                {
                    runStart = a;
                }
                continue;
            }
            if (runStart >= 0)
            {
                runs.push_back({runStart, a, b});
                runStart = -1;
            }
            if (count == 0)
            {
                continue;
            }
            // the polygon of the corners at or above thr, and the places where its edge crosses the cell's edges
            static constexpr float cx[4] = {0.0f, 1.0f, 1.0f, 0.0f};
            static constexpr float cy[4] = {0.0f, 0.0f, 1.0f, 1.0f};
            float qx[8];
            float qy[8];
            int n = 0;
            for (int k = 0; k < 4; ++k)
            {
                const int k2 = (k + 1) % 4;
                if (in[k])
                {
                    qx[n] = cx[k];
                    qy[n] = cy[k];
                    ++n;
                }
                if (in[k] != in[k2])
                {
                    // always from the lower corner of the edge to the higher, so the cell next door finds the same
                    // place
                    const bool forward = (k == 0 || k == 1);
                    const int lo = forward ? k : k2;
                    const int hi = forward ? k2 : k;
                    const float t = (thr - v[lo]) / (v[hi] - v[lo]);
                    qx[n] = cx[lo] + (cx[hi] - cx[lo]) * t;
                    qy[n] = cy[lo] + (cy[hi] - cy[lo]) * t;
                    ++n;
                }
            }
            for (int k = 0; k < n; ++k)
            {
                qx[k] = p.X(static_cast<float>(ci0) + (static_cast<float>(a) + qx[k]) * fs);
                qy[k] = p.Y(static_cast<float>(cj0) + (static_cast<float>(b) + qy[k]) * fs);
            }
            for (int k = 1; k + 1 < n; ++k)
            {
                p.b->AddTriangle(qx[0], qy[0], qx[k], qy[k], qx[k + 1], qy[k + 1], colour);
            }
        }
        if (runStart >= 0)
        {
            runs.push_back({runStart, W, b});
        }
        // a run that is the same as one of the row below goes on up; the others are drawn now
        next.clear();
        std::size_t oi = 0;
        for (Run& r : runs)
        {
            while (oi < open.size() && (open[oi].a0 < r.a0 || (open[oi].a0 == r.a0 && open[oi].a1 < r.a1)))
            {
                emitRect(open[oi], b);
                ++oi;
            }
            if (oi < open.size() && open[oi].a0 == r.a0 && open[oi].a1 == r.a1)
            {
                r.b0 = open[oi].b0;
                ++oi;
            }
            next.push_back(r);
        }
        for (; oi < open.size(); ++oi)
        {
            emitRect(open[oi], b);
        }
        open.swap(next);
    }
    for (const Run& r : open)
    {
        emitRect(r, H);
    }
}

} // namespace map_detail

/// Where the paper goes and what part of the plan it shows. `valid` is false without a town or a screen. With a plan
/// the window shows the whole plan (zoom 1), or a part of it (zoom up to 4), never beyond the plan's edge and its
/// margin.
inline MapFrame ComputeMapFrame(int width, int height, const MapScene& s)
{
    namespace md = map_detail;
    MapFrame f;
    if (s.town == nullptr || width <= 0 || height <= 0)
    {
        return f;
    }
    f.screenW = width;
    f.screenH = height;
    const town::Plan& plan = s.town->plan;
    const float zoom = std::isfinite(s.zoom) ? std::clamp(s.zoom, 1.0f, md::kMaxZoom) : 1.0f;
    if (plan.Valid())
    {
        const float west = plan.x0 - md::kPlanMargin;
        const float east = plan.x0 + static_cast<float>(plan.nx) * plan.cell + md::kPlanMargin;
        const float south = plan.z0 - md::kPlanMargin;
        const float north = plan.z0 + static_cast<float>(plan.nz) * plan.cell + md::kPlanMargin;
        const float whole = std::max(east - west, north - south);
        f.extent = zoom <= 1.0f ? std::max(whole, md::kMinExtent) : std::max(whole / zoom, md::kMinZoomExtent);
        f.extent = std::min(f.extent, std::max(whole, md::kMinExtent));
        const float panX = std::isfinite(s.panX) ? s.panX : 0.0f;
        const float panZ = std::isfinite(s.panZ) ? s.panZ : 0.0f;
        float cx = 0.5f * (west + east) + panX;
        float cz = 0.5f * (south + north) + panZ;
        if (f.extent <= east - west)
        {
            cx = std::clamp(cx, west + 0.5f * f.extent, east - 0.5f * f.extent);
        }
        else
        {
            cx = 0.5f * (west + east);
        }
        if (f.extent <= north - south)
        {
            cz = std::clamp(cz, south + 0.5f * f.extent, north - 0.5f * f.extent);
        }
        else
        {
            cz = 0.5f * (south + north);
        }
        f.wx0 = cx - 0.5f * f.extent;
        f.wz1 = cz + 0.5f * f.extent;
    }
    else
    {
        // a town without a plan: the window holds the player, the tower, the lantern and the markers
        float x0 = 1.0e9f;
        float x1 = -1.0e9f;
        float z0 = 1.0e9f;
        float z1 = -1.0e9f;
        const auto add = [&](float x, float z)
        {
            x0 = std::min(x0, x);
            x1 = std::max(x1, x);
            z0 = std::min(z0, z);
            z1 = std::max(z1, z);
        };
        add(s.player.x, s.player.z);
        Vec3 mast;
        if (s.town->MastAt(mast))
        {
            add(mast.x, mast.z);
        }
        if (s.hasLantern)
        {
            add(s.lantern.x, s.lantern.z);
        }
        if (s.memory != nullptr)
        {
            for (int k = 0; k < s.memory->MarkerCount(); ++k)
            {
                add(s.memory->Marker(k).x, s.memory->Marker(k).z);
            }
        }
        x0 -= md::kMargin;
        x1 += md::kMargin;
        z0 -= md::kMargin;
        z1 += md::kMargin;
        f.extent = std::max({x1 - x0, z1 - z0, md::kMinExtent});
        f.wx0 = 0.5f * (x0 + x1) - 0.5f * f.extent;
        f.wz1 = 0.5f * (z0 + z1) + 0.5f * f.extent;
    }
    f.size = std::floor(0.80f * static_cast<float>(std::min(width, height)));
    f.px = std::floor(0.5f * (static_cast<float>(width) - f.size));
    f.py = std::floor(0.5f * (static_cast<float>(height) - f.size) - 0.03f * static_cast<float>(height));
    f.valid = f.size > 8.0f;
    return f;
}

/// Where button `k` is: x, y, width, height in pixels.
inline void MapOptionRect(const MapFrame& f, int k, float& x, float& y, float& w, float& h)
{
    w = std::max(120.0f, f.size * 0.27f);
    h = std::max(26.0f, f.size * 0.052f);
    x = f.px + f.size + 14.0f;
    if (f.screenW > 0 && x + w > static_cast<float>(f.screenW) - 6.0f)
    {
        x = std::max(0.0f, static_cast<float>(f.screenW) - 6.0f - w);
    }
    // the two views, then a gap, then the controls of the window
    y = f.py + 22.0f + static_cast<float>(k) * (h + 8.0f) + (k >= kOptZoomIn ? 10.0f : 0.0f);
}

/// The button under a point of the screen (a MapOption), or -1.
inline int MapOptionAt(const MapFrame& f, float sx, float sy)
{
    if (!f.valid)
    {
        return -1;
    }
    for (int k = 0; k < kOptCount; ++k)
    {
        float x = 0.0f, y = 0.0f, w = 0.0f, h = 0.0f;
        MapOptionRect(f, k, x, y, w, h);
        if (sx >= x && sx <= x + w && sy >= y && sy <= y + h)
        {
            return k;
        }
    }
    return -1;
}

/// What the cached layer was made from: when any of it changes the layer is made again.
struct MapCacheKey
{
    const void* plan = nullptr;
    const void* memory = nullptr;
    int seen = -1;
    int top = -1;
    int mode = -1;
    float px = 0.0f, py = 0.0f, size = 0.0f, wx0 = 0.0f, wz1 = 0.0f, extent = 0.0f;
    bool operator==(const MapCacheKey&) const = default;
};

/// The ink and the fog, kept between frames. Hold one for the map; pass it to BuildMap.
struct MapCache
{
    std::unique_ptr<OverlayBatch> layer;
    MapCacheKey key;
    bool valid = false;
    int rebuilds = 0; ///< how many times the layer was made (for the tests)
    void Invalidate()
    {
        valid = false;
    }
};

namespace map_detail
{

/// The ink and the fog, in order: the sheets of the plan where the cells are seen, then the clouds over the rest.
inline void BuildTerrain(OverlayBatch& out, const MapScene& s, const MapFrame& f)
{
    const town::Plan& plan = s.town->plan;
    if (!plan.Valid())
    {
        return;
    }
    const Painter p{&out, &f, &plan};
    const int top = s.top == 1 ? 1 : 0;
    const int bottom = 1 - top;
    // the ink is cut on a grid of `lod` plan cells a side when the window holds many cells (the whole town: 2 x 2): the
    // shapes are smooth either way, and the polygons are fewer
    const int lod = std::clamp(static_cast<int>(std::ceil(f.extent / plan.cell / 256.0f)), 1, 4);
    const int margin = 2 * lod;
    const int wi0 = plan.CellX(f.wx0) - margin;
    const int wi1 = plan.CellX(f.wx0 + f.extent) + margin;
    const int wj0 = plan.CellZ(f.wz1 - f.extent) - margin;
    const int wj1 = plan.CellZ(f.wz1) + margin;
    const int W = wi1 - wi0 + 1;
    const int H = wj1 - wj0 + 1;
    if (W <= 0 || H <= 0 || W > 4096 || H > 4096)
    {
        return;
    }
    const std::size_t cells = static_cast<std::size_t>(W) * static_cast<std::size_t>(H);

    // ---- the fog field: 1 where nothing is seen, 0 where it is, a soft edge between; the edge is made uneven by
    // noise, and only the edge (so that the middle of what is seen stays clear)
    const int R = std::clamp(static_cast<int>(std::lround(kFeatherMetres / plan.cell)), 0, 8);
    const int PW = W + 2 * R;
    const int PH = H + 2 * R;
    const auto unseen = [&](int i, int j) -> double
    {
        // beyond the plan the edge cell counts: the fog (or the clear) goes on to the edge of the paper
        i = std::clamp(i, 0, plan.nx - 1);
        j = std::clamp(j, 0, plan.nz - 1);
        return (s.memory != nullptr && s.memory->Seen(i, j)) ? 0.0 : 1.0;
    };
    std::vector<float> fog(cells);
    if (R == 0)
    {
        for (int y = 0; y < H; ++y)
        {
            for (int x = 0; x < W; ++x)
            {
                fog[static_cast<std::size_t>(y) * W + x] = static_cast<float>(unseen(wi0 + x, wj0 + y));
            }
        }
    }
    else
    {
        std::vector<double> sat(static_cast<std::size_t>(PW + 1) * static_cast<std::size_t>(PH + 1), 0.0);
        for (int b = 0; b < PH; ++b)
        {
            double row = 0.0;
            for (int a = 0; a < PW; ++a)
            {
                row += unseen(wi0 - R + a, wj0 - R + b);
                sat[static_cast<std::size_t>(b + 1) * (PW + 1) + (a + 1)] =
                    sat[static_cast<std::size_t>(b) * (PW + 1) + (a + 1)] + row;
            }
        }
        const double norm = 1.0 / static_cast<double>((2 * R + 1) * (2 * R + 1));
        const auto S = [&](int a, int b) { return sat[static_cast<std::size_t>(b) * (PW + 1) + a]; };
        for (int y = 0; y < H; ++y)
        {
            for (int x = 0; x < W; ++x)
            {
                const double sum =
                    S(x + 2 * R + 1, y + 2 * R + 1) - S(x, y + 2 * R + 1) - S(x + 2 * R + 1, y) + S(x, y);
                fog[static_cast<std::size_t>(y) * W + x] = static_cast<float>(sum * norm);
            }
        }
    }
    std::vector<float> wisp(cells, 0.0f);
    for (int y = 0; y < H; ++y)
    {
        for (int x = 0; x < W; ++x)
        {
            const int i = wi0 + x;
            const int j = wj0 + y;
            const float c = static_cast<float>(plan.cell);
            const float wx =
                static_cast<float>(i) * c; // noise in metres from the plan's corner: the same at every zoom
            const float wz = static_cast<float>(j) * c;
            const float n = 0.6f * ValueNoise(wx / 5.0f, wz / 5.0f, 1u) + 0.4f * ValueNoise(wx / 2.2f, wz / 2.2f, 2u);
            float& v = fog[static_cast<std::size_t>(y) * W + x];
            v += 0.25f * n * std::min(1.0f, v * 6.0f);
            wisp[static_cast<std::size_t>(y) * W + x] =
                v >= 0.92f
                    ? (0.7f * ValueNoise(wx / 30.0f, wz / 30.0f, 5u) + 0.3f * ValueNoise(wx / 12.0f, wz / 12.0f, 6u))
                    : 0.0f;
        }
    }

    // ---- the ink of the two sheets, only where the cells are seen
    const auto floorDiv = [](int a, int b) { return a >= 0 ? a / b : -((-a + b - 1) / b); };
    const int ci0 =
        floorDiv(wi0, lod) * lod; // the coarse grid starts at a multiple of lod, so it does not move with the window
    const int cj0 = floorDiv(wj0, lod) * lod;
    const int CW = (wi1 - ci0) / lod + 1;
    const int CH = (wj1 - cj0) / lod + 1;
    const float share = 1.0f / static_cast<float>(lod * lod);
    std::vector<float> corner;
    std::vector<std::int16_t> codes(cells);
    std::vector<float> grid(static_cast<std::size_t>(CW) * static_cast<std::size_t>(CH));
    for (int pass = 0; pass < 2; ++pass)
    {
        int present[2][5] = {};
        for (int y = 0; y < H; ++y)
        {
            for (int x = 0; x < W; ++x)
            {
                const int i = wi0 + x;
                const int j = wj0 + y;
                int code = 0;
                if (plan.In(i, j) && fog[static_cast<std::size_t>(y) * W + x] < kVisibleBelow)
                {
                    const int a = plan.At(0, i, j);
                    const int c = plan.At(1, i, j);
                    if (s.mode == 1 && a == c)
                    {
                        code = 0;
                    }
                    else if (a == c)
                    {
                        code = pass == 0 ? a : 0; // in both moments, the same: one neutral ink
                    }
                    else
                    {
                        const int world = pass == 0 ? bottom : top;
                        const int cls = world == 0 ? a : c;
                        code = cls != 0 ? (0x100 | (world << 4) | cls) : 0;
                    }
                }
                codes[static_cast<std::size_t>(y) * W + x] = static_cast<std::int16_t>(code);
                if (code > 0 && code < 0x100)
                {
                    present[0][code & 7] = 1;
                }
                else if (code >= 0x100)
                {
                    present[1][code & 7] = 1;
                }
            }
        }
        for (int kind = 0; kind < 2; ++kind)
        {
            for (int cls = 1; cls < town::kPlanClassCount; ++cls)
            {
                if (present[kind][cls] == 0)
                {
                    continue;
                }
                const int world = pass == 0 ? bottom : top;
                const int want = kind == 0 ? cls : (0x100 | (world << 4) | cls);
                std::fill(grid.begin(), grid.end(), 0.0f);
                for (int y = 0; y < H; ++y)
                {
                    const std::size_t rowBase =
                        static_cast<std::size_t>(((wj0 + y) - cj0) / lod) * static_cast<std::size_t>(CW);
                    for (int x = 0; x < W; ++x)
                    {
                        if (codes[static_cast<std::size_t>(y) * static_cast<std::size_t>(W) +
                                  static_cast<std::size_t>(x)] == want)
                        {
                            grid[rowBase + static_cast<std::size_t>(((wi0 + x) - ci0) / lod)] += share;
                        }
                    }
                }
                const OverlayColor ink = kind == 0 ? Neutral(cls) : OfMoment(cls, world, pass == 0 ? 0.70f : 1.0f);
                FillField(p, grid.data(), CW, CH, ci0, cj0, lod, 0.0f,
                          cls == town::kPlanCliff ? kCliffLevel : kInkLevel, ink, corner);
            }
        }
    }

    // ---- the fog: soft layers, thin at the edge and thick inside, then lighter and darker wisps on the thick part
    const OverlayColor base =
        top == 0 ? OverlayColor{0.62f, 0.54f, 0.68f, 1.0f} : OverlayColor{0.38f, 0.52f, 0.62f, 1.0f};
    const OverlayColor pale =
        top == 0 ? OverlayColor{0.86f, 0.80f, 0.90f, 1.0f} : OverlayColor{0.72f, 0.86f, 0.94f, 1.0f};
    const OverlayColor deep =
        top == 0 ? OverlayColor{0.40f, 0.32f, 0.50f, 1.0f} : OverlayColor{0.18f, 0.30f, 0.42f, 1.0f};
    const float thr[4] = {0.12f, 0.32f, 0.55f, 0.80f};
    const float alpha[4] = {0.20f, 0.26f, 0.40f, 0.88f};
    for (int k = 0; k < 4; ++k)
    {
        OverlayColor c = base;
        c.a = alpha[k];
        FillField(p, fog.data(), W, H, wi0, wj0, 1, 0.0f, thr[k], c, corner);
    }
    std::vector<float> mist(cells);
    for (std::size_t k = 0; k < cells; ++k)
    {
        mist[k] = wisp[k] > 0.0f ? wisp[k] : 0.0f;
    }
    OverlayColor c = pale;
    c.a = 0.10f;
    FillField(p, mist.data(), W, H, wi0, wj0, 1, 0.0f, 0.64f, c, corner);
    for (std::size_t k = 0; k < cells; ++k)
    {
        mist[k] = wisp[k] > 0.0f ? 1.0f - wisp[k] : 0.0f;
    }
    c = deep;
    c.a = 0.07f;
    FillField(p, mist.data(), W, H, wi0, wj0, 1, 0.0f, 0.66f, c, corner);
}

/// A shape of a marker as points round (0, 0), radius r.
struct ShapePoints
{
    float x[12];
    float y[12];
    int n = 0;
};

inline ShapePoints MarkerShape(int type, float r)
{
    ShapePoints s{};
    const auto add = [&](float x, float y)
    {
        s.x[s.n] = x;
        s.y[s.n] = y;
        ++s.n;
    };
    switch (type)
    {
    case 1: // diamond
        add(0.0f, -1.25f * r);
        add(r, 0.0f);
        add(0.0f, 1.25f * r);
        add(-r, 0.0f);
        break;
    case 2: // round
        for (int k = 0; k < 12; ++k)
        {
            const float a = 6.2831853f * static_cast<float>(k) / 12.0f;
            add(std::sin(a) * r, -std::cos(a) * r);
        }
        break;
    case 3: // triangle
        add(0.0f, -1.2f * r);
        add(1.1f * r, 0.8f * r);
        add(-1.1f * r, 0.8f * r);
        break;
    case 4: // square
        add(-0.95f * r, -0.95f * r);
        add(0.95f * r, -0.95f * r);
        add(0.95f * r, 0.95f * r);
        add(-0.95f * r, 0.95f * r);
        break;
    default: // star (type 5)
        for (int k = 0; k < 10; ++k)
        {
            const float a = 6.2831853f * static_cast<float>(k) / 10.0f;
            const float rr = (k % 2 == 0 ? 1.3f : 0.55f) * r;
            add(std::sin(a) * rr, -std::cos(a) * rr);
        }
        break;
    }
    return s;
}

/// One marker at (mx, my): type 0 is a flag on a stem, the others a shape with a dark edge.
inline void DrawMarker(OverlayBatch& b, int type, float mx, float my, float u, const OverlayColor& ink)
{
    const OverlayColor edge{0.10f, 0.09f, 0.09f, ink.a};
    if (type <= 0)
    {
        b.AddLine(mx, my, mx, my - 9.0f * u, std::max(1.0f, 0.8f * u), {0.12f, 0.12f, 0.14f, ink.a});
        b.AddTriangle(mx, my - 9.0f * u, mx + 6.5f * u, my - 6.8f * u, mx, my - 4.6f * u, ink);
        return;
    }
    const auto fan = [&](float r, const OverlayColor& c)
    {
        const ShapePoints sp = MarkerShape(type, r);
        for (int k = 0; k < sp.n; ++k)
        {
            const int k2 = (k + 1) % sp.n;
            b.AddTriangle(mx, my, mx + sp.x[k], my + sp.y[k], mx + sp.x[k2], my + sp.y[k2], c);
        }
    };
    fan(5.0f * u, edge);
    fan(3.7f * u, ink);
}

/// A ring (a polygon of lines).
inline void Ring(OverlayBatch& b, const MapFrame& f, float cx, float cy, float r, float thickness,
                 const OverlayColor& c)
{
    constexpr int kSegments = 48;
    float lx = 0.0f;
    float ly = 0.0f;
    for (int k = 0; k <= kSegments; ++k)
    {
        const float a = 6.2831853f * static_cast<float>(k) / static_cast<float>(kSegments);
        const float x = std::clamp(cx + std::sin(a) * r, f.px, f.px + f.size);
        const float y = std::clamp(cy - std::cos(a) * r, f.py, f.py + f.size);
        if (k > 0)
        {
            b.AddLine(lx, ly, x, y, thickness, c);
        }
        lx = x;
        ly = y;
    }
}

inline void Disc(OverlayBatch& b, const MapFrame& f, float cx, float cy, float r, const OverlayColor& c)
{
    constexpr int kSegments = 32;
    float lx = 0.0f;
    float ly = 0.0f;
    for (int k = 0; k <= kSegments; ++k)
    {
        const float a = 6.2831853f * static_cast<float>(k) / static_cast<float>(kSegments);
        const float x = std::clamp(cx + std::sin(a) * r, f.px, f.px + f.size);
        const float y = std::clamp(cy - std::cos(a) * r, f.py, f.py + f.size);
        if (k > 0)
        {
            b.AddTriangle(cx, cy, lx, ly, x, y, c);
        }
        lx = x;
        ly = y;
    }
}

/// The things that change, in the state they are in now: the footprint of every box of that state, in the moment the
/// box belongs to (the sheet on top is drawn strongly, the other one faintly), only where the player has seen the
/// place. A thing in its second state has a pale outline. Bells are not drawn (the lamps of the mast show them).
inline void DrawThings(OverlayBatch& b, const MapScene& s, const MapFrame& f, float u, int top)
{
    if (s.town == nullptr || s.memory == nullptr || !s.town->plan.Valid())
    {
        return;
    }
    const town::Plan& plan = s.town->plan;
    const std::size_t n = std::min(s.town->things.size(), s.thingOpen.size());
    const auto clampX = [&](float x) { return std::clamp(x, f.px, f.px + f.size); };
    const auto clampY = [&](float y) { return std::clamp(y, f.py, f.py + f.size); };
    for (std::size_t i = 0; i < n; ++i)
    {
        const town::ThingDef& thing = s.town->things[i];
        if (thing.verb == town::Verb::Ring)
        {
            continue;
        }
        const std::uint8_t state = s.thingOpen[i] ? 1 : 0;
        for (const town::ThingBox& box : thing.boxes)
        {
            if (box.state != state || !s.memory->Seen(plan.CellX(box.centre.x), plan.CellZ(box.centre.z)))
            {
                continue;
            }
            const float c = std::cos(box.yaw);
            const float sn = std::sin(box.yaw);
            // never smaller than a few pixels: a crate of 1.2 m would be a dot
            const float minHalf = 1.6f * u / f.Scale();
            const float hx = std::max(box.half.x, minHalf);
            const float hz = std::max(box.half.z, minHalf);
            float sx[4];
            float sy[4];
            const float lx[4] = {-hx, hx, hx, -hx};
            const float lz[4] = {-hz, -hz, hz, hz};
            for (int k = 0; k < 4; ++k)
            {
                f.ToScreen(box.centre.x + lx[k] * c - lz[k] * sn, box.centre.z + lx[k] * sn + lz[k] * c, sx[k], sy[k]);
                sx[k] = clampX(sx[k]);
                sy[k] = clampY(sy[k]);
            }
            for (int pass = 0; pass < 2; ++pass)
            {
                const int world = pass == 0 ? 1 - top : top; // the sheet underneath first, the one on top over it
                if ((box.worlds & (1u << world)) == 0u)
                {
                    continue;
                }
                const bool onTop = world == top;
                OverlayColor ink =
                    world == 0 ? OverlayColor{0.55f, 0.22f, 0.04f, 1.0f} : OverlayColor{0.04f, 0.22f, 0.50f, 1.0f};
                ink.a = onTop ? 0.92f : 0.30f;
                b.AddTriangle(sx[0], sy[0], sx[1], sy[1], sx[2], sy[2], ink);
                b.AddTriangle(sx[0], sy[0], sx[2], sy[2], sx[3], sy[3], ink);
                if (state == 1)
                {
                    const OverlayColor edge{0.98f, 0.94f, 0.80f, onTop ? 0.95f : 0.35f};
                    const float t = std::max(1.0f, 0.5f * u);
                    for (int k = 0; k < 4; ++k)
                    {
                        b.AddLine(sx[k], sy[k], sx[(k + 1) % 4], sy[(k + 1) % 4], t, edge);
                    }
                }
            }
        }
    }
}

} // namespace map_detail

/// Draws the map over the picture: the dimmed screen, the paper with the two sheets under the fog, the marks, the
/// buttons and the keys at its foot. With a `cache` the ink and the fog are made again only when they change.
inline void BuildMap(OverlayBatch& b, int width, int height, const MapScene& s, const MapFrame& f,
                     MapCache* cache = nullptr)
{
    namespace md = map_detail;
    if (!f.valid || s.town == nullptr)
    {
        return;
    }
    const float u = std::max(1.0f, f.size / 180.0f); // one unit of the marks: about 3.5 px on a 640 px paper
    const town::Plan& plan = s.town->plan;
    const int top = s.top == 1 ? 1 : 0;
    const int bottom = 1 - top;
    const float scale = f.Scale();
    const auto onPaper = [&](float x, float y, float m)
    { return x >= f.px - m && x <= f.px + f.size + m && y >= f.py - m && y <= f.py + f.size + m; };
    // the room is dimmed, the paper has a shadow and a cream ground
    b.AddRect(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height), {0.0f, 0.0f, 0.0f, 0.55f});
    b.AddRect(f.px + 6.0f, f.py + 6.0f, f.size, f.size, {0.0f, 0.0f, 0.0f, 0.35f});
    b.AddRect(f.px, f.py, f.size, f.size, {0.94f, 0.90f, 0.80f, 0.97f});
    // the ink and the fog
    if (plan.Valid())
    {
        if (cache != nullptr)
        {
            MapCacheKey key;
            key.plan = &plan;
            key.memory = s.memory;
            key.seen = s.memory != nullptr ? s.memory->SeenCount() : -1;
            key.top = top;
            key.mode = s.mode;
            key.px = f.px;
            key.py = f.py;
            key.size = f.size;
            key.wx0 = f.wx0;
            key.wz1 = f.wz1;
            key.extent = f.extent;
            if (!cache->layer)
            {
                cache->layer = std::make_unique<OverlayBatch>();
                cache->valid = false;
            }
            if (!cache->valid || !(cache->key == key))
            {
                cache->layer->Clear();
                md::BuildTerrain(*cache->layer, s, f);
                cache->key = key;
                cache->valid = true;
                ++cache->rebuilds;
            }
            b.Append(*cache->layer);
        }
        else
        {
            md::BuildTerrain(b, s, f);
        }
    }
    // the things that change, as they are now
    md::DrawThings(b, s, f, u, top);
    // the puzzles to solve, through the fog: broad rings, not the place itself. The next one is the strongest.
    {
        const OverlayColor halo =
            top == 0 ? OverlayColor{0.98f, 0.66f, 0.16f, 1.0f} : OverlayColor{0.42f, 0.80f, 1.0f, 1.0f};
        const std::size_t n = std::min<std::size_t>(s.town->steps.size(), s.done.size());
        for (std::size_t k = 0; k < n; ++k)
        {
            if (s.done[k])
            {
                continue;
            }
            const bool goal = static_cast<int>(k) == s.goal;
            const Vec3& at = s.town->steps[k].at;
            // the ring is not centred on the place: the hand of a map-maker, not a pin
            const float wx = at.x + (md::Hash01(static_cast<int>(k), 3, 11u) - 0.5f) * 2.0f * md::kHaloJitter * 0.5f;
            const float wz = at.z + (md::Hash01(static_cast<int>(k), 5, 12u) - 0.5f) * 2.0f * md::kHaloJitter * 0.5f;
            float hx = 0.0f;
            float hy = 0.0f;
            f.ToScreen(wx, wz, hx, hy);
            const float r = std::max((goal ? 1.0f : 0.7f) * md::kHaloMetres * scale, (goal ? 14.0f : 10.0f) * u * 0.5f);
            if (!onPaper(hx, hy, 0.0f))
            {
                continue;
            }
            const float t = std::max(1.0f, (goal ? 0.7f : 0.45f) * u);
            if (goal)
            {
                md::Disc(b, f, hx, hy, r, {halo.r, halo.g, halo.b, 0.16f});
                md::Ring(b, f, hx, hy, 0.62f * r, t, {halo.r, halo.g, halo.b, 0.70f});
            }
            md::Ring(b, f, hx, hy, r, t, {halo.r, halo.g, halo.b, goal ? 0.95f : 0.60f});
            const float text = std::max(2.0f, std::floor((goal ? 1.4f : 1.0f) * u * 0.8f));
            b.AddText(hx - 0.5f * OverlayBatch::TextWidth("?", text), hy - 0.5f * OverlayBatch::TextHeight(text), text,
                      "?", {halo.r, halo.g, halo.b, goal ? 1.0f : 0.75f});
        }
    }
    // the frame: in the colour of the sheet on top
    const OverlayColor topTint = top == 0 ? md::Warm(1.0f) : md::Cool(1.0f);
    b.AddFrame(f.px - 3.0f, f.py - 3.0f, f.size + 6.0f, f.size + 6.0f, 3.0f, topTint);
    b.AddFrame(f.px, f.py, f.size, f.size, 1.0f, {0.25f, 0.22f, 0.20f, 0.8f});
    // the two sheets, as two chips in the corner: the one on top is in front, named beside them
    {
        const float cx = f.px + 10.0f * u;
        const float cy = f.py + 10.0f * u;
        const OverlayColor colour[2] = {md::Warm(0.95f), md::Cool(0.95f)};
        b.AddRect(cx + 4.0f * u, cy + 4.0f * u, 9.0f * u, 9.0f * u, colour[bottom]);
        b.AddRect(cx, cy, 9.0f * u, 9.0f * u, colour[top]);
        b.AddFrame(cx, cy, 9.0f * u, 9.0f * u, std::max(1.0f, 0.6f * u), {0.15f, 0.13f, 0.12f, 0.9f});
        const float text = std::max(2.0f, std::floor(u * 0.8f));
        const char* name = top == 0 ? "A  PAST" : "B  NOW";
        const float tx = cx + 16.0f * u;
        const float ty = cy + 0.5f * (9.0f * u - OverlayBatch::TextHeight(text));
        b.AddRect(tx - 3.0f, ty - 3.0f, OverlayBatch::TextWidth(name, text) + 6.0f,
                  OverlayBatch::TextHeight(text) + 6.0f, {0.0f, 0.0f, 0.0f, 0.45f});
        b.AddText(tx, ty, text, name, {1.0f, 1.0f, 1.0f, 1.0f});
    }
    // north: a small arrow with its letter, top right
    {
        const float cx = f.px + f.size - 12.0f * u;
        const float cy = f.py + 14.0f * u;
        b.AddTriangle(cx, cy - 7.0f * u, cx + 3.5f * u, cy + 3.0f * u, cx, cy + 1.0f * u, {0.20f, 0.17f, 0.16f, 0.95f});
        b.AddTriangle(cx, cy - 7.0f * u, cx - 3.5f * u, cy + 3.0f * u, cx, cy + 1.0f * u, {0.45f, 0.40f, 0.36f, 0.95f});
        const float text = std::max(2.0f, std::floor(u * 0.8f));
        b.AddText(cx - 0.5f * OverlayBatch::TextWidth("N", text), cy + 5.0f * u, text, "N",
                  {0.20f, 0.17f, 0.16f, 0.95f});
    }
    // the tower and the three lamps of its mast: always there
    Vec3 mast;
    if (s.town->MastAt(mast))
    {
        float tx = 0.0f;
        float ty = 0.0f;
        f.ToScreen(mast.x, mast.z, tx, ty);
        if (onPaper(tx, ty - 10.0f * u, 0.0f))
        {
            b.AddLine(tx, ty, tx, ty - 6.0f * u, std::max(1.0f, 0.7f * u), {0.18f, 0.16f, 0.16f, 0.95f});
            const int n = static_cast<int>(s.town->emblems.size());
            for (int k = 0; k < n; ++k)
            {
                const float ex = tx + (static_cast<float>(k) - 0.5f * static_cast<float>(n - 1)) * 7.0f * u;
                const float ey = ty - 10.0f * u;
                const bool lit = k < static_cast<int>(s.lit.size()) && s.lit[static_cast<std::size_t>(k)];
                b.AddRect(ex - 3.2f * u, ey - 3.2f * u, 6.4f * u, 6.4f * u,
                          lit ? OverlayColor{1.0f, 0.80f, 0.40f, 1.0f} : OverlayColor{0.34f, 0.35f, 0.40f, 1.0f});
                b.AddFrame(ex - 3.2f * u, ey - 3.2f * u, 6.4f * u, 6.4f * u, std::max(1.0f, 0.7f * u),
                           {0.14f, 0.12f, 0.12f, 1.0f});
            }
        }
    }
    // the lantern, set down
    if (s.hasLantern)
    {
        float lx = 0.0f;
        float ly = 0.0f;
        f.ToScreen(s.lantern.x, s.lantern.z, lx, ly);
        if (onPaper(lx, ly, 0.0f))
        {
            const auto diamond = [&](float r, const OverlayColor& c)
            {
                b.AddTriangle(lx, ly - r, lx + r, ly, lx, ly + r, c);
                b.AddTriangle(lx, ly - r, lx - r, ly, lx, ly + r, c);
            };
            diamond(4.6f * u, {0.15f, 0.13f, 0.12f, 1.0f});
            diamond(3.4f * u, {1.0f, 0.78f, 0.38f, 1.0f});
        }
    }
    // the markers: the shape the player chose, in the colour of the moment it was set in (paler on the other sheet)
    if (s.memory != nullptr)
    {
        for (int k = 0; k < s.memory->MarkerCount(); ++k)
        {
            const town::MapMarker& m = s.memory->Marker(k);
            float mx = 0.0f;
            float my = 0.0f;
            f.ToScreen(m.x, m.z, mx, my);
            if (onPaper(mx, my, 0.0f))
            {
                md::DrawMarker(b, m.type, mx, my, u, md::MarkerInk(m.layer, m.layer == top ? 1.0f : 0.55f));
            }
        }
    }
    // the player: an arrow that points where the camera looks
    {
        float ax = 0.0f;
        float ay = 0.0f;
        f.ToScreen(s.player.x, s.player.z, ax, ay);
        if (onPaper(ax, ay, 0.0f))
        {
            const float dx = std::sin(s.yaw);
            const float dy = -std::cos(s.yaw);
            const float nx = -dy;
            const float ny = dx;
            const auto arrow = [&](float k, const OverlayColor& c)
            {
                b.AddTriangle(ax + dx * 8.0f * u * k, ay + dy * 8.0f * u * k,
                              ax - dx * 5.0f * u * k + nx * 5.0f * u * k, ay - dy * 5.0f * u * k + ny * 5.0f * u * k,
                              ax - dx * 2.0f * u * k, ay - dy * 2.0f * u * k, c);
                b.AddTriangle(ax + dx * 8.0f * u * k, ay + dy * 8.0f * u * k,
                              ax - dx * 5.0f * u * k - nx * 5.0f * u * k, ay - dy * 5.0f * u * k - ny * 5.0f * u * k,
                              ax - dx * 2.0f * u * k, ay - dy * 2.0f * u * k, c);
            };
            arrow(1.35f, {0.96f, 0.94f, 0.88f, 1.0f});
            arrow(1.0f, {0.60f, 0.10f, 0.08f, 1.0f});
        }
    }
    // the buttons beside the paper: clickable, so the mouse does all of it
    {
        const char* labels[kOptCount] = {"MAP", "DIFFERENCE", "ZOOM +", "ZOOM -", "CENTER", "MARK"};
        for (int k = 0; k < kOptCount; ++k)
        {
            float bx = 0.0f, by = 0.0f, bw = 0.0f, bh = 0.0f;
            MapOptionRect(f, k, bx, by, bw, bh);
            const bool active = (k == kOptMap && s.mode == 0) || (k == kOptDifference && s.mode == 1);
            const OverlayColor fill =
                active ? (top == 0 ? md::Warm(0.90f) : md::Cool(0.90f)) : OverlayColor{0.10f, 0.12f, 0.14f, 0.78f};
            b.AddRect(bx, by, bw, bh, fill);
            b.AddFrame(bx, by, bw, bh, 1.0f, {0.94f, 0.90f, 0.80f, 0.95f});
            const float fit = std::floor((bw - 16.0f) / (static_cast<float>(10 * font::kAdvance - 1)));
            const float text = std::max(1.0f, std::min(std::floor(bh / 11.0f), fit));
            b.AddText(bx + 8.0f, by + 0.5f * (bh - OverlayBatch::TextHeight(text)), text, labels[k], {1, 1, 1, 1});
            if (k == kOptMarker)
            {
                // the shape the next marker will have, at the right end of the button (a flag stands on its point)
                md::DrawMarker(b, s.markerType, bx + bw - 1.1f * bh, by + (s.markerType <= 0 ? 0.92f : 0.5f) * bh,
                               std::max(0.5f, 0.075f * bh), md::MarkerInk(top, 1.0f));
            }
        }
    }

    // the keys at the foot of the paper
    {
        float unit = std::max(2.0f, std::floor(static_cast<float>(height) / 320.0f));
        struct Hint
        {
            const char* key;
            const char* verb;
        };
        const Hint hints[5] = {{"TAB", "TURN SHEET"}, {"E", "MARK"}, {"T", "TYPE"}, {"+ -", "ZOOM"}, {"M", "CLOSE"}};
        const auto measure = [&](float un)
        {
            const float pad = 2.0f * un;
            float total = 0.0f;
            for (const Hint& h : hints)
            {
                total += OverlayBatch::TextWidth(h.key, un) + 2.0f * pad + 3.0f * un +
                         OverlayBatch::TextWidth(h.verb, un) + 8.0f * un;
            }
            return total - 8.0f * un;
        };
        while (unit > 1.0f && measure(unit) > 0.96f * static_cast<float>(width))
        {
            unit -= 1.0f;
        }
        const float pad = 2.0f * unit;
        const float total = measure(unit);
        float x = std::floor(0.5f * (static_cast<float>(width) - total));
        const float y = f.py + f.size + 3.0f + 5.0f * unit;
        const float row = OverlayBatch::TextHeight(unit) + 2.0f * pad;
        b.AddRect(x - 3.0f * unit, y - pad - unit, total + 6.0f * unit, row + 2.0f * unit, {0.0f, 0.0f, 0.0f, 0.55f});
        for (const Hint& h : hints)
        {
            const float keyWidth = OverlayBatch::TextWidth(h.key, unit) + 2.0f * pad;
            b.AddFrame(x, y - pad, keyWidth, row, std::max(1.0f, std::floor(unit * 0.5f)), {1.0f, 0.86f, 0.45f, 1.0f});
            b.AddText(x + pad, y, unit, h.key, {1.0f, 0.86f, 0.45f, 1.0f});
            b.AddText(x + keyWidth + 3.0f * unit, y, unit, h.verb, {1.0f, 1.0f, 1.0f, 1.0f});
            x += keyWidth + 3.0f * unit + OverlayBatch::TextWidth(h.verb, unit) + 8.0f * unit;
        }
    }
}

} // namespace dl
