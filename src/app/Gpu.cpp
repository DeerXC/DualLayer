#include "app/Gpu.h"

#include "render/StaticBatch.h"
#include "scene/Lamps.h"

namespace dl
{

void ApplyTownThemes(Models& models, const town::TownData& town)
{
    const auto lin = [](const float c[3])
    {
        return DirectX::XMFLOAT3(batch_detail::SrgbToLinear(c[0]), batch_detail::SrgbToLinear(c[1]),
                                 batch_detail::SrgbToLinear(c[2]));
    };
    Models::Theme themes[2];
    for (int i = 0; i < 2; ++i)
    {
        const town::Light& l = town.lights[i];
        Models::Theme& t = themes[i];
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
        const float coolBounce[3] = {0.21f, 0.22f, 0.24f}; // the ground of B is weathered stone: it bounces a grey, not a blue
        t.groundFill = lin(i == 0 ? warmBounce : coolBounce);
        t.mistBase = town.sea + 2.0f;
        t.mistDensity = i == 0 ? 0.0018f : 0.0036f;
        t.tint = DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f); // the town has its own colours: no stone tint
        t.weathering = 0.0f;
    }
    models.SetThemes(themes[0], themes[1]);
}

bool Gpu::Init(const Window& window, const Config& config, int adapter)
{
    if (!device->Init(window.Handle(), window.Width(), window.Height(), adapter,
                      config.GetBool("render", "vsync", true)) ||
        !models.Init(device->GetDevice(), config.GetInt("models", "segments", 32)) ||
        !overlay.Init(device->GetDevice()))
    {
        return false;
    }
    timer.Init(device->GetDevice()); // a device without timestamp queries is not an error: the title bar says gpu n/a
    models.SetTimer(&timer);
    models.LoadThemes(config);   // the looks of the layers: built in, [layer_a] / [layer_b] in the config change them
    models.LoadLighting(config); // the shadows, the lamps, the exposure: [lighting] in the config
    models.SetStaticLamps(
        town::TownLamps(town)); // the street lamps of the town: the renderer chooses the nearest ones every frame
    // the town (scene/Town.h): its own looks, its polygons baked into the static batch of each layer, its still water
    // into the water pass
    ApplyTownThemes(models, town);
    models.SetStaticTown(town.BakeLayer(0), town.BakeLayer(1));
    models.SetStaticWater(town.BakeWater(0), town.BakeWater(1));
    return true;
}

void Gpu::Shutdown()
{
    overlay.Shutdown();
    models.Shutdown();
    timer.Shutdown();
    device->Shutdown();
}

bool Gpu::Recover(const Window& window, const Config& config, int adapter)
{
    Shutdown();
    device = std::make_unique<Device>(); // a new object: a Device that was lost is never reused
    return Init(window, config, adapter);
}

} // namespace dl
