# Changelog

The detailed record of each step before 2026-11-24 (every test count, every deliberate bug that a test caught) is in the
git history of this file; it is summarised here.

## m2-layers (in progress)

### 2026-11-25: the centre, and the whole main path proved

- Made in the model: the centre (`docs/DESIGN.md` 7.4): the west bridge falls whole in B and its spans lie round the gorge
  as a stair down to its floor; a gate on the rock keeps it shut in A (seen, never crossed); the lift on the south face is
  the thing `lift` (verb LOWER): its winch on the roof of the lift house over the moat lowers the cage in A; in B the
  lowered cage leaves its frame standing, a ladder of girders (steps of 1.5 m) up to an upper station on the plaza, and
  the cage never lowered lies wrecked at the foot. The old tunnel portal and cistern way are gone from the path. A
  parapet round the rim of the gorge (open at the bridges); walls between the districts (the lane below the locks, the
  plank, the gatehouse, the gardens north of the market street); the walls along the streets of the puzzles 9.5 m high
  (5 m let the body walk along their tops past the plank). The lock quays laid as flat tiles of 0.5 m (the coarse ground
  left lips the body could not pass inside the bend of the locks); a walkway over the head of the top basin to the water
  tower. Smaller: the scaffold steps 1.4 / 2.9 / 4.4 m and the terrace 6 m; the balcony of the gatehouse 2.4 m (1.5 m over
  the crate); the plank between roofs of 4.8 m (the yard rises to the sides); the east bridge reaches the flat of its
  bastion. `build_all()` builds the town in one place. `data/town.dlt` format 3 (verb 4: lower).
- Made in the game: the prompt LOWER.
- Checked: `tests/test_route.cpp` passes (11): the solver plays the sea gate, the canal, the day bell, the lantern, the
  lift, the dawn bell, the plank, the gatehouse and the night bell in this order; each is reached, nothing further on is
  reached before its turn, and the bastion is not reached before the lantern. It found on the way: a way round the canal
  past the end of the west terrace wall, two ways round the lantern (the gardens north of the market street; a slope
  the solver now treats as a stair), the walk along the street walls past the plank, a yard climbed to the neighbour roof,
  a step of 1.66 m to the balcony. Its ball stands 0.06 m over the floor (clear of a stair of 32 degrees);
  `test_route every x y z ...` prints every cell of a way. In `run_tests.ps1` (it takes a few minutes).
  `tests/test_things.cpp` 46, `tests/test_town.cpp` 35, `tests/test_lantern.cpp` 38, `tests/test_overlay.cpp` 281: pass.

### 2026-11-25: every district placed (first pass)

- Made in the model: the canal of the customs (harbour, beat 4: a crate pushed in A floats to the far quay and lies on the
  dry bed in B, a step); the scaffold of the market (west: the lantern on the scaffold of A under the roof of a shed, climbed
  in B, onto the terrace); the plank between two roofs (east: laid in A, holds in B); the gatehouse of the theatre (north:
  a crate pushed under its balcony in A, a jump from the balcony with a switch at its top onto the heap of B); the three
  bells rung in A at their ropes (a lamp lit in B over the rope; the title bar counts them). Walls along the streets at the
  puzzles and two retaining walls of the west terrace close the ways round. Foliage (crowns, scrub) is drawn, not solid.
  The town wall stands 4.5 m over the ground beside it. No house or arch over a puzzle. `data/town.dlt` format 3 (a verb
  per thing: OPEN, PUSH, LAY ACROSS, RING).
- Made in the game: the prompts PUSH, LAY ACROSS, RING, CALL THE LANTERN; E 8 m or more from the set-down lantern calls it
  back (refused when the body stands on its circle); a switch takes the coyote time away (no late jump off a floor that
  went with the moment).
- Made: `tests/test_route.cpp`, a solver over both moments of the whole town (walk, a jump onto a ledge of 1.65 m,
  the switch where the body stands, the switch in the air, the lantern as a circle) that plays the things in the order of
  the main path and lists any way round a puzzle; `test_route x y z [things] [L]` prints the way it found to a point.
- Checked: `tests/test_things.cpp` 46, `tests/test_town.cpp` 35 (five streets closed in A by the puzzles),
  `tests/test_lantern.cpp` 38: all pass. `test_route`: the sea gate, the canal and the day bell are reached in order.
- `tools/check.ps1`: the smoke run expects the start in B (Tab goes to A first), walks back east with S (the start looks
  west, at the heap), and waits for a killed game to end before it reads the log.
- Known issue (closed the same day, below): `test_route` stopped at the plank.

### 2026-11-25: the harbour, beat 3

- Decided: the harbour teaches the switch, the causal rule and the switch in the air; the lantern is first met in the shaft
  of the west, where it is the only answer (`docs/DESIGN.md` 8, 13, 14).
- Made in the model (`build_air_step`): the lower street closed at 36 m in both moments: in A by the barricade of the
  customs (5 m), with two crates of 1.2 and 2.4 m against it; in B by the front of a fallen house (3.4 m) that steps down
  on its far side, and the garden walls that fell with it (a way round below the street was found by the flood).
  `street_frame` reads a street at a distance as the game does.
- Checked: `tests/test_things.cpp` 46 (13 new): the flood: closed in A and in B, reached in B from the top of the sea
  stair, the far side open; in B alone a jump from the street does not reach the heap; the real body climbs the crates in A,
  a jump without a switch ends at the barricade, with a switch at its top it lands on the heap of B (3.6 m up) and walks
  down to the street. `tests/test_town.cpp`: in A the lower street is closed too.

### 2026-11-25: the harbour, beats 1 and 2

- Made: the things that change (`docs/DESIGN.md` 3.3, 12 step 4). `scene/Things.h`: a thing has a handle and boxes, each in
  its moments and in one state (shut, open); E at the handle in A opens it in A and in B at once, in B E does nothing, an
  opening that would close on the body is refused. The boxes of the state now are extra solids (`Moments::SetExtra`, with
  the same rule for the circle as the town's triangles), drawn as cubes; the boxes of B of an opened thing carry its trace
  (`Traces`), the handle of a shut thing glows gold in A. The prompt OPEN.
- Made in the model (`tools/layout/city_model.py` `build_harbour_beats`), `data/town.dlt` format 2 (the moment of the
  start, the things): beat 1, the switch: the game starts in B on the quay, where a heap of wreck closes the way west in B
  only. Beat 2, the causal rule: the portcullis of the sea gate (shut in both moments) is raised by its gold winch in A;
  the cargo of the customs fills the passage behind it in A; in B the cargo is gone and the portcullis stays raised. The
  heap of rubble of B in the sea gate is gone (the rusted portcullis closes it now).
- `RunTown`: starts in the file's moment (B: one ripple at the start), E opens a thing before it uses the lantern, the
  things drawn (the table of objects holds 128), the traces kept.
- Checked: `tests/test_things.cpp` (new, 33): the rules on a thing of their own; the data; a flood of every place the body
  walks to (steps of 1.2 m, any drop that lands): in B the start is cut off from the sea stair by the heap, in A not; from
  the foot of the stair the lower town is reached only in B after the gate was opened in A, never in A, never with it
  shut; the real body walks to the winch in A, opens it, is stopped by the cargo, switches where it stands and walks up in
  B. Caught when E is allowed in B. `tests/test_town.cpp`: in A every street is walked but the sea stair (the cargo).
  The other tests unchanged. Found by reasoning, not yet by a test: the bridge example of the lantern can be crossed by
  a switch in the air (`docs/DESIGN.md` 14).

### 2026-11-24: the lantern

- Made: the lantern (`docs/DESIGN.md` 3.2). `scene/Lantern.h`: E sets it down 0.9 m ahead on a floor of A, in A only and
  standing; E takes it up within 1.8 m in either moment, refused when that would leave the body inside something or without
  the floor it stands on. Set down, it keeps a sphere of A (3.5 m) in B. `scene/Moments.h`: the solid of both moments with
  that circle (a triangle of A counts where its point nearest to the ball is inside, a triangle of B where it is outside);
  `Player::SetTown` takes it (was `SetMeshes`). `TownData::Submerged` with a circle: the water of A is water inside it in B.
  `Models::SetLantern`, `cbuffer Frame` `lanternMode` and `lantern` (192 bytes): in B the batch of B is drawn without the
  inside of the sphere, then the batch and the still water of A in the light of A only inside it, with a warm rim
  (`LanternClip`). `RunTown`: E, the prompts PUT DOWN and TAKE, the lantern drawn, the switch checked with the circle; the
  place a fall comes back to is one where the body stands in both moments without the circle.
- Made: `Traces` in `scene/Lantern.h` (a thing changed in A comes up in B with a faint warm glow over 1.5 s); used by the
  first things that change, in step 4.
- Checked: `tests/test_lantern.cpp` (new, 38, on a small town of its own: the circle; the solid with it in both moments,
  a bridge of A held in B inside it and not outside, a wall of A inside and outside it; the rules of setting down and taking
  up; the body cannot cross a 6 m gap in B and walks across it with the lantern set down on the bridge in A; the water of A
  inside the circle in B; traces). Caught when the circle is ignored, when the refusal to take is removed, and when the
  side of the circle is not looked at. `tests/test_town.cpp` built with `SetTown` (it needs `data/town.dlt`, not in this
  upload). The other tests unchanged. The shader is compiled by the game at start: check with `tools/check.ps1`.

### 2026-11-24: the town of the model in the game

- Made: the layout model is the map. `python tools/layout/city_model.py export` checks the model and writes `data/town.dlt`
  (1.5 MB: lights, 32 materials, 19 stations, 10 streets, about 28700 polygons). `scene/Town.h` reads it, bakes one batch per
  moment (each polygon a fan with one normal, both sides; terrain and water from above) and the water of A, and builds the
  solid of each moment; `scene/TriMesh.h` is a solid of triangles in a grid of 4 m cells with a push-out like the ground's.
  `Player::SetMeshes`; `Models::SetStaticWater` (still water baked and drawn in the water pass), `Models::SetThemes`;
  `Landscape::AddSea` takes the level of the sea. `main.cpp`: `[map] town` (default true) runs `RunTown`: walk, switch
  (refused into a surface or into the water of the other moment), sink, fall, come back to the last place stood still on (and
  R); the looks of both moments from the model's lights. The greybox map stays with `[map] town = false`.
- Fixed in the model (found by the walking test): the sea stair ran into a tower of the sea gate (now through its middle);
  the market stairs ran into the parapet of the well (now west of it); the cliff stair zig-zagged into its own gate and ended
  in a pit in the graveyard (now straight; the graveyard at z 78 .. 96). The start is at x 14 on the quay, clear of the sea
  stair.
- Changed: `tools/check.ps1` expects the eye on the quay at -4.65 m. The post-build step copies `data\` next to the exe.
- Checked: `tests/test_town.cpp` (new, 35: the file refused when damaged; the data; the triangle solid on a floor and a
  wall from both sides; the drawn fronts face their normals, the water of A only; the body stands at the start in both
  moments, fits there and walks north on the quay; every street walked end to end in A without a jump; in B the sea stair
  and the lower street are closed by rubble; the moat is water in A and not in B), the other tests unchanged.

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
- The model checked and fixed (`tools/layout/city_model.py`, `docs/DESIGN.md` 7.5): a `check` mode measures every street
  against its slope limit and every joint of the main path; street ends are pinned to what they meet; the market street
  reaches the west bastion, the east stair the first quay, the upper street the top of the theatre, the cliff stair the
  graveyard through a gate in the wall; stairs beside the lock heads; the well rim and stair fixed (they were under the
  ground of the market); the tunnel portal in the gorge, the lift cage and frame, the gate of the west bridge, the gate
  tower of the bastion; the shortcuts of the plan removed and the main path drawn as in 7.4; the street_b view shows the
  circle of the lantern.
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
| KI-09 | The town has no puzzles, no lamps and no bells yet | start with `[map] town = true` | the rebuild is at step 3 of `docs/DESIGN.md` 12 | the town is walked, the lantern works, nothing more | step 4 |
| KI-10 | In the town a fall comes back to the last place the body stood still on, which may be just before a jump that keeps failing | jump short of a gap and fall | no anchor lamps in the town yet | a player may need R | the lamps of the town with step 4 |

## Closed issues

| ID | Symptom | Closed | How |
|---|---|---|---|
| KI-03 | The Release build treats warnings as errors; the M1 and M2 files had not been compiled with MSVC | 2026-11-17 | `Release \| x64` was the configuration of the first real build; after the `min` / `max` fix it ran. The rule stays: a new file must build in Release (check Q-9, done by `tools/check.ps1`) |
| KI-02 | A lost device ended the program | 2026-11-17 | `Gpu::Recover` in `main.cpp` builds device, triangle and models again (checks K-2, K-3 passed). Only the `F9` test path was run; a real device loss (driver reset, GPU switched off) has not been tried |
| KI-05 | The GPU used by default on the dual-GPU laptop was not confirmed | 2026-11-17 | the code picks the high-performance adapter and exports the two driver hints; check K-1 passed (NVIDIA RTX 5060) |
| KI-04 | Reverse-Z was undecided: depth is the plain `D32_FLOAT`, LESS, clear 1.0 | 2026-11-18 | decided to **keep** it: the scene is small (floor 24 m, near 0.1 m, far 100 m), no flicker has been seen, and the two ripple depth buffers use the same convention. Revisit only if far objects flicker (it would change `Projection.h`, `Device::BeginFrame`, `Models::CreateStates`, the ripple depth buffers and the depth state of `Triangle`) |
