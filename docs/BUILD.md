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
| Tab | switch the moment (A the past, warm; B the present, cold); a switch into something solid or into water is refused and the ripple turns back |
| E | (walking, in the town) at a gold handle in A, open its thing (the prompt says OPEN; in B a handle does nothing); else set the lantern down a step ahead, in A only (the prompt says PUT DOWN), or take it up within reach (TAKE). In the greybox map: take the orb in reach, or put the held one down one step ahead (on a socket, a plate or a hole when near one; the prompt says TAKE, PUT DOWN or PLACE). At a lit lamp after the first bell: the list of lamps (A / D choose, E travels, W / S / Space close) |
| R | back to the lit lamp of the room, else its entry; puts back the objects of the room (the crate, the orbs, the canal). After an ending: the last moment again |
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
| `[map]` | `town` (true) | the town of the layout model (`data/town.dlt`); false: the greybox of the previous map with its puzzles |

The town is data made by the layout model. After a change of `tools/layout/city_model.py`, write it again (Python 3, numpy,
matplotlib; about a minute):

    python tools/layout/city_model.py export

It checks the model first and writes nothing when the check finds a problem.

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

| No. | Area | Do | Expected |
|---|---|---|---|
| E-1 | Start and the engine | start, look north; Tab; Tab again during the ripple | on the quay: the stair up to the guardhouse, the town wall, the bell tower of the cathedral over the town (a spire in A, a broken shaft in B); a ring opens from the centre and turns round without a jump; no D3D11 / DXGI message in the output window |
| E-2 | Start and the engine | the title bar after the start; the log | `static batch A` and `B` lines count tens of thousands of triangles; a few dozen draws; the frame time steady |
| E-3 | Start and the engine | close the window | the live-object report lists only the device |
| E-4 | Walking, falls and recovery | walk up and down ramps and stairs, turning with the mouse | the picture does not shake; the walk does not stop at the foot of a ramp |
| E-5 | Walking, falls and recovery | fall off a ramp, off the side of the terrace into the lane, off the quay into the sea; press R | the body comes back beside the lit lamp of the room (a ring of light), else at its entry; never stands on the ground outside |
| E-6 | Walking, falls and recovery | in B, in the dry cellar and on the floor of the dry pool, press Tab; then on the causeway | refused in the cellar and the pool; allowed on the causeway |
| E-7 | The prologue | from the guardhouse: past the lamp, the gate in A and in B, the heap in B, the ditch (beam of B, jump, Tab in the air), the gap | the lamp lights as the body passes; in B the heap closes the way; the switch in the air lands on the planks; the ditch has stairs back up |
| E-8 | The prologue | the courtyard from the overlook, then inside; in A push the crate west; then onto the plate; through the north door in B | flagstones, a blind arcade, stacks of crates (A) and fallen masonry (B); the push west is refused and the blocker flashes in both layers; the plate is brass and glows; the door opens onto the square |
| E-9 | The square and the cathedral | the square in A and in B: the rim, the pool, the causeway, the arcade, the bars of the undercroft | in A the pool is water (waves, the sky and the bed in it, foam along the rim, the aqueduct falling white); in B it is dry and walked by two stairs; a light behind the bars |
| E-10 | The square and the cathedral | in A walk off the rim into the pool | a splash where the body went in; for half a second the view is tinted under the surface; Tab does nothing meanwhile; then back at the lamp |
| E-11 | The square and the cathedral | look up at the cathedral; take a hidden light and look again | the belfry and the great bell in A, broken teeth and the cracked bell in B; one more lamp lit on the ring for each light |
| E-12 | West | the west wing: W1 to W4 (the orb to the socket, the plate and the lever, the orb over the hole), the seal, the shortcut | each step does what it says; the orb takes the colour of the world it is in; TAB SWITCH WORLD is offered in front of a socket of the other world |
| E-13 | West | the lane, the market, the well: down in A to the south-west corner, Tab, down in B to the floor, the light and the day bell; back up | the wooden stretch in A only, the water of A over the west and north stretches; the bell rings when walked up to |
| E-14 | West | B: the passage from the west hall to the north corridor | a breach at each end in B, panels in A |
| E-15 | East | the east hall: the lock wheel, the crate, the quay; the weir; in B onto the crate, into the breach, the lever; out onto the rim | the wheels turn only in A (gold columns of light until turned); the crate rises, then drifts; the bars open for good; R by the canal puts it back |
| E-16 | East | in B the canal bed, the cellar to the grate of the west wing | a light on the bed and one under the courtyard; in A both ends of the cellar are closed and it is under water |
| E-17 | East | the lock walk and the tower lock in A and in B; in B push the crate to the wall, climb it, jump, Tab, through the door, under the bell | three falls between the locks in A; dry rooms in B; the crate floats under the door in A; the dawn bell rings and the water of A rises |
| E-18 | North | the north hall: landing 2 and the crystal; the heap and the second crystal; the lift after the north seal; the sky bridge in A and in B | the portcullis moves into B; the cage and the step change worlds; the lift carries the body up; the middle of the sky bridge is gone in B |
| E-19 | North | the theatre: up to the promenade in A, at the panel jump and Tab in the air; the graveyard and its bell | the jump goes through the breach in B onto the terrace and its light; the night bell rings and night falls in A |
| E-20 | The city | after a bell, E at a lit lamp; after two bells, walk toward a hidden light | the list of lamps (only lit ones); the lantern pulses faster nearer a light, with a mark at the edge when it is not ahead |
| E-21 | The city | stay 30 s, then 60 s in one room | the pale outlines of the other world grow bright, then pulse |
| E-22 | The city | from the balcony and the top of the lock walk, look round in A and in B | streets winding round the centre with houses turned to them and lamps; the halls clad as houses; in B shells by the sea, roofless houses with rafters in the lower town, grass in the streets, reeds in the harbour, felled pines; long swells on the sea; nothing floats, no void |
| E-23 | The city | reach the goal: stay in A through the three strokes; R and switch to B; with the three bells hold Tab | THE PAST (the water stays), THE PRESENT (the water drains and the walls stay wet for a while), BOTH; R plays the last moment again |
| E-24 | The town | start (the town is the default) | on the quay: one ripple from warm dusk into the cold present; a heap of wreck across the quay to the west, the sea gate beyond it; the title bar says the quay |
| E-25 | The town | Tab, look round; Tab back | A: the same town in warm dusk, the quay clear; B: cold moonlight, roofs gone by the sea, the bell tower broken; a few lanterns still lit along the main path |
| E-26 | The town | walk up the sea stair, the lower street, the market stairs and the market street to the west bastion; look at the bridge | streets walked without a jump; arches over the streets; the market with its well; the bridge over the gorge to the rock, whole in A, fallen in the middle in B |
| E-27 | The town | in A walk off the bastion into the moat; jump off the quay into the sea | the moat: the view tinted under water for half a second, then back on the bastion; the sea: back on the quay; the title bar counts the falls |
| E-28 | The town | walk to the locks, the theatre and up the cliff stair to the graveyard | five locks with water in A and stairs beside each head; the theatre facing the rock; the graveyard over the town |
| E-29 | The lantern | on the west bastion in A, walk onto the bridge, E (PUT DOWN) in its middle, walk back to the bastion, Tab | in B the bridge has fallen, but its middle stands in a sphere of warm evening light with a glowing rim; the lantern glows on it |
| E-30 | The lantern | in B jump from the bastion onto the middle of the bridge; E there; then walk back to the bastion and E within reach of the lantern | on the bridge E is refused (the log: the body stands on what it keeps); from the bastion the lantern is taken up and the middle of the bridge is gone in B |
| E-31 | The lantern | in B try E with the lantern in hand; in A set it down by a wall and walk through the circle in B | E does nothing in B (the log: the past only); inside the circle the walls of A stop the body in B as well |
| E-32 | The sea gate | in A walk past the heap to the sea gate; E at the gold winch (OPEN); walk in; Tab; walk up | the portcullis rises; the cargo stops the body in the passage; in B no cargo, the rusted portcullis raised with a faint warm glow; up the stair into the lower town |
| E-33 | The sea gate | restart; in B at the winch press E; in A walk into the shut portcullis | in B nothing opens (the log: nothing done in the present changes it), no prompt; in A the portcullis stops the body |
| E-35 | The canal | at the foot of the market stairs in A, E at the crate (PUSH); Tab; in B climb down the fallen quay, onto the crate, onto the far quay | in A the crate floats to the far quay; in B it lies on the dry bed against it |
| E-36 | The lantern | on the market street in A, E on the middle platform of the scaffold; Tab; climb; jump onto the terrace; walk 8 m away, E | in B the scaffold stands in the circle; the terrace is reached; E calls the lantern back (CALL THE LANTERN) |
| E-37 | The bells | at the clock tower in A, E (RING) | the title bar: bells 1 of 3; in B a lamp burns over the rope |
| E-34 | The air step | from the top of the sea stair walk west on the lower street; in A climb the two crates by the barricade; jump toward it; jump again and press Tab at the top | the barricade (A) and the heap of a fallen house (B) close the street; without Tab the jump ends at the barricade; with Tab the body lands on the heap in B and walks down its steps toward the market |

Only when the device recovery is touched: `[debug] device_reset_key = 1`, F5, press F9 during a ripple. The picture goes on
after a short flicker and the log says `recovered (1 of at most 5)`.
