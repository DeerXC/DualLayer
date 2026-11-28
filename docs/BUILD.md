# Build

Controls and checks apply to the current build. The lens, the beacons, the three ways and the finale at the tower that are specified in [Design](DESIGN.md) but not built yet are not listed here.

## Requirements

1. Visual Studio 2026 with the workload **Desktop development with C++**, toolset v145.
2. **Windows SDK 10.0.26100.0** (the project pins this version; install it rather than retargeting).
3. For the GPU debug layer of the Debug build: Windows **Settings > System > Optional features > Graphics Tools**. Without it
   the game still runs and the log says that the debug layer is not available.
4. Python 3 (optional for the game; NumPy and Matplotlib for the town data, see Configuration). `tests\run_tests.ps1 -Changed` and
   `tools\check.ps1 -Quick` ask it which tests a change can reach; without it every test runs.

## Build and run

1. Open `DualLayer.slnx` (or `DualLayer.vcxproj`), choose `Debug | x64`, press **F5**. The working directory of the debugger
   is the project folder, so `shaders/` and `config/` are found.
2. `build\x64\<Config>\DualLayer.exe` also runs from Explorer: a post-build step copies `shaders\` and `config\` next to it.
3. The logic tests alone: `.\tests\run_tests.ps1` in a Developer PowerShell for VS; one test: `.\tests\run_tests.ps1 -Only town`;
   only the tests that the uncommitted changes can reach: `.\tests\run_tests.ps1 -Changed` (see Test and commit routine).
   The longest is `route` (a few minutes with /O2). None of them needs a window or a GPU. On any system with g++ or clang++:
   `python tools/run_tests.py` builds and runs all of them side by side (`--list` shows them, `-j 4` limits the builds, `--strict`
   makes warnings errors, `--export` writes `data/town.dlt` first; one test: `python tools/run_tests.py town`). `town`, `route`, `things`, `map`, `firstminute` and `lighting` read `data/town.dlt`;
   if it is missing, `python tools/layout/city_model.py export` writes it (see Configuration).

## Keys

| Key | Action |
|---|---|
| Click | capture the mouse for mouse look |
| Esc | give the mouse back; a second Esc quits |
| W A S D | walk relative to the camera (flying: move) |
| Shift | hold to sprint in the town; flying (F or N): four times faster |
| Mouse, arrow keys | turn |
| Space | jump; held = full jump, tap = small hop; hold while moving towards a suitable ledge in the town to climb |
| Tab | switch the moment (A the past, warm; B the present, cold); a switch into something solid or into water is refused and the ripple turns back |
| E | (walking, in the town) at a gold handle in A, open its thing (the prompt says OPEN; in B a handle does nothing); else set the lantern down a step ahead, in A only (the prompt says PUT DOWN), or take it up within reach (TAKE). |
| R | back to the last place the body stood still on |
| H | ask the lantern for more help: the lantern is raised and a warm beam sweeps over the place for 6 s; pressed again during the sweep after 90 s without progress, a frame shows round the thing to use. The flame over the lantern always leans toward the next thing to do (it leans further, with sparks, after 30 s without progress) |
| L | look at the light in parts (the title bar says which): everything, no shadows, no lamps, only the shadow of the sun or the moon (white where it reaches, grey in a shadow), only the lamps |
| M | open the map: two sheets of tracing paper, the past (warm) over the present (cool); the game stands still behind it, the pointer is free. In the map: Tab turns the other sheet to the top, E puts a marker where the pointer is (or takes the one there away; five at most), M or Esc closes it |
| F | walking / flying |
| Space / E, Q | up / down (flying: Space rises as E does) |
| N | collision on / off (off also means flying) |
| Z / X | wireframe / level of detail |
| F5 | reload `config/default.ini` |
| F9 | rebuild every GPU object (only with `[debug] device_reset_key = 1`): the test of the device recovery |

Command line: `--adapter N` chooses the N-th adapter listed in `logs/dual.log`.

The overlay shows a dot in the middle of the view (not in third person), the key prompt of the moment at the bottom, and the
name of the place at the top left. The title bar shows the adapter, the frame time, the GPU time of the frame and of its parts (shadow, scene, lines, rain, hud), the body, the moment, the movement mode,
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
| | `move_speed` (12), `turn_speed` (1.5) | flying speed (m/s; Shift multiplies it by 4) and arrow-key turn speed (rad/s) |
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
| `[guidance]` | `enabled` (true), `lean_degrees` (18), `lean_idle_degrees` (40), `idle_seconds` (30), `sweep_seconds` (6), `ghost_after_seconds` (90), `ghost_seconds` (12) | the flame of the lantern and H: how far the flame leans toward the goal, how far and after how long without progress it grows restless, how long the beam stays, when the second press shows the thing to use and for how long |
| `[map]` | `reveal_radius` (15) | metres round the body that the map shows |
| `[lighting]` | `shadows` (true), `shadow_size` (2048), `shadow_radius` (60), `shadow_strength` (0.85), `shadow_bias` (3), `shadow_normal` (1.5) | the shadow map of the sun and the moon: texels along a side (512 .. 4096), metres round the camera, how much of the direct light a shadow takes, the depth bias and the normal offset in texels |
| | `point_lights` (true), `lamps_a` (0.75), `lamps_b` (1), `lantern` (1) | the lamps of the town, in A and in B, and the lantern of the traveller with the lamps that the bells light |
| | `sun_a` (1), `sun_b` (0.7), `ambient_a` (0.85), `ambient_b` (1.2), `sun_scatter` (0.25) | the light of the sun and of the moon, the sky light of each, the haze toward the sun |
| | `style` (1), `toon` (1), `toon_edge` (1.5), `cool` (0.75), `rim` (0.28), `lamp_rings` (0.6) | the style of the light (docs/DESIGN.md 6.3 and 6.10): its master (0 is the plain light), the sun in hard tones and the soft edge of a step in pixels, the cool fill of the shadows, the rim of an edge, the lamps in rings |
| | `clouds_a` (0.38), `clouds_b` (0.28), `wash` (0.9), `damp` (0.5), `haze` (0.85), `far_fog` (1), `highlight` (1) | the shadows of drifting clouds in each moment, the painted blotches of the stone, the stone damp near the water, the far haze that leans to a hue, the extra fog of the far distance, and how far a very bright light (a lamp or the lantern on a pale ground) goes toward grey instead of one flat orange |
| | `shafts` (true), `shafts_a` (0.55), `shafts_b` (0.4), `shaft_steps` (14) | the rays of light in the air: on or off, their strength in each moment, the samples along a ray of sight (4 .. 32) |
| | `grade` (1), `saturation` (1), `contrast` (1), `vignette` (1) | the grade of the picture: how far the darks and the lights lean to the colours of the look, and how much of its colour, contrast and dark corner is used |
| | `exposure` (1), `knee` (0.78), `halos` (true), `halo_a` (0.35), `halo_b` (1) | the exposure and the knee of the soft shoulder; the glow round the lit lamps and its strength in each moment |
| `[lines]` | `amount` (1), `width` (2), `crease_width` (1.2), `silhouette` (6), `crease` (30), `fade_near` (40), `fade_far` (70), `far_alpha` (0.3), `local` (0.35) | the ink lines (docs/DESIGN.md 6.10): the master (0: none), the width of an outline and of an inner edge in pixels at 1080 high, how far a pixel must stand out to be on an outline, the turn in degrees of an inner edge, the fade distances in metres, the strength of an outline far away, the share of the picture's own colour in the line; none in plain |
| `[rain]` | `amount` (1), `density` (1), `speed` (1), `length` (1), `width` (1), `alpha` (1), `rings` (1), `ring_reach` (22), `wet` (1), `overcast` (0.6), `fog` (0.25) | the rain of B (docs/DESIGN.md 6.11): the master (0: no rain and no wet look), the streaks (how many, how fast, how long and wide, how visible), the rings on the ground and how far they reach in metres, the wet stone, the overcast sky, the thicker fog; none in A or in plain |

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
   (no error, the ripple pictures were made, the shadow map was made, the title bar says light full, a normal end, no fall, the jump height). It presses keys for about 35 seconds:
   the keyboard and the mouse must stay untouched. No key is left down, even when the script is stopped;
4. the repository: nothing that must not be uploaded (build output, logs, private files, `config/local.ini`, one-time
   scripts `tools\apply_*` / `fix_*` / `docs_*` / `add_*`, files over 50 MB), and whether the hooks are on.

The table at the end says `OK`, `WARN`, `SKIP` or `FAIL` per step; the exit code is 0 unless something failed. Switches:
`-Quick` (the daily check, see Test and commit routine), `-Fast`, `-SkipBuild`, `-SkipSmoke`.

To check and, only when everything passed, commit and push:

```powershell
.\tools\check.ps1 -Commit "feat(stage): add the west wing" -Push
```

The message must be `type(scope): subject`, at most 72 characters, without a final period or a double quote. `git add -A`,
`git audit` (at most 90 s), `git commit` (120 s) and `git push` (180 s) run in turn and stop at the first failure; a command
that does not answer in its time is stopped and reported. `--no-verify` is never used.

## Test and commit routine

One routine for every change, in a Developer PowerShell for VS opened in the repository folder. A new window needs the execution
policy once: `Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass` (for good: `-Scope CurrentUser -ExecutionPolicy RemoteSigned`).

| Step | Command | Expect |
|---|---|---|
| 1. Bring the change in | copy or unpack the files (an update package: `dl-update`, see Helper scripts), then `git status --short` | the changed files and nothing else |
| 2. Test what the change can reach | `.\tests\run_tests.ps1 -Changed` (one test: `-Only style`) | `all N test(s) passed`; N is the number of tests the change can reach, possibly 0 |
| 3. Check and commit | `.\tools\check.ps1 -Quick -Commit "type(scope): subject"` | every row `OK` (or `SKIP` with its reason), then `git commit` `OK` |
| 4. Push | `git push` | the branch moves, and the hosted runner runs every logic test (`tests.yml`) |

`-Quick` is the daily check: it runs the tests that a change can reach (a changed file is one of their sources or is included by them, or the code names a changed file of `data/` or `config/`, or anything under `tools/layout/` changed; `python tools/affected_tests.py --explain` says which and why), builds Debug and Release incrementally, runs the game only when something in `src/`, `shaders/`, `config/`, `data/` or the project file changed, and then does what the full check does (repository check, audit, hooks). A doubt runs everything.

Use the full check (`.\tools\check.ps1 -Commit "type(scope): subject"`) before a milestone tag, after a change of `tools/layout`, the town
data or `src/scene` (the route test takes a few minutes with /O2), and after a long break. The game run takes about 35 seconds and
needs the keyboard and the mouse untouched.

## Helper scripts outside the repository

Two small scripts live in a private folder outside the repository, beside the commit hooks and the audit script, and are on the PATH.
They are not part of the repository. Open a terminal in the repository; neither needs a path.

| Command | Does |
|---|---|
| `dl-update` | puts an update package into the repository: takes the newest `DualLayer_Update_*.zip` of the Downloads folder (or the zip given as an argument; `--list` only shows what is inside), writes its files over the ones of the same name, runs the `_apply.py` of the package once when there is one (an edit made in place: a line of a table, a section of a config file) and shows `git status --short`. It refuses a package that would write outside the repository, and never commits or pushes |
| `pack` | writes source files as text files `upload_part_NN.txt` in the folder `<repository>_upload` beside the repository, so that they can be attached to a message: every tracked and new file by default; `--changed` (what differs from the last commit), `--since REV`, `--only PATH...`, with `--add PATH` and `--skip GLOB`. It leaves out secrets, binary files and files over 2 MB, never writes inside the repository and removes the older parts of its folder first |

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
| E-16 | The first minute | start, touch nothing for 3 s | in the cold present on the quay: the heap of wreck on the left, the towers of the sea gate left of the middle, and high in the sky right of the middle three dark lamps on a yard (a sphere, a column, a box) over the roofs; the traveller with the lantern in the hand, its flame leaning toward the sea gate |
| E-17 | The flame | walk about on the quay; stand still for 35 s; then walk somewhere new | from every place the flame leans toward the winch of the sea gate; after 30 s without moving it leans further and throws sparks; walking to new ground calms it; at the winch it stands upright |
| E-18 | H | press H; wait 7 s; stand still for 95 s, then press H twice | the arm raises the lantern and a warm beam swings over the place and settles toward the winch, and ends by itself; after the long wait the second press shows a faint frame round the winch with a line of motes to the lantern (12 s); a press while it shows puts it away |
| E-19 | The bells | ring the day bell in the market (E at its rope in A); look at the mast from the quay | the title bar says lamps 1 of 3; the lamp of the well (the sphere, on the left) glows warm, the other two stay dark; it glows in B as well |
| E-20 | The map | press M at the start; walk to the sea stair; press M, Tab, E with the pointer on the paper, M | the paper is blank but for a disc round the arrow and the tower north of it with three dark squares; Tab turns the other sheet to the top (the frame changes colour; the world does not switch); E plants a flag and, once closed, a thin column of light stands in the world (in A and in B); E on the flag takes it away; M closes and the mouse looks again; opening it at the west bastion reveals the town far round |
| E-21 | The light | start and touch nothing; press L five times, then once more | the title bar says light no shadow, no lamps, shadow, lamps, plain and then full again; in "shadow" the quay is white and the traveller, the heap of wreck, the sea wall and the posts leave grey shadows that all lie the same way; in "lamps" only the warm pool of the lantern near the feet and the lit lamps along the sea stair show; no speckle (acne) on the stone, no shadow floating away from its object |
| E-22 | The shadows | in A walk along the quay and up the sea stair and turn the camera slowly; Tab; walk and turn again | the shadows of A are long and low (the sun is low), those of B short and steeper (the moon is high); they do not crawl or shimmer when the body walks or the camera turns; their edges are soft; they end softly toward the far town, not with a line; the shadow of the traveller follows the traveller |
| E-23 | The lamps | in B walk the sea stair and the lower street; E in A sets the lantern down, Tab, walk 6 m away, E calls it back | the lit lamps of the path are warm pools about 14 m across with a soft glow round each, in A all lamps are lit and weaker; the lantern lights the stone round the traveller and flickers; set down it stays with its circle; no lamp blinks on or off as the body walks; a bell rung lights its lamp on the mast and the tower under it |
| E-24 | The exposure | stand with the sun of A in the picture, then in the light of a lamp of B; edit `[lighting]` (`exposure`, `knee`, `lamps_b`, `shadows = false`) and press F5 | no wall is a flat white patch; the pool of a lamp keeps its hue; the edits take effect at once and the game goes on; note the frame time on the laptop with and without shadows (the shadow pass draws the town a second time) |
| E-25 | The style | start in B and look along the quay, Tab to A and look at the sea wall, then turn about 130 degrees toward the sun; walk to the lower town in A and in B; press L until the title bar says light plain, and back to full; edit `[lighting]` (`style`, `shafts`, `clouds_a`, `wash`, `grade`) and press F5 | in full the sun falls on the faces in three hard tones, the shade side is bluer than the lit side, the traveller has a bright edge on the sun side, slow soft patches of cloud shadow cross the quay (the wall and the street below are under the same cloud) and the stone has soft blotches and, near the water, a darker, cooler foot; the lamps show soft rings with a whiter core; toward the sun rays of light show between the buildings and in the sky; the distance is lavender in A and teal in B; the corners are a little dark; in plain the same view is flat and warm or flat and cold; every landing, edge and interaction reads in both; no flicker or grain in the rays when the camera turns; the edits take effect at once; note the frame time on the laptop and the desktop with `shafts = false` and with `shaft_steps = 8` |
| E-26 | The traveller and the lantern | start in B and look at the traveller from the front (turn the camera), from the side and from behind; walk, sprint (Shift) and stop; jump; press H; Tab to A; E sets the lantern down, look at it, E takes it up | the traveller is a rounded figure with a round hood and peak, a scarf, a bell of a coat, boots and a small pack with a bedroll, 1.72 m high; the feet are on the floor in every step and the body bobs a little; the knee of the leg that swings forward bends; the scarf and the coat vent stream behind at a run; the lantern hangs from the ring in the right hand, sways with the stride and never touches the floor; with H the arm raises it; its glass glows, the flame inside wavers, the glow round it is soft and does not cut into the walls of a street; the light on the stone is amber and flickers and moves with the sway; set down, it hangs from the arm of a small iron post with the glass at the centre of the circle, and is taken up again; the traveller never disappears (the table of objects has room: 512) |
| E-27 | The map | (after `python tools/layout/city_model.py export`, which writes the plan again) in a new game press M at once; Tab; click DIFFERENCE and MAP; click ZOOM + three times, move with the arrow keys, click CENTER, then ZOOM - until the whole town shows; press T and E several times at different places, in A and in B; walk a few streets, open the map again; solve the first puzzle (the sea gate) and open the map | the whole town is on the paper at once under a soft cloud of fog (violet in A, teal in B), with only a small clear patch round the player; nine rings with a question mark show in the fog, the next one stronger; no building, street or water is inked under the fog; the ink is smooth: no staircase edges, no stripes of squares; Tab swaps the sheet and the colour of the fog; the buttons act; zoom stops at 4 x and at 1 x; the window cannot leave the town; the shape of the next mark is shown on the MARK button; up to ten marks, each in the colour of the sheet it was set on and paler on the other sheet; walking clears the fog behind the traveller and the cleared part stays; no long thin stripes of brown across the town; after the sea gate its ring is gone, its district is lit and the gate shows a pale outline; push the canal crate and open the map: the crate is at its new place (a pale outline); lower the lift in A and look at B: its stairs show; the map opens at once on a laptop (note the frame time while it is open and while moving the window) |
| E-28 | The fabric of the town | after `python tools/layout/city_model.py export`: from the market plaza walk the garden lane to its cistern and its terrace; from the east stair take the mill lane to its terrace; walk the lower street, the lower east lane and the quays of the locks; look at the town from the west bastion and from the quay; Tab | every house has its door on a street, a lane, a quay or the plaza, and no house stands in the middle of a block; the garden lane climbs from the plaza without a jump to a cistern (a stone drum: a wooden lid in A, the lid fallen in in B) and a terrace with a parapet; the mill lane leaves the east stair and ends on a terrace over the five basins; opening the map on either terrace shows the town round it; olives stand only on open ground, none on a street or at a door; net racks west of the lower street (nets in A, bare rails in B); at the first frame the three lamps of the mast are still in sight |
| E-29 | The street network | after `python tools/layout/city_model.py export`: from the sea gate walk the lower street, take each of the four passages (a stone gate with a lantern at its mouth) to the cooper lane, cross the fish market; walk the weaver lane to its green, the vine lane up to the garden lane (the chapel on the way) and back; after the plank walk the tanner loop; after the gatehouse the wall lane to the north terrace; look at the corridors (the lower street, the market stairs, the market street, the upper street) from the street and from a high place; open the map on each green and terrace | the lower town is made of blocks: two rows of houses between the lower street and the cooper lane, a passage every 12 to 17 m, barrels, crates and benches at doors, never on a lane; the fish market has a fountain (water in A, a dry basin in B) and three stalls; the walls of the corridors read as the fronts of houses (two rows of windows with shutters, doors at the foot, a cornice and red eaves at the top) and no ledge on them can be climbed; the vine lane and the garden lane make a loop with the cistern and the chapel (a small stone chapel with a bell-cote, roofless in B); every dead-end lane ends at a green or a terrace with a bench and a view, and the map shows the town round it; sheds, woodpiles, well-heads, carts and arbours stand in the yards, plots of vegetables and vines and olives, figs and cypresses on the open ground; no lane goes round a puzzle (`tests\run_tests.ps1 -Only route` passes); the frame time is the same as before E-28 (note it in the lower town and on the market terrace) |
| E-30 | Rooms and puzzles in the world | after `python tools/layout/city_model.py export`: walk into each of the six rooms (fish market, cooper lane, the lock quay, the wall lane three times) in A and in B; look out of the window of the watch room and of the mill; walk to the sea gate, the canal crate, the plank and the gatehouse crate and look at what stands round each; walk the lower town under the washing; walk to the boat yard in the south-west, the rope walk at the south-east and the quiet garden in the north-centre | the door is easy to find and the third-person camera stays inside every room; the dinner table, the barrels, the loom, the sacks, the telescope and the small bed read at once and say something without text; in B the same room is open to the sky and something is broken, except the small glow on the child's bed; the theatre seen from the watch room has columns and an awning in A and stumps in B, the locks seen from the mill are full in A and dry in B; the bars of a window cannot be passed; every thing that can be used has a gold cap or grip in A (dull in B) and it is the first thing the eye finds there, without hiding the answer; the crate at the canal belongs to a cargo, the plank to roofers, the gatehouse crate to a hoist; nothing stands on a way that a puzzle uses (`tests\run_tests.ps1 -Only route` and `-Only things` pass); washing hangs over the passages at 4.6 m (a jump does not reach it); the boat is whole in A and ribs in B; the frame time is the same as before E-29 |
| E-31 | One style | after `python tools/layout/city_model.py export`: start the game; walk the lower town, the fish market, the market street, up to the clock tower and the market hall, the garden lane and the vine lane to the chapel (walk in), the upper street; Tab; press L until "plain"; set `wash` of `[lighting]` to 0, 0.6 and 0.9 and press F5 | each house has a lime wash of its own and never the neighbour's; the roofs are four ages of one tile; the stone of the streets is darker than the walls; shutters, awnings and a few flowers are the only cold or saturated colours (and gold is only on what can be used); the stone, plaster and roofs have soft blotches and tall stains that move with the light, not a pattern of blocks; near the water they are darker and cooler; B is the same town in the same colours, cooler and greyer; in greyscale (a screenshot, desaturated) the roof is darker than the wall and the street darker than the wall in A and in B; the market hall has a shop front under each arch, the clock tower has a clock on each face, the chapel can be entered by a short ramp and has an altar and benches; the facades of the corridors have a different lime wash and shutter in each bay; `wash = 0` gives flat colour; the frame time is the same as before E-30 (note it in the lower town and at the market) |
| E-32 | Fly keys and the moon | start the game; press N (noclip: the title bar says so) and fly: hold Space to rise, Q to sink, Shift to go four times as fast; fly a lap round the town and over the harbour; Tab and look at B from the air and from the quay (after `python tools/layout/city_model.py export`) | Space lifts as E does and Shift is clearly faster (12 m/s, 48 with Shift: the whole town in about six seconds); walking is unchanged (Shift sprints, Space jumps); B is silver stone under a lavender-grey sky, weathered and warm, not blue ice: the stone and the plaster keep their hue, only duller, the shadows lean blue-violet, the lamps are amber; change `B_AGE` in `tools/layout/town_style.py` (0 cold, 1 aged) and export again if the moon is too grey or too blue |
| E-33 | The court, the roofs, ten rooms, the ground | after `python tools/layout/city_model.py export`: walk the lower street to the gate in its north wall (a round arch, at 45 m) and into the court; walk the market street, the market stairs and the upper street and look up; walk into the four new rooms (baker, scribe, lamplighter, sailmaker); fly over the town (N, Space, Shift) and look at the edges of the grass, the fields, the sand and the rock | the gate in the lower street leads to a court with a fountain to one side (water in A, dry in B with a lamp of the memory in the basin), an olive (leaf in A, bare in B), benches, laundry over it and a door at the end; the court is a dead end and every wall of it is too high to climb; the long walls of the corridors have a roof over each house, some a storey higher than the next, chimneys, dormers and here and there a balcony at the third floor, and nothing of it can be reached from the street; the baker has an oven (fire in A), the scribe a model of the town on a table (a wall in A, a ring of stumps in B), the lamplighter lanterns on chains (one lit in B), the sailmaker sails on a pole (down in B); the ground has no staircase edges between grass, field, sand and rock; the frame time is the same as before E-32 (note it in the lower town and over the harbour) |
| E-34 | The atmosphere | at the quay look from the horizon up to the sky and down over the water; Tab between A and B, also while the window is resized; set `[render] town_atmosphere = false`, press F5, compare, set it back to `true` | the sky stays tied to the world as the camera turns; each side of the ripple has its own sky and haze; a lantern set down in A changes the nearby scene in B and not the sky; the sea gate and its handle stay readable; `false` gives the plain backdrop |
| E-35 | The traveller and the camera | at the quay walk, sprint, turn and stop against a wall; at the sea gate turn the camera by the stonework and switch where the view becomes blocked; at the first aerial switch look at the landing | the lantern follows the right hand and leaves it when set down; the camera retracts without crossing the wall and extends again outside; the intended landing stays in view |
| E-36 | The ink lines and the far colours | start in B on the quay, then Tab to A; look along the quay, at the market stairs and from the air (N, Space, Shift); press L until the title bar says light plain, and back; edit `[lines]` and `[lighting]` (`far_fog`, `haze`, `highlight`) and press F5 | a thin dark line round every outline and at the foot of a wall, none inside a flat face or on a gentle slope; brown-black in A, indigo-black in B; no crawl or flicker; the water over the lines; the far hills and the sea of A pale lavender and the sky at the horizon the same colour, with no band of orange; a pool of lantern or lamp light on the paving of A pale amber in the middle and orange at the edge; long shadows in A; in plain no line and the far colours as the model says; the log has no `ink lines are off`; note the GPU time of the lines in the title bar with `amount` 1 and 0 |
| E-37 | The rain of B and the GPU time | start in B on the quay; look along it, at the sea and the sky; turn the camera slowly; Tab to A and back; press L until plain, and back; set the lantern down in A, Tab, look at its circle; edit `[rain]` and press F5; read the title bar | fine pale strokes slanted along the moonlight at three depths, over the sky and the sea and in front of the walls but not through a nearer wall; small rings on the ground near the traveller (none beyond about 22 m); the streaks stay fixed to the screen and do not shimmer; the stone a little darker and shinier, the sky grey-blue and flat, the moon a soft glow, the fog thicker; the circle of the lantern dry; none in A or in plain; the ink lines still seen; the route reads in the moon alone; `amount` 0 is B as it was; the log has no `the rain is off`; the title bar says `gpu` with a number; note the GPU time of the frame and of the rain on both machines (streaks still fall under eaves: that is R2) |

Only when the device recovery is touched: `[debug] device_reset_key = 1`, F5, press F9 during a ripple. The picture goes on
after a short flicker and the log says `recovered (1 of at most 5)`.

## Town review cameras

Run `python tools/layout/review_town.py` from the project root (Python 3, NumPy and Matplotlib). The tool writes matched A/B PNGs under `build/town-review/`, without replacing `data/town.dlt`. Use `--shot 05_north` for just the proposed tower approach or `--out` for a different output directory.

The market and north shots approximate the default orbit camera. The silhouette is a 1.7 m scale marker, not the player asset. The offline renderer does not reproduce camera collision, dynamic lighting or gameplay. Orange plan lines connect route anchors; the gold dashed approach is a proposal, not geometry or a verified path.
