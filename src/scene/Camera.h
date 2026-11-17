// Camera.h - a free-fly camera: a position and two angles. Plain maths, no DirectX, so it is tested without a window (tests/test_camera.cpp).
// Space:  left-handed, +Y up, +X right, +Z forward (docs/CONVENTIONS.md). yaw = 0 looks along +Z; a positive yaw turns toward +X (to the right).
// UNITS:  metres, radians, seconds.
// Owns:   its position and angles.   Threading: main thread only.
#pragma once

namespace dl
{
struct Vec3
{
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

/// What the player asks for in this frame. Every value is -1 .. +1 (0 = nothing).
struct CameraInput
{
    float moveForward = 0.0f; // W = +1, S = -1
    float moveRight = 0.0f;   // D = +1, A = -1
    float moveUp = 0.0f;      // E = +1, Q = -1
    float lookRight = 0.0f;   // right arrow = +1, left arrow = -1
    float lookUp = 0.0f;      // up arrow = +1, down arrow = -1
};

class Camera
{
public:
    struct Settings
    {
        float moveSpeed = 3.0f; // UNITS: m/s    TUNE: default 3, range [0.5, 20]
        float turnSpeed = 1.5f; // UNITS: rad/s  TUNE: default 1.5 (about 86 degrees per second)
    };

    // WHY 89 degrees and not 90: looking straight up, "forward" and "up" point the same way and the view matrix cannot be built.
    static constexpr float kMaxPitch = 1.5533f; // UNITS: radians (89 degrees)
    // WHY: after a pause in the debugger one frame can last many seconds; without a limit the camera would jump across the world.
    static constexpr float kMaxStep = 0.1f; // UNITS: seconds

    void SetSettings(const Settings& settings) { m_settings = settings; }

    /// Puts the camera somewhere. pitch is clamped to +-kMaxPitch, yaw is wrapped into [-pi, pi).
    void SetPose(const Vec3& position, float yawRadians, float pitchRadians);

    /// Back to the start: (0, 0, -3) looking at the origin.
    void Reset();

    /// Moves and turns the camera. dt <= 0 (or NaN) does nothing; dt is limited to kMaxStep.
    void Update(const CameraInput& input, float dt);

    Vec3 Position() const { return m_position; }
    float Yaw() const { return m_yaw; }
    float Pitch() const { return m_pitch; }
    Vec3 Forward() const; ///< unit length: where the camera looks
    Vec3 Right() const;   ///< unit length, horizontal: to the right of Forward()

private:
    Settings m_settings;
    Vec3 m_position{0.0f, 0.0f, -3.0f};
    float m_yaw = 0.0f;   // radians
    float m_pitch = 0.0f; // radians, positive = looking up
};
} // namespace dl
