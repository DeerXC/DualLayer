# Changelog

## m2-layers (in progress: the switch and the looks are accepted, 2026-11-17)

- Made: a layer mask on every scene object (A = 1, B = 2, both = 3) and the `Tab` key that switches the visible layer; a theme per layer (background, light colour, light direction, sky colour) with built-in values and optional `[layer_a]` / `[layer_b]` / `[layers]` keys in the config; a fade between the layers (default 0.35 s, smoothstep, the objects swap when it is half way); `models.hlsl` and `FrameConstants` now carry the sky colours (144 bytes); the title bar and the triangle count follow the visible layer.
- Changed: the background colour is chosen by the layer, so `[render] clear_r/g/b` are no longer read.
- Why: the M2 step "two layers": first the label and the switch, then the look that makes the two layers feel like two worlds.
- Fixed: the first build stopped at `std::min` / `std::max` in `Models::Update` (C2589): `windows.h` defines `min` and `max` as macros when `NOMINMAX` is seen too late. The fade now uses `std::clamp` (see docs/CONVENTIONS.md, "windows.h").
- Checked: the project builds and runs on Windows (RTX 5060 laptop), together with the M1 code that had not been compiled before (the build that stopped at C2589 was `Release | x64` and showed 0 warnings).
- Made (known-issue pass): a lost device is now handled: `Gpu` in `main.cpp` shuts device, triangle and models down and builds them again on a new `Device` (up to 5 times, then a message and exit); the layer, fade, spin, wireframe and the camera survive; the `F9` key (`[debug] device_reset_key = 1`) runs the same path as a test. The two driver hints `NvOptimusEnablement` and `AmdPowerXpressRequestHighPerformance` are exported. The acceptance checks in docs/BUILD.md now have numbers (M0-1 .. M2-5, K-1 .. K-4).
- Checked: all numbered checks of docs/BUILD.md passed on the laptop on 2026-11-17 (M0-1 .. M0-8, M1-1 .. M1-7, M2-1 .. M2-5, K-1 .. K-4), in Debug; K-4 is the Release build.
- Next: `docs/understanding/M2-layers.md`; the layers as two render targets and the ripple that reveals the other layer; collision per layer.

## m1-models (built and run together with M2, 2026-11-17)

- Made: a depth buffer in `Device` (D32_FLOAT, cleared to 1.0, made again on resize); `render/MeshData` (cube, sphere, cylinder, torus on the CPU, no Windows in it) with `tests/test_mesh.cpp`; `render/Models` (vertex / index buffers, one shader `models.hlsl`, one directional light, a checkered floor, wireframe / level-of-detail / spin keys); `render/Projection.h` (one lens for every pass, `Triangle` uses it too); the M0 triangle is hidden by default (`[test_triangle] show`).
- Why: the M1 step "depth + models + light" (draw a cube and at least two more models); the shapes are made in code first, loading files comes with the exporter (M1 step 4).
- Checked: compiled and run on Windows together with the M2 code (the first compile happened with M2); checks M1-1 .. M1-7 passed on 2026-11-17.
- Next: `docs/understanding/M1-models.md`; the OBJ / glTF loader; a decision record on Reverse-Z.

## m0-project-skeleton (in progress, 2026-11-10)

- Made: repository files, hand-written VS 2026 project (SDK pinned), Log / Assert / Config with tests, window with DPI awareness, D3D11 device with adapter list and debug layer, GPU names and events, flip-model swap chain with an sRGB view, test triangle.
- Checked: the project builds and runs on the laptop (first confirmed on 2026-11-17); checks M0-1 .. M0-8 passed.
- Next: fill in the M0 understanding note.

## Known issues

| ID | Symptom | Reproduce | Cause (if known) | Impact | Plan |
|---|---|---|---|---|---|
| KI-01 | Shaders are compiled at run time from `shaders/` | start the exe from a folder without `shaders/` | by design (hot reload comes in a later milestone); the post-build step copies `shaders\` next to the exe | log error, no picture | accepted for now; precompile for the delivery build |
| KI-04 | Reverse-Z is undecided: depth is the plain `D32_FLOAT`, LESS, clear 1.0 | - | not tested yet | far objects can z-fight at long distance (not seen in the present scene: floor 24 m, camera near 0.1 m / far 100 m) | decide together with the per-layer render targets (the next M2 step creates new depth buffers anyway); then change `Projection.h`, `Device::BeginFrame`, `Models::CreateStates` and the depth state of `Triangle` in one go |

## Closed issues

| ID | Symptom | Closed | How |
|---|---|---|---|
| KI-03 | The Release build treats warnings as errors; the M1 and M2 files had not been compiled with MSVC | 2026-11-17 | `Release \| x64` was the configuration of the first real build; after the `min` / `max` fix it ran. The rule stays: a new file must build in Release (check K-4) |
| KI-02 | A lost device ended the program | 2026-11-17 | `Gpu::Recover` in `main.cpp` builds device, triangle and models again (check K-2, K-3 passed). Only the `F9` test path was run; a real device loss (driver reset, GPU switched off) has not been tried |
| KI-05 | The GPU used by default on the dual-GPU laptop was not confirmed | 2026-11-17 | the code picks the high-performance adapter and exports the two driver hints; check K-1 passed (NVIDIA RTX 5060) |
