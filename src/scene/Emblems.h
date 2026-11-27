// Emblems.h - the three lamps of the signal mast over the bell tower: the far goal, the whole quest log of the game
//             (docs/DESIGN.md 10.1 and 10.5).
// Rule:      a lamp is dark until the bell of its district is rung (a thing of scene/Things.h, named in the town file);
//            then it is lit, in both moments. Three lamps, three shapes (a sphere for the well of the west, a column
//            for the cliff of the north, a box for the lock of the east): no words, no counter.
// Why a mast: the sea wall in front of the quay hides the bell tower itself in both moments
//            (tests/test_firstminute.cpp casts the rays), so the lamps hang on a mast that stands over the tower and
//            clears the wall: the player sees them from the first frame.
// Drawn:     each lamp and a pale collar round it, so that a dark lamp reads against the sky of B as well as that of A.
// Plain data and plain maths, no Windows, no Direct3D: tested without a window (tests/test_firstminute.cpp).
// UNITS: metres, seconds. Space: left-handed, +Y up.
#pragma once

#include "scene/SceneData.h"
#include "scene/Things.h"
#include "scene/Town.h"

#include <cmath>
#include <cstddef>

namespace dl::town
{

inline constexpr int kEmblemObjects = 2; ///< objects per emblem: the lamp and its collar

/// True when the bell of `e` has been rung.
inline bool EmblemLit(const TownData& town, const Things& things, const Emblem& e)
{
    for (std::size_t i = 0; i < town.things.size(); ++i)
    {
        if (town.things[i].name == e.bell)
        {
            return things.Open(i);
        }
    }
    return false;
}

/// How many of the lamps are lit.
inline int LitEmblems(const TownData& town, const Things& things)
{
    int n = 0;
    for (const Emblem& e : town.emblems)
    {
        n += EmblemLit(town, things, e) ? 1 : 0;
    }
    return n;
}

/// Adds the lamps and their collars, in both moments. Returns the new count; nothing for a table without room.
inline int AddEmblems(const TownData& town, const Things& things, scene::SceneObject* out, int count, int capacity,
                      float seconds)
{
    if (out == nullptr || count < 0 || capacity - count < kEmblemObjects * static_cast<int>(town.emblems.size()))
    {
        return count;
    }
    int index = 0;
    for (const Emblem& e : town.emblems)
    {
        const bool lit = EmblemLit(town, things, e);
        scene::SceneObject lamp{};
        lamp.position[0] = e.at.x;
        lamp.position[1] = e.at.y;
        lamp.position[2] = e.at.z;
        switch (e.shape)
        {
        case 0:
            lamp.shape = scene::ShapeKind::Sphere;
            lamp.scale[0] = lamp.scale[1] = lamp.scale[2] = e.size;
            break;
        case 1:
            lamp.shape = scene::ShapeKind::Cylinder;
            lamp.scale[0] = lamp.scale[2] = 0.75f * e.size;
            lamp.scale[1] = 1.3f * e.size;
            break;
        default:
            lamp.shape = scene::ShapeKind::Cube;
            lamp.scale[0] = lamp.scale[1] = lamp.scale[2] = 0.85f * e.size;
            break;
        }
        const float dark[3] = {0.40f, 0.43f,
                               0.50f}; // light enough to read against the dark sky of B, dark enough against that of A
        const float warm[3] = {1.0f, 0.78f, 0.38f};
        for (int c = 0; c < 3; ++c)
        {
            lamp.colorSrgb[c] = lit ? warm[c] : dark[c];
        }
        lamp.shineStrength = lit ? 0.0f : 0.25f;
        lamp.shininess = 24.0f;
        lamp.glow = lit ? 1.4f + 0.25f * std::sin(seconds * 2.2f + 2.1f * static_cast<float>(index)) : 0.0f;
        lamp.layerMask = scene::kLayerBoth;
        out[count++] = lamp;
        scene::SceneObject collar{};
        collar.shape = scene::ShapeKind::Torus;
        collar.position[0] = e.at.x;
        collar.position[1] = e.at.y;
        collar.position[2] = e.at.z;
        collar.scale[0] = collar.scale[2] = 0.72f * e.size;
        collar.scale[1] = 0.12f * e.size;
        collar.colorSrgb[0] = lit ? 1.0f : 0.66f;
        collar.colorSrgb[1] = lit ? 0.86f : 0.72f;
        collar.colorSrgb[2] = lit ? 0.56f : 0.82f;
        collar.glow = lit ? 1.0f : 0.10f;
        collar.layerMask = scene::kLayerBoth;
        out[count++] = collar;
        ++index;
    }
    return count;
}

} // namespace dl::town
