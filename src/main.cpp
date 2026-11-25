// main.cpp - entry point and the frame loop of the town.
// Frame:   PumpMessages > (F5: reload config) > (resize) > Update (camera, switch, walk, E, fall) > Render
//          [GPU event "Frame" > "Opaque.Models" > "Overlay"] > Present
// Town:    scene/Town.h loads data/town.dlt, written by tools/layout/city_model.py: the polygons of both moments, baked
// into one
//          static batch per layer, and the solid of each moment (scene/TriMesh.h, scene/Moments.h). Every frame the
//          walking body (scene/Player.h) is moved against the solid of the moment it is in; what is drawn and what is
//          solid come from the same data.
// Keys:    W A S D move, the mouse turns (click into the window to capture it; Esc gives it back, a second Esc quits),
// the arrow
//          keys turn as well, Shift sprints, Space jumps (held = the full jump, a tap = a small hop) and climbs a
//          ledge, Tab switches the moment (A, the past <-> B, the present), E opens a thing of the past or sets the
//          lantern down / takes it up, R goes back to the last place the body stood still on, F5 reloads
//          config/default.ini, F walking / flying, N collision on / off, Z wireframe, X the next level of detail, C the
//          shapes stop / turn, Q / E down / up (flying). F9 (only with [debug] device_reset_key = 1) throws the GPU
//          objects away and builds them again: the test of the recovery.
// Switch:  a switch that would put the body inside something, or under the water of the other moment, is refused: the
// ripple
//          opens a little and turns back (scene/LayerGuard.h), and the old moment stays solid meanwhile.
// Camera:  third person by default ([player] third_person): the camera orbits behind the body and is pulled in by the
// walls
//          (town::CameraClearDistance). A hard landing lowers the eye for a moment ([camera] land_dip).
// Overlay: drawn last (render/Hud.h): a dot, the one prompt of the key that does something now, a status line.
// Lost device: Present reports it, the loop builds device, triangle and models again (Gpu::Recover); the moment, the
// spin, the
//          town and the camera stay as they were. After kMaxRecoveries tries the program says so and ends.
// Args:    --adapter N   choose the N-th adapter listed in the log (overrides [render] adapter)

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include "core/Config.h"
#include "core/Log.h"
#include "core/Window.h"
#include "render/Device.h"
#include "render/GpuDebug.h"
#include "render/Hud.h"
#include "render/Models.h"
#include "render/Overlay.h"
#include "render/OverlayBatch.h"
#include "render/Projection.h"
#include "render/Triangle.h"
#include "scene/Camera.h"
#include "scene/LayerGuard.h"
#include "scene/Lantern.h"
#include "scene/Player.h"
#include "scene/Prompts.h"
#include "scene/SceneData.h"
#include "scene/Things.h"
#include "scene/Town.h"
#include "scene/TownEffects.h"
#include "scene/TownPlayer.h"

#include <DirectXMath.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

// WHY these two exported variables: on a laptop with two GPUs they ask the NVIDIA / AMD driver to run this program on
// the fast GPU. Device::Init also chooses the adapter itself (the log lists them); this is a second safety. They must
// be real globals with exactly these names.
extern "C"
{
    __declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
    __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}

namespace
{

constexpr char kConfigPath[] = "config/default.ini";
constexpr int kMaxRecoveries = 5; // after this many lost devices in one run the program gives up
constexpr float kDefaultBodyRadius =
    0.35f; // UNITS: metres, the ball of the camera. TUNE: [collision] radius, 0.05 .. 1
constexpr float kDefaultMouseSensitivity =
    0.0022f; // UNITS: radians per count of the mouse. TUNE: [mouse] sensitivity, 0.0002 .. 0.02
constexpr float kDefaultLandDip =
    0.012f; // UNITS: metres of dip per m/s of landing speed. TUNE: [camera] land_dip, 0 = off
constexpr float kDefaultLandDipMax = 0.15f; // UNITS: metres, the deepest dip. TUNE: [camera] land_dip_max
constexpr float kLandDipReturn = 14.0f;     // UNITS: 1 / seconds; the eye comes back up in about 0.2 s
constexpr float kEyeEase = 30.0f;   // UNITS: 1 / seconds; small changes of the eye height are eased over about 0.05 s
constexpr float kEyeEaseMax = 0.5f; // UNITS: metres; a larger change (a respawn, a teleport) is followed at once
constexpr float kSeaNorthEdge = -60.0f; // UNITS: metres (z): the sea ends here to the north, the town stands beyond it
static_assert(dl::kFarPlane <= dl::town::kViewDistance,
              "scene/TownEffects.h kViewDistance must reach the far plane: the sea is sized by it");

dl::log::Level ParseLevel(const std::string& text)
{
    if (text == "trace")
    {
        return dl::log::Level::Trace;
    }
    if (text == "warn")
    {
        return dl::log::Level::Warn;
    }
    if (text == "error")
    {
        return dl::log::Level::Error;
    }
    return dl::log::Level::Info;
}

void LoadConfig(dl::Config& config, dl::Camera& camera)
{
    const dl::Config::ParseResult result = config.LoadFile(kConfigPath);
    for (const std::string& error : result.errors)
    {
        DL_LOG_WARN("Config", "%s: %s", kConfigPath, error.c_str());
    }
    DL_LOG_INFO("Config", "%s: %d values", kConfigPath, result.entries);
    dl::log::SetMinLevel(ParseLevel(config.GetString("log", "level", "info")));

    // The camera speeds come from the config file: change them, press F5, no recompiling.
    dl::Camera::Settings settings; // the defaults live in Camera.h
    settings.moveSpeed = config.GetFloat("camera", "move_speed", settings.moveSpeed);
    settings.turnSpeed = config.GetFloat("camera", "turn_speed", settings.turnSpeed);
    camera.SetSettings(settings);
}

/// The walking body: its numbers come from [player] and [collision]; Player pulls nonsense values into a range that
/// works.
void ApplyPlayerConfig(const dl::Config& config, dl::Player& player)
{
    dl::Player::Settings settings; // the defaults live in Player.h
    settings.radius = config.GetFloat("collision", "radius", kDefaultBodyRadius);
    settings.eyeHeight = config.GetFloat("player", "eye_height", settings.eyeHeight);
    settings.sprintMultiplier = config.GetFloat("player", "sprint_multiplier", settings.sprintMultiplier);
    settings.walkSpeed = config.GetFloat("player", "walk_speed", settings.walkSpeed);
    settings.groundAccel = config.GetFloat("player", "ground_accel", settings.groundAccel);
    settings.groundDecel = config.GetFloat("player", "ground_decel", settings.groundDecel);
    settings.airAccel = config.GetFloat("player", "air_accel", settings.airAccel);
    settings.gravity = config.GetFloat("player", "gravity", settings.gravity);
    settings.fallGravityScale = config.GetFloat("player", "fall_gravity_scale", settings.fallGravityScale);
    settings.jumpHeight = config.GetFloat("player", "jump_height", settings.jumpHeight);
    settings.jumpCut = config.GetFloat("player", "jump_cut", settings.jumpCut);
    player.SetSettings(settings);
}

int ParseAdapterArg(int argc, char** argv)
{
    for (int i = 1; i + 1 < argc; ++i)
    {
        if (std::strcmp(argv[i], "--adapter") == 0)
        {
            return std::atoi(argv[i + 1]);
        }
    }
    return -2; // not given
}

/// True while the key is held down right now.
bool IsDown(int virtualKey)
{
    return (GetAsyncKeyState(virtualKey) & 0x8000) != 0;
}

/// True once per press of a key (a key held down counts once). `wasDown` remembers the last frame.
bool PressedOnce(int virtualKey, bool& wasDown, bool focused)
{
    const bool down = focused && IsDown(virtualKey);
    const bool pressed = down && !wasDown;
    wasDown = down;
    return pressed;
}

/// Reads the keyboard: W/S forward/back, D/A right/left, E/Q up/down, arrow keys turn. Each value is -1, 0 or +1.
dl::CameraInput ReadCameraKeys()
{
    dl::CameraInput input;
    input.moveForward = (IsDown('W') ? 1.0f : 0.0f) - (IsDown('S') ? 1.0f : 0.0f);
    input.moveRight = (IsDown('D') ? 1.0f : 0.0f) - (IsDown('A') ? 1.0f : 0.0f);
    input.moveUp = (IsDown('E') ? 1.0f : 0.0f) - (IsDown('Q') ? 1.0f : 0.0f);
    input.lookRight = (IsDown(VK_RIGHT) ? 1.0f : 0.0f) - (IsDown(VK_LEFT) ? 1.0f : 0.0f);
    input.lookUp = (IsDown(VK_UP) ? 1.0f : 0.0f) - (IsDown(VK_DOWN) ? 1.0f : 0.0f);
    return input;
}

/// The looks of the two moments from the lights of the model (tools/layout/city_model.py LIGHT): its colours are sRGB,
/// the renderer works in linear light.
void ApplyTownThemes(dl::Models& models, const dl::town::TownData& town)
{
    const auto lin = [](const float c[3])
    {
        return DirectX::XMFLOAT3(dl::batch_detail::SrgbToLinear(c[0]), dl::batch_detail::SrgbToLinear(c[1]),
                                 dl::batch_detail::SrgbToLinear(c[2]));
    };
    dl::Models::Theme themes[2];
    for (int i = 0; i < 2; ++i)
    {
        const dl::town::Light& l = town.lights[i];
        dl::Models::Theme& t = themes[i];
        t.toLight = DirectX::XMFLOAT3(l.toLight[0], l.toLight[1], l.toLight[2]);
        t.lightColor = lin(l.color);
        t.sky = lin(l.shade);
        t.clear = lin(l.horizon);
        t.fogDensity = l.fog;
        t.gradientSky = true;
        // The procedural sky is deliberately less saturated than the model preview.
        const float aZenith[3] = {0.51f, 0.59f, 0.72f};
        t.zenith = lin(i == 0 ? aZenith : l.skyTop);
        const float warmBounce[3] = {0.29f, 0.22f, 0.18f};
        const float coolBounce[3] = {0.17f, 0.22f, 0.27f};
        t.groundFill = lin(i == 0 ? warmBounce : coolBounce);
        t.mistBase = town.sea + 2.0f;
        t.mistDensity = i == 0 ? 0.0018f : 0.0042f;
        t.tint = DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f); // the town has its own colours: no stone tint
        t.weathering = 0.0f;
    }
    models.SetThemes(themes[0], themes[1]);
}

/// Everything that lives on the GPU. A lost device takes all of it, so all of it is made again together (Recover).
struct Gpu
{
    explicit Gpu(const dl::town::TownData& townData) : town(townData)
    {
    }

    std::unique_ptr<dl::Device> device = std::make_unique<dl::Device>();
    dl::Triangle triangle;
    dl::Models models;
    dl::Overlay overlay;
    const dl::town::TownData& town; // BORROWS: the town outlives the GPU objects

    bool Init(const dl::Window& window, const dl::Config& config, int adapter)
    {
        if (!device->Init(window.Handle(), window.Width(), window.Height(), adapter,
                          config.GetBool("render", "vsync", true)) ||
            !triangle.Init(device->GetDevice()) ||
            !models.Init(device->GetDevice(), config.GetInt("models", "segments", 32)) ||
            !overlay.Init(device->GetDevice()))
        {
            return false;
        }
        models.LoadThemes(config); // the looks of the layers: built in, [layer_a] / [layer_b] in the config change them
        // the town (scene/Town.h): its own looks, its polygons baked into the static batch of each layer, its still
        // water into the water pass
        ApplyTownThemes(models, town);
        models.SetStaticExtra(town.BakeLayer(0), town.BakeLayer(1));
        models.SetStaticWater(town.BakeWater(0), town.BakeWater(1));
        models.SetStatic(nullptr, 0);
        return true;
    }

    /// WHY this order: every object must be released before Device::Shutdown reports leaks.
    void Shutdown()
    {
        overlay.Shutdown();
        models.Shutdown();
        triangle.Shutdown();
        device->Shutdown();
    }

    /// A new device, made from nothing. What the models remember (layer, ripple, spin, wireframe) is kept; the level of
    /// detail goes back to [models] segments.
    bool Recover(const dl::Window& window, const dl::Config& config, int adapter)
    {
        Shutdown();
        device = std::make_unique<dl::Device>(); // a new object: a Device that was lost is never reused
        return Init(window, config, adapter);
    }
};

/// Where and when the body came back to the last place it stood still on (the ring on the floor).
struct Respawn
{
    dl::Vec3 at{};
    float since = -1.0f; ///< UNITS: seconds since the last respawn; negative before the first
};

/// What E does when it is pressed on the ground: a thing of the past within reach first (it opens in A only, the causal
/// rule; its trace comes up in B), otherwise the lantern (called back from afar, set down ahead in A, or taken up).
/// Every outcome goes to the log; a refusal never changes the state.
void HandleUse(const dl::town::TownData& town, dl::town::Things& things, dl::town::Traces& traces,
               dl::town::Lantern& lantern, dl::town::Moments& moments, dl::town::Moments& plain,
               const dl::Player& player, const dl::Camera& camera, int solidLayer)
{
    bool used = false;
    const int which = things.InReach(player.Feet());
    if (which >= 0)
    {
        using TR = dl::town::Things::Result;
        const dl::Vec3 balls[3] = {player.Ball(0), player.Ball(1), player.Ball(2)};
        const char* name = town.things[static_cast<std::size_t>(which)].name.c_str();
        switch (things.Use(player.Feet(), balls, 3, player.GetSettings().radius, solidLayer, moments, traces))
        {
        case TR::Opened:
            things.ApplyTo(plain);
            used = true;
            DL_LOG_INFO("Stage", "%s: done in the past (bells %d of %d)", name, things.BellsRung(), things.BellCount());
            break;
        case TR::OnlyInPast:
            used = true;
            DL_LOG_INFO("Stage", "%s: nothing done in the present changes it", name);
            break;
        case TR::Refused:
            used = true;
            DL_LOG_INFO("Stage", "%s not opened: it would close on the body", name);
            break;
        case TR::AlreadyOpen:
        case TR::Nothing:
            break;
        }
    }
    if (used)
    {
        return;
    }
    // the lantern: E sets it down ahead in A, or takes it up within reach (refused when that would leave the body in
    // something or without its floor)
    if (lantern.Far(player.Feet()))
    {
        const auto r =
            lantern.Recall(player.Feet(), player.Grounded(), solidLayer, player.GetSettings().radius, moments);
        DL_LOG_INFO("Stage", "%s",
                    r == dl::town::Lantern::Result::Taken
                        ? "the lantern called back"
                        : "the lantern not called back: the body stands on its circle");
        return;
    }
    using LR = dl::town::Lantern::Result;
    switch (lantern.Use(player.Feet(), player.Grounded(), camera.Forward(), solidLayer, player.GetSettings().radius,
                        moments))
    {
    case LR::Put:
        DL_LOG_INFO("Stage", "lantern set down at %.1f %.1f %.1f: a circle of the past", lantern.At().x, lantern.At().y,
                    lantern.At().z);
        break;
    case LR::Taken:
        DL_LOG_INFO("Stage", "lantern taken up");
        break;
    case LR::OnlyInPast:
        DL_LOG_INFO("Stage", "the lantern is set down in the past only");
        break;
    case LR::NoFloor:
        DL_LOG_INFO("Stage", "no floor of the past to set the lantern on");
        break;
    case LR::RefusedTake:
        DL_LOG_INFO("Stage", "lantern not taken: the body stands on what it keeps");
        break;
    case LR::Nothing:
        break;
    }
}

/// The prompt of the town: the state of the game read into scene/Prompts.h. `active`: the body walks and is not
/// sinking. `layer`: the moment on the screen (0 = A).
dl::prompt::Shown ChoosePrompt(const dl::town::TownData& town, const dl::town::Things& things,
                               const dl::town::Lantern& lantern, const dl::Player& player, int layer, bool active)
{
    dl::prompt::Input in;
    in.active = active;
    in.layer = layer;
    const int handle = things.InReach(player.Feet());
    if (handle >= 0 && !things.Open(static_cast<std::size_t>(handle)))
    {
        in.shutThingVerb = static_cast<int>(town.things[static_cast<std::size_t>(handle)].verb);
    }
    in.lanternFar = lantern.Far(player.Feet());
    in.lanternInReach = lantern.InReach(player.Feet());
    in.lanternCarried = lantern.Carried();
    in.grounded = player.Grounded();
    return dl::prompt::Choose(in);
}

/// The title bar: adapter, size, speed, where the body is, the mode and the progress. tools/check.ps1 reads it, so the
/// words "layer", "walk", "fly", "noclip" and "quay" are part of the contract.
void SetTitleBar(dl::Window& window, const Gpu& gpu, const dl::town::TownData& town, const dl::town::Things& things,
                 bool collide, bool walk, const dl::Vec3& body, double fps, double milliseconds, int falls)
{
    const wchar_t* mode = collide ? (walk ? L"collide | walk" : L"collide | fly") : L"noclip";
    wchar_t title[384];
    swprintf_s(title,
               L"DualLayer | %s | %d x %d | %.1f FPS (%.2f ms) | body %.1f %.1f %.1f | %s | %s | town: %hs | bells %d "
               L"of %d | falls %d%s",
               gpu.device->AdapterName().c_str(), gpu.device->Width(), gpu.device->Height(), fps, milliseconds, body.x,
               body.y, body.z, gpu.models.StatusLine().c_str(), mode, town.NearestPlace(body.x, body.z),
               things.BellsRung(), things.BellCount(), falls,
               window.MouseCaptured() ? L"" : L" | click for mouse look");
    window.SetTitle(title);
}

/// True when the body fits in `layer` of the town (with the circle of the lantern): none of its three balls overlaps a
/// surface (a hair less than its radius, so that standing on a floor fits) and its feet are not under the water of that
/// layer.
bool FitsInTown(const dl::Player& player, const dl::town::Moments& moments, const dl::town::TownData& town, int layer)
{
    const float r = player.GetSettings().radius;
    for (int i = 0; i < 3; ++i)
    {
        dl::Vec3 c = player.Ball(i);
        if (moments.PushOut(c, r - 0.03f, layer) > 0.005f)
        {
            return false;
        }
    }
    return !town.Submerged(player.Feet(), layer, moments.GetCircle());
}

/// The town of the model, walked in both moments, with the things that change (scene/Things.h). One loop: it reads the
/// keys, switches the moment through the guard, walks, opens, falls, sinks and draws. A fall or the water brings the
/// body back to the last place it stood still on; R does the same. Returns the exit code.
int RunTown(dl::Window& window, Gpu& gpu, dl::Config& config, dl::Camera& camera, int adapter,
            const dl::town::TownData& town)
{
    const std::array<dl::town::TriMesh, 2> meshes = {town.BuildMesh(0), town.BuildMesh(1)};
    DL_LOG_INFO("Main", "town solid: %d triangles in A, %d in B", meshes[0].TriangleCount(), meshes[1].TriangleCount());
    dl::Player player;
    ApplyPlayerConfig(config, player);
    player.SetScene(nullptr, 0);
    dl::town::Moments moments(&meshes[0], &meshes[1]); // the solid of both moments, with the circle of the lantern
    dl::town::Moments plain(&meshes[0], &meshes[1]);   // the same without it: where a fall may come back to
    dl::town::Lantern lantern;                         // carried, or set down in A (scene/Lantern.h)
    dl::town::Things things; // the things that change: opened in A, the trace in B (scene/Things.h)
    dl::town::Traces traces;
    things.Init(town);
    things.ApplyTo(moments);
    things.ApplyTo(plain);
    player.SetTown(&moments);
    player.SetSpawn(town.spawn);
    player.Respawn();
    camera.SetPose(player.Eye(), town.spawnYaw, config.GetBool("player", "third_person", true) ? -0.18f : 0.0f);
    if (town.startLayer == 1)
    {
        gpu.models
            .ToggleLayer(); // the game opens in the present (docs/DESIGN.md 3.4): the first ripple goes from A to B
    }
    dl::LayerGuard guard;
    constexpr int kDrawCapacity = 128;
    static dl::scene::SceneObject drawTable[kDrawCapacity] = {};
    static dl::OverlayBatch hud; // WHY static: 288 KB, too large for the stack
    Respawn respawn;
    dl::Vec3 safe = town.spawn; // the last place the body stood still on: where a fall comes back to
    float stood = 0.0f;         // UNITS: seconds the body has stood on the ground without a break
    float sinking = -1.0f;      // UNITS: seconds left of sinking in water before it comes back; negative: not sinking
    int falls = 0;

    LARGE_INTEGER frequency{};
    LARGE_INTEGER last{};
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&last);
    double fpsTimer = 0.0;
    int fpsFrames = 0;
    float landDip = 0.0f;
    float eyeY = player.Eye().y;
    bool keyDown[9] = {}; // Z, X, C, N, F, Space, Tab, R, E
    bool resetTestKeyWasDown = false;
    bool collide = config.GetBool("collision", "enabled", true);
    bool walk = config.GetBool("player", "walk", true);
    bool wasWalking = walk && collide;
    float avatarYaw = town.spawnYaw;
    float gaitPhase = 0.0f;
    float gaitAmount = 0.0f;
    float boom = 0.0f;
    int recoveries = 0;

    while (window.PumpMessages())
    {
        const bool focused = GetForegroundWindow() == window.Handle();
        const bool resetTest =
            config.GetBool("debug", "device_reset_key", false) && PressedOnce(VK_F9, resetTestKeyWasDown, focused);
        if (gpu.device->Lost() || resetTest)
        {
            DL_LOG_WARN("Main", "%s",
                        resetTest ? "F9: the GPU objects are made again (test)"
                                  : "the device is lost: making it again");
            ++recoveries;
            if (recoveries > kMaxRecoveries || !gpu.Recover(window, config, adapter))
            {
                DL_LOG_ERROR("Main", "the device could not be made again (%d tries)", recoveries);
                MessageBoxA(window.Handle(),
                            "The graphics device was lost and could not be recovered. See logs/dual.log", "DualLayer",
                            MB_OK | MB_ICONERROR);
                return 1;
            }
            DL_LOG_INFO("Main", "recovered (%d of at most %d)", recoveries, kMaxRecoveries);
            QueryPerformanceCounter(&last);
            continue;
        }
        if (window.ConsumeReloadKey())
        {
            LoadConfig(config, camera);
            gpu.models.LoadThemes(config);
            ApplyTownThemes(gpu.models, town);
            gpu.device->SetVsync(config.GetBool("render", "vsync", true));
            ApplyPlayerConfig(config, player);
            collide = config.GetBool("collision", "enabled", true);
            walk = config.GetBool("player", "walk", true);
        }
        if (window.ConsumeResized())
        {
            gpu.device->Resize(window.Width(), window.Height());
        }
        if (window.IsMinimized())
        {
            Sleep(50);
            continue;
        }
        LARGE_INTEGER now{};
        QueryPerformanceCounter(&now);
        const double dt = static_cast<double>(now.QuadPart - last.QuadPart) / static_cast<double>(frequency.QuadPart);
        last = now;
        const float dtf = static_cast<float>(dt);

        long mouseDx = 0;
        long mouseDy = 0;
        window.ConsumeMouseDelta(mouseDx, mouseDy);
        if (mouseDx != 0 || mouseDy != 0)
        {
            const float sensitivity =
                std::clamp(config.GetFloat("mouse", "sensitivity", kDefaultMouseSensitivity), 0.0002f, 0.02f);
            const float invert = config.GetBool("mouse", "invert_y", false) ? -1.0f : 1.0f;
            camera.SetPose(camera.Position(), camera.Yaw() + static_cast<float>(mouseDx) * sensitivity,
                           camera.Pitch() - invert * static_cast<float>(mouseDy) * sensitivity);
        }

        // ---- keys
        const dl::CameraInput keys = focused ? ReadCameraKeys() : dl::CameraInput{};
        if (PressedOnce('Z', keyDown[0], focused))
        {
            gpu.models.ToggleWireframe();
        }
        if (PressedOnce('X', keyDown[1], focused))
        {
            gpu.models.CycleDetail();
        }
        if (PressedOnce('C', keyDown[2], focused))
        {
            gpu.models.ToggleSpin();
        }
        if (PressedOnce('N', keyDown[3], focused))
        {
            collide = !collide;
            DL_LOG_INFO("Main", "collision %s", collide ? "on" : "off");
        }
        if (PressedOnce('F', keyDown[4], focused))
        {
            walk = !walk;
            DL_LOG_INFO("Main", "movement: %s", walk ? "walk" : "fly");
        }
        const bool jumpPressed = PressedOnce(VK_SPACE, keyDown[5], focused);
        const bool usePressed = PressedOnce('E', keyDown[8], focused); // walking: set the lantern down or take it up
        const bool walking = walk && collide;
        const bool thirdPerson = config.GetBool("player", "third_person", true);
        const bool orbit = walking && thirdPerson;

        // ---- the switch, through the guard: refused into a wall or into the water of the other moment
        if (PressedOnce(VK_TAB, keyDown[6], focused && !IsDown(VK_MENU) && sinking < 0.0f))
        {
            const int solidNow = guard.Refusing() ? guard.HeldLayer() : gpu.models.CollisionLayer();
            const int target = 1 - gpu.models.CollisionLayer();
            const bool fits = !walking || FitsInTown(player, moments, town, target);
            switch (guard.OnSwitchRequest(solidNow, gpu.models.LayerSwitching(), fits))
            {
            case dl::LayerGuard::Action::Switch:
                gpu.models.ToggleLayer();
                break;
            case dl::LayerGuard::Action::Refuse:
                gpu.models.ToggleLayer();
                DL_LOG_INFO("Main", "switch refused: the body would be inside something in layer %s",
                            target == 0 ? "A" : "B");
                break;
            case dl::LayerGuard::Action::Ignore:
                break;
            }
        }
        if (guard.Update(dtf))
        {
            gpu.models.ToggleLayer();
        }

        // ---- the camera
        if (walking && !wasWalking)
        {
            // Debug fly is not a shortcut through the town in the orbit prototype.
            if (thirdPerson)
            {
                boom = 0.0f;
            }
            else
            {
                player.Place(camera.Position());
            }
        }
        wasWalking = walking;
        if (walking)
        {
            dl::CameraInput lookOnly;
            lookOnly.lookRight = keys.lookRight;
            lookOnly.lookUp = keys.lookUp;
            camera.Update(lookOnly, dtf);
        }
        else
        {
            camera.Update(keys, dtf);
        }
        gpu.models.Update(dtf);
        const int solidLayer = guard.Refusing() ? guard.HeldLayer() : gpu.models.CollisionLayer();

        const bool resetPressed = PressedOnce('R', keyDown[7], focused);
        const dl::Vec3 feetBeforeMove = player.Feet();
        if (walking)
        {
            dl::Player::Input move;
            move.moveForward = sinking >= 0.0f ? 0.0f : keys.moveForward;
            move.moveRight = sinking >= 0.0f ? 0.0f : keys.moveRight;
            move.jumpPressed = jumpPressed && sinking < 0.0f;
            move.jumpHeld = focused && IsDown(VK_SPACE);
            move.sprint = focused && IsDown(VK_SHIFT) && sinking < 0.0f && config.GetBool("player", "sprint", true);
            player.Update(move, camera.Forward(), solidLayer, gpu.models.SpinSeconds(), dtf);
            const dl::Vec3 wish = player.WishDirection();
            if (wish.x * wish.x + wish.z * wish.z > 0.5f)
            {
                const float targetYaw = std::atan2(wish.x, wish.z);
                const float error = std::remainder(targetYaw - avatarYaw, 6.2831853f);
                avatarYaw += error * (1.0f - std::exp(-22.0f * dtf));
            }
            const float climb = std::clamp(config.GetFloat("player", "mantle_height", 2.2f), 0.0f, 2.2f);
            if (move.jumpHeld && sinking < 0.0f && config.GetBool("player", "mantle", true) &&
                dl::town::TryTownMantle(player, moments, town, solidLayer, climb))
            {
                DL_LOG_INFO("Player", "mantle: ledge reached");
            }

            if (usePressed && sinking < 0.0f)
            {
                HandleUse(town, things, traces, lantern, moments, plain, player, camera, solidLayer);
            }

            // into the water: it sinks for a moment, then comes back; below the sea or the town: a fall
            if (sinking < 0.0f && town.Submerged(player.Feet(), solidLayer, moments.GetCircle()))
            {
                sinking = dl::town::kSinkSeconds;
                DL_LOG_INFO("Main", "into the water");
            }
            bool back = resetPressed;
            if (sinking >= 0.0f)
            {
                sinking -= dtf;
                back = back || sinking < 0.0f;
            }
            back = back || player.Feet().y < town.sea - 3.0f || player.JustRespawned();
            if (back)
            {
                sinking = -1.0f;
                player.SetSpawn(safe);
                player.Respawn();
                respawn = Respawn{player.Feet(), 0.0f};
                landDip = 0.0f;
                boom = 0.0f;
                gaitAmount = 0.0f;
                gaitPhase = 0.0f;
                stood = 0.0f;
                if (!resetPressed)
                {
                    ++falls;
                    DL_LOG_INFO("Main", "fell: back at the %s", town.NearestPlace(safe.x, safe.z));
                }
            }
            // the last place it stood still on, out of the water, standing in both moments without the circle: a return
            // there never ends in a wall or in the air, whatever is switched and wherever the lantern is
            stood = player.Grounded() && sinking < 0.0f ? stood + dtf : 0.0f;
            const float radius = player.GetSettings().radius;
            if (stood > 0.5f && FitsInTown(player, plain, town, 0) && FitsInTown(player, plain, town, 1) &&
                plain.Supports(player.Feet(), radius, 0) && plain.Supports(player.Feet(), radius, 1))
            {
                safe = player.Feet();
            }
            float peak = 0.0f;
            if (player.ConsumeLanding(peak))
            {
                DL_LOG_INFO("Player", "jump peak %.2f m", peak);
            }
            float impact = 0.0f;
            if (player.ConsumeImpact(impact))
            {
                const float perSpeed = std::clamp(config.GetFloat("camera", "land_dip", kDefaultLandDip), 0.0f, 0.05f);
                const float deepest =
                    std::clamp(config.GetFloat("camera", "land_dip_max", kDefaultLandDipMax), 0.0f, 0.5f);
                landDip = std::max(landDip, std::min(impact * perSpeed, deepest));
            }
            landDip *= std::exp(-kLandDipReturn * dtf);
            dl::Vec3 eye = player.Eye();
            const float rise = eye.y - eyeY;
            eyeY = (player.Grounded() && std::fabs(rise) < kEyeEaseMax)
                       ? eyeY + rise * (1.0f - std::exp(-kEyeEase * dtf))
                       : eye.y;
            eye.y = eyeY - landDip;
            if (orbit)
            {
                const float pitch = std::clamp(camera.Pitch(), -0.90f, 0.38f);
                const float reach = std::clamp(config.GetFloat("camera", "orbit_distance", 3.8f), 2.0f, 6.0f);
                eye.y -= 0.20f;
                camera.SetPose(eye, camera.Yaw(), pitch);
                const dl::Vec3 direction = camera.Forward();
                const float clear = dl::town::CameraClearDistance(moments, eye, direction, reach, solidLayer,
                                                                  gpu.models.LayerSwitching());
                boom = (std::min)(clear, boom + 10.0f * dtf);
                camera.SetPose(dl::town::Along(eye, direction, -boom), camera.Yaw(), pitch);
            }
            else
            {
                camera.SetPose(eye, camera.Yaw(), camera.Pitch());
            }
        }
        else if (collide) // flying with collision: the camera is a ball pushed out of the town
        {
            dl::Vec3 c = camera.Position();
            if (moments.PushOut(c, std::clamp(config.GetFloat("collision", "radius", kDefaultBodyRadius), 0.05f, 1.0f),
                                solidLayer) > 0.0f)
            {
                camera.SetPose(c, camera.Yaw(), camera.Pitch());
            }
        }
        respawn.since += dtf;
        traces.Update(dtf);

        // ---- what is drawn: the town is in the static batch; here only the sea and the ring of a return
        int drawn = dl::town::AddSea(camera.Position(), drawTable, 0, kDrawCapacity, town.sea, kSeaNorthEdge);
        drawn = lantern.AddDraw(drawTable, drawn, kDrawCapacity);
        drawn = things.AddDraw(drawTable, drawn, kDrawCapacity, traces);
        const dl::town::Circle circle = moments.GetCircle();
        gpu.models.SetLantern(DirectX::XMFLOAT3(circle.centre.x, circle.centre.y, circle.centre.z), circle.radius);
        drawn = dl::town::AddRespawnRing(respawn.at, respawn.since, drawTable, drawn, kDrawCapacity);
        if (orbit)
        {
            const dl::Vec3 feet = player.Feet();
            const float travel = std::hypot(feet.x - feetBeforeMove.x, feet.z - feetBeforeMove.z);
            const float walkSpeed = player.GetSettings().walkSpeed;
            // Use actual travel rather than requested velocity, so pushing into a wall does not animate a run.
            const float moved = dtf > 0.0f && travel <= 2.0f * walkSpeed * dtf ? travel : 0.0f;
            const float targetGait =
                player.Grounded() && walkSpeed > 0.0f && dtf > 0.0f ? moved / (walkSpeed * dtf) : 0.0f;
            gaitAmount += (targetGait - gaitAmount) * (1.0f - std::exp(-18.0f * dtf));
            if (player.Grounded())
            {
                gaitPhase = std::remainder(gaitPhase + moved * 4.6f, 6.2831853f);
            }
            if (boom >= 0.85f)
            {
                drawn = dl::town::AddTownAvatar(drawTable, drawn, kDrawCapacity, player, avatarYaw, gaitPhase,
                                                gaitAmount, lantern.Carried());
            }
        }
        gpu.models.SetObjects(drawTable, drawn);

        const dl::Vec3 pos = camera.Position();
        const dl::Vec3 body = walking ? player.Eye() : pos;
        const dl::Vec3 dir = camera.Forward();
        const DirectX::XMMATRIX view = DirectX::XMMatrixLookToLH(DirectX::XMVectorSet(pos.x, pos.y, pos.z, 1.0f),
                                                                 DirectX::XMVectorSet(dir.x, dir.y, dir.z, 0.0f),
                                                                 DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
        float clear[4] = {};
        gpu.models.ClearColor(clear);
        {
            dl::GpuEvent frame(gpu.device->GetAnnotation(), L"Frame");
            gpu.device->BeginFrame(clear);
            const float aspect = static_cast<float>(gpu.device->Width()) / static_cast<float>(gpu.device->Height());
            {
                dl::GpuEvent pass(gpu.device->GetAnnotation(), L"Opaque.Models");
                gpu.models.Draw(
                    gpu.device->GetContext(), view,
                    dl::MakeProjection(aspect, config.GetFloat("camera", "fov_deg", dl::kDefaultFovYDegrees)),
                    DirectX::XMFLOAT3(pos.x, pos.y, pos.z));
            }
            {
                char status[160];
                std::snprintf(status, sizeof(status), "%s", town.NearestPlace(body.x, body.z));
                const dl::prompt::Shown prompt =
                    ChoosePrompt(town, things, lantern, player, gpu.models.CollisionLayer(), walking && sinking < 0.0f);
                const float under = sinking >= 0.0f ? 1.0f : 0.0f;
                dl::BuildHud(hud, gpu.device->Width(), gpu.device->Height(), prompt, status, under, !orbit);
                dl::GpuEvent pass(gpu.device->GetAnnotation(), L"Overlay");
                gpu.overlay.Draw(gpu.device->GetContext(), hud, gpu.device->Width(), gpu.device->Height());
            }
        }
        gpu.device->EndFrame();

        fpsTimer += dt;
        ++fpsFrames;
        if (fpsTimer >= 0.5)
        {
            SetTitleBar(window, gpu, town, things, collide, walk, body, fpsFrames / fpsTimer,
                        1000.0 * fpsTimer / fpsFrames, falls);
            fpsTimer = 0.0;
            fpsFrames = 0;
        }
    }
    return 0;
}

int RunApp(HINSTANCE instance, int argc, char** argv)
{
    CreateDirectoryA("logs", nullptr);
    dl::log::OpenFile("logs/dual.log");
    DL_LOG_INFO("Main", "start");

    dl::Config config;
    dl::Camera camera;
    LoadConfig(config, camera);

    dl::Window window;
    if (!window.Create(instance, L"DualLayer", config.GetInt("window", "width", 1600),
                       config.GetInt("window", "height", 900)))
    {
        return 1;
    }

    int adapter = config.GetInt("render", "adapter", -1);
    const int adapterArg = ParseAdapterArg(argc, argv);
    if (adapterArg != -2)
    {
        adapter = adapterArg;
    }

    static dl::town::TownData town; // WHY static: a few MB, kept for the whole run
    std::string error;
    if (!town.Load(dl::town::kPath, error))
    {
        DL_LOG_ERROR("Main", "the town could not be loaded: %s", error.c_str());
        MessageBoxA(window.Handle(), "The town could not be loaded. See logs/dual.log", "DualLayer",
                    MB_OK | MB_ICONERROR);
        return 1;
    }
    DL_LOG_INFO("Main", "town: %zu polygons, %zu materials, %zu places", town.polygons.size(), town.materials.size(),
                town.places.size());

    Gpu gpu(town);
    if (!gpu.Init(window, config, adapter))
    {
        MessageBoxA(window.Handle(), "Initialisation failed. See logs/dual.log", "DualLayer", MB_OK | MB_ICONERROR);
        gpu.Shutdown();
        return 1;
    }
    const int code = RunTown(window, gpu, config, camera, adapter, town);
    DL_LOG_INFO("Main", "shutdown");
    gpu.Shutdown(); // models, triangle, then the device (which reports whatever is still alive as a leak)
    dl::log::Close();
    return code;
}

} // namespace

int WINAPI WinMain(HINSTANCE instance, HINSTANCE, LPSTR, int)
{
    return RunApp(instance, __argc, __argv);
}
