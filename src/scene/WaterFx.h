// WaterFx.h - what water does besides standing there (docs/LEVEL_DESIGN.md 5.9): its foam, its falls between the locks, the
//            wet band it leaves on the walls when it goes down, the splash of a body that falls in, and the view from under it.
// Sheets:    every surface of water of A now (CollectWaters): the static waters as the Waterworks hold them (raised by the
//            dawn bell, drained at the end) and the canal as its lock holds it.
// Foam:      a thin rim of foam round every sheet where it meets its walls, and a pad of white water at the foot of every fall
//            (AddFoam). Drawn as scene::kMatFoam: the shader breaks it into moving flecks.
// Cascades:  between two locks the water of the upper one runs out through the lock head and falls into the lower (AddCascades),
//            and out of the tower lock into the upper lock: three falls of scene::kMatFlow, each with its foam.
// Wet band:  when a surface goes down (the canal let out, every water drained at the end) the walls stay wet where it stood: a
//            dark band from the new surface up to the old, that dries in kDrySeconds (WetMarks). Rising water leaves none.
// Splash:    a body that falls into water sinks for kSinkSeconds before it comes back (main.cpp): rings of foam spread on the
//            surface where it went in, drops fly up and fall back (Splashes). Under the surface the screen is tinted
//            (Underwater, drawn by main.cpp).
// Everything here is drawn only, layer A (the water of B has gone), and never solid.
// Plain data and plain logic, no Windows, no Direct3D: tested without a window (tests/test_terrain.cpp, TestWaterFx).
// UNITS: metres, seconds.
#pragma once

#include "scene/Canal.h"
#include "scene/SceneData.h"
#include "scene/Stage.h"

#include <array>
#include <cmath>

namespace dl::stage
{

struct WaterSheet
{
    float x0, z0, x1, z1; ///< the surface
    float top;            ///< world y of the surface now
    float bed;            ///< world y of its bed
};

inline constexpr int kMaxSheets = 24;

/// Every surface of water of A now: the static waters (as `works` holds them) and the canal. Returns how many.
inline int CollectWaters(const Waterworks& works, const canal::Canal& canal, WaterSheet* out, int capacity)
{
    int n = 0;
    ForEachWater([&](const shapes::Water& w) {
        if (out != nullptr && n < capacity)
        {
            out[n++] = {w.x0, w.z0, w.x1, w.z1, works.Top(w), w.top - w.depth};
        }
    });
    const canal::Desc& d = canal.Described();
    if (out != nullptr && n < capacity)
    {
        out[n++] = {d.x0, d.z0, d.x1, d.z1, canal.WaterTop(), d.bed};
    }
    return n;
}

/// The surface of the water at (x, z) whose surface is the highest under `y` + 2 m, or false (no water there).
inline bool WaterTopAt(const WaterSheet* sheets, int n, float x, float z, float y, float& top)
{
    bool found = false;
    for (int i = 0; i < n; ++i)
    {
        const WaterSheet& s = sheets[i];
        if (s.top - s.bed > 0.02f && x > s.x0 && x < s.x1 && z > s.z0 && z < s.z1 && s.top < y + 2.0f && (!found || s.top > top))
        {
            top = s.top;
            found = true;
        }
    }
    return found;
}

/// True when a point (the eye) in `layer` is under the surface of a water.
inline bool Underwater(const WaterSheet* sheets, int n, const Vec3& eye, int layer)
{
    for (int i = 0; layer == 0 && i < n; ++i)
    {
        const WaterSheet& s = sheets[i];
        if (eye.x > s.x0 && eye.x < s.x1 && eye.z > s.z0 && eye.z < s.z1 && eye.y < s.top - 0.02f && eye.y > s.bed)
        {
            return true;
        }
    }
    return false;
}

// ---- foam and cascades

inline constexpr Rgb kFoamRgb{0.92f, 0.95f, 0.96f};
inline constexpr float kRimWidth = 0.22f;   ///< UNITS: metres of foam along the walls
inline constexpr float kFoamOver = 0.012f;  ///< UNITS: metres the foam stands over the surface (never fights with it)

inline SceneObject Foam(float x0, float y0, float z0, float x1, float y1, float z1, float opacity)
{
    SceneObject o = Span(x0, y0, z0, x1, y1, z1, kFoamRgb, kLayerA);
    o.checker = scene::kMatFoam;
    o.glow = opacity;
    o.shineStrength = 0.0f;
    return o;
}

/// One fall from (x, z) on a lock head: a sheet of running water `width` wide (along z), from `from` down to `to`, its face on
/// x = `face` toward -x, and the white water at its foot.
inline int AddFall(float face, float zMid, float width, float from, float to, SceneObject* out, int count, int capacity)
{
    if (out == nullptr || capacity - count < 2 || from - to < 0.1f)
    {
        return count;
    }
    SceneObject sheet = Span(face - 0.22f, to - 0.05f, zMid - 0.5f * width, face, from, zMid + 0.5f * width, kWaterRgb, kLayerA);
    sheet.checker = scene::kMatFlow;
    sheet.glow = 0.85f;
    out[count++] = sheet;
    out[count++] = Foam(face - 1.6f, to, zMid - 0.5f * width - 0.4f, face, to + kFoamOver + 0.02f, zMid + 0.5f * width + 0.4f, 0.9f);
    return count;
}

/// The falls between the locks: out of each lock through the head west of it into the lock below, while both hold water.
inline int AddCascades(const Waterworks& works, SceneObject* out, int count, int capacity)
{
    const float zMid = 0.5f * (kBasinZ0 + kLockWalkZ0);
    for (int k = 0; k + 1 < kLockCount; ++k) // lock k + 1 falls into lock k over the head at x kTerraceX1[k]
    {
        const float upper = works.Top(kLockWaters[k + 1]);
        const float lower = works.Top(kLockWaters[k]);
        if (upper - (kLockWaters[k + 1].top - kLockWaters[k + 1].depth) > 0.05f)
        {
            count = AddFall(kTerraceX1[k], zMid, 0.9f, upper - 0.15f, lower, out, count, capacity);
        }
    }
    const float tower = works.Top(kTowerLockWaters[0]);
    const float upper = works.Top(kLockWaters[kLockCount - 1]);
    if (tower - (kTowerLockWaters[0].top - kTowerLockWaters[0].depth) > 0.05f)
    {
        count = AddFall(kTerraceX1[kLockCount - 1], zMid, 0.9f, tower - 0.15f, upper, out, count, capacity);
    }
    return count;
}

/// The rim of foam round every sheet that holds water, and a pad at the foot of the fall of the aqueduct into the pool.
inline int AddFoam(const WaterSheet* sheets, int n, bool aqueductRuns, SceneObject* out, int count, int capacity)
{
    for (int i = 0; i < n && out != nullptr && capacity - count >= 4; ++i)
    {
        const WaterSheet& s = sheets[i];
        if (s.top - s.bed < 0.08f)
        {
            continue; // too shallow to foam
        }
        const float y0 = s.top - 0.01f;
        const float y1 = s.top + kFoamOver;
        const float w = kRimWidth;
        out[count++] = Foam(s.x0, y0, s.z0, s.x1, y1, s.z0 + w, 0.55f);
        out[count++] = Foam(s.x0, y0, s.z1 - w, s.x1, y1, s.z1, 0.55f);
        out[count++] = Foam(s.x0, y0, s.z0 + w, s.x0 + w, y1, s.z1 - w, 0.55f);
        out[count++] = Foam(s.x1 - w, y0, s.z0 + w, s.x1, y1, s.z1 - w, 0.55f);
    }
    if (aqueductRuns && out != nullptr && count < capacity)
    {
        const float pool = kWaters[kPoolWater].top;
        out[count++] = Foam(kAqueductWest - 1.8f, pool - 0.01f, kAqueductZ0 - 0.6f, kAqueductWest + 0.4f, pool + kFoamOver + 0.03f, kAqueductZ1 + 0.6f, 0.95f);
    }
    return count;
}

// ---- the wet band

inline constexpr float kDrySeconds = 25.0f; ///< UNITS: seconds the walls take to dry
inline constexpr float kWetMin = 0.05f;     ///< UNITS: metres: a drop smaller than this leaves no band

class WetMarks
{
  public:
    /// Follows the sheets (the same list, in the same order, every frame): a surface that goes down leaves its old height as a
    /// wet mark, which dries in kDrySeconds; a surface that rises covers it.
    void Update(const WaterSheet* sheets, int n, float dt)
    {
        for (int i = 0; i < n && i < kMaxSheets; ++i)
        {
            Mark& m = m_marks[static_cast<std::size_t>(i)];
            const float top = sheets[i].top;
            if (!m.seen || top >= m.high)
            {
                m.high = top; // rising (or first seen): nothing is wet above the surface
                m.wet = 0.0f;
                m.seen = true;
                continue;
            }
            if (m.high - top > kWetMin && m.wet <= 0.0f && m.age <= 0.0f)
            {
                m.wet = 1.0f; // it has just gone down: the walls are wet from here
            }
            if (m.wet > 0.0f)
            {
                m.age += dt > 0.0f ? dt : 0.0f;
                m.wet = 1.0f - m.age / kDrySeconds;
                if (m.wet <= 0.0f)
                {
                    m.wet = 0.0f;
                    m.high = top; // dry: the mark is gone
                    m.age = 0.0f;
                }
            }
        }
    }

    /// 0 .. 1: how wet the band of sheet `i` is (0: none), and its top.
    float Wetness(int i) const
    {
        return i >= 0 && i < kMaxSheets ? m_marks[static_cast<std::size_t>(i)].wet : 0.0f;
    }
    float High(int i) const
    {
        return i >= 0 && i < kMaxSheets ? m_marks[static_cast<std::size_t>(i)].high : 0.0f;
    }

    /// The bands: for each wet sheet a box from its surface to the mark, 1 cm inside its walls (the shader draws its sides
    /// only, darker at the foot and fading as it dries).
    int AddDraw(const WaterSheet* sheets, int n, SceneObject* out, int count, int capacity) const
    {
        for (int i = 0; i < n && i < kMaxSheets && out != nullptr && count < capacity; ++i)
        {
            const Mark& m = m_marks[static_cast<std::size_t>(i)];
            const WaterSheet& s = sheets[i];
            if (m.wet <= 0.0f || m.high - s.top < kWetMin)
            {
                continue;
            }
            SceneObject band = Span(s.x0 + 0.01f, s.top, s.z0 + 0.01f, s.x1 - 0.01f, m.high, s.z1 - 0.01f, Rgb{0.05f, 0.06f, 0.05f}, kLayerA);
            band.checker = scene::kMatWetBand;
            band.glow = m.wet;
            out[count++] = band;
        }
        return count;
    }

  private:
    struct Mark
    {
        float high = 0.0f;
        float wet = 0.0f;
        float age = 0.0f;
        bool seen = false;
    };
    std::array<Mark, kMaxSheets> m_marks{};
};

// ---- the splash

inline constexpr float kSinkSeconds = 0.55f;  ///< UNITS: seconds a body sinks before it comes back
inline constexpr float kSplashSeconds = 1.6f; ///< UNITS: seconds a splash lasts
inline constexpr int kSplashRings = 2;
inline constexpr int kRingPieces = 14;
inline constexpr int kSplashDrops = 8;
inline constexpr int kSplashDraw = kSplashRings * kRingPieces + kSplashDrops; ///< drawn objects of one splash

class Splashes
{
  public:
    static constexpr int kMax = 4;

    /// A splash where something went into water: `at` on the surface.
    void Spawn(const Vec3& at)
    {
        Splash& s = m_splashes[static_cast<std::size_t>(m_next)];
        s.at = at;
        s.age = 0.0f;
        s.live = true;
        m_next = (m_next + 1) % kMax;
    }

    void Update(float dt)
    {
        for (Splash& s : m_splashes)
        {
            if (s.live)
            {
                s.age += dt > 0.0f ? dt : 0.0f;
                s.live = s.age < kSplashSeconds;
            }
        }
    }

    int Live() const
    {
        int n = 0;
        for (const Splash& s : m_splashes)
        {
            n += s.live ? 1 : 0;
        }
        return n;
    }

    /// Rings of foam spreading on the surface (later rings later and smaller), and drops thrown up and out that fall back
    /// under the surface. Needs at most kMax * kSplashDraw places.
    int AddDraw(SceneObject* out, int count, int capacity) const
    {
        for (const Splash& s : m_splashes)
        {
            if (!s.live || out == nullptr || capacity - count < kSplashDraw)
            {
                continue;
            }
            const float t = s.age / kSplashSeconds; // 0 .. 1
            for (int r = 0; r < kSplashRings; ++r)
            {
                const float tr = t - 0.15f * static_cast<float>(r);
                if (tr <= 0.0f)
                {
                    continue;
                }
                // a ring of foam on the surface, laid as kRingPieces short flat pieces round the circle
                const float radius = (0.4f + 3.0f * std::sqrt(tr)) * (1.0f - 0.25f * static_cast<float>(r));
                const float piece = 6.2831853f * radius / static_cast<float>(kRingPieces) * 1.08f;
                const float opacity = (1.0f - tr) * (1.0f - tr) * 0.9f;
                for (int k = 0; k < kRingPieces; ++k)
                {
                    const float a = 6.2831853f * static_cast<float>(k) / static_cast<float>(kRingPieces);
                    const float px = s.at.x + radius * std::cos(a);
                    const float pz = s.at.z + radius * std::sin(a);
                    SceneObject o = Foam(px - 0.5f * piece, s.at.y - 0.01f, pz - 0.14f, px + 0.5f * piece, s.at.y + kFoamOver, pz + 0.14f, opacity);
                    o.yaw = 1.5707963f - a; // its length along the circle
                    out[count++] = o;
                }
            }
            for (int d = 0; d < kSplashDrops; ++d)
            {
                const float a = 6.2831853f * (static_cast<float>(d) + 0.37f * static_cast<float>(d % 3)) / static_cast<float>(kSplashDrops);
                const float speed = 1.6f + 0.5f * static_cast<float>(d % 3);
                const float up = 3.6f + 0.8f * static_cast<float>((d * 5) % 4);
                const float time = s.age;
                const float y = s.at.y + up * time - 4.9f * time * time;
                if (y < s.at.y - 0.05f)
                {
                    continue; // fallen back in
                }
                const float x = s.at.x + std::cos(a) * speed * time;
                const float z = s.at.z + std::sin(a) * speed * time;
                SceneObject drop{};
                drop.shape = ShapeKind::Sphere;
                drop.position[0] = x;
                drop.position[1] = y;
                drop.position[2] = z;
                drop.scale[0] = drop.scale[1] = drop.scale[2] = 0.12f;
                drop.colorSrgb[0] = kFoamRgb.r;
                drop.colorSrgb[1] = kFoamRgb.g;
                drop.colorSrgb[2] = kFoamRgb.b;
                drop.checker = scene::kMatFoam;
                drop.layerMask = kLayerA;
                drop.glow = 0.9f;
                out[count++] = drop;
            }
        }
        return count;
    }

  private:
    struct Splash
    {
        Vec3 at{};
        float age = 0.0f;
        bool live = false;
    };
    std::array<Splash, kMax> m_splashes{};
    int m_next = 0;
};

/// Everything above for the drawn table of a frame. Needs at most kWaterFxDraw places.
inline constexpr int kWaterFxDraw = 4 * kMaxSheets + 1 + 2 * kLockCount + kMaxSheets + Splashes::kMax * kSplashDraw;

inline int AddWaterFx(const WaterSheet* sheets, int n, const Waterworks& works, const WetMarks& wet, const Splashes& splashes, SceneObject* out,
                      int count, int capacity)
{
    count = AddFoam(sheets, n, !works.Closed(), out, count, capacity);
    count = AddCascades(works, out, count, capacity);
    count = wet.AddDraw(sheets, n, out, count, capacity);
    return splashes.AddDraw(out, count, capacity);
}

} // namespace dl::stage
