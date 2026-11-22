# DualLayer

> 日本語：赤と青、重なり合う二つの世界を切り替えて進む 3D アクションパズル。ゲームエンジンを使わず C++20 / Direct3D 11 で制作中。
>
> English: a 3D action puzzle about two overlapping worlds, red and blue, built without a game engine on C++20 / Direct3D 11.

Status: in development (milestone M2). The whole map can be walked from the start to the top of the tower; the hub, the
three wings and the tower stand as grey boxes, and their rooms are being built from the design in
[docs/LEVEL_DESIGN.md](docs/LEVEL_DESIGN.md).

## Highlights

- **One rule, many uses.** The only ability is switching between layer A (red) and layer B (blue). A wall of one layer is
  air in the other; a jump can start in one layer and land in the other; a block that exists in both layers is stopped by
  the walls of both.
- **Puzzles proved by a solver.** A breadth-first solver proves that the courtyard puzzle takes 10 pushes, cannot be solved
  in one layer alone, and looks like 2 pushes to a player who ignores the other layer. Its layout was chosen by a search with
  the same solver, and the walls in the game are generated at compile time from the same rows, so the proof and the game
  cannot drift apart.
- **A map that is walked and searched by its tests.** A test drives the real walking body through the whole map: every
  room, every shortcut, every ramp, the tower to its top. Another one searches it for skips: from about 4600 places to
  stand, about 74000 jumps in every direction, with and without a switch of layers in the air. Its first runs found five
  ways round the map that no play-test had found.
- **Layer switch as a ripple.** Both layers are rendered into their own off-screen pictures and a circle opens from the
  centre of the screen; pressed again, it turns round without a jump.
- **Hints without words.** Every object of one layer leaves a thin outline in the other; a refused push outlines what was in
  the way; a beacon over the goal is seen from the first step.
- **Hand-written engine layer.** Direct3D 11 device and swap chain, depth, constant buffers, HLSL compiled at run time,
  procedural meshes with winding tests, collision of boxes, spheres, cylinders and tori, a walking body with tuned feel
  (acceleration, variable jump height, coyote time, jump buffer, ground snap on ramps), recovery from a lost GPU device.
- **One command checks everything.** `tools/check.ps1` runs the logic tests, a full Debug and Release rebuild (and proves the
  exe under test is the one just built), a scripted game run and a repository safety check, and commits only when all of it
  passed.

## Demo

A video and a GIF will be added when the first wing is finished.

## Role

Single developer: game design, level design, code, tests and documentation.

## Build and run

Open `DualLayer.slnx` in Visual Studio 2026, choose `Debug | x64` and press F5. Click into the window for mouse look.
Requirements, keys, the check command and the configuration: [docs/BUILD.md](docs/BUILD.md).

## Documents

| Document | Contents |
|---|---|
| [docs/BUILD.md](docs/BUILD.md) | Requirements, build, keys, configuration, the one-command check, what is checked by eye |
| [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) | One frame, the modules, layers and the ripple, collision and the walking body, the puzzle, the map, hints |
| [docs/LEVEL_DESIGN.md](docs/LEVEL_DESIGN.md) | The design of the map: goals, objects, rooms, proofs, build order and risks |
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
