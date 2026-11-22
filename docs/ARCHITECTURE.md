# Architecture

## One frame

```
WinMain > RunApp
  once:   log file, config, window (DPI aware), Gpu::Init (Device, Triangle, Models), the map (Puzzle::Init from
          Stage::MakeStage), the walking body at the entry of the start room
  frame:  1. Window::PumpMessages
          2. lost device (or F9)   > Gpu::Recover: every GPU object made again on a new Device; the map and the body stay
          3. F5                    > reload the config, the themes of the layers, the settings of the body
          4. resize                > Device::Resize
          5. mouse and keys        > turn the camera; Tab goes through the LayerGuard (a switch into something is refused)
          6. Models::Update        > the spin clock and the ripple clock
          7. the solid table       > Puzzle::Solids(layer) + Stage::AddMapSolids (the closed doors)
          8. walking               > Player::Update, Puzzle::Update, seals and hidden lights, the room, a fall, the goal;
             flying                > ResolveSphere on the camera ball
          9. the drawn table       > Stage::BuildDrawTable + AddMapDraw + the hints + the lights > Models::SetObjects
         10. render                > Device::BeginFrame, Models::Draw (one layer, or the ripple), Device::EndFrame (Present)
  end:    Gpu::Shutdown (Models, Triangle, Device: the debug layer reports leaks), log closed
```

What is drawn and what is solid come from the same state in the same frame, so they cannot disagree.

## Modules

```
main
 +- render   Device, Models, MeshData, LayerRipple, Projection, ShaderCompiler, GpuDebug, Triangle   > core, scene (data only)
 +- scene    Camera, SceneData, Collision, Player, Puzzle, BlockMaze, Courtyard, Stage,
             StageHints, StageLights                                                               > standard library only
 +- core     Log, Assert, Config, Window                                                           > standard library, Windows
```

A dependency only points down. `render` is the only place that talks to Direct3D. Everything in `scene` and the pure parts
of `core` and `render` (`MeshData`, `LayerRipple`) have no Windows or Direct3D dependency and are tested without a window.

## Layers and the ripple

Every object carries a layer mask (A = 1, B = 2, both = 3); a draw call and a collision skip an object whose mask does not
hold the layer. Each layer has its own theme (light, sky, background), never mixed.

`LayerRipple` keeps the timing of a switch. While no ripple runs, one layer is drawn straight to the back buffer. During a
ripple, `Models::DrawRipple` draws the outer layer and the inner layer into two off-screen pictures (sRGB colour and their
own depth), then one screen-covering triangle (`shaders/ripple.hlsl`) takes each pixel from one picture or the other and
draws a glowing ring. Tab during a ripple turns it round: only the radius changes, so nothing jumps. `StateGuard` puts back
every pipeline state the ripple changes.

## Collision and the walking body

`ResolveSphere` (`scene/Collision.h`) pushes a ball out of every solid of a layer, exactly for boxes (also turned ones,
such as ramps), spheres, cylinders and tori, in the local space of each object, in a few rounds. It only moves the ball;
speed is the job of the body.

`Player` is three balls of the collision radius (feet, halfway, eye); the camera is the eye. A frame is cut into steps
shorter than the radius. In each step: the ground speed moves toward the keys with an acceleration; gravity (stronger while
falling); the jump (its rise is cut when Space is let go); the move; the push-out. The push removes the part of the speed
that goes into what was hit, so a floor ends a fall and a too steep slope turns it into a slide. The walk is taken away
only by solids that are not ground (`AddWallPush`, per solid): a ramp keeps the walk, a wall stops it. A probe below the
feet decides whether the body stands (up to about 53 degrees). A body that stood and did not jump is kept on the ground when
the ground drops by up to 0.35 m in one step (`SnapToGround`), so it walks down ramps and low steps without small hops.
Coyote time and a jump buffer make the timing forgiving. `main.cpp` eases small changes of the eye height and dips the eye
on a hard landing.

## The block puzzle

`Puzzle` holds the purple block (both layers), the plate (layer A) and the door it opens (layer B). The first push needs
a short walk into a face; held, the block goes on cell after cell, a little slower than the walk so the body stays at its
face. A push is refused when the next cell is not free in both layers; the refusal names the blocker. `LayerGuard` refuses
a layer switch that would put the body inside something.

`BlockMaze` is the same puzzle as a grid: a breadth-first search over (the cell of the block, the region the body can
reach), where the body moves through the union of the two layers and over the block, and the block only through their
intersection. `Courtyard` holds the rows of the courtyard; `tools/maze_search.cpp` chose them.

## The map

`Stage` is the map as data: fixed boxes, ramps (boxes turned round X), the walls of the courtyard generated at compile
time from the rows of `Courtyard`, the rooms (bounds, entry, whether R resets a puzzle), the seals of the wings and the
doors they open. Every frame `main.cpp` builds the solid table of the layer of the body and the drawn table from it.

`StageHints` adds to the drawn table what the map says without words: an outline in the other layer for every
single-layer box, the outline of a refused push's blocker in both layers, the frame of the courtyard door, the beacon over
the goal and the rings of the clear. `StageLights` keeps the hidden lights. None of these is ever solid.

## Device recovery

`Present` reports a removed or reset device; `Device` only sets `Lost()`. `main.cpp` keeps every GPU object in one `Gpu`
struct, shuts it down, makes a new `Device` and initialises everything again, at most five times per run. The map, the body
and the camera live outside it and are kept. F9 runs the same path for testing.

## Debugging

- Every GPU object is named `<System>.<Resource>`; every pass is a `GpuEvent` (RenderDoc, PIX, the VS graphics debugger).
- Debug builds: the D3D11 and DXGI debug layers, a break on error under a debugger, a leak report at exit.
- `logs/dual.log`: rooms, seals, block moves, refused pushes and switches, falls, jump heights, the clear.
