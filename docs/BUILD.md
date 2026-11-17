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

`Esc` quit, `F5` reload `config/default.ini`. `W A S D` / `Q E` move, arrow keys turn, `R` puts the camera back. `Z` wireframe on / off, `X` the next level of
detail (8, 16, 32, 64 segments), `C` the shapes stop / turn. Command line: `--adapter N` (see the adapter list in `logs/dual.log`).
Optional keys in `config/default.ini` (the program has these defaults without them): `[models] segments = 32`, `[test_triangle] show = 0`.

## What to check on the first run (M0 acceptance)

| Check | Expected |
|---|---|
| A window opens, sharp (not blurry) on the 150% laptop screen | |
| Title bar shows the GPU name, the size and the FPS | on the laptop: **NVIDIA RTX 5060**, not the AMD 610M |
| The triangle: **red on the right, green on top, blue on the left** (since M1 it is hidden: put `[test_triangle]` `show = 1` in `config/default.ini` and press F5) | the conventions are right |
| `logs/dual.log` lists both adapters and says which one is used | |
| Resize the window, minimise and restore it | no crash, the picture follows |
| Edit `clear_b` in `config/default.ini`, save, press F5 | the background colour changes without recompiling |
| Close the window in Debug | the output window shows a live-object report that lists only the device |
| Output window | no D3D11 / DXGI warnings or errors |
| M1: the scene | four shapes (red cube, blue sphere, grey cylinder, yellow torus) stand on a checkered floor, in front of the camera at its start position |
| M1: depth | walk round them (W A S D): a shape in front hides the floor and the shapes behind it; nothing flickers where a shape touches the floor |
| M1: `Z` | wireframe on / off; the triangles show that the cube has 12, the sphere hundreds |
| M1: `X` | the sphere goes 8 > 16 > 32 > 64 segments: its outline gets round and the highlight smooth; the title bar says how many triangles there are |
| M1: `C` | the shapes stop / turn |
| M1: resize | the picture follows and the depth keeps working (the output window has no ERROR) |
| M1: close in Debug | the live-object report lists only the device (no `Models.*`, no `Frame.Depth`) |
