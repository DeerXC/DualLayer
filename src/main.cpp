// main.cpp - entry point: the log, the config, the window, the town, the GPU objects, and then the session of the town.
// Town:    data/town.dlt, written by tools/layout/city_model.py (scene/Town.h), is loaded once and kept for the whole
//          run; the game cannot start without it.
// Session: app/TownSession.h is the frame loop. Everything else of the game lives in src/scene (no Windows, tested
//          without a window), src/render (Direct3D 11) and src/core (log, config, window).
// Args:    --adapter N   choose the N-th adapter listed in the log (overrides [render] adapter)

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include "app/AppConfig.h"
#include "app/Gpu.h"
#include "app/TownSession.h"
#include "core/Config.h"
#include "core/Log.h"
#include "core/Window.h"
#include "scene/Camera.h"
#include "scene/Town.h"

#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>

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

/// The N of `--adapter N`, or -2 when it was not given.
int ParseAdapterArg(int argc, char** argv)
{
    for (int i = 1; i + 1 < argc; ++i)
    {
        if (std::strcmp(argv[i], "--adapter") == 0)
        {
            return std::atoi(argv[i + 1]);
        }
    }
    return -2;
}

int RunApp(HINSTANCE instance, int argc, char** argv)
{
    CreateDirectoryA("logs", nullptr);
    dl::log::OpenFile("logs/dual.log");
    DL_LOG_INFO("Main", "start");

    dl::Config config;
    dl::Camera camera;
    dl::LoadConfig(config, camera);

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
    DL_LOG_INFO("Main", "town: %zu polygons, %zu materials, %zu places, %zu steps", town.polygons.size(),
                town.materials.size(), town.places.size(), town.steps.size());

    dl::Gpu gpu(town);
    if (!gpu.Init(window, config, adapter))
    {
        MessageBoxA(window.Handle(), "Initialisation failed. See logs/dual.log", "DualLayer", MB_OK | MB_ICONERROR);
        gpu.Shutdown();
        return 1;
    }
    int code = 0;
    {
        const auto session = std::make_unique<dl::TownSession>(window, gpu, config, camera, adapter, town);
        code = session->Run();
    }
    DL_LOG_INFO("Main", "shutdown");
    gpu.Shutdown(); // models, overlay, then the device (which reports whatever is still alive as a leak)
    dl::log::Close();
    return code;
}

} // namespace

int WINAPI WinMain(HINSTANCE instance, HINSTANCE, LPSTR, int)
{
    return RunApp(instance, __argc, __argv);
}
