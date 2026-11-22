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
3. the game, with a fresh `logs\dual.log`: it starts in the guardhouse, walks out through its arch, jumps with Space held, switches walking and
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
| E-1 | start, look north; Tab | in the guardhouse; its arch frames the cathedral and the beacon; no D3D11 / DXGI message in the output window. A: a warm evening, sandstone, a light haze, the iron gate shut. B: a cold grey light, dark wet stone with moss on top faces, a thick fog, the wall broken open where the gate was |
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
| E-23 | anywhere: boxes of one world | wood or iron in A, fallen masonry in B, never plain red or blue; a thin red (A) or blue (B) edge at their base in their own world and an outline in the other; the aqueduct over the east of the square, its waterfall into the pool in A, fallen in B |

Only when the device recovery is touched: `[debug] device_reset_key = 1`, F5, press F9 during a ripple. The picture goes on
after a short flicker and the log says `recovered (1 of at most 5)`.




