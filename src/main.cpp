// main.cpp - entry point and the frame loop.
// Frame:  PumpMessages > (F5: reload config) > (resize) > Update (camera, models) > Render [GPU event "Frame" > "Opaque.Models"
//         (> "Opaque.TestTriangle", when [test_triangle] show = 1)] > Present
// Keys:   W A S D move, Q / E down / up, arrow keys turn, R puts the camera back, F5 reloads config/default.ini, Esc quits
//         Z wireframe on / off, X the next level of detail (8, 16, 32, 64 segments), C the shapes stop / turn
//         Tab switches the layer (A <-> B)
//         F9 (only with [debug] device_reset_key = 1) throws the GPU objects away and builds them again: the test of the recovery
// Lost device: Present reports it, the loop builds device, triangle and models again (Gpu::Recover); the layer, the spin and the
//         camera stay as they were. After kMaxRecoveries tries the program says so and ends.
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

#include <DirectXMath.h>

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

    /// A new device, made from nothing. What the models remember (layer, fade, spin, wireframe) is kept; the level of
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

    LARGE_INTEGER frequency{};
    LARGE_INTEGER last{};
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&last);
    double fpsTimer = 0.0;
    int fpsFrames = 0;
    float angle = 0.0f;           // radians
    bool resetKeyWasDown = false; // WHY: R must act once per press, not on every frame it is held
    bool wireKeyWasDown = false;
    bool detailKeyWasDown = false;
    bool spinKeyWasDown = false;
    bool layerKeyWasDown = false;
    bool resetTestKeyWasDown = false;
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
        angle += DirectX::XMConvertToRadians(config.GetFloat("test_triangle", "spin_deg_per_sec", 0.0f)) * static_cast<float>(dt);

        // ---- camera
        camera.Update(focused ? ReadCameraKeys() : dl::CameraInput{}, static_cast<float>(dt));
        const bool resetKeyDown = focused && IsDown('R');
        if (resetKeyDown && !resetKeyWasDown)
        {
            camera.Reset();
        }
        resetKeyWasDown = resetKeyDown;
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
        // WHY not while Alt is down: Alt + Tab switches windows and must not also switch the layer
        if (PressedOnce(VK_TAB, layerKeyWasDown, focused && !IsDown(VK_MENU)))
        {
            gpu.models.ToggleLayer();
        }
        gpu.models.Update(static_cast<float>(dt));

        const bool showTriangle = config.GetBool("test_triangle", "show", false); // the M0 triangle: off now that there are models

        // The VIEW matrix: "look from here, in this direction, with +Y as up". Moving the camera = moving the whole world the other way.
        const dl::Vec3 pos = camera.Position();
        const dl::Vec3 dir = camera.Forward();
        const DirectX::XMMATRIX view = DirectX::XMMatrixLookToLH(DirectX::XMVectorSet(pos.x, pos.y, pos.z, 1.0f),
                                                                  DirectX::XMVectorSet(dir.x, dir.y, dir.z, 0.0f),
                                                                  DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));

        // the background belongs to the layer (M2): the models know the two colours and mix them during the fade
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
            wchar_t title[256];
            swprintf_s(title, L"DualLayer | %s | %d x %d | %.1f FPS (%.2f ms) | camera %.1f %.1f %.1f | %s",
                       gpu.device->AdapterName().c_str(), gpu.device->Width(), gpu.device->Height(), fpsFrames / fpsTimer,
                       1000.0 * fpsTimer / fpsFrames, pos.x, pos.y, pos.z, gpu.models.StatusLine().c_str());
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
