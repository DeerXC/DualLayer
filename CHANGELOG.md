# Changelog

The detailed record of each step before 2026-11-24 (every test count, every deliberate bug that a test caught) is in the
git history of this file; it is summarised here.

## m2-layers (in progress)

### 2026-11-24: the design revised

- Decided: A is the past and B the present; the causal rule (what is done in A leaves its trace in B, never the other
  way); a lantern set down in A keeps a circle of A in B; puzzles start from the present; the player's changes show as
  traces; the greybox rooms are not kept, and the whole town is rebuilt on the layout model; puzzles that depend on another
  district and shortcuts back to finished places are dropped; the seal of a district is folded into its bell. An optional
  story twist is kept for the story stage. See `docs/DESIGN.md`.
- Changed (documents): `docs/DESIGN.md` is the one design document; it replaces `docs/LEVEL_DESIGN.md` and the art, map and
  gameplay notes. `README.md` describes the new direction. `docs/BUILD.md`: the keys say what the moments are and what E and
  R do now; the checks by eye are merged into 23 (E-1 .. E-23), without the ones that described what is gone (red and blue
  edges, ravines with the sea in them). `docs/ARCHITECTURE.md` and `docs/CONVENTIONS.md`: the pale outline instead of the
  edges, the sunken lanes and the open ground instead of the pits at -16, `WaterFx` among the modules, the two rows of the
  switch guard merged into one. This file is summarised.
- The model of the map (`tools/layout/city_model.py`, version 2): one hill town round the Rock of the Bell, a gorge with two
  bridges, the market and its well, five locks, the theatre and the graveyard on the cliff; a warm A and a cold B; storm
  damage by exposure; arches over the streets; varied houses. Its numbers are checked against the limits of the game in
  `docs/DESIGN.md` 7.5.

### 2026-11-23: the city

- Made: anchor lamps and a fall line per room; the prologue in seven beats from a harbour quay; worlds told by material and
  the aqueduct; the canal (lock and weir), the undercroft, the cellar; the prisms N1 and N2; orbs that never get stuck; the
  ground (`Terrain`, `Landscape`: hills, a mountain, the bay, the sea), plants by their ground, houses turned to their
  streets; recovery (no switch into water, places of return out of water); the bells and the memory fragments
  (`CityState`); the courtyard as a cloister; the scenery round the map (`Scenery`); the lock walk and the tower lock with
  the water tower; the market and the well; the theatre (N3); the graveyard; lamp travel; the three endings (`Ending`);
  resonance and the stuck timer (`Guidance`); the bell tower on the cathedral with a ring of lamps for the lights taken.
- Changed, after the model: the pits are sunken lanes and a body on the ground has fallen out of the map (`OnOpenGround`);
  the red and blue edges are gone, the outline in the other world is pale; the houses follow the streets of the model and
  are damaged by exposure; the halls are clad as houses and the square has an arcade.
- Made (water): water is drawn after everything opaque, blended and sorted (`Models`); the shader gives still water, the
  sea, running water, foam and the wet band their own look; `WaterFx` adds foam, the falls between the locks, the wet band
  and splashes; a body that falls into water sinks for 0.55 s before it comes back.
- Fixed: the switch guard asked only the map of the puzzle (now the whole table, `Puzzle::FitsIn`); `tests/test_stage.cpp`
  ran out of stack under MSVC (its solid tables are on the heap); the search for escapes counted places inside boxes; the
  parapet of the graveyard was reached from the pedestal of its lamp.
- Made: `BlockMaze::MinSwitches` (the courtyard needs two switches at the fewest).
- Checked with g++ at the end of the day: `test_stage` 4870, `test_collision` 4582, `test_water` 524, `test_hints` 359,
  `test_overlay` 226, `test_terrain` 185, `test_mechanisms` 106, `test_player` 104, `test_shapes` 80, `test_lights` 51,
  `test_puzzle` 50, `test_maze` 43, `test_ripple` 42, `test_batch` 20 checks passed; `test_reach`: 0 skips in 410240
  jumps from about 11000 places; also with a stack of 1 MB. `tools/check.ps1` ran on the laptop during the day (it found
  the stack overflow above); the water shader is compiled by the game at start.

### 2026-11-22: the map as a city

- Decided: a city in two states round a hub, three wings that each teach one object, shortcuts, a tower opened by three
  seals.
- Made: the skeleton (hub, wings, tower, rooms, seals); the search for skips (`tests/test_reach.cpp`), whose first runs found
  five ways round the map, all fixed; the west wing (`Mechanisms`, `WestWing`: orbs, sockets, plates, levers, gates); the
  overlay (prompts, a dot, a status line) with a bitmap font; the two looks of the city (fog, tint, weathering); the
  vocabulary (`Shapes`: stairs, railings, arcades, bridges, breaches, roofs, water, lifts) and the static batch; the ring of
  districts (a passage of B, a sky bridge of A, a lift); the sunken square and the cathedral.
- Changed, after a play-test: a ground snap on ramps, a faster walk (5.5 m/s), a field of view of 70 degrees, hub walls of
  10 m, the tower as the landmark.
- Fixed: the body could not walk up a ramp; a wall at the foot of a ramp kept the walk.

### 2026-11-19 .. 21: the first stages

- Made: the walking body (`Player`: three balls, gravity, a jump of exactly its height, coyote time, a jump buffer, a cut
  jump, acceleration); the puzzle logic (`Puzzle`: a block in both layers, a plate, a door, the switch guard); the first
  stage; the outlines of the other layer (`StageHints`); hidden lights; the block puzzle as a grid with a breadth-first
  solver (`BlockMaze`) and a layout chosen by a search (`tools/maze_search.cpp`); the courtyard in the game.
- Fixed: `tools/check.ps1` tested an old exe (now a full rebuild and a time check); a shader compile function was copied in
  two files (`ShaderCompiler`).

### 2026-11-17 .. 18: layers

- Made: a layer mask on every object and Tab; a theme per layer; the ripple (both layers drawn off-screen, a circle that
  opens from the centre and turns round); collision per layer (`Collision`: boxes, spheres, cylinders, tori); device
  recovery; `tools/check.ps1` (the tests, both builds, a game run, the repository check, the commit).
- Fixed: `std::min` / `std::max` broken by `windows.h` (C2589).

## m1-models (built and run together with M2, 2026-11-17)

- Made: a depth buffer in `Device` (D32_FLOAT, cleared to 1.0, made again on resize); `render/MeshData` (cube, sphere, cylinder, torus on the CPU, no Windows in it) with `tests/test_mesh.cpp`; `render/Models` (vertex / index buffers, one shader `models.hlsl`, one directional light, a checkered floor, wireframe / level-of-detail / spin keys); `render/Projection.h` (one lens for every pass, `Triangle` uses it too); the M0 triangle is hidden by default (`[test_triangle] show`).
- Why: the M1 step "depth + models + light" (draw a cube and at least two more models); the shapes are made in code first, loading files comes with the exporter (M1 step 4).
- Checked: compiled and run on Windows together with the M2 code (the first compile happened with M2); checks M1-1 .. M1-7 passed on 2026-11-17.

## m0-project-skeleton (in progress, 2026-11-10)

- Made: repository files, hand-written VS 2026 project (SDK pinned), Log / Assert / Config with tests, window with DPI awareness, D3D11 device with adapter list and debug layer, GPU names and events, flip-model swap chain with an sRGB view, test triangle.
- Checked: the project builds and runs on the laptop (first confirmed on 2026-11-17); checks M0-1 .. M0-8 passed.

## Known issues

| ID | Symptom | Reproduce | Cause (if known) | Impact | Plan |
|---|---|---|---|---|---|
| KI-01 | Shaders are compiled at run time from `shaders/` | start the exe from a folder without `shaders/` | by design (hot reload comes in a later milestone); the post-build step copies `shaders\` next to the exe | log error, no picture | accepted for now; precompile for the delivery build |
| KI-06 | Comments in the code point to sections of `docs/LEVEL_DESIGN.md`, which `docs/DESIGN.md` replaced | read a comment such as "docs/LEVEL_DESIGN.md 6.4.2" | the design document was merged on 2026-11-24 | the section is found only in the git history | the comments are rewritten with the rebuild of the town |
| KI-07 | Most logic tests build without `/WX` | `tests/run_tests.ps1` | only `core` and `camera` have `/WX` | a new warning in a test is reported as WARN, not as a failure | turn `/WX` on test by test once a run shows no warning |
| KI-08 | `.github/workflows/build.yml` has not run on a hosted runner | - | - | the CI file is untested | run it once the repository is public |

## Closed issues

| ID | Symptom | Closed | How |
|---|---|---|---|
| KI-03 | The Release build treats warnings as errors; the M1 and M2 files had not been compiled with MSVC | 2026-11-17 | `Release \| x64` was the configuration of the first real build; after the `min` / `max` fix it ran. The rule stays: a new file must build in Release (check Q-9, done by `tools/check.ps1`) |
| KI-02 | A lost device ended the program | 2026-11-17 | `Gpu::Recover` in `main.cpp` builds device, triangle and models again (checks K-2, K-3 passed). Only the `F9` test path was run; a real device loss (driver reset, GPU switched off) has not been tried |
| KI-05 | The GPU used by default on the dual-GPU laptop was not confirmed | 2026-11-17 | the code picks the high-performance adapter and exports the two driver hints; check K-1 passed (NVIDIA RTX 5060) |
| KI-04 | Reverse-Z was undecided: depth is the plain `D32_FLOAT`, LESS, clear 1.0 | 2026-11-18 | decided to **keep** it: the scene is small (floor 24 m, near 0.1 m, far 100 m), no flicker has been seen, and the two ripple depth buffers use the same convention. Revisit only if far objects flicker (it would change `Projection.h`, `Device::BeginFrame`, `Models::CreateStates`, the ripple depth buffers and the depth state of `Triangle`) |
