// SceneData.h - what is in the scene: the floor and four shapes, where they stand, and in which layer they exist.
// Why here: the renderer (render/Models.cpp) and the collision (scene/Collision.h) read the SAME table, so what is drawn
//            and what is solid can never disagree. Plain data: no Windows, no Direct3D (tested without a window).
// Space:     left-handed, +Y up (docs/CONVENTIONS.md). UNITS: metres, radians, seconds.
// Rules:     A shape is 1 metre big before the scale (a sphere: across). The world matrix of an object is
//            Scale * RotationX(tilt) * RotationY(spinSpeed * time) * Translation (row vectors, read left to right).
//            The collision relies on the shapes being round where the table says so: SceneIsSupported() checks it when
//            the program is compiled, so a wrong edit of the table is an error and not a bug.
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

// ---- the layers. A layer is a LABEL on an object, a bit of a mask: A = bit 0, B = bit 1, both = 3.
inline constexpr unsigned kLayerA = 1u << 0;
inline constexpr unsigned kLayerB = 1u << 1;
inline constexpr unsigned kLayerBoth = kLayerA | kLayerB;

// ---- the placement
// The floor is a cube squashed flat: its TOP is at y = kFloorY. Every object stands on it.
inline constexpr float kFloorY = -1.0f;
// The camera starts at (0, 0, -3) looking along +Z (scene/Camera.h): the row of shapes is 6 metres in front of it.
inline constexpr float kRowZ = 3.0f;
// The torus lies flat in the XZ plane round the Y axis (render/MeshData.h). At scale 1:
inline constexpr float kTorusMajor = 0.8f; // UNITS: metres, from the centre of the ring to the middle of the tube
inline constexpr float kTorusMinor = 0.3f; // UNITS: metres, the radius of the tube

struct SceneObject
{
    ShapeKind shape;
    float position[3]; // UNITS: metres, the centre of the object
    float scale[3];    // UNITS: metres: a shape is 1 metre big (a sphere: across), this stretches it
    float tilt;        // radians, round X, before the turn
    float spinSpeed;   // radians per second, round Y (negative: the other way)
    float colorSrgb[3];
    float checker; // 1 = the checkered floor
    float shineStrength;
    float shininess;
    unsigned layerMask = kLayerBoth; // which layers have this object (not written = both)
    float glow = 0.0f;               // light of its own: 0 = none, 1 = its colour at full strength (a lamp, a hint, an orb)
};

// THE SCENE. Edit it to move or to change something. Every object stands on the floor: y = kFloorY + half of its height
// after the scale (the torus: half of its tilted extent, 0.8 * sin(tilt) + 0.3).
// The members must be written in the order of the struct (C++20 designated initialisers): layerMask comes last.
inline constexpr SceneObject kScene[] = {
    // the floor: a cube of 24 x 0.2 x 24 metres, its top at y = -1; it is in both layers
    {.shape = ShapeKind::Cube,
     .position = {0.0f, kFloorY - 0.1f, kRowZ},
     .scale = {24.0f, 0.2f, 24.0f},
     .tilt = 0.0f,
     .spinSpeed = 0.0f,
     .colorSrgb = {0.30f, 0.33f, 0.40f},
     .checker = 1.0f,
     .shineStrength = 0.04f,
     .shininess = 16.0f,
     .layerMask = kLayerBoth},
    // the cube: 1.6 metres, red, layer A
    {.shape = ShapeKind::Cube,
     .position = {-3.3f, kFloorY + 0.8f, kRowZ},
     .scale = {1.6f, 1.6f, 1.6f},
     .tilt = 0.0f,
     .spinSpeed = 0.55f,
     .colorSrgb = {0.86f, 0.24f, 0.22f},
     .checker = 0.0f,
     .shineStrength = 0.18f,
     .shininess = 32.0f,
     .layerMask = kLayerA},
    // the sphere: 1.8 metres across, blue, layer B
    {.shape = ShapeKind::Sphere,
     .position = {-1.1f, kFloorY + 0.9f, kRowZ},
     .scale = {1.8f, 1.8f, 1.8f},
     .tilt = 0.0f,
     .spinSpeed = 0.80f,
     .colorSrgb = {0.20f, 0.46f, 0.92f},
     .checker = 0.0f,
     .shineStrength = 0.45f,
     .shininess = 64.0f,
     .layerMask = kLayerB},
    // the cylinder: 1.3 metres across, 1.8 high, light grey, layer A
    {.shape = ShapeKind::Cylinder,
     .position = {1.1f, kFloorY + 0.9f, kRowZ},
     .scale = {1.3f, 1.8f, 1.3f},
     .tilt = 0.0f,
     .spinSpeed = -0.65f,
     .colorSrgb = {0.82f, 0.83f, 0.86f},
     .checker = 0.0f,
     .shineStrength = 0.30f,
     .shininess = 48.0f,
     .layerMask = kLayerA},
    // the torus: a ring of 0.8 metres (to the middle of the tube) and a tube of 0.3, tilted so that the hole shows; layer B
    {.shape = ShapeKind::Torus,
     .position = {3.3f, kFloorY + 0.75f, kRowZ},
     .scale = {1.0f, 1.0f, 1.0f},
     .tilt = 0.6f,
     .spinSpeed = 0.90f,
     .colorSrgb = {0.96f, 0.76f, 0.24f},
     .checker = 0.0f,
     .shineStrength = 0.55f,
     .shininess = 80.0f,
     .layerMask = kLayerB},
};
inline constexpr int kSceneCount = static_cast<int>(sizeof(kScene) / sizeof(kScene[0]));

/// What the collision can handle: a sphere is scaled the same on all three axes, a cylinder is round (x = z), a torus
/// is scaled the same on all three axes; a cube may be stretched freely. Checked below, when the program is compiled.
constexpr bool SceneIsSupported()
{
    for (const SceneObject& o : kScene)
    {
        const bool isUniform = o.scale[0] == o.scale[1] && o.scale[1] == o.scale[2];
        const bool isRound = o.scale[0] == o.scale[2];
        if ((o.shape == ShapeKind::Sphere || o.shape == ShapeKind::Torus) && !isUniform)
        {
            return false;
        }
        if (o.shape == ShapeKind::Cylinder && !isRound)
        {
            return false;
        }
        if (!(o.scale[0] > 0.0f && o.scale[1] > 0.0f && o.scale[2] > 0.0f))
        {
            return false;
        }
        if (o.layerMask == 0u || (o.layerMask & ~kLayerBoth) != 0u)
        {
            return false;
        }
    }
    return true;
}
static_assert(SceneIsSupported(), "scene/SceneData.h: a sphere / torus must be scaled evenly, a cylinder must be round, "
                                  "every scale must be above 0, and every layer mask must be A, B or both");

} // namespace dl::scene

