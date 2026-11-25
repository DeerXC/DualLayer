// Collision.h - keeps a ball (the body of the player) out of the solid things of the scene, layer by layer.
// Plain maths, no Windows, no Direct3D: tested without a window (tests/test_collision.cpp).
// Rule:      only the objects that exist in the given layer are solid. The floor is in both layers, the red cube and
// the
//            grey cylinder only in A, the blue sphere and the yellow torus only in B (scene/SceneData.h).
// Table:     the objects come from a table. The short form of ResolveSphere uses the scene table of the program
//            (scene::kScene); the long form takes any table, so that a test can bring its own.
// Shapes:    cube = a box that can be stretched and turned; sphere; cylinder with flat ends; torus = a tube bent into a
//            ring. All of them are exact (no bounding boxes), so the ball touches what the player sees.
// Turning:   the shapes turn with time (spinSpeed * spinSeconds). Pass the same time that the renderer uses
//            (Models::SpinSeconds), or the solid and the picture drift apart.
// Method:    a ball outside the bounding sphere of a solid is skipped at once (BoundingRadius); otherwise the centre of
// the
//            ball is moved into the local space of the object (the inverse of the world matrix of the renderer), pushed
//            out along the shortest way, and moved back. Up to kPasses rounds, so that a ball between two solids
//            settles. No speed and no time: the camera moves, then this corrects the place.
// Space:     left-handed, +Y up. DirectXMath RotationX / RotationY on a row vector are written out here (a rotation of
//            +90 degrees round Y turns +Z into +X; round X it turns +Y into +Z), see tests/test_collision.cpp.
// UNITS:     metres, radians, seconds.
// Note:      no std::min / std::max here: windows.h may define them as macros (docs/CONVENTIONS.md).
#pragma once

#include "scene/Camera.h" // Vec3
#include "scene/SceneData.h"

#include <cmath>

namespace dl
{

struct CollisionResult
{
    Vec3 position;       ///< where the centre of the ball is now
    bool pushed = false; ///< true when the ball had to be moved
};

namespace collision_detail
{

// a ball that is pushed by less than this (metres) counts as not touching: it keeps a settled ball from "pushing" every
// frame
inline constexpr float kTouch = 1.0e-5f;
// WHY 16: in the narrow gap between two solids (the tilted torus touches the floor) each push undoes a part of the
// other; the ball settles after a few rounds. A real frame moves the ball a few centimetres, so it is far from this
// limit.
inline constexpr int kPasses = 16;

inline float Clamp(float v, float lo, float hi)
{
    return v < lo ? lo : (v > hi ? hi : v);
}

inline float Abs(float v)
{
    return v < 0.0f ? -v : v;
}

inline Vec3 Add(const Vec3& a, const Vec3& b)
{
    return {a.x + b.x, a.y + b.y, a.z + b.z};
}

inline Vec3 Sub(const Vec3& a, const Vec3& b)
{
    return {a.x - b.x, a.y - b.y, a.z - b.z};
}

/// DirectXMath XMMatrixRotationY(a) on a row vector: x' = x cos + z sin, z' = -x sin + z cos.
inline Vec3 RotateY(const Vec3& v, float a)
{
    const float c = std::cos(a);
    const float s = std::sin(a);
    return {v.x * c + v.z * s, v.y, -v.x * s + v.z * c};
}

/// DirectXMath XMMatrixRotationX(a) on a row vector: y' = y cos - z sin, z' = y sin + z cos.
inline Vec3 RotateX(const Vec3& v, float a)
{
    const float c = std::cos(a);
    const float s = std::sin(a);
    return {v.x, v.y * c - v.z * s, v.y * s + v.z * c};
}

/// The world matrix of the renderer is Scale * RotationX(tilt) * RotationY(spin + yaw) * Translation. So the way from
/// the local space of an object to the world is: tilt, then turn, then move; and back: move back, turn back, tilt back.
inline Vec3 ToLocal(const scene::SceneObject& o, const Vec3& world, float spinSeconds)
{
    const Vec3 moved = Sub(world, {o.position[0], o.position[1], o.position[2]});
    return RotateX(RotateY(moved, -(o.spinSpeed * spinSeconds + o.yaw)), -o.tilt);
}

inline Vec3 ToWorld(const scene::SceneObject& o, const Vec3& local, float spinSeconds)
{
    return Add(RotateY(RotateX(local, o.tilt), o.spinSpeed * spinSeconds + o.yaw),
               {o.position[0], o.position[1], o.position[2]});
}

// Each Push function works in the local space of the object (scale already in it). It moves `c` so that a ball of
// `radius` no longer overlaps the solid, and returns how deep the overlap was (0 = it did not touch).

/// A box with the half sizes h (the cube of 1 metre, stretched by the scale).
inline float PushOutOfBox(Vec3& c, const Vec3& h, float radius)
{
    const Vec3 closest{Clamp(c.x, -h.x, h.x), Clamp(c.y, -h.y, h.y), Clamp(c.z, -h.z, h.z)};
    const Vec3 d = Sub(c, closest);
    const float dist2 = d.x * d.x + d.y * d.y + d.z * d.z;
    if (dist2 > 1.0e-12f)
    {
        const float dist = std::sqrt(dist2);
        if (dist >= radius)
        {
            return 0.0f;
        }
        const float k = radius / dist;
        c = Add(closest, {d.x * k, d.y * k, d.z * k});
        return radius - dist;
    }
    // the centre is inside the box (or exactly on its surface): leave through the nearest face
    const float gap[3] = {h.x - Abs(c.x), h.y - Abs(c.y), h.z - Abs(c.z)};
    int axis = 0;
    if (gap[1] < gap[axis])
    {
        axis = 1;
    }
    if (gap[2] < gap[axis])
    {
        axis = 2;
    }
    float* value = axis == 0 ? &c.x : (axis == 1 ? &c.y : &c.z);
    const float half = axis == 0 ? h.x : (axis == 1 ? h.y : h.z);
    const float sign = *value < 0.0f ? -1.0f : 1.0f;
    *value = sign * (half + radius);
    return gap[axis] + radius;
}

/// A ball of radius R round the origin.
inline float PushOutOfSphere(Vec3& c, float R, float radius)
{
    const float len = std::sqrt(c.x * c.x + c.y * c.y + c.z * c.z);
    if (len >= R + radius)
    {
        return 0.0f;
    }
    if (len > 1.0e-6f)
    {
        const float k = (R + radius) / len;
        c = {c.x * k, c.y * k, c.z * k};
    }
    else
    {
        c = {0.0f, R + radius, 0.0f}; // exactly in the middle: up
    }
    return R + radius - len;
}

/// A cylinder with flat ends: radius R round the Y axis, from y = -H to y = +H.
inline float PushOutOfCylinder(Vec3& c, float R, float H, float radius)
{
    const float rho = std::sqrt(c.x * c.x + c.z * c.z);
    const float dirX = rho > 1.0e-6f ? c.x / rho : 1.0f; // the direction away from the axis (any, on the axis itself)
    const float dirZ = rho > 1.0e-6f ? c.z / rho : 0.0f;
    if (rho <= R && Abs(c.y) <= H)
    {
        // the centre is inside: leave through the side or through an end, whichever is nearer
        const float toSide = R - rho;
        const float toEnd = H - Abs(c.y);
        if (toSide < toEnd)
        {
            c.x = dirX * (R + radius);
            c.z = dirZ * (R + radius);
            return toSide + radius;
        }
        c.y = (c.y < 0.0f ? -1.0f : 1.0f) * (H + radius);
        return toEnd + radius;
    }
    const float nearRho = rho < R ? rho : R;
    const Vec3 closest{dirX * nearRho, Clamp(c.y, -H, H), dirZ * nearRho};
    const Vec3 d = Sub(c, closest);
    const float dist = std::sqrt(d.x * d.x + d.y * d.y + d.z * d.z);
    if (dist >= radius || dist <= 0.0f)
    {
        return 0.0f;
    }
    const float k = radius / dist;
    c = Add(closest, {d.x * k, d.y * k, d.z * k});
    return radius - dist;
}

/// A torus lying flat round the Y axis: the tube has the radius `tubeRadius` and its middle runs on a circle of radius
/// `ringRadius`.
inline float PushOutOfTorus(Vec3& c, float ringRadius, float tubeRadius, float radius)
{
    const float rho = std::sqrt(c.x * c.x + c.z * c.z);
    const float dirX = rho > 1.0e-6f ? c.x / rho : 1.0f;
    const float dirZ = rho > 1.0e-6f ? c.z / rho : 0.0f;
    const Vec3 core{dirX * ringRadius, 0.0f,
                    dirZ * ringRadius}; // the nearest point of the circle that the tube is bent round
    const Vec3 d = Sub(c, core);
    const float dist = std::sqrt(d.x * d.x + d.y * d.y + d.z * d.z);
    const float reach = tubeRadius + radius;
    if (dist >= reach)
    {
        return 0.0f;
    }
    if (dist > 1.0e-6f)
    {
        const float k = reach / dist;
        c = Add(core, {d.x * k, d.y * k, d.z * k});
    }
    else
    {
        c = Add(core, {0.0f, reach, 0.0f}); // exactly on the middle of the tube: up
    }
    return reach - dist;
}

/// The radius of a sphere round the centre of `o` that holds the whole object at every turn (the turn and the tilt are
/// round its centre). A ball farther than this plus its radius cannot touch it.
inline float BoundingRadius(const scene::SceneObject& o)
{
    const float hx = 0.5f * o.scale[0];
    const float hy = 0.5f * o.scale[1];
    const float hz = 0.5f * o.scale[2];
    switch (o.shape)
    {
    case scene::ShapeKind::Sphere:
        return hx;
    case scene::ShapeKind::Cylinder:
        return std::sqrt(hx * hx + hy * hy);
    case scene::ShapeKind::Torus:
        return (scene::kTorusMajor + scene::kTorusMinor) * o.scale[0];
    case scene::ShapeKind::Cube:
    case scene::ShapeKind::Count:
        break;
    }
    return std::sqrt(hx * hx + hy * hy + hz * hz);
}

/// Pushes the ball out of one object. Returns the depth of the overlap (0 = no contact) and changes `world`.
inline float PushOutOf(const scene::SceneObject& o, Vec3& world, float radius, float spinSeconds)
{
    // the broad phase: a ball outside the bounding sphere cannot touch the object (exact, so nothing changes but the
    // time; most of the boxes of a table are far from the body)
    const float dx = world.x - o.position[0];
    const float dy = world.y - o.position[1];
    const float dz = world.z - o.position[2];
    const float reach = BoundingRadius(o) + radius;
    if (dx * dx + dy * dy + dz * dz >= reach * reach)
    {
        return 0.0f;
    }
    Vec3 c = ToLocal(o, world, spinSeconds);
    float depth = 0.0f;
    switch (o.shape)
    {
    case scene::ShapeKind::Cube:
        depth = PushOutOfBox(c, {0.5f * o.scale[0], 0.5f * o.scale[1], 0.5f * o.scale[2]}, radius);
        break;
    case scene::ShapeKind::Sphere:
        depth = PushOutOfSphere(c, 0.5f * o.scale[0], radius);
        break;
    case scene::ShapeKind::Cylinder:
        depth = PushOutOfCylinder(c, 0.5f * o.scale[0], 0.5f * o.scale[1], radius);
        break;
    case scene::ShapeKind::Torus:
        depth = PushOutOfTorus(c, scene::kTorusMajor * o.scale[0], scene::kTorusMinor * o.scale[0], radius);
        break;
    case scene::ShapeKind::Count:
        break;
    }
    if (depth > 0.0f)
    {
        world = ToWorld(o, c, spinSeconds);
    }
    return depth;
}

} // namespace collision_detail

/// Moves a ball (the centre `center`, radius `radius`) out of every solid of `layer` (0 = A, 1 = B) in the table
/// `objects` (`objectCount` entries). spinSeconds: the time that the renderer uses for the turning shapes
/// (Models::SpinSeconds). A centre that is not a finite number is returned as it is (and not pushed): the caller has a
/// bigger problem. A null table or a count of 0 means nothing is solid.
inline CollisionResult ResolveSphere(const Vec3& center, float radius, int layer, float spinSeconds,
                                     const scene::SceneObject* objects, int objectCount)
{
    CollisionResult result;
    result.position = center;
    const bool finite = std::isfinite(center.x) && std::isfinite(center.y) && std::isfinite(center.z) &&
                        std::isfinite(spinSeconds) && std::isfinite(radius);
    if (!finite || radius <= 0.0f || (layer != 0 && layer != 1) || objects == nullptr || objectCount <= 0)
    {
        return result;
    }
    const unsigned layerBit = 1u << layer;
    for (int pass = 0; pass < collision_detail::kPasses; ++pass)
    {
        bool touched = false;
        for (int i = 0; i < objectCount; ++i)
        {
            const scene::SceneObject& object = objects[i];
            if ((object.layerMask & layerBit) == 0u)
            {
                continue; // not in this layer: not solid
            }
            if (collision_detail::PushOutOf(object, result.position, radius, spinSeconds) > collision_detail::kTouch)
            {
                touched = true;
                result.pushed = true;
            }
        }
        if (!touched)
        {
            break;
        }
    }
    return result;
}

/// The same, over the scene table of the program (scene/SceneData.h).
inline CollisionResult ResolveSphere(const Vec3& center, float radius, int layer, float spinSeconds)
{
    return ResolveSphere(center, radius, layer, spinSeconds, scene::kScene, scene::kSceneCount);
}

} // namespace dl
