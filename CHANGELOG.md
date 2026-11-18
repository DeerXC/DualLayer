# Changelog

## m2-layers (in progress: layers and ripple accepted, 2026-11-18)

- Made (2026-11-17): a layer mask on every scene object (A = 1, B = 2, both = 3) and the `Tab` key; a theme per layer (background, light colour, light direction, sky colour) with built-in values and optional `[layer_a]` / `[layer_b]` keys in the config; `models.hlsl` and `FrameConstants` carry the sky colours (144 bytes); the title bar and the triangle count follow the layer. The first switch was a colour fade.
- Fixed (2026-11-17): the first build stopped at `std::min` / `std::max` in `Models::Update` (C2589): `windows.h` defines `min` and `max` as macros when `NOMINMAX` is seen too late (the build was `Release | x64` and showed 0 warnings). See docs/CONVENTIONS.md, "windows.h".
- Made (2026-11-17, known-issue pass): a lost device is handled: `Gpu` in `main.cpp` shuts device, triangle and models down and builds them again on a new `Device` (up to 5 times, then a message and exit); the `F9` key (`[debug] device_reset_key = 1`) runs the same path as a test; `NvOptimusEnablement` and `AmdPowerXpressRequestHighPerformance` are exported.
- Checked (2026-11-17): all numbered checks of M0, M1, M2 and K-1 .. K-4 passed on the laptop (RTX 5060, Debug; K-4 is the Release build).
- Made (2026-11-18): the ripple replaces the fade. `Tab` opens a circle from the centre of the screen: both layers are drawn complete into two off-screen pictures (sRGB colour + their own depth), and `shaders/ripple.hlsl` draws one screen-covering triangle that takes the new layer inside the circle and the old one outside, with a glowing ring that bends the picture a little. While no ripple runs, one layer is drawn straight to the back buffer as before. `Tab` during a ripple turns the circle round: no jump. `LayerRipple.h` holds the timing without any Direct3D. `StateGuard` now also puts back the render target, the viewport, the shader resources and the sampler.
- Changed (2026-11-18): themes are no longer mixed: a layer is always drawn with its own theme, and the background colour is the one of the layer on the screen. `[layers] fade_seconds` is replaced by `[ripple]` (`seconds`, `width`, `distortion`, `center_x`, `center_y`). `docs/BUILD.md` now has a 9-line quick check instead of the long list.
- Why: the M2 step "two layers" is the show piece of the work: the other world should open up where the player looks, not fade in.
- Checked (2026-11-18): the timing logic of `LayerRipple` was compiled and run on its own (idle, open, turn round, cut, time changed during a ripple). The full build and the quick check Q-1 .. Q-9 of docs/BUILD.md passed on the laptop (RTX 5060): ripple, turning it round, resize during a ripple, no debug-layer warning, no leak at exit, Release build.
- Not tested: the `LayerRipple` checks are not part of `tests/run_tests.ps1` yet; a real device loss during a ripple (only the `F9` path exists).
- Next: collision per layer.

## m1-models (built and run together with M2, 2026-11-17)

- Made: a depth buffer in `Device` (D32_FLOAT, cleared to 1.0, made again on resize); `render/MeshData` (cube, sphere, cylinder, torus on the CPU, no Windows in it) with `tests/test_mesh.cpp`; `render/Models` (vertex / index buffers, one shader `models.hlsl`, one directional light, a checkered floor, wireframe / level-of-detail / spin keys); `render/Projection.h` (one lens for every pass, `Triangle` uses it too); the M0 triangle is hidden by default (`[test_triangle] show`).
- Why: the M1 step "depth + models + light" (draw a cube and at least two more models); the shapes are made in code first, loading files comes with the exporter (M1 step 4).
- Checked: compiled and run on Windows together with the M2 code (the first compile happened with M2); checks M1-1 .. M1-7 passed on 2026-11-17.
- Next: the OBJ / glTF loader.

## m0-project-skeleton (in progress, 2026-11-10)

- Made: repository files, hand-written VS 2026 project (SDK pinned), Log / Assert / Config with tests, window with DPI awareness, D3D11 device with adapter list and debug layer, GPU names and events, flip-model swap chain with an sRGB view, test triangle.
- Checked: the project builds and runs on the laptop (first confirmed on 2026-11-17); checks M0-1 .. M0-8 passed.

## Known issues

| ID | Symptom | Reproduce | Cause (if known) | Impact | Plan |
|---|---|---|---|---|---|
| KI-01 | Shaders are compiled at run time from `shaders/` | start the exe from a folder without `shaders/` | by design (hot reload comes in a later milestone); the post-build step copies `shaders\` next to the exe | log error, no picture | accepted for now; precompile for the delivery build |

## Closed issues

| ID | Symptom | Closed | How |
|---|---|---|---|
| KI-03 | The Release build treats warnings as errors; the M1 and M2 files had not been compiled with MSVC | 2026-11-17 | `Release \| x64` was the configuration of the first real build; after the `min` / `max` fix it ran. The rule stays: a new file must build in Release (check Q-9) |
| KI-02 | A lost device ended the program | 2026-11-17 | `Gpu::Recover` in `main.cpp` builds device, triangle and models again (checks K-2, K-3 passed). Only the `F9` test path was run; a real device loss (driver reset, GPU switched off) has not been tried |
| KI-05 | The GPU used by default on the dual-GPU laptop was not confirmed | 2026-11-17 | the code picks the high-performance adapter and exports the two driver hints; check K-1 passed (NVIDIA RTX 5060) |
| KI-04 | Reverse-Z was undecided: depth is the plain `D32_FLOAT`, LESS, clear 1.0 | 2026-11-18 | decided to **keep** it: the scene is small (floor 24 m, near 0.1 m, far 100 m), no flicker has been seen, and the two ripple pictures use the same convention. Revisit only if far objects flicker (it would change `Projection.h`, `Device::BeginFrame`, `Models::CreateStates`, the ripple depth buffers and the depth state of `Triangle`) |
