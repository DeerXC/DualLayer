// Lights.h - the point lights of the town (street lamps, the lantern, lit lamps of the mast) and the choice of the ones
// a frame sends to the GPU.
// Why:       the shader lights every pixel with the sun or moon of the moment and with at most kMaxLamps point lights
//            (a loop over a constant buffer, no light culling pass). A town has far more lamps than that, so every
//            frame the nearest and strongest ones of the moment on the screen are chosen here (SelectLamps) and the
//            rest are left out.
// Pop:       a lamp that is chosen or left out must not blink on and off as the camera walks: its light fades out
//            between LampFade's two distances, and the ones left out are the least important by the same measure.
// Flicker:   a flame is never still: a lamp with `flicker` > 0 changes its strength by up to that part, two slow sines
//            with a phase of its own, so that two lamps never flicker together.
// Plain data and plain maths, no Windows, no Direct3D: tested without a window (tests/test_lighting.cpp).
// UNITS:     metres, seconds. Colours are LINEAR (the intensity is folded in: 1 is the light of the sun on a lit wall).
#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

namespace dl
{

inline constexpr int kMaxLamps =
    32; ///< KEEP IN SYNC: MAX_LAMPS in shaders/models.hlsl and LampConstants in render/Models.cpp

/// One point light. `layers`: bit 0 = it shines in A (the past), bit 1 in B (the present).
struct PointLight
{
    float pos[3] = {0.0f, 0.0f, 0.0f};
    float range = 10.0f; ///< UNITS: metres; the light is zero from this distance on
    float color[3] = {1.0f, 1.0f, 1.0f};
    unsigned layers = 3u;
    float flicker = 0.0f; ///< 0 .. 0.4: the part of its strength that it loses and gains
    float phase = 0.0f;   ///< radians: where its flicker is at time 0
    float halo =
        0.0f; ///< UNITS: metres: the diameter of the soft glow drawn round it (0: none; scene/Lamps.h AddHalos)
};

/// What goes to the GPU for one moment: the lamps as (x, y, z, range) and (r, g, b, 0), `count` of them.
struct LampBatch
{
    int count = 0;
    float pos[kMaxLamps][4] = {};
    float color[kMaxLamps][4] = {};
};

namespace lamp_detail
{

inline float Smooth(float e0, float e1, float v)
{
    const float t = std::clamp((v - e0) / (e1 - e0), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

} // namespace lamp_detail

/// 1 near the camera, 0 far away: a lamp is at full strength within 40 m + half its range of the camera and gone 25 m
/// further.
inline float LampFade(float distance, float range)
{
    const float start = 40.0f + 0.5f * std::max(range, 0.0f);
    return 1.0f - lamp_detail::Smooth(start, start + 25.0f, distance);
}

/// The factor of a flame at `seconds`: between 1 - amount and 1 + amount, never still (a slow breath, a quicker
/// waver and a fine tremble).
inline float LampFlicker(float amount, float phase, float seconds)
{
    const float a = std::clamp(amount, 0.0f, 0.4f);
    return 1.0f +
           a * (0.5f * std::sin(seconds * 7.3f + phase) + 0.3f * std::sin(seconds * 12.1f + phase * 1.7f + 1.3f) +
                0.2f * std::sin(seconds * 23.9f + phase * 2.3f + 0.7f));
}

/// Chooses at most kMaxLamps lamps of the moment `layer` (0 = A, 1 = B) from the fixed lamps of the town and the moving
/// ones (the lantern), by how much each lights the surroundings of the camera at `eye`; their colour is multiplied by
/// the fade, the flicker and `fixedScale` / `movingScale`. A scale of 0 or less turns that group off.
inline void SelectLamps(const std::vector<PointLight>& fixed, const std::vector<PointLight>& moving, int layer,
                        const float eye[3], float fixedScale, float movingScale, float seconds, LampBatch& out)
{
    struct Pick
    {
        float score;
        float fade;
        float scale;
        const PointLight* light;
    };
    static thread_local std::vector<Pick> picks; // WHY kept: no allocation in the frame
    picks.clear();
    out = LampBatch{};
    const unsigned bit = 1u << (layer == 0 ? 0 : 1);
    const auto take = [&](const std::vector<PointLight>& list, float scale)
    {
        if (!(scale > 0.0f))
        {
            return;
        }
        for (const PointLight& l : list)
        {
            if ((l.layers & bit) == 0u || !(l.range > 0.0f))
            {
                continue;
            }
            const float dx = l.pos[0] - eye[0];
            const float dy = l.pos[1] - eye[1];
            const float dz = l.pos[2] - eye[2];
            const float d2 = dx * dx + dy * dy + dz * dz;
            const float fade = LampFade(std::sqrt(d2), l.range);
            if (!(fade > 0.0f))
            {
                continue;
            }
            const float strength = std::max({l.color[0], l.color[1], l.color[2]}) * scale;
            const float score = strength * fade * l.range * l.range / (d2 + l.range * l.range);
            picks.push_back({score, fade, scale, &l});
        }
    };
    take(fixed, fixedScale);
    take(moving, movingScale);
    const std::size_t keep = std::min<std::size_t>(picks.size(), static_cast<std::size_t>(kMaxLamps));
    std::partial_sort(picks.begin(), picks.begin() + static_cast<std::ptrdiff_t>(keep), picks.end(),
                      [](const Pick& a, const Pick& b)
                      { return a.score != b.score ? a.score > b.score : a.light < b.light; });
    for (std::size_t i = 0; i < keep; ++i)
    {
        const Pick& p = picks[i];
        const float k = p.fade * p.scale * LampFlicker(p.light->flicker, p.light->phase, seconds);
        float* pos = out.pos[i];
        float* col = out.color[i];
        pos[0] = p.light->pos[0];
        pos[1] = p.light->pos[1];
        pos[2] = p.light->pos[2];
        pos[3] = p.light->range;
        col[0] = p.light->color[0] * k;
        col[1] = p.light->color[1] * k;
        col[2] = p.light->color[2] * k;
        col[3] = 0.0f;
    }
    out.count = static_cast<int>(keep);
}

} // namespace dl
