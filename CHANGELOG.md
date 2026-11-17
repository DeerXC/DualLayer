# Changelog

## m1-models (in progress, 2026-11-15)

- Made: a depth buffer in `Device` (D32_FLOAT, cleared to 1.0, made again on resize); `render/MeshData` (cube, sphere, cylinder, torus on the CPU, no Windows in it) with `tests/test_mesh.cpp`; `render/Models` (vertex / index buffers, one shader `models.hlsl`, one directional light, a checkered floor, wireframe / level-of-detail / spin keys); `render/Projection.h` (one lens for every pass, `Triangle` uses it too); the M0 triangle is hidden by default (`[test_triangle] show`).
- Why: the M1 step "depth + models + light" (draw a cube and at least two more models); the shapes are made in code first, loading files comes with the exporter (M1 step 4).
- Next: first run on the laptop (docs/BUILD.md, M1 rows); `docs/understanding/M1-models.md`; the OBJ / glTF loader; a decision record on Reverse-Z.
- Risk: the M1 code has not been compiled on Windows yet (see KI-03).

## m0-project-skeleton (in progress, 2026-11-10)

- Made: repository files, hand-written VS 2026 project (SDK pinned), Log / Assert / Config with tests, window with DPI awareness, D3D11 device with adapter list and debug layer, GPU names and events, flip-model swap chain with an sRGB view, test triangle.
- Next: first build and run on the laptop (docs/BUILD.md checks), fill in the M0 understanding note.
- Risk: CI is unverified (see KI-05).

## Known issues

| ID | Symptom | Reproduce | Cause (if known) | Impact | Plan |
|---|---|---|---|---|---|
| KI-01 | Shaders are compiled at run time from `shaders/` | start the exe from a folder without `shaders/` | by design for M0 (hot reload comes in a later milestone) | log error, no triangle | precompile for release builds before delivery |
| KI-02 | A lost device ends the program | unplug/disable the GPU while running | recovery not written yet | exit | later milestone |
| KI-03 | The Release build treats warnings as errors; the M1 files have not been compiled with MSVC yet | build `Release \| x64` | `DualLayer.vcxproj` (Release: `TreatWarningAsError`) | a warning that Debug does not show stops the Release build | read the first warning and fix it; if it is not worth it, note it here |
| KI-04 | Reverse-Z is undecided: depth is the plain `D32_FLOAT`, LESS, clear 1.0 | - | not tested yet | far objects can z-fight at long distance | test it and record the decision |
| KI-05 | The build workflow has not run on a hosted runner yet | run the workflow manually | the hosted runner may not have Visual Studio 2026 | CI result unknown | run it once and record the result |
| KI-06 | The GPU used by default on the dual-GPU laptop is not confirmed | read the adapter list in `logs/dual.log` | Windows may default new programs to the integrated GPU | performance numbers could come from the wrong GPU | enumerate the adapters with DXGI and confirm |
