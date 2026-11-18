# DualLayer (working title)

> 日本語：赤と青、二つの現実を切り替えながら進む3Dアクションパズル（制作中・M2）
>
> English: a 3D action-puzzle about two overlapping realities, red and blue (work in progress, milestone M2).

Self-made engine layer on C++20 / Direct3D 11. No game engine.

Status: milestone M2 in progress. Builds and runs on Windows; the two layers and the ripple that switches between them work.

## Highlights

- Direct3D 11 renderer written directly against the API: device and adapter choice, depth buffer, constant buffers, off-screen render targets, HLSL shaders compiled at run time.
- Shapes (cube, sphere, cylinder, torus) generated in code, with unit tests that check every triangle's winding and normal.
- Two layers, A and B, in one scene: each object carries a layer mask, and each layer has its own light, sky and background.
- `Tab` opens a ripple from the centre of the screen: both layers are drawn into their own pictures and a circle shows the new layer inside and the old one outside. Pressed again during the ripple, the circle turns round without a jump.
- Recovery from a lost graphics device: all GPU objects are rebuilt while the camera and the layer state stay as they were.
- Looks and speeds are read from `config/default.ini` and reloaded with F5, without recompiling.

## Demo

A demo video has not been recorded yet.

## Role

Single developer: design, code, documentation and tests.

## Build and run

Open `DualLayer.slnx` in Visual Studio 2026, choose `Debug | x64` and press F5. Details: [docs/BUILD.md](docs/BUILD.md).

## Documents

| Document | Contents |
|---|---|
| [docs/BUILD.md](docs/BUILD.md) | Requirements, build and run, keys, quick check |
| [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) | One frame step by step, layers and the ripple, modules and their dependencies, debugging facilities |
| [docs/CONVENTIONS.md](docs/CONVENTIONS.md) | Coordinate system, matrices, colour, depth, layers, naming |
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
