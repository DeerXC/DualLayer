// test_camera.cpp - tests of the camera maths (no window, no GPU).
//   g++ -std=c++20 -Wall -Wextra -I src tests/test_camera.cpp src/scene/Camera.cpp -o test_camera
//   MSVC (Developer PowerShell):  .\tests\run_tests.ps1
#include "check.h"
#include "scene/Camera.h"

#include <cmath>
#include <cstdio>
#include <initializer_list>
#include <limits>

namespace
{

bool Near(float a, float b, float eps = 1e-3f)
{
    return std::fabs(a - b) <= eps;
}

float Length(const dl::Vec3& v)
{
    return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

/// Runs the camera for `seconds` with the same input every frame (frames of 0.05 s).
void Run(dl::Camera& camera, const dl::CameraInput& input, float seconds)
{
    const int frames = static_cast<int>(seconds / 0.05f + 0.5f);
    for (int i = 0; i < frames; ++i)
    {
        camera.Update(input, 0.05f);
    }
}

void TestStart()
{
    dl::Camera c;
    CHECK(Near(c.Position().x, 0.0f) && Near(c.Position().y, 0.0f) && Near(c.Position().z, -3.0f));
    CHECK(Near(c.Forward().x, 0.0f) && Near(c.Forward().y, 0.0f) && Near(c.Forward().z, 1.0f)); // looks along +Z
    CHECK(Near(c.Right().x, 1.0f) && Near(c.Right().y, 0.0f) && Near(c.Right().z, 0.0f));       // right is +X
}

void TestMove()
{
    dl::Camera c;
    dl::CameraInput in;
    in.moveForward = 1.0f;
    Run(c, in, 1.0f); // 3 m/s for 1 s: from z = -3 to z = 0
    CHECK(Near(c.Position().z, 0.0f) && Near(c.Position().x, 0.0f));

    c.Reset();
    in = dl::CameraInput{};
    in.moveRight = 1.0f;
    Run(c, in, 1.0f);
    CHECK(Near(c.Position().x, 3.0f) && Near(c.Position().z, -3.0f));

    c.Reset();
    in = dl::CameraInput{};
    in.moveUp = -1.0f;
    Run(c, in, 1.0f);
    CHECK(Near(c.Position().y, -3.0f));

    // Space and E both lift (the same input, +1), Shift asks for a boost: 4 times the speed, never more than kMaxBoost, never less than 1,
    // and a bad number is no boost
    c.Reset();
    in = dl::CameraInput{};
    in.moveUp = 1.0f;
    in.boost = 4.0f;
    Run(c, in, 1.0f);
    CHECK(Near(c.Position().y, 12.0f));
    for (const float b : {0.2f, 0.0f, -3.0f, std::nanf(""), 1.0e9f})
    {
        c.Reset();
        in = dl::CameraInput{};
        in.moveForward = 1.0f;
        in.boost = b;
        Run(c, in, 1.0f);
        const float expect = std::isfinite(b) ? std::fmax(1.0f, std::fmin(b, dl::Camera::kMaxBoost)) : 1.0f;
        CHECK(Near(c.Position().z, -3.0f + 3.0f * expect));
    }
    in = dl::CameraInput{};
    in.moveForward = 1.0f;
    in.moveUp = 1.0f;
    in.boost = 4.0f;
    c.Reset();
    Run(c, in, 1.0f); // diagonal and boosted: normalised first, then boosted
    CHECK(Near(std::sqrt(c.Position().y * c.Position().y + (c.Position().z + 3.0f) * (c.Position().z + 3.0f)), 12.0f));

    // two keys at once are not faster than one
    c.Reset();
    in = dl::CameraInput{};
    in.moveForward = 1.0f;
    in.moveRight = 1.0f;
    Run(c, in, 1.0f);
    const dl::Vec3 p = c.Position();
    CHECK(Near(std::sqrt(p.x * p.x + (p.z + 3.0f) * (p.z + 3.0f)), 3.0f));

    // opposite keys cancel (the keyboard code gives forward = +1 - 1 = 0)
    c.Reset();
    in = dl::CameraInput{};
    Run(c, in, 1.0f);
    CHECK(Near(c.Position().z, -3.0f));

    // the speed is a setting
    c.Reset();
    c.SetSettings(dl::Camera::Settings{6.0f, 1.5f});
    in = dl::CameraInput{};
    in.moveRight = 1.0f;
    Run(c, in, 1.0f);
    CHECK(Near(c.Position().x, 6.0f));
}

void TestTurn()
{
    dl::Camera c;
    dl::CameraInput in;
    in.lookRight = 1.0f;
    Run(c, in, 1.0f); // 1.5 rad/s for 1 s
    CHECK(Near(c.Yaw(), 1.5f));
    CHECK(c.Forward().x > 0.9f); // turned toward +X: to the right

    // facing +X: forward is +X and "right" is -Z (left-handed, Y up)
    c.SetPose(dl::Vec3{0, 0, 0}, 1.5707963f, 0.0f);
    CHECK(Near(c.Forward().x, 1.0f) && Near(c.Forward().z, 0.0f));
    CHECK(Near(c.Right().x, 0.0f) && Near(c.Right().z, -1.0f));
    in = dl::CameraInput{};
    in.moveForward = 1.0f;
    Run(c, in, 1.0f);
    CHECK(Near(c.Position().x, 3.0f) && Near(c.Position().z, 0.0f)); // W walks along the new forward
}

void TestPitch()
{
    dl::Camera c;
    dl::CameraInput in;
    in.lookUp = 1.0f;
    Run(c, in, 10.0f);                                    // much longer than needed
    CHECK(Near(c.Pitch(), dl::Camera::kMaxPitch, 1e-5f)); // stops at 89 degrees
    CHECK(c.Forward().y < 1.0f && Near(Length(c.Forward()), 1.0f));
    CHECK(Near(Length(c.Right()), 1.0f));
    in.lookUp = -1.0f;
    Run(c, in, 20.0f);
    CHECK(Near(c.Pitch(), -dl::Camera::kMaxPitch, 1e-5f));
}

void TestPoseAndSafety()
{
    dl::Camera c;
    c.SetPose(dl::Vec3{1, 2, 3}, 7.0f, 5.0f);
    CHECK(Near(c.Yaw(), 7.0f - 6.2831853f)); // 7 rad wrapped into [-pi, pi)
    CHECK(Near(c.Pitch(), dl::Camera::kMaxPitch, 1e-5f));
    c.SetPose(dl::Vec3{}, -4.0f, 0.0f);
    CHECK(Near(c.Yaw(), -4.0f + 6.2831853f));

    // a huge frame time is limited to 0.1 s: 3 m/s * 0.1 s = 0.3 m
    c.Reset();
    dl::CameraInput in;
    in.moveForward = 1.0f;
    c.Update(in, 10.0f);
    CHECK(Near(c.Position().z, -3.0f + 0.3f));

    // zero, negative and NaN frame times do nothing
    c.Reset();
    c.Update(in, 0.0f);
    c.Update(in, -1.0f);
    c.Update(in, std::numeric_limits<float>::quiet_NaN());
    CHECK(Near(c.Position().z, -3.0f));

    c.SetPose(dl::Vec3{5, 5, 5}, 1.0f, 1.0f);
    c.Reset();
    CHECK(Near(c.Position().z, -3.0f) && Near(c.Yaw(), 0.0f) && Near(c.Pitch(), 0.0f));
}
} // namespace

int main()
{
    TestStart();
    TestMove();
    TestTurn();
    TestPitch();
    TestPoseAndSafety();
    std::printf("%d checks passed, %d failed\n", g_passed, g_failed);
    return g_failed == 0 ? 0 : 1;
}
