// main.cpp - entry point and the frame loop.
// Frame:  PumpMessages > (F5: reload config) > (resize) > Update (camera, models, the map, walking or collision) > Render
//         [GPU event "Frame" > "Opaque.Models" (> "Opaque.TestTriangle", when [test_triangle] show = 1)] > Present
// Map:    scene/Stage.h: the fixed boxes, the block puzzle of the courtyard (scene/Puzzle.h), the rooms, the seals of the
//         wings and the doors they open. Every frame the drawn table goes to the renderer and the solid table of the layer
//         of the body (Puzzle::Solids and the closed doors of the map) to the body, so what is drawn and what is solid come
//         from the same state. The drawn table also holds what the map says without words (scene/StageHints.h) and the
//         hidden lights (scene/StageLights.h).
// Keys:   W A S D move, the mouse turns (click into the window to capture it; Esc gives it back, a second Esc quits), the arrow
//         keys turn as well, R goes back to the entry of the room the body is in (and puts back the puzzle of that room),
//         F5 reloads config/default.ini
//         Space jumps (walking; held = the full jump, a tap = a small hop), Q / E down / up (flying)
//         E (walking) takes the orb in reach, or puts the held one down (scene/Mechanisms.h)
// Overlay: drawn last (render/Overlay.h): a dot in the middle of the view, the prompt of the key that does something now
//         (scene/Prompts.h: E TAKE / PUT DOWN / PLACE, TAB SWITCH WORLD until it is learned, R RESET ROOM when stuck) and a
//         small status line (the room, the seals, the hidden lights). [hints] prompts = false hides the prompts.
//         F walking / flying (walking: gravity, the body stands on the floor and on objects; flying: a free camera)
//         Z wireframe on / off, X the next level of detail (8, 16, 32, 64 segments), C the shapes stop / turn
//         Tab switches the layer (A <-> B). A switch that would put the body inside something is refused: the ripple opens
//         a little and turns back (Puzzle::BodyFits, LayerGuard), and the old layer stays solid meanwhile.
//         N collision on / off (off = fly through everything; walking needs collision, so N off also means flying)
//         F9 (only with [debug] device_reset_key = 1) throws the GPU objects away and builds them again: the test of the recovery
// Walking: scene/Player.h. The camera is the eye of a body of three balls; the camera only turns, the body walks, falls and jumps.
//         A hard landing lowers the eye for a moment ([camera] land_dip). A body that falls below the map goes back to the
//         entry of the room it last stood in.
// Flying: the camera is one ball ([collision] radius); after it has moved, the ball is pushed out of the solid objects of the layer
//         that the screen is going to show (scene/Collision.h); the camera keeps its direction.
// Lost device: Present reports it, the loop builds device, triangle and models again (Gpu::Recover); the layer, the spin, the
//         map and the camera stay as they were. After kMaxRecoveries tries the program says so and ends.
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
#include "render/Overlay.h"
#include "render/OverlayBatch.h"
#include "render/Projection.h"
#include "render/Triangle.h"
#include "scene/Anchors.h"
#include "scene/Camera.h"
#include "scene/CityParts.h"
#include "scene/CityState.h"
#include "scene/Collision.h"
#include "scene/EndScreen.h"
#include "scene/Ending.h"
#include "scene/Guidance.h"
#include "scene/LampTravel.h"
#include "scene/LockCrate.h"
#include "scene/Landscape.h"
#include "scene/Lantern.h"
#include "scene/OrbReturn.h"
#include "scene/Player.h"
#include "scene/Prompts.h"
#include "scene/Puzzle.h"
#include "scene/Recovery.h"
#include "scene/SceneData.h"
#include "scene/Stage.h"
#include "scene/StageHints.h"
#include "scene/StageLights.h"
#include "scene/Things.h"
#include "scene/Town.h"
#include "scene/TownPlayer.h"
#include "scene/WaterFx.h"

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
constexpr float kEyeEase = 30.0f;                   // UNITS: 1 / seconds; small changes of the eye height are eased over about 0.05 s
constexpr float kEyeEaseMax = 0.5f;                 // UNITS: metres; a larger change (a respawn, a teleport) is followed at once
constexpr int kMaxSolids = dl::puzzle::Puzzle::kMaxSolids + 4 + dl::mech::kMaxGates + dl::stage::kLiftCount + 1 + 8 + 8 + 1; // + doors, gates, lifts, the crate, the water for orbs, fragments
static_assert(dl::kFarPlane <= dl::stage::kViewDistance, "scene/Landscape.h kViewDistance must reach the far plane: the sea and the land are sized by it");

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

/// The looks of the two moments from the lights of the model (tools/layout/city_model.py LIGHT): its colours are sRGB, the
/// renderer works in linear light.
void ApplyTownThemes(dl::Models& models, const dl::town::TownData& town)
{
    const auto lin = [](const float c[3]) {
        return DirectX::XMFLOAT3(dl::batch_detail::SrgbToLinear(c[0]), dl::batch_detail::SrgbToLinear(c[1]), dl::batch_detail::SrgbToLinear(c[2]));
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
    std::unique_ptr<dl::Device> device = std::make_unique<dl::Device>();
    dl::Triangle triangle;
    dl::Models models;
    dl::Overlay overlay;
    const dl::town::TownData* town = nullptr; // set: the town of the model is drawn instead of the greybox map

    bool Init(const dl::Window& window, const dl::Config& config, int adapter)
    {
        if (!device->Init(window.Handle(), window.Width(), window.Height(), adapter, config.GetBool("render", "vsync", true)) ||
            !triangle.Init(device->GetDevice()) || !models.Init(device->GetDevice(), config.GetInt("models", "segments", 32)) ||
            !overlay.Init(device->GetDevice()))
        {
            return false;
        }
        models.LoadThemes(config); // the looks of the layers: built in, [layer_a] / [layer_b] in the config change them
        if (town != nullptr)
        {
            // the town of the model (scene/Town.h): its own looks, its polygons baked into the static batch of each layer,
            // its still water into the water pass; nothing of the greybox map
            ApplyTownThemes(models, *town);
            models.SetStaticExtra(town->BakeLayer(0), town->BakeLayer(1));
            models.SetStaticWater(town->BakeWater(0), town->BakeWater(1));
            models.SetStatic(nullptr, 0);
            return true;
        }
        // the fixed city: baked once into one draw call per layer (render/StaticBatch.h); the drawn table then holds the rest
        static std::vector<dl::scene::SceneObject> fixedCity;
        if (fixedCity.empty())
        {
            static dl::scene::SceneObject all[dl::stage::kMaxDrawObjects];
            const dl::puzzle::Puzzle none;
            dl::stage::BuildDrawTable(none, false, all);
            fixedCity.assign(all, all + dl::stage::StaticDrawCount());
        }
        // the ground, the sea floor and the plants (scene/Landscape.h): baked once, joined to the static batch of each layer
        static const std::array<dl::BatchMesh, 2> world = {dl::stage::BakeWorld(0), dl::stage::BakeWorld(1)};
        models.SetStaticExtra(world[0], world[1]);
        models.SetStatic(fixedCity.data(), static_cast<int>(fixedCity.size()));
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

/// Fills the drawn table with the map as it is now: the map, its state, then what it says without words. Returns the count.
struct Respawn
{
    dl::Vec3 at{};
    float since = -1.0f; ///< UNITS: seconds since the last respawn; negative before the first
};

/// What water does this frame (scene/WaterFx.h): its sheets now, the wet bands, the splashes, the body sinking.
struct WaterState
{
    std::array<dl::stage::WaterSheet, dl::stage::kMaxSheets> sheets{};
    int count = 0;
    dl::stage::WetMarks wet;
    dl::stage::Splashes splashes;
    float sinking = -1.0f; // UNITS: seconds left of a body sinking in water before it comes back; negative: not sinking
};

int BuildFrameTable(const dl::puzzle::Puzzle& puzzle, const dl::stage::SealSet& seals, const dl::stage::LightSet& lights,
                    const dl::mech::MechanismSet& mech, const std::array<dl::shapes::Lift, dl::stage::kLiftCount>& lifts,
                    const dl::canal::Canal& canal, const dl::stage::Waterworks& waters, const dl::stage::LockCrate& lockCrate,
                    const dl::stage::AnchorSet& anchors, const Respawn& respawn, const Respawn& orbReturn,
                    const dl::Player& player, const dl::Vec3& eye, const dl::Vec3& look, const dl::Config& config, float sinceClear,
                    int room, float secondsInRoom, float clock, const WaterState& water, dl::scene::SceneObject (&table)[dl::stage::kMaxDrawObjects])
{
    namespace st = dl::stage;
    const bool cleared = sinceClear >= 0.0f;
    const bool ghosts = config.GetBool("hints", "ghosts", true); // the outlines of the other layer: [hints] ghosts = false turns them off
    int count = st::BuildDrawTable(puzzle, cleared, table);
    count = st::AddMapDraw(seals, table, count, st::kMaxDrawObjects);
    count = st::AddLifts(lifts, table, count, st::kMaxDrawObjects);
    count = canal.AddDraw(table, count, st::kMaxDrawObjects);
    count = lockCrate.AddDraw(table, count, st::kMaxDrawObjects); // the crate of the tower lock (scene/LockCrate.h)
    count = st::AddWaterDraw(waters, table, count, st::kMaxDrawObjects); // the water of A as it is now (it drains at the end)
    count = st::AddWaterFx(water.sheets.data(), water.count, waters, water.wet, water.splashes, table, count, st::kMaxDrawObjects); // foam, falls, wet walls
    count = anchors.AddDraw(table, count, st::kMaxDrawObjects);
    count = st::AddFragments(lights, true, table, count, st::kMaxDrawObjects); // the pieces of A that the lights brought back (B)
    count = st::AddSea(eye, table, count, st::kMaxDrawObjects);              // the sea round the eye, wider than the far plane
    count = st::AddRespawnRing(respawn.at, respawn.since, table, count, st::kMaxDrawObjects);
    count = st::AddRespawnRing(orbReturn.at, orbReturn.since, table, count, st::kMaxDrawObjects);
    const int hintFirst = count;
    if (ghosts)
    {
        count = st::AddGhosts(table, count, st::kMaxDrawObjects);
    }
    // the moving parts of the wings: the gates that are there and the pads of the triggers (outlined in the other layer like
    // the fixed boxes), then the orbs
    const int movingFirst = count;
    count = mech.AddFixedDraw(table, count, st::kMaxDrawObjects);
    if (ghosts)
    {
        const int movingCount = count - movingFirst;
        count = st::AddOutlines(table + movingFirst, movingCount, table, count, st::kMaxDrawObjects);
    }
    // stuck in a room for a while: its outlines grow brighter, then pulse (scene/Guidance.h)
    st::BrightenRoom(table, hintFirst, count, room, secondsInRoom, clock);
    count = mech.AddOrbDraw(player, look, table, count, st::kMaxDrawObjects);
    // guidance: a column of light over every place that waits for something, and where a bridge will appear
    count = mech.AddBeacons(table, count, st::kMaxDrawObjects);
    count = canal.AddBeacons(table, count, st::kMaxDrawObjects);
    count = st::AddGatePreviews(mech, table, count, st::kMaxDrawObjects);
    // a refused push: the outline of what was in the way, in both layers, for a moment
    count = st::AddBlockedFlash(puzzle, table, count, st::kMaxDrawObjects);
    // the blue frame of the door, seen in A while the door is closed for a body in A: the plate shows what it does
    count = st::AddDoorFrame(table, count, st::kMaxDrawObjects, puzzle.DoorOpen(0));
    count = st::AddGoalBeacon(table, count, st::kMaxDrawObjects, sinceClear);
    count = st::AddClearRings(table, count, st::kMaxDrawObjects, sinceClear);
    count = lights.AddToDrawTable(table, count, st::kMaxDrawObjects);
    count = st::AddLightRing(lights, table, count, st::kMaxDrawObjects); // the ring of lamps round the bell tower
    if (lights.All())
    {
        st::RecolourGoal(table, count); // every hidden light taken: the goal light turns cool white
    }
    return count;
}

/// The solid table of a body (or an orb) in `layer`: the puzzle's table (the boxes of the map, the block, the courtyard door
/// while closed), the closed doors of the map, the gates of the wings that are there and the memory fragments brought back.
/// Returns the count.
int BuildSolidTable(dl::puzzle::Puzzle& puzzle, const dl::stage::SealSet& seals, const dl::stage::LightSet& lights, const dl::stage::LockCrate& lockCrate,
                    const dl::mech::MechanismSet& mech,
                    const std::array<dl::shapes::Lift, dl::stage::kLiftCount>& lifts, const dl::canal::Canal& canal, int layer,
                    dl::scene::SceneObject (&table)[kMaxSolids])
{
    int count = 0;
    const dl::scene::SceneObject* solids = puzzle.Solids(layer, count);
    for (int i = 0; i < count; ++i)
    {
        table[i] = solids[i];
    }
    count = dl::stage::AddMapSolids(seals, table, count, kMaxSolids);
    count = dl::stage::AddLifts(lifts, table, count, kMaxSolids);
    count = canal.AddSolids(layer, table, count, kMaxSolids);
    count = dl::stage::AddFragments(lights, false, table, count, kMaxSolids);
    count = lockCrate.AddSolids(layer, table, count, kMaxSolids);
    return mech.AddSolids(layer, table, count, kMaxSolids);
}

/// The overlay of one frame: the dot in the middle, the prompts at the bottom, the status line at the top left. Sizes follow
/// the height of the window, so the text is as big on a laptop as on a desktop screen.
void BuildHud(dl::OverlayBatch& hud, int width, int height, const std::array<dl::prompt::Shown, 2>& prompts, const char* status,
              const dl::endscreen::Screen* end, const dl::stage::Resonance& resonance, float clock, float underwater, bool reticle = true)
{
    namespace pr = dl::prompt;
    hud.Clear();
    const float w = static_cast<float>(width);
    const float h = static_cast<float>(height);
    // under water: the view is tinted deep green-blue and darker (scene/WaterFx.h Underwater)
    if (underwater > 0.0f)
    {
        hud.AddRect(0.0f, 0.0f, w, h, {0.03f, 0.20f, 0.26f, 0.55f * underwater});
    }
    const float unit = std::max(2.0f, std::floor(h / 320.0f)); // one font pixel, in screen pixels
    // the dot in the middle of the view: what E looks at
    const float dot = std::max(2.0f, std::floor(unit * 0.8f));
    if (reticle)
    {
        hud.AddRect(std::floor(0.5f * (w - dot)), std::floor(0.5f * (h - dot)), dot, dot, {1.0f, 1.0f, 1.0f, 0.75f});
    }
    // the status line
    const float statusUnit = std::max(2.0f, std::floor(unit * 0.6f));
    hud.AddRect(0.0f, 0.0f, dl::OverlayBatch::TextWidth(status, statusUnit) + 4.0f * statusUnit, dl::OverlayBatch::TextHeight(statusUnit) + 3.0f * statusUnit,
                {0.0f, 0.0f, 0.0f, 0.35f});
    hud.AddText(2.0f * statusUnit, 1.5f * statusUnit, statusUnit, status, {0.92f, 0.92f, 0.92f, 0.85f});
    // the prompts, one row each, from the bottom up: [KEY] VERB
    float y = h - 6.0f * unit - dl::OverlayBatch::TextHeight(unit);
    for (const pr::Shown& shown : prompts)
    {
        if (shown.kind == pr::Kind::Count)
        {
            continue;
        }
        const pr::Text text = pr::TextOf(shown.kind);
        const float a = shown.alpha;
        const float pad = 2.0f * unit;
        const float keyWidth = dl::OverlayBatch::TextWidth(text.key, unit) + 2.0f * pad;
        const float verbWidth = dl::OverlayBatch::TextWidth(text.verb, unit);
        const float rowHeight = dl::OverlayBatch::TextHeight(unit) + 2.0f * pad;
        const float total = keyWidth + 3.0f * unit + verbWidth;
        const float x = std::floor(0.5f * (w - total));
        hud.AddRect(x - 2.0f * unit, y - pad, total + 4.0f * unit, rowHeight, {0.0f, 0.0f, 0.0f, 0.45f * a});
        hud.AddFrame(x, y - pad, keyWidth, rowHeight, std::max(1.0f, std::floor(unit * 0.5f)), {1.0f, 0.86f, 0.45f, a});
        hud.AddText(x + pad, y, unit, text.key, {1.0f, 0.86f, 0.45f, a});
        hud.AddText(x + keyWidth + 3.0f * unit, y, unit, text.verb, {1.0f, 1.0f, 1.0f, a});
        y -= rowHeight + 2.0f * unit;
    }
    // the resonance of the lantern (scene/Guidance.h): a small light at the bottom of the screen that pulses faster as a hidden
    // light comes nearer; a mark at the left or right edge when the light is not ahead
    if (resonance.light >= 0)
    {
        const float pulse = 0.5f + 0.5f * std::sin(6.2831853f * resonance.rate * clock);
        const float a = (0.25f + 0.75f * pulse) * (0.35f + 0.65f * resonance.strength);
        const float size = std::floor(unit * (3.0f + 3.0f * resonance.strength));
        hud.AddRect(std::floor(0.5f * (w - size)), h - 3.0f * unit - size, size, size, {1.0f, 0.82f, 0.45f, a});
        if (resonance.side != 0)
        {
            const float mark = 3.0f * unit;
            const float mx = resonance.side < 0 ? 2.0f * unit : w - 2.0f * unit - mark;
            hud.AddRect(mx, std::floor(0.5f * (h - 4.0f * mark)), mark, 4.0f * mark, {1.0f, 0.82f, 0.45f, 0.5f * a});
        }
    }
    // the end screen: a panel in the middle, the first line large
    if (end != nullptr && end->count > 0)
    {
        const float big = unit * 2.0f;
        float panelWidth = dl::OverlayBatch::TextWidth(end->lines[0], big);
        float panelHeight = dl::OverlayBatch::TextHeight(big);
        for (int i = 1; i < end->count; ++i)
        {
            panelWidth = std::max(panelWidth, dl::OverlayBatch::TextWidth(end->lines[i], unit));
            panelHeight += dl::OverlayBatch::TextHeight(unit) + 3.0f * unit;
        }
        const float pad = 8.0f * unit;
        const float px = std::floor(0.5f * (w - panelWidth) - pad);
        const float py = std::floor(0.5f * (h - panelHeight) - pad);
        hud.AddRect(px, py, panelWidth + 2.0f * pad, panelHeight + 2.0f * pad, {0.0f, 0.0f, 0.0f, 0.6f});
        hud.AddFrame(px, py, panelWidth + 2.0f * pad, panelHeight + 2.0f * pad, std::max(1.0f, std::floor(unit * 0.5f)), {1.0f, 0.86f, 0.45f, 0.9f});
        float ty = py + pad;
        hud.AddText(std::floor(0.5f * (w - dl::OverlayBatch::TextWidth(end->lines[0], big))), ty, big, end->lines[0], {1.0f, 0.86f, 0.45f, 1.0f});
        ty += dl::OverlayBatch::TextHeight(big) + 3.0f * unit;
        for (int i = 1; i < end->count; ++i)
        {
            hud.AddText(std::floor(0.5f * (w - dl::OverlayBatch::TextWidth(end->lines[i], unit))), ty, unit, end->lines[i], {1.0f, 1.0f, 1.0f, 0.95f});
            ty += dl::OverlayBatch::TextHeight(unit) + 3.0f * unit;
        }
    }
}

/// True when the body fits in `layer` of the town (with the circle of the lantern): none of its three balls overlaps a surface
/// (a hair less than its radius, so that standing on a floor fits) and its feet are not under the water of that layer.
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

/// The town of the model (scene/Town.h), walked in both moments, with the things that change (scene/Things.h; docs/DESIGN.md
/// 12, step 4: the harbour so far). This loop walks, switches, opens, falls, sinks and draws. A fall or the water brings the body back to the last place it stood still
/// on; R does the same. Returns the exit code.
int RunTown(dl::Window& window, Gpu& gpu, dl::Config& config, dl::Camera& camera, int adapter, const dl::town::TownData& town)
{
    namespace st = dl::stage;
    const std::array<dl::town::TriMesh, 2> meshes = {town.BuildMesh(0), town.BuildMesh(1)};
    DL_LOG_INFO("Main", "town solid: %d triangles in A, %d in B", meshes[0].TriangleCount(), meshes[1].TriangleCount());
    dl::Player player;
    ApplyPlayerConfig(config, player);
    player.SetScene(nullptr, 0);
    dl::town::Moments moments(&meshes[0], &meshes[1]); // the solid of both moments, with the circle of the lantern
    dl::town::Moments plain(&meshes[0], &meshes[1]);   // the same without it: where a fall may come back to
    dl::town::Lantern lantern;                          // carried, or set down in A (scene/Lantern.h)
    dl::town::Things things;                            // the things that change: opened in A, the trace in B (scene/Things.h)
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
        gpu.models.ToggleLayer(); // the game opens in the present (docs/DESIGN.md 3.4): the first ripple goes from A to B
    }
    dl::puzzle::LayerGuard guard;
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
    double seconds = 0.0;
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
        const bool resetTest = config.GetBool("debug", "device_reset_key", false) && PressedOnce(VK_F9, resetTestKeyWasDown, focused);
        if (gpu.device->Lost() || resetTest)
        {
            DL_LOG_WARN("Main", "%s", resetTest ? "F9: the GPU objects are made again (test)" : "the device is lost: making it again");
            ++recoveries;
            if (recoveries > kMaxRecoveries || !gpu.Recover(window, config, adapter))
            {
                DL_LOG_ERROR("Main", "the device could not be made again (%d tries)", recoveries);
                MessageBoxA(window.Handle(), "The graphics device was lost and could not be recovered. See logs/dual.log", "DualLayer", MB_OK | MB_ICONERROR);
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
        seconds += dt;

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
            case dl::puzzle::LayerGuard::Action::Switch:
                gpu.models.ToggleLayer();
                break;
            case dl::puzzle::LayerGuard::Action::Refuse:
                gpu.models.ToggleLayer();
                DL_LOG_INFO("Main", "switch refused: the body would be inside something in layer %s", target == 0 ? "A" : "B");
                break;
            case dl::puzzle::LayerGuard::Action::Ignore:
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
            move.sprint = focused && IsDown(VK_SHIFT) && sinking < 0.0f &&
                          config.GetBool("player", "sprint", true);
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

            // E at the handle of a thing: it opens, in A only (the causal rule); its trace comes up in B
            bool used = false;
            if (usePressed && sinking < 0.0f && things.InReach(player.Feet()) >= 0)
            {
                using TR = dl::town::Things::Result;
                const dl::Vec3 balls[3] = {player.Ball(0), player.Ball(1), player.Ball(2)};
                const int which = things.InReach(player.Feet());
                switch (things.Use(player.Feet(), balls, 3, player.GetSettings().radius, solidLayer, moments, traces))
                {
                case TR::Opened:
                    things.ApplyTo(plain);
                    used = true;
                    DL_LOG_INFO("Stage", "%s: done in the past (bells %d of %d)", town.things[static_cast<std::size_t>(which)].name.c_str(),
                                things.BellsRung(), things.BellCount());
                    break;
                case TR::OnlyInPast:
                    used = true;
                    DL_LOG_INFO("Stage", "%s: nothing done in the present changes it", town.things[static_cast<std::size_t>(which)].name.c_str());
                    break;
                case TR::Refused:
                    used = true;
                    DL_LOG_INFO("Stage", "%s not opened: it would close on the body", town.things[static_cast<std::size_t>(which)].name.c_str());
                    break;
                case TR::AlreadyOpen:
                case TR::Nothing:
                    break;
                }
            }

            // the lantern: E sets it down ahead in A, or takes it up within reach (refused when that would leave the body in
            // something or without its floor)
            if (usePressed && !used && sinking < 0.0f && lantern.Far(player.Feet()))
            {
                used = true;
                const auto r = lantern.Recall(player.Feet(), player.Grounded(), solidLayer, player.GetSettings().radius, moments);
                DL_LOG_INFO("Stage", "%s", r == dl::town::Lantern::Result::Taken ? "the lantern called back" : "the lantern not called back: the body stands on its circle");
            }
            if (usePressed && !used && sinking < 0.0f)
            {
                using LR = dl::town::Lantern::Result;
                const LR result = lantern.Use(player.Feet(), player.Grounded(), camera.Forward(), solidLayer, player.GetSettings().radius, moments);
                switch (result)
                {
                case LR::Put:
                    DL_LOG_INFO("Stage", "lantern set down at %.1f %.1f %.1f: a circle of the past", lantern.At().x, lantern.At().y, lantern.At().z);
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

            // into the water: it sinks for a moment, then comes back; below the sea or the town: a fall
            if (sinking < 0.0f && town.Submerged(player.Feet(), solidLayer, moments.GetCircle()))
            {
                sinking = st::kSinkSeconds;
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
            // the last place it stood still on, out of the water, standing in both moments without the circle: a return there
            // never ends in a wall or in the air, whatever is switched and wherever the lantern is
            stood = player.Grounded() && sinking < 0.0f ? stood + dtf : 0.0f;
            const float radius = player.GetSettings().radius;
            if (stood > 0.5f && FitsInTown(player, plain, town, 0) && FitsInTown(player, plain, town, 1) && plain.Supports(player.Feet(), radius, 0) &&
                plain.Supports(player.Feet(), radius, 1))
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
                const float deepest = std::clamp(config.GetFloat("camera", "land_dip_max", kDefaultLandDipMax), 0.0f, 0.5f);
                landDip = std::max(landDip, std::min(impact * perSpeed, deepest));
            }
            landDip *= std::exp(-kLandDipReturn * dtf);
            dl::Vec3 eye = player.Eye();
            const float rise = eye.y - eyeY;
            eyeY = (player.Grounded() && std::fabs(rise) < kEyeEaseMax) ? eyeY + rise * (1.0f - std::exp(-kEyeEase * dtf)) : eye.y;
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
            if (moments.PushOut(c, std::clamp(config.GetFloat("collision", "radius", kDefaultBodyRadius), 0.05f, 1.0f), solidLayer) > 0.0f)
            {
                camera.SetPose(c, camera.Yaw(), camera.Pitch());
            }
        }
        respawn.since += dtf;
        traces.Update(dtf);

        // ---- what is drawn: the town is in the static batch; here only the sea and the ring of a return
        int drawn = st::AddSea(camera.Position(), drawTable, 0, kDrawCapacity, town.sea, -60.0f);
        drawn = lantern.AddDraw(drawTable, drawn, kDrawCapacity);
        drawn = things.AddDraw(drawTable, drawn, kDrawCapacity, traces);
        const dl::town::Circle circle = moments.GetCircle();
        gpu.models.SetLantern(DirectX::XMFLOAT3(circle.centre.x, circle.centre.y, circle.centre.z), circle.radius);
        drawn = st::AddRespawnRing(respawn.at, respawn.since, drawTable, drawn, kDrawCapacity);
        if (orbit)
        {
            const dl::Vec3 feet = player.Feet();
            const float travel = std::hypot(feet.x - feetBeforeMove.x, feet.z - feetBeforeMove.z);
            const float walkSpeed = player.GetSettings().walkSpeed;
            // Use actual travel rather than requested velocity, so pushing into a wall does not animate a run.
            const float moved = dtf > 0.0f && travel <= 2.0f * walkSpeed * dtf ? travel : 0.0f;
            const float targetGait = player.Grounded() && walkSpeed > 0.0f && dtf > 0.0f
                                         ? moved / (walkSpeed * dtf) : 0.0f;
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
        const DirectX::XMMATRIX view = DirectX::XMMatrixLookToLH(DirectX::XMVectorSet(pos.x, pos.y, pos.z, 1.0f), DirectX::XMVectorSet(dir.x, dir.y, dir.z, 0.0f),
                                                                  DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
        float clear[4] = {};
        gpu.models.ClearColor(clear);
        {
            dl::GpuEvent frame(gpu.device->GetAnnotation(), L"Frame");
            gpu.device->BeginFrame(clear);
            const float aspect = static_cast<float>(gpu.device->Width()) / static_cast<float>(gpu.device->Height());
            {
                dl::GpuEvent pass(gpu.device->GetAnnotation(), L"Opaque.Models");
                gpu.models.Draw(gpu.device->GetContext(), view, dl::MakeProjection(aspect, config.GetFloat("camera", "fov_deg", dl::kDefaultFovYDegrees)),
                                DirectX::XMFLOAT3(pos.x, pos.y, pos.z));
            }
            {
                char status[160];
                std::snprintf(status, sizeof(status), "%s", town.NearestPlace(body.x, body.z));
                // the only prompt of the town: what E does here (a handle of the past first, then the lantern)
                std::array<dl::prompt::Shown, 2> none{};
                const int handle = things.InReach(player.Feet());
                if (walking && sinking < 0.0f)
                {
                    if (handle >= 0 && !things.Open(static_cast<std::size_t>(handle)) && gpu.models.CollisionLayer() == 0)
                    {
                        static constexpr dl::prompt::Kind kOfVerb[5] = {dl::prompt::Kind::Open, dl::prompt::Kind::Push, dl::prompt::Kind::Lay,
                                                                        dl::prompt::Kind::Ring, dl::prompt::Kind::Lower};
                        const int verb = static_cast<int>(town.things[static_cast<std::size_t>(handle)].verb);
                        none[0] = dl::prompt::Shown{kOfVerb[verb >= 0 && verb < 5 ? verb : 0], 1.0f};
                    }
                    else if (lantern.Far(player.Feet()))
                    {
                        none[0] = dl::prompt::Shown{dl::prompt::Kind::Recall, dl::prompt::kLearnedAlpha};
                    }
                    else if (!lantern.Carried() && lantern.InReach(player.Feet()))
                    {
                        none[0] = dl::prompt::Shown{dl::prompt::Kind::Take, 1.0f};
                    }
                    else if (lantern.Carried() && gpu.models.CollisionLayer() == 0 && player.Grounded())
                    {
                        none[0] = dl::prompt::Shown{dl::prompt::Kind::PutDown, 0.6f};
                    }
                }
                const float under = sinking >= 0.0f ? 1.0f : 0.0f;
                BuildHud(hud, gpu.device->Width(), gpu.device->Height(), none, status, nullptr, st::Resonance{}, static_cast<float>(seconds), under, !orbit);
                dl::GpuEvent pass(gpu.device->GetAnnotation(), L"Overlay");
                gpu.overlay.Draw(gpu.device->GetContext(), hud, gpu.device->Width(), gpu.device->Height());
            }
        }
        gpu.device->EndFrame();

        fpsTimer += dt;
        ++fpsFrames;
        if (fpsTimer >= 0.5)
        {
            const wchar_t* mode = collide ? (walk ? L"collide | walk" : L"collide | fly") : L"noclip";
            wchar_t title[384];
            swprintf_s(title, L"DualLayer | %s | %d x %d | %.1f FPS (%.2f ms) | body %.1f %.1f %.1f | %s | %s | town: %hs | bells %d of %d | falls %d%s",
                       gpu.device->AdapterName().c_str(), gpu.device->Width(), gpu.device->Height(), fpsFrames / fpsTimer, 1000.0 * fpsTimer / fpsFrames,
                       body.x, body.y, body.z, gpu.models.StatusLine().c_str(), mode, town.NearestPlace(body.x, body.z), things.BellsRung(), things.BellCount(), falls,
                       window.MouseCaptured() ? L"" : L" | click for mouse look");
            window.SetTitle(title);
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

    // ---- which map: the town of the model (docs/DESIGN.md 12, step 2) or the greybox of the previous map ([map] town = false)
    static dl::town::TownData town; // WHY static: a few MB, kept for the whole run
    const bool townMode = config.GetBool("map", "town", true);
    if (townMode)
    {
        std::string error;
        if (!town.Load(dl::town::kPath, error))
        {
            DL_LOG_ERROR("Main", "the town could not be loaded: %s", error.c_str());
            MessageBoxA(window.Handle(), "The town could not be loaded. See logs/dual.log", "DualLayer", MB_OK | MB_ICONERROR);
            return 1;
        }
        DL_LOG_INFO("Main", "town: %zu polygons, %zu materials, %zu places", town.polygons.size(), town.materials.size(), town.places.size());
    }

    Gpu gpu;
    gpu.town = townMode ? &town : nullptr;
    if (!gpu.Init(window, config, adapter))
    {
        MessageBoxA(window.Handle(), "Initialisation failed. See logs/dual.log", "DualLayer", MB_OK | MB_ICONERROR);
        gpu.Shutdown();
        return 1;
    }
    if (townMode)
    {
        const int code = RunTown(window, gpu, config, camera, adapter, town);
        DL_LOG_INFO("Main", "shutdown");
        gpu.Shutdown();
        dl::log::Close();
        return code;
    }

    // ---- the map: its puzzle, the guard of the layer switch, the room the body is in, the seals, the hidden lights
    dl::puzzle::Puzzle puzzle;
    if (!puzzle.Init(dl::stage::MakeStage()))
    {
        DL_LOG_ERROR("Main", "the map could not be set up (scene/Stage.h)");
        MessageBoxA(window.Handle(), "The map could not be set up. See logs/dual.log", "DualLayer", MB_OK | MB_ICONERROR);
        gpu.Shutdown();
        return 1;
    }
    dl::puzzle::LayerGuard guard;
    dl::stage::SealSet seals;   // once taken, taken for the rest of the run
    dl::stage::LightSet lights; // the same
    int room = dl::stage::kStartRoom;
    // WHY static: the renderer and the body keep pointers to these tables until the next frame, and they are too large for
    // the stack of a function that runs for the whole program
    static dl::scene::SceneObject drawTable[dl::stage::kMaxDrawObjects] = {};
    static dl::scene::SceneObject solidTable[kMaxSolids] = {};
    static dl::scene::SceneObject solidsA[kMaxSolids] = {}; // the solid tables of each layer, for the orbs
    static dl::scene::SceneObject solidsB[kMaxSolids] = {};
    std::array<dl::shapes::Lift, dl::stage::kLiftCount> lifts = dl::stage::MakeLifts(); // the shortcuts that move
    dl::canal::Canal canal; // the canal and lock of the east district (scene/Canal.h)
    canal.Init(dl::stage::MakeCanal());
    dl::stage::Waterworks waters; // the water of A; the master sluice drains it at the end
    dl::stage::CityState city;    // what the bells have changed (scene/CityState.h)
    WaterState water;             // foam, falls, wet walls, splashes, the body sinking (scene/WaterFx.h)
    dl::stage::LampTravel travel; // the list of the lit lamps (scene/LampTravel.h)
    dl::stage::Ending ending;     // the last moment and its ending (scene/Ending.h)
    dl::stage::LockCrate lockCrate; // the crate of the tower lock (scene/LockCrate.h)
    bool travelLeftWasDown = false;
    bool travelRightWasDown = false;
    int bellsRung = 0;
    dl::stage::AnchorSet anchors; // the anchor lamps: where a fall or R comes back (scene/Anchors.h)
    Respawn respawn;
    Respawn orbReturn; // the ring where an orb came back to its start (scene/OrbReturn.h)
    int falls = 0;     // falls and R, for the end screen
    dl::mech::MechanismSet mech; // every moving part of the city (scene/CityParts.h)
    if (!mech.Init(dl::stage::CityParts()))
    {
        DL_LOG_ERROR("Main", "the moving parts could not be set up (scene/CityParts.h)");
        MessageBoxA(window.Handle(), "The map could not be set up. See logs/dual.log", "DualLayer", MB_OK | MB_ICONERROR);
        gpu.Shutdown();
        return 1;
    }

    static dl::OverlayBatch hud; // WHY static: 288 KB, too large for the stack
    dl::prompt::Tracker prompts; // how often each prompt was followed: a learned one is dimmed
    double secondsInRoom = 0.0;  // UNITS: seconds since the body came into the room it is in (R is offered after a while)

    dl::Player player;
    ApplyPlayerConfig(config, player);
    player.SetLand(&dl::stage::TheLand()); // the ground of the world (scene/Landscape.h): solid in both layers
    player.SetSpawn(dl::stage::kRooms[room].entry);
    player.Respawn();
    camera.SetPose(player.Eye(), camera.Yaw(), camera.Pitch()); // the first frame already looks from the start

    LARGE_INTEGER frequency{};
    LARGE_INTEGER last{};
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&last);
    double fpsTimer = 0.0;
    int fpsFrames = 0;
    float angle = 0.0f;           // radians
    float landDip = 0.0f;         // UNITS: metres the eye is lowered after a hard landing (it comes back up by itself)
    float eyeY = player.Eye().y;  // UNITS: metres, the height of the eye as the camera shows it (eased, see kEyeEase)
    double stageSeconds = 0.0;    // UNITS: seconds since the start
    double clearSeconds = -1.0;   // the time of the clear; negative while the goal is not reached
    double sinceClear = -1.0;     // UNITS: seconds since the clear (the rings and the beacon use it); negative before
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
    bool useKeyWasDown = false;
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
            if (gpu.town != nullptr)
            {
                ApplyTownThemes(gpu.models, *gpu.town);
            }
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
        else
        {
            sinceClear += dt;
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
        const bool usePressed = PressedOnce('E', useKeyWasDown, focused); // walking: take or put the orb (flying: E is up)
        const bool walking = walk && collide; // a body that walks needs something to stand on

        // ---- the layer switch, through the guard. WHY not while Alt is down: Alt + Tab switches windows and must not also
        // switch the layer
        if (PressedOnce(VK_TAB, layerKeyWasDown, focused && !IsDown(VK_MENU) && water.sinking < 0.0f)) // (no way out of water by a switch)
        {
            const int solidNow = guard.Refusing() ? guard.HeldLayer() : gpu.models.CollisionLayer();
            const int target = 1 - gpu.models.CollisionLayer();
            // WHY the whole solid table of the target layer (not only the map of the puzzle): a switch into a gate of a wing,
            // a lift or the crate must be refused as well (solidsA is scratch here; it is built again after the body moves)
            const int targetCount = BuildSolidTable(puzzle, seals, lights, lockCrate, mech, lifts, canal, target, solidsA);
            // ... and into the water of the other world (scene/Recovery.h): the body would only come up again at the entry
            const bool fits = !walking || dl::stage::SwitchAllowed(player, target, gpu.models.SpinSeconds(), solidsA, targetCount, waters, canal);
            switch (guard.OnSwitchRequest(solidNow, gpu.models.LayerSwitching(), fits))
            {
            case dl::puzzle::LayerGuard::Action::Switch:
                gpu.models.ToggleLayer();
                prompts.Used(dl::prompt::Kind::SwitchWorld);
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
        const bool replay = resetKeyDown && !resetKeyWasDown && ending.Done() && sinceClear >= static_cast<double>(dl::endscreen::kDelay);
        if (replay)
        {
            // the last moment again (scene/Ending.h): the waters full, the body on the balcony, which starts it at once
            ending.Reset();
            clearSeconds = -1.0;
            sinceClear = -1.0;
            waters.Reopen();
            player.SetSpawn({0.0f, dl::stage::kSummitY, 28.0f});
            player.Respawn();
            respawn = Respawn{player.Feet(), 0.0f};
            camera.SetPose(player.Eye(), camera.Yaw(), camera.Pitch());
            DL_LOG_INFO("Stage", "the last moment again");
        }
        else if (resetKeyDown && !resetKeyWasDown)
        {
            const dl::stage::Room& here = dl::stage::kRooms[room];
            if (room == dl::stage::RoomIndex("east hall") || room == dl::stage::RoomIndex("east quay") || room == dl::stage::RoomIndex("canal bed"))
            {
                canal.Reset(); // R by the canal puts back the water, the weir and the crate (docs/LEVEL_DESIGN.md, 14.4)
            }
            landDip = 0.0f;
            if (room == dl::stage::RoomIndex("tower lock") || room == dl::stage::RoomIndex("water tower"))
            {
                lockCrate.Reset(); // R by the tower lock puts its crate back at its start
            }
            if (room == dl::stage::RoomIndex("courtyard"))
            {
                puzzle.Reset(); // R in the courtyard also puts the block back: a block in a dead end never ends the game
            }
            mech.ResetRoom(room); // and the orbs of the room go back (what is latched stays)
            prompts.Used(dl::prompt::Kind::ResetRoom);
            secondsInRoom = 0.0;
            player.SetSpawn(dl::stage::SafeRespawn(room, gpu.models.CollisionLayer(), anchors, waters, canal));
            player.Respawn();
            respawn = Respawn{player.Feet(), 0.0f};
            ++falls;
            camera.SetPose(player.Eye(), camera.Yaw(), camera.Pitch());
            DL_LOG_INFO("Main", "R: back to the %s of the %s%s", dl::stage::AnchorSet::AnchorOf(room) >= 0 && anchors.Lit(dl::stage::AnchorSet::AnchorOf(room)) ? "lamp" : "entry",
                        here.name, here.puzzle ? ", its puzzle is put back" : "");
        }
        resetKeyWasDown = resetKeyDown;
        gpu.models.Update(dtf);

        // the layer whose objects are solid: the one the screen is going to show, or the old one while a refusal plays
        const int solidLayer = guard.Refusing() ? guard.HeldLayer() : gpu.models.CollisionLayer();
        if (walking)
        {
            // the lifts move first and carry a body that stands on one; then the solid table holds them where they are now
            dl::stage::UpdateLifts(lifts, player, [&seals](int seal) { return seals.Taken(seal); }, dtf);
            if (canal.Update(player, solidLayer, dtf)) // the lock wheel turns only in A; the crate of A carries a body on it
            {
                DL_LOG_INFO("Stage", "the lock wheel is turned: the canal fills");
            }
        }
        if (respawn.since >= 0.0f)
        {
            respawn.since += dtf;
        }
        if (orbReturn.since >= 0.0f)
        {
            orbReturn.since += dtf;
        }
        // what the bells have changed (scene/CityState.h): the water of A, the machinery, the night
        city.FromSeals(seals);
        city.Update(dtf);
        if (city.Count() != bellsRung)
        {
            DL_LOG_INFO("Stage", "a bell has rung (%d of %d): water +%.1f m, machinery %s, night %s", city.Count(), dl::stage::kBellCount,
                        static_cast<double>(city.WaterRise()), city.Machinery() ? "on" : "off", city.Night() ? "falls" : "not yet");
            bellsRung = city.Count();
        }
        waters.SetRaise(city.RiseNow());
        canal.SetMachinery(city.Machinery());
        gpu.models.SetNight(city.NightNow());
        waters.Update(dtf);
        canal.SetDrained(waters.Drained());
        lockCrate.SetWaterTop(waters.Top(dl::stage::kTowerLockWaters[0]));
        water.count = dl::stage::CollectWaters(waters, canal, water.sheets.data(), dl::stage::kMaxSheets);
        water.wet.Update(water.sheets.data(), water.count, dtf); // a surface that went down leaves its walls wet
        water.splashes.Update(dtf);
        const int solidCount = BuildSolidTable(puzzle, seals, lights, lockCrate, mech, lifts, canal, solidLayer, solidTable);

        // ---- the body or the ball: after the camera and the shapes have moved, keep it out of what is solid
        if (walking)
        {
            // lamp travel (scene/LampTravel.h): while the list is open the body stands; A and D choose, E goes
            const auto lampFree = [&](const dl::Vec3& at) {
                dl::Player probe = player;
                probe.SetSpawn(at);
                probe.Respawn();
                return dl::puzzle::Puzzle::FitsIn(probe, solidLayer, gpu.models.SpinSeconds(), solidTable, solidCount);
            };
            travel.Follow(anchors, player.Feet());
            bool lampUsed = false;
            if (travel.Open())
            {
                const bool left = PressedOnce('A', travelLeftWasDown, focused);
                const bool right = PressedOnce('D', travelRightWasDown, focused);
                if (left || right)
                {
                    travel.Step(anchors, right ? +1 : -1, lampFree);
                }
                if (usePressed)
                {
                    const int to = travel.Selected();
                    player.SetSpawn(travel.Go());
                    player.Respawn();
                    respawn = Respawn{player.Feet(), 0.0f};
                    DL_LOG_INFO("Stage", "lamp travel: to %s", dl::stage::kAnchors[to].name);
                    lampUsed = true;
                }
                else if (jumpPressed || keys.moveForward != 0.0f)
                {
                    travel.Close();
                }
            }
            else if (usePressed && travel.TryOpen(anchors, player.Feet(), city.Count(), lampFree))
            {
                lampUsed = true;
                DL_LOG_INFO("Stage", "lamp travel: the list is open");
            }
            dl::Player::Input move;
            const bool still = travel.Open() || water.sinking >= 0.0f; // the list of lamps is open, or the body sinks
            move.moveForward = still ? 0.0f : keys.moveForward;
            move.moveRight = still ? 0.0f : keys.moveRight;
            move.jumpPressed = jumpPressed && !still && !lampUsed;
            move.jumpHeld = focused && IsDown(VK_SPACE);
            player.SetScene(solidTable, solidCount);
            player.Update(move, camera.Forward(), solidLayer, gpu.models.SpinSeconds(), dtf);
            puzzle.Update(player, dtf);
            lockCrate.Update(player, solidLayer, player.GetSettings().radius, dtf);
            if (usePressed && !lampUsed)
            {
                int which = -1;
                dl::Vec3 spot{};
                const bool wouldPlace = mech.SpotAhead(player, solidLayer, camera.Forward(), spot) != dl::mech::MechanismSet::kSpotNone;
                const dl::mech::Action action = mech.Interact(player, solidLayer, camera.Forward(), solidTable, solidCount, which);
                if (action == dl::mech::Action::Took)
                {
                    prompts.Used(dl::prompt::Kind::Take);
                }
                else if (action == dl::mech::Action::Put)
                {
                    prompts.Used(wouldPlace ? dl::prompt::Kind::Place : dl::prompt::Kind::PutDown);
                }
                if (action != dl::mech::Action::Nothing)
                {
                    DL_LOG_INFO("Stage", "orb %d %s", which,
                                action == dl::mech::Action::Took ? "taken" : (action == dl::mech::Action::Put ? "put down" : "not put down: something is in the way"));
                }
            }
            const int bodyA = BuildSolidTable(puzzle, seals, lights, lockCrate, mech, lifts, canal, 0, solidsA);
            const int countA = canal.AddWaterForOrbs(solidsA, dl::stage::AddWaterForOrbs(solidsA, bodyA, kMaxSolids, waters), kMaxSolids); // orbs float
            const int countB = BuildSolidTable(puzzle, seals, lights, lockCrate, mech, lifts, canal, 1, solidsB);
            mech.Update(player, solidLayer, solidsA, countA, solidsB, countB, dtf);
            // an orb that came to rest where its world cannot take it back goes back to its start (scene/OrbReturn.h)
            const int stranded = dl::stage::ReturnStrandedOrbs(mech, solidsA, bodyA, solidsB, countB, canal);
            if (stranded >= 0)
            {
                DL_LOG_INFO("Stage", "orb %d was out of reach of its world: back at its start", stranded);
            }
            for (int i = 0; i < mech.OrbCount(); ++i)
            {
                if (mech.ConsumeReturned(i))
                {
                    const dl::Vec3 start = mech.StartOf(i);
                    orbReturn = Respawn{{start.x, start.y - dl::mech::kOrbRadius, start.z}, 0.0f};
                }
            }
            for (int t = 0; t < mech.TriggerCount(); ++t)
            {
                if (mech.Changed(t))
                {
                    DL_LOG_INFO("Stage", "%s %s", mech.TriggerAt(t).name, mech.Active(t) ? "on" : "off");
                }
            }
            // the room: the one the body stands in; a fall below the map or under water goes back to its entry. WHY before the
            // seals and the lights: a body that went under water in this frame must not take what lies below the surface
            const int standing = dl::stage::CurrentRoom(player.Feet(), player.Grounded(), room);
            secondsInRoom += dt;
            if (standing != room)
            {
                room = standing;
                secondsInRoom = 0.0;
                DL_LOG_INFO("Stage", "room: %s", dl::stage::kRooms[room].name);
            }
            // into water: a splash where it went in, and the body sinks for a moment (the view goes under) before it comes back
            const bool inWater = dl::stage::Submerged(player.Feet(), solidLayer, waters) || canal.Submerged(player.Feet(), solidLayer);
            if (inWater && water.sinking < 0.0f)
            {
                float top = player.Feet().y;
                if (dl::stage::WaterTopAt(water.sheets.data(), water.count, player.Feet().x, player.Feet().z, player.Feet().y, top))
                {
                    water.splashes.Spawn({player.Feet().x, top, player.Feet().z});
                }
                water.sinking = dl::stage::kSinkSeconds;
                DL_LOG_INFO("Main", "into the water");
            }
            bool sunk = false;
            if (water.sinking >= 0.0f)
            {
                water.sinking -= dtf;
                sunk = water.sinking < 0.0f;
            }
            if (sunk || (water.sinking < 0.0f && (player.Feet().y < dl::stage::FallLine(room) || dl::stage::OnOpenGround(player.Feet(), player.Grounded()) ||
                                                  player.JustRespawned())))
            {
                water.sinking = -1.0f;
                player.SetSpawn(dl::stage::SafeRespawn(room, solidLayer, anchors, waters, canal)); // the lamp, else the entry, out of the water
                player.Respawn();
                respawn = Respawn{player.Feet(), 0.0f};
                ++falls;
                landDip = 0.0f;
                DL_LOG_INFO("Main", "fell: back at the %s", dl::stage::kRooms[room].name);
            }
            if (city.RingAt(player))
            {
                DL_LOG_INFO("Stage", "a bell is rung where it hangs (%d of %d rung)", city.Count(), dl::stage::kBellCount);
            }
            const int lamp = anchors.Update(player);
            if (lamp >= 0)
            {
                DL_LOG_INFO("Stage", "anchor lamp lit: %s", dl::stage::kAnchors[lamp].name);
            }
            const int seal = seals.Collect(player);
            if (seal >= 0)
            {
                DL_LOG_INFO("Stage", "seal %d of %d: %s%s", seals.Count(), dl::stage::kSealCount, dl::stage::kSeals[seal].name,
                            seals.TowerOpen() ? " - the cathedral is open" : "");
            }
            const int light = lights.Collect(player);
            if (light >= 0)
            {
                DL_LOG_INFO("Stage", "hidden light %d of %d: %s", lights.Count(), dl::stage::kHiddenLightCount,
                            dl::stage::kHiddenLights[light].name);
                for (const dl::stage::Fragment& f : dl::stage::kFragments)
                {
                    if (f.light == light)
                    {
                        DL_LOG_INFO("Stage", "a memory comes back in B: %s", f.name);
                    }
                }
            }

            // the last moment (scene/Ending.h): the goal starts three strokes of the great bell; the world at the third decides
            if (clearSeconds < 0.0 && !ending.Running() && !ending.Done() && dl::stage::ReachedGoal(player))
            {
                ending.Start();
                DL_LOG_INFO("Stage", "the last moment: the great bell strikes");
            }
            const int stroke = ending.Update(dtf, solidLayer, focused && IsDown(VK_TAB), city.Count());
            if (stroke > 0)
            {
                respawn = Respawn{player.Feet(), 0.0f}; // a ring with each stroke
                DL_LOG_INFO("Stage", "stroke %d of %d, in layer %c", stroke, dl::stage::Ending::kStrokes, solidLayer == 0 ? 'A' : 'B');
            }
            if (clearSeconds < 0.0 && ending.Done())
            {
                clearSeconds = stageSeconds;
                sinceClear = 0.0;
                if (dl::stage::Ending::Drains(ending.Kind()))
                {
                    waters.Close(); // time moves on: the master sluice drains every water of A (docs/LEVEL_DESIGN.md, 14.13)
                }
                DL_LOG_INFO("Stage", "ending: %s; clear in %.1f s, hidden lights %d of %d, falls %d", dl::stage::Ending::Title(ending.Kind()), clearSeconds,
                            lights.Count(), dl::stage::kHiddenLightCount, falls);
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
            // the eye follows the body smoothly up and down ramps and steps, so that the many small corrections of the
            // collision do not shake the picture; in the air (a jump, a fall) it follows at once
            const float rise = eye.y - eyeY;
            eyeY = (player.Grounded() && std::fabs(rise) < kEyeEaseMax) ? eyeY + rise * (1.0f - std::exp(-kEyeEase * dtf)) : eye.y;
            eye.y = eyeY - landDip;
            camera.SetPose(eye, camera.Yaw(), camera.Pitch()); // only the place changes, not where it looks
        }
        else if (collide)
        {
            landDip = 0.0f;
            const float radius = std::clamp(config.GetFloat("collision", "radius", kDefaultBodyRadius), 0.05f, 1.0f);
            const dl::CollisionResult body =
                dl::ResolveSphere(camera.Position(), radius, solidLayer, gpu.models.SpinSeconds(), solidTable, solidCount);
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

        // ---- what is drawn: the map as it is now
        // the fixed city (the first StaticDrawCount entries of the table) is in the static batch: only the rest is given
        const int drawn = BuildFrameTable(puzzle, seals, lights, mech, lifts, canal, waters, lockCrate, anchors, respawn, orbReturn, player, camera.Position(), camera.Forward(), config,
                                         static_cast<float>(sinceClear), room, static_cast<float>(secondsInRoom), static_cast<float>(stageSeconds),
                                         water, drawTable);
        const int fixedCount = dl::stage::StaticDrawCount();
        gpu.models.SetObjects(drawTable + fixedCount, drawn - fixedCount);

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
                gpu.models.Draw(gpu.device->GetContext(), view, dl::MakeProjection(aspect, config.GetFloat("camera", "fov_deg", dl::kDefaultFovYDegrees)), DirectX::XMFLOAT3(pos.x, pos.y, pos.z));
            }
            if (showTriangle)
            {
                dl::GpuEvent pass(gpu.device->GetAnnotation(), L"Opaque.TestTriangle");
                gpu.triangle.Draw(gpu.device->GetContext(), aspect, angle, view);
            }
            {
                // ---- the overlay: what E would do now, the first lesson (TAB), R when stuck, and the status line
                dl::prompt::Input in;
                in.walking = walking;
                in.holding = mech.HeldOrb() >= 0;
                dl::Vec3 spot{};
                int otherWorld = dl::mech::MechanismSet::kSpotNone;
                in.wouldPlace = in.holding && mech.SpotAhead(player, solidLayer, camera.Forward(), spot, &otherWorld) != dl::mech::MechanismSet::kSpotNone;
                in.otherWorldSpot = in.holding && otherWorld != dl::mech::MechanismSet::kSpotNone;
                in.orbInReach = !in.holding && mech.OrbInReach(player, solidLayer, solidTable, solidCount) >= 0;
                in.puzzleRoom = dl::stage::kRooms[room].puzzle;
                in.secondsInRoom = static_cast<float>(secondsInRoom);
                std::array<dl::prompt::Shown, 2> shown = prompts.Choose(in);
                if (!config.GetBool("hints", "prompts", true))
                {
                    shown = {};
                }
                char status[128];
                std::snprintf(status, sizeof(status), "%s  *  SEALS %d/%d  *  LIGHTS %d/%d%s", dl::stage::kRooms[room].name, seals.Count(),
                              dl::stage::kSealCount, lights.Count(), dl::stage::kHiddenLightCount, clearSeconds >= 0.0 ? "  *  CLEAR" : "");
                const dl::endscreen::Screen end =
                    dl::endscreen::Make(clearSeconds, lights.Count(), dl::stage::kHiddenLightCount, falls, dl::stage::Ending::Title(ending.Kind()));
                const bool showEnd = clearSeconds >= 0.0 && sinceClear >= static_cast<double>(dl::endscreen::kDelay);
                const dl::endscreen::Screen lampList = travel.Panel();
                const dl::stage::Resonance resonance = dl::stage::ResonanceAt(lights, city.Count(), player.Feet(), camera.Forward());
                const dl::Vec3 eyeNow = camera.Position();
                const float underwater = dl::stage::Underwater(water.sheets.data(), water.count, eyeNow, gpu.models.CollisionLayer()) ? 1.0f : 0.0f;
                BuildHud(hud, gpu.device->Width(), gpu.device->Height(), shown, status, showEnd ? &end : (travel.Open() ? &lampList : nullptr), resonance,
                         static_cast<float>(stageSeconds), underwater);
                dl::GpuEvent pass(gpu.device->GetAnnotation(), L"Overlay");
                gpu.overlay.Draw(gpu.device->GetContext(), hud, gpu.device->Width(), gpu.device->Height());
            }
        }
        gpu.device->EndFrame();

        fpsTimer += dt;
        ++fpsFrames;
        if (fpsTimer >= 0.5)
        {
            const wchar_t* mode = collide ? (walk ? L"collide | walk" : L"collide | fly") : L"noclip";
            wchar_t stageText[128];
            if (clearSeconds >= 0.0)
            {
                swprintf_s(stageText, L"CLEAR %.1f s | lights %d/%d", clearSeconds, lights.Count(), dl::stage::kHiddenLightCount);
            }
            else
            {
                swprintf_s(stageText, L"%hs | %.0f s | seals %d/%d | lights %d/%d", dl::stage::kRooms[room].name, stageSeconds,
                           seals.Count(), dl::stage::kSealCount, lights.Count(), dl::stage::kHiddenLightCount);
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
