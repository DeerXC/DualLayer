# Build

## Needed on every development machine

1. Visual Studio 2026 with the workload **Desktop development with C++**, toolset v145.
2. **Windows SDK 10.0.26100.0** (the project pins this version).
3. For the Debug build's GPU debug layer: Windows **Settings > System > Optional features > Graphics Tools**. Without it the game still runs; the log says "debug layer is not available".

## Build and run

1. Open `DualLayer.slnx` (or the `.vcxproj`), choose `Debug | x64`, press **F5**. The debugger's working directory is the project folder, so `shaders/` and `config/` are found.
2. `build\x64\<Config>\DualLayer.exe` also runs when started from Explorer: a post-build step copies `shaders\` and `config\` next to it.
3. Logic tests: open a *Developer PowerShell for VS* in the repository folder and run `.\tests\run_tests.ps1` (Config / Log / Assert, the camera, the shapes of the models).

## Keys

`Esc` quit, `F5` reload `config/default.ini`. `W A S D` / `Q E` move, arrow keys turn, `R` puts the camera back. `Z` wireframe on / off, `X` the next level of detail (8, 16, 32, 64 segments), `C` the shapes stop / turn. `Tab` the other layer (A: warm light, red cube and grey cylinder; B: cool light, blue sphere and yellow torus; the floor is in both). `F9` (only with `[debug] device_reset_key = 1`) builds the GPU objects again: the test of the device recovery. Command line: `--adapter N` (see the adapter list in `logs/dual.log`).

## Optional keys in `config/default.ini`

The program has built-in values for all of them, so the file may leave them out.

| Section | Keys | Meaning |
|---|---|---|
| `[models]` | `segments = 32` | how round the sphere, cylinder and torus are |
| `[test_triangle]` | `show = 0` | show the M0 test triangle |
| `[debug]` | `device_reset_key = 0` | 1: the `F9` key tests the recovery from a lost device |
| `[layers]` | `fade_seconds = 0.35` | time of the A <-> B change; 0 is a cut |
| `[layer_a]`, `[layer_b]` | `clear_r/g/b` | background colour (linear) |
| | `light_r/g/b` | colour of the light (linear) |
| | `light_x/y/z` | direction from a surface to the light (any length except 0) |
| | `sky_r/g/b` | sky light on a surface that looks up (linear); the ground light is derived from it |

Since M2 the background colour belongs to the layer, so `[render] clear_r/g/b` are no longer read.

## Acceptance checks

Each check has a number, so a result can be written down as "M2-3 ok" or "M1-2 fails: what was seen". Everything is done in the Debug build unless it says otherwise. The results are recorded in the CHANGELOG, not here.

| No. | Check | Expected |
|---|---|---|
| M0-1 | A window opens on the 150% laptop screen | sharp, not blurry |
| M0-2 | Title bar | shows the GPU name, the size and the FPS; on the laptop **NVIDIA RTX 5060**, not the AMD 610M |
| M0-3 | `logs/dual.log` | lists both adapters and says which one is used |
| M0-4 | The test triangle: put `[test_triangle]` `show = 1` in `config/default.ini` and press F5 | **red on the right, green on top, blue on the left** (the conventions are right) |
| M0-5 | Resize the window, minimise and restore it | no crash, the picture follows |
| M0-6 | Edit `clear_b` in `[layer_b]` of `config/default.ini`, save, press F5 | the background of layer B changes without recompiling |
| M0-7 | Close the window in Debug | the output window shows a live-object report that lists only the device |
| M0-8 | Output window while running | no D3D11 / DXGI warnings or errors |
| M1-1 | The scene at the start position | on layer A the red cube and the grey cylinder stand on a checkered floor; after `Tab` the blue sphere and the yellow torus stand there |
| M1-2 | Depth: walk round the shapes (W A S D) | a shape in front hides the floor and the shapes behind it; nothing flickers where a shape touches the floor |
| M1-3 | `Z` | wireframe on / off; on layer A the cube has 12 triangles, on layer B the sphere has hundreds |
| M1-4 | `X` on layer B | the sphere goes 8 > 16 > 32 > 64 segments: its outline gets round and the highlight smooth; the title bar says how many triangles the visible layer has |
| M1-5 | `C` | the shapes stop / turn |
| M1-6 | Resize | the picture follows and the depth keeps working (no ERROR in the output window) |
| M1-7 | Close in Debug | the live-object report lists only the device (no `Models.*`, no `Frame.Depth`) |
| M2-1 | `Tab` | the title bar ends with `layer A` / `layer B`; the light, the shadows and the background change colour in about a third of a second; the shapes swap half way through |
| M2-2 | The floor | it stays in both layers and takes the colour of the light |
| M2-3 | `Alt` + `Tab` | switches windows only; the layer does not change |
| M2-4 | Press `Tab` twice quickly | the change turns round in the middle, no jump |
| M2-5 | `fade_seconds` in `[layers]`, then F5 | 0 is a cut, 2 is a slow fade |
| K-1 | Known issue KI-05: read the title bar and the first lines of `logs/dual.log` | the used adapter is the NVIDIA RTX 5060 |
| K-2 | Known issue KI-02: put `[debug]` `device_reset_key = 1` in the config, press F5, then `F9` | the picture goes on after a short flicker; the log says `recovered (1 of at most 5)`; the layer and the camera are unchanged |
| K-3 | After K-2: press `Tab`, close the window in Debug | the layer still switches; the live-object report lists only the device |
| K-4 | Known issue KI-03: build `Release \| x64` | no error, no warning that stops the build |
