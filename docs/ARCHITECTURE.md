# Architecture

## One frame

```
WinMain > RunApp
  once:   log file, config, window (DPI aware), Gpu::Init (Device, Triangle, Models; the ground and the plants baked into the
          static batch: Landscape::BakeWorld), the map (Puzzle::Init from Stage::MakeStage), the walking body at the entry
          of the start room, with the ground of the world (Player::SetLand)
  frame:  1. Window::PumpMessages
          2. lost device (or F9)   > Gpu::Recover: every GPU object made again on a new Device; the map and the body stay
          3. F5                    > reload the config, the themes of the layers, the settings of the body
          4. resize                > Device::Resize
          5. mouse and keys        > turn the camera; Tab goes through the LayerGuard (a switch into something, or into the
                                     water of the other world, is refused: Recovery::SwitchAllowed)
          6. Models::Update        > the spin clock and the ripple clock
          7. the city state        > the seals ring the bells (CityState): the water of A rises, the machinery turns, night
                                     falls in A (Models::SetNight)
             the solid table       > the lifts and the canal move; Puzzle::Solids(layer) + Stage::AddMapSolids (the closed
                                     doors) + the lifts + the crate of the canal + the memory fragments + the gates of the
                                     wings
          8. walking               > Player::Update, Puzzle::Update, E (take or put an orb), MechanismSet::Update with the
                                     solid table of each layer, the room, a fall (below the fall line of the room or in
                                     water: back to Recovery::SafeRespawn), anchor lamps, seals and hidden lights, the goal;
             flying                > ResolveSphere on the camera ball
          9. the drawn table       > Stage::BuildDrawTable + AddMapDraw + the fragments + the sea round the eye + the hints +
                                     the lights > Models::SetObjects
         10. render                > Device::BeginFrame, Models::Draw (one layer, or the ripple), Overlay::Draw (prompts,
                                     status line), Device::EndFrame (Present)
  end:    Gpu::Shutdown (Models, Triangle, Device: the debug layer reports leaks), log closed
```

What is drawn and what is solid come from the same state in the same frame, so they cannot disagree.

## Modules

```
main
 +- render   Device, Models, MeshData, LayerRipple, Projection, ShaderCompiler, GpuDebug, Triangle,
             Overlay, OverlayBatch, Font5x7, StaticBatch                                           > core, scene (data only)
 +- scene    Camera, SceneData, Collision, Player, Puzzle, BlockMaze, Courtyard, Stage,
             StageHints, StageLights, Mechanisms, WestWing, CityParts, Prompts, Shapes, Anchors,
             Canal, OrbReturn, EndScreen, Terrain, Landscape, Recovery, CityState, Scenery,
             LampTravel, Ending, Guidance                                                          > standard library only
 +- core     Log, Assert, Config, Window                                                           > standard library, Windows
```

A dependency only points down. `render` is the only place that talks to Direct3D. Everything in `scene` and the pure parts
of `core` and `render` (`MeshData`, `LayerRipple`, `OverlayBatch`, `Font5x7`, `StaticBatch`) have no Windows or Direct3D dependency and are tested without a window.

## Layers and the ripple

Every object carries a layer mask (A = 1, B = 2, both = 3); a draw call and a collision skip an object whose mask does not
hold the layer. Each layer has its own theme (light, sky, background, fog, and the stone of the city), never mixed: A is
the whole city in a warm evening, B the same city after a storm, cold and foggy, its stone dark and mossy. The model shader
gives the city (objects of both layers that do not glow) the tint and the weathering of the layer, and fogs everything by
its distance into the background colour; glowing things are seen through more of the fog.

`LayerRipple` keeps the timing of a switch. While no ripple runs, one layer is drawn straight to the back buffer. During a
ripple, `Models::DrawRipple` draws the outer layer and the inner layer into two off-screen pictures (sRGB colour and their
own depth), then one screen-covering triangle (`shaders/ripple.hlsl`) takes each pixel from one picture or the other and
draws a glowing ring. Tab during a ripple turns it round: only the radius changes, so nothing jumps. `StateGuard` puts back
every pipeline state the ripple changes.

## The static batch

The fixed city (every box of the map that does not move, and the drawn-only decor) is baked once into one vertex buffer per
layer by `StaticBatch`: world positions and normals, the linear colour with the city flag, and the shader parameters per
vertex. `Models` draws it with one call per layer (`VSBatched`), then the moving and changing objects one by one (`VSMain`);
both feed the same pixel shader. The baked list is kept on the CPU, so a new device after a lost one bakes it again. The
drawn table keeps its order (`Stage::StaticDrawCount` fixed entries first), so `main.cpp` gives the renderer only the rest.

## The ground

`Terrain` is a height field: a grid of heights, each cell two triangles; `Height()` is the height of the triangles, so what is
drawn and what is solid are the same surface. A `Land` is two fields: 2 m cells round the map and 8 m cells out past the far
plane, with a hole under the fine one; the edge of the fine field is pinned to the coarse one, so they meet without a crack.
`Land::PushOut` moves a ball out of the triangles near it (the closest point on each), and the walking body asks it with its
solids (`Player::SetLand`): the ground is solid in both layers.

`Landscape` makes the land of the current map from noise (the hills, the mountain, the coast) and the site data of `Stage`:
the town levelled at the street, the pits (`Stage::kPits`) below every fall line. It places the plants by the ground they grow
on, and bakes the ground (flat-shaded triangles) and the plants (boxes) of each layer into one mesh that `Models` joins to its
static batch (`SetStaticExtra`), with the scenery of `Scenery` (the harbour, the town wall, the bell tower, the doors and
windows of the houses, the districts round the map: the market and its well, the theatre, the graveyard, the water tower, the
street lamps; boxes, drawn only). The sea is one box per layer round the eye, drawn each frame (`AddSea`). `Stage::Footings`
adds drawn rock under the walls and decks at the edges of the pits. `tests/test_terrain.cpp` proves that the ground is under
every floor, that no body reaches it from the map, and that every view down meets the map, the land or the sea.

## Recovery and the city state

`Recovery` keeps a body from getting stuck: `SwitchAllowed` refuses a switch into anything solid or into water of the target
layer; `SafeRespawn` gives the place a fall or R comes back to, out of the water of the layer of the body. `CityState` reads
the bells from the seals and eases what they change (the water of A, the night of A); the canal turns its lock by itself once
the machinery runs (`Canal::SetMachinery`); `Waterworks::SetRaise` lifts every water of A. The memory fragments are pieces of B
that appear once their hidden light is taken, solid in the tables of B. `LampTravel` holds the list of the lit lamps that E opens at a
lamp once a bell has rung; `main.cpp` freezes the walk while it is open and moves the body to the chosen lamp. `Ending` runs the last moment (three strokes; the world at the
third decides the ending). `Guidance` gives the resonance of the lantern near a hidden light, drawn by `BuildHud`, and brightens
the outlines of a room the body has stayed in (`BuildFrameTable`).

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
Coyote time and a jump buffer make the timing forgiving. `main.cpp` eases small changes of the eye height and dips the eye
on a hard landing.

## The block puzzle

`Puzzle` holds the crate of the courtyard (the block, both layers; `CrateBands` gives its drawn bands), the plate (layer A)
and the door it opens (layer B). The first push needs
a short walk into a face; held, the block goes on cell after cell, a little slower than the walk so the body stays at its
face. A push is refused when the next cell is not free in both layers; the refusal names the blocker. `LayerGuard` refuses
a layer switch that would put the body inside something.

`BlockMaze` is the same puzzle as a grid: a breadth-first search over (the cell of the block, the region the body can
reach), where the body moves through the union of the two layers and over the block, and the block only through their
intersection. `Courtyard` holds the rows of the courtyard; `tools/maze_search.cpp` chose them.

## The moving parts of a wing

`Mechanisms` holds the parts of a wing that change while it is played, as data and rules: orbs, triggers (sockets,
plates, levers; a trigger may latch) and gates (boxes that are there while their trigger is on, such as a bridge, or while
it is off, such as a door or bars). An orb is in one layer: carried, it takes the layer of the body; put down, it stays,
and falls straight down through what is not in its layer. Every frame `main.cpp` adds the gates that are there to the
solid tables, lets E take or put an orb, updates the orbs with the solid table of each layer and the triggers with the
body, and adds the gates, the pads and the orbs to the drawn table. `WestWing` is the data of the west wing.

## The map

`Stage` is the map as data: fixed boxes, ramps (boxes turned round X), the walls of the courtyard generated at compile
time from the rows of `Courtyard`, the rooms (bounds, entry, whether R resets a puzzle), the seals of the wings and the
doors they open. Every frame `main.cpp` builds the solid table of the layer of the body and the drawn table from it.

`tests/test_reach.cpp` searches the map for skips: from every place to stand, the real body jumps in every direction, with
and without a switch at the top, and a jump that ends higher in a room that is not joined by walking fails the test. The
landmarks of the wings are in the drawn table only; they stand outside every room.

The hub is a sunken square: a rim at street level holds every opening, and a pool round the cathedral is water in A
(`Stage::kWaters`, a wall) and dry in B. `main.cpp` puts a body that went under water back at the room entry before it
takes seals or lights, so nothing under the surface of A can be taken. The cathedral (the tower) stands on a plinth in the
pool; its rose window and spire are drawn only, above every place a body reaches.

The silhouette of the city is drawn only and generated from the map (`Stage::Battlements`, `Stage::Skyline`): merlons on
every thin stone wall at the height of the city walls, left out where a floor or a ceiling is near; houses and towers on
every plot of a grid round the map that is far enough from every room and box. All of it is part of `kDecor` and so of the
static batch.

The map is a ring: besides the square, a passage of B joins the west hall to the north corridor and a sky bridge of A joins the
north hall to the east hall. Lifts (`Stage::kLifts`) are moved by `UpdateLifts` before the solid tables are built, so the
body stands on them where they are in this frame; a lift is opened by a seal.

`Shapes` is the vocabulary the map is built from: stairs (walked as a ramp, drawn as steps), railings, arcades, bridges whole
or broken in B, breach walls, roofs, water of layer A (a body under it goes back to the room entry; orbs float on it) and
lifts (a platform that carries what stands on it, `Player::Carry`).

Every room has its own fall line (`Stage::FallLine`), a few metres under its floors. `Anchors` keeps the anchor lamps: a lit
lamp is where a fall or R in its room comes back (`AnchorSet::RespawnPoint`), else the entry of the room. `Canal` is the lock
of the east district: water of layer A whose level a wheel raises (only in A), and a crate that floats on it in A and lies on
the bed in B; a second wheel, the weir, lets the crate drift west, in both layers at once. It moves before the solid tables are
built, like a lift, and carries a body that stands on the crate of its layer.

`CityParts` joins every moving part of the city into the one `MechanismSet` of `main.cpp`: the west wing, the bars and the
lever of the undercroft, the prism of the north hall. A gate may swap its layer instead of coming and going (the portcullis that
the prism moves). After `MechanismSet::Update`, `OrbReturn` asks of every orb that came to rest whether a body of its world
could take it back (a place to stand within reach, in a room, out of the water); a stranded orb goes back to its start.
`EndScreen` makes the lines of the panel shown after the goal. `Stage::Waterworks` keeps the water of A: full until the goal is
reached, then drained to the beds in a few seconds (the master sluice); every water is drawn each frame for that reason.

The switch guard (`LayerGuard`) is asked with the whole solid table of the target layer (`Puzzle::FitsIn`): the map, the
gates of the wings, the lifts and the crate. `Collision::PushOutOf` skips a solid whose bounding sphere the ball cannot
reach, an exact broad phase.

Single-layer boxes are drawn in their material (wood or iron in A, fallen masonry in B); red and blue are only their edges
in their own layer and their outlines in the other (`StageHints::WorldTinted`, `AddWorldEdges`). The water of A is fed by the
aqueduct (`Stage::kWaterFeeds`, drawn by `AqueductDecor`), which is broken in B.

`StageHints` adds to the drawn table what the map says without words: an outline in the other layer for every
single-layer box, the outline of a refused push's blocker in both layers, the frame of the courtyard door, the beacon over
the goal and the rings of the clear. `StageLights` keeps the hidden lights. None of these is ever solid.

## The overlay

`OverlayBatch` collects what is drawn over the picture in one frame as triangles in screen pixels: rectangles, frames and
text in `Font5x7`, a bitmap font written for the project (each row of lit pixels of a glyph is one rectangle). `Overlay`
copies the batch into one dynamic vertex buffer (pixels to clip space, sRGB to linear) and draws it last, with alpha
blending and without depth. `Prompts` decides which key prompt is shown from the state of the game (an orb in reach, the
place a held orb would go, how often a prompt was followed); `Mechanisms` answers those questions with the same code that
E uses, so the prompt never promises what E would not do.

## Device recovery

`Present` reports a removed or reset device; `Device` only sets `Lost()`. `main.cpp` keeps every GPU object in one `Gpu`
struct, shuts it down, makes a new `Device` and initialises everything again, at most five times per run. The map, the body
and the camera live outside it and are kept. F9 runs the same path for testing.

## Debugging

- Every GPU object is named `<System>.<Resource>`; every pass is a `GpuEvent` (RenderDoc, PIX, the VS graphics debugger).
- Debug builds: the D3D11 and DXGI debug layers, a break on error under a debugger, a leak report at exit.
- `logs/dual.log`: rooms, seals, block moves, refused pushes and switches, falls, jump heights, the clear.
