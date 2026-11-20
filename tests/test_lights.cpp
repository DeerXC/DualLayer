// test_lights.cpp - tests of the hidden lights (no window, no GPU): src/scene/StageLights.h on the stage of src/scene/Stage.h.
//   g++ -std=c++20 -Wall -Wextra -I src tests/test_lights.cpp -o test_lights
//   MSVC (Developer PowerShell):  .\tests\run_tests.ps1
// Header-only: there is no .cpp to add to the project. What is checked:
//   1. the logic: a light is taken when a ball of the body touches it, once; it stays taken; the count and All(); a taken
//      light is not drawn any more; the goal light changes colour only when asked, and only the goal light
//   2. where they are, with the numbers of the walking body (jump 2.0 m, eye 1.55 m, ball radius 0.35 m, 4 m/s, gravity 30
//      up and 48 down): light 0 is reached by a jump in B and cannot be touched from any side of the red wall in A; light 1
//      is out of reach of every jump from the floor and within reach of a jump from the top of the block
// The flights are computed with the same rules as Player.h (a held jump, the walking speed at once); the full body is played
// in tests/test_stage.cpp. Returns 1 when a check fails.

#include "scene/StageLights.h"

#include <cmath>
#include <cstdio>
#include <initializer_list>

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

// the numbers of the walking body (scene/Player.h, the defaults)
constexpr float kRadius = 0.35f;
constexpr float kEye = 1.55f;
constexpr float kWalk = 4.0f;
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

/// True when the ball `i` of a body with its feet at `feet` touches light `light`.
bool Touches(const Vec3& feet, int light)
{
    FakeBody body{feet, {}};
    st::LightSet set;
    return set.Collect(body) == light || set.Taken(light);
}

/// The flight of a held jump from `start` (feet), moving along (dirX, dirZ) at the walking speed, until the feet are 6 m
/// below the start. True when a ball touches `light` on the way.
bool JumpReaches(const Vec3& start, float dirX, float dirZ, int light)
{
    const float jumpSpeed = std::sqrt(2.0f * kGravityUp * kJumpHeight);
    Vec3 feet = start;
    float vy = jumpSpeed;
    const float dt = 1.0f / 600.0f;
    for (int i = 0; i < 6000 && feet.y > start.y - 6.0f; ++i)
    {
        feet.x += dirX * kWalk * dt;
        feet.z += dirZ * kWalk * dt;
        feet.y += vy * dt;
        vy -= (vy > 0.0f ? kGravityUp : kGravityDown) * dt;
        if (Touches(feet, light))
        {
            return true;
        }
    }
    return false;
}

void TestLogic()
{
    st::LightSet set;
    CHECK(set.Count() == 0 && !set.All());
    FakeBody far{{0.0f, kFloorY, -3.0f}, {}};
    CHECK(set.Collect(far) == -1);
    // touching light 1 with the top ball
    FakeBody at{{st::kHiddenLights[1].centre.x, st::kHiddenLights[1].centre.y - kEye, st::kHiddenLights[1].centre.z}, {}};
    CHECK(set.Collect(at) == 1 && set.Taken(1) && set.Count() == 1);
    CHECK(set.Collect(at) == -1 && set.Count() == 1); // once only
    // the drawn table: the two lights not yet taken
    SceneObject table[8] = {};
    CHECK(st::kHiddenLightCount == 2);
    CHECK(set.AddToDrawTable(table, 0, 8) == 1); // the one not yet taken
    CHECK(table[0].shape == ShapeKind::Sphere && table[0].layerMask == kLayerBoth);
    CHECK(table[0].position[2] == st::kHiddenLights[0].centre.z);
    CHECK(set.AddToDrawTable(table, 8, 8) == 8);     // only as many as there is room for
    CHECK(set.AddToDrawTable(nullptr, 3, 8) == 3);
    for (int i : {0})
    {
        FakeBody b{{st::kHiddenLights[i].centre.x, st::kHiddenLights[i].centre.y - kRadius, st::kHiddenLights[i].centre.z}, {}};
        CHECK(set.Collect(b) == i);
    }
    CHECK(set.All() && set.Count() == st::kHiddenLightCount);
    CHECK(set.AddToDrawTable(table, 0, 8) == 0); // nothing left to draw
    // the goal light changes colour; nothing else does
    SceneObject drawn[st::kMaxDrawObjects] = {};
    const dl::puzzle::Puzzle puzzle;
    const int n = st::BuildDrawTable(puzzle, drawn);
    SceneObject before[st::kMaxDrawObjects] = {};
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

void TestLightInTheWall()
{
    const SceneObject& wall = st::kStageStatics[1];
    const float wallFront = wall.position[2] - 0.5f * wall.scale[2];
    const float wallBack = wall.position[2] + 0.5f * wall.scale[2];
    const float wallTop = wall.position[1] + 0.5f * wall.scale[1];
    const Vec3 c = st::kHiddenLights[0].centre;
    // the light is inside the wall (so it is hidden there in A)
    CHECK(c.z - st::kLightRadius >= wallFront - 1e-4f && c.z + st::kLightRadius <= wallBack + 1e-4f && c.y + st::kLightRadius <= wallTop);
    // in A a ball cannot come closer than its radius to the faces: try every height in front of, behind and above the wall
    bool touched = false;
    for (float y = kFloorY; y < wallTop + 1.0f; y += 0.01f)
    {
        touched = touched || Touches({c.x, y - kRadius, wallFront - kRadius}, 0) || Touches({c.x, y - kRadius, wallBack + kRadius}, 0);
    }
    touched = touched || Touches({c.x, wallTop, c.z}, 0); // standing on the top (it cannot be reached, and still)
    CHECK(!touched);
    // in B: a held jump straight up, from the red outline on the floor under it
    CHECK(JumpReaches({c.x, kFloorY, c.z}, 0.0f, 0.0f, 0));
    // the main path (x = 0, a jump anywhere along the corridor) never touches it
    bool onPath = false;
    for (float z = 0.0f; z < 6.0f; z += 0.25f)
    {
        onPath = onPath || JumpReaches({0.0f, kFloorY, z}, 0.0f, 1.0f, 0);
    }
    CHECK(!onPath);
}

void TestLightOverTheBlock()
{
    const Vec3 c = st::kHiddenLights[1].centre;
    // from the floor: no jump, in any direction and from any nearby start, reaches it
    bool fromFloor = false;
    for (float dx = -2.0f; dx <= 2.0f; dx += 0.25f)
    {
        for (float dz = -2.0f; dz <= 2.0f; dz += 0.25f)
        {
            fromFloor = fromFloor || JumpReaches({c.x + dx, kFloorY, c.z + dz}, 0.0f, 0.0f, 1) ||
                        JumpReaches({c.x + dx, kFloorY, c.z + dz}, 0.7071f, 0.7071f, 1);
        }
    }
    CHECK(!fromFloor);
    // from the top of the block resting in the cell under it
    CHECK(JumpReaches({c.x, kFloorY + 1.0f, c.z}, 0.0f, 0.0f, 1));
    // the cell under it is a cell the block may use
    const dl::puzzle::StageDesc d = st::MakeStage();
    const int cx = static_cast<int>(std::lround(c.x));
    const int cz = static_cast<int>(std::lround(c.z));
    CHECK(cx >= d.minCell.x && cx <= d.maxCell.x && cz >= d.minCell.z && cz <= d.maxCell.z);
    // from the blue ledge (the highest place of the main path): out of reach
    bool fromLedge = false;
    for (float x = 0.5f; x <= 2.0f; x += 0.25f)
    {
        fromLedge = fromLedge || JumpReaches({x, st::kDoorwayLow, 19.9f}, -0.55f, -0.83f, 1);
    }
    CHECK(!fromLedge);
}

} // namespace

int main()
{
    TestLogic();
    TestLightInTheWall();
    TestLightOverTheBlock();
    std::printf("%d checks passed, %d failed\n", g_passed, g_failed);
    return g_failed == 0 ? 0 : 1;
}
