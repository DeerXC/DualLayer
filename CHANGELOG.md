# Changelog

The detailed record of each step before 2026-11-24 (every test count, every deliberate bug that a test caught) is in the
git history of this file; it is summarised here.

## m2-layers (in progress)

### 2026-11-26: exploration map pass 2 of 2

- Changed: the map is smooth. Ink is no longer one rectangle per cell: the plan is averaged over 2 x 2 cells and cut along a level (`map_detail::FillField`), so a staircase becomes a slope; with many cells in the window the cut is made on a grid of 2 x 2 (up to 4 x 4) plan cells with partial cover. Neighbouring whole cells are joined into rectangles, along a row and up the rows, and every shared edge is computed from the same numbers (no cracks). On the real town the drawn area is within 3% of the cells' area (a round building of 20 m and one of 60 m are tested). The streaks of steep ground that the plan holds are not drawn (cliff is cut at a higher level).
- Changed: the plan of the model (`tools/layout/city_model.py build_plan`) counts as cliff only terrain steeper than about 57 degrees (`PLAN_CLIFF_NY` 0.55; it was 0.75, 41 degrees) and drops cliff of a single cell. The risers of the hillside's terraces were long thin stripes across the town on the map; the cliff of the plan falls from 23748 to 6303 cells in A. `data/town.dlt` is written again (`python tools/layout/city_model.py export`); the file is in this update.
- Added: the things that change are drawn as they are now (`map_detail::DrawThings`, `MapScene::thingOpen`): the footprint of each box of the present state in the moment it belongs to, strong on the sheet on top and faint on the other, only where the place is seen; a thing in its second state has a pale outline; bells are not drawn. The crates, the plank, the gate and the stairs of the lift in B follow what the player does.
- Changed: the whole town is under fog from the first frame and the paper shows all of it (zoom 1; it used to show what was seen, with a margin). The fog is a soft cloud (four layers, then pale and dark wisps) with an uneven edge; nothing under it is inked. The feather and the noise are in metres, so they look the same at every zoom.
- Added: every puzzle still to be solved shows through the fog as a broad ring with a question mark (the next one with a second ring and a disc); its centre is a few metres from the true place. A solved puzzle lights its district (22 m) for good (`MapMemory::Reveal`, once per step) and its ring goes. `Guidance::IsDone`.
- Added: zoom 1 x to 4 x, moving the window, centring on the player, six marker shapes and ten markers, each remembering the sheet it was set on (`MapMarker::type`, `::layer`); the buttons ZOOM +, ZOOM -, CENTER and MARK beside MAP and DIFFERENCE; the keys +, -, number pad, Page Up/Down, arrows, C or R and T. The name of the sheet on top (A PAST, B NOW) and a north arrow are on the paper. The window never leaves the plan.
- Changed: the ink and the fog are kept in a `MapCache` and made again only when the cells seen, the sheet, the mode or the window change. `OverlayBatch` holds 240000 vertices (it was 90000) and has `Append`. The largest map (everything seen, any zoom) uses about 150000.
- Checked: `test_map` (448 checks) and `test_overlay` (220) build with g++ -O2 -Wall -Wextra -Wshadow -Wconversion without a warning and pass against the real `data/town.dlt`, as do `test_town`, `test_guidance`; the map was drawn to a picture through a software rasteriser (the real `BuildMap`) to judge the shapes. `TownSession.cpp` could not be built here (Windows): run `tools\check.ps1`, then play E-27 of `docs/BUILD.md`.

### 2026-11-26: exploration map pass 1 of 2

- Changed: the town plan is sampled every 0.5 m instead of 1 m (400 x 404 cells). Unseen map space is stylised vellum fog: violet in A and teal in B; walking and overlooks still clear the same world-space radius.
- Added: clickable MAP / DIFFERENCE choices beside the paper. DIFFERENCE hides plan cells shared by A and B and keeps only the warm/cool differences; Tab still turns the top sheet and never switches the game world. The map accepts left clicks while mouse capture is disabled.
- Checked: `test_map` passes 126 checks with no compiler warning; TownSession and Window compile for Windows with the warning set used by the project. Pass 2 remains the puzzle halos, solved-region illumination, live map-state changes, height bands, zoom/pan/recenter and expanded typed markers.

### 2026-11-26: the traveller and the lantern

- Changed: the traveller is a stylised figure (`scene/TownPlayer.h`, `docs/DESIGN.md` 6.7), 45 parts instead of 15, 1.72 m from the feet to the top of the hood as before (a test measures it in every step of a stride). Rounded forms replace the boxes: a round hood with a peak, a face with two dark eyes, a bell of a coat, a belt with a pouch, brass buttons, a pack with a bedroll, big boots. The scarf is a collar and two tails that stream behind at a run; the coat has a vent that lifts. The teal coat, the hood and the pack of the starting silhouette are kept; the coat and the hood are a little lighter, so that they read against the moonlit stone of B.
- Changed: the figure is posed with two bones per limb. A leg that swings forward bends its knee; the body comes down as the legs spread, so that the foot that stands is on the floor (before, a foot could hang above it); the upper body leans about the waist when he runs; the arm that holds the lantern bends at the elbow and raises it for H. A breath moves the chest when he stands still.
- Changed: the lantern (`scene/LanternModel.h`, new): a bail ring, a chimney, a brass cap, four iron posts, a glass globe that glows, a flame inside that wavers, a rim and a foot, 12 parts. The one model is used in both states. In the hand it hangs from the ring a little outside the right hand and sways with the stride. Set down (`scene/Lantern.h`) it hangs from the arm of a small iron post, with the glass at the centre of its circle (0.45 m over the floor), instead of a lamp cube on a stake. A set-down lantern draws 16 parts and sways a little.
- Changed: the flame of the guidance (`scene/Flame.h`) is a spindle (a sphere, pointed at both ends) over the chimney, 0.34 m long (it was a cylinder of 0.42 m) and 0.085 m across.
- Changed: the light of the lantern. Its colour is an amber (1, 0.76, 0.44) instead of the white-gold of the lamps of the mast; it flickers by 12% (it was 10%); its glow is 1.0 m across (it was 1.4 m, a disc that cut into the wall and the body); the light starts at the glass and follows the sway; a set-down lantern lights from 4 cm over the middle of the glass. `LampFlicker` (`render/Lights.h`) has a third, finer tremble (still within its amount, for every lamp). `HaloFalloff` (`shaders/lighting.hlsli`) is a broad soft skirt with a brighter heart, so that no glow has an edge; it is a little tighter for every lamp.
- Changed: the table of what is drawn one by one holds 512 objects (it was 400): the traveller with the lantern and the flame is 61 parts and a set-down lantern with its flame 20. The beam of H starts at the new place of the lantern in the raised hand.
- Checked: all 19 fast logic tests build without a warning (g++ -O2 -Wall -Wextra -Wshadow -Wconversion) and pass, and `test_route` plays the nine steps in order (12 checks); `town_player` (80 checks) measures the stature, the floor, the stride, the lean, the scarf, the knee, the lantern in the hand and the flame; `lantern` (56) the post, the hook, the swing and the turn of the model; `guidance` (59) and `lighting` (92) are updated for the new counts. Every source of the game compiles for Windows with clang against a stand-in for DirectXMath without a warning and the shaders pass the HLSL front end. A picture of the figure was made with a software stand-in for the shader to judge the shapes; it is not the game. MSVC, the Visual Studio build and a run of the game are not done: run `tools\check.ps1`, then play E-26 of `docs/BUILD.md`.

### 2026-11-26: the style of the light

- Added: the light is hand-painted, not photographic (`shaders/lighting.hlsli`, "the style of the light"; `docs/DESIGN.md` 6.3). The sun falls on a face in three soft bands (`ToonRamp`); the sky light of a face that the sun does not reach leans to a cool colour (blue-violet in A, indigo in B); clouds drift soft-edged shadows over the town along the direction of the light (`CloudShade`); an edge that faces the light has a rim (`RimTerm`: none on a floor, a third on the stone of the city); a lamp lights its pool in four soft rings with a whiter core and an amber edge (`LampShadeStyled`); the stone is painted with soft blotches of value and hue, tall on a wall like stains, and is darker and cooler near the water (`PaintAlbedo`); the far haze leans to lavender in A and to teal in B (`StyleHaze`); and the picture is graded in the model shader before the soft shoulder (`StyleGrade`: contrast round the middle grey, the darks and the lights toned, saturation, a dark corner).
- Added: rays of light (`LightShafts` in `models.hlsl`). Each ray of sight, from the eye to a surface or into the sky, is sampled `shaft_steps` times (14) through the shadow map to 48 m, moved by a noise per pixel; only a ray that crosses the edge of a shadow shows a shaft (`ShaftEdge`), so the sky and the distance keep their gradient; the air is thicker near the sea and the rays are strongest toward the sun.
- Added: `[lighting]` keys `style`, `toon`, `cool`, `rim`, `clouds_a`, `clouds_b`, `lamp_rings`, `shafts`, `shafts_a`, `shafts_b`, `shaft_steps`, `wash`, `damp`, `haze`, `grade`, `saturation`, `contrast`, `vignette` (`config/default.ini`, `docs/BUILD.md`); the colours of A and B are `StyleLookOf` in `render/LightingSettings.h`, and `MakeStyle` turns the numbers into the registers of the shader. `style = 0` gives the light of the entry above. The key L has a sixth view, "plain": the same light with none of the style.
- Changed: the frame buffer of the model shader is 464 bytes (it was 368): `gradeParams2` is used and six registers follow it (`styleCool`, `styleLow`, `styleHigh`, `styleLight`, `styleFar`, `styleMisc`). `LampShade` is now `LampShadeStyled` with a style of 0 (the old function stays and gives the same numbers). `Finish` takes the place on the screen. `tests/hlsl_shim.h` has `smoothstep`, `lerp`, `frac`, `floor`, `sin` and `sqrt`. The name `far` is not used in the C++ of the settings (`windows.h` defines it): it is `hazeTint`.
- Checked: the new `tests/test_style.cpp` (95 checks) and 18 of the other 19 logic tests pass (`lighting` still 92); the route solver `test_route`, which does not touch the light, was not run again. It runs the functions of the style as C++: the rings of a lamp (0 at 0, only rising, plateaus and risers, an amount of 0 exactly the smooth light), the bands (the ends, only rising, flat inside a band), the clouds (the range, the share of the ground under them, a short soft edge, the drift, the same value along a ray to the sun), the rim, the cool fill only in the shadow, the painted stone and the damp, the haze, the grade (identity when neutral, the middle grey kept, no negative colour), the settings and the registers (plain is all zeros, the master, the shafts need the shadows, A and B differ). The three shaders compile with a HLSL front end (not the compiler of the game: it compiles them at start, so an error shows in the log); its offsets for every field of the frame buffer (464 bytes) are those of the C++ struct, checked field by field. `Models.cpp`, `TownSession.cpp` and `Gpu.cpp` compile for Windows with clang (-Wall -Wextra -Wshadow -Wconversion, no warning). A software picture of the town through a port of the shader (the real functions of `lighting.hlsli`, the real `MakeStyle`, `FitSunShadow` and `SelectLamps`) was used to set the looks; it is not the game. MSVC, the Visual Studio build and a run of the game are not done: run `tools\check.ps1`, then play E-25 of `docs/BUILD.md`, and note the frame time with `shafts = false` and `shaft_steps = 8`.

### 2026-11-26: the light

- Added: the sun of A and the moon of B cast shadows (`render/ShadowMath.h`, the shadow pass of `Models::DrawObjects`). One map of 2048 x 2048 texels covers 60 m round a point ahead of the camera, moved on the texel grid so that nothing crawls; the same two vertex shaders draw into it with no pixel shader, both sides, with no clip by the near plane. The picture looks it up with 3 x 3 taps of a hardware comparison, a normal offset and a bias in texels, and fades it out at the edge of the map. A shadow takes 85% of the direct light and none of the sky light. The circle of the lantern has none. A map that cannot be made switches the shadows off and the log says so.
- Added: the lamps are lights (`render/Lights.h`, `scene/Lamps.h`). The model writes a lamp for every lantern it builds (45 of the streets, 2 of the plaza, 2 lights of the harbour, 9 of the ring on the tower, 1 on the grave; `data/town.dlt` format 6); the game chooses the 32 that matter for a frame, fades a lamp out with distance (no blinking), flickers it and lights every surface with it (`LampShade` in `shaders/lighting.hlsli`). The lantern of the traveller is a light in the hand (`AddTownAvatar` says where its glass is) and set down; a lamp of the signal mast that a bell has lit lights the tower, the lamp over the rope of a bell that was rung lights its street in B.
- Added: a soft shoulder instead of the cut-off of the screen (the exposure, then `SoftShoulder`, knee 0.78: nothing below it changes), a halo (`kMatHalo`, a soft sphere added to the picture over each lit lamp: 35% in A, 100% in B), and the haze of the fog glows toward the sun. The key L looks at the light in parts (no shadows, no lamps, only the shadow, only the lamps).
- Changed: the moon of B is at 0.8 and its sky light at 0.9 (`[lighting]`), so that the lamps and the lantern read in it; A as before. All the numbers are in the new `[lighting]` section of `config/default.ini` (F5 reloads it). The pools of light painted on the ground under the lamps of B (a polygon of the material `glow`) are gone: the lamps light the ground themselves. The frame buffer of the model shader is 368 bytes (it was 256) and there is a new buffer `Lamps` (b2); `StateGuard` keeps three buffers; `SceneData.h` has `kMatHalo` and `IsHalo`; `IsWaterMaterial` stops at 6. `TownSession` draws the table of 400 objects (it was 256) for the halos. `tools/check.ps1` reads the title bar (`light full`) and the log (`shadow map`).
- Checked: the new `tests/test_lighting.cpp` (92 checks) and the other 18 logic tests pass. It runs the maths of `shaders/lighting.hlsli` as C++ through a stand-in for the vector types, the choice of lamps, the shadow box, the settings, the lamps of the town file, the lantern, the bell lamps and the halos; and the whole shadow chain on a toy scene (a depth map drawn by a software rasteriser through the matrix, looked up with the formulas of `ShadowAt`): 13,895 points of ground, 265 of them in the shadow of a box, none speckled and none missing, and none speckled on a plane that the light grazes at a cosine of 0.25 (without the bias, 13,610 of the 13,895 are speckled: the test sees acne). The three shaders compile with a HLSL front end, which is not the compiler of the game (it compiles them at start, so an error shows in the log), and the offsets of every field of the three constant buffers that this front end computes (368, 160 and 1040 bytes) are those of the C++ structs. Every source compiles for Windows with clang (-Wall -Wextra -Wshadow -Wconversion, no warning) against a stand-in for DirectXMath. The picture of the lamp light on the plan of the town was made with the real shader maths. MSVC, the Visual Studio build and a run of the game are not done: run `tools\check.ps1`, then play E-21 to E-24 of `docs/BUILD.md`.

### 2026-11-26: the first minute and the map

- Found, by casting rays (`tests/test_firstminute.cpp`): **the sea wall hides the bell tower from the quay in both moments.** From
  the eye at the start, every ray to the south face of the tower is blocked by the wall 8 m ahead (its top is at +1.5 m); only the top
  5 m of the spire of A clears it, and the game starts in B. So the design of 2026-11-26 ("the tower with its three dark emblems in sight
  from the first frame") could not be built as written. Solved with a signal mast over the tower (`city_model.py build_signal_mast`):
  three round lamps on a yard at 61 m, clear of the wall, which the test proves for every lamp from the camera and the eye in A and in B.
  A first version of the mast put the middle lamp on the axis of the mast, which hid it from the quay; the same test caught it.
- Added: the three lamps (`scene/Emblems.h`: dark until the bell of their district is rung; a sphere for the well, a column for the cliff,
  a box for the lock; a pale collar each, so that a dark lamp reads against the sky of B). The title bar says "lamps N of 3".
- Added: the first frame is set by the town file (spawn yaw -0.55, pitch +0.29): the heap of the quay, the winch of the sea gate and the
  lamps are in the picture for the lenses of 60, 70 and 90 degrees and the screens 16:9, 16:10 and 4:3 (the lamps reach 0.88 of the half
  height at the narrowest). A first pitch of 0.22 put the tops of the lamps out of the picture at 60 degrees: found by the same test.
- Replaced (KI-15, in part): the fallback pillar of light is gone. The help is the lantern as compass (`scene/Guidance.h`,
  `scene/Flame.h`): its flame, in the hand and set down, leans 18 degrees toward the next step of the main path (the signal mast when all
  are done), and 40 degrees with sparks after 30 s without progress; upright at the goal. H in two steps: the lantern is raised and a warm
  beam sweeps for 6 s; a second press after 90 s without progress shows a frame round the thing to use and a line to it for 12 s. The
  flame is also on the avatar's lantern (`AddTownAvatar` takes a flame and how high the arm is raised) and on the lantern set down. The
  design had said 10 degrees; at the distance of the camera that cannot be seen, so it is `[guidance] lean_degrees`.
- Added: the map (`scene/MapMemory.h`, `render/MapView.h`, `src/app/TownSession`): M opens two sheets of tracing paper over the frozen
  game; what stands in both moments is neutral, what stands in one is warm (A) or cool (B), the sheet on top shows over the other
  (Tab turns it); only what the body has been within 15 m of, or has looked over from an overlook, is drawn; the tower and its lamps
  always; the player's arrow, the lantern, the markers (E with the pointer; five at most, each a column of light in the world). Drawn
  into the overlay batch, so no new shader or pass. The model writes the plan (cells of 1 m, four classes, both moments) and the
  overlooks. The prompt M MAP comes after 40 s of walking.
- Changed: `data/town.dlt` is format 5 (spawn pitch, emblems, overlooks, the plan). `OverlayBatch` has `AddTriangle` and `AddLine` and
  holds 90000 vertices (the whole town on the map is about 12500). `Window` has `CursorClient`, `SetCaptureAllowed`, `SetEscapeQuits` and
  `ConsumeEscape` (the pointer is free and Esc closes the map instead of the game). `tools/check.ps1` walks with D, not S (the new first
  frame looks north-west, and S would walk off the edge of the quay), and checks that M opens and closes the map and that Tab inside it
  does not switch the world.
- Checked: all 17 fast logic tests build without a warning (g++ -O2 -Wall -Wextra -Wshadow -Wconversion) and pass, among them the new
  `test_firstminute` (40 checks), `test_map` (124) and the rewritten `test_guidance` (59); `test_route` plays the nine steps in order
  (12 checks); every source of the game compiles for Windows with clang (-Wall -Wextra -Wshadow -Wconversion, no warning) against a
  stand-in for DirectXMath. The map was drawn from the real town to a picture and looked at. MSVC, the Visual Studio build and a run of
  the game are not done: run `tools\check.ps1`, then play E-16 to E-20 of `docs/BUILD.md`.
- Caught on the way: a variable called `near` (a `windows.h` macro) in the new guidance code compiled for the tests and not for Windows;
  the compile of every source for Windows found it before the commit, as for `far` the day before.

### 2026-11-26: the design of guidance and order

- Changed (documents): `docs/DESIGN.md` is rewritten where play showed it was wrong: a player who did not press H did not know
  what to do. The game now asks one thing from the first frame (the tower with three dark emblems) and the main route is no
  longer one line: the harbour and the lower town are the teaching ground, after which three ways (west, east, north) are done
  in any order, each of three beats and a bell, joined by the rock and not by each other (7.3, 7.4, 8). Guidance is layered
  (10): a goal in sight, a beacon at each way, the lantern's flame as a compass, H for more, a map of two sheets of tracing paper
  with markers, the glyph board as the whole quest log, and a lens on holding Tab (3.8). A minimap, a task list and a compass bar
  were considered and not adopted. Puzzle rooms get three beats and a list of templates (8). Nothing of this is built: the
  pillar and H of the entry below are the fallback.

### 2026-11-26: the guidance, two bugs found, the legacy removed

- Found and fixed: **an invisible floor in the town.** `Player::SetScene(nullptr, 0)` meant "no table" but fell back to the demo
  scene of the first prototype (`scene::kScene`): its floor of 24 x 24 m at y -1 was solid in the town, over the gorge beside the
  rock. A body dropped at (11, 7, 9) came to rest at y -1.00 instead of the gorge floor at -6.00. The body no longer knows any
  table of objects: it meets the solid of the town and nothing else.
- Found and fixed: **a wall touched together with a floor was taken for ground in every other frame.** The walk speed at a wall
  was 1.58 m/s and 0 in alternate frames (the push of the floor and of the wall were added before the test "does it point up").
  `Moments::WallPush` asks each triangle on its own, as the old code asked each box. Found by `tests/test_player.cpp`, whose
  wall test now runs on the triangles of the town and not on boxes.
- Found and fixed: `scene/Guidance.h` named a variable `far`, which `windows.h` defines as a macro: it compiled for the tests and
  not for Windows. Found by compiling every source for Windows (clang, -Wall -Wextra -Wshadow -Wconversion) before the commit.
- Added: the guidance (`docs/DESIGN.md` 10): `scene/Guidance.h`, `[guidance]` in the config, H. The main path is data now: the
  model writes `steps` into the town file (format 4); the game guides along them and `tests/test_route.cpp` plays them (the
  scaffold of the west was computed twice, in the model and in the test; now once). The prompt TAB SWITCH WORLD for a player who
  has not switched after 8 s of walking (`scene/Prompts.h`). The title bar says "steps N of 9".
- Removed (KI-12): `scene/Collision.h`, the height field and the noise (`scene/Terrain.h`; the triangle maths stays in
  `scene/TriMath.h`), the demo scene, the object table and the height field of `Player`, the spin of shapes, the night theme, the
  baking of boxes (`StaticBatch.h` keeps the vertex and the joining), the M0 test triangle with its shader, and their tests
  (`test_collision`, `test_terrain`; `test_batch` and `test_player` were rewritten on triangles): 7 files and 1,610 lines, and the
  parts of `Models`, `Player`, `SceneData` and `StaticBatch` that only they used.
- Changed (KI-11): `main.cpp` is 112 lines (it was 856); the loop is `app/TownSession` (steps of about 20 lines each: `ReadKeys`,
  `SwitchMoment`, `WalkAndUse`, `ReturnFromFall`, `PlaceEye`, `BuildDrawTable`, `Render`), the GPU objects `app/Gpu`, the config
  `app/AppConfig`, the keyboard `app/Input.h`.
- Checked: all 15 fast logic tests build without a warning (g++ -O2 -Wall -Wextra -Wshadow -Wconversion) and pass, and
  `test_route` plays the nine steps in order; every source of the game compiles for Windows with clang against a stand-in for
  DirectXMath (no warning). MSVC, the Visual Studio build and a run of the game are not done: run `tools\check.ps1`, then
  play E-16 and E-17 of `docs/BUILD.md`.

### 2026-11-26: the code tidied for review

- Removed: the greybox prototype that the town replaced (the hand-placed rooms, the block, orb and canal puzzles, the seals, the
  bells, the endings, the ground and the plants of the old map) with its tests and `tools/maze_search.cpp`: 23 headers, 9 tests.
  It stays in the git tag `m2-greybox`. `[map] town` and `[hints]` are gone from the configuration; the game always runs the town.
- Changed: `main.cpp` is the town only (1668 -> about 860 lines); the E key (`HandleUse`), the prompt (`ChoosePrompt`) and the title
  bar (`SetTitleBar`) left the frame loop. New small modules, each with a test: `scene/LayerGuard.h` (out of the old puzzle
  header), `scene/TownEffects.h` (the sea and the ring of a return), `render/Hud.h` (the overlay). `scene/Prompts.h` now holds
  one pure function from the state of the game to the prompt. `tests/test_batch.cpp` bakes a table made in the test;
  `tests/test_terrain.cpp` keeps the height field; `tests/test_overlay.cpp` keeps the font, the batch and the prompts.
- Style: every C++ file formatted with the repository `.clang-format` (no line over 120 columns); comments that pointed to the
  retired level design were rewritten; `docs/ARCHITECTURE.md`, `docs/CONVENTIONS.md`, `docs/BUILD.md` (the by-eye checks are
  numbered again from E-1) and the README describe only what exists.
- Checked: all 16 logic tests build without a warning (g++ -O2 -Wall -Wextra) and pass; every source file of the game compiles
  for Windows with clang (-Wall -Wextra, no warning) against a stand-in for DirectXMath. MSVC, the Visual Studio build and a run
  of the game on the laptop are not done: run `tools\check.ps1` before the commit.

### 2026-11-25: west bridge descent readability

- Refined the west bridgehead in B so the first fallen spans are visible through an open iron rail; the first two landing faces use lighter stone to make the descent readable without changing its collision or the A bridge.
- Added `tools/layout/west_loop_preview.py` for matched A/B, plan and elevation review of the west loop. The maintenance gallery remains a layout proposal, not runtime geometry.
- Consolidated the exploration rules in [Design](docs/DESIGN.md): a readable main spine, optional local loops, branches that answer visible spatial questions, useful reconnections before the finale and no forced district-spanning backtracking.
- `tests/test_town.cpp` (35) and `tests/test_town_player.cpp` (14) pass with g++. The full route check reached the west wreck, gorge, lift, plaza and water-tower bell before the local run was stopped; run the complete Windows check before release.

### Town camera review

- Added matched A/B review cameras, a route-anchor plan and a proposed north approach overlay from the existing model. Fixed preview polygon conversion and orthographic depth order. No game geometry, puzzle or runtime changes.


### 2026-11-25: town atmosphere prototype

- Added per-moment gradient skies, a sun or moon halo, ground bounce and near-ground mist. The normal view and both ripple pictures use the same sky pass; the lantern patch does not draw a separate sky.
- The atmosphere can be disabled from the render settings for comparison. Cast shadows, material textures and Windows visual acceptance remain open. No map geometry or puzzle logic changed.

### 2026-11-25: town player first pass

- Added the town player module and focused tests. The traveller distinguishes walking from sprinting and carries a visible lantern. The camera retracts around walls, and ledge climbs check support, clearance and water.
- The town-player, player and lantern logic tests pass with g++. Windows build, visual tuning and the sprint/climb route-bypass pass remain open. No town layout or finale code changed.

### 2026-11-25: next-version design consolidated

- Merged the approved direction and bell-tower finale into [Design](docs/DESIGN.md). Updated the README and marked the build and architecture notes as baseline documentation. No source, map data or tests changed.

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
| KI-15 | The guidance is built in part: the lamps, the flame, H (two steps), the first frame and the map; not built: a beacon at the start of a way, the lens (holding Tab), the third press of H, the glyphs on the gates, the flame upright in the lower town | read `docs/DESIGN.md` 10 | the three ways and their gates are not modelled (KI-16) | the player is guided to the next step but not yet shown a choice between three ways | build with the three ways (DESIGN 12, steps 2 and 3) |
| KI-16 | The main route is one line (west, rock, east, north), in the model, in `steps` and in `tests/test_route.cpp`; the three ways of DESIGN 7.4 are not modelled | read `steps`; the locks start from the rock, the upper street from the top lock | the order was fixed before the town was linear in play | the districts cannot be done in any order | model the stairs from the lower town, the links to the plaza, `goals`, and the tests of DESIGN 11 (step 3) |
| KI-13 | The lamps of the mast, the flame, H, the map and the first frame have not been seen in the game | play E-16 .. E-20 | built on the 2026-11-26 without a Windows run | the lamps may be too small, too dim or lost in the fog or the sky of B; the flame may lean too little to see; the beam and the frame may cross walls; the map may be crowded | tune `[guidance]`, `Emblems.h` (size in the model, glow) and `MapView.h` after a play-test |
| KI-17 | The light (the shadows, the lamps, the shoulder, the halos) has not been seen in the game | play E-21 .. E-24 | built on 2026-11-26 without a Windows run or a GPU | the shadows may speckle or float off their objects; the lamps and the lantern may be too strong or too weak against the moon of B; a halo may hide its lamp; the picture may be darker than before (the moon at 0.8, the shoulder); the shadow pass may cost frames on the laptop | tune `[lighting]` (`shadow_bias`, `shadow_normal`, `lamps_b`, `lantern`, `sun_b`, `exposure`, `halo_b`, `shadow_size`), then fix what the numbers cannot |
| KI-14 | The lantern has no safe early demonstration before the scaffold of the west (docs/DESIGN.md 10, 12) | play the harbour | not built | the first required use of the lantern is the scaffold | a small demonstration in the harbour, with its own proof in the route test |
| KI-07 | Most logic tests build without `/WX` | `tests/run_tests.ps1` | only `core` and `camera` have `/WX` | a new warning in a test is reported as WARN, not as a failure | turn `/WX` on test by test once a run shows no warning |
| KI-08 | Hosted build status is not documented for this baseline | Compare this version with the hosted workflow run | - | A past successful run does not verify the current revision | Confirm the workflow on the intended release revision |
| KI-09 | The uploaded town has district puzzles and bells, but its final bell route is not specified as complete | Follow the main route in the default town | The documented route proof ends at the night bell | The new finale must not be inferred from the legacy ending | Prototype and verify the intended final approach; see [Design](docs/DESIGN.md#5-bell-tower-finale-and-ending-views) |
| KI-10 | In the town a fall comes back to the last place the body stood still on, which may be just before a jump that keeps failing | jump short of a gap and fall | no anchor lamps in the town yet | a player may need R | the lamps of the town with step 4 |

## Closed issues

| ID | Symptom | Closed | How |
|---|---|---|---|
| KI-11 | `RunTown` in `main.cpp` was one loop of about 390 lines | 2026-11-26 | `app/TownSession`: the loop in steps of about 20 lines |
| KI-12 | Code that only the removed greybox used was still in the renderer and the body (night theme, object table, height field) | 2026-11-26 | removed; the invisible floor it caused is closed with it |
| KI-06 | Comments in the code pointed to sections of `docs/LEVEL_DESIGN.md`, which `docs/DESIGN.md` replaced | 2026-11-26 | the greybox code that carried most of them was removed and the rest of the comments were rewritten |
| KI-03 | The Release build treats warnings as errors; the M1 and M2 files had not been compiled with MSVC | 2026-11-17 | `Release \| x64` was the configuration of the first real build; after the `min` / `max` fix it ran. The rule stays: a new file must build in Release (check Q-9, done by `tools/check.ps1`) |
| KI-02 | A lost device ended the program | 2026-11-17 | `Gpu::Recover` in `main.cpp` builds device, triangle and models again (checks K-2, K-3 passed). Only the `F9` test path was run; a real device loss (driver reset, GPU switched off) has not been tried |
| KI-05 | The GPU used by default on the dual-GPU laptop was not confirmed | 2026-11-17 | the code picks the high-performance adapter and exports the two driver hints; check K-1 passed (NVIDIA RTX 5060) |
| KI-04 | Reverse-Z was undecided: depth is the plain `D32_FLOAT`, LESS, clear 1.0 | 2026-11-18 | decided to **keep** it: the scene is small (floor 24 m, near 0.1 m, far 100 m), no flicker has been seen, and the two ripple depth buffers use the same convention. Revisit only if far objects flicker (it would change `Projection.h`, `Device::BeginFrame`, `Models::CreateStates`, the ripple depth buffers and the depth state of `Triangle`) |
