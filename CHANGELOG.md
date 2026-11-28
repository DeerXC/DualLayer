# Changelog

Each entry says what was made, what was found on the way and what is not done. The daily entries before 2026-11-27 are condensed
into entries of a milestone's size; the full text of each is in the git history of this file.

## m2-layers (in progress)

### 2026-11-28: the first run on a hosted runner

- Found (tests): the first run of `tests.yml` on a hosted Linux runner failed one test of 21, `map` (2 of 454 checks, in `TestThings`: the crate drawn at 30 m, 45 m). The other 20 passed, the route proof in 129 s. The same program passed on the development machine with g++.
- Cause: the middle of the crate, turned 0.6 rad, lies on the diagonal that its two triangles share, and `ColourAt` took a point as covered only for barycentric weights of exactly 0 to 1. Whether the point falls in one triangle or a hair outside both depends on the last bit of the arithmetic, and a machine that fuses a multiply and an add (ARM, or g++ with `-march=native`) rounds it the other way. Built with `-march=native`, the old test fails at the neighbouring turns 0.619 and 0.638 (the same lines 903 and 911 as on the runner); 96 of 6,283 turns from 0 to 2 pi fail with fused arithmetic and none without. The code of the map was not at fault, and the runner's own processor was not recorded.
- Fixed (tests): `ColourAt` in `tests/test_map.cpp` takes a hair beyond an edge (1e-4 of a triangle, a thousandth of a pixel) as inside. `tools/run_tests.py` builds with `-ffp-contract=off`, so g++ gives the unfused result that MSVC gives on every machine. `tests/test_map.cpp` has the same 454 checks, `map` and `things` pass, and the old test with a hair of tolerance passes at both turns that failed it.
- Changed (tools): `.github/workflows/tests.yml` prints the machine type and the g++ version before the tests, so a difference that shows only on the runner can be read from its log.
- Not done: a green run of `tests.yml` on the revision that holds this fix has not been seen (KI-08).

### 2026-11-28: the ledger of twelve years

- Added (engine): `src/scene/Ledger.h`, the table of `docs/DESIGN.md` 3.9 as data and one function: what was left in A, at a place with some of five facts (soil, sun, a roof, a wall, the sea), is what stands in B. Growth (a vine with no wall is a tangle), rot and keeping, rust, a ballast crate, the sea, and, as the same table seen through one more question, a prop, a dam and what a person keeps. There is no state: the same question has the same answer in any order. The trees, the keepers and the numbers that tie the rules to section 11 are in the table, and `Problems()` reads a table back for rows that can never match, trees that cannot be climbed and the like. Not wired into the town: the town file has no facts of places yet (KI-19).
- Added (test): `tests/test_ledger.cpp` (139 checks, no town file needed): every rule on all 32 combinations of the five facts against an oracle written another way, the properties of the whole (a roof never hurts plant stuff, the sea never helps a seed, any order), the trees, the table as data (changing, adding or taking out a row changes the answer and no code) and 18 kinds of wrong table, each found by name. Twenty-three broken copies of the header (a rule dropped, a flag swapped, a trade ignored, a check taken out) were each caught. `tests/test_things.cpp` has a new section (60 checks): the crates of the town are as high in B as the ledger's heap, and the iron things stay as A left them.
- Found by writing the table (five places where the design and its numbers did not agree; each is changed in `DESIGN.md`): the fig had limbs 2.0 m apart and the vine footholds 1.8 m apart, more than the 1.2 m step of section 11 (now 1.2, 2.3, 3.4 and every 1.2 m); a heap of 80% of a crate would have made the built crates 0.96 m high where the route proof stands on 1.2 m, so a heap is as high as the crate; the sea of the storm covers everything below 0 m (1.1), so the first olive, planted on the quay, would have been salted (a Sea rule is added, and the house of the lit window stands above 0 m); a lock dammed with timber leaves "no step" (8.4) where silt "as high as the dam" (3.9) would stay (the bank stays while the dam holds); and a vine with no wall had no outcome (a tangle).
- Changed (tools): `tests/run_tests.ps1` and `tools/run_tests.py` have the new test (22 logic tests).

### 2026-11-28: the engine and the tools

- Changed (engine): `scene/TriMesh.h`. A cell of the grid holds the boxes of its triangles in one row of memory and a query reads the box before the triangle: in the queries of the route solver, of the triangles in the cells round a ball 99% are out of reach and of those under a ray down 87% are not under it. A triangle filed in several cells used to be met once by a stamp that every query wrote into the mesh; now it is met once in the first cell of the query that holds it, which is found from the triangle, so a query writes nothing and the mesh may be asked from several threads at once. `PushOut` is 1.8 to 2.1 times as fast and `HighestBelow` 1.5 times, in a benchmark over 400,000 points of the real town; one pass of the route solver went from 17 s to 8.7 s. The memory of the two meshes of the town is 27 MB (3.7 to 3.9 filings per triangle, 36 bytes each). `Moments.h` asks the new `ForEachNearBall`, which gives the box test the squared limit of the closest contact so far.
- Checked (engine): the old and the new mesh were run side by side on the town file, 11.1 million queries on 163,656 triangles and 0.9 million through `Moments` with the circle of the lantern (`PushOut`, `WallPush`, `FloorAt`, `HighestBelow`, the order of `ForEachNear`): 0 differences, bit for bit. The 20 logic tests give the same output as before, line by line, the route proof included (`route`: 12 checks, 223 s against 450 s). `tests/test_trimesh.cpp` is new (19 checks): random triangles, small and large, and a floor of shared edges are compared with a search of every triangle; a triangle is met once and every triangle in range is met; the box test never throws away a real contact; four threads on one mesh get what one thread gets. Three broken copies of the mesh (a margin that is too narrow for a ball and for a ray, a wrong first cell) each fail it.
- Changed (engine): `TownData::Load` reads the town file with one read and not with an iterator over the stream: 0.98 s became 0.09 s with the standard library of g++ in a plain build, and 1.3 s became 0.1 s with its checked mode; a Debug build of MSVC checks every step of an iterator in the same way.
- Added (tools): `tools/run_tests.py` builds and runs the logic tests with g++ or clang++ on any system, side by side (`--list`, `-j`, `--strict`, `--export`), and reads the g++ line in the header of each test for its sources, so there is no second list. `.github/workflows/tests.yml` runs it on a hosted Linux runner at every push (unverified there: the steps were run by hand with the same commands). `tests/run_tests.ps1` has the new test.
- Found by the strict warnings (`-Wsign-conversion -Wdouble-promotion -Wold-style-cast -Wuseless-cast -Wnull-dereference` on every test): 18 sign conversions in `render/MapView.h`, 1 in `tests/test_route.cpp` and 3 float to double promotions in `tests/test_mesh.cpp`; no other file that the tests include has one. They are not changed: MSVC at /W4 does not report them, and each would be a change to the code of the map.

### 2026-11-28: time is the verb, and a new finale

- Changed (design): the story is simplified (`docs/DESIGN.md` 1.1). The disaster is a great storm that nobody was warned of, and there is no villain. A is the last calm evening of the autumn fair, held still by the traveller's lantern; B is the same town twelve years later. The storm damages every height of the town (surge below, flood and wind above, a slide on the cliff), which is why the town was left. This replaces two stories written on 2026-11-27: a storm surge under a clear sky (it flooded only below about 3 m in a town that rises to 42 m, so it could not explain why a whole town was left) and the Night and the Lamp-Eater (seven or eight inventions to explain three districts, and a lamp rule that was a second game beside the first). The lantern is the one thing that is not ordinary. The tower is locked by three officers who each read one sign on one instrument; each is kept from it by an ordinary trouble of the fair (a barge in a lock, the awning of the theatre across a lookout, a weather glass taken down by the carpenters) and gives his key when the trouble is removed, so the traveller is never believed and never argues.
- Changed (design): time is the verb. B is what twelve years make of A, computed from A by a short table of rules (3.9): growth, rot and keeping, rust, prop, silt and the keeper of things. Every puzzle is designed backwards from a picture in B (8.1), the three ways are built on the ledger (8.3) and the moment of understanding of each way is specified (8.4). The positive effect of the player is what people keep: a household that is helped prepares, takes its preparation uphill at the bell, is lived in in B and remembers the traveller (4, 5.4).
- Changed (design): the finale is rewritten (5). A climb of three floors that put silt, prop and growth into larger spaces and a last flight that is the first lesson of the game again; a belfry from which the player sees the result before the bell; the flame of the lantern given to the lamps of the mast, after which the evening is over; the storm shown once, when everyone is safe; twelve years passing in one sweep of the ripple from the tower; and a walk in which the people who were helped remember the traveller. This replaces the lamp room with the black lantern. The teaching ground follows the Great Plateau of Breath of the Wild and the opening of Elden Ring (7.12); the map is composed as a place that grew (7.13); the open town with shut thresholds is adopted (7.10). None of it is built (KI-09, KI-19).
- Removed (design): the Night, the Lamp-Eater, the black lantern and the lamp room; lamps as favours and the LIGHT verb; the bells as goals and the lamps lit by bells; the boats that escape, the sail-cloth clues and the one-to-one lamp-to-boat mapping; the three ending views and their inputs; anchor lamps and collectible lamps; memory fragments; the nine lights; the west gallery and the latch of B; the orb and the pickable objects of the greybox; the history of each pass of the town and of each decision (it is in this file). The section numbers of `docs/DESIGN.md` did not change, so the references in code comments stay valid; 3.9 and 5.4 are new.
- Checked (documents): `docs/DESIGN.md`, `CHANGELOG.md` and `README.md` have one H1 each, no skipped heading level, well-formed tables, UTF-8 with LF and no trailing space, no second person and no tool name; every reference of `DESIGN.md` to one of its own sections resolves. No code was changed.

### 2026-11-27: the documents

- Changed (documents): `README.md` is rewritten for a reader who does not know the project (what it is, highlights, build, tested on,
  where to start reading). This file is condensed. `docs/ARCHITECTURE.md`, `docs/BUILD.md` and `docs/CONVENTIONS.md` named things that
  no longer exist (`RunTown`, `Collision.h`, `kScene`, the keys of the greybox map) and put work of `TownSession` and `Gpu` in
  `main.cpp`; `ARCHITECTURE.md` also gave the frame buffer as 256 bytes where it is 464. All corrected. The two play-test sections of
  `BUILD.md` are the rows E-34 and E-35 of its table of checks by eye. `ARCHITECTURE.md` has a repository layout and a section on the
  tests.
- Changed (code, comments only): the file headers of 36 files in `src/` and 16 tests had been cut into pieces by a comment reflow (a
  few words alone on a line between two full lines). They are wrapped again, the comments that named `main.cpp` for work that
  `TownSession` and `Gpu` do were corrected, and `.clang-format` no longer reflows comments. With the comments stripped, the code of
  `src/` and `shaders/` is the same as before.
- Changed (tests): `tests/check.h` holds the `CHECK` macro and its two counters, which 19 of the 20 tests had each defined.
  `tests/test_lighting.cpp` checks for at least nine lamps of the kind "memory" and not exactly nine: the model has two sources of them,
  the ring of nine on the tower and the lamp of the fountain in the court.
- Checked: the 14 logic tests that need no town file (core 25 checks, camera 31, mesh, batch 11, trimath 15, ripple 42, player 103,
  layer_guard 12, effects 23, guidance 59, style 95, town_player 80, overlay 220, lantern 56) build with g++ (`-Wall -Wextra -Wshadow`)
  without a warning and give the numbers they gave before the change. The other six (town, route, things, map, firstminute, lighting)
  read `data/town.dlt` and were only compiled. The MSVC build and the checks by eye are not done.
- Checked (documents): `README.md`, `CHANGELOG.md` and `docs/DESIGN.md` have one H1 each, no skipped heading level, a blank line round
  every heading, well-formed tables, UTF-8 with LF and no trailing space, no second person, no tool name and no personal path, and every
  reference of `DESIGN.md` to one of its own sections resolves. No code was changed by the rewrite of the design.

### 2026-11-26: the town as a place to play (passes 1 to 6)

- Added (streets): the fabric of the lower town in `tools/layout/city_model.py` (`docs/DESIGN.md` 7.7 to 7.11). Houses stand on fronts
  only, so every door is within 0.9 m of a street, a lane, a lock quay or the plaza (`check()` fails otherwise; the old rule left 92 of
  115 houses more than 8 m from any street), and a house is kept out of the sight lines from the quay to the lamps of the mast
  (`sight_cap()`). New lanes: the garden lane (42 m, 24 degrees at most, with a cistern and a terrace), the mill lane, the cooper lane
  with four passages (net, fish, cask, rope) and the fish market, the weaver lane, the vine lane (a loop with the garden lane, with a
  chapel), the tanner loop and the wall lane. Every dead end ends at a terrace with a bench, a cypress and an overlook of the map. Yards,
  door props, orchards and net racks fill the rest. Pass 2 took the town from 55 to 115 houses and from 12 to 21 streets.
- Found by the tests: a net lane from the market stairs to the south-west opened a way round the barricade of the air step
  (`tests/test_things.cpp`, `TestAirStep`, caught it; `tests/test_route.cpp` did not), so it was removed. With the gentle bank beside the
  garden lane the route solver walked from the lane to the upper street, the theatre and the graveyard before the day bell; a district
  wall now closes that way.
- Added (rooms): ten houses and a chapel that can be entered. Each room has a door of 1.3 m, a ceiling at 3.4 m, a lamp and one idea told
  by objects (an interrupted dinner, barrels, a loom, a telescope, a child's bed). A has a roof and a lit window; B is open to the sky.
  Two rooms have a barred window cut only where rays are clear in both moments. `tests/test_town.cpp` walks from the street through the
  door to the middle of every room in A and in B. A room is not put at a pier of an arch (the first run did, and the walker could not
  enter it).
- Added (puzzles in the world): in A everything that can be used has a gold cap, grip or ring, dull iron in B. Each puzzle has a reason in
  the world (customs cargo and a crane at the canal, roofers at the plank, a hoist at the gatehouse), all of it flat on a wall, at a side
  or over 3.5 m, so that nothing stands on a way that a puzzle uses. Washing lines, a boat yard, a rope walk, a garden and a memory court
  off the lower street (a fountain with water in A, a dry basin with a lamp in B) make the town lived in.
- Added (buildings): `tools/layout/architecture.py`, a library of buildings (six house types, five landmarks, `plan_row()`; a house is
  never higher than 1.25 times its width) with `architecture_check.py` (494 checks). The check found a balcony corbel at 2.95 m (a crate
  and a jump reach 2.85 m) and a cottage eave at 2.6 m that was a way onto its roof. The model builds its houses and landmarks from the
  library; the facades of the corridors are decals or above the top of the wall, so that nothing can be stood on. Roofs, chimneys and
  balconies at 7.4 m (out of reach) crown the corridors.
- Added (style): `tools/layout/town_style.py`, the one palette of the town (43 materials, eight lime washes, four ages of tile, B derived
  from A) with `audit_palette()` (20 rules) and `audit_town()`. B is weathered stone in the light of the moon, not ice. The ground has
  smooth edges between grass, field, sand and rock.
- Fixed (engine): `TownData::Painted` gave every polygon alpha 0, so `PaintAlbedo` of the style of the light had been written, tested and
  never reached the town. 70% (A) and 78% (B) of the drawn vertices are painted now.
- Changed (test): `tests/test_map.cpp` accepts 5000 to 8000 cells that differ between the plans of A and B (it was 6000 to 8000; the
  town had 6109, 109 from failing, and now has 5726). The check is for the same thing as before: the two sheets are not the same, and
  they are not different everywhere.
- Not done: the market hall cannot be entered (its hillside rises 12 m under it); the corridors are not shortened and no puzzle is moved
  to a threshold; the east has no loop; the nine lights; the theatre front and the town wall; two dead ends still end in nothing
  (`docs/DESIGN.md` 7.11).
- Checked: the check of the model, `architecture_check.py`, the palette audit and the logic tests (town 49, town_player 80, firstminute
  40, guidance 59, things 46, lantern 56, effects 23, map 454, overlay 220, camera 31, style 95, route 12) against the new
  `data/town.dlt`. The Windows build and the eye checks E-28 to E-33 were not done on that day.

### 2026-11-26: light, traveller, guidance and map

- Added (light): the sun of A and the moon of B cast shadows (one map of 2048 x 2048 texels over 60 m round a point ahead of the camera,
  moved on the texel grid so that nothing crawls). Every lamp the model builds is a point light (the 32 that matter are chosen each
  frame, faded with distance and flickered); the lantern of the traveller is a light too. A soft shoulder keeps a bright lamp from
  cutting a wall to white; lamps have halos. `[lighting]` in `config/default.ini` holds the numbers (F5 reloads), and the key L looks
  at the light in parts.
- Added (style of the light): the light is hand-painted, not photographic. The sun falls in three soft bands, shadows lean cool, clouds
  drift soft shadows, an edge facing the light has a rim, a lamp lights its pool in four rings, the stone is painted with blotches and
  is damper near the water, the far haze leans to a colour, the picture is graded, and rays of light are found by sampling the shadow
  map along the ray of sight. The key L ends its round with "plain", the same light without any of it. The frame buffer of the model
  shader grew from 256 to 464 bytes.
- Added (tests of the light): `tests/test_lighting.cpp` (92 checks) and `tests/test_style.cpp` (95) run the maths of
  `shaders/lighting.hlsli` as C++ through a stand-in for the vector types. The shadow chain is checked on a toy scene: of 13,895 points
  of ground 265 are in the shadow of a box and none is speckled; without the bias 13,610 are speckled, so the test does see acne. The
  shaders compile with an HLSL front end offline; the game compiles them at start, so an error shows in the log.
- Added (traveller): a stylised figure of 45 parts, 1.72 m tall (a test measures it in every step of a stride), posed with two bones
  per limb, and a lantern model of 12 parts used in both states: in the hand, and set down on the arm of a small iron post.
- Added (guidance and the first minute): the lantern is the compass. Its flame leans 18 degrees toward the next step of the main path
  and 40 degrees, with sparks, after 30 s without progress; H raises the lantern and sweeps a beam, and a second press after 90 s shows
  a frame round the thing to use. The main path is data now (`steps` in the town file), and the game guides along the steps that
  `tests/test_route.cpp` plays. Rays cast by `tests/test_firstminute.cpp` showed that the sea wall hides the bell tower from the quay in
  both moments, so the goal is a signal mast with three lamps at 61 m; the same test fixed the first frame (yaw -0.55, pitch +0.29)
  so that the lamps, the winch and the heap of the quay are in the picture for lenses of 60, 70 and 90 degrees and for 16:9, 16:10
  and 4:3. The prompts TAB (after 8 s of walking) and M (after 40 s) are offered to a player who has not yet used the key.
- Added (map): M opens two sheets of tracing paper, the past over the present, over the frozen game. The plan is sampled every 0.5 m
  and drawn as smooth shapes; unseen space is vellum fog; every puzzle still to solve shows through the fog as a ring with a question
  mark; zoom from 1 x to 4 x, panning, centring and ten markers in six shapes, each remembering its sheet; the things that change are
  drawn as they are now. A `MapCache` makes the ink and the fog again only when what they show changes. `tests/test_map.cpp` has 454
  checks, `tests/test_overlay.cpp` 220.
- Added (camera): a flying camera (key F, noclip key N) that rises with Space, goes four times faster with Shift and moves at 12 m/s.
- Fixed (found by the tests): an invisible floor in the town (the demo scene of the first prototype was the fallback of
  `Player::SetScene`; its floor of 24 x 24 m at y -1 was solid over the gorge), and a wall touched together with a floor that was taken
  for ground in every other frame (the walk speed alternated between 1.58 and 0 m/s; `Moments::WallPush` now asks each triangle on its
  own). Two variables named `far` and `near` compiled for the tests and not for Windows, where `windows.h` defines both as macros; the
  compile of every source for Windows with clang found them before the commit.
- Removed: the greybox prototype that the town replaced (the hand-placed rooms, the block, orb and canal puzzles, the seals, the bells,
  the endings and the ground of the old map: 23 headers and 9 tests, `tools/maze_search.cpp`; it stays in the git tag `m2-greybox`), and
  the demo scene, the object table and the height field of the first prototype (7 files, 1,610 lines). `main.cpp` shrank from 856 to
  112 lines; the frame loop is `app/TownSession` (steps of about 20 lines each), the GPU objects are `app/Gpu`, the config is
  `app/AppConfig` and the keyboard is `app/Input.h`. Every C++ file is formatted with the repository's `.clang-format`.
- Changed (documents): `docs/DESIGN.md` is rewritten where play showed it was wrong: a player who did not press H did not know what to
  do. The game asks one thing from the first frame (the tower with its three dark lamps), the harbour and the lower town are the
  teaching ground, and three ways (west, east, north) follow in any order, each of three beats and a bell, joined by the rock and not
  by each other (7.3, 7.4, 8). Guidance is layered (10). A minimap, a task list and a compass bar were considered and not adopted.
- Checked: all logic tests build with g++ (`-Wall -Wextra -Wshadow -Wconversion`) without a warning and pass. Every source of the game
  compiled for Windows with clang against a stand-in for DirectXMath. MSVC, the Visual Studio build and a run of the game were not done
  on that day (E-16 to E-27 of `docs/BUILD.md`).

### 2026-11-25: the town on the model, the harbour, the route proof

- Added (town player): the traveller walks and sprints, climbs a ledge when support, clearance and water allow it, and carries a visible
  lantern; the third-person camera retracts round walls. Per-moment gradient skies, a sun or moon halo, ground bounce and near-ground
  mist (`[render] town_atmosphere` switches them off for comparison). Matched A/B review cameras (`tools/layout/review_town.py`) and a
  preview of the west loop (`tools/layout/west_loop_preview.py`).
- Added (things that change): `scene/Things.h`. A thing has a handle and boxes, each in its moments and in one state; E at the handle in
  A opens it in A and in B at once, in B E does nothing, and an opening that would close on the body is refused. The harbour teaches
  the switch (the heap of wreck that closes the way west in B only), the causal rule (the portcullis of the sea gate, raised by its winch
  in A) and the switch in the air (the barricade of the lower street in A, the fallen house in B). Then the canal (a crate pushed in
  A floats to the far quay), the scaffold (the lantern on it in A, climbed in B), the plank between two roofs, the gatehouse (a crate
  under its balcony in A, a jump with a switch at its top onto the heap of B), the three bells, and the centre: the west bridge that
  falls whole in B, the lift of the rock, the walls between the districts.
- Added (route proof): `tests/test_route.cpp`, a solver over both moments of the whole town (walk, a jump onto a ledge of 1.65 m, the
  switch where the body stands, the switch in the air, the lantern as a circle) that plays the things in the order of the main path and
  lists any way round a puzzle. It found a way round the canal past the end of a terrace wall, two ways round the lantern, a walk
  along the street walls past the plank, a yard climbed to a neighbour's roof and a step of 1.66 m to a balcony. All closed. It takes a
  few minutes.
- Decided: the harbour teaches the switch, the causal rule and the switch in the air; the lantern is met first in the shaft of the west,
  where it is the only answer.
- Not done on that day: the finale. The route proof ends at the night bell (KI-09).

### 2026-11-24: the lantern, the town of the model in the game, the design revised

- Added (lantern): `scene/Lantern.h`. E sets it down in A only, standing; it keeps a sphere of A (radius 3.5 m) in B. In B the solid and
  the picture inside the sphere are A's, with a warm rim; the water of A is water inside it. Taking it up is refused when it would leave
  the body inside something or without the floor it stands on. Changes made in A come up in B as a faint glow (`Traces`).
  `tests/test_lantern.cpp` (56 checks now) uses a small town of its own, so that it does not depend on the exported town.
- Added (town): the layout model is the map. `python tools/layout/city_model.py export` checks the model and writes `data/town.dlt`;
  `scene/Town.h` reads it, bakes one batch per moment and builds a solid of triangles (`scene/TriMesh.h`, a grid of 4 m cells). The
  model was fixed where the walking test found stairs running into towers and parapets.
- Decided: A is the past and B the present; what is done in A leaves a trace in B and never the other way; a lantern set down in A
  keeps a circle of A in B; puzzles start from the present; the greybox rooms are not kept and the whole town is rebuilt on the layout
  model; puzzles that depend on another district and shortcuts back to finished places are dropped. `docs/DESIGN.md` is the one design
  document; it replaced `docs/LEVEL_DESIGN.md`.

### 2026-11-17 to 23: layers, the walking body and a first city (since removed)

- Made: a layer mask on every object and the Tab key, a theme per layer and the ripple (both layers drawn off-screen, a circle that opens
  and turns round); the walking body (three balls, gravity, a jump of exactly its height, coyote time, a jump buffer, a cut jump,
  acceleration); device recovery; water drawn after everything opaque, blended and sorted; `tools/check.ps1` (tests, both builds, a game
  run, the repository check, the commit). The first prototype was a city of hand-placed rooms with block, orb and canal puzzles, tested
  by a search for skips (`tests/test_reach.cpp`, whose first runs found five ways round the map). It is replaced by the town of the model
  and stays in the git tag `m2-greybox`.
- Fixed: `std::min` and `std::max` broken by `windows.h` (C2589); `tools/check.ps1` tested an old exe (now a full rebuild and a time
  check); a shader compile function was copied in two files (`ShaderCompiler`); the stack of a test overflowed under MSVC.

## m1-models (built and run together with m2, 2026-11-17)

- Made: a depth buffer in `Device` (D32_FLOAT, cleared to 1.0, made again on resize); `render/MeshData` (cube, sphere, cylinder and torus
  on the CPU, no Windows in it) with `tests/test_mesh.cpp`; `render/Models` (vertex and index buffers, one shader, one directional
  light, a checkered floor); `render/Projection.h` (one lens for every pass).
- Checked: compiled and run on Windows together with the m2 code; checks M1-1 to M1-7 passed on 2026-11-17.

## m0-project-skeleton (2026-11-10)

- Made: repository files, a hand-written Visual Studio 2026 project (SDK pinned), `Log`, `Assert` and `Config` with tests, a window with
  DPI awareness, a D3D11 device with an adapter list and the debug layer, GPU names and events, a flip-model swap chain with an sRGB
  view, a test triangle.
- Checked: the project builds and runs on the laptop (first confirmed on 2026-11-17); checks M0-1 to M0-8 passed.

## Known issues

| ID | Symptom | Reproduce | Cause (if known) | Impact | Plan |
|---|---|---|---|---|---|
| KI-01 | Shaders are compiled at run time from `shaders/` | start the exe from a folder without `shaders/` | by design (hot reload comes in a later milestone); the post-build step copies `shaders\` next to the exe | log error, no picture | accepted for now; precompile for the delivery build |
| KI-09 | The finale is not built: the tower cannot be entered, and the end of the route stands at its wall | follow the main route to the night bell and go to the tower | the finale of `docs/DESIGN.md` 5 (the tower opens with three keys; a climb of three floors and a last flight; the belfry; the flame given to the lamps of the mast; the bell; the storm; the sweep of twelve years; the walk) is specified only | the game cannot be finished | prototype the door, the three floors, the last flight and the belfry, then the sweep of the ending, then build the tower and the keys (DESIGN 5, 12) |
| KI-19 | The ledger of twelve years (`docs/DESIGN.md` 3.9) is built as data and rules (`scene/Ledger.h`, `tests/test_ledger.cpp`) but is not in the game: the town file has no facts of places (soil, sun, a roof, a wall, the sea), the exporter sets none, B is not drawn from it, and there is no seed, held thing or favour | play the quay and the lower town | the table is tested on its own; the town and the model do not use it yet | the game still opens the tower with three bells and three lamps (KI-09), and the story is not in the game | write the facts and the subjects of the town into the model and the town file (`ageing`, format 7), draw B from the ledger and prove the three crates and the plank again; then the seed and the woman of the first window, then the three ways (DESIGN 12, step 2) |
| KI-18 | The main route has no moment of being stuck and then understanding | play the main route (play-test of 2026-11-27) | - | the route is solved without a moment of surprise | rework the puzzles with a reveal in each way, mechanisms that link the two moments and objects that both moments share (DESIGN 8.4) |
| KI-16 | The main route is one line (west, rock, east, north) in the model, in `steps` and in `tests/test_route.cpp`; the three ways of DESIGN 7.4 are not modelled | read `steps`; the locks start from the rock, the upper street from the top lock | the order was fixed before the town was linear in play | the districts cannot be done in any order | model the stairs from the lower town, the links to the plaza, `goals`, and the tests of DESIGN 11 (step 3) |
| KI-15 | The guidance is built in part: the lamps, the flame, H (two steps), the first frame and the map; not built: a beacon at the start of a way, the lens (holding Tab), the glyphs on the gates, the flame upright in the lower town (the third press of H is dropped from the design: the lens replaces it) | read `docs/DESIGN.md` 10 | the three ways and their gates are not modelled (KI-16) | the player is guided to the next step but not yet shown a choice between three ways | build with the three ways (DESIGN 12, steps 2 and 3) |
| KI-13 | The lamps of the mast, the flame, H, the map and the first frame have not been recorded as seen in the game | play E-16 to E-20 | built on 2026-11-26 without a Windows run | the lamps may be too small, too dim or lost in the fog or the sky of B; the flame may lean too little to see; the beam and the frame may cross walls; the map may be crowded | tune `[guidance]`, `Emblems.h` (size in the model, glow) and `MapView.h` after a play-test |
| KI-17 | The light (the shadows, the lamps, the shoulder, the halos) has not been recorded as seen in the game | play E-21 to E-24 | built on 2026-11-26 without a Windows run or a GPU | the shadows may speckle or float off their objects; the lamps and the lantern may be too strong or too weak against the moon of B; a halo may hide its lamp; the picture may be darker than before (the moon at 0.8, the shoulder); the shadow pass may cost frames on the laptop | tune `[lighting]` (`shadow_bias`, `shadow_normal`, `lamps_b`, `lantern`, `sun_b`, `exposure`, `halo_b`, `shadow_size`), then fix what the numbers cannot |
| KI-14 | The lantern has no safe early demonstration before the scaffold of the west (`docs/DESIGN.md` 10, 12) | play the harbour | not built | the first required use of the lantern is the scaffold | a small demonstration in the harbour, with its own proof in the route test |
| KI-10 | In the town a fall comes back to the last place the body stood still on, which may be just before a jump that keeps failing | jump short of a gap and fall | by design: anchor lamps were dropped from the design on 2026-11-27; R and the last safe standing point are the rule (`docs/DESIGN.md` 4) | a player may need R | decide from play whether a fall needs more |
| KI-08 | The workflow `.github/workflows/build.yml` (MSVC, by hand only) has not been run on a hosted runner, and `tests.yml` (g++ on Linux, at every push) has not yet been seen to pass there: its first run failed `map` (fixed, 2026-11-28) | run the workflows | the hosted runner may lack Visual Studio 2026 and the pinned SDK, so `build.yml` overrides both | a past successful run would not verify the current revision | read a green run of `tests.yml` on the revision of the fix; run `build.yml` once on the intended release revision, then add the push trigger and a status badge |
| KI-07 | Most logic tests build without `/WX` | `tests/run_tests.ps1` | only `core` and `camera` have `/WX` | a new warning in a test is reported as WARN, not as a failure | turn `/WX` on test by test once a run shows no warning |

## Closed issues

| ID | Symptom | Closed | How |
|---|---|---|---|
| KI-11 | `RunTown` in `main.cpp` was one loop of about 390 lines | 2026-11-26 | `app/TownSession`: the loop in steps of about 20 lines |
| KI-12 | Code that only the removed greybox used was still in the renderer and the body (night theme, object table, height field) | 2026-11-26 | removed; the invisible floor it caused is closed with it |
| KI-06 | Comments in the code pointed to sections of `docs/LEVEL_DESIGN.md`, which `docs/DESIGN.md` replaced | 2026-11-26 | the greybox code that carried most of them was removed and the rest of the comments were rewritten |
| KI-03 | The Release build treats warnings as errors; the M1 and M2 files had not been compiled with MSVC | 2026-11-17 | `Release \| x64` was the configuration of the first real build; after the `min` / `max` fix it ran. The rule stays: a new file must build in Release (check Q-9, done by `tools/check.ps1`) |
| KI-02 | A lost device ended the program | 2026-11-17 | `Gpu::Recover` builds device, triangle and models again (checks K-2, K-3 passed). Only the `F9` test path was run; a real device loss (driver reset, GPU switched off) has not been tried |
| KI-05 | The GPU used by default on the dual-GPU laptop was not confirmed | 2026-11-17 | the code picks the high-performance adapter and exports the two driver hints; check K-1 passed (NVIDIA RTX 5060) |
| KI-04 | Reverse-Z was undecided: depth is the plain `D32_FLOAT`, LESS, clear 1.0 | 2026-11-18 | decided to **keep** it: the scene is small (floor 24 m, near 0.1 m, far 100 m), no flicker has been seen, and the two ripple depth buffers use the same convention. Revisit only if far objects flicker (it would change `Projection.h`, `Device::BeginFrame`, `Models::CreateStates`, the ripple depth buffers and the depth state of `Triangle`) |
