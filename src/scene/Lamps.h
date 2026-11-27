// Lamps.h - the lights of the town as the renderer wants them (render/Lights.h): the lamps of the streets and the
// harbour that the model writes into the town file, and the ones that change while the game is played: the lantern of
// the traveller (in the hand or set down), the lamps of the signal mast that a ringing of a bell has lit, and the lamp
// over the rope of a bell that was rung.
// Fixed:    TownLamps once; the renderer keeps them (Models::SetStaticLamps) and every frame chooses the nearest ones.
// Moving:   MovingLamps every frame (Models::SetMovingLamps).
// Halos:    AddHalos gives a soft glowing sphere (scene::kMatHalo) for every lamp near the camera, to be drawn over the
//           picture: a lit lamp is a light in the air as well as on the walls. A halo is weaker in A (the sun hides a
//           lamp) than in B.
// Colour:   the model gives sRGB, the renderer works in linear light: a lamp's colour is converted here and multiplied
//           by its strength.
// Plain data and plain maths, no Windows, no Direct3D: tested without a window (tests/test_lighting.cpp).
// UNITS:    metres, seconds.
#pragma once

#include "render/Lights.h"
#include "render/StaticBatch.h"
#include "scene/Camera.h" // Vec3
#include "scene/Emblems.h"
#include "scene/Lantern.h"
#include "scene/SceneData.h"
#include "scene/Things.h"
#include "scene/Town.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace dl::town
{

inline constexpr float kLanternLightRange = 13.0f; ///< UNITS: metres the lantern of the traveller lights
inline constexpr float kLanternLightStrength =
    10.0f; ///< its strength (a street lamp is 18, but 3.4 m up: the lantern is 1.2 m)
inline constexpr float kLanternFlicker =
    0.12f; ///< the part of its strength that the flame of the lantern gains and loses
inline constexpr float kLanternHalo =
    1.0f; ///< UNITS: metres across: the glow round the glass (smaller than a street lamp's:
          ///< it is carried through narrow streets and must not cut into the walls)
inline constexpr float kEmblemLightRange = 22.0f; ///< a lit lamp of the mast lights the tower under it
inline constexpr float kEmblemLightStrength = 36.0f;
inline constexpr float kBellLightRange = 12.0f; ///< the lamp over the rope of a bell that was rung (B)
inline constexpr float kBellLightStrength = 12.0f;
inline constexpr float kHaloReach = 80.0f; ///< UNITS: metres: a lamp farther than this from the camera has no halo
inline constexpr int kMaxHalos = 36;       ///< lamps that get halos in a frame (each in the moments it shines in)

/// The size of the halo of a kind of lamp (metres across; 0: none): small round a lantern in the hand, wide round a
/// lighthouse.
inline float HaloOfKind(LampKind kind)
{
    switch (kind)
    {
    case LampKind::Street:
        return 2.4f;
    case LampKind::Post:
        return 2.6f;
    case LampKind::Light:
        return 7.0f;
    case LampKind::Memory:
        return 1.8f;
    case LampKind::Grave:
        return 1.8f;
    }
    return 0.0f;
}

namespace lamps_detail
{

inline float Linear(float srgb)
{
    return batch_detail::SrgbToLinear(srgb);
}

inline float ToSrgb(float linear)
{
    const float c = std::clamp(linear, 0.0f, 1.0f);
    return c <= 0.0031308f ? c * 12.92f : 1.055f * std::pow(c, 1.0f / 2.4f) - 0.055f;
}

/// A light of the colour `srgb` (3 values) and the strength `strength`.
inline PointLight Make(const Vec3& at, float range, const float srgb[3], float strength, float flicker, float phase,
                       unsigned layers, float halo)
{
    PointLight l;
    l.pos[0] = at.x;
    l.pos[1] = at.y;
    l.pos[2] = at.z;
    l.range = range;
    for (int i = 0; i < 3; ++i)
    {
        l.color[i] = Linear(srgb[i]) * strength;
    }
    l.layers = layers;
    l.flicker = flicker;
    l.phase = phase;
    l.halo = halo;
    return l;
}

} // namespace lamps_detail

/// The lamps of the town file as point lights. A phase of the flicker of its own for each, from where it stands.
inline std::vector<PointLight> TownLamps(const TownData& town)
{
    std::vector<PointLight> out;
    out.reserve(town.lamps.size());
    for (const Lamp& lamp : town.lamps)
    {
        const float phase = 6.2831853f * std::fmod(std::fabs(lamp.at.x * 12.9898f + lamp.at.z * 78.233f), 1.0f);
        out.push_back(lamps_detail::Make(lamp.at, lamp.range, lamp.color, lamp.intensity, lamp.flicker, phase,
                                         lamp.worlds, HaloOfKind(lamp.kind)));
    }
    return out;
}

/// The lamps that change, into `out` (cleared first): the lantern (in the hand at `hand` when it is carried, else where
/// it stands), a lit lamp of the mast for each bell rung, and the lamp over the rope of each bell rung (in B).
inline void MovingLamps(const TownData& town, const Things& things, const Lantern& lantern, const Vec3& hand,
                        std::vector<PointLight>& out)
{
    out.clear();
    const float warm[3] = {1.0f, 0.84f, 0.58f};
    const Vec3 at =
        lantern.Carried() ? hand : Vec3{lantern.At().x, lantern.At().y + kLanternUp + 0.04f, lantern.At().z};
    // the flame of a lantern is a deeper amber than the white-gold of a lit lamp of the mast
    const float flame[3] = {1.0f, 0.76f, 0.44f};
    out.push_back(lamps_detail::Make(at, kLanternLightRange, flame, kLanternLightStrength, kLanternFlicker, 0.7f, 3u,
                                     kLanternHalo));
    for (const Emblem& e : town.emblems)
    {
        if (EmblemLit(town, things, e))
        {
            out.push_back(
                lamps_detail::Make(e.at, kEmblemLightRange, warm, kEmblemLightStrength, 0.04f, e.at.x, 3u, 6.0f));
        }
    }
    for (std::size_t i = 0; i < town.things.size(); ++i)
    {
        if (town.things[i].verb != Verb::Ring || !things.Open(i))
        {
            continue;
        }
        for (const ThingBox& b : town.things[i].boxes)
        {
            if (b.state == 1 && town.materials[b.material].name == "lantern")
            {
                out.push_back(lamps_detail::Make(b.centre, kBellLightRange, warm, kBellLightStrength, 0.08f, b.centre.x,
                                                 b.worlds, 2.0f));
            }
        }
    }
}

/// Adds a halo (a sphere of kMatHalo, its glow the strength) for each of the nearest lamps within kHaloReach of `eye`,
/// once for every moment it shines in: A with `strengthA`, B with `strengthB` (0 or less: none in that moment). Returns
/// the new count.
inline int AddHalos(const std::vector<PointLight>& fixed, const std::vector<PointLight>& moving, const float eye[3],
                    float strengthA, float strengthB, float seconds, scene::SceneObject* out, int count, int capacity)
{
    if (out == nullptr || count < 0)
    {
        return count;
    }
    struct Pick
    {
        float d2;
        const PointLight* light;
    };
    static thread_local std::vector<Pick> picks;
    picks.clear();
    for (const std::vector<PointLight>* list : {&fixed, &moving})
    {
        for (const PointLight& l : *list)
        {
            const float dx = l.pos[0] - eye[0];
            const float dy = l.pos[1] - eye[1];
            const float dz = l.pos[2] - eye[2];
            const float d2 = dx * dx + dy * dy + dz * dz;
            if (l.halo > 0.0f && d2 < kHaloReach * kHaloReach)
            {
                picks.push_back({d2, &l});
            }
        }
    }
    const std::size_t keep = std::min<std::size_t>(picks.size(), static_cast<std::size_t>(kMaxHalos));
    std::partial_sort(picks.begin(), picks.begin() + static_cast<std::ptrdiff_t>(keep), picks.end(),
                      [](const Pick& a, const Pick& b) { return a.d2 != b.d2 ? a.d2 < b.d2 : a.light < b.light; });
    for (std::size_t i = 0; i < keep; ++i)
    {
        const PointLight& l = *picks[i].light;
        const float peak = std::max({l.color[0], l.color[1], l.color[2], 1.0e-4f});
        for (int layer = 0; layer < 2; ++layer)
        {
            const float strength = layer == 0 ? strengthA : strengthB;
            if ((l.layers & (1u << layer)) == 0u || !(strength > 0.0f) || count >= capacity)
            {
                continue;
            }
            scene::SceneObject o{};
            o.shape = scene::ShapeKind::Sphere;
            o.position[0] = l.pos[0];
            o.position[1] = l.pos[1];
            o.position[2] = l.pos[2];
            o.scale[0] = o.scale[1] = o.scale[2] = l.halo;
            for (int c = 0; c < 3; ++c)
            {
                o.colorSrgb[c] = lamps_detail::ToSrgb(l.color[c] / peak);
            }
            o.checker = scene::kMatHalo;
            o.glow = 0.5f * strength *
                     LampFlicker(l.flicker, l.phase, seconds); // half: the two faces of the sphere are added
            o.layerMask = layer == 0 ? scene::kLayerA : scene::kLayerB;
            out[count++] = o;
        }
    }
    return count;
}

} // namespace dl::town
