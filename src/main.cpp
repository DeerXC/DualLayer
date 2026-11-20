// main.cpp - entry point and the frame loop.
// Frame:  PumpMessages > (F5: reload config) > (resize) > Update (camera, models, the stage, walking or collision) > Render
//         [GPU event "Frame" > "Opaque.Models" (> "Opaque.TestTriangle", when [test_triangle] show = 1)] > Present
// Stage:  scene/Stage.h, the first stage: the fixed objects, the purple block, the plate and the door (scene/Puzzle.h). Every
//         frame the drawn table (Stage::BuildDrawTable) goes to the renderer and the solid table of the layer of the body
//         (Puzzle::Solids) to the body, so what is drawn and what is solid come from the same state.
// Keys:   W A S D move, the mouse turns (click into the window to capture it; Esc gives it back, a second Esc quits), the arrow
//         keys turn as well, R goes back to the last checkpoint (and puts the block back), F5 reloads config/default.ini
//         Space jumps (walking; held = the full jump, a tap = a small hop), Q / E down / up (flying)
//         F walking / flying (walking: gravity, the body stands on the floor and on objects; flying: a free camera)
//         Z wireframe on / off, X the next level of detail (8, 16, 32, 64 segments), C the shapes stop / turn
//         Tab switches the layer (A <-> B). A switch that would put the body inside something is refused: the ripple opens
//         a little and turns back (Puzzle::BodyFits, LayerGuard), and the old layer stays solid meanwhile.
//         N collision on / off (off = fly through everything; walking needs collision, so N off also means flying)
//         F9 (only with [debug] device_reset_key = 1) throws the GPU objects away and builds them again: the test of the recovery
// Walking: scene/Player.h. The camera is the eye of a body of three balls; the camera only turns, the body walks, falls and jumps.
//         A hard landing lowers the eye for a moment ([camera] land_dip). A body that falls below the stage goes back to the
//         last checkpoint it stood on.
// Flying: the camera is one ball ([collision] radius); after it has moved, the ball is pushed out of the solid objects of the layer
//         that the screen is going to show (scene/Collision.h); the camera keeps its direction.
// Lost device: Present reports it, the loop builds device, triangle and models again (Gpu::Recover); the layer, the spin, the
//         stage and the camera stay as they were. After kMaxRecoveries tries the program says so and ends.
// Args:   --adapter N   choose the N-th adapter listed in the log (overrides [render] adapter)

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include "core/Config.h"
#include "core/Log.h"
#include "core/Window.h"
#include "render/Device.h"
#include "render/GpuDebug.h"
#include "render/Models.h"
#include "render/Projection.h"
#include "render/Triangle.h"
#include "scene/Camera.h"
#include "scene/Collision.h"
#include "scene/Player.h"
#include "scene/Puzzle.h"
#include "scene/SceneData.h"
#include "scene/Stage.h"
#include "scene/StageHints.h"
#include "scene/StageLights.h"

#include <DirectXMath.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>

// WHY these two exported variables: on a laptop with two GPUs they ask the NVIDIA / AMD driver to run this program on the fast GPU.
// Device::Init also chooses the adapter itself (the log lists them); this is a second safety. They must be real globals with exactly these names.
extern "C"
{
__declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}

namespace
{

constexpr char kConfigPath[] = "config/default.ini";
constexpr int kMaxRecoveries = 5; // after this many lost devices in one run the program gives up
constexpr float kDefaultBodyRadius = 0.35f; // UNITS: metres, the ball of the camera. TUNE: [collision] radius, 0.05 .. 1
constexpr float kDefaultMouseSensitivity = 0.0022f; // UNITS: radians per count of the mouse. TUNE: [mouse] sensitivity, 0.0002 .. 0.02
constexpr float kDefaultLandDip = 0.012f;           // UNITS: metres of dip per m/s of landing speed. TUNE: [camera] land_dip, 0 = off
constexpr float kDefaultLandDipMax = 0.15f;         // UNITS: metres, the deepest dip. TUNE: [camera] land_dip_max
constexpr float kLandDipReturn = 14.0f;             // UNITS: 1 / seconds; the eye comes back up in about 0.2 s

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

/// The walking body: its numbers come from [player] and [collision]; Player pulls nonsense values into a range that works.
void ApplyPlayerConfig(const dl::Config& config, dl::Player& player)
{
    dl::Player::Settings settings; // the defaults live in Player.h
    settings.radius = config.GetFloat("collision", "radius", kDefaultBodyRadius);
    settings.eyeHeight = config.GetFloat("player", "eye_height", settings.eyeHeight);
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

/// Everything that lives on the GPU. A lost device takes all of it, so all of it is made again together (Recover).
struct Gpu
{
    std::unique_ptr<dl::Device> device = std::make_unique<dl::Device>();
    dl::Triangle triangle;
    dl::Models models;

    bool Init(const dl::Window& window, const dl::Config& config, int adapter)
    {
        if (!device->Init(window.Handle(), window.Width(), window.Height(), adapter, config.GetBool("render", "vsync", true)) ||
            !triangle.Init(device->GetDevice()) || !models.Init(device->GetDevice(), config.GetInt("models", "segments", 32)))
        {
            return false;
        }
        models.LoadThemes(config); // the looks of the layers: built in, [layer_a] / [layer_b] in the config change them
        return true;
    }

    /// WHY this order: every object must be released before Device::Shutdown reports leaks.
    void Shutdown()
    {
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

int RunApp(HINSTANCE instance, int argc, char** argv)
{
    CreateDirectoryA("logs", nullptr);
    dl::log::OpenFile("logs/dual.log");
    DL_LOG_INFO("Main", "start");

    dl::Config config;
    dl::Camera camera;
    LoadConfig(config, camera);

    dl::Window window;
    if (!window.Create(instance, L"DualLayer", config.GetInt("window", "width", 1600), config.GetInt("window", "height", 900)))
    {
        return 1;
    }

    int adapter = config.GetInt("render", "adapter", -1);
    const int adapterArg = ParseAdapterArg(argc, argv);
    if (adapterArg != -2)
    {
        adapter = adapterArg;
    }

    Gpu gpu;
    if (!gpu.Init(window, config, adapter))
    {
        MessageBoxA(window.Handle(), "Initialisation failed. See logs/dual.log", "DualLayer", MB_OK | MB_ICONERROR);
        gpu.Shutdown();
        return 1;
    }

    // ---- the stage: its puzzle, the guard of the layer switch, the checkpoint the body has reached
    dl::puzzle::Puzzle puzzle;
    if (!puzzle.Init(dl::stage::MakeStage()))
    {
        DL_LOG_ERROR("Main", "the stage could not be set up (scene/Stage.h)");
        MessageBoxA(window.Handle(), "The stage could not be set up. See logs/dual.log", "DualLayer", MB_OK | MB_ICONERROR);
        gpu.Shutdown();
        return 1;
    }
    dl::puzzle::LayerGuard guard;
    dl::stage::LightSet lights; // the three hidden lights: once taken, taken for the rest of the run
    int checkpoint = 0;
    // WHY outside the loop: the renderer keeps a pointer to the drawn table until the next SetObjects, and the title bar and
    // the X key read it between two frames
    dl::scene::SceneObject drawTable[dl::stage::kMaxDrawObjects] = {};

    dl::Player player;
    ApplyPlayerConfig(config, player);
    player.SetSpawn(dl::stage::kCheckpoints[0].feet);
    player.Respawn();

    LARGE_INTEGER frequency{};
    LARGE_INTEGER last{};
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&last);
    double fpsTimer = 0.0;
    int fpsFrames = 0;
    float angle = 0.0f;           // radians
    float landDip = 0.0f;         // UNITS: metres the eye is lowered after a hard landing (it comes back up by itself)
    double stageSeconds = 0.0;    // UNITS: seconds since the start (or the last R at the first checkpoint)
    double clearSeconds = -1.0;   // the time of the clear; negative while the goal is not reached
    int blockMoves = 0;           // for the log: a move and a refused push are written once
    float lastFlash = 0.0f;
    bool resetKeyWasDown = false; // WHY: R must act once per press, not on every frame it is held
    bool wireKeyWasDown = false;
    bool detailKeyWasDown = false;
    bool spinKeyWasDown = false;
    bool layerKeyWasDown = false;
    bool collideKeyWasDown = false;
    bool walkKeyWasDown = false;
    bool jumpKeyWasDown = false;
    bool resetTestKeyWasDown = false;
    bool collide = config.GetBool("collision", "enabled", true); // N switches it; F5 sets it from the config again
    bool walk = config.GetBool("player", "walk", true);          // F switches it; F5 sets it from the config again
    bool wasWalking = walk && collide;                           // the body starts at the spawn point, so the first frame needs no Place
    int recoveries = 0;

    while (window.PumpMessages())
    {
        // keys count only while OUR window is in front (typing in another window must not move the camera)
        const bool focused = GetForegroundWindow() == window.Handle();

        // ---- a lost device (or the F9 test) is handled first: nothing else may touch the old objects
        const bool resetTest = config.GetBool("debug", "device_reset_key", false) && PressedOnce(VK_F9, resetTestKeyWasDown, focused);
        if (gpu.device->Lost() || resetTest)
        {
            DL_LOG_WARN("Main", "%s", resetTest ? "F9: the GPU objects are made again (test)" : "the device is lost: making it again");
            ++recoveries;
            if (recoveries > kMaxRecoveries || !gpu.Recover(window, config, adapter))
            {
                DL_LOG_ERROR("Main", "the device could not be made again (%d tries)", recoveries);
                MessageBoxA(window.Handle(), "The graphics device was lost and could not be recovered. See logs/dual.log", "DualLayer",
                            MB_OK | MB_ICONERROR);
                break;
            }
            DL_LOG_INFO("Main", "recovered (%d of at most %d)", recoveries, kMaxRecoveries);
            QueryPerformanceCounter(&last); // the time spent here must not become one huge frame
            continue;
        }

        if (window.ConsumeReloadKey())
        {
            LoadConfig(config, camera);
            gpu.models.LoadThemes(config);
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
            Sleep(50); // WHY: do not burn the GPU for a window nobody sees
            continue;
        }

        LARGE_INTEGER now{};
        QueryPerformanceCounter(&now);
        const double dt = static_cast<double>(now.QuadPart - last.QuadPart) / static_cast<double>(frequency.QuadPart);
        last = now;
        const float dtf = static_cast<float>(dt);
        angle += DirectX::XMConvertToRadians(config.GetFloat("test_triangle", "spin_deg_per_sec", 0.0f)) * dtf;
        if (clearSeconds < 0.0)
        {
            stageSeconds += dt;
        }

        // ---- the mouse turns the camera (only while it is captured: the window collects nothing otherwise)
        long mouseDx = 0;
        long mouseDy = 0;
        window.ConsumeMouseDelta(mouseDx, mouseDy);
        if (mouseDx != 0 || mouseDy != 0)
        {
            const float sensitivity = std::clamp(config.GetFloat("mouse", "sensitivity", kDefaultMouseSensitivity), 0.0002f, 0.02f);
            const float invert = config.GetBool("mouse", "invert_y", false) ? -1.0f : 1.0f;
            camera.SetPose(camera.Position(), camera.Yaw() + static_cast<float>(mouseDx) * sensitivity,
                           camera.Pitch() - invert * static_cast<float>(mouseDy) * sensitivity);
        }

        // ---- keys
        const dl::CameraInput keys = focused ? ReadCameraKeys() : dl::CameraInput{};
        if (PressedOnce('Z', wireKeyWasDown, focused))
        {
            gpu.models.ToggleWireframe();
        }
        if (PressedOnce('X', detailKeyWasDown, focused))
        {
            gpu.models.CycleDetail();
        }
        if (PressedOnce('C', spinKeyWasDown, focused))
        {
            gpu.models.ToggleSpin();
        }
        if (PressedOnce('N', collideKeyWasDown, focused))
        {
            collide = !collide;
            DL_LOG_INFO("Main", "collision %s", collide ? "on" : "off");
        }
        if (PressedOnce('F', walkKeyWasDown, focused))
        {
            walk = !walk;
            DL_LOG_INFO("Main", "movement: %s", walk ? "walk" : "fly");
        }
        const bool jumpPressed = PressedOnce(VK_SPACE, jumpKeyWasDown, focused);
        const bool walking = walk && collide; // a body that walks needs something to stand on

        // ---- the layer switch, through the guard. WHY not while Alt is down: Alt + Tab switches windows and must not also
        // switch the layer
        if (PressedOnce(VK_TAB, layerKeyWasDown, focused && !IsDown(VK_MENU)))
        {
            const int solidNow = guard.Refusing() ? guard.HeldLayer() : gpu.models.CollisionLayer();
            const int target = 1 - gpu.models.CollisionLayer();
            const bool fits = !walking || puzzle.BodyFits(player, target, gpu.models.SpinSeconds());
            switch (guard.OnSwitchRequest(solidNow, gpu.models.LayerSwitching(), fits))
            {
            case dl::puzzle::LayerGuard::Action::Switch:
                gpu.models.ToggleLayer();
                break;
            case dl::puzzle::LayerGuard::Action::Refuse:
                gpu.models.ToggleLayer(); // the ripple opens a little; the guard turns it back
                DL_LOG_INFO("Main", "switch refused: the body would be inside something in layer %s", target == 0 ? "A" : "B");
                break;
            case dl::puzzle::LayerGuard::Action::Ignore:
                break;
            }
        }
        if (guard.Update(dtf))
        {
            gpu.models.ToggleLayer(); // a refused ripple turns round and closes
        }

        // ---- the camera. Walking: it only turns, the body moves. Flying: it moves itself.
        if (walking && !wasWalking)
        {
            player.Place(camera.Position()); // walking starts again where the camera is: the body falls from there
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
        const bool resetKeyDown = focused && IsDown('R');
        if (resetKeyDown && !resetKeyWasDown)
        {
            landDip = 0.0f;
            puzzle.Reset(); // R also puts the block back: a block pushed into a corner never ends the game
            if (walking)
            {
                player.SetSpawn(dl::stage::kCheckpoints[checkpoint].feet);
                player.Respawn();
            }
            else
            {
                camera.Reset();
            }
            DL_LOG_INFO("Main", "R: back to checkpoint %d, the block is back at its start", checkpoint);
        }
        resetKeyWasDown = resetKeyDown;
        gpu.models.Update(dtf);

        // the layer whose objects are solid: the one the screen is going to show, or the old one while a refusal plays
        const int solidLayer = guard.Refusing() ? guard.HeldLayer() : gpu.models.CollisionLayer();
        int solidCount = 0;
        const dl::scene::SceneObject* solids = puzzle.Solids(solidLayer, solidCount);

        // ---- the body or the ball: after the camera and the shapes have moved, keep it out of what is solid
        if (walking)
        {
            dl::Player::Input move;
            move.moveForward = keys.moveForward;
            move.moveRight = keys.moveRight;
            move.jumpPressed = jumpPressed;
            move.jumpHeld = focused && IsDown(VK_SPACE);
            player.SetScene(solids, solidCount);
            player.Update(move, camera.Forward(), solidLayer, gpu.models.SpinSeconds(), dtf);
            puzzle.Update(player, dtf);
            const int taken = lights.Collect(player);
            if (taken >= 0)
            {
                DL_LOG_INFO("Stage", "hidden light %d of %d: %s", lights.Count(), dl::stage::kHiddenLightCount, dl::stage::kHiddenLights[taken].name);
            }

            // the checkpoint: reached by standing on the floor of a part; a fall below the stage goes back to it
            const int reached = dl::stage::CheckpointFor(player.Feet(), player.Grounded(), checkpoint);
            if (reached != checkpoint)
            {
                checkpoint = reached;
                DL_LOG_INFO("Stage", "checkpoint %d", checkpoint);
            }
            if (player.Feet().y < dl::stage::kFallY || player.JustRespawned())
            {
                player.SetSpawn(dl::stage::kCheckpoints[checkpoint].feet);
                player.Respawn();
                landDip = 0.0f;
                DL_LOG_INFO("Main", "fell: back at checkpoint %d", checkpoint);
            }
            if (clearSeconds < 0.0 && dl::stage::ReachedGoal(player.Feet(), player.Grounded()))
            {
                clearSeconds = stageSeconds;
                DL_LOG_INFO("Stage", "clear in %.1f s", clearSeconds);
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
                const float deepest = std::clamp(config.GetFloat("camera", "land_dip_max", kDefaultLandDipMax), 0.0f, 0.5f);
                landDip = std::max(landDip, std::min(impact * perSpeed, deepest));
            }
            landDip *= std::exp(-kLandDipReturn * dtf); // back up, quickly at first and then softly
            dl::Vec3 eye = player.Eye();
            eye.y -= landDip;
            camera.SetPose(eye, camera.Yaw(), camera.Pitch()); // only the place changes, not where it looks
        }
        else if (collide)
        {
            landDip = 0.0f;
            const float radius = std::clamp(config.GetFloat("collision", "radius", kDefaultBodyRadius), 0.05f, 1.0f);
            const dl::CollisionResult body =
                dl::ResolveSphere(camera.Position(), radius, solidLayer, gpu.models.SpinSeconds(), solids, solidCount);
            if (body.pushed)
            {
                camera.SetPose(body.position, camera.Yaw(), camera.Pitch());
            }
        }

        // the log: a move of the block, and a refused push (with what was in the way)
        if (puzzle.Moves() != blockMoves)
        {
            blockMoves = puzzle.Moves();
            DL_LOG_INFO("Stage", "block in cell (%d, %d)", puzzle.BlockCell().x, puzzle.BlockCell().z);
        }
        if (puzzle.BlockedFlash() > lastFlash)
        {
            DL_LOG_INFO("Stage", "push refused: blocked by %d", puzzle.LastBlocker());
        }
        lastFlash = puzzle.BlockedFlash();

        // ---- what is drawn: the stage as it is now
        // the outlines of the other layer: a hint, drawn only ([hints] ghosts = 0 turns them off)
        int drawCount = dl::stage::BuildDrawTable(puzzle, drawTable);
        if (config.GetBool("hints", "ghosts", true))
        {
            drawCount = dl::stage::AddGhosts(drawTable, drawCount, dl::stage::kMaxDrawObjects);
        }
        drawCount = lights.AddToDrawTable(drawTable, drawCount, dl::stage::kMaxDrawObjects);
        if (lights.All())
        {
            dl::stage::RecolourGoal(drawTable, drawCount); // every hidden light taken: the goal light turns cool white
        }
        gpu.models.SetObjects(drawTable, drawCount);

        const bool showTriangle = config.GetBool("test_triangle", "show", false); // the M0 triangle: off now that there are models

        // The VIEW matrix: "look from here, in this direction, with +Y as up". Moving the camera = moving the whole world the other way.
        const dl::Vec3 pos = camera.Position();
        const dl::Vec3 dir = camera.Forward();
        const DirectX::XMMATRIX view = DirectX::XMMatrixLookToLH(DirectX::XMVectorSet(pos.x, pos.y, pos.z, 1.0f),
                                                                  DirectX::XMVectorSet(dir.x, dir.y, dir.z, 0.0f),
                                                                  DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));

        // the background belongs to the layer on the screen (M2): the models know the colours
        float clear[4] = {};
        gpu.models.ClearColor(clear);

        {
            dl::GpuEvent frame(gpu.device->GetAnnotation(), L"Frame");
            gpu.device->BeginFrame(clear);
            const float aspect = static_cast<float>(gpu.device->Width()) / static_cast<float>(gpu.device->Height());
            {
                dl::GpuEvent pass(gpu.device->GetAnnotation(), L"Opaque.Models");
                gpu.models.Draw(gpu.device->GetContext(), view, dl::MakeProjection(aspect), DirectX::XMFLOAT3(pos.x, pos.y, pos.z));
            }
            if (showTriangle)
            {
                dl::GpuEvent pass(gpu.device->GetAnnotation(), L"Opaque.TestTriangle");
                gpu.triangle.Draw(gpu.device->GetContext(), aspect, angle, view);
            }
        }
        gpu.device->EndFrame();

        fpsTimer += dt;
        ++fpsFrames;
        if (fpsTimer >= 0.5)
        {
            const wchar_t* mode = collide ? (walk ? L"collide | walk" : L"collide | fly") : L"noclip";
            wchar_t stageText[64];
            if (clearSeconds >= 0.0)
            {
                swprintf_s(stageText, L"CLEAR %.1f s | lights %d/%d", clearSeconds, lights.Count(), dl::stage::kHiddenLightCount);
            }
            else
            {
                swprintf_s(stageText, L"part %d | %.0f s | lights %d/%d", checkpoint + 1, stageSeconds, lights.Count(), dl::stage::kHiddenLightCount);
            }
            wchar_t title[384];
            swprintf_s(title, L"DualLayer | %s | %d x %d | %.1f FPS (%.2f ms) | camera %.1f %.1f %.1f | %s | %s | %s%s",
                       gpu.device->AdapterName().c_str(), gpu.device->Width(), gpu.device->Height(), fpsFrames / fpsTimer,
                       1000.0 * fpsTimer / fpsFrames, pos.x, pos.y, pos.z, gpu.models.StatusLine().c_str(), mode, stageText,
                       window.MouseCaptured() ? L"" : L" | click for mouse look");
            window.SetTitle(title);
            fpsTimer = 0.0;
            fpsFrames = 0;
        }
    }

    DL_LOG_INFO("Main", "shutdown");
    gpu.Shutdown(); // models, triangle, then the device (which reports whatever is still alive as a leak)
    dl::log::Close();
    return 0;
}

} // namespace

int WINAPI WinMain(HINSTANCE instance, HINSTANCE, LPSTR, int)
{
    return RunApp(instance, __argc, __argv);
}
