# Build

## Needed on every development machine

1. Visual Studio 2026 with the workload **Desktop development with C++**, toolset v145.
2. **Windows SDK 10.0.26100.0** (the project pins this version).
3. For the Debug build's GPU debug layer: Windows **Settings > System > Optional features > Graphics Tools**. Without it the game still runs; the log says "debug layer is not available".

## Build and run

1. Open `DualLayer.slnx` (or the `.vcxproj`), choose `Debug | x64`, press **F5**. The debugger's working directory is the project folder, so `shaders/` and `config/` are found.
2. `build\x64\<Config>\DualLayer.exe` also runs when started from Explorer: a post-build step copies `shaders\` and `config\` next to it.
3. Logic tests: `.\tests\run_tests.ps1` in a *Developer PowerShell for VS* (Config / Log / Assert, the camera, the shapes of the models, the collision, the layer ripple). `tools\check.ps1` runs them for you (see "One command" below).

## One command

```powershell
.\tools\check.ps1
```

It can be started from any PowerShell and finds the project and the Visual Studio build tools by itself. It runs, in this order:

1. the logic tests (`tests\run_tests.ps1`);
2. the `Debug | x64` and the `Release | x64` build, and counts errors and warnings (Release treats warnings as errors);
3. the game: it starts, waits for the window, presses `Tab` and `N`, resizes the window during a ripple, closes it, and reads the title bar and `logs\dual.log` (no `[ERROR]` line, the ripple pictures were made, the game ended normally). This part presses keys for about 25 seconds: do not touch the keyboard or the mouse. If the game window is not in front, the keys are skipped and the table says so;
4. the repository: nothing that must not be uploaded (build output, logs, private local files, `config/local.ini`, files over 50 MB), and whether the hooks are on.

The last table says `OK`, `WARN`, `SKIP` or `FAIL` for every step; the exit code is 0 unless something failed. Useful switches: `-SkipBuild`, `-SkipSmoke`.

To check and, only when everything passed, commit (and push) in one go:

```powershell
.\tools\check.ps1 -Commit "feat(scene): add per-layer collision" -Push
```

The message must be `type(scope): subject`, at most 72 characters, without a final period; then `git add -A`, `git audit`, `git commit` and (with `-Push`) `git push` run one after the other and stop at the first failure. `--no-verify` is never used.

## Keys

`Esc` quit, `F5` reload `config/default.ini`. `W A S D` / `Q E` move, arrow keys turn, `R` puts the camera back. `Z` wireframe on / off, `X` the next level of detail (8, 16, 32, 64 segments), `C` the shapes stop / turn. `N` collision on / off (off: fly through everything). `Tab` goes to the other layer with a ripple from the centre of the screen (A: warm light, red cube and grey cylinder; B: cool light, blue sphere and yellow torus; the floor is in both); `Tab` again during the ripple turns it round. `F9` (only with `[debug] device_reset_key = 1`) builds the GPU objects again: the test of the device recovery. Command line: `--adapter N` (see the adapter list in `logs/dual.log`).

## Optional keys in `config/default.ini`

The program has built-in values for all of them, so the file may leave them out.

| Section | Keys | Meaning |
|---|---|---|
| `[models]` | `segments = 32` | how round the sphere, cylinder and torus are |
| `[test_triangle]` | `show = 0` | show the M0 test triangle |
| `[collision]` | `enabled = 1` | 1: the camera is a ball that cannot go into solid objects; 0: it flies through (the `N` key switches it while running) |
| | `radius = 0.35` | radius of the ball, in metres (0.05 .. 1) |
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

## Quick check

Do it in Debug after a change. The long list of M0, M1 and the first M2 checks passed on 2026-11-17 (see the CHANGELOG). The column "By" says who does a check: `script` = `tools\check.ps1` does it; `eye` = you look. Write a result as "Q-2 ok" or "Q-2 fails: what was seen".

| No. | By | Do | Expected |
|---|---|---|---|
| Q-1 | script, eye | F5 | the title bar shows the **NVIDIA RTX 5060** and `layer A`; the red cube and the grey cylinder stand on the checkered floor; the output window has no D3D11 / DXGI warning or error (the script checks the first two and the log; the output window is for the eye) |
| Q-2 | script, eye | `Tab` once | a ring opens from the centre of the screen and spreads in about one second; inside it the blue sphere and the yellow torus, in cool light; the ring bends the picture a little and glows; at the end the title bar says `layer B` and nothing flickers (the script checks the title bar and that the ripple pictures were made; the look is for the eye) |
| Q-3 | script | `Tab` again | the same, from the centre, back to `layer A` |
| Q-4 | script | `Tab`, then `Tab` again after about 0.3 s | the circle stops and closes again; the end is layer A (that it does so without a jump is for the eye) |
| Q-5 | script | `Tab` and resize the window during the ripple | no crash; the ripple fills the new size; no `[ERROR]` in the log |
| Q-6 | eye | `W A S D`, `Z`, `X`, `C` while a ripple runs | the camera, the wireframe, the detail and the spin all work inside and outside the circle |
| Q-7 | eye | Edit `[ripple]` `seconds` to `0` and then `3` in the config, F5 | 0 is a cut, 3 is a slow ripple |
| Q-8 | eye | Close the window in Debug | the live-object report in the output window lists only the device (no `Models.*`) |
| Q-9 | script | Build `Release \| x64` | no error and no warning that stops the build |
| Q-10 | eye | Layer A: fly into the red cube and the grey cylinder, then down onto the floor (`Q`) | the camera stops at their surface and cannot go below the floor; the title bar ends with `collide`; the shapes turn and push the camera gently aside |
| Q-11 | eye | `Tab`, then fly to where the red cube stood | in layer B the cube is not there and nothing stops you; the blue sphere and the yellow torus are solid now |
| Q-12 | eye | Layer B: fly through the hole of the torus (it is 1 m wide, the ball 0.7 m) | you can pass through the hole; the ring itself is solid |
| Q-13 | eye | `Tab` while the camera is inside the other layer's object | the camera is pushed out to its surface, no jump to a far place, no flicker |
| Q-14 | script | `N` | the title bar ends with `noclip` and you fly through everything; `N` again, the ball is solid again (that the ball really passes through is for the eye) |

What is left for the eye after a green `check.ps1`: the look of Q-2, Q-4 and Q-6 to Q-8, and Q-10 to Q-13 (about three minutes).

Only when the device recovery has been touched: put `[debug]` `device_reset_key = 1` in the config, F5, press `F9` during a ripple. The picture goes on after a short flicker, the log says `recovered (1 of at most 5)`, and `Tab` still works.
