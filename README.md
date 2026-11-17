# DualLayer (working title)

> 日本語：赤と青、二つの現実を切り替えながら進む3Dアクションパズル（制作中・M1）
>
> English: a 3D action-puzzle about two overlapping realities, red and blue (work in progress, milestone M1).

Self-made engine layer on C++20 / Direct3D 11. No game engine.

Status: M1 in progress (depth buffer, four 3D models, one light).

## Documents

| Document | Contents |
|---|---|
| [docs/BUILD.md](docs/BUILD.md) | Requirements, build and run, keys, acceptance checks |
| [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) | One frame step by step, modules and their dependencies, debugging facilities |
| [docs/CONVENTIONS.md](docs/CONVENTIONS.md) | Coordinate system, matrices, colour, depth, naming |
| [CHANGELOG.md](CHANGELOG.md) | Milestone records and known issues |
| [docs/understanding](docs/understanding) | Notes written after reading the code and stepping through it in the debugger |

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
