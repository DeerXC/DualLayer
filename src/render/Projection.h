// Projection.h - the one projection matrix of the program, so that every pass sees the world with the same lens.
// Space:   left-handed (docs/CONVENTIONS.md), a perspective with a vertical field of view.
// FOV:     70 degrees vertical by default (about 102 degrees across a 16:9 screen); [camera] fov_deg in the config
//          changes it, clamped to kMinFovYDegrees .. kMaxFovYDegrees. The first value, 45 degrees, felt like looking
//          through a tube.
// UNITS:   degrees in the config and at the call, radians inside; metres. The near plane must stay well above 0: depth
//          precision is spent mostly close to it.
// Depth:   0 = the near plane, 1 = the far plane (the default Direct3D range); Device::BeginFrame clears to 1. The far
//          plane holds the whole map and the beacon from any place in it.
#pragma once

#include <DirectXMath.h>

namespace dl
{

constexpr float kDefaultFovYDegrees = 70.0f; // UNITS: degrees, vertical. TUNE: [camera] fov_deg
constexpr float kMinFovYDegrees = 40.0f;
constexpr float kMaxFovYDegrees = 100.0f;
constexpr float kNearPlane = 0.1f;  // UNITS: metres
constexpr float kFarPlane = 300.0f; // UNITS: metres

/// The projection for a screen of width / height `aspect`, with a vertical field of view of `fovYDegrees` (clamped; a
/// NaN gets the default).
inline DirectX::XMMATRIX MakeProjection(float aspect, float fovYDegrees = kDefaultFovYDegrees)
{
    float fov = fovYDegrees == fovYDegrees ? fovYDegrees : kDefaultFovYDegrees; // (a NaN is not equal to itself)
    fov = fov < kMinFovYDegrees ? kMinFovYDegrees : (fov > kMaxFovYDegrees ? kMaxFovYDegrees : fov);
    return DirectX::XMMatrixPerspectiveFovLH(DirectX::XMConvertToRadians(fov), aspect, kNearPlane, kFarPlane);
}

} // namespace dl
