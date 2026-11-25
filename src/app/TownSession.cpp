#include "app/TownSession.h"

#include "app/AppConfig.h"
#include "app/Input.h"
#include "core/Log.h"
#include "render/GpuDebug.h"
#include "render/Hud.h"
#include "render/Projection.h"
#include "scene/Emblems.h"
#include "scene/Prompts.h"
#include "scene/TownEffects.h"
#include "scene/TownPlayer.h"

#include <DirectXMath.h>

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace dl
{

namespace
{

constexpr int kMaxRecoveries = 5;                   // after this many lost devices in one run the program gives up
constexpr float kDefaultMouseSensitivity = 0.0022f; // UNITS: radians per count of the mouse. TUNE: [mouse] sensitivity
constexpr float kDefaultLandDip =
    0.012f; // UNITS: metres of dip per m/s of landing speed. TUNE: [camera] land_dip, 0 = off
constexpr float kDefaultLandDipMax = 0.15f; // UNITS: metres, the deepest dip. TUNE: [camera] land_dip_max
constexpr float kDefaultBodyRadius =
    0.35f;                              // UNITS: metres, the ball of the camera. TUNE: [collision] radius, 0.05 .. 1
constexpr float kLandDipReturn = 14.0f; // UNITS: 1 / seconds; the eye comes back up in about 0.2 s
constexpr float kEyeEase = 30.0f;   // UNITS: 1 / seconds; small changes of the eye height are eased over about 0.05 s
constexpr float kEyeEaseMax = 0.5f; // UNITS: metres; a larger change (a respawn, a teleport) is followed at once
constexpr float kSeaNorthEdge = -60.0f; // UNITS: metres (z): the sea ends here to the north, the town stands beyond it
constexpr float kSwitchOfferSeconds =
    8.0f; // UNITS: seconds of walking before TAB is offered to a player who has not switched
constexpr float kMapOfferSeconds =
    40.0f; // UNITS: seconds of walking before M is offered to a player who has not opened the map
constexpr float kMarkerHeight = 16.0f; // UNITS: metres: a marker stands in the world as a column of light this tall
constexpr float kMarkerGrab = 10.0f; // UNITS: pixels: a marker this close to the pointer is the one that is taken away
static_assert(kFarPlane <= town::kViewDistance,
              "scene/TownEffects.h kViewDistance must reach the far plane: the sea is sized by it");

std::int64_t Counter()
{
    LARGE_INTEGER value{};
    QueryPerformanceCounter(&value);
    return value.QuadPart;
}

std::int64_t CounterFrequency()
{
    LARGE_INTEGER value{};
    QueryPerformanceFrequency(&value);
    return value.QuadPart;
}

/// True when the body fits in `layer` of the town (with the circle of the lantern): none of its three balls overlaps a
/// surface (a hair less than its radius, so that standing on a floor fits) and its feet are not under the water of that
/// layer.
bool FitsInTown(const Player& player, const town::Moments& moments, const town::TownData& town, int layer)
{
    const float r = player.GetSettings().radius;
    for (int i = 0; i < 3; ++i)
    {
        Vec3 c = player.Ball(i);
        if (moments.PushOut(c, r - 0.03f, layer) > 0.005f)
        {
            return false;
        }
    }
    return !town.Submerged(player.Feet(), layer, moments.GetCircle());
}

/// What E does when it is pressed on the ground: a thing of the past within reach first (it opens in A only, the causal
/// rule; its trace comes up in B), otherwise the lantern (called back from afar, set down ahead in A, or taken up).
/// Every outcome goes to the log; a refusal never changes the state.
void HandleUse(const town::TownData& town, town::Things& things, town::Traces& traces, town::Lantern& lantern,
               town::Moments& moments, town::Moments& plain, const Player& player, const Camera& camera, int solidLayer)
{
    bool used = false;
    const int which = things.InReach(player.Feet());
    if (which >= 0)
    {
        using TR = town::Things::Result;
        const Vec3 balls[3] = {player.Ball(0), player.Ball(1), player.Ball(2)};
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
                    r == town::Lantern::Result::Taken ? "the lantern called back"
                                                      : "the lantern not called back: the body stands on its circle");
        return;
    }
    using LR = town::Lantern::Result;
    switch (lantern.Use(player.Feet(), player.Grounded(), camera.Forward(), solidLayer, player.GetSettings().radius,
                        moments))
    {
    case LR::Put:
        DL_LOG_INFO("Stage", "lantern set down at %.1f %.1f %.1f: a circle of the past",
                    static_cast<double>(lantern.At().x), static_cast<double>(lantern.At().y),
                    static_cast<double>(lantern.At().z));
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
/// sinking. `layer`: the moment on the screen (0 = A). `offerSwitch`: the player has not used TAB yet. `offerMap`: nor
/// opened the map.
prompt::Shown ChoosePrompt(const town::TownData& town, const town::Things& things, const town::Lantern& lantern,
                           const Player& player, int layer, bool active, bool offerSwitch, bool offerMap)
{
    prompt::Input in;
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
    in.offerSwitch = offerSwitch;
    in.offerMap = offerMap;
    return prompt::Choose(in);
}

} // namespace

TownSession::TownSession(Window& window, Gpu& gpu, Config& config, Camera& camera, int adapter,
                         const town::TownData& town)
    : m_window(window), m_gpu(gpu), m_config(config), m_camera(camera), m_adapter(adapter), m_town(town),
      m_meshes{town.BuildMesh(0), town.BuildMesh(1)}, m_moments(&m_meshes[0], &m_meshes[1]),
      m_plain(&m_meshes[0], &m_meshes[1]), m_hud(std::make_unique<OverlayBatch>())
{
    DL_LOG_INFO("Main", "town solid: %d triangles in A, %d in B", m_meshes[0].TriangleCount(),
                m_meshes[1].TriangleCount());
    ApplyPlayerConfig(m_config, m_player);
    m_things.Init(m_town);
    m_things.ApplyTo(m_moments);
    m_things.ApplyTo(m_plain);
    m_guidance.Init(m_town, ReadGuidanceSettings(m_config));
    m_memory.Init(m_town.plan, ReadMapRevealRadius(m_config));
    m_player.SetTown(&m_moments);
    m_player.SetSpawn(m_town.spawn);
    m_player.Respawn();
    m_memory.Update(m_town, m_player.Feet());
    // the first frame: where the town file says (docs/DESIGN.md 10.6)
    const bool third = m_config.GetBool("player", "third_person", true);
    m_camera.SetPose(m_player.Eye(), m_town.spawnYaw, third ? m_town.spawnPitch : 0.0f);
    if (m_town.startLayer == 1)
    {
        m_gpu.models
            .ToggleLayer(); // the game opens in the present (docs/DESIGN.md 3.4): the first ripple goes from A to B
    }
    m_safe = m_town.spawn;
    m_eyeY = m_player.Eye().y;
    m_avatarYaw = m_town.spawnYaw;
    m_collide = m_config.GetBool("collision", "enabled", true);
    m_walk = m_config.GetBool("player", "walk", true);
    m_wasWalking = m_walk && m_collide;
    m_frequency = CounterFrequency();
    m_last = Counter();
}

int TownSession::Run()
{
    while (m_window.PumpMessages())
    {
        Frame frame;
        frame.focused = GetForegroundWindow() == m_window.Handle();
        const bool resetTest = m_config.GetBool("debug", "device_reset_key", false) &&
                               input::PressedOnce(VK_F9, m_deviceResetDown, frame.focused);
        if (m_gpu.device->Lost() || resetTest)
        {
            if (!RecoverDevice(resetTest))
            {
                return 1;
            }
            continue;
        }
        if (m_window.ConsumeReloadKey())
        {
            ReloadConfig();
        }
        if (m_window.ConsumeResized())
        {
            m_gpu.device->Resize(m_window.Width(), m_window.Height());
        }
        if (m_window.IsMinimized())
        {
            Sleep(50);
            continue;
        }
        frame.seconds = NextFrameSeconds();
        frame.dt = static_cast<float>(frame.seconds);

        if (!m_mapOpen)
        {
            LookWithMouse();
        }
        ReadKeys(frame);
        MapKeys(frame);
        if (m_mapOpen)
        {
            m_gpu.models.Update(0.0f); // the game stands still behind the paper
            frame.solidLayer = m_gpu.models.CollisionLayer();
        }
        else
        {
            SwitchMoment(frame);
            UpdateCamera(frame);
            m_gpu.models.Update(frame.dt);
            frame.solidLayer = m_guard.Refusing() ? m_guard.HeldLayer() : m_gpu.models.CollisionLayer();
            frame.feetBeforeMove = m_player.Feet();
            if (frame.walking)
            {
                MoveBody(frame);
            }
            else if (m_collide)
            {
                PushFlyingCamera(frame);
            }
            m_respawn.since += frame.dt;
            m_traces.Update(frame.dt);
            m_clock += frame.dt;
            UpdateGuidance(frame);
        }
        BuildDrawTable(frame);
        Render(frame);
        UpdateTitle(frame);
    }
    return 0;
}

bool TownSession::RecoverDevice(bool resetTest)
{
    DL_LOG_WARN("Main", "%s",
                resetTest ? "F9: the GPU objects are made again (test)" : "the device is lost: making it again");
    ++m_recoveries;
    if (m_recoveries > kMaxRecoveries || !m_gpu.Recover(m_window, m_config, m_adapter))
    {
        DL_LOG_ERROR("Main", "the device could not be made again (%d tries)", m_recoveries);
        MessageBoxA(m_window.Handle(), "The graphics device was lost and could not be recovered. See logs/dual.log",
                    "DualLayer", MB_OK | MB_ICONERROR);
        return false;
    }
    DL_LOG_INFO("Main", "recovered (%d of at most %d)", m_recoveries, kMaxRecoveries);
    m_last = Counter();
    return true;
}

void TownSession::ReloadConfig()
{
    LoadConfig(m_config, m_camera);
    m_gpu.models.LoadThemes(m_config);
    ApplyTownThemes(m_gpu.models, m_town);
    m_gpu.device->SetVsync(m_config.GetBool("render", "vsync", true));
    ApplyPlayerConfig(m_config, m_player);
    m_guidance.SetSettings(ReadGuidanceSettings(m_config));
    m_memory.SetRevealRadius(ReadMapRevealRadius(m_config));
    m_collide = m_config.GetBool("collision", "enabled", true);
    m_walk = m_config.GetBool("player", "walk", true);
}

double TownSession::NextFrameSeconds()
{
    const std::int64_t now = Counter();
    const double seconds = static_cast<double>(now - m_last) / static_cast<double>(m_frequency);
    m_last = now;
    return seconds;
}

void TownSession::LookWithMouse()
{
    long mouseDx = 0;
    long mouseDy = 0;
    m_window.ConsumeMouseDelta(mouseDx, mouseDy);
    if (mouseDx == 0 && mouseDy == 0)
    {
        return;
    }
    const float sensitivity =
        std::clamp(m_config.GetFloat("mouse", "sensitivity", kDefaultMouseSensitivity), 0.0002f, 0.02f);
    const float invert = m_config.GetBool("mouse", "invert_y", false) ? -1.0f : 1.0f;
    m_camera.SetPose(m_camera.Position(), m_camera.Yaw() + static_cast<float>(mouseDx) * sensitivity,
                     m_camera.Pitch() - invert * static_cast<float>(mouseDy) * sensitivity);
}

void TownSession::ReadKeys(Frame& frame)
{
    const bool focused = frame.focused;
    frame.keys = focused ? input::ReadCameraKeys() : CameraInput{};
    if (input::PressedOnce('Z', m_wireDown, focused))
    {
        m_gpu.models.ToggleWireframe();
    }
    if (input::PressedOnce('X', m_detailDown, focused))
    {
        m_gpu.models.CycleDetail();
    }
    if (input::PressedOnce('N', m_collideDown, focused))
    {
        m_collide = !m_collide;
        DL_LOG_INFO("Main", "collision %s", m_collide ? "on" : "off");
    }
    if (input::PressedOnce('F', m_walkDown, focused))
    {
        m_walk = !m_walk;
        DL_LOG_INFO("Main", "movement: %s", m_walk ? "walk" : "fly");
    }
    frame.jumpPressed = input::PressedOnce(VK_SPACE, m_jumpDown, focused);
    frame.jumpHeld = focused && input::IsDown(VK_SPACE);
    frame.usePressed =
        input::PressedOnce('E', m_useDown, focused); // walking: the lantern or a thing; in the map: a marker
    frame.resetPressed = input::PressedOnce('R', m_resetDown, focused);
    if (input::PressedOnce('H', m_hintDown, focused) && !m_mapOpen)
    {
        m_guidance.Press(); // the lantern raised and a beam over the place; again, and the thing to use
    }
    frame.walking = m_walk && m_collide;
    frame.thirdPerson = m_config.GetBool("player", "third_person", true);
    frame.orbit = frame.walking && frame.thirdPerson;
}

void TownSession::MapKeys(const Frame& frame)
{
    const bool m = input::PressedOnce('M', m_mapDown, frame.focused);
    if (!m_mapOpen)
    {
        if (m && m_sinking < 0.0f)
        {
            OpenMap();
        }
        return;
    }
    const bool escape = m_window.ConsumeEscape();
    const bool tab =
        input::PressedOnce(VK_TAB, m_tabDown, frame.focused); // (SwitchMoment does not run while the map is open)
    if (m || escape)
    {
        CloseMap();
        return;
    }
    if (tab)
    {
        m_mapTop = 1 - m_mapTop; // the other sheet on top
    }
    if (frame.usePressed)
    {
        PlaceMarker(frame);
    }
}

void TownSession::OpenMap()
{
    m_mapOpen = true;
    m_mapOpened = true;
    m_mapTop = m_gpu.models.CollisionLayer(); // the sheet of the moment the player is in is on top
    m_mapWasCaptured = m_window.MouseCaptured();
    m_window.SetMouseCaptured(false); // the pointer is free: it marks
    m_window.SetCaptureAllowed(false);
    m_window.SetEscapeQuits(false);
    DL_LOG_INFO("Map", "opened: %d cells seen, %d markers", m_memory.SeenCount(), m_memory.MarkerCount());
}

void TownSession::CloseMap()
{
    m_mapOpen = false;
    m_window.SetCaptureAllowed(true);
    m_window.SetEscapeQuits(true);
    if (m_mapWasCaptured)
    {
        m_window.SetMouseCaptured(true);
    }
}

MapScene TownSession::MakeMapScene(const Frame& frame) const
{
    MapScene scene;
    scene.town = &m_town;
    scene.memory = &m_memory;
    scene.top = m_mapTop;
    scene.player = frame.walking ? m_player.Feet() : m_camera.Position();
    scene.yaw = m_camera.Yaw();
    scene.hasLantern = !m_lantern.Carried();
    scene.lantern = m_lantern.At();
    for (std::size_t k = 0; k < m_town.emblems.size() && k < scene.lit.size(); ++k)
    {
        scene.lit[k] = town::EmblemLit(m_town, m_things, m_town.emblems[k]);
    }
    return scene;
}

void TownSession::PlaceMarker(const Frame& frame)
{
    const MapScene scene = MakeMapScene(frame);
    const MapFrame paper = ComputeMapFrame(m_gpu.device->Width(), m_gpu.device->Height(), scene);
    float x = scene.player.x;
    float z = scene.player.z;
    int cx = 0;
    int cy = 0;
    float wx = 0.0f;
    float wz = 0.0f;
    if (m_window.CursorClient(cx, cy) && paper.ToWorld(static_cast<float>(cx), static_cast<float>(cy), wx, wz))
    {
        x = wx; // where the pointer is on the paper (else where the body is)
        z = wz;
    }
    float y = m_town.sea;
    float floorY = 0.0f;
    if (m_moments.FloorAt(x, z, 120.0f, -40.0f, 0, floorY) || m_moments.FloorAt(x, z, 120.0f, -40.0f, 1, floorY))
    {
        y = floorY;
    }
    const float grab = paper.valid ? kMarkerGrab / paper.Scale() : 3.0f;
    const bool placed = m_memory.ToggleMarker({x, y, z}, grab);
    DL_LOG_INFO("Map", "marker %s at %.0f %.0f (%d of %d)", placed ? "placed" : "taken away", static_cast<double>(x),
                static_cast<double>(z), m_memory.MarkerCount(), town::MapMemory::kMaxMarkers);
}

void TownSession::SwitchMoment(const Frame& frame)
{
    // the switch, through the guard: refused into a wall or into the water of the other moment
    if (input::PressedOnce(VK_TAB, m_tabDown, frame.focused && !input::IsDown(VK_MENU) && m_sinking < 0.0f))
    {
        const int solidNow = m_guard.Refusing() ? m_guard.HeldLayer() : m_gpu.models.CollisionLayer();
        const int target = 1 - m_gpu.models.CollisionLayer();
        const bool fits = !frame.walking || FitsInTown(m_player, m_moments, m_town, target);
        switch (m_guard.OnSwitchRequest(solidNow, m_gpu.models.LayerSwitching(), fits))
        {
        case LayerGuard::Action::Switch:
            ++m_switches;
            m_gpu.models.ToggleLayer();
            break;
        case LayerGuard::Action::Refuse:
            ++m_switches;
            m_gpu.models.ToggleLayer();
            DL_LOG_INFO("Main", "switch refused: the body would be inside something in layer %s",
                        target == 0 ? "A" : "B");
            break;
        case LayerGuard::Action::Ignore:
            break;
        }
    }
    if (m_guard.Update(frame.dt))
    {
        m_gpu.models.ToggleLayer();
    }
}

void TownSession::UpdateCamera(const Frame& frame)
{
    if (frame.walking && !m_wasWalking)
    {
        // Debug fly is not a shortcut through the town in the orbit prototype.
        if (frame.thirdPerson)
        {
            m_boom = 0.0f;
        }
        else
        {
            m_player.Place(m_camera.Position());
        }
    }
    m_wasWalking = frame.walking;
    if (frame.walking)
    {
        CameraInput lookOnly;
        lookOnly.lookRight = frame.keys.lookRight;
        lookOnly.lookUp = frame.keys.lookUp;
        m_camera.Update(lookOnly, frame.dt);
    }
    else
    {
        m_camera.Update(frame.keys, frame.dt);
    }
}

void TownSession::MoveBody(const Frame& frame)
{
    WalkAndUse(frame);
    ReturnFromFall(frame);
    PlaceEye(frame);
}

void TownSession::WalkAndUse(const Frame& frame)
{
    const bool sinking = m_sinking >= 0.0f;
    Player::Input move;
    move.moveForward = sinking ? 0.0f : frame.keys.moveForward;
    move.moveRight = sinking ? 0.0f : frame.keys.moveRight;
    move.jumpPressed = frame.jumpPressed && !sinking;
    move.jumpHeld = frame.jumpHeld;
    move.sprint = frame.focused && input::IsDown(VK_SHIFT) && !sinking && m_config.GetBool("player", "sprint", true);
    m_player.Update(move, m_camera.Forward(), frame.solidLayer, frame.dt);
    const Vec3 wish = m_player.WishDirection();
    if (wish.x * wish.x + wish.z * wish.z > 0.5f)
    {
        const float targetYaw = std::atan2(wish.x, wish.z);
        const float error = std::remainder(targetYaw - m_avatarYaw, 6.2831853f);
        m_avatarYaw += error * (1.0f - std::exp(-22.0f * frame.dt));
    }
    const float climb = std::clamp(m_config.GetFloat("player", "mantle_height", 2.2f), 0.0f, 2.2f);
    if (move.jumpHeld && !sinking && m_config.GetBool("player", "mantle", true) &&
        town::TryTownMantle(m_player, m_moments, m_town, frame.solidLayer, climb))
    {
        DL_LOG_INFO("Player", "mantle: ledge reached");
    }
    if (frame.usePressed && !sinking)
    {
        HandleUse(m_town, m_things, m_traces, m_lantern, m_moments, m_plain, m_player, m_camera, frame.solidLayer);
    }
}

void TownSession::ReturnFromFall(const Frame& frame)
{
    // into the water: it sinks for a moment, then comes back; below the sea or the town: a fall
    if (m_sinking < 0.0f && m_town.Submerged(m_player.Feet(), frame.solidLayer, m_moments.GetCircle()))
    {
        m_sinking = town::kSinkSeconds;
        DL_LOG_INFO("Main", "into the water");
    }
    bool back = frame.resetPressed;
    if (m_sinking >= 0.0f)
    {
        m_sinking -= frame.dt;
        back = back || m_sinking < 0.0f;
    }
    back = back || m_player.Feet().y < m_town.sea - 3.0f || m_player.JustRespawned();
    if (back)
    {
        m_sinking = -1.0f;
        m_player.SetSpawn(m_safe);
        m_player.Respawn();
        m_respawn = Respawn{m_player.Feet(), 0.0f};
        m_landDip = 0.0f;
        m_boom = 0.0f;
        m_gaitAmount = 0.0f;
        m_gaitPhase = 0.0f;
        m_stood = 0.0f;
        if (!frame.resetPressed)
        {
            ++m_falls;
            DL_LOG_INFO("Main", "fell: back at the %s", m_town.NearestPlace(m_safe.x, m_safe.z));
        }
    }
    // the last place it stood still on, out of the water, standing in both moments without the circle: a return there
    // never ends in a wall or in the air, whatever is switched and wherever the lantern is
    m_stood = m_player.Grounded() && m_sinking < 0.0f ? m_stood + frame.dt : 0.0f;
    const float radius = m_player.GetSettings().radius;
    if (m_stood > 0.5f && FitsInTown(m_player, m_plain, m_town, 0) && FitsInTown(m_player, m_plain, m_town, 1) &&
        m_plain.Supports(m_player.Feet(), radius, 0) && m_plain.Supports(m_player.Feet(), radius, 1))
    {
        m_safe = m_player.Feet();
    }
}

void TownSession::PlaceEye(const Frame& frame)
{
    float peak = 0.0f;
    if (m_player.ConsumeLanding(peak))
    {
        DL_LOG_INFO("Player", "jump peak %.2f m", static_cast<double>(peak));
    }
    float impact = 0.0f;
    if (m_player.ConsumeImpact(impact))
    {
        const float perSpeed = std::clamp(m_config.GetFloat("camera", "land_dip", kDefaultLandDip), 0.0f, 0.05f);
        const float deepest = std::clamp(m_config.GetFloat("camera", "land_dip_max", kDefaultLandDipMax), 0.0f, 0.5f);
        m_landDip = std::max(m_landDip, std::min(impact * perSpeed, deepest));
    }
    m_landDip *= std::exp(-kLandDipReturn * frame.dt);
    Vec3 eye = m_player.Eye();
    const float rise = eye.y - m_eyeY;
    m_eyeY = (m_player.Grounded() && std::fabs(rise) < kEyeEaseMax)
                 ? m_eyeY + rise * (1.0f - std::exp(-kEyeEase * frame.dt))
                 : eye.y;
    eye.y = m_eyeY - m_landDip;
    if (frame.orbit)
    {
        const float pitch = std::clamp(m_camera.Pitch(), -0.90f, 0.38f);
        const float reach = std::clamp(m_config.GetFloat("camera", "orbit_distance", 3.8f), 2.0f, 6.0f);
        eye.y -= 0.20f;
        m_camera.SetPose(eye, m_camera.Yaw(), pitch);
        const Vec3 direction = m_camera.Forward();
        const float clear = town::CameraClearDistance(m_moments, eye, direction, reach, frame.solidLayer,
                                                      m_gpu.models.LayerSwitching());
        m_boom = (std::min)(clear, m_boom + 10.0f * frame.dt);
        m_camera.SetPose(town::Along(eye, direction, -m_boom), m_camera.Yaw(), pitch);
    }
    else
    {
        m_camera.SetPose(eye, m_camera.Yaw(), m_camera.Pitch());
    }
}

void TownSession::PushFlyingCamera(const Frame& frame)
{
    // flying with collision: the camera is a ball pushed out of the town
    Vec3 c = m_camera.Position();
    const float radius = std::clamp(m_config.GetFloat("collision", "radius", kDefaultBodyRadius), 0.05f, 1.0f);
    if (m_moments.PushOut(c, radius, frame.solidLayer) > 0.0f)
    {
        m_camera.SetPose(c, m_camera.Yaw(), m_camera.Pitch());
    }
}

void TownSession::UpdateGuidance(const Frame& frame)
{
    const bool active = frame.walking && m_sinking < 0.0f;
    m_walkSeconds += active ? frame.dt : 0.0f;
    m_guidance.Update(frame.dt, m_player.Feet(), m_things, m_lantern, active);
    m_memory.Update(m_town, m_player.Feet());
}

void TownSession::BuildDrawTable(const Frame& frame)
{
    // what is drawn one by one: the town is in the static batch; here the sea, the signal mast's lamps, the lantern,
    // the things, the ring of a return, the markers, the guidance and the avatar
    scene::SceneObject* table = m_drawTable.data();
    int drawn = town::AddSea(m_camera.Position(), table, 0, kDrawCapacity, m_town.sea, kSeaNorthEdge);
    drawn = town::AddEmblems(m_town, m_things, table, drawn, kDrawCapacity, m_clock);
    const town::Flame setDownFlame = m_guidance.FlameFrom(m_lantern.At());
    drawn = m_lantern.AddDraw(table, drawn, kDrawCapacity, &setDownFlame, m_clock);
    drawn = m_things.AddDraw(table, drawn, kDrawCapacity, m_traces);
    const town::Circle circle = m_moments.GetCircle();
    m_gpu.models.SetLantern(DirectX::XMFLOAT3(circle.centre.x, circle.centre.y, circle.centre.z), circle.radius);
    drawn = town::AddRespawnRing(m_respawn.at, m_respawn.since, table, drawn, kDrawCapacity);
    for (int k = 0; k < m_memory.MarkerCount() && drawn < kDrawCapacity; ++k)
    {
        const town::MapMarker& mk = m_memory.Marker(k); // a thin column of light, in both moments
        scene::SceneObject o{};
        o.shape = scene::ShapeKind::Cube;
        o.position[0] = mk.x;
        o.position[1] = mk.y + 0.5f * kMarkerHeight;
        o.position[2] = mk.z;
        o.scale[0] = o.scale[2] = 0.18f;
        o.scale[1] = kMarkerHeight;
        o.colorSrgb[0] = 0.62f;
        o.colorSrgb[1] = 0.90f;
        o.colorSrgb[2] = 1.0f;
        o.glow = 1.3f;
        o.layerMask = scene::kLayerBoth;
        table[drawn++] = o;
    }
    // the hand of the avatar (the beam of H starts there)
    const float sy = std::sin(m_avatarYaw);
    const float cy = std::cos(m_avatarYaw);
    const Vec3 feet = m_player.Feet();
    const Vec3 hand{feet.x + sy * 0.45f + cy * 0.31f, feet.y + 1.3f, feet.z + cy * 0.45f - sy * 0.31f};
    drawn = m_guidance.AddDraw(table, drawn, kDrawCapacity, m_clock, hand);
    if (frame.orbit)
    {
        const float travel = std::hypot(feet.x - frame.feetBeforeMove.x, feet.z - frame.feetBeforeMove.z);
        const float walkSpeed = m_player.GetSettings().walkSpeed;
        // Use actual travel rather than requested velocity, so pushing into a wall does not animate a run.
        const float moved = frame.dt > 0.0f && travel <= 2.0f * walkSpeed * frame.dt ? travel : 0.0f;
        const float targetGait =
            m_player.Grounded() && walkSpeed > 0.0f && frame.dt > 0.0f ? moved / (walkSpeed * frame.dt) : 0.0f;
        m_gaitAmount += (targetGait - m_gaitAmount) * (1.0f - std::exp(-18.0f * frame.dt));
        if (m_player.Grounded())
        {
            m_gaitPhase = std::remainder(m_gaitPhase + moved * 4.6f, 6.2831853f);
        }
        if (m_boom >= 0.85f)
        {
            const town::Flame flame = m_guidance.FlameFrom(feet);
            drawn = town::AddTownAvatar(table, drawn, kDrawCapacity, m_player, m_avatarYaw, m_gaitPhase, m_gaitAmount,
                                        m_lantern.Carried(), &flame, m_guidance.Raise(), m_clock);
        }
    }
    m_gpu.models.SetObjects(table, drawn);
}

void TownSession::Render(const Frame& frame)
{
    const Vec3 pos = m_camera.Position();
    const Vec3 body = frame.walking ? m_player.Eye() : pos;
    const Vec3 dir = m_camera.Forward();
    const DirectX::XMMATRIX view = DirectX::XMMatrixLookToLH(DirectX::XMVectorSet(pos.x, pos.y, pos.z, 1.0f),
                                                             DirectX::XMVectorSet(dir.x, dir.y, dir.z, 0.0f),
                                                             DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
    float clear[4] = {};
    m_gpu.models.ClearColor(clear);
    {
        GpuEvent frameEvent(m_gpu.device->GetAnnotation(), L"Frame");
        m_gpu.device->BeginFrame(clear);
        const float aspect = static_cast<float>(m_gpu.device->Width()) / static_cast<float>(m_gpu.device->Height());
        {
            GpuEvent pass(m_gpu.device->GetAnnotation(), L"Opaque.Models");
            m_gpu.models.Draw(m_gpu.device->GetContext(), view,
                              MakeProjection(aspect, m_config.GetFloat("camera", "fov_deg", kDefaultFovYDegrees)),
                              DirectX::XMFLOAT3(pos.x, pos.y, pos.z));
        }
        {
            char status[160];
            std::snprintf(status, sizeof(status), "%s", m_town.NearestPlace(body.x, body.z));
            const bool active = frame.walking && m_sinking < 0.0f && !m_mapOpen;
            const bool offerSwitch = m_switches == 0 && m_walkSeconds >= kSwitchOfferSeconds;
            const bool offerMap = !m_mapOpened && m_walkSeconds >= kMapOfferSeconds;
            const prompt::Shown prompt = ChoosePrompt(m_town, m_things, m_lantern, m_player,
                                                      m_gpu.models.CollisionLayer(), active, offerSwitch, offerMap);
            const float under = m_sinking >= 0.0f ? 1.0f : 0.0f;
            BuildHud(*m_hud, m_gpu.device->Width(), m_gpu.device->Height(), prompt, status, under,
                     !frame.orbit && !m_mapOpen);
            if (m_mapOpen)
            {
                const MapScene scene = MakeMapScene(frame);
                BuildMap(*m_hud, m_gpu.device->Width(), m_gpu.device->Height(), scene,
                         ComputeMapFrame(m_gpu.device->Width(), m_gpu.device->Height(), scene));
            }
            GpuEvent pass(m_gpu.device->GetAnnotation(), L"Overlay");
            m_gpu.overlay.Draw(m_gpu.device->GetContext(), *m_hud, m_gpu.device->Width(), m_gpu.device->Height());
        }
    }
    m_gpu.device->EndFrame();
}

void TownSession::UpdateTitle(const Frame& frame)
{
    m_fpsTimer += frame.seconds;
    ++m_fpsFrames;
    if (m_fpsTimer < 0.5)
    {
        return;
    }
    // The title bar: adapter, size, speed, where the body is, the mode and the progress. tools/check.ps1 reads it, so
    // the words "layer", "walk", "fly", "noclip" and "quay" are part of the contract.
    const Vec3 body = frame.walking ? m_player.Eye() : m_camera.Position();
    const wchar_t* mode = m_collide ? (m_walk ? L"collide | walk" : L"collide | fly") : L"noclip";
    wchar_t title[460];
    swprintf_s(
        title,
        L"DualLayer | %s | %d x %d | %.1f FPS (%.2f ms) | body %.1f %.1f %.1f | %s | %s | town: %hs | bells %d of %d | "
        L"lamps %d of %d | steps %d of %d | falls %d%s%s",
        m_gpu.device->AdapterName().c_str(), m_gpu.device->Width(), m_gpu.device->Height(),
        static_cast<double>(m_fpsFrames) / m_fpsTimer, 1000.0 * m_fpsTimer / static_cast<double>(m_fpsFrames),
        static_cast<double>(body.x), static_cast<double>(body.y), static_cast<double>(body.z),
        m_gpu.models.StatusLine().c_str(), mode, m_town.NearestPlace(body.x, body.z), m_things.BellsRung(),
        m_things.BellCount(), town::LitEmblems(m_town, m_things), static_cast<int>(m_town.emblems.size()),
        m_guidance.Done(), static_cast<int>(m_town.steps.size()), m_falls, m_mapOpen ? L" | map" : L"",
        m_window.MouseCaptured() ? L"" : (m_mapOpen ? L"" : L" | click for mouse look"));
    m_window.SetTitle(title);
    m_fpsTimer = 0.0;
    m_fpsFrames = 0;
}

} // namespace dl
