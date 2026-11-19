// test_player.cpp - tests of the walking body (no window, no GPU): src/scene/Player.h on the real scene table.
//   g++ -std=c++20 -Wall -Wextra -I src tests/test_player.cpp -o test_player
//   MSVC (Developer PowerShell):  .\tests\run_tests.ps1
// Header-only: there is no .cpp to add to the project. What is checked:
//   1. standing: the body rests on the floor (feet at the top of the floor, eye at the eye height) and a fall ends there
//   2. walking: forward, back and sideways at the walking speed; two keys are not faster; looking up or down does not slow it
//   3. jumping: the feet rise exactly the jump height, the landing is reported once, a jump pressed shortly before the landing
//      happens at the landing (jump buffer), a jump shortly after leaving an edge still works (coyote time) and a later one does not
//   4. layers: the red cube stops the body in A and not in B; standing on top of an object and switching to the layer in which it
//      does not exist makes the body fall; the cylinder can be jumped onto
//   5. falling off the floor puts the body back at the spawn point
//   6. a long random run in both layers: never a bad number, never inside a solid
//   7. bad input (NaN, zero or negative time, a huge frame, nonsense settings) changes nothing that matters
// Returns 1 when a check fails.

#include "scene/Player.h"

#include <cmath>
#include <cstdio>
#include <initializer_list>
#include <limits>

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

using dl::Player;
using dl::Vec3;
using namespace dl::scene;

constexpr float kFloor = kFloorY;           // the top of the floor
constexpr float kDt = 1.0f / 60.0f;         // UNITS: seconds, one frame
const Vec3 kAhead{0.0f, 0.0f, 1.0f};        // looking along +Z, as the camera does at the start

bool Near(float a, float b, float eps)
{
    return std::fabs(a - b) <= eps;
}

/// A player and the clock of the turning shapes, so that a test can run the world forward.
struct World
{
    Player player;
    float spin = 0.0f;
    int layer = 0;

    World()
    {
        player.Respawn();
    }

    void Run(float seconds, Player::Input input = {}, Vec3 look = kAhead, float dt = kDt)
    {
        const int frames = static_cast<int>(std::lround(seconds / dt));
        for (int i = 0; i < frames; ++i)
        {
            spin += dt;
            player.Update(input, look, layer, spin, dt);
            input.jumpPressed = false; // a key goes down once
        }
    }

    void PlaceFeet(const Vec3& feet)
    {
        player.Place({feet.x, feet.y + player.GetSettings().eyeHeight, feet.z});
    }
};

Player::Input Keys(float forward, float right = 0.0f, bool jump = false)
{
    Player::Input input;
    input.moveForward = forward;
    input.moveRight = right;
    input.jumpPressed = jump;
    return input;
}

void TestStanding()
{
    World w;
    w.Run(1.0f);
    CHECK(w.player.Grounded());
    CHECK(Near(w.player.Feet().y, kFloor, 0.005f));
    CHECK(Near(w.player.Eye().y, kFloor + w.player.GetSettings().eyeHeight, 0.005f));
    CHECK(Near(w.player.VerticalSpeed(), 0.0f, 0.5f));
    CHECK(Near(w.player.Feet().x, 0.0f, 1e-3f) && Near(w.player.Feet().z, -3.0f, 1e-3f)); // it did not drift
    // a fall from a height ends on the floor, in both layers
    for (int layer = 0; layer < 2; ++layer)
    {
        World f;
        f.layer = layer;
        f.player.Place({0.0f, 6.0f, -3.0f});
        f.Run(0.2f);
        CHECK(!f.player.Grounded()); // still in the air
        f.Run(1.5f);
        CHECK(f.player.Grounded() && Near(f.player.Feet().y, kFloor, 0.005f));
    }
}

void TestWalking()
{
    const float speed = Player().GetSettings().walkSpeed;
    {
        World w; // forward: along +Z
        w.Run(0.5f, Keys(1.0f));
        CHECK(Near(w.player.Feet().z, -3.0f + 0.5f * speed, 0.05f) && Near(w.player.Feet().x, 0.0f, 0.01f));
        CHECK(w.player.Grounded() && Near(w.player.Feet().y, kFloor, 0.005f)); // and it stays on the floor
    }
    {
        World w; // back
        w.Run(0.5f, Keys(-1.0f));
        CHECK(Near(w.player.Feet().z, -3.0f - 0.5f * speed, 0.05f));
    }
    {
        World w; // sideways: right of +Z is +X
        w.Run(0.5f, Keys(0.0f, 1.0f));
        CHECK(Near(w.player.Feet().x, 0.5f * speed, 0.05f) && Near(w.player.Feet().z, -3.0f, 0.01f));
        World l;
        l.Run(0.5f, Keys(0.0f, -1.0f));
        CHECK(Near(l.player.Feet().x, -0.5f * speed, 0.05f));
    }
    {
        World w; // two keys together: the same speed, at 45 degrees
        w.Run(0.5f, Keys(1.0f, 1.0f));
        const float dx = w.player.Feet().x;
        const float dz = w.player.Feet().z + 3.0f;
        CHECK(Near(std::sqrt(dx * dx + dz * dz), 0.5f * speed, 0.05f) && Near(dx, dz, 0.01f));
    }
    {
        World w; // looking up or down does not change the speed on the ground
        w.Run(0.5f, Keys(1.0f), {0.0f, 0.7071f, 0.7071f});
        CHECK(Near(w.player.Feet().z, -3.0f + 0.5f * speed, 0.05f));
        World d;
        d.Run(0.5f, Keys(1.0f), {0.0f, -0.7071f, 0.7071f});
        CHECK(Near(d.player.Feet().z, -3.0f + 0.5f * speed, 0.05f));
    }
    {
        World w; // looking along +X: forward is +X
        w.Run(0.5f, Keys(1.0f), {1.0f, 0.0f, 0.0f});
        CHECK(Near(w.player.Feet().x, 0.5f * speed, 0.05f) && Near(w.player.Feet().z, -3.0f, 0.01f));
        World g; // looking straight up: no direction on the ground, no walking
        g.Run(0.5f, Keys(1.0f), {0.0f, 1.0f, 0.0f});
        CHECK(Near(g.player.Feet().x, 0.0f, 1e-3f) && Near(g.player.Feet().z, -3.0f, 1e-3f));
    }
    {
        World w; // the walking speed can be changed, and 0 means standing still
        Player::Settings s;
        s.walkSpeed = 0.0f;
        w.player.SetSettings(s);
        w.Run(0.5f, Keys(1.0f));
        CHECK(Near(w.player.Feet().z, -3.0f, 1e-3f));
    }
}

void TestJumping()
{
    // the height of a jump is the jump height, measured as the highest point of the feet (not only through the report)
    for (float h : {0.5f, 1.2f, 2.0f})
    {
        World w;
        Player::Settings s;
        s.jumpHeight = h;
        w.player.SetSettings(s);
        w.Run(0.3f);
        w.Run(0.02f, Keys(0.0f, 0.0f, true));
        float top = kFloor;
        for (int i = 0; i < 240; ++i)
        {
            w.Run(kDt);
            top = w.player.Feet().y > top ? w.player.Feet().y : top;
        }
        CHECK(Near(top - kFloor, h, 0.03f));
        CHECK(w.player.Grounded() && Near(w.player.Feet().y, kFloor, 0.005f)); // and it is back down
    }
    {
        World w; // the landing is reported once, with the peak
        w.Run(0.3f);
        w.Run(kDt, Keys(0.0f, 0.0f, true));
        float peak = -1.0f;
        int landings = 0;
        for (int i = 0; i < 240; ++i)
        {
            w.Run(kDt);
            float p = 0.0f;
            if (w.player.ConsumeLanding(p))
            {
                ++landings;
                peak = p;
            }
        }
        CHECK(landings == 1 && Near(peak, w.player.GetSettings().jumpHeight, 0.03f));
        float none = 0.0f;
        CHECK(!w.player.ConsumeLanding(none));
    }
    {
        World w; // no jump in the air
        w.Run(0.3f);
        w.Run(kDt, Keys(0.0f, 0.0f, true));
        w.Run(0.3f);
        const float vBefore = w.player.VerticalSpeed(); // falling or near the top
        w.Run(kDt, Keys(0.0f, 0.0f, true));              // pressed in the air, far from the ground: nothing happens now
        CHECK(w.player.VerticalSpeed() <= vBefore + 0.1f);
    }
    {
        World w; // a jump pressed a moment before the landing happens at the landing (jump buffer)
        w.Run(0.3f);
        w.Run(kDt, Keys(0.0f, 0.0f, true));
        int guard = 0;
        const float fine = 1.0f / 240.0f; // a finer clock: at 1/60 s the body falls 0.18 m per frame and could skip the window
        while (!(w.player.VerticalSpeed() < 0.0f && w.player.Feet().y - kFloor < 0.12f) && guard++ < 2400)
        {
            w.Run(fine, {}, kAhead, fine);
        }
        CHECK(guard < 2400);
        w.Run(fine, Keys(0.0f, 0.0f, true), kAhead, fine); // pressed less than 0.12 m above the floor
        float rising = 0.0f;
        for (int i = 0; i < 12; ++i)
        {
            w.Run(kDt);
            rising = w.player.VerticalSpeed() > rising ? w.player.VerticalSpeed() : rising;
        }
        CHECK(rising > 5.0f);
    }
    {
        World w; // no jump with jump height 0
        Player::Settings s;
        s.jumpHeight = 0.0f;
        w.player.SetSettings(s);
        w.Run(0.3f);
        w.Run(0.5f, Keys(0.0f, 0.0f, true));
        float p = 0.0f;
        CHECK(!w.player.ConsumeLanding(p) && Near(w.player.Feet().y, kFloor, 0.005f));
    }
}

void TestEdge()
{
    // walk +Z off the end of the floor (it ends at z = 15), with a clock of 1/120 s so that the moment is found exactly
    const float dt = 1.0f / 120.0f;
    const auto walkOff = [&](World& w) {
        w.PlaceFeet({0.0f, kFloor, 14.0f});
        w.Run(0.3f, Keys(0.0f), kAhead, dt);
        int guard = 0;
        while (w.player.Grounded() && guard++ < 1200)
        {
            w.Run(dt, Keys(1.0f), kAhead, dt);
        }
        return guard < 1200;
    };
    {
        World w; // coyote time: a jump right after leaving the edge still works
        CHECK(walkOff(w));
        w.Run(dt, Keys(1.0f, 0.0f, true), kAhead, dt);
        CHECK(w.player.VerticalSpeed() > 5.0f);
    }
    {
        World w; // 0.2 s after leaving the edge it does not
        CHECK(walkOff(w));
        w.Run(0.2f, Keys(1.0f), kAhead, dt);
        w.Run(dt, Keys(1.0f, 0.0f, true), kAhead, dt);
        CHECK(w.player.VerticalSpeed() < 1.0f);
    }
    {
        World w; // the fall ends at the spawn point
        CHECK(walkOff(w));
        bool respawned = false;
        for (int i = 0; i < 600 && !respawned; ++i)
        {
            w.Run(kDt, Keys(1.0f));
            respawned = w.player.JustRespawned();
        }
        CHECK(respawned);
        CHECK(Near(w.player.Feet().x, 0.0f, 1e-4f) && Near(w.player.Feet().y, kFloor, 1e-4f) && Near(w.player.Feet().z, -3.0f, 1e-4f));
        CHECK(Near(w.player.VerticalSpeed(), 0.0f, 1e-4f));
        w.Run(kDt); // and the flag is off one frame later
        CHECK(!w.player.JustRespawned());
        w.Run(1.0f);
        CHECK(w.player.Grounded());
    }
    {
        World w; // the spawn point can be moved
        w.player.SetSpawn({2.0f, kFloor, -4.0f});
        w.player.Respawn();
        CHECK(Near(w.player.Feet().x, 2.0f, 1e-5f) && Near(w.player.Feet().z, -4.0f, 1e-5f));
    }
}

void TestLayers()
{
    const SceneObject* cube = nullptr;
    const SceneObject* cylinder = nullptr;
    for (const SceneObject& o : kScene)
    {
        if (o.shape == ShapeKind::Cube && o.layerMask == kLayerA)
        {
            cube = &o;
        }
        if (o.shape == ShapeKind::Cylinder && o.layerMask == kLayerA)
        {
            cylinder = &o;
        }
    }
    CHECK(cube != nullptr && cylinder != nullptr);
    if (cube == nullptr || cylinder == nullptr)
    {
        return;
    }
    const float cubeTop = cube->position[1] + 0.5f * cube->scale[1];
    const float cylinderTop = cylinder->position[1] + 0.5f * cylinder->scale[1];
    const Vec3 east{1.0f, 0.0f, 0.0f};
    {
        World a; // walking east into the red cube: stopped in A ...
        a.PlaceFeet({-6.0f, kFloor, kRowZ});
        a.Run(1.0f, Keys(1.0f), east);
        CHECK(a.player.Feet().x < cube->position[0] - 0.5f);
        World b; // ... free in B (until the blue sphere, which is solid in B)
        b.layer = 1;
        b.PlaceFeet({-6.0f, kFloor, kRowZ});
        b.Run(1.0f, Keys(1.0f), east);
        CHECK(b.player.Feet().x > cube->position[0] + 0.3f);
    }
    {
        World w; // standing on top of the red cube (A), then the layer changes to B: the cube is gone, the body falls
        w.PlaceFeet({cube->position[0], cubeTop + 0.2f, cube->position[2]});
        w.Run(1.0f);
        CHECK(w.player.Grounded() && Near(w.player.Feet().y, cubeTop, 0.01f));
        w.layer = 1;
        w.Run(0.1f);
        CHECK(!w.player.Grounded() && w.player.VerticalSpeed() < -1.0f);
        w.Run(1.5f);
        CHECK(w.player.Grounded() && Near(w.player.Feet().y, kFloor, 0.01f));
    }
    {
        // a steep side is not a place to stand: on the blue sphere (B) at 60 degrees from the top the body is not grounded and slides off
        const SceneObject* sphere = nullptr;
        for (const SceneObject& o : kScene)
        {
            if (o.shape == ShapeKind::Sphere)
            {
                sphere = &o;
            }
        }
        CHECK(sphere != nullptr);
        if (sphere != nullptr)
        {
            World w;
            w.layer = 1;
            const float reach = 0.5f * sphere->scale[0] + w.player.GetSettings().radius; // the ball centre is this far from the sphere centre
            const float angle = 1.0472f;                                                // 60 degrees from straight up
            w.PlaceFeet({sphere->position[0] + reach * std::sin(angle),
                         sphere->position[1] + reach * std::cos(angle) - w.player.GetSettings().radius + 0.05f, sphere->position[2]});
            int groundedFrames = 0;
            float fastest = 0.0f;
            for (int i = 0; i < 12; ++i) // the first 0.2 s: on the slope
            {
                w.Run(kDt);
                groundedFrames += w.player.Grounded() ? 1 : 0;
                fastest = w.player.SlideSpeed() > fastest ? w.player.SlideSpeed() : fastest;
            }
            CHECK(groundedFrames == 0); // the slope is too steep to stand on
            CHECK(fastest > 0.5f);      // and the fall turns into a slide
            w.Run(2.0f);
            CHECK(w.player.Grounded() && Near(w.player.Feet().y, kFloor, 0.01f));
            const float restX = w.player.Feet().x; // on the floor the slide is over: friction
            w.Run(0.5f);
            CHECK(w.player.SlideSpeed() < 1e-4f && Near(w.player.Feet().x, restX, 0.01f));
        }
    }
    {
        World w; // the cylinder top is a place to stand as well
        w.PlaceFeet({cylinder->position[0], cylinderTop + 0.3f, cylinder->position[2]});
        w.Run(1.0f);
        CHECK(w.player.Grounded() && Near(w.player.Feet().y, cylinderTop, 0.01f));
    }
    {
        World w; // the cylinder can be jumped onto from the floor (the jump height is above its height)
        w.PlaceFeet({cylinder->position[0] - 1.1f, kFloor, cylinder->position[2]});
        w.Run(0.3f);
        w.Run(kDt, Keys(1.0f, 0.0f, true), east);
        w.Run(0.45f, Keys(1.0f), east); // forward for a short time only: a longer walk crosses the top and leaves on the other side
        w.Run(0.5f);
        CHECK(w.player.Grounded() && w.player.Feet().y > cylinderTop - 0.1f);
    }
}

/// True when none of the three balls of the body overlaps a solid of `layer` by more than a little.
bool BodyIsClear(const Player& p, int layer, float spin)
{
    const Player::Settings s = p.GetSettings();
    for (int i = 0; i < 3; ++i)
    {
        const float t = static_cast<float>(i) / 2.0f;
        const Vec3 c{p.Feet().x, p.Feet().y + s.radius + (s.eyeHeight - s.radius) * t, p.Feet().z};
        const dl::CollisionResult r = dl::ResolveSphere(c, s.radius, layer, spin);
        const float dx = r.position.x - c.x;
        const float dy = r.position.y - c.y;
        const float dz = r.position.z - c.z;
        if (std::sqrt(dx * dx + dy * dy + dz * dz) > 4e-3f)
        {
            return false;
        }
    }
    return true;
}

void TestRandomRun()
{
    unsigned state = 2024u;
    const auto next = [&state]() {
        state = state * 1664525u + 1013904223u;
        return static_cast<float>((state >> 8) & 0xFFFFu) / 65535.0f; // 0 .. 1
    };
    World w;
    int bad = 0;
    int respawns = 0;
    int unclear = 0;
    Player::Input input;
    Vec3 look = kAhead;
    for (int i = 0; i < 40000; ++i)
    {
        if (i % 45 == 0) // a new wish every 0.75 s
        {
            input.moveForward = std::floor(next() * 3.0f) - 1.0f; // -1, 0, 1
            input.moveRight = std::floor(next() * 3.0f) - 1.0f;
            const float yaw = 6.2831853f * next();
            const float pitch = (next() - 0.5f) * 2.4f;
            look = {std::sin(yaw) * std::cos(pitch), std::sin(pitch), std::cos(yaw) * std::cos(pitch)};
        }
        if (i % 700 == 0)
        {
            w.layer = 1 - w.layer; // the layer changes, often while the body is inside the other layer's objects
        }
        input.jumpPressed = next() < 0.01f;
        const float dt = 1.0f / 200.0f + next() * (1.0f / 20.0f - 1.0f / 200.0f);
        w.spin += dt;
        w.player.Update(input, look, w.layer, w.spin, dt);
        const Vec3 f = w.player.Feet();
        if (!(std::isfinite(f.x) && std::isfinite(f.y) && std::isfinite(f.z) && std::isfinite(w.player.VerticalSpeed())))
        {
            ++bad;
        }
        if (w.player.JustRespawned())
        {
            ++respawns;
        }
        else if (!BodyIsClear(w.player, w.layer, w.spin))
        {
            ++unclear;
        }
        if (f.y < kFloor - 21.0f || f.y > kFloor + 20.0f)
        {
            ++bad;
        }
    }
    CHECK(bad == 0);
    CHECK(unclear == 0);
    if (unclear != 0)
    {
        std::printf("  (%d steps ended with a ball inside a solid)\n", unclear);
    }
    // a random walk of about 1400 s leaves the 24 m floor now and then: the run must have used the fall rule, and must not be
    // falling all the time
    CHECK(respawns > 0 && respawns < 400);
}

void TestBadInput()
{
    const float notANumber = std::numeric_limits<float>::quiet_NaN();
    const float infinity = std::numeric_limits<float>::infinity();
    World w;
    w.Run(0.5f);
    const Vec3 before = w.player.Feet();
    w.player.Update(Keys(1.0f), kAhead, 0, w.spin, notANumber); // no time, no change
    w.player.Update(Keys(1.0f), kAhead, 0, w.spin, -1.0f);
    w.player.Update(Keys(1.0f), kAhead, 0, w.spin, 0.0f);
    w.player.Update(Keys(1.0f), kAhead, 0, notANumber, kDt);
    CHECK(Near(w.player.Feet().x, before.x, 1e-6f) && Near(w.player.Feet().z, before.z, 1e-6f) && Near(w.player.Feet().y, before.y, 1e-6f));
    // bad keys and a bad direction: no walking, no bad numbers
    w.player.Update(Keys(notANumber, infinity), kAhead, 0, w.spin, kDt);
    w.player.Update(Keys(1.0f), {notANumber, 0.0f, 1.0f}, 0, w.spin, kDt);
    CHECK(std::isfinite(w.player.Feet().x) && std::isfinite(w.player.Feet().y) && std::isfinite(w.player.Feet().z));
    CHECK(Near(w.player.Feet().x, before.x, 1e-3f) && Near(w.player.Feet().z, before.z, 1e-3f));
    // a layer that does not exist: nothing is solid, so the body falls through the floor and comes back at the spawn point
    World bad;
    bad.layer = 7;
    bad.Run(3.0f);
    CHECK(std::isfinite(bad.player.Feet().y));
    // a huge frame (a stall) is cut to 0.1 s: the body stays on the floor and does not tunnel
    World stall;
    stall.Run(0.5f);
    stall.player.Update(Keys(1.0f), kAhead, 0, stall.spin, 5.0f);
    CHECK(Near(stall.player.Feet().y, kFloor, 0.01f) && stall.player.Feet().z < -3.0f + 0.5f);
    // a place that is not a number is ignored
    World p;
    const Vec3 spot = p.player.Feet();
    p.player.Place({notANumber, 0.0f, 0.0f});
    p.player.SetSpawn({infinity, 0.0f, 0.0f});
    CHECK(Near(p.player.Feet().x, spot.x, 1e-6f) && Near(p.player.Feet().y, spot.y, 1e-6f));
    // nonsense settings are pulled into a range that works, and the body still stands
    World s;
    Player::Settings nonsense;
    nonsense.radius = notANumber;
    nonsense.eyeHeight = -4.0f;
    nonsense.walkSpeed = 1.0e9f;
    nonsense.gravity = 0.0f;
    nonsense.jumpHeight = infinity;
    s.player.SetSettings(nonsense);
    const Player::Settings fixed = s.player.GetSettings();
    CHECK(std::isfinite(fixed.radius) && fixed.radius >= 0.05f && fixed.radius <= 1.0f);
    CHECK(fixed.eyeHeight >= fixed.radius + 0.1f - 1e-6f && fixed.walkSpeed <= 20.0f && fixed.gravity >= 1.0f && fixed.jumpHeight <= 5.0f);
    s.player.Respawn();
    s.Run(1.0f);
    CHECK(std::isfinite(s.player.Feet().y));
}

} // namespace

int main()
{
    TestStanding();
    TestWalking();
    TestJumping();
    TestEdge();
    TestLayers();
    TestRandomRun();
    TestBadInput();
    std::printf("%d checks passed, %d failed\n", g_passed, g_failed);
    return g_failed == 0 ? 0 : 1;
}
