# Architecture

The code as built: one program that loads the town as data and walks a body through it in two moments. The design is
[DESIGN.md](DESIGN.md); the rules of the code (units, coordinates, colour, depth) are in [CONVENTIONS.md](CONVENTIONS.md).
An earlier prototype with hand-placed rooms and puzzles was removed on 2026-11-26; it is in the git tag `m2-greybox`.

## One frame

```
WinMain > RunApp
  once:   log file, config, window (DPI aware), the town (TownData::Load), Gpu::Init (Device, Triangle, Models, Overlay; the
          polygons of the town baked into the static batch of each moment), RunTown: the walking body at the spawn point
  frame:  1. Window::PumpMessages
          2. lost device (or F9)   > Gpu::Recover: every GPU object made again on a new Device; the town and the body stay
          3. F5                    > reload the config, the themes of the moments, the settings of the body
          4. resize                > Device::Resize
          5. mouse and keys        > turn the camera; Tab goes through the LayerGuard (a switch into something, or into the
                                     water of the other moment, is refused)
          6. Models::Update        > the spin clock and the ripple clock
          7. walking               > Player::Update against the solid of the moment (Moments, with the circle of the lantern),
                                     the ledge climb (TryTownMantle), E (HandleUse: a thing of the past, else the lantern), water
                                     and falls (back to the last place the body stood still on), the camera (orbit, pulled in by walls)
             flying                > Moments::PushOut on the camera ball
          8. the drawn table       > the sea round the eye, the lantern, the things and their traces, the ring of a return, the
                                     avatar > Models::SetObjects
          9. render                > Device::BeginFrame, Models::Draw (one moment, or the ripple), Overlay::Draw (BuildHud: the
                                     prompt, the status line), Device::EndFrame (Present)
  end:    Gpu::Shutdown (Models, Triangle, Device: the debug layer reports leaks), log closed
```

What is drawn and what is solid come from the same data in the same frame, so they cannot disagree.

## Modules

```
main
 +- render   Device, Models, MeshData, LayerRipple, Projection, ShaderCompiler, GpuDebug, Triangle,
             Overlay, OverlayBatch, Font5x7, StaticBatch, Hud                                      > core, scene (data only)
 +- scene    Camera, SceneData, Collision, Terrain, Player, TownPlayer, Town, TriMesh, Moments,
             Lantern, Things, LayerGuard, Prompts, TownEffects                                     > standard library only
 +- core     Log, Assert, Config, Window                                                           > standard library, Windows
```

A dependency only points down. `render` is the only place that talks to Direct3D. Everything in `scene` and the pure parts
of `core` and `render` (`MeshData`, `LayerRipple`, `OverlayBatch`, `Font5x7`, `StaticBatch`, `Hud`) have no Windows or
Direct3D dependency and are tested without a window.

| Module | Job |
|---|---|
| `scene/Town.h` | reads `data/town.dlt`; bakes the drawn polygons; builds the solid of each moment |
| `scene/TriMesh.h` | triangles filed in a grid of 4 m cells; the push-out of a ball |
| `scene/Moments.h` | the solid of both moments with the circle of the lantern |
| `scene/Lantern.h` | the lantern: set down in A, a circle of A in B; the traces of things changed in A |
| `scene/Things.h` | the things that change: opened in A, a trace in B (the causal rule) |
| `scene/Player.h`, `scene/TownPlayer.h` | the walking body; the town's climb, camera clearance and avatar |
| `scene/LayerGuard.h` | refuses a switch that would put the body inside something |
| `scene/Prompts.h` | the one key prompt, chosen from the state of the game |
| `scene/TownEffects.h` | the sea round the eye and the ring of a return |
| `render/Hud.h` | the overlay of one frame |

## The town

The layout model `tools/layout/city_model.py` writes the town as data: `python tools/layout/city_model.py export` checks the
model and writes `data/town.dlt` (the format is in the docstring of its `export`). `scene/Town.h` reads it: the lights of the
two moments, the materials (an sRGB colour in A and in B, and whether it shines), the stations of the main path, the middle
lines of the streets, and the polygons of the town with their moment and their kind (solid, terrain, overlay, water).

- Drawn: `TownData::BakeLayer` gives one batch per moment, each polygon as a fan of triangles with one normal (Newell's), from
  both sides because the model's polygons have no fixed winding (the terrain and a surface of water only from above). The
  colours are the model's own (`color.a` 0: no stone tint). `BakeWater` gives the water of A, drawn by
  `Models::SetStaticWater` in the water pass. The looks of the two moments come from the model's lights (`ApplyTownThemes`
  in `main.cpp`).
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
- The loop: `RunTown` in `main.cpp` (with `HandleUse` for E, `ChoosePrompt` and `SetTitleBar`) walks, switches (refused into a surface or into the water of the other moment, with the
  circle), opens a thing at its handle or else sets the lantern down and takes it up (E), sinks in water, falls, and comes back to the last place the body stood
  still on (also R); that place is one where the body stands in both moments without the circle, so a return never ends in
  the air or in a wall. It draws the sea round the eye and
  the ring of a return (`scene/TownEffects.h`), the lantern and the things; the town is in the static batch. The game starts in the moment the
  file gives (B).

## Layers and the ripple

Every object carries a layer mask (A = 1, B = 2, both = 3); a draw call and a collision skip an object whose mask does not
hold the layer. Each layer has its own theme (light, sky, background, fog, and the stone of the city), never mixed: A is
the past, the whole city in a warm evening, B the present, the same city after a storm, cold and foggy, its stone dark and mossy. The model shader
gives the city (objects of both layers that do not glow) the tint and the weathering of the layer, and fogs everything by
its distance into the background colour; glowing things are seen through more of the fog. In the town, the renderer
also draws a world-aligned sky before the opaque scene, using the same full-screen triangle vertex shader as the
ripple. Each ripple target gets its own sky; the local lantern pass changes only geometry and water. The 256-byte
frame buffer carries the camera axes, sky colour and low-mist parameters. Opaque objects and water share the same
distance and height haze.

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

## Collision and the walking body

`ResolveSphere` (`scene/Collision.h`) pushes a ball out of every solid of a layer, exactly for boxes (also turned ones:
tilted round X, such as ramps, or turned round Y by a fixed yaw, such as the houses), spheres, cylinders and tori, in the
local space of each object, in a few rounds. It only moves the ball; speed is the job of the body.

`Player` is three balls of the collision radius (feet, halfway, eye); the camera is the eye. A frame is cut into steps
shorter than the radius. In each step: the ground speed moves toward the keys with an acceleration; gravity (stronger while
falling); the jump (its rise is cut when Space is let go); the move; the push-out. The push removes the part of the speed
that goes into what was hit, so a floor ends a fall and a too steep slope turns it into a slide. The walk is taken away
only by solids that are not ground (`AddWallPush`, per solid): a ramp keeps the walk, a wall stops it. A probe below the
feet decides whether the body stands (up to about 53 degrees). A body that stood and did not jump is kept on the ground when
the ground drops by up to 0.35 m in one step (`SnapToGround`), so it walks down ramps and low steps without small hops.
Coyote time and a jump buffer make the timing forgiving. `RunTown` eases small changes of the eye height and dips the eye
on a hard landing.

## The overlay

`OverlayBatch` collects what is drawn over the picture in one frame as triangles in screen pixels: rectangles, frames and
text in `Font5x7`, a bitmap font written for the project (each row of lit pixels of a glyph is one rectangle). `Overlay`
copies the batch into one dynamic vertex buffer (pixels to clip space, sRGB to linear) and draws it last, with alpha
blending and without depth. `Hud` fills the batch; `Prompts` decides which prompt it shows from a small `Input` that `main.cpp`
reads from the game, so the rule is tested without a window.

## Device recovery

`Present` reports a removed or reset device; `Device` only sets `Lost()`. `main.cpp` keeps every GPU object in one `Gpu`
struct, shuts it down, makes a new `Device` and initialises everything again, at most five times per run. The town, the body
and the camera live outside it and are kept. F9 runs the same path for testing.

## Debugging

- Every GPU object is named `<System>.<Resource>`; every pass is a `GpuEvent` (RenderDoc, PIX, the VS graphics debugger).
- Debug builds: the D3D11 and DXGI debug layers, a break on error under a debugger, a leak report at exit.
- `logs/dual.log`: the town loaded, things opened, the lantern, refused switches, falls, jump heights.
