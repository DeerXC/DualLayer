// test_lights.cpp - tests of the hidden light (no window, no GPU): src/scene/StageLights.h on the stage of src/scene/Stage.h.
//   g++ -std=c++20 -Wall -Wextra -I src tests/test_lights.cpp -o test_lights
//   MSVC (Developer PowerShell):  .\tests\run_tests.ps1
// Header-only: there is no .cpp to add to the project. What is checked:
//   1. the logic: a light is taken when a ball of the body touches it, once; it stays taken; a taken light is not drawn any
//      more; the goal light changes colour only when asked, and only the goal light
//   2. where it is, with the numbers of the walking body (jump 2.0 m, eye 1.55 m, ball radius 0.35 m, 4 m/s, gravity 30 up
//      and 48 down): the light in the gate cannot be touched from any side of the gate in A, is reached by a jump in B, and
//      is not touched by jumps along the main path
// The flights are computed with the rules of Player.h (a held jump, the walking speed at once); the full body is played in
// tests/test_stage.cpp. Returns 1 when a check fails.

#include "scene/StageLights.h"

#include <cmath>
#include <cstdio>

namespace
{

int g_failed = 0;
int g_passed = 0;

#define CHECK(cond)                                                                                                    \
    do                                                                                                                 \
    {                                                                                                                  \
        if (cond)                                                                                                      \
        {                                                                                                              \
            ++g_passed;                                                                                                \
        }                                                                                                              \
        else                                                                                                           \
        {                                                                                                              \
            ++g_failed;                                                                                                \
            std::printf("FAIL line %d: %s\n", __LINE__, #cond);                                                        \
        }                                                                                                              \
    } while (0)

using dl::Vec3;
using namespace dl::scene;
namespace st = dl::stage;

constexpr float kRadius = 0.35f;
constexpr float kEye = 1.55f;
constexpr float kWalkSpeed = 4.0f;
constexpr float kGravityUp = 30.0f;
constexpr float kGravityDown = 30.0f * 1.6f;
constexpr float kJumpHeight = 2.0f;

/// A body as the lights see it: three balls above the feet, like Player.
struct FakeBody
{
    Vec3 feet;
    struct Settings
    {
        float radius = kRadius;
    } settings;
    const Settings& GetSettings() const
    {
        return settings;
    }
    Vec3 Ball(int i) const
    {
        const float t = static_cast<float>(i) / 2.0f;
        return {feet.x, feet.y + kRadius + (kEye - kRadius) * t, feet.z};
    }
};

bool Touches(const Vec3& feet)
{
    FakeBody body{feet, {}};
    st::LightSet set;
    return set.Collect(body) == 0;
}

/// The flight of a held jump from `start` (feet), moving along (dirX, dirZ) at the walking speed, until the feet are 6 m
/// below the start. True when a ball touches the light on the way.
bool JumpReaches(const Vec3& start, float dirX, float dirZ)
{
    const float jumpSpeed = std::sqrt(2.0f * kGravityUp * kJumpHeight);
    Vec3 feet = start;
    float vy = jumpSpeed;
    const float dt = 1.0f / 600.0f;
    for (int i = 0; i < 6000 && feet.y > start.y - 6.0f; ++i)
    {
        feet.x += dirX * kWalkSpeed * dt;
        feet.z += dirZ * kWalkSpeed * dt;
        feet.y += vy * dt;
        vy -= (vy > 0.0f ? kGravityUp : kGravityDown) * dt;
        if (Touches(feet))
        {
            return true;
        }
    }
    return false;
}

void TestLogic()
{
    st::LightSet set;
    CHECK(set.Count() == 0 && !set.All() && st::kHiddenLightCount == 1);
    FakeBody far{{0.0f, st::kTerraceY, -3.0f}, {}};
    CHECK(set.Collect(far) == -1);
    SceneObject table[4] = {};
    CHECK(set.AddToDrawTable(table, 0, 4) == 1 && table[0].shape == ShapeKind::Sphere && table[0].layerMask == kLayerBoth);
    CHECK(set.AddToDrawTable(table, 4, 4) == 4 && set.AddToDrawTable(nullptr, 3, 4) == 3);
    const Vec3 c = st::kHiddenLights[0].centre;
    FakeBody at{{c.x, c.y - kEye, c.z}, {}};
    CHECK(set.Collect(at) == 0 && set.Taken(0) && set.All());
    CHECK(set.Collect(at) == -1 && set.Count() == 1); // once only
    CHECK(set.AddToDrawTable(table, 0, 4) == 0);      // nothing left to draw
    // the goal light changes colour; nothing else does
    static SceneObject drawn[st::kMaxDrawObjects] = {};
    const dl::puzzle::Puzzle puzzle;
    const int n = st::BuildDrawTable(puzzle, false, drawn);
    static SceneObject before[st::kMaxDrawObjects] = {};
    for (int i = 0; i < n; ++i)
    {
        before[i] = drawn[i];
    }
    CHECK(st::RecolourGoal(drawn, n));
    int changed = 0;
    for (int i = 0; i < n; ++i)
    {
        if (drawn[i].colorSrgb[0] != before[i].colorSrgb[0] || drawn[i].colorSrgb[2] != before[i].colorSrgb[2])
        {
            ++changed;
            CHECK(drawn[i].shape == ShapeKind::Sphere && drawn[i].colorSrgb[2] == st::kGoalAllColour[2]);
        }
    }
    CHECK(changed == 1);
    CHECK(!st::RecolourGoal(drawn, 0) && !st::RecolourGoal(nullptr, n));
}

void TestLightInTheGate()
{
    const SceneObject& gate = st::kStageStatics[1];
    CHECK(gate.layerMask == kLayerA);
    const float front = gate.position[2] - 0.5f * gate.scale[2];
    const float back = gate.position[2] + 0.5f * gate.scale[2];
    const float top = gate.position[1] + 0.5f * gate.scale[1];
    const Vec3 c = st::kHiddenLights[0].centre;
    // the light is inside the gate (so it is hidden there in A)
    CHECK(c.z - st::kLightRadius >= front - 1e-4f && c.z + st::kLightRadius <= back + 1e-4f && c.y + st::kLightRadius <= top);
    // in A a ball cannot come closer than its radius to the faces: try every height in front of, behind and above the gate
    bool touched = false;
    for (float y = st::kTerraceY; y < top + 1.0f; y += 0.01f)
    {
        touched = touched || Touches({c.x, y - kRadius, front - kRadius}) || Touches({c.x, y - kRadius, back + kRadius});
    }
    touched = touched || Touches({c.x, top, c.z}); // standing on its top (which cannot be reached anyway)
    CHECK(!touched);
    // in B: a held jump straight up from the red outline on the terrace
    CHECK(JumpReaches({c.x, st::kTerraceY, c.z}, 0.0f, 0.0f));
    // the main path (x = 0, a jump anywhere along the terrace) never touches it
    bool onPath = false;
    for (float z = 0.0f; z < 6.0f; z += 0.25f)
    {
        onPath = onPath || JumpReaches({0.0f, st::kTerraceY, z}, 0.0f, 1.0f);
    }
    CHECK(!onPath);
}

} // namespace

int main()
{
    TestLogic();
    TestLightInTheGate();
    std::printf("%d checks passed, %d failed\n", g_passed, g_failed);
    return g_failed == 0 ? 0 : 1;
}
