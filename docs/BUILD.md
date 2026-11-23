# Build

## Requirements

1. Visual Studio 2026 with the workload **Desktop development with C++**, toolset v145.
2. **Windows SDK 10.0.26100.0** (the project pins this version; install it rather than retargeting).
3. For the GPU debug layer of the Debug build: Windows **Settings > System > Optional features > Graphics Tools**. Without it
   the game still runs and the log says that the debug layer is not available.

## Build and run

1. Open `DualLayer.slnx` (or `DualLayer.vcxproj`), choose `Debug | x64`, press **F5**. The working directory of the debugger
   is the project folder, so `shaders/` and `config/` are found.
2. `build\x64\<Config>\DualLayer.exe` also runs from Explorer: a post-build step copies `shaders\` and `config\` next to it.
3. The logic tests alone: `.\tests\run_tests.ps1` in a Developer PowerShell for VS; one test: `.\tests\run_tests.ps1 -Only stage`.
   The longest are `reach` and `terrain` (about a minute each with /O2).

## Keys

| Key | Action |
|---|---|
| Click | capture the mouse for mouse look |
| Esc | give the mouse back; a second Esc quits |
| W A S D | walk (flying: move) |
| Mouse, arrow keys | turn |
| Space | jump; held = the full jump, a tap = a small hop |
| Tab | switch the layer (A red, B blue); a switch into something is refused and the ripple turns back |
| E | (walking) take the orb in reach, or put the held one down one step ahead; near a socket, a plate or a hole it is placed on it. The prompt at the bottom of the screen says which (TAKE, PUT DOWN, PLACE) |
| R | back to the entry of the current room; it also puts back the block (courtyard) or the orbs (west wing) of the room |
| F | walking / flying |
| Q / E | down / up (flying) |
| N | collision on / off (off also means flying) |
| Z / X / C | wireframe / level of detail / turning shapes on or off |
| F5 | reload `config/default.ini` |
| F9 | rebuild every GPU object (only with `[debug] device_reset_key = 1`): the test of the device recovery |

Command line: `--adapter N` chooses the N-th adapter listed in `logs/dual.log`.

The overlay shows a dot in the middle of the view, the key prompt of the moment at the bottom, and the room, the seals and
the hidden lights at the top left. The title bar shows the adapter, the frame time, the camera, the layer, the movement mode, the room, the time, the seals and
the hidden lights; `CLEAR` and the time at the goal.

## Configuration

`config/default.ini` is read at start and on F5. Every key is optional; the code has a default for each, and values that make
no sense are pulled into a working range.

| Section | Keys (default) | Meaning |
|---|---|---|
| `[window]` | `width` (1600), `height` (900) | the client size in physical pixels |
| `[render]` | `vsync` (true), `adapter` (-1) | -1: the first hardware GPU in high-performance order |
| `[log]` | `level` (info) | trace, info, warn or error |
| `[camera]` | `fov_deg` (70) | vertical field of view, 40 .. 100 degrees |
| | `land_dip` (0.012), `land_dip_max` (0.15) | metres the eye dips per m/s of landing speed, and at most; 0 turns it off |
| | `move_speed` (3), `turn_speed` (1.5) | flying speed (m/s) and arrow-key turn speed (rad/s) |
| `[mouse]` | `sensitivity` (0.0022), `invert_y` (false) | radians per count of the mouse |
| `[player]` | `walk` (true) | start walking (false: flying) |
| | `walk_speed` (5.5) | m/s |
| | `ground_accel` (95), `ground_decel` (120), `air_accel` (40) | m/s^2 toward the walking speed, to a stop, and in the air |
| | `gravity` (30), `fall_gravity_scale` (1.6) | m/s^2 while rising; times this while falling |
| | `jump_height` (2.0), `jump_cut` (0.45) | metres with Space held; the part of the rise kept when it is let go early |
| | `eye_height` (1.55) | metres from the feet to the eye |
| `[collision]` | `enabled` (true), `radius` (0.35) | the body is solid; the radius of its three balls |
| `[ripple]` | `seconds` (0.9), `width` (0.07), `distortion` (0.012), `center_x` / `center_y` (0.5) | the layer switch; `seconds = 0` is a cut |
| `[layer_a]`, `[layer_b]` | `clear_r/g/b`, `light_r/g/b`, `light_x/y/z`, `sky_r/g/b` | the background (also the colour of the fog), the light and the sky of each layer (linear colour) |
| | `fog_density` (A 0.006, B 0.022), `tint_r/g/b` (A 1.25 1.0 0.74, B 0.62 0.70 0.78), `weathering` (A 0.15, B 0.85) | the fog per metre; the stone of the city; how uneven its blocks are and how much moss grows on what looks up |
| `[hints]` | `ghosts` (true), `prompts` (true) | the outlines of the other layer; the key prompts at the bottom of the screen |
| `[models]` | `segments` (32) | how round the round shapes are |
| `[debug]` | `device_reset_key` (false) | F9 tests the device recovery |
| `[test_triangle]` | `show` (false), `spin_deg_per_sec` (0) | the first test triangle |

## One command

```powershell
.\tools\check.ps1
```

It starts from any PowerShell, finds the project and the Visual Studio build tools by itself, and runs in this order:

1. the logic tests (`tests\run_tests.ps1`), then a build-only pass over `tools\maze_search.cpp`; a compiler warning in a test
   is reported as WARN;
2. a full rebuild of `Debug | x64` and `Release | x64` (Release treats warnings as errors), then a check that the exe was
   written by this build and is newer than every source file. An unzipped file can carry an old time stamp, and an
   incremental build would then keep the old exe. `-Fast` builds incrementally; the time check still runs;
3. the game, with a fresh `logs\dual.log`: it starts on the quay of the harbour, walks along it, jumps with Space held, switches walking and
   flying, switches layers (also during a resize), turns collision off and on, closes, and reads the title bar and the log
   (no error, the ripple pictures were made, a normal end, no fall, the jump height). It presses keys for about 35 seconds:
   the keyboard and the mouse must stay untouched. No key is left down, even when the script is stopped;
4. the repository: nothing that must not be uploaded (build output, logs, private files, `config/local.ini`, one-time
   scripts `tools\apply_*` / `fix_*` / `docs_*` / `add_*`, files over 50 MB), and whether the hooks are on.

The table at the end says `OK`, `WARN`, `SKIP` or `FAIL` per step; the exit code is 0 unless something failed. Switches:
`-Fast`, `-SkipBuild`, `-SkipSmoke`.

To check and, only when everything passed, commit and push:

```powershell
.\tools\check.ps1 -Commit "feat(stage): add the west wing" -Push
```

The message must be `type(scope): subject`, at most 72 characters, without a final period or a double quote. `git add -A`,
`git audit` (at most 90 s), `git commit` (120 s) and `git push` (180 s) run in turn and stop at the first failure; a command
that does not answer in its time is stopped and reported. `--no-verify` is never used.

## Checked by eye

What `check.ps1` cannot see. Run in Debug after a change that touches it; write a result as "E-3 ok" or "E-3 fails: what was
seen".

| No. | Do | Expected |
|---|---|---|
| E-1 | start, look north; Tab | on the quay of the harbour: the stair between two walls up to the guardhouse, the town wall, the bell tower over the town (whole in A, a stump in B); boats and ships in A, a wreck in B; no D3D11 / DXGI message in the output window |
| E-2 | Tab, then Tab again during the ripple | a ring opens from the centre and the other layer shows inside it; the second Tab turns it round without a jump |
| E-3 | at the gap: onto the beam of B; jump, Tab in the air | the body lands on the planks of A; at their end the courtyard is seen from above |
| E-4 | in the courtyard, in A, push the block west | refused; the outline of the blue wall flashes on the floor in both layers; no blue box appears in A |
| E-5 | push the block onto the plate; walk through the north door in B | the sunken square: the causeway straight ahead to the portal of the cathedral, its three dark seal lamps, the rose window over it, the spire; in A the pool is water, in B it is dry |
| E-6 | walk up and down the ramps of the wings and the tower, turning with the mouse | the picture does not shake; the walk does not stop at the foot of a ramp |
| E-7 | west wing: W1 the orb to the socket (in A it is put down beside it, red-gold, and TAB SWITCH WORLD is offered; switched with it to B, it turns blue and is placed: the bridge); W2 the other orb onto the plate in A, through the door in B, the lever, back for the orb; W3 the orb over the hole (in A it lies on the lid, in B it falls and the bars open); the seal; up the shortcut | each step does what it says; the held orb is seen in front of the view; an orb left in one layer leaves a small mark in the other; one lamp lights and the shortcut opens into the hub |
| E-8 | east wing up to the courtyard wall top, the seal, off its north end | the body drops onto the gallery, which looks over the square; off its north end onto the east rim; the gallery cannot be jumped onto from the rim |
| E-9 | north wing up to the seal, along the ledge through the window, off it; on the ledge jump toward the cathedral | the body drops onto the west rim of the square; nothing higher is reached from the ledge |
| E-10 | with three seals, along the causeway, up the six ramps of the cathedral | the balcony looks back over the map; `CLEAR` with the time |
| E-11 | fall off a ramp; press R in the courtyard | the body comes back at the entry of the room; R puts the block back |
| E-12 | close the window | the live-object report lists only the device |
| E-13 | in the square, look round | the archive (west), the water ring (east) and the bell tower (north) stand over the walls |
| E-14 | start; walk to the first orb of the west wing; take it; walk to the trench | TAB SWITCH WORLD at the bottom until Tab was used three times; a dot in the middle; the status line at the top left; E TAKE near the orb, E PUT DOWN while holding it, E PLACE in front of the socket; a blue column of light over the socket seen from across the hall; the outline of the bridge over the trench in both layers |
| E-15 | in the west wing: the ramp down, the trench in A and in B, the west wall of the square; the title bar | steps on the ramp down (walked smoothly); water in the trench in A, gone in B; falling into it in A brings the body back at the entry of the hall; an orb put down over it floats; an arcade along the west wall of the hub; roofs over the wing corridors; the title bar shows a few dozen draws, not hundreds |
| E-16 | B: west hall north wall, the passage, the north corridor; A: landing 1 of the north hall, the bridge, the lookout, the stairs; the lift by the north landing before and after the north seal | B: a breach with rubble in each end of the passage (in A a red panel); A: a walled bridge east to a lookout over the city and stairs into the east hall (in B its middle is gone); the lift stays down before the seal and afterwards carries the body up to the seal landing |
| E-17 | the square: walk off the rim into the pool in A; in B down the south-east stairs, round the cathedral, up the north-west stairs; behind the cathedral in B; jump out of the pool | A: the body comes back at the entry of the square; B: the stairs are walked without a hop, the pool is walked round between the cathedral, its buttresses and the stairs; a hidden light lies on the floor behind the cathedral (in A it is under the water and cannot be taken); a jump climbs out of the pool anywhere |
| E-18 | the silhouette: from the terrace, the square, the gallery, the sky bridge and the balcony, look round in A and in B; the title bar | battlements along the tops of the city walls, none on a walkway or under the ledge; a city of roofs and towers all round, nothing floating, no house close enough to jump at; in B most of it is lost in the fog; the spire is seen from every district; the draw count in the title bar is as before (the city is baked) |
| E-19 | the prologue: walk from the guardhouse; past the lamp; the gate in A and in B; the heap in B; Tab; the ditch: onto the beam in B, jump, Tab; then fall into the ditch on purpose | the lamp lights as the body passes; in B the heap closes the way and in A it is gone; the switch in the air lands on the planks; from the ditch floor the stairs lead back up; nothing else climbs out |
| E-20 | fall off the terrace after the first lamp; press R; fall off a room without a lamp | the body comes back beside the lamp with a ring of light; in a room without a lamp, on the carved tile of its entry; the lamp looks the same in A and in B |
| E-21 | the east hall: try to jump the canal in A and in B; turn the wheel in B, then in A; the crate; the quay; R | nothing crosses; the wheel turns only in A, the water rises and the crate with it; from the crate to the quay and up the ramp; R puts the water down again |
| E-22 | in B, down the stairs at the east end of the canal, along the bed | the bed is dry, the crate lies on it, a hidden light waits in the west corner; the stairs lead back up |
| E-24 | carry an orb to the east hall and put it down over the canal in A, then in B | in A it floats out of reach and goes back to its start with a ring of light; in B it lies on the dry bed |
| E-25 | the east rim of the square, at the bars under the gallery | through the bars: a passage, a hidden light, a column of light over a lever; the bars hold in A and in B |
| E-26 | E2: lock, crate, quay; the weir at the west end of the quay; in B into the canal east of the parapet, onto the crate, into the breach; the lever; out onto the rim | the weir turns only in A and the crate drifts west; in B the crate lies under the breach; the bars open and stay open; R in the hall puts the crate back |
| E-27 | the north hall, landing 2: walk west in A, in B; the crystal; then west in A | A: the portcullis stops the walk on the bridge; B: no bridge; on the crystal the portcullis moves into B (its outline shows in A); A walks to the seal |
| E-28 | reach the goal | after the rings, a panel: CLEAR, the time, the hidden lights, the falls |
| E-29 | the canal bed in B: into the mouth under the quay, along the cellar, down to the grate; then the lower room of the west wing, the lever, back through the cellar | the grate holds and the lower room is seen through it; the lever opens it for good; a hidden light under the courtyard; in A both ends are closed by iron panels and the cellar is under water |
| E-30 | the north hall: the heap by the west wall in A and in B; the second crystal on the floor; then B | before: in A a wooden step and no heap, in B a caged heap; after: the cage is in A and the step in B; B climbs to a light in the wall |
| E-31 | reach the goal and look down from the balcony | the master sluice turns gold; the aqueduct and the waterfall stop; the pool, the trench and the canal drain in about 4 s; the crate settles on the bed |
| E-32 | the east hall before the wheels are turned | a gold column of light over each wheel; R on the quay or the bed puts the canal back |
| E-23 | anywhere: boxes of one world | wood or iron in A, fallen masonry in B, never plain red or blue; a thin red (A) or blue (B) edge at their base in their own world and an outline in the other; the aqueduct over the east of the square, its waterfall into the pool in A, fallen in B |
| E-33 | the title bar after the start; the log | the start takes no longer than before (the ground and the plants are baked once); `static batch A` and `B` lines in the log count tens of thousands of triangles; the frame time is about as before |
| E-34 | from the terrace look round in A and in B; walk to the side of the terrace and look down | the town on level ground all round, hills east and west, the mountain north, the bay and the sea south; under the terrace a ravine of rock with the sea in it, the terrace on its footing; no edge of the world and no void below the horizon; in B most of it in the fog |
| E-35 | fall off the side of the terrace, off the east path on the courtyard wall, off the middle of the sky bridge in B | each time a fall: the body comes back at its lamp or entry, never stands on the ground outside |
| E-36 | look over the town from the balcony of the cathedral | houses turned round the cathedral, ochre in the west, white in the east, grey in the north; gardens with lemon trees and cypresses; in B a quarter of the houses are roofless shells, merlons and pieces of the cornices missing; plinths and cornices on the walls; arched heads in the three openings of the square |
| E-37 | in B, stand in the dry cellar and press Tab; on the floor of the dry pool and press Tab; on the causeway press Tab | refused in the cellar and in the pool (the ripple opens a little and turns back); allowed on the causeway |
| E-38 | take the east seal, then watch the pool and the trench in A | the water rises a little over about 3 s; the rim and the causeway stay dry |
| E-39 | take the west seal, then come to the canal without touching the wheel | the lock has opened by itself and the crate stands high; R puts it down and it rises again by itself |
| E-40 | take the north seal, then stay in A for about 6 s | evening turns to night in A (a dark blue sky, a cool light); B does not change |
| E-41 | take the light on the floor of the pool (B), then look at the south-west corner of the pool in B and in A | in B pale glowing steps lead up out of the pool there; in A there is nothing; the light inside the gate brings a pale bench beside the breach (B) |
| E-42 | the courtyard from the overlook, then inside it, in A and in B; push the crate | flagstones with joints; a blind arcade on three walls; in A stacks of crates as walls, in B fallen masonry with a broken top, moss in a few joints and a broken string course; the crate (wood, three iron bands) slides from flagstone to flagstone; the plate is brass and brightens when pressed; nothing of the arcade is in the way |
| E-43 | walk off the quay into the sea; climb the stair and jump sideways at its middle and at the landing | the sea is a fall back to the lantern of the quay; a sideways jump on the stair stays between its walls |
| E-44 | from the balcony of the cathedral and from the terrace, look west and round the town | the bell tower west of the west wing (64 m, a ring of lamps, a belfry, a stepped spire) in A; in B a stump of 24 m and its top lying on the ground to the south in three pieces, houses gone under it; a town wall with towers round the town; doors on the houses and lit windows in A |
| E-45 | the east hall: through the new door in its east wall; up the lock walk to the top terrace and back, in A and in B; in A step into a lock; in B go down into each lock and up its stair; take the light in the middle lock (B) | four terraces and three stairs; the locks hold water in A (a body that goes in comes back at the lamp of the locks, a switch into the water is refused) and are dry rooms in B; the water tower over the top terrace (its tank and bell in A, broken in B) |
| E-46 | look over the town from the balcony and from the top of the lock walk | the market with its well, stalls and clock tower to the west; the theatre bowl to the north and the graveyard on the hillside beyond the wall; street lamps lit in A; in B the well dry with the bell at its foot, the theatre broken, a lone grave with a lantern |
| E-47 | the west hall: through the new door in its west wall, along the lane into the market; walk round it in A and in B; climb a counter and an awning in A | a walled square with stalls (awnings in A, bare posts and rotten crates in B), the clock tower, the lamp of the market; the well with its parapet; nothing to walk through |
| E-48 | the well: down in A to the south-west corner, Tab, down in B to the floor, take the light; back up in B to the corner, Tab, up in A | the wooden south stretch in A only; the water of A over the west and north stretches; the day bell on the dry floor in B; Tab refused while in the water's place in A |
| E-49 | look down from the gap, the terrace and the sky bridge | rock ravines below, no sea inside the town; the sea only in the bay and the harbour |
| E-50 | the north hall: walk between the two ramps to the door under landing 1, up the lane onto the stage; down into the orchestra; up the aisle at the east end of the rows to the promenade, in A and in B | the stage wall with columns and painted scenery (A), broken (B); rows of seats; in B the west part of the upper rows and the promenade gone, rubble below |
| E-51 | in A on the promenade, walk west to the wooden panel in the back wall; jump at it and press Tab in the air; take the light | the jump goes through the breach onto the terrace behind; without Tab the panel stops the body |
| E-52 | from the east end of the promenade of the theatre, through the door in the back wall, north into the graveyard, in A and in B; walk up to the bell in the arch of the tower in the north-east corner | rows of graves with flowers (A), a lone grave with a lantern (B); the hill and the town wall over the parapet; at the bell the log says the night bell is rung and night falls in A |
| E-53 | after a bell (a seal or the night bell), stand at a lit lamp and press E; A and D; E; then again and W | a panel LAMPS with the name of a lamp; A and D change it (only lit lamps); E puts the body beside that lamp with a ring; W closes the panel; while it is open the body does not walk |
| E-54 | reach the goal light; stay in A through the three strokes; after the panel press R, switch to B before the third stroke; then with the three bells rung, hold Tab through the third | three strokes with a ring each; the panel says THE PAST (the water stays), THE PRESENT (the water drains), BOTH; R puts the body back on the balcony and the water is full again |
| E-55 | after two bells (two seals, or a seal and the night bell), walk toward a hidden light not yet taken, turn away from it | a small gold light at the bottom of the screen pulses faster as it comes nearer; turned away, a gold mark at the left or right edge |
| E-56 | stay in the courtyard (or any room) without leaving it for 30 s, then 60 s | the outlines of the other world in the room grow brighter, then pulse |

Only when the device recovery is touched: `[debug] device_reset_key = 1`, F5, press F9 during a ripple. The picture goes on
after a short flicker and the log says `recovered (1 of at most 5)`.
