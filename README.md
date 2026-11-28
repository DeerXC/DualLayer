# DualLayer

日本語：同じ港町の「過去」（大嵐の前夜、祭りの最後の夕暮れ）と「現在」（その12年後）を切り替えて探索する、戦闘のない 3D 探索パズル。過去（A）で行ったことは、12年という時間が何を残したかとして現在（B）に現れ、その逆は起きません。種を植えれば木になり、野ざらしの木材は朽ち、開けたままの鉄の門は錆びて開いたままになります。過去に置いたランタンは、現在の廃墟に小さな「過去の一角」を残します。C++20 と Direct3D 11 で、ゲームエンジンを使わずに制作しています。

DualLayer is a 3D exploration puzzle without combat, set in two versions of one harbour town: A, the last calm evening before a great storm, and B, the same town twelve years later. What is done in the past is what twelve years make of it in the present: a seed planted in A is a tree in B, raw timber rots, an iron gate stays as it was left. Nothing done in the present changes the past. A lantern set down in A keeps a small piece of A standing in B. It is written in C++20 and Direct3D 11 without a game engine.

**Status:** playable prototype (milestone `m2-layers`, in progress). The story, the three-district structure and the finale at the tower are specified in [docs/DESIGN.md](docs/DESIGN.md) and are not built yet. Known issues are listed in [CHANGELOG.md](CHANGELOG.md).

**Demo video:** not yet published.

## Highlights

- **One rule, two worlds.** The only mechanic is switching between A and B. Opening a gate or pushing a crate in A changes what is found in B; the reverse never happens. Every puzzle is built on this single rule.
- **Own engine code.** Rendering, collision, the walking body, shadows, point lights, the map and device-loss recovery are written for this project on Direct3D 11 and DirectXMath. No game engine and no other third-party library.
- **Hand-painted light.** The sun of A and the moon of B cast shadows. Lamps are point lights. Sun in soft bands, cool shadows, drifting cloud shadows and light shafts give a painted look instead of a photographic one. One key shows the plain light for comparison.
- **Tests without a GPU.** Twenty-one logic tests run without a window, on Windows with MSVC and on any system with g++ or clang++ (`python tools/run_tests.py`). The shader maths is plain C++-compatible HLSL, so the tests run it on the CPU. A route solver plays the main path step by step and reports any way round a puzzle. One command (`tools/check.ps1`) runs the tests, a full Debug and Release build, a short play of the game and a repository check.

## Build and run

1. Open `DualLayer.slnx` in Visual Studio 2026 (workload: Desktop development with C++, toolset v145, Windows SDK 10.0.26100.0).
2. Choose `Debug | x64` and press F5. Click into the window for mouse look.

Requirements, keys, configuration and the check command are in [docs/BUILD.md](docs/BUILD.md).

## Tested on

| Item | Value |
|---|---|
| CPU | AMD Ryzen 7 8840HX |
| Memory | 15.2 GB |
| GPU | NVIDIA GeForce RTX 5060 Laptop GPU (driver 32.0.16.1742); AMD Radeon 610M is also present |
| Display | 2560 x 1600, 150% scaling |
| OS | Windows 11 Home |
| Toolchain | Visual Studio Community 2026 (18.10), toolset v145, Windows SDK 10.0.26100.0 |

## Where to start reading

| Topic | Files |
|---|---|
| The causal rule | `src/scene/Things.h`, `src/scene/Lantern.h` |
| Collision and the walking body | `src/scene/TriMesh.h`, `src/scene/Moments.h`, `src/scene/Player.h` |
| Light and shadows | `shaders/lighting.hlsli`, `src/render/ShadowMath.h`, `src/render/Lights.h` |
| Route proof | `tests/test_route.cpp` |
| Town generator (Python) | `tools/layout/city_model.py` |

## Documents

| Document | Contents |
|---|---|
| [docs/DESIGN.md](docs/DESIGN.md) | The game: story, principles, the switch, the causal rule, favours, the finale, art, the map, puzzles, guidance, limits and proof |
| [docs/BUILD.md](docs/BUILD.md) | Requirements, build, keys, configuration, the one-command check |
| [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) | The code as built: one frame, modules, the town, layers, collision, light |
| [docs/CONVENTIONS.md](docs/CONVENTIONS.md) | Coordinate system, matrices, colour, depth, layers, naming |
| [CHANGELOG.md](CHANGELOG.md) | Milestone records and known issues |

## Role

Single developer: game design, level design, code, tests and documentation.

## License

No license has been chosen yet. Until then the repository has no LICENSE file, which means all rights reserved: the code may be read but not reused.
