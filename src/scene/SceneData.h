// SceneData.h - what the renderer draws one by one: a shape, where it stands, how it looks and in which moment it
// exists. Why here: the objects of the town loop (the lantern, the things that change, the avatar, the sea, the
// guidance) are made by
//            the scene code and drawn by render/Models.cpp; both only need this record. Plain data: no Windows, no
//            Direct3D.
// Space:     left-handed, +Y up (docs/CONVENTIONS.md). UNITS: metres, radians.
// Rules:     A shape is 1 metre big before the scale (a sphere: across). The world matrix of an object is
//            Scale * RotationX(tilt) * RotationY(yaw) * Translation (row vectors, read left to right).
#pragma once

namespace dl::scene
{

enum class ShapeKind
{
    Cube,
    Sphere,
    Cylinder,
    Torus,
    Count
};

// ---- the moments. A moment is a LABEL on an object, a bit of a mask: A = bit 0, B = bit 1, both = 3.
inline constexpr unsigned kLayerA = 1u << 0;
inline constexpr unsigned kLayerB = 1u << 1;
inline constexpr unsigned kLayerBoth = kLayerA | kLayerB;

// The torus lies flat in the XZ plane round the Y axis (render/MeshData.h). At scale 1:
inline constexpr float kTorusMajor = 0.8f; // UNITS: metres, from the centre of the ring to the middle of the tube
inline constexpr float kTorusMinor = 0.3f; // UNITS: metres, the radius of the tube

// Materials: `checker` also says how a surface is drawn (shaders/models.hlsl). 0 a plain surface, 1 the checkered
// floor; from 2 on, water, drawn after everything else and seen through (render/Models.cpp): its `glow` is then how
// opaque it is.
inline constexpr float kMatWater = 2.0f; ///< still water: pools, locks, the canal, the well (small waves)
inline constexpr float kMatFlow =
    3.0f; ///< running and falling water: the channel, spouts, waterfalls (streaks, white water)
inline constexpr float kMatSea = 4.0f;     ///< the sea (long swells, white crests)
inline constexpr float kMatWetBand = 5.0f; ///< the dark wet band left on walls when water goes down (its sides only)
inline constexpr float kMatFoam = 6.0f;    ///< foam: the rim of a surface, the foot of a fall, splash rings and drops
inline constexpr float kMatHalo =
    7.0f; ///< a soft glow round a lamp: a sphere added to the picture, its `glow` the strength (docs/DESIGN.md 6.3)

constexpr bool IsWaterMaterial(float checker)
{
    return checker > 1.5f && checker < 6.5f;
}

/// A halo is drawn after the water, added to what is behind it (no blending by alpha), not lit, and casts no shadow.
constexpr bool IsHalo(float checker)
{
    return checker > 6.5f;
}

struct SceneObject
{
    ShapeKind shape;
    float position[3]; // UNITS: metres, the centre of the object
    float scale[3];    // UNITS: metres: a shape is 1 metre big (a sphere: across), this stretches it
    float tilt;        // radians, round X, before the turn
    float colorSrgb[3];
    float checker; // 1 = the checkered floor; 2 .. 6 = water (kMatWater ..)
    float shineStrength;
    float shininess;
    unsigned layerMask = kLayerBoth; // which moments have this object (not written = both)
    float glow = 0.0f; // light of its own: 0 = none, 1 = its colour at full strength (a lamp, a hint, an orb)
    float yaw = 0.0f;  // radians, a fixed turn round Y after the tilt (a house turned to its street); 0 = axis-aligned
};

} // namespace dl::scene
