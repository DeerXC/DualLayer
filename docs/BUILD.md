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
3. the game, with a fresh `logs\dual.log`: it starts on the terrace, walks, jumps with Space held, switches walking and
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
| E-1 | start, look north; Tab | the beacon over the tower is seen over the town wall; the view is wide; no D3D11 / DXGI message in the output window. A: a warm evening, sandstone, a light haze, the gate shut. B: a cold grey light, dark wet stone with moss on top faces, a thick fog, the wall broken open where the gate was, with rubble |
| E-2 | Tab, then Tab again during the ripple | a ring opens from the centre and the other layer shows inside it; the second Tab turns it round without a jump |
| E-3 | through the breach in B; onto the blue stone; jump, Tab in the air | the body lands on the red strip of A; at the end of the strip the courtyard is seen from above |
| E-4 | in the courtyard, in A, push the block west | refused; the outline of the blue wall flashes on the floor in both layers; no blue box appears in A |
| E-5 | push the block onto the plate; walk through the north door in B | the hub, with the tower and three dark seal lamps |
| E-6 | walk up and down the ramps of the wings and the tower, turning with the mouse | the picture does not shake; the walk does not stop at the foot of a ramp |
| E-7 | west wing: W1 the orb to the socket (in A it is put down beside it, red-gold, and TAB SWITCH WORLD is offered; switched with it to B, it turns blue and is placed: the bridge); W2 the other orb onto the plate in A, through the door in B, the lever, back for the orb; W3 the orb over the hole (in A it lies on the lid, in B it falls and the bars open); the seal; up the shortcut | each step does what it says; the held orb is seen in front of the view; an orb left in one layer leaves a small mark in the other; one lamp lights and the shortcut opens into the hub |
| E-8 | east wing up to the courtyard wall top, the seal, off its north end | the body drops onto the gallery of the hub, which looks over the hub; off its edge into the hub; the gallery cannot be jumped onto from the floor |
| E-9 | north wing up to the seal, along the ledge through the window, off it; on the ledge jump toward the tower | the body drops into the hub; nothing higher is reached from the ledge |
| E-10 | with three seals, up the six ramps of the tower | the balcony looks back over the map; `CLEAR` with the time |
| E-11 | fall off a ramp; press R in the courtyard | the body comes back at the entry of the room; R puts the block back |
| E-12 | close the window | the live-object report lists only the device |
| E-13 | in the hub, look round | the archive (west), the water ring (east) and the bell tower (north) stand over the walls |
| E-14 | start; walk to the first orb of the west wing; take it; walk to the trench | TAB SWITCH WORLD at the bottom until Tab was used three times; a dot in the middle; the status line at the top left; E TAKE near the orb, E PUT DOWN while holding it, E PLACE in front of the socket; a blue column of light over the socket seen from across the hall; the outline of the bridge over the trench in both layers |

Only when the device recovery is touched: `[debug] device_reset_key = 1`, F5, press F9 during a ripple. The picture goes on
after a short flicker and the log says `recovered (1 of at most 5)`.

