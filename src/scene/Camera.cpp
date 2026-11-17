#include "scene/Camera.h"

#include <cmath>

namespace dl
{
namespace
{
constexpr float kPi = 3.14159265f;
constexpr float kTwoPi = 6.28318531f;

/// Any angle -> the same direction inside [-pi, pi).
float WrapAngle(float radians)
{
    float a = std::fmod(radians + kPi, kTwoPi);
    if (a < 0.0f)
    {
        a += kTwoPi;
    }
    return a - kPi;
}

float ClampPitch(float radians)
{
    if (radians > Camera::kMaxPitch)
    {
        return Camera::kMaxPitch;
    }
    if (radians < -Camera::kMaxPitch)
    {
        return -Camera::kMaxPitch;
    }
    return radians;
}
} // namespace

void Camera::SetPose(const Vec3& position, float yawRadians, float pitchRadians)
{
    m_position = position;
    m_yaw = WrapAngle(yawRadians);
    m_pitch = ClampPitch(pitchRadians);
}

void Camera::Reset()
{
    m_position = Vec3{0.0f, 0.0f, -3.0f};
    m_yaw = 0.0f;
    m_pitch = 0.0f;
}

Vec3 Camera::Forward() const
{
    // yaw turns around the Y axis, pitch tilts up or down. At yaw = 0 and pitch = 0 this is (0, 0, 1): +Z.
    const float cosPitch = std::cos(m_pitch);
    return Vec3{std::sin(m_yaw) * cosPitch, std::sin(m_pitch), std::cos(m_yaw) * cosPitch};
}

Vec3 Camera::Right() const
{
    // Always horizontal, so "strafe right" never lifts the camera. At yaw = 0 this is (1, 0, 0): +X.
    return Vec3{std::cos(m_yaw), 0.0f, -std::sin(m_yaw)};
}

void Camera::Update(const CameraInput& input, float dt)
{
    if (!(dt > 0.0f)) // "not greater" also catches NaN
    {
        return;
    }
    if (dt > kMaxStep)
    {
        dt = kMaxStep;
    }

    // 1. turn
    m_yaw = WrapAngle(m_yaw + input.lookRight * m_settings.turnSpeed * dt);
    m_pitch = ClampPitch(m_pitch + input.lookUp * m_settings.turnSpeed * dt);

    // 2. move: add up the wanted directions; WHY normalise: W and D together must not be faster than W alone (1.41 times).
    const Vec3 forward = Forward();
    const Vec3 right = Right();
    Vec3 wish{forward.x * input.moveForward + right.x * input.moveRight, forward.y * input.moveForward + input.moveUp,
              forward.z * input.moveForward + right.z * input.moveRight};
    const float length = std::sqrt(wish.x * wish.x + wish.y * wish.y + wish.z * wish.z);
    if (length > 1.0f)
    {
        wish.x /= length;
        wish.y /= length;
        wish.z /= length;
    }
    const float step = m_settings.moveSpeed * dt; // UNITS: metres
    m_position.x += wish.x * step;
    m_position.y += wish.y * step;
    m_position.z += wish.z * step;
}
} // namespace dl
