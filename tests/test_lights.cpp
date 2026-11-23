// test_lights.cpp - tests of the hidden lights (no window, no GPU): src/scene/StageLights.h on the map of src/scene/Stage.h.
//   g++ -std=c++20 -Wall -Wextra -I src tests/test_lights.cpp -o test_lights
//   MSVC (Developer PowerShell):  .\tests\run_tests.ps1 -Only lights
// Header-only: there is no .cpp to add to the project. What is checked:
//   1. the logic: a light is taken when a ball of the body touches it, once; it stays taken; a taken light is not drawn any
//      more; with every light taken the goal light changes colour, and only the goal light
//   2. the light in the gate, with the numbers of the walking body (the jump and the walking speed of Player.h; flights
//      computed with its rules, a held jump): not touched from any side of the gate in A, reached by a jump in B, not touched
//      by jumps along the main path
//   3. the light on the floor of the pool: under the water of A (every pose that touches it is under water), taken standing in
//      B, not reached from the rim
//   4. the light on the bed of the canal: under the water of A at either level, taken standing in B, not reached from the quay
//   5. the light behind the bars: taken in the undercroft, never from the rim of the square through the bars
//   6. the light in the wall of the north hall: taken on the ledge of N2, never by a jump from the floor
//   7. the light under the courtyard: taken in the cellar, not from the courtyard above
// Returns 1 when a check fails.

#include "scene/StageLights.h"

#include <cmath>
#include <cstddef>
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

const dl::Player::Settings kBody{};

/// A body as the lights see it: three balls above the feet, like Player.
struct FakeBody
{
    Vec3 feet;
    struct Settings
    {
        float radius = kBody.radius;
    } settings;
    const Settings& GetSettings() const
    {
        return settings;
    }
    Vec3 Ball(int i) const
    {
        const float t = static_cast<float>(i) / 2.0f;
        return {feet.x, feet.y + kBody.radius + (kBody.eyeHeight - kBody.radius) * t, feet.z};
    }
};

bool Touches(const Vec3& feet, int light)
{
    FakeBody body{feet, {}};
    st::LightSet set;
    set.Collect(body);
    return set.Taken(light);
}

/// The flight of a held jump from `start` (feet), moving along (dirX, dirZ) at the walking speed, until the feet are 8 m
/// below the start. True when a ball touches `light` on the way.
bool JumpReaches(const Vec3& start, float dirX, float dirZ, int light)
{
    const float jumpSpeed = std::sqrt(2.0f * kBody.gravity * kBody.jumpHeight);
    Vec3 feet = start;
    float vy = jumpSpeed;
    const float dt = 1.0f / 600.0f;
    for (int i = 0; i < 8000 && feet.y > start.y - 8.0f; ++i)
    {
        feet.x += dirX * kBody.walkSpeed * dt;
        feet.z += dirZ * kBody.walkSpeed * dt;
        feet.y += vy * dt;
        vy -= (vy > 0.0f ? kBody.gravity : kBody.gravity * kBody.fallGravityScale) * dt;
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
    FakeBody far{{0.0f, st::kTerraceY, -3.0f}, {}};
    CHECK(set.Collect(far) == -1);
    SceneObject table[16] = {};
    CHECK(set.AddToDrawTable(table, 0, 16) == st::kHiddenLightCount && table[0].shape == ShapeKind::Sphere && table[0].layerMask == kLayerBoth);
    CHECK(set.AddToDrawTable(table, 16, 16) == 16 && set.AddToDrawTable(nullptr, 3, 16) == 3);
    for (int i = 0; i < st::kHiddenLightCount; ++i)
    {
        const Vec3 c = st::kHiddenLights[i].centre;
        FakeBody at{{c.x, c.y - kBody.eyeHeight, c.z}, {}};
        CHECK(set.Collect(at) == i && set.Taken(i));
        CHECK(set.Collect(at) == -1); // once only
    }
    CHECK(set.All() && set.Count() == st::kHiddenLightCount);
    CHECK(set.AddToDrawTable(table, 0, 8) == 0); // nothing left to draw
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
    const SceneObject& gate = st::kStageStatics[static_cast<std::size_t>(st::kGateIndex)];
    CHECK(gate.layerMask == kLayerA && std::fabs(gate.position[2] - st::kGateZ) < 1e-4f);
    const float front = gate.position[2] - 0.5f * gate.scale[2];
    const float back = gate.position[2] + 0.5f * gate.scale[2];
    const float top = gate.position[1] + 0.5f * gate.scale[1];
    const Vec3 c = st::kHiddenLights[0].centre;
    CHECK(c.z - st::kLightRadius >= front - 1e-4f && c.z + st::kLightRadius <= back + 1e-4f && c.y + st::kLightRadius <= top);
    bool touched = false; // in A a ball cannot come closer than its radius to the faces of the gate
    for (float y = st::kTerraceY; y < top + 1.0f; y += 0.01f)
    {
        touched = touched || Touches({c.x, y - kBody.radius, front - kBody.radius}, 0) || Touches({c.x, y - kBody.radius, back + kBody.radius}, 0);
    }
    CHECK(!touched);
    CHECK(JumpReaches({c.x, st::kTerraceY, c.z}, 0.0f, 0.0f, 0)); // in B, from the red outline on the terrace
    bool onPath = false; // the main path (x = 0, a jump anywhere along the terrace) never touches it
    for (float z = st::kGateZ - 4.0f; z < st::kGateZ + 4.0f; z += 0.25f)
    {
        onPath = onPath || JumpReaches({0.0f, st::kTerraceY, z}, 0.0f, 1.0f, 0);
    }
    CHECK(!onPath);
}

void TestLightInThePool()
{
    const Vec3 c = st::kHiddenLights[1].centre;
    const dl::shapes::Water& pool = st::kWaters[st::kPoolWater];
    CHECK(c.x > pool.x0 && c.x < pool.x1 && c.z > pool.z0 && c.z < pool.z1 && c.y + st::kLightRadius < pool.top - 0.3f); // under the water of A
    CHECK(Touches({c.x, st::kSquareLowY, c.z}, 1)); // in B a body standing on the dry floor takes it
    // in A every pose of a body that touches it is under the water: main.cpp puts such a body back before it takes lights
    bool dryTouch = false;
    for (float dx = -1.0f; dx <= 1.0f; dx += 0.05f)
    {
        for (float dz = -1.0f; dz <= 1.0f; dz += 0.05f)
        {
            for (float y = st::kSquareLowY; y < kFloorY + 1.0f; y += 0.02f)
            {
                const Vec3 feet{c.x + dx, y, c.z + dz};
                dryTouch = dryTouch || (Touches(feet, 1) && !st::Submerged(feet, 0));
            }
        }
    }
    CHECK(!dryTouch);
    // nothing reaches it from the rim without going into the pool
    CHECK(!Touches({c.x, kFloorY, st::kPoolNorth + 0.1f}, 1) && !JumpReaches({c.x, kFloorY, st::kPoolNorth + 0.3f}, 0.0f, 0.0f, 1));
}

void TestLightInTheCanal()
{
    const Vec3 c = st::kHiddenLights[2].centre;
    const dl::canal::Desc d = st::MakeCanal();
    CHECK(c.x > d.x0 && c.x < d.x1 && c.z > d.z0 && c.z < d.z1);
    CHECK(Touches({c.x, d.bed, c.z}, 2)); // in B a body standing on the dry bed takes it
    // in A, at either level of the water, every pose that touches it is in the water
    for (int high = 0; high < 2; ++high)
    {
        dl::canal::Canal canal;
        canal.Init(d);
        if (high == 1)
        {
            struct Hand
            {
                Vec3 at;
                Vec3 Ball(int) const { return at; }
                Vec3 Feet() const { return at; }
                bool Grounded() const { return false; }
                void Carry(const Vec3&) {}
            } hand{d.wheel};
            canal.Update(hand, 0, 0.01f);
            for (int i = 0; i < 1000; ++i)
            {
                canal.Update(hand, 0, 0.01f);
            }
            CHECK(canal.High());
        }
        bool dryTouch = false;
        for (float dx = -1.0f; dx <= 1.0f; dx += 0.05f)
        {
            for (float dz = -1.0f; dz <= 1.0f; dz += 0.05f)
            {
                for (float y = d.bed; y < kFloorY + 1.0f; y += 0.02f)
                {
                    const Vec3 feet{c.x + dx, y, c.z + dz};
                    dryTouch = dryTouch || (Touches(feet, 2) && !canal.Submerged(feet, 0));
                }
            }
        }
        CHECK(!dryTouch);
    }
    // nothing reaches it from the quay or the hall floor without going into the canal
    CHECK(!JumpReaches({c.x, st::kQuayY, st::kCanalSouth - 0.3f}, 0.0f, 0.0f, 2) && !Touches({c.x, st::kQuayY, st::kCanalSouth - 0.1f}, 2));
}

void TestLightBehindBars()
{
    const Vec3 c = st::kHiddenLights[3].centre;
    // taken standing in the undercroft; never from the rim of the square, where the bars stop the body (balls up to the
    // bars: x <= kHubEast - radius), at any height a jump reaches
    CHECK(Touches({c.x, st::kUndercroftY, c.z}, 3));
    bool fromRim = false;
    for (float z = st::kBayZ0 - 0.5f; z <= st::kBayZ1 + 0.5f; z += 0.05f)
    {
        for (float y = kFloorY; y < kFloorY + 2.1f; y += 0.05f)
        {
            fromRim = fromRim || Touches({st::kHubEast - kBody.radius, y, z}, 3);
        }
    }
    CHECK(!fromRim);
    CHECK(c.x > st::kHubEast + 0.5f && c.x < 14.5f && c.z > st::kBayZ0 && c.z < st::kBayZ1); // inside the undercroft
}

void TestLightInTheWall()
{
    const Vec3 c = st::kHiddenLights[4].centre;
    // taken standing on the ledge of N2 at the wall; never from the floor of the north hall, by a jump in either layer (the
    // body comes no closer to the wall than its radius)
    CHECK(Touches({-9.0f + kBody.radius, st::kN2LedgeTop, c.z}, 4));
    bool fromFloor = false;
    for (float z = c.z - 1.0f; z <= c.z + 1.0f; z += 0.1f)
    {
        fromFloor = fromFloor || JumpReaches({-9.0f + kBody.radius, kFloorY, z}, 0.0f, 0.0f, 4);
    }
    CHECK(!fromFloor);
    CHECK(c.x < -9.0f && c.x + st::kLightRadius > -9.0f); // in the wall, a little of it showing
}

void TestLightUnderTheCourtyard()
{
    const Vec3 c = st::kHiddenLights[5].centre;
    CHECK(Touches({c.x, st::kCellarY, c.z}, 5)); // standing in the cellar
    bool fromAbove = false; // the courtyard above: its floor is 2.5 m higher, and the ceiling of the cellar is between
    for (float x = -3.0f; x <= 3.0f; x += 0.25f)
    {
        fromAbove = fromAbove || Touches({x, kFloorY, c.z}, 5);
    }
    CHECK(!fromAbove && c.y + st::kLightRadius < st::kCellarCeiling);
}

} // namespace

int main()
{
    TestLogic();
    TestLightInTheGate();
    TestLightInThePool();
    TestLightInTheCanal();
    TestLightBehindBars();
    TestLightInTheWall();
    TestLightUnderTheCourtyard();
    std::printf("%d checks passed, %d failed\n", g_passed, g_failed);
    return g_failed == 0 ? 0 : 1;
}
