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

`Esc` quit, `F5` reload `config/default.ini`. `W A S D` / `Q E` move, arrow keys turn, `R` puts the camera back. `Z` wireframe on / off, `X` the next level of detail (8, 16, 32, 64 segments), `C` the shapes stop / turn. `Tab` goes to the other layer with a ripple from the centre of the screen (A: warm light, red cube and grey cylinder; B: cool light, blue sphere and yellow torus; the floor is in both); `Tab` again during the ripple turns it round. `F9` (only with `[debug] device_reset_key = 1`) builds the GPU objects again: the test of the device recovery. Command line: `--adapter N` (see the adapter list in `logs/dual.log`).

## Optional keys in `config/default.ini`

The program has built-in values for all of them, so the file may leave them out.

| Section | Keys | Meaning |
|---|---|---|
| `[models]` | `segments = 32` | how round the sphere, cylinder and torus are |
| `[test_triangle]` | `show = 0` | show the M0 test triangle |
| `[debug]` | `device_reset_key = 0` | 1: the `F9` key tests the recovery from a lost device |
| `[ripple]` | `seconds = 0.9` | time for the circle to cover the screen; 0 is a cut without a ripple |
| | `width = 0.07` | width of the bright ring, in screen heights (0.005 .. 0.5) |
| | `distortion = 0.012` | how far the ring bends the picture (0 .. 0.1; 0 is flat) |
| | `center_x`, `center_y` = 0.5 | where the circle opens, 0 .. 1 over the screen, (0, 0) = top left |
| `[layer_a]`, `[layer_b]` | `clear_r/g/b` | background colour (linear) |
| | `light_r/g/b` | colour of the light (linear) |
| | `light_x/y/z` | direction from a surface to the light (any length except 0) |
| | `sky_r/g/b` | sky light on a surface that looks up (linear); the ground light is derived from it |

`[render] clear_r/g/b` and `[layers] fade_seconds` are no longer read: the background belongs to the layer, and the ripple replaced the fade.

## Quick check (about 3 minutes)

Do it in Debug after a change. The long list of M0, M1 and the first M2 checks passed on 2026-11-17 (see the CHANGELOG); this list is what to repeat. Write a result as "Q-2 ok" or "Q-2 fails: what was seen".

| No. | Do | Expected |
|---|---|---|
| Q-1 | F5 | the title bar shows the **NVIDIA RTX 5060** and `layer A`; the red cube and the grey cylinder stand on the checkered floor; the output window has no D3D11 / DXGI warning or error |
| Q-2 | `Tab` once | a ring opens from the centre of the screen and spreads in about one second; inside it the blue sphere and the yellow torus, in cool light; the ring bends the picture a little and glows; at the end the title bar says `layer B` and nothing flickers |
| Q-3 | `Tab` again | the same, from the centre, back to `layer A` |
| Q-4 | `Tab`, then `Tab` again after about 0.3 s | the circle stops and closes again, without a jump; the end is layer A |
| Q-5 | `Tab` and resize the window during the ripple, then `Tab` again | no crash; the ripple fills the new size; the output window has no ERROR |
| Q-6 | `W A S D`, `Z`, `X`, `C` while a ripple runs | the camera, the wireframe, the detail and the spin all work inside and outside the circle |
| Q-7 | Edit `[ripple]` `seconds` to `0` and then `3` in the config, F5 | 0 is a cut, 3 is a slow ripple |
| Q-8 | Close the window in Debug | the live-object report lists only the device (no `Models.*`) |
| Q-9 | Build `Release \| x64` | no error and no warning that stops the build |

Only when the device recovery has been touched: put `[debug]` `device_reset_key = 1` in the config, F5, press `F9` during a ripple. The picture goes on after a short flicker, the log says `recovered (1 of at most 5)`, and `Tab` still works.
