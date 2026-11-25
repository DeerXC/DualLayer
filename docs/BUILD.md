# Build

Controls and checks apply to the current build. The new sprint, climb, hint key and boat finale that are specified in [Design](DESIGN.md) but not built yet are not listed here.

## Requirements

1. Visual Studio 2026 with the workload **Desktop development with C++**, toolset v145.
2. **Windows SDK 10.0.26100.0** (the project pins this version; install it rather than retargeting).
3. For the GPU debug layer of the Debug build: Windows **Settings > System > Optional features > Graphics Tools**. Without it
   the game still runs and the log says that the debug layer is not available.

## Build and run

1. Open `DualLayer.slnx` (or `DualLayer.vcxproj`), choose `Debug | x64`, press **F5**. The working directory of the debugger
   is the project folder, so `shaders/` and `config/` are found.
2. `build\x64\<Config>\DualLayer.exe` also runs from Explorer: a post-build step copies `shaders\` and `config\` next to it.
3. The logic tests alone: `.\tests\run_tests.ps1` in a Developer PowerShell for VS; one test: `.\tests\run_tests.ps1 -Only town`.
   The longest is `route` (a few minutes with /O2).

## Keys

| Key | Action |
|---|---|
| Click | capture the mouse for mouse look |
| Esc | give the mouse back; a second Esc quits |
| W A S D | walk relative to the camera (flying: move) |
| Shift | hold to sprint in the town |
| Mouse, arrow keys | turn |
| Space | jump; held = full jump, tap = small hop; hold while moving towards a suitable ledge in the town to climb |
| Tab | switch the moment (A the past, warm; B the present, cold); a switch into something solid or into water is refused and the ripple turns back |
| E | (walking, in the town) at a gold handle in A, open its thing (the prompt says OPEN; in B a handle does nothing); else set the lantern down a step ahead, in A only (the prompt says PUT DOWN), or take it up within reach (TAKE). In the greybox map: take the orb in reach, or put the held one down one step ahead (on a socket, a plate or a hole when near one; the prompt says TAKE, PUT DOWN or PLACE). At a lit lamp after the first bell: the list of lamps (A / D choose, E travels, W / S / Space close) |
| R | back to the last place the body stood still on |
| F | walking / flying |
| Q / E | down / up (flying) |
| N | collision on / off (off also means flying) |
| Z / X / C | wireframe / level of detail / turning shapes on or off |
| F5 | reload `config/default.ini` |
| F9 | rebuild every GPU object (only with `[debug] device_reset_key = 1`): the test of the device recovery |

Command line: `--adapter N` chooses the N-th adapter listed in `logs/dual.log`.

The overlay shows a dot in the middle of the view (not in third person), the key prompt of the moment at the bottom, and the
name of the place at the top left. The title bar shows the adapter, the frame time, the body, the moment, the movement mode,
the place, the bells and the falls.

## Configuration

`config/default.ini` is read at start and on F5. Every key is optional; the code has a default for each, and values that make
no sense are pulled into a working range.

| Section | Keys (default) | Meaning |
|---|---|---|
| `[window]` | `width` (1600), `height` (900) | the client size in physical pixels |
| `[render]` | `vsync` (true), `adapter` (-1) | -1: the first hardware GPU in high-performance order |
| | `town_atmosphere` (true) | the sky, the haze and the mist of the town; false restores the plain backdrop for comparison |
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
| `[models]` | `segments` (32) | how round the round shapes are |
| `[debug]` | `device_reset_key` (false) | F9 tests the device recovery |
| `[test_triangle]` | `show` (false), `spin_deg_per_sec` (0) | the first test triangle |

The town is data made by the layout model. After a change of `tools/layout/city_model.py`, write it again (Python 3, numpy,
matplotlib; about a minute):

    python tools/layout/city_model.py export

It checks the model first and writes nothing when the check finds a problem.

## One command

```powershell
.\tools\check.ps1
```

It starts from any PowerShell, finds the project and the Visual Studio build tools by itself, and runs in this order:

1. the logic tests (`tests\run_tests.ps1`); a compiler warning in a test is reported as WARN;
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

## Town atmosphere play-test

- At the quay, look from the horizon towards the sky and down over the water. The sky should stay tied to the world as the camera turns. Look through the sea gate and check the next step and the gate handle remain readable.
- Switch between A and B in the same position and again while the window is resizing. Each side of the ripple should have its own sky and haze. In B, a placed lantern changes the nearby scene, not the sky.
- Set `[render] town_atmosphere = false` in `config/default.ini` and press F5 to compare with the baseline. Restore `true` after testing. Record paired views from the same positions and check laptop and desktop performance before further art work.

## Town player play-test

- At the quay, compare walking, sprinting, turning and stopping against a wall. The lantern should follow the right hand and disappear from it when placed.
- At the sea gate, turn the camera by the stonework and switch where a clear view becomes blocked. The camera should retract without crossing the wall and extend again outside.
- At the first aerial switch, check that the intended landing remains visible. Try to bypass the old gates with sprint jumps or climbs before changing the layout.

The focused logic test is `.\tests\run_tests.ps1 -Only town_player` in Developer PowerShell. If `data/town.dlt` is missing, `python tools/layout/city_model.py export` generates it from the existing map before the game is run.

## Checked by eye

What `check.ps1` cannot see. Run in Debug after a change that touches it; write a result as "E-3 ok" or "E-3 fails: what was
seen".

| No. | Area | Do | Expected |
|---|---|---|---|
| E-1 | The town | start (the town is the default) | on the quay: one ripple from warm dusk into the cold present; a heap of wreck across the quay to the west, the sea gate beyond it; the title bar says the quay |
| E-2 | The town | Tab, look round; Tab back | A: the same town in warm dusk, the quay clear; B: cold moonlight, roofs gone by the sea, the bell tower broken; a few lanterns still lit along the main path |
| E-3 | The town | walk up the sea stair, the lower street, the market stairs and the market street to the west bastion; look at the bridge | streets walked without a jump; arches over the streets; the market with its well; the bridge over the gorge to the rock, whole in A, fallen in the middle in B |
| E-4 | The town | in A walk off the bastion into the moat; jump off the quay into the sea | the moat: the view tinted under water for half a second, then back on the bastion; the sea: back on the quay; the title bar counts the falls |
| E-5 | The town | walk to the locks, the theatre and up the cliff stair to the graveyard | five locks with water in A and stairs beside each head; the theatre facing the rock; the graveyard over the town |
| E-6 | The lantern | on the west bastion in A, walk onto the bridge, E (PUT DOWN) in its middle, walk back to the bastion, Tab | in B the bridge has fallen, but its middle stands in a sphere of warm evening light with a glowing rim; the lantern glows on it |
| E-7 | The lantern | in B jump from the bastion onto the middle of the bridge; E there; then walk back to the bastion and E within reach of the lantern | on the bridge E is refused (the log: the body stands on what it keeps); from the bastion the lantern is taken up and the middle of the bridge is gone in B |
| E-8 | The lantern | in B try E with the lantern in hand; in A set it down by a wall and walk through the circle in B | E does nothing in B (the log: the past only); inside the circle the walls of A stop the body in B as well |
| E-9 | The sea gate | in A walk past the heap to the sea gate; E at the gold winch (OPEN); walk in; Tab; walk up | the portcullis rises; the cargo stops the body in the passage; in B no cargo, the rusted portcullis raised with a faint warm glow; up the stair into the lower town |
| E-10 | The sea gate | restart; in B at the winch press E; in A walk into the shut portcullis | in B nothing opens (the log: nothing done in the present changes it), no prompt; in A the portcullis stops the body |
| E-11 | The canal | at the foot of the market stairs in A, E at the crate (PUSH); Tab; in B climb down the fallen quay, onto the crate, onto the far quay | in A the crate floats to the far quay; in B it lies on the dry bed against it |
| E-12 | The lantern | on the market street in A, E on the middle platform of the scaffold; Tab; climb; jump onto the terrace; walk 8 m away, E | in B the scaffold stands in the circle; the terrace is reached; E calls the lantern back (CALL THE LANTERN) |
| E-13 | The bells | at the clock tower in A, E (RING) | the title bar: bells 1 of 3; in B a lamp burns over the rope |
| E-14 | The lift | from the west bastion in B walk down the wreck of the bridge to the floor of the gorge; at the lift house climb the heap onto its roof; Tab; E (LOWER); Tab; climb the frame | in A the cage goes down; in B the frame stands up the face of the rock and leads to the plaza (before E, in B, a wreck lies at the foot and nothing climbs) |
| E-15 | The air step | from the top of the sea stair walk west on the lower street; in A climb the two crates by the barricade; jump toward it; jump again and press Tab at the top | the barricade (A) and the heap of a fallen house (B) close the street; without Tab the jump ends at the barricade; with Tab the body lands on the heap in B and walks down its steps toward the market |

Only when the device recovery is touched: `[debug] device_reset_key = 1`, F5, press F9 during a ripple. The picture goes on
after a short flicker and the log says `recovered (1 of at most 5)`.

## Town review cameras

Run `python tools/layout/review_town.py` from the project root (Python 3, NumPy and Matplotlib). The tool writes matched A/B PNGs under `build/town-review/`, without replacing `data/town.dlt`. Use `--shot 05_north` for just the proposed tower approach or `--out` for a different output directory.

The market and north shots approximate the default orbit camera. The silhouette is a 1.7 m scale marker, not the player asset. The offline renderer does not reproduce camera collision, dynamic lighting or gameplay. Orange plan lines connect route anchors; the gold dashed approach is a proposal, not geometry or a verified path.
