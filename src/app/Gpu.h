// Gpu.h - everything that lives on the GPU, made and lost together: the device, the models and the overlay.
// Owns:      the device (a new one is made after a loss: a Device that was lost is never reused), Models, Overlay.
// Borrows:   the town, whose polygons are baked into the static batch of each moment at Init.
// Recovery:  a lost device takes all of it, so all of it is made again together (Recover); what lives outside (the
//            town, the body, the camera) is kept.
#pragma once

#include "core/Config.h"
#include "core/Window.h"
#include "render/Device.h"
#include "render/GpuTimer.h"
#include "render/Models.h"
#include "render/Overlay.h"
#include "scene/Town.h"

#include <memory>

namespace dl
{

/// The looks of the two moments from the lights of the model (tools/layout/city_model.py LIGHT): its colours are sRGB,
/// the renderer works in linear light.
void ApplyTownThemes(Models& models, const town::TownData& town);

struct Gpu
{
    explicit Gpu(const town::TownData& townData) : town(townData)
    {
    }

    std::unique_ptr<Device> device = std::make_unique<Device>();
    Models models;
    Overlay overlay;
    GpuTimer timer;             ///< how long the GPU takes for the parts of a frame (the title bar shows it)
    const town::TownData& town; // BORROWS: the town outlives the GPU objects

    bool Init(const Window& window, const Config& config, int adapter);
    /// WHY this order: every object must be released before Device::Shutdown reports leaks.
    void Shutdown();
    /// A new device, made from nothing. What the models remember (layer, ripple, wireframe) is kept; the level of
    /// detail goes back to [models] segments.
    bool Recover(const Window& window, const Config& config, int adapter);
};

} // namespace dl
