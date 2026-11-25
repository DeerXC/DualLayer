// MapView.h - the map: two sheets of tracing paper (docs/DESIGN.md 10.4). Drawn into an OverlayBatch, so it needs no
// new pass. Sheets:    the plan of the past (A) and of the present (B) are laid one over the other. What stands in both
// is drawn once, in
//            a neutral ink; what stands in one moment only is warm (A) or cool (B), the colours of the two moments;
//            where both moments have something and it differs, both are drawn and the sheet on top shows over the
//            other. The player turns the other sheet to the top with Tab.
// Seen:      only the cells the player has seen are drawn (scene/MapMemory.h); the rest is blank paper. The tower, with
// its
//            three emblems, is always drawn: it is the goal, and it was in sight from the first frame.
// Holds:     the player (an arrow that points the way the camera looks), the lantern, the tower with the lit and dark
// lamps of
//            its mast, the markers the player left. Never: a corner minimap, a list, the place of anything unseen, or
//            what to do. No words but the keys at the foot of the paper.
// Window:    the paper shows the seen part of the plan with a margin, and always the player and the tower, at least 90
// m across;
//            the plan keeps its north up.
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

namespace dl
{

/// What the map shows now.
struct MapScene
{
    const town::TownData* town = nullptr;
    const town::MapMemory* memory = nullptr;
    int top = 0; ///< the sheet on top: 0 = A, 1 = B
    Vec3 player{};
    float yaw = 0.0f;        ///< where the camera looks (0 = north, + toward east)
    bool hasLantern = false; ///< the lantern is set down
    Vec3 lantern{};
    std::array<bool, 16> lit{}; ///< which emblems are lit (the order of town.emblems)
};

/// Where the paper is on the screen, and which part of the plan it shows.
struct MapFrame
{
    bool valid = false;
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

namespace map_detail
{

inline constexpr float kMinExtent = 90.0f; ///< UNITS: metres: the window is never smaller than this
inline constexpr float kMargin = 14.0f;    ///< UNITS: metres round what is seen

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

inline float Snap(float v)
{
    return std::floor(v + 0.5f);
}

} // namespace map_detail

/// Where the paper goes and what part of the plan it shows. `valid` is false without a town or a screen.
inline MapFrame ComputeMapFrame(int width, int height, const MapScene& s)
{
    namespace md = map_detail;
    MapFrame f;
    if (s.town == nullptr || width <= 0 || height <= 0)
    {
        return f;
    }
    const town::Plan& plan = s.town->plan;
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
        int i0 = 0;
        int i1 = 0;
        int j0 = 0;
        int j1 = 0;
        if (plan.Valid() && s.memory->SeenBounds(i0, i1, j0, j1))
        {
            add(plan.x0 + static_cast<float>(i0) * plan.cell, plan.z0 + static_cast<float>(j0) * plan.cell);
            add(plan.x0 + static_cast<float>(i1 + 1) * plan.cell, plan.z0 + static_cast<float>(j1 + 1) * plan.cell);
        }
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
    if (plan.Valid())
    {
        const float west = plan.x0;
        const float east = plan.x0 + static_cast<float>(plan.nx) * plan.cell;
        const float south = plan.z0;
        const float north = plan.z0 + static_cast<float>(plan.nz) * plan.cell;
        if (f.extent <= east - west)
        {
            f.wx0 = std::clamp(f.wx0, west, east - f.extent);
        }
        if (f.extent <= north - south)
        {
            f.wz1 = std::clamp(f.wz1, south + f.extent, north);
        }
    }
    f.size = std::floor(0.80f * static_cast<float>(std::min(width, height)));
    f.px = std::floor(0.5f * (static_cast<float>(width) - f.size));
    f.py = std::floor(0.5f * (static_cast<float>(height) - f.size) - 0.03f * static_cast<float>(height));
    f.valid = f.size > 8.0f;
    return f;
}

/// Draws the map over the picture: the dimmed screen, the paper with the two sheets, the marks and the keys at its
/// foot.
inline void BuildMap(OverlayBatch& b, int width, int height, const MapScene& s, const MapFrame& f)
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
    // the room is dimmed, the paper has a shadow, a cream ground and a frame in the colour of the sheet on top
    b.AddRect(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height), {0.0f, 0.0f, 0.0f, 0.55f});
    b.AddRect(f.px + 6.0f, f.py + 6.0f, f.size, f.size, {0.0f, 0.0f, 0.0f, 0.35f});
    b.AddRect(f.px, f.py, f.size, f.size, {0.94f, 0.90f, 0.80f, 0.97f});
    if (plan.Valid())
    {
        const float scale = f.Scale();
        const int i0 = std::max(0, plan.CellX(f.wx0));
        const int i1 = std::min(plan.nx - 1, plan.CellX(f.wx0 + f.extent));
        const int j0 = std::max(0, plan.CellZ(f.wz1 - f.extent));
        const int j1 = std::min(plan.nz - 1, plan.CellZ(f.wz1));
        const float left = f.px;
        const float right = f.px + f.size;
        const float up = f.py;
        const float down = f.py + f.size;
        const auto edgeX = [&](int i)
        {
            return std::clamp(md::Snap(f.px + (plan.x0 + static_cast<float>(i) * plan.cell - f.wx0) * scale), left,
                              right);
        };
        const auto edgeY =
            [&](int j) // the south edge of row j - 1 is the north edge of row j... the edge at z0 + j * cell
        {
            return std::clamp(md::Snap(f.py + (f.wz1 - (plan.z0 + static_cast<float>(j) * plan.cell)) * scale), up,
                              down);
        };
        const auto code = [&](int i, int j, int pass) -> int
        {
            if (s.memory == nullptr || !s.memory->Seen(i, j))
            {
                return 0;
            }
            const int a = plan.At(0, i, j);
            const int c = plan.At(1, i, j);
            if (a == c)
            {
                return pass == 0 ? a : 0; // in both moments, the same: one neutral ink
            }
            const int world = pass == 0 ? bottom : top;
            const int cls = world == 0 ? a : c;
            return cls != 0 ? (0x100 | (world << 4) | cls) : 0;
        };
        for (int pass = 0; pass < 2; ++pass)
        {
            for (int j = j1; j >= j0; --j)
            {
                const float ytop = edgeY(j + 1);
                const float ybot = edgeY(j);
                if (!(ybot > ytop))
                {
                    continue;
                }
                int runStart = i0;
                int runCode = code(i0, j, pass);
                for (int i = i0 + 1; i <= i1 + 1; ++i)
                {
                    const int c = i <= i1 ? code(i, j, pass) : -1;
                    if (c == runCode)
                    {
                        continue;
                    }
                    if (runCode != 0)
                    {
                        OverlayColor ink;
                        if (runCode < 0x100)
                        {
                            ink = md::Neutral(runCode);
                        }
                        else
                        {
                            ink = md::OfMoment(runCode & 15, (runCode >> 4) & 1, pass == 0 ? 0.70f : 1.0f);
                        }
                        const float xa = edgeX(runStart);
                        const float xb = edgeX(i);
                        b.AddRect(xa, ytop, xb - xa, ybot - ytop, ink);
                    }
                    runStart = i;
                    runCode = c;
                }
            }
        }
    }
    // the frame: in the colour of the sheet on top
    const OverlayColor topTint = top == 0 ? md::Warm(1.0f) : md::Cool(1.0f);
    b.AddFrame(f.px - 3.0f, f.py - 3.0f, f.size + 6.0f, f.size + 6.0f, 3.0f, topTint);
    b.AddFrame(f.px, f.py, f.size, f.size, 1.0f, {0.25f, 0.22f, 0.20f, 0.8f});
    // the two sheets, as two chips in the corner: the one on top is in front
    {
        const float cx = f.px + 10.0f * u;
        const float cy = f.py + 10.0f * u;
        const OverlayColor colour[2] = {md::Warm(0.95f), md::Cool(0.95f)};
        b.AddRect(cx + 4.0f * u, cy + 4.0f * u, 9.0f * u, 9.0f * u, colour[bottom]);
        b.AddRect(cx, cy, 9.0f * u, 9.0f * u, colour[top]);
        b.AddFrame(cx, cy, 9.0f * u, 9.0f * u, std::max(1.0f, 0.6f * u), {0.15f, 0.13f, 0.12f, 0.9f});
    }
    // the tower and the three lamps of its mast: always there
    Vec3 mast;
    if (s.town->MastAt(mast))
    {
        float tx = 0.0f;
        float ty = 0.0f;
        f.ToScreen(mast.x, mast.z, tx, ty);
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
    // the lantern, set down
    if (s.hasLantern)
    {
        float lx = 0.0f;
        float ly = 0.0f;
        f.ToScreen(s.lantern.x, s.lantern.z, lx, ly);
        const auto diamond = [&](float r, const OverlayColor& c)
        {
            b.AddTriangle(lx, ly - r, lx + r, ly, lx, ly + r, c);
            b.AddTriangle(lx, ly - r, lx - r, ly, lx, ly + r, c);
        };
        diamond(4.6f * u, {0.15f, 0.13f, 0.12f, 1.0f});
        diamond(3.4f * u, {1.0f, 0.78f, 0.38f, 1.0f});
    }
    // the markers: a flag on a stem
    if (s.memory != nullptr)
    {
        for (int k = 0; k < s.memory->MarkerCount(); ++k)
        {
            float mx = 0.0f;
            float my = 0.0f;
            f.ToScreen(s.memory->Marker(k).x, s.memory->Marker(k).z, mx, my);
            b.AddLine(mx, my, mx, my - 9.0f * u, std::max(1.0f, 0.8f * u), {0.12f, 0.12f, 0.14f, 1.0f});
            b.AddTriangle(mx, my - 9.0f * u, mx + 6.5f * u, my - 6.8f * u, mx, my - 4.6f * u,
                          {0.08f, 0.56f, 0.88f, 1.0f});
        }
    }
    // the player: an arrow that points where the camera looks
    {
        float ax = 0.0f;
        float ay = 0.0f;
        f.ToScreen(s.player.x, s.player.z, ax, ay);
        const float dx = std::sin(s.yaw);
        const float dy = -std::cos(s.yaw);
        const float nx = -dy;
        const float ny = dx;
        const auto arrow = [&](float k, const OverlayColor& c)
        {
            b.AddTriangle(ax + dx * 8.0f * u * k, ay + dy * 8.0f * u * k, ax - dx * 5.0f * u * k + nx * 5.0f * u * k,
                          ay - dy * 5.0f * u * k + ny * 5.0f * u * k, ax - dx * 2.0f * u * k, ay - dy * 2.0f * u * k,
                          c);
            b.AddTriangle(ax + dx * 8.0f * u * k, ay + dy * 8.0f * u * k, ax - dx * 5.0f * u * k - nx * 5.0f * u * k,
                          ay - dy * 5.0f * u * k - ny * 5.0f * u * k, ax - dx * 2.0f * u * k, ay - dy * 2.0f * u * k,
                          c);
        };
        arrow(1.35f, {0.96f, 0.94f, 0.88f, 1.0f});
        arrow(1.0f, {0.60f, 0.10f, 0.08f, 1.0f});
    }
    // the keys at the foot of the paper: the only words of the map
    {
        const float unit = std::max(2.0f, std::floor(static_cast<float>(height) / 320.0f));
        struct Hint
        {
            const char* key;
            const char* verb;
        };
        const Hint hints[3] = {{"TAB", "TURN THE SHEET"}, {"E", "MARK"}, {"M", "CLOSE"}};
        const float pad = 2.0f * unit;
        float total = 0.0f;
        for (const Hint& h : hints)
        {
            total += OverlayBatch::TextWidth(h.key, unit) + 2.0f * pad + 3.0f * unit +
                     OverlayBatch::TextWidth(h.verb, unit) + 8.0f * unit;
        }
        total -= 8.0f * unit;
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
