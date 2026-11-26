# DualLayer

日本語：同じ港町の「過去」と「現在」を切り替え、過去に置いたランタンで現在に道を残す、戦闘のない 3D 探索パズル。C++20 / Direct3D 11 で制作中。現行版は三人称の移動（走り、よじ登り、ランタンの持ち運び）を試作済み。

DualLayer is a 3D exploration puzzle without combat, set in two versions of one harbour town. Actions in the past leave traces in the present. A lantern placed in the past keeps a small piece of it standing in the present. The project uses C++20 and Direct3D 11 without a game engine.

**M2 prototype:** the current build loads the model-based town and supports world switching, causal objects, the lantern patch, a first pass of third-person movement (sprint, ledge climb, a visible lantern) and help that is part of the world: three dark lamps on a signal mast over the bell tower (the goal, lit by the bells), a lantern whose flame leans toward the next thing to do (H asks for more), and a map on two sheets of tracing paper, the past over the present (M). The next version is specified in [Design](docs/DESIGN.md); its compact town and bell-tower finale are not presented as implemented.

## Highlights

- **One-way cause and effect.** Opening a gate or moving a crate in A changes what is found in B. The lantern provides a local exception to the appearance of the ruined town, not a reverse change to the past.
- **One town, two surfaces.** Town data drives both view and collision, including the lantern patch and the water check. The ripple draws the two layers separately before compositing them.
- **Route testing.** The main path is data (`steps` in the town file, written by the layout model). A reachability solver (`tests/test_route.cpp`) plays those steps in order and reports any way round a puzzle; the game guides along the same steps (`scene/Guidance.h`: the flame of the lantern leans toward the next one). Further logic tests cover the lantern, causal objects, the walking body, the guidance, the map, and the first minute: rays cast from the first frame prove that the goal is in sight for every lens and screen. All of them run without a window or a GPU.
- **A town lit by sun, moon and lamps.** A shadow map of the sun of A and the moon of B that follows the camera on its texel grid; every lit lantern of the model is a point light (the 32 nearest and strongest are chosen every frame); the lantern of the traveller and the lamps that the bells light answer what the player did; a soft shoulder keeps a bright lamp from cutting a wall to white. The maths of the lights runs as C++ in the tests without a GPU.
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
| [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) | The code as built: one frame, the modules, the town, layers and the ripple, collision and the walking body, water, the overlay |
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
