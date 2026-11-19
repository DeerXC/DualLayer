# DualLayer (working title)

> 日本語：赤と青、二つの現実を切り替えながら進む3Dアクションパズル（制作中・M2）
>
> English: a 3D action-puzzle about two overlapping realities, red and blue (work in progress, milestone M2).

Self-made engine layer on C++20 / Direct3D 11. No game engine.

Status: milestone M2 in progress. Builds and runs on Windows; the two layers, the ripple that switches between them and the collision per layer work. A walking body (gravity, jump, standing on objects) works as well.

## Highlights

- Direct3D 11 renderer written directly against the API: device and adapter choice, depth buffer, constant buffers, off-screen render targets, HLSL shaders compiled at run time.
- Shapes (cube, sphere, cylinder, torus) generated in code, with unit tests that check every triangle's winding and normal.
- Two layers, A and B, in one scene: each object carries a layer mask, and each layer has its own light, sky and background.
- `Tab` opens a ripple from the centre of the screen: both layers are drawn into their own pictures and a circle shows the new layer inside and the old one outside. Pressed again during the ripple, the circle turns round without a jump.
- Collision per layer: the camera is a ball that is pushed out of the solid objects of the visible layer (box, sphere, cylinder and torus, all exact; the hole of the torus can be flown through). An object is solid only in its own layers, and the renderer and the collision read one scene table, so what is drawn is what is solid.
- Recovery from a lost graphics device: all GPU objects are rebuilt while the camera and the layer state stay as they were.
- Walking: the camera is the eye of a body of three balls that falls, lands, jumps, stands on objects and slides off slopes that are too steep. Standing on an object and switching layers takes the ground away.
- Looks and speeds are read from `config/default.ini` and reloaded with F5, without recompiling.
- One command, `tools/check.ps1`, runs the logic tests, the Debug and Release builds, a scripted game run and a repository safety check.

## Demo

A demo video has not been recorded yet.

## Role

Single developer: design, code, documentation and tests.

## Build and run

Open `DualLayer.slnx` in Visual Studio 2026, choose `Debug | x64` and press F5. Details: [docs/BUILD.md](docs/BUILD.md).

## Documents

| Document | Contents |
|---|---|
| [docs/BUILD.md](docs/BUILD.md) | Requirements, build and run, the one-command check, keys, quick check |
| [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) | One frame step by step, layers, ripple, collision and walking, modules and their dependencies, debugging facilities |
| [docs/CONVENTIONS.md](docs/CONVENTIONS.md) | Coordinate system, matrices, colour, depth, layers, collision, walking, naming |
| [CHANGELOG.md](CHANGELOG.md) | Milestone records and known issues |

## Tested on

| Item | Value |
|---|---|
| CPU | AMD Ryzen 7 8840HX with Radeon Graphics |
| Memory | 15.2 GB |
| GPU | NVIDIA GeForce RTX 5060 Laptop GPU (driver 32.0.16.1742); AMD Radeon 610M is also present |
| Display | 2560 x 1600, 150% scaling |
| OS | Windows 11 Home |
| Toolchain | Visual Studio Community 2026 (18.10), toolset v145, Windows SDK 10.0.26100.0 |

Performance numbers will state which machine they were measured on.

## License

Not decided yet. Until then the repository has no LICENSE file, which means all rights reserved: the code may be read but not reused.
