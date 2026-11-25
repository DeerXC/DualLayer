# DualLayer

日本語：同じ港町の「過去」と「現在」を切り替え、過去に置いたランタンで現在に道を残す、戦闘のない 3D 探索パズル。C++20 / Direct3D 11 で制作中。現行版は一人称で、次の版では三人称操作を試作する。

DualLayer is a 3D exploration puzzle without combat, set in two versions of one harbour town. Actions in the past leave traces in the present. A lantern placed in the past keeps a small piece of it standing in the present. The project uses C++20 and Direct3D 11 without a game engine.

**M2 prototype:** the current first-person build loads the model-based town and supports world switching, causal objects and the lantern patch. The next version is specified in [Design](docs/DESIGN.md); its third-person movement, compact town and bell-tower finale are not presented as implemented. The former greybox remains available with `[map] town = false`.

## Highlights

- **One-way cause and effect.** Opening a gate or moving a crate in A changes what is found in B. The lantern provides a local exception to the appearance of the ruined town, not a reverse change to the past.
- **One town, two surfaces.** Town data drives both view and collision, including the lantern patch and the water check. The ripple draws the two layers separately before compositing them.
- **Route testing.** The uploaded baseline includes a reachability solver for the model-based route and tests for lantern placement, causal objects and collision. Older greybox solver results apply to that prototype only.
- **Custom rendering and movement.** Static batching, procedural meshes, water shading, collision, variable-height jumping and device recovery are implemented without a game engine.

## Demo

Demo video: not yet published.

## Role

Single developer: game design, level design, code, tests and documentation.

## Build and run

Open `DualLayer.slnx` in Visual Studio 2026, choose `Debug | x64` and press F5. Click into the window for mouse look.
Requirements, keys, the check command and the configuration: [docs/BUILD.md](docs/BUILD.md).

## Documents

| Document | Contents |
|---|---|
| [docs/DESIGN.md](docs/DESIGN.md) | The game: principles, the switch, the lantern and the causal rule, art, the map, puzzles, limits and proof, order of work |
| [docs/BUILD.md](docs/BUILD.md) | Requirements, build, keys, configuration, the one-command check, what is checked by eye |
| [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) | The code as built: one frame, the modules, layers and the ripple, collision and the walking body, the puzzle, the map, water |
| [docs/CONVENTIONS.md](docs/CONVENTIONS.md) | Coordinate system, matrices, colour, depth, layers, naming |
| [CHANGELOG.md](CHANGELOG.md) | Milestone records and known issues |

## Tested on

| Item | Value |
|---|---|
| CPU | AMD Ryzen 7 8840HX |
| Memory | 15.2 GB |
| GPU | NVIDIA GeForce RTX 5060 Laptop GPU (driver 32.0.16.1742); AMD Radeon 610M is also present |
| Display | 2560 x 1600, 150% scaling |
| OS | Windows 11 Home |
| Toolchain | Visual Studio Community 2026 (18.10), toolset v145, Windows SDK 10.0.26100.0 |

## License

Not decided yet. Until then the repository has no LICENSE file, which means all rights reserved: the code may be read but
not reused.
