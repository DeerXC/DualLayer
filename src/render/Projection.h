// Projection.h - the one projection matrix of the program, so that every pass sees the world with the same lens.
// Space:   left-handed (docs/CONVENTIONS.md), a perspective with the vertical field of view kFovY.
// UNITS:   radians, metres. The near plane must stay well above 0: depth precision is spent mostly close to it.
// Depth:   0 = the near plane, 1 = the far plane (the default Direct3D range); Device::BeginFrame clears to 1.
#pragma once

#include <DirectXMath.h>

namespace dl
{
constexpr float kFovY = DirectX::XM_PIDIV4; // UNITS: radians (45 degrees, vertical)
constexpr float kNearPlane = 0.1f;          // UNITS: metres
constexpr float kFarPlane = 100.0f;         // UNITS: metres

inline DirectX::XMMATRIX MakeProjection(float aspect)
{
    return DirectX::XMMatrixPerspectiveFovLH(kFovY, aspect, kNearPlane, kFarPlane);
}
} // namespace dl
