// main.cpp - entry point and the frame loop.
// Frame:  PumpMessages > (F5: reload config) > (resize) > Update (camera, models) > Render [GPU event "Frame" > "Opaque.Models"
//         (> "Opaque.TestTriangle", when [test_triangle] show = 1)] > Present
// Keys:   W A S D move, Q / E down / up, arrow keys turn, R puts the camera back, F5 reloads config/default.ini, Esc quits
//         Z wireframe on / off, X the next level of detail (8, 16, 32, 64 segments), C the shapes stop / turn
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
#include <string>

namespace
{
constexpr char kConfigPath[] = "config/default.ini";

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

    dl::Device device;
    dl::Triangle triangle;
    dl::Models models;
    if (!device.Init(window.Handle(), window.Width(), window.Height(), adapter, config.GetBool("render", "vsync", true)) ||
        !triangle.Init(device.GetDevice()) || !models.Init(device.GetDevice(), config.GetInt("models", "segments", 32)))
    {
        MessageBoxA(window.Handle(), "Initialisation failed. See logs/dual.log", "DualLayer", MB_OK | MB_ICONERROR);
        models.Shutdown(); // WHY: every object must be released before Device::Shutdown reports leaks
        triangle.Shutdown();
        device.Shutdown();
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

    while (window.PumpMessages() && !device.Lost())
    {
        if (window.ConsumeReloadKey())
        {
            LoadConfig(config, camera);
            device.SetVsync(config.GetBool("render", "vsync", true));
        }
        if (window.ConsumeResized())
        {
            device.Resize(window.Width(), window.Height());
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

        // ---- camera: keys count only while OUR window is in front (typing in another window must not move it)
        const bool focused = GetForegroundWindow() == window.Handle();
        camera.Update(focused ? ReadCameraKeys() : dl::CameraInput{}, static_cast<float>(dt));
        const bool resetKeyDown = focused && IsDown('R');
        if (resetKeyDown && !resetKeyWasDown)
        {
            camera.Reset();
        }
        resetKeyWasDown = resetKeyDown;
        if (PressedOnce('Z', wireKeyWasDown, focused))
        {
            models.ToggleWireframe();
        }
        if (PressedOnce('X', detailKeyWasDown, focused))
        {
            models.CycleDetail();
        }
        if (PressedOnce('C', spinKeyWasDown, focused))
        {
            models.ToggleSpin();
        }
        models.Update(static_cast<float>(dt));
        const bool showTriangle = config.GetBool("test_triangle", "show", false); // the M0 triangle: off now that there are models

        // The VIEW matrix: "look from here, in this direction, with +Y as up". Moving the camera = moving the whole world the other way.
        const dl::Vec3 pos = camera.Position();
        const dl::Vec3 dir = camera.Forward();
        const DirectX::XMMATRIX view = DirectX::XMMatrixLookToLH(DirectX::XMVectorSet(pos.x, pos.y, pos.z, 1.0f),
                                                                  DirectX::XMVectorSet(dir.x, dir.y, dir.z, 0.0f),
                                                                  DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));

        const float clear[4] = {config.GetFloat("render", "clear_r", 0.01f), config.GetFloat("render", "clear_g", 0.015f),
                                config.GetFloat("render", "clear_b", 0.04f), 1.0f};
        {
            dl::GpuEvent frame(device.GetAnnotation(), L"Frame");
            device.BeginFrame(clear);
            const float aspect = static_cast<float>(device.Width()) / static_cast<float>(device.Height());
            {
                dl::GpuEvent pass(device.GetAnnotation(), L"Opaque.Models");
                models.Draw(device.GetContext(), view, dl::MakeProjection(aspect), DirectX::XMFLOAT3(pos.x, pos.y, pos.z));
            }
            if (showTriangle)
            {
                dl::GpuEvent pass(device.GetAnnotation(), L"Opaque.TestTriangle");
                triangle.Draw(device.GetContext(), aspect, angle, view);
            }
        }
        device.EndFrame();

        fpsTimer += dt;
        ++fpsFrames;
        if (fpsTimer >= 0.5)
        {
            wchar_t title[256];
            swprintf_s(title, L"DualLayer | %s | %d x %d | %.1f FPS (%.2f ms) | camera %.1f %.1f %.1f | %s",
                       device.AdapterName().c_str(), device.Width(), device.Height(), fpsFrames / fpsTimer,
                       1000.0 * fpsTimer / fpsFrames, pos.x, pos.y, pos.z, models.StatusLine().c_str());
            window.SetTitle(title);
            fpsTimer = 0.0;
            fpsFrames = 0;
        }
    }

    DL_LOG_INFO("Main", "shutdown");
    models.Shutdown(); // WHY: before Device::Shutdown, which reports whatever is still alive as a leak
    triangle.Shutdown();
    device.Shutdown();
    dl::log::Close();
    return 0;
}
} // namespace

int WINAPI WinMain(HINSTANCE instance, HINSTANCE, LPSTR, int)
{
    return RunApp(instance, __argc, __argv);
}

