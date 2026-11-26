# Build

Controls and checks apply to the current build. The lens, the beacons, the three ways and the boat finale that are specified in [Design](DESIGN.md) but not built yet are not listed here.

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
| H | ask the lantern for more help: the lantern is raised and a warm beam sweeps over the place for 6 s; pressed again during the sweep after 90 s without progress, a frame shows round the thing to use. The flame over the lantern always leans toward the next thing to do (it leans further, with sparks, after 30 s without progress) |
| L | look at the light in parts (the title bar says which): everything, no shadows, no lamps, only the shadow of the sun or the moon (white where it reaches, grey in a shadow), only the lamps |
| M | open the map: two sheets of tracing paper, the past (warm) over the present (cool); the game stands still behind it, the pointer is free. In the map: Tab turns the other sheet to the top, E puts a marker where the pointer is (or takes the one there away; five at most), M or Esc closes it |
| F | walking / flying |
| Q / E | down / up (flying) |
| N | collision on / off (off also means flying) |
| Z / X | wireframe / level of detail |
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
| `[guidance]` | `enabled` (true), `lean_degrees` (18), `lean_idle_degrees` (40), `idle_seconds` (30), `sweep_seconds` (6), `ghost_after_seconds` (90), `ghost_seconds` (12) | the flame of the lantern and H: how far the flame leans toward the goal, how far and after how long without progress it grows restless, how long the beam stays, when the second press shows the thing to use and for how long |
| `[map]` | `reveal_radius` (15) | metres round the body that the map shows |
| `[lighting]` | `shadows` (true), `shadow_size` (2048), `shadow_radius` (60), `shadow_strength` (0.85), `shadow_bias` (3), `shadow_normal` (1.5) | the shadow map of the sun and the moon: texels along a side (512 .. 4096), metres round the camera, how much of the direct light a shadow takes, the depth bias and the normal offset in texels |
| | `point_lights` (true), `lamps_a` (0.55), `lamps_b` (1), `lantern` (1) | the lamps of the town, in A and in B, and the lantern of the traveller with the lamps that the bells light |
| | `sun_a` (1), `sun_b` (0.8), `ambient_a` (1), `ambient_b` (0.9), `sun_scatter` (0.25) | the light of the sun and of the moon, the sky light of each, the haze toward the sun |
| | `style` (1), `toon` (0.6), `cool` (0.75), `rim` (0.28), `lamp_rings` (0.6) | the style of the light (docs/DESIGN.md 6.3): its master (0 is the plain light), the sun in soft bands, the cool fill of the shadows, the rim of an edge, the lamps in rings |
| | `clouds_a` (0.38), `clouds_b` (0.28), `wash` (0.9), `damp` (0.5), `haze` (0.6) | the shadows of drifting clouds in each moment, the painted blotches of the stone, the stone damp near the water, the far haze that leans to a colour |
| | `shafts` (true), `shafts_a` (0.55), `shafts_b` (0.4), `shaft_steps` (14) | the rays of light in the air: on or off, their strength in each moment, the samples along a ray of sight (4 .. 32) |
| | `grade` (1), `saturation` (1), `contrast` (1), `vignette` (1) | the grade of the picture: how far the darks and the lights lean to the colours of the look, and how much of its colour, contrast and dark corner is used |
| | `exposure` (1), `knee` (0.78), `halos` (true), `halo_a` (0.35), `halo_b` (1) | the exposure and the knee of the soft shoulder; the glow round the lit lamps and its strength in each moment |

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
| E-16 | The first minute | start, touch nothing for 3 s | in the cold present on the quay: the heap of wreck on the left, the towers of the sea gate left of the middle, and high in the sky right of the middle three dark lamps on a yard (a sphere, a column, a box) over the roofs; the traveller with the lantern in the hand, its flame leaning toward the sea gate |
| E-17 | The flame | walk about on the quay; stand still for 35 s; then walk somewhere new | from every place the flame leans toward the winch of the sea gate; after 30 s without moving it leans further and throws sparks; walking to new ground calms it; at the winch it stands upright |
| E-18 | H | press H; wait 7 s; stand still for 95 s, then press H twice | the arm raises the lantern and a warm beam swings over the place and settles toward the winch, and ends by itself; after the long wait the second press shows a faint frame round the winch with a line of motes to the lantern (12 s); a press while it shows puts it away |
| E-19 | The bells | ring the day bell in the market (E at its rope in A); look at the mast from the quay | the title bar says lamps 1 of 3; the lamp of the well (the sphere, on the left) glows warm, the other two stay dark; it glows in B as well |
| E-20 | The map | press M at the start; walk to the sea stair; press M, Tab, E with the pointer on the paper, M | the paper is blank but for a disc round the arrow and the tower north of it with three dark squares; Tab turns the other sheet to the top (the frame changes colour; the world does not switch); E plants a flag and, once closed, a thin column of light stands in the world (in A and in B); E on the flag takes it away; M closes and the mouse looks again; opening it at the west bastion reveals the town far round |
| E-21 | The light | start and touch nothing; press L five times, then once more | the title bar says light no shadow, no lamps, shadow, lamps, plain and then full again; in "shadow" the quay is white and the traveller, the heap of wreck, the sea wall and the posts leave grey shadows that all lie the same way; in "lamps" only the warm pool of the lantern near the feet and the lit lamps along the sea stair show; no speckle (acne) on the stone, no shadow floating away from its object |
| E-22 | The shadows | in A walk along the quay and up the sea stair and turn the camera slowly; Tab; walk and turn again | the shadows of A are long and low (the sun is low), those of B short and steeper (the moon is high); they do not crawl or shimmer when the body walks or the camera turns; their edges are soft; they end softly toward the far town, not with a line; the shadow of the traveller follows the traveller |
| E-23 | The lamps | in B walk the sea stair and the lower street; E in A sets the lantern down, Tab, walk 6 m away, E calls it back | the lit lamps of the path are warm pools about 14 m across with a soft glow round each, in A all lamps are lit and weaker; the lantern lights the stone round the traveller and flickers; set down it stays with its circle; no lamp blinks on or off as the body walks; a bell rung lights its lamp on the mast and the tower under it |
| E-24 | The exposure | stand with the sun of A in the picture, then in the light of a lamp of B; edit `[lighting]` (`exposure`, `knee`, `lamps_b`, `shadows = false`) and press F5 | no wall is a flat white patch; the pool of a lamp keeps its hue; the edits take effect at once and the game goes on; note the frame time on the laptop with and without shadows (the shadow pass draws the town a second time) |
| E-25 | The style | start in B and look along the quay, Tab to A and look at the sea wall, then turn about 130 degrees toward the sun; walk to the lower town in A and in B; press L until the title bar says light plain, and back to full; edit `[lighting]` (`style`, `shafts`, `clouds_a`, `wash`, `grade`) and press F5 | in full the sun falls on the faces in soft bands, the shade side is bluer than the lit side, the traveller has a bright edge on the sun side, slow soft patches of cloud shadow cross the quay (the wall and the street below are under the same cloud) and the stone has soft blotches and, near the water, a darker, cooler foot; the lamps show soft rings with a whiter core; toward the sun rays of light show between the buildings and in the sky; the distance is lavender in A and teal in B; the corners are a little dark; in plain the same view is flat and warm or flat and cold; every landing, edge and interaction reads in both; no flicker or grain in the rays when the camera turns; the edits take effect at once; note the frame time on the laptop and the desktop with `shafts = false` and with `shaft_steps = 8` |
| E-26 | The traveller and the lantern | start in B and look at the traveller from the front (turn the camera), from the side and from behind; walk, sprint (Shift) and stop; jump; press H; Tab to A; E sets the lantern down, look at it, E takes it up | the traveller is a rounded figure with a round hood and peak, a scarf, a bell of a coat, boots and a small pack with a bedroll, 1.72 m high; the feet are on the floor in every step and the body bobs a little; the knee of the leg that swings forward bends; the scarf and the coat vent stream behind at a run; the lantern hangs from the ring in the right hand, sways with the stride and never touches the floor; with H the arm raises it; its glass glows, the flame inside wavers, the glow round it is soft and does not cut into the walls of a street; the light on the stone is amber and flickers and moves with the sway; set down, it hangs from the arm of a small iron post with the glass at the centre of the circle, and is taken up again; the traveller never disappears (the table of objects has room: 512) |

Only when the device recovery is touched: `[debug] device_reset_key = 1`, F5, press F9 during a ripple. The picture goes on
after a short flicker and the log says `recovered (1 of at most 5)`.

## Town review cameras

Run `python tools/layout/review_town.py` from the project root (Python 3, NumPy and Matplotlib). The tool writes matched A/B PNGs under `build/town-review/`, without replacing `data/town.dlt`. Use `--shot 05_north` for just the proposed tower approach or `--out` for a different output directory.

The market and north shots approximate the default orbit camera. The silhouette is a 1.7 m scale marker, not the player asset. The offline renderer does not reproduce camera collision, dynamic lighting or gameplay. Orange plan lines connect route anchors; the gold dashed approach is a proposal, not geometry or a verified path.
