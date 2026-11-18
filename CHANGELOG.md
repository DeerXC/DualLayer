# Changelog

## m2-layers (in progress: layers, ripple and collision accepted, 2026-11-18)

- Made (2026-11-17): a layer mask on every scene object (A = 1, B = 2, both = 3) and the `Tab` key; a theme per layer (background, light colour, light direction, sky colour) with built-in values and optional `[layer_a]` / `[layer_b]` keys in the config; `models.hlsl` and `FrameConstants` carry the sky colours (144 bytes); the title bar and the triangle count follow the layer. The first switch was a colour fade.
- Fixed (2026-11-17): the first build stopped at `std::min` / `std::max` in `Models::Update` (C2589): `windows.h` defines `min` and `max` as macros when `NOMINMAX` is seen too late (the build was `Release | x64` and showed 0 warnings). See docs/CONVENTIONS.md, "windows.h".
- Made (2026-11-17, known-issue pass): a lost device is handled: `Gpu` in `main.cpp` shuts device, triangle and models down and builds them again on a new `Device` (up to 5 times, then a message and exit); the `F9` key (`[debug] device_reset_key = 1`) runs the same path as a test; `NvOptimusEnablement` and `AmdPowerXpressRequestHighPerformance` are exported.
- Checked (2026-11-17): all numbered checks of M0, M1, M2 and K-1 .. K-4 passed on the laptop (RTX 5060, Debug; K-4 is the Release build).
- Made (2026-11-18): the ripple replaces the fade. `Tab` opens a circle from the centre of the screen: both layers are drawn complete into two off-screen pictures (sRGB colour + their own depth), and `shaders/ripple.hlsl` draws one screen-covering triangle that takes the new layer inside the circle and the old one outside, with a glowing ring that bends the picture a little. While no ripple runs, one layer is drawn straight to the back buffer as before. `Tab` during a ripple turns the circle round: no jump. `LayerRipple.h` holds the timing without any Direct3D. `StateGuard` now also puts back the render target, the viewport, the shader resources and the sampler.
- Changed (2026-11-18): themes are no longer mixed: a layer is always drawn with its own theme, and the background colour is the one of the layer on the screen. `[layers] fade_seconds` is replaced by `[ripple]` (`seconds`, `width`, `distortion`, `center_x`, `center_y`).
- Why: the M2 step "two layers" is the show piece of the work: the other world should open up where the player looks, not fade in.
- Checked (2026-11-18): the full build and the quick check Q-1 .. Q-9 passed on the laptop (RTX 5060): ripple, turning it round, resize during a ripple, no debug-layer warning, no leak at exit, Release build.
- Made (2026-11-18, collision per layer): the camera is a ball (radius 0.35 m) that is pushed out of the solid objects of the layer the screen is going to show. New `scene/SceneData.h`: the scene table moved out of `Models.cpp`, so the renderer and the collision read the same data; a `static_assert` stops the build when a sphere or torus is stretched unevenly or a cylinder is not round. New `scene/Collision.h` (header only, no Windows): exact box, sphere, cylinder and torus, in the local space of each object (the inverse of the world matrix of the renderer); the turning shapes use `Models::SpinSeconds()`, the time of the renderer. During a ripple the new layer is already solid (`Models::CollisionLayer()`). `main.cpp`: collision after the camera and the shapes have moved, `[collision] enabled` / `radius`, the `N` key (collide / noclip), `collide` / `noclip` in the title bar. New `tests/test_collision.cpp`.
- Why: the M2 step "layers: drawing and collision" of the plan; the other layer has to be a different place to move in, not only a different picture.
- Checked (2026-11-18): `tests/test_collision.cpp` passes with g++ (4578 checks, `-Wall -Wextra -Wconversion -Wshadow -Wpedantic`): DirectXMath rotation facts, the layer rule, every shape pushed out to exactly one radius, 4000 random points in both layers at many moments of the turning (never inside, measured in a second, independent way), a settled ball is not pushed again, a walk into the cube is stopped in A and free in B, bad input. Three deliberate bugs in the code (wrong turn sign, wrong order of tilt and turn, inverted layer rule) were each caught.
- Made (2026-11-18, checks in one command): `tools/check.ps1` runs the logic tests, the Debug and Release build, a game run that presses `Tab` and `N`, resizes the window during a ripple and reads the title bar and `logs/dual.log`, and the repository safety check; with `-Commit` (and `-Push`) it commits only when everything passed, through `git audit`. `tests/run_tests.ps1` now builds and runs `test_collision` and the new `tests/test_ripple.cpp` (the timing of the ripple: open, turn round, cut, monotonic opening; it passes with g++ and catches three deliberate bugs). `DualLayer.vcxproj` lists the new headers, `ripple.hlsl` and `tools/check.ps1`, and its first comment points to `docs/BUILD.md` (it still named a deleted file). The quick check in `docs/BUILD.md` now says which checks the script does and which are left for the eye.
- Checked (2026-11-18): `tools/check.ps1` ran on the laptop and everything passed: the logic tests built with MSVC and ran (core, camera, collision, ripple, mesh), `Debug | x64` and `Release | x64` built, the game run (Tab, Tab twice, resize during a ripple, N, a clean close, no `[ERROR]` in the log), and the repository check. The eye checks Q-10 .. Q-13 (collision per layer) were done and passed.
- Open: `/WX` is still off for `test_collision`, `test_ripple` and `test_mesh` in `tests/run_tests.ps1`; turn it on for them when a run shows no `warning C` line.
- Limits: the ball is still a free-fly camera, there is no gravity or walking yet; the floor is a finite slab (24 x 24 m), so beyond its edge there is nothing to stand on. A real device loss during a ripple has not been tried (only the `F9` path exists).
- Next: a walking player on the same `ResolveSphere` (gravity, ground contact, jump), then stage 1.

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
| KI-03 | The Release build treats warnings as errors; the M1 and M2 files had not been compiled with MSVC | 2026-11-17 | `Release \| x64` was the configuration of the first real build; after the `min` / `max` fix it ran. The rule stays: a new file must build in Release (check Q-9, done by `tools/check.ps1`) |
| KI-02 | A lost device ended the program | 2026-11-17 | `Gpu::Recover` in `main.cpp` builds device, triangle and models again (checks K-2, K-3 passed). Only the `F9` test path was run; a real device loss (driver reset, GPU switched off) has not been tried |
| KI-05 | The GPU used by default on the dual-GPU laptop was not confirmed | 2026-11-17 | the code picks the high-performance adapter and exports the two driver hints; check K-1 passed (NVIDIA RTX 5060) |
| KI-04 | Reverse-Z was undecided: depth is the plain `D32_FLOAT`, LESS, clear 1.0 | 2026-11-18 | decided to **keep** it: the scene is small (floor 24 m, near 0.1 m, far 100 m), no flicker has been seen, and the two ripple pictures use the same convention. Revisit only if far objects flicker (it would change `Projection.h`, `Device::BeginFrame`, `Models::CreateStates`, the ripple depth buffers and the depth state of `Triangle`) |
