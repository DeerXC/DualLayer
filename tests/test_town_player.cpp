#include "scene/TownPlayer.h"
#include <cstdio>

using namespace dl;
using namespace dl::town;

static int failures = 0;
static int passed = 0;
#define CHECK(value) do { if (!(value)) { std::printf("FAIL %d: %s\n", __LINE__, #value); ++failures; } else { ++passed; } } while (false)

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
        p.Update({}, {0, 0, 1}, 0, 0, 1.0f / 120.0f);
    }
    for (int i = 0; i < 100; ++i)
    {
        Player::Input in;
        in.moveForward = forward ? 1.0f : 0.0f;
        in.jumpPressed = i == 0;
        in.jumpHeld = true;
        p.Update(in, {0, 0, 1}, 0, 0, 1.0f / 120.0f);
        if (TryTownMantle(p, m, t, 0, maxRise))
        {
            return p.Grounded() && p.Feet().y > height && p.Feet().z > 0.0f;
        }
    }
    return false;
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
    scene::SceneObject objects[20]{};
    const Player player;
    CHECK(AddTownAvatar(objects, 0, 20, player, 0.0f, 0.0f, 0.0f, false) == 15);
    // The hood is object 3 and is unpitched, so its upper extent is half its Y scale.
    CHECK(objects[3].shape == scene::ShapeKind::Sphere);
    CHECK(std::fabs(objects[3].position[1] + objects[3].scale[1] * 0.5f - player.Feet().y - 1.72f) < 0.001f);
    CHECK(AddTownAvatar(objects, 0, 20, player, 0.0f, 1.0f, 1.6f, true) == 19);
    CHECK(objects[0].tilt > 0.15f && objects[17].glow > 0.7f && objects[17].layerMask == scene::kLayerBoth);
    CHECK(std::fabs(objects[8].tilt - objects[11].tilt) > 0.8f);
    CHECK(AddTownAvatar(objects, 2, 20, player, 0.0f, 0.0f, 0.0f, true) == 2);
    std::printf("town_player: %d passed, %d failed\n", passed, failures);
    return failures != 0 ? 1 : 0;
}
