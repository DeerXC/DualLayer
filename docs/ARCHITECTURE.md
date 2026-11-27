# Architecture

The code as built: one program that loads the town as data and walks a body through it in two moments. The design is
[DESIGN.md](DESIGN.md); the rules of the code (units, coordinates, colour, depth) are in [CONVENTIONS.md](CONVENTIONS.md).
An earlier prototype with hand-placed rooms and puzzles was removed on 2026-11-26; it is in the git tag `m2-greybox`.

## Repository layout

| Path | Contents |
|---|---|
| `src/` | the game: `core`, `scene`, `render`, `app` and `main.cpp` (see Modules) |
| `shaders/` | HLSL, compiled by the game at start |
| `data/` | `town.dlt`, the town as data, written by the layout model |
| `config/` | `default.ini`, every tunable value |
| `tests/` | the logic tests, `check.h` (the shared check macro) and `run_tests.ps1` |
| `tools/` | `check.ps1` (the one-command check); `layout/` is the layout model of the town in Python: `city_model.py` (the model and the export), `architecture.py` (the buildings), `town_style.py` (the palette) and the review tools |
| `docs/` | design, build, architecture and conventions |

## One frame

```
WinMain > RunApp
  once:   log file, config, window (DPI aware), the town (TownData::Load), Gpu::Init (Device, Models, Overlay; the polygons of
          the town baked into the static batch of each moment), TownSession: the solid of both moments, the body at the spawn point
  frame:  TownSession::Run
          1. Window::PumpMessages
          2. lost device (or F9)   > RecoverDevice, Gpu::Recover: every GPU object made again on a new Device; the town and the body stay
          3. F5                    > ReloadConfig: the config, the themes of the moments, the lighting, the settings of the body and of the guidance
          4. resize                > Device::Resize
          5. LookWithMouse, ReadKeys   > the mouse turns the camera (not while the map is open); Z X N F toggles; the edges of Space, E, R and H
             MapKeys                > M opens and closes the map; inside it Tab turns the sheet, E marks, T changes the shape of the next mark, + - zoom, the arrow keys move, C centres; the game stands still
          6. SwitchMoment          > Tab goes through the LayerGuard (a switch into something, or into the water of the other
                                     moment, is refused)
          7. UpdateCamera, Models::Update   > the camera keys; the ripple clock
          8. MoveBody (walking)    > WalkAndUse: Player::Update against the solid of the moment (Moments, with the circle of the
                                     lantern), the ledge climb (TryTownMantle), E (HandleUse: a thing of the past, else the lantern)
                                     ReturnFromFall: water, falls, R: back to the last place the body stood still on
                                     PlaceEye: the landing dip, the eased eye, the orbit camera pulled in by walls
             PushFlyingCamera (flying)  > Moments::PushOut on the camera ball
          9. UpdateGuidance        > the goal and the progress of the flame (scene/Guidance.h); what the body has seen (MapMemory)
         10. BuildDrawTable        > the sea round the eye, the lamps of the signal mast, the lantern and its flame, the things and
                                     their traces, the ring of a return, the markers, the beam or the frame of H, the avatar, the halos of the lamps > Models::SetObjects;
                                     the lamps that move (scene/Lamps.h MovingLamps: the lantern, the lit lamps of the mast, the lamps of the bells rung) > Models::SetMovingLamps
         11. Render                > Device::BeginFrame, Models::Draw (the shadow map and the picture of one moment, or the ripple), Overlay::Draw (BuildHud: the
                                     prompt, the status line; BuildMap over it when the map is open), Device::EndFrame (Present)
         12. UpdateTitle           > adapter, speed, body, moment, place, bells, steps, falls
  end:    Gpu::Shutdown (Models, Overlay, Device: the debug layer reports leaks), log closed
```

What is drawn and what is solid come from the same data in the same frame, so they cannot disagree.

## Modules

```
main
 +- app      AppConfig, Gpu, Input, TownSession                                                    > render, scene, core
 +- render   Device, Models, MeshData, LayerRipple, Projection, ShaderCompiler, GpuDebug,
             Overlay, OverlayBatch, Font5x7, StaticBatch, Hud, MapView, Lights, ShadowMath,
             LightingSettings                                                                       > core, scene (data only)
 +- scene    Camera, SceneData, TriMath, TriMesh, Moments, Player, TownPlayer, Town, Lantern, LanternModel,
             Things, Guidance, Flame, Emblems, Lamps, MapMemory, LayerGuard, Prompts, TownEffects     > standard library only
 +- core     Log, Assert, Config, Window                                                           > standard library, Windows
```

A dependency only points down. `app` is the game loop and the only place that reads the keyboard; `render` is the only place
that talks to Direct3D. Everything in `scene` and the pure parts of `core` and `render` (`MeshData`, `LayerRipple`,
`OverlayBatch`, `Font5x7`, `StaticBatch`, `Hud`, `MapView`) have no Windows or Direct3D dependency and are tested without a window.

| Module | Job |
|---|---|
| `app/TownSession.h` | the frame loop of the town, in small steps (see One frame) |
| `app/Gpu.h`, `app/AppConfig.h`, `app/Input.h` | the GPU objects made and lost together; the config of the program; the keyboard |
| `scene/Town.h` | reads `data/town.dlt`; bakes the drawn polygons; builds the solid of each moment; the steps of the main path |
| `scene/TriMath.h`, `scene/TriMesh.h` | the closest point of a triangle; triangles filed in a grid of 4 m cells and the push-out of a ball |
| `scene/Moments.h` | the solid of both moments with the circle of the lantern; the push-out and the wall push |
| `scene/Lantern.h`, `scene/LanternModel.h` | the lantern: set down in A, a circle of A in B; the traces of things changed in A; the model of the lantern and of the post it hangs from when it is set down (also what the traveller carries) |
| `scene/Things.h` | the things that change: opened in A, a trace in B (the causal rule) |
| `scene/Guidance.h`, `scene/Flame.h` | the goal (the first step of the main path not done), the progress of the player, the flame of the lantern that leans toward the goal, the two steps of H |
| `scene/Emblems.h` | the three lamps of the signal mast, dark until the bell of their district is rung |
| `scene/MapMemory.h`, `render/MapView.h` | what the player has seen and the markers they left; the map drawn on two sheets of tracing paper |
| `scene/Player.h`, `scene/TownPlayer.h` | the walking body; the town's climb, camera clearance and the avatar (a stylised figure of 45 parts, two-bone legs and arms, the lantern in the right hand) |
| `scene/LayerGuard.h` | refuses a switch that would put the body inside something |
| `scene/Prompts.h` | the one key prompt, chosen from the state of the game |
| `scene/TownEffects.h` | the sea round the eye and the ring of a return |
| `render/Hud.h` | the overlay of one frame |
| `render/Lights.h`, `scene/Lamps.h` | the point lights: the choice of the 32 for a frame (the moment, the distance, the flicker); the lamps of the town file, the lantern, the lamps that bells light, and the halos |
| `render/ShadowMath.h`, `render/LightingSettings.h` | the matrix of the shadow box on the texel grid; every number of `[lighting]`, the looks of A and B (`StyleLookOf`) and their registers (`MakeStyle`) |

## The town

The layout model `tools/layout/city_model.py` writes the town as data: `python tools/layout/city_model.py export` checks the
model and writes `data/town.dlt` (the format is in the docstring of its `export`). `scene/Town.h` reads it: the lights of the
two moments, the materials (an sRGB colour in A and in B, and whether it shines), the stations of the main path, the middle
lines of the streets, the lamps, and the polygons of the town with their moment and their kind (solid, terrain, overlay, water).

- Drawn: `TownData::BakeLayer` gives one batch per moment, each polygon as a fan of triangles with one normal (Newell's), from
  both sides because the model's polygons have no fixed winding (the terrain and a surface of water only from above). The
  colours are the model's own (`color.a` 0: no stone tint). `BakeWater` gives the water of A, drawn by
  `Models::SetStaticWater` in the water pass. The looks of the two moments come from the model's lights (`ApplyTownThemes`
  in `app/Gpu.cpp`).
- Solid: `TownData::BuildMesh` gives a `TriMesh` per moment (its solid and terrain polygons), triangles filed in a grid of 4 m
  cells; `TriMesh::PushOut` moves a ball out of them as the ground does. `Moments` holds both and the circle of the
  lantern; the walking body takes it with `Player::SetTown`.
- The lantern (`scene/Lantern.h`): set down in A, it keeps a sphere of A (radius 3.5 m) in B. Solid: `Moments::PushOut`
  in B takes a triangle of A when the point of it nearest to the ball lies inside the sphere, a triangle of B when that point
  lies outside, so a floor of A ends under the centre of the ball at the edge, where its picture ends. Drawn:
  `Models::SetLantern`; in B the static batch of B is drawn without the inside of the sphere, then the batch of A, in the
  light of A, only inside it, with a warm rim (`lanternMode` in `cbuffer Frame`, `LanternClip` in `shaders/models.hlsl`); the
  same for the still water. The water of A is water inside the sphere in B (`TownData::Submerged` with a circle).
  `Traces` keeps the traces of things changed in A.
- The things that change (`scene/Things.h`): from the model (`TownData::things`: a verb (what the prompt says: OPEN,
  PUSH, LAY ACROSS, RING, LOWER), a handle and boxes, each in its moments
  and in one state, shut or open). E at a handle in A opens the thing in A and B at once (the causal rule); in B it does
  nothing; an opening that would close on the body is refused. The boxes of the state now are extra solids of their
  moments (`Moments::SetExtra`, the same rule with the circle as the town's triangles), drawn as cubes in the table of
  objects each frame; the boxes of B of an opened thing carry its trace; the handle of a shut thing glows gold in A.
- The loop: `TownSession` (`app/TownSession.h`, with `HandleUse` for E and `ChoosePrompt`) walks, switches (refused into a
  surface or into the water of the other moment, with the circle), opens a thing at its handle or else sets the lantern down and
  takes it up (E), sinks in water, falls, and comes back to the last place the body stood still on (also R); that place is one
  where the body stands in both moments without the circle, so a return never ends in a wall or in the air. It draws the sea
  round the eye and the ring of a return (`scene/TownEffects.h`), the lantern, the things and the guidance; the town is in the
  static batch. The game starts in the moment the file gives (B).

## Layers and the ripple

Every object carries a layer mask (A = 1, B = 2, both = 3); a draw call and a collision skip an object whose mask does not
hold the layer. Each layer has its own theme (light, sky, background, fog, and the stone of the city), never mixed: A is
the past, the whole city in a warm evening, B the present, the same city after a storm, cold and foggy, its stone dark and mossy. The model shader
gives the city (objects of both layers that do not glow) the tint and the weathering of the layer, and fogs everything by
its distance into the background colour; glowing things are seen through more of the fog. In the town, the renderer
also draws a world-aligned sky before the opaque scene, using the same full-screen triangle vertex shader as the
ripple. Each ripple target gets its own sky; the local lantern pass changes only geometry and water. The frame
buffer (464 bytes: see Light) carries the camera axes, sky colour and low-mist parameters, the matrix of the shadow box, the
exposure and the registers of the style. Opaque objects and water share the same distance and height haze.

`LayerRipple` keeps the timing of a switch. While no ripple runs, one layer is drawn straight to the back buffer. During a
ripple, `Models::DrawRipple` draws the outer layer and the inner layer into two off-screen pictures (sRGB colour and their
own depth), then one screen-covering triangle (`shaders/ripple.hlsl`) takes each pixel from one picture or the other and
draws a glowing ring. Tab during a ripple turns it round: only the radius changes, so nothing jumps. `StateGuard` puts back
every pipeline state the ripple changes.

## The static batch

The town (every polygon of both moments, and any box that does not move) is baked once into one vertex buffer per layer by
`TownData::BakeLayer` and `StaticBatch`: world positions and normals, the linear colour with the city flag, and the shader
parameters per vertex. `Models` draws it with one call per layer (`VSBatched`), then the moving and changing objects one by
one (`VSMain`); both feed the same pixel shader. The baked list is kept on the CPU, so a new device after a lost one bakes it
again.

## Water

The still water of A is baked by `TownData::BakeWater` and drawn in its own pass; the sea and the objects of a water material
(`scene::IsWaterMaterial`) are drawn after everything opaque, sorted far to near, blended and without writing the depth, so that
what is under a surface shows through it and one surface never hides another. The pixel shader gives each material its life
(waves, the sky, the sun, streaks, foam). A body that goes into water sinks for `kSinkSeconds` before it comes back, and the view
is tinted under the surface (`BuildHud`). `TownData::Submerged` answers whether a point is under the water of a moment, with the
circle of the lantern.

## Light

The model shader (`shaders/models.hlsl`) lights a surface with five terms: the sky light (a hemisphere, lighter above than below), the sun of A or the moon of B (Lambert, times the shadow), a highlight, the glow of the surface itself, and the lamps of the moment. The sum is faded into the fog and goes through the soft shoulder (`Finish`: the exposure, then `SoftShoulder` of `shaders/lighting.hlsli`). The maths that needs no texture and no buffer is in `lighting.hlsli`, written in the part of HLSL that is also C++, so that `tests/test_lighting.cpp` runs it.

- Shadow: `Models::DrawObjects` first draws the casters of the moment into one depth texture (`CreateShadowMap`: 32 bit, a depth view and a shader view) with the same two vertex shaders as the picture and no pixel shader, seen from the light with the matrix of `FitSunShadow` (`render/ShadowMath.h`: an orthographic box that follows the camera and moves on the texel grid). The pass draws both sides, is not clipped by the near plane of the box, and puts back the render target and the viewport it found. The picture then reads the map (t0, a comparison sampler, 3 x 3 taps of a hardware 2 x 2 comparison) after a move of the surface along its normal; the shadow fades out over the last tenth of the map. In the circle of the lantern (the part of B that shows A) there is no shadow. A map that cannot be made switches the shadows off (the log says so) and nothing else changes.
- Lamps: the town's lamps come from the file (`TownData::lamps`; the model writes one for each lantern it builds, `city_model.py lamp`), the moving ones from `MovingLamps` every frame. `SelectLamps` (`render/Lights.h`) takes for each moment the 32 that light the surroundings of the camera most (strength, distance, range), fades a lamp out with distance and gives it its flicker; they go to the constant buffer `Lamps` (b2) and the shader adds `LampShade` for each: a diffuse term and a highlight, falling with the inverse square and ending exactly at the range of the lamp.
- Halos: `AddHalos` makes a soft sphere (`kMatHalo`) for each of the nearest lamps; `Models` draws them after the water, added to the picture (an additive blend, depth tested and not written, both sides), and the shader gives them a strength that falls to 0 at their rim and into the fog.
- The haze: the fog colour gets the light of the sun scattered toward the sun (`SunScatter`).
- Style: the pixel shader builds a `StyleParams` (`lighting.hlsli`, "the style of the light") from the seven registers that end the frame buffer (`gradeParams2`, `styleCool`, `styleLow`, `styleHigh`, `styleLight`, `styleFar`, `styleMisc`; 464 bytes in all) and every part is a function of that file that `tests/test_style.cpp` runs as C++: `ToonRamp`, `CloudShade`, `RimTerm`, `LampShadeStyled` (`LampRings`, `LampWarmth`), `PaintAlbedo`, `StyleShade` (the plain sum with the cool fill and the rim), `StyleHaze`, `StyleGrade` (`StyleContrast`, the tones, the saturation, the dark corner; it comes before `SoftShoulder` in `Finish`) and `ShaftPhase`. The one part that needs the shadow map is `LightShafts` in `models.hlsl`: it walks the ray of sight from the eye to the surface (to 48 m) in `shaft_steps` samples, moved by a noise per pixel, and adds the light that the map lets through, thicker near the sea; the sky pass does the same along the ray into the sky. `render/LightingSettings.h` turns `[lighting]` into those registers (`MakeStyle`; the colours of the cool fill, the tones and the haze of A and B are `StyleLookOf`), and `Models::DrawObjects` copies them into the frame buffer. All of it is 0 in the view "plain".
- Debug: the key L cycles the debug view of `gradeParams.z` in the frame buffer: everything, no shadows, no lamps, only the shadow, only the lamps, plain (the style left out: `MakeStyle` with `plain` true gives all zeros).

## Collision and the walking body

`TriMesh::PushOut` moves a ball out of the triangles near it: the closest point on each triangle (`scene/TriMath.h`), a few rounds.
`Moments::PushOut` does the same over the solid of a moment, the things that are open now and the circle of the lantern. They
only move the ball; speed is the job of the body.

`Player` is three balls of the collision radius (feet, halfway, eye); the camera is the eye. A frame is cut into steps
shorter than the radius. In each step: the ground speed moves toward the keys with an acceleration; gravity (stronger while
falling); the jump (its rise is cut when Space is let go); the move; the push-out. The push removes the part of the speed
that goes into what was hit, so a floor ends a fall and a too steep slope turns it into a slide. The walk is taken away only
by contacts that are not ground: `Moments::WallPush` asks each triangle on its own (a floor touched in the same step is never
mixed into a wall: the sum of the two can point up), so a ramp keeps the walk and a wall stops it. A probe below the feet
decides whether the body stands (up to about 53 degrees). A body that stood and did not jump is kept on the ground when the
ground drops by up to 0.35 m in one step (`SnapToGround`), so it walks down ramps and low steps without small hops. Coyote
time and a jump buffer make the timing forgiving. `TownSession::PlaceEye` eases small changes of the eye height and dips the
eye on a hard landing.

## Guidance

`scene/Guidance.h` follows the main path (`TownData::steps`, written by the model in the order the path is played). The goal is
the first step that is not done: a thing open, or the lantern set down at its spot; when every step is done, the signal mast. The
help is the lantern as compass (`scene/Flame.h`): its flame, in the hand and set down, leans toward the goal (18 degrees; 40 and
with sparks after 30 s without progress, where progress is a step done, a new 6 m cell of ground or a new closest approach by 4 m;
upright within 2 m of the goal). H asks for more in two steps: the lantern is raised and a beam sweeps for 6 s; a second press, only
after 90 s without progress, shows a frame round the thing to use and a line to it. `TownSession` also offers the prompts TAB (after
8 s of walking, to a player who has not switched) and M (after 40 s, to one who has not opened the map) (`Prompts.h`).

## The signal mast and the first frame

The sea wall hides the bell tower from the quay in both moments (`tests/test_firstminute.cpp` casts the rays), so the far goal is a
signal mast over it with three lamps (`TownData::emblems`, drawn by `scene/Emblems.h`): dark until the thing named in the file (the
bell of their district) is open, then warm and glowing in both moments. The first frame is set by the town file (`spawnYaw`,
`spawnPitch`): the heap of the quay, the winch of the sea gate and the lamps are in the picture for every lens and screen.

## The map

The model writes the plan of both moments (`TownData::plan`, cells of 0.5 m, a class each: water, cliff, building, street) and the
overlooks. `MapMemory` marks the cells the body has been near (15 m), has looked over from an overlook or has lit by solving a puzzle,
and holds the markers (ten at most, six shapes, the moment each was set in). `MapView` draws two sheets of tracing paper into the
overlay batch (neutral ink for what is in both moments, warm for A only, cool for B only, the sheet on top over the other) as smooth
shapes (`FillField`: 2 x 2 averages cut along a level, whole cells joined into rectangles), the fog over what is not seen, the rings of
the puzzles to solve, the things that change in their present state (`MapScene::thingOpen`), the player, the lantern, the tower with its
lamps and the markers; it needs no pass of its own. The ink and the
fog are kept in a `MapCache` (a second `OverlayBatch`) and made again only when the cells seen, the sheet, the mode or the window change.
The batch holds 240000 vertices; the largest map (the whole town seen, every zoom) uses about 150000. While it is open `TownSession` stands still, the pointer is free (`Window::CursorClient`, `SetCaptureAllowed`) and
Esc closes it (`SetEscapeQuits`, `ConsumeEscape`).

## The overlay

`OverlayBatch` collects what is drawn over the picture in one frame as triangles in screen pixels: rectangles, frames and
text in `Font5x7`, a bitmap font written for the project (each row of lit pixels of a glyph is one rectangle). `Overlay`
copies the batch into one dynamic vertex buffer (pixels to clip space, sRGB to linear) and draws it last, with alpha
blending and without depth. `Hud` fills the batch; `Prompts` decides which prompt it shows from a small `Input` that
`TownSession` fills from the game (`ChoosePrompt`), so the rule is tested without a window.

## Device recovery

`Present` reports a removed or reset device; `Device` only sets `Lost()`. `TownSession::RecoverDevice` then shuts down the
`Gpu` object (`app/Gpu.h`: every GPU object in one place), makes a new `Device` and initialises everything again (`Gpu::Recover`),
at most five times per run (`kMaxRecoveries`). The town, the body and the camera live outside it and are kept. F9 runs the same
path for testing.

## Tests

Each file in `tests/` is a program of its own: `tests/run_tests.ps1` builds and runs them with MSVC, and the header of each
file gives the g++ command. All but `test_mesh.cpp` share `tests/check.h`. None needs a window or a GPU: the shader maths of `shaders/lighting.hlsli`
is written in the part of HLSL that is also C++ and runs through `tests/hlsl_shim.h`, and `test_route.cpp` plays the main path
of the whole town with a solver and reports any way round a puzzle.

## Debugging

- Every GPU object is named `<System>.<Resource>`; every pass is a `GpuEvent` (RenderDoc, PIX, the VS graphics debugger).
- Debug builds: the D3D11 and DXGI debug layers, a break on error under a debugger, a leak report at exit.
- `logs/dual.log`: the town loaded, things opened, the lantern, refused switches, falls, jump heights.
