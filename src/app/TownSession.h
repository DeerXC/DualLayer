// TownSession.h - the town, walked: one session from the first frame to the close of the window.
// Frame:    PumpMessages > (lost device: Recover) > (F5: reload the config) > (resize) > mouse and keys > the map (M) >
// the
//           switch through the LayerGuard > the camera > the body (walk, use, water, fall, return) > the guidance and
//           the map memory > the objects of the frame > render [GPU event "Frame" > "Opaque.Models" > "Overlay"] >
//           Present > the title bar
// Owns:     the solid of both moments (scene/TriMesh.h) with and without the circle of the lantern, the walking body,
// the lantern,
//           the things that change and their traces, the guard of the switch, the guidance, what the player has seen of
//           the map and the markers they left, and the state of the camera and of the avatar.
// Borrows:  the window, the GPU objects, the config, the camera and the town (they outlive the session).
// Keys:     W A S D move, the mouse turns (click into the window to capture it; Esc gives it back, a second Esc quits),
// the arrow
//           keys turn as well, Shift sprints, Space jumps (held = the full jump, a tap = a small hop) and climbs a
//           ledge, Tab switches the moment (A, the past <-> B, the present), E opens a thing of the past or sets the
//           lantern down / takes it up, H asks the lantern for more help (docs/DESIGN.md 10.3), L looks at the light in
//           parts (everything, no shadows, no lamps, only the shadow of the sun, only the lamps: docs/DESIGN.md 6.3), M
//           opens the map (docs/DESIGN.md 10.4), R goes back to the last place the body stood still on, F5 reloads
//           config/default.ini, F walking / flying, N collision on / off, Z wireframe, X the next level of detail, Q /
//           E down / up (flying). F9 (only with [debug] device_reset_key = 1) throws the GPU objects away and builds
//           them again: the test of the recovery.
// Map:      M opens two sheets of tracing paper over the frozen game; the mouse is free; Tab turns the other sheet to
// the top, E
//           puts a marker where the pointer is (or takes the one there away), M or Esc closes it. Time stands still
//           while it is open (the guidance clock too).
// Switch:   a switch that would put the body inside something, or under the water of the other moment, is refused: the
// ripple
//           opens a little and turns back (scene/LayerGuard.h), and the old moment stays solid meanwhile.
// Camera:   third person by default ([player] third_person): the camera orbits behind the body and is pulled in by the
// walls
//           (town::CameraClearDistance). A hard landing lowers the eye for a moment ([camera] land_dip). The first
//           frame looks where the town file says (spawn yaw and pitch): the heap of the quay, the winch of the sea gate
//           and the lamps of the signal mast (tests/test_firstminute.cpp).
// Overlay:  drawn last (render/Hud.h, render/MapView.h): a dot, the one prompt of the key that does something now, a
// status line. Lost device: Present reports it, the loop builds device and models again (Gpu::Recover); the moment, the
// town and the camera
//           stay as they were. After kMaxRecoveries tries the session says so and ends.
// Threading: main thread only. The object is not copied or moved: the solids hold pointers into it.
#pragma once

#include "app/Gpu.h"
#include "core/Config.h"
#include "core/Window.h"
#include "render/MapView.h"
#include "render/OverlayBatch.h"
#include "scene/Camera.h"
#include "scene/Guidance.h"
#include "scene/Lantern.h"
#include "scene/LayerGuard.h"
#include "scene/MapMemory.h"
#include "scene/Moments.h"
#include "scene/Player.h"
#include "scene/SceneData.h"
#include "scene/Things.h"
#include "scene/Town.h"
#include "scene/TriMesh.h"

#include <array>
#include <cstdint>
#include <memory>
#include <vector>

namespace dl
{

class TownSession
{
  public:
    TownSession(Window& window, Gpu& gpu, Config& config, Camera& camera, int adapter, const town::TownData& town);
    TownSession(const TownSession&) = delete;
    TownSession& operator=(const TownSession&) = delete;

    /// Runs until the window closes. Returns the exit code: 0, or 1 when the device was lost and could not be made
    /// again.
    int Run();

  private:
    /// What one frame knows: the time, the keys, and what they mean for the body.
    struct Frame
    {
        bool focused = false;      ///< the window is in front: only then do keys count
        float dt = 0.0f;           ///< UNITS: seconds since the last frame
        double seconds = 0.0;      ///< the same as a double (the frame rate)
        CameraInput keys;          ///< the movement keys
        bool jumpPressed = false;  ///< Space went down in this frame
        bool jumpHeld = false;     ///< Space is down
        bool usePressed = false;   ///< E went down
        bool resetPressed = false; ///< R went down
        bool walking = false;      ///< the body walks (not flying, not without collision)
        bool thirdPerson = false;  ///< [player] third_person
        bool orbit = false;        ///< walking in third person: the camera orbits the body
        int solidLayer = 0;        ///< the moment whose solid the body meets now
        Vec3 feetBeforeMove{};     ///< where the feet were before the body moved (the gait of the avatar)
    };

    /// Where and when the body came back to the last place it stood still on (the ring on the floor).
    struct Respawn
    {
        Vec3 at{};
        float since = -1.0f; ///< UNITS: seconds since the last respawn; negative before the first
    };

    // ---- the steps of a frame, in order
    bool RecoverDevice(bool resetTest);
    void ReloadConfig();
    double NextFrameSeconds();
    void LookWithMouse();
    void ReadKeys(Frame& frame);
    void MapKeys(const Frame& frame);
    void OpenMap();
    void CloseMap();
    void PlaceMarker(const Frame& frame);
    MapScene MakeMapScene(const Frame& frame) const;
    void SwitchMoment(const Frame& frame);
    void UpdateCamera(const Frame& frame);
    void MoveBody(const Frame& frame);
    void WalkAndUse(const Frame& frame);
    void ReturnFromFall(const Frame& frame);
    void PlaceEye(const Frame& frame);
    void PushFlyingCamera(const Frame& frame);
    void UpdateGuidance(const Frame& frame);
    void BuildDrawTable(const Frame& frame);
    void Render(const Frame& frame);
    void UpdateTitle(const Frame& frame);

    Window& m_window;
    Gpu& m_gpu;
    Config& m_config;
    Camera& m_camera;
    int m_adapter;
    const town::TownData& m_town;

    // ---- the world
    std::array<town::TriMesh, 2> m_meshes; ///< the solid of the town in A and in B
    town::Moments m_moments;               ///< both moments with the circle of the lantern
    town::Moments m_plain;                 ///< the same without it: where a fall may come back to
    Player m_player;
    town::Lantern m_lantern; ///< carried, or set down in A (scene/Lantern.h)
    town::Things m_things;   ///< the things that change: opened in A, the trace in B (scene/Things.h)
    town::Traces m_traces;
    town::Guidance m_guidance;
    town::MapMemory m_memory;
    std::vector<PointLight>
        m_townLamps; ///< the lamps of the town file (for the halos: the renderer keeps its own copy)
    std::vector<PointLight> m_movingLamps; ///< the lantern and the lit lamps of this frame
    LayerGuard m_guard;

    // ---- what is drawn
    static constexpr int kDrawCapacity = 400;
    std::array<scene::SceneObject, kDrawCapacity> m_drawTable{};
    std::unique_ptr<OverlayBatch> m_hud; ///< WHY on the heap: 1.4 MB, too large for the stack
    Respawn m_respawn;

    // ---- the state of the body
    Vec3 m_safe{};           ///< the last place the body stood still on: where a fall comes back to
    float m_stood = 0.0f;    ///< UNITS: seconds the body has stood on the ground without a break
    float m_sinking = -1.0f; ///< UNITS: seconds left of sinking in water before it comes back; negative: not sinking
    int m_falls = 0;
    float m_landDip = 0.0f; ///< UNITS: metres the eye is lowered by a hard landing
    float m_eyeY = 0.0f;    ///< UNITS: metres, the height of the eye as the camera shows it (eased)
    float m_avatarYaw = 0.0f;
    float m_gaitPhase = 0.0f;
    float m_gaitAmount = 0.0f;
    float m_boom = 0.0f; ///< UNITS: metres, how far the camera is behind the body now

    // ---- the first lesson and the clocks
    int m_lightView = 0; ///< what the picture shows of the light: 0 everything .. 4 only the lamps (key L)
    int m_switches = 0;  ///< how many times the player asked for a switch
    float m_walkSeconds =
        0.0f;             ///< UNITS: seconds the body has walked (the prompts of TAB and M are offered after a while)
    float m_clock = 0.0f; ///< UNITS: seconds since the start, without the time the map was open

    // ---- the map
    bool m_mapOpen = false;
    bool m_mapOpened = false;      ///< the player has opened it at least once (the prompt of M goes)
    bool m_mapWasCaptured = false; ///< the mouse was captured when the map opened: it is again when it closes
    int m_mapTop = 0;              ///< the sheet on top: 0 = A, 1 = B

    // ---- the loop
    bool m_collide = true;
    bool m_walk = true;
    bool m_wasWalking = true;
    int m_recoveries = 0;
    std::int64_t m_frequency = 1;
    std::int64_t m_last = 0;
    double m_fpsTimer = 0.0;
    int m_fpsFrames = 0;

    // ---- the memory of the keys (a press is the edge of a key that is down)
    bool m_wireDown = false;
    bool m_detailDown = false;
    bool m_collideDown = false;
    bool m_walkDown = false;
    bool m_jumpDown = false;
    bool m_tabDown = false;
    bool m_resetDown = false;
    bool m_useDown = false;
    bool m_hintDown = false;
    bool m_lightDown = false;
    bool m_mapDown = false;
    bool m_deviceResetDown = false;
};

} // namespace dl
