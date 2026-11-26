#include "scene/TownPlayer.h"
#include <algorithm>
#include <cmath>
#include <cstdio>

using namespace dl;
using namespace dl::town;

static int failures = 0;
static int passed = 0;
#define CHECK(value)                                                                                                   \
    do                                                                                                                 \
    {                                                                                                                  \
        if (!(value))                                                                                                  \
        {                                                                                                              \
            std::printf("FAIL %d: %s\n", __LINE__, #value);                                                            \
            ++failures;                                                                                                \
        }                                                                                                              \
        else                                                                                                           \
        {                                                                                                              \
            ++passed;                                                                                                  \
        }                                                                                                              \
    } while (false)

static void Quad(TriMesh& mesh, Vec3 a, Vec3 b, Vec3 c, Vec3 d)
{
    mesh.Add(a, b, c);
    mesh.Add(a, c, d);
}

static bool Climb(float height, float maxRise, bool ceiling, bool forward, bool water = false)
{
    TriMesh a;
    Quad(a, {-4, 0, -4}, {4, 0, -4}, {4, 0, 4}, {-4, 0, 4});
    Quad(a, {-2, 0, 0}, {2, 0, 0}, {2, height, 0}, {-2, height, 0});
    Quad(a, {-2, height, 0}, {2, height, 0}, {2, height, 2}, {-2, height, 2});
    if (ceiling)
    {
        Quad(a, {-2, 3.75f, 0}, {2, 3.75f, 0}, {2, 3.75f, 2}, {-2, 3.75f, 2});
    }
    a.Build();
    Moments m(&a, &a);
    TownData t;
    t.sea = -10.0f;
    if (water)
    {
        t.vertices = {{-2, 3, 0}, {2, 3, 0}, {2, 3, 2}, {-2, 3, 2}};
        t.polygons.push_back({1, Kind::Water, 0, 0, 4});
        t.waters.push_back(0);
    }
    Player p;

    p.SetTown(&m);
    p.SetSpawn({0, 0, -0.60f});
    p.Respawn();
    for (int i = 0; i < 5; ++i)
    {
        p.Update({}, {0, 0, 1}, 0, 1.0f / 120.0f);
    }
    for (int i = 0; i < 100; ++i)
    {
        Player::Input in;
        in.moveForward = forward ? 1.0f : 0.0f;
        in.jumpPressed = i == 0;
        in.jumpHeld = true;
        p.Update(in, {0, 0, 1}, 0, 1.0f / 120.0f);
        if (TryTownMantle(p, m, t, 0, maxRise))
        {
            return p.Grounded() && p.Feet().y > height && p.Feet().z > 0.0f;
        }
    }
    return false;
}

// ---- the traveller (docs/DESIGN.md 6.7): built on a floor, so that the body stands (a default body is in the air)
static float TopOf(const scene::SceneObject& o)
{
    // an upper bound of the highest point of the part: the half height turned by the tilt, plus the half width carried
    // up by it (a sphere, an ellipsoid, reaches exactly the root of the sum of the squares)
    const float h = 0.5f * o.scale[1] * std::cos(o.tilt);
    const float w = 0.5f * (std::max)(o.scale[0], o.scale[2]) * std::sin(o.tilt);
    const float reach = o.shape == scene::ShapeKind::Sphere ? std::sqrt(h * h + w * w) : std::fabs(h) + std::fabs(w);
    return o.position[1] + reach;
}

static float BottomOf(const scene::SceneObject& o)
{
    return o.position[1] - (TopOf(o) - o.position[1]);
}

static void TestAvatar()
{
    TriMesh floor;
    Quad(floor, {-6, 0, -6}, {6, 0, -6}, {6, 0, 6}, {-6, 0, 6});
    floor.Build();
    Moments ground(&floor, &floor);
    Player body;
    body.SetTown(&ground);
    body.SetSpawn({0, 0, 0});
    body.Respawn();
    for (int i = 0; i < 30; ++i)
    {
        body.Update({}, {0, 0, 1}, 0, 1.0f / 120.0f);
    }
    const Player air; // never put on a floor: in the air
    CHECK(body.Grounded() && !air.Grounded());
    const float feetY = body.Feet().y;
    scene::SceneObject o[96]{};
    Flame f;
    f.on = true;
    f.yaw = 0.5f;
    f.tilt = 0.3f;

    // the parts: the body, the lantern in the hand, the flame over it; no room: nothing, the count as it was
    const int bare = AddTownAvatar(o, 0, 96, body, 0.0f, 0.0f, 0.0f, false);
    CHECK(bare == kAvatarBodyObjects);
    CHECK(AddTownAvatar(o, 0, 96, body, 0.0f, 0.0f, 0.0f, true) == kAvatarBodyObjects + kLanternModelObjects);
    CHECK(AddTownAvatar(o, 0, 96, body, 0.0f, 0.0f, 0.0f, true, &f) ==
          kAvatarBodyObjects + kLanternModelObjects + kFlameObjects);
    CHECK(AddTownAvatar(o, 2, 20, body, 0.0f, 0.0f, 0.0f, true) == 2);
    CHECK(AddTownAvatar(o, 0, kAvatarBodyObjects + kLanternModelObjects - 1, body, 0.0f, 0.0f, 0.0f, true) == 0);
    CHECK(AddTownAvatar(nullptr, 0, 96, body, 0.0f, 0.0f, 0.0f, true) == 0);

    // the stature: 1.72 m from the feet to the top of the hood, standing; the hood is the highest part
    AddTownAvatar(o, 0, 96, body, 0.0f, 0.0f, 0.0f, false);
    float top = -1.0f;
    float bottom = 1.0f;
    for (int i = 0; i < bare; ++i)
    {
        top = (std::max)(top, TopOf(o[i]) - feetY);
        bottom = (std::min)(bottom, BottomOf(o[i]) - feetY);
    }
    CHECK(o[kAvatarHood].shape == scene::ShapeKind::Sphere);
    CHECK(std::fabs(TopOf(o[kAvatarHood]) - feetY - kAvatarStature) < 0.001f);
    CHECK(std::fabs(top - kAvatarStature) < 0.001f); // nothing is higher than the hood
    CHECK(bottom > -0.001f && bottom < 0.02f);       // the boots stand on the floor

    // walking, running and jumping: never taller than the stature, nothing sunk far into the floor, both moments
    const float gaits[3] = {0.6f, 1.0f, 1.6f};
    for (int g = 0; g < 3; ++g)
    {
        for (int step = 0; step < 8; ++step)
        {
            const float phase = 0.785398f * static_cast<float>(step);
            const int n = AddTownAvatar(o, 0, 96, body, 0.7f, phase, gaits[g], true, &f, 0.0f, 0.3f);
            float hi = -1.0f;
            float lo = 1.0f;
            for (int i = 0; i < kAvatarBodyObjects; ++i)
            {
                hi = (std::max)(hi, TopOf(o[i]) - feetY);
                lo = (std::min)(lo, BottomOf(o[i]) - feetY);
            }
            CHECK(n == kAvatarBodyObjects + kLanternModelObjects + kFlameObjects);
            CHECK(hi < kAvatarStature + 0.002f && lo > -0.04f);
        }
    }
    CHECK(AddTownAvatar(o, 0, 96, air, 0.0f, 0.0f, 0.0f, true) == kAvatarBodyObjects + kLanternModelObjects);
    bool both = true;
    for (int i = 0; i < kAvatarBodyObjects + kLanternModelObjects; ++i)
    {
        both = both && o[i].layerMask == scene::kLayerBoth && std::isfinite(o[i].position[0]) &&
               std::isfinite(o[i].position[1]) && std::isfinite(o[i].position[2]) && o[i].scale[0] > 0.0f &&
               o[i].scale[1] > 0.0f && o[i].scale[2] > 0.0f;
    }
    CHECK(both);

    // the stride: the feet are far apart in the middle of a step and together when he stands; the hips come down as
    // the legs spread; he leans forward when he runs and the scarf streams behind
    const auto boot = [&](int side) -> const scene::SceneObject&
    { return o[kAvatarFirstLeg + side * kAvatarLegObjects + 4]; };
    AddTownAvatar(o, 0, 96, body, 0.0f, 0.0f, 0.0f, false);
    const float together = std::fabs(boot(0).position[2] - boot(1).position[2]);
    const float chestStand = o[0].position[1];
    const float scarfStand = o[16].tilt;
    AddTownAvatar(o, 0, 96, body, 0.0f, 1.5707963f, 1.0f, false);
    CHECK(together < 0.01f && std::fabs(boot(0).position[2] - boot(1).position[2]) > 0.40f);
    CHECK(o[0].position[1] < chestStand - 0.02f);
    AddTownAvatar(o, 0, 96, body, 0.0f, 1.5707963f, 1.6f, false);
    CHECK(o[0].tilt > 0.15f && o[16].tilt > scarfStand + 0.5f);
    AddTownAvatar(o, 0, 96, body, 0.0f, 0.0f, 1.0f, false); // the legs pass each other: one swings forward
    // a leg that swings forward bends its knee (the shin is not parallel to the thigh); one that stands does not
    const float thigh = o[kAvatarFirstLeg].tilt;
    const float shin = o[kAvatarFirstLeg + 2].tilt;
    const float thigh2 = o[kAvatarFirstLeg + kAvatarLegObjects].tilt;
    const float shin2 = o[kAvatarFirstLeg + kAvatarLegObjects + 2].tilt;
    CHECK(std::fabs((shin - thigh) - (shin2 - thigh2)) > 0.5f);

    // the lantern: in the right hand, ahead of the body and under the shoulder; the arm raises it with H
    Vec3 low{};
    Vec3 high{};
    AddTownAvatar(o, 0, 96, body, 0.0f, 0.0f, 0.0f, true, nullptr, 0.0f, 0.0f, &low);
    const int with = AddTownAvatar(o, 0, 96, body, 0.0f, 0.0f, 0.0f, true, &f, 1.0f, 0.0f, &high);
    CHECK(with == kAvatarBodyObjects + kLanternModelObjects + kFlameObjects);
    CHECK(low.x > 0.15f && low.z > 0.2f && low.y > feetY + 0.6f && low.y < feetY + 1.0f);
    CHECK(high.y > low.y + 0.3f);
    // its glass glows in both moments, and the plume of the flame stands over the chimney, over the glass
    const scene::SceneObject& plume = o[kAvatarBodyObjects + kLanternModelObjects];
    CHECK(plume.glow > 1.0f && plume.position[1] > high.y + 0.1f);
    float glow = 0.0f;
    for (int i = kAvatarBodyObjects; i < kAvatarBodyObjects + kLanternModelObjects; ++i)
    {
        glow = (std::max)(glow, o[i].glow);
    }
    CHECK(glow > 2.0f); // the flame inside the glass
    // while he walks the lantern sways, and it never touches the floor
    float lowest = 10.0f;
    for (int step = 0; step < 8; ++step)
    {
        AddTownAvatar(o, 0, 96, body, 0.0f, 0.785398f * static_cast<float>(step), 1.0f, true);
        for (int i = kAvatarBodyObjects; i < kAvatarBodyObjects + kLanternModelObjects; ++i)
        {
            lowest = (std::min)(lowest, BottomOf(o[i]) - feetY);
        }
    }
    CHECK(lowest > 0.3f);
}

int main()
{
    TriMesh b;
    Quad(b, {-2, -1, -2}, {2, -1, -2}, {2, 3, -2}, {-2, 3, -2});
    b.Build();
    const Moments m(nullptr, &b);
    const Vec3 eye{0, 1, 0};
    const Vec3 forward{0, 0, 1};
    CHECK(CameraClearDistance(m, eye, forward, 4, 0, false) == 4.0f);
    const float blocked = CameraClearDistance(m, eye, forward, 4, 1, false);
    CHECK(blocked > 1.5f && blocked < 1.81f);
    CHECK(CameraClearDistance(m, eye, forward, 4, 0, true) == blocked);
    CHECK(CameraClearDistance(m, {0, 1, -2}, forward, 4, 1, false) == 0.0f);
    CHECK(Climb(2.10f, 2.20f, false, true));
    CHECK(!Climb(2.10f, 2.0f, false, true));
    CHECK(!Climb(2.10f, 2.20f, true, true));
    CHECK(!Climb(2.10f, 2.20f, false, false));
    CHECK(!Climb(2.10f, 2.20f, false, true, true));
    TestAvatar();
    std::printf("town_player: %d passed, %d failed\n", passed, failures);
    return failures != 0 ? 1 : 0;
}
