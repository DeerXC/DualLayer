# DualLayer

> 日本語：同じ港町の「過去」と「現在」を切り替え、過去に置いたランタンの光で現在に道を残して進む一人称 3D 探索パズル。戦闘なし。ゲームエンジンを使わず C++20 / Direct3D 11 で制作中。
>
> English: a first-person 3D exploration puzzle in one harbour town at two moments, the past and the present. A lantern
> set down in the past keeps a piece of it standing in the present. No combat. Built without a game engine on C++20 /
> Direct3D 11.

Status: in development (milestone M2). The design was revised on 2026-11-24 ([docs/DESIGN.md](docs/DESIGN.md)): one hill
town climbed in a spiral round a rock with a bell tower, a warm past against a cold present, a causal rule (what is done in
the past leaves its trace in the present) and the lantern. The game now loads the town of the layout model
(`tools/layout/city_model.py`, exported as `data/town.dlt`) and walks it in both moments; its puzzles are being rebuilt.
The greybox of the previous map, with every district, the bells and the three endings, is still played with
`[map] town = false`, and every system below runs in it.

## Highlights

- **One rule, many uses.** The only ability is switching between two moments of one town: A, the past, whole in a warm
  evening, and B, the present, in ruins, cold and foggy. A wall of one moment is air in the other; a jump can start in one
  and land in the other; a crate that exists in both is stopped by the walls of both; a thing works only in its own moment.
- **Puzzles proved by a solver.** A breadth-first solver proves that the courtyard puzzle takes 10 pushes, cannot be solved
  in one layer alone, needs two switches at the fewest, and looks like 2 pushes to a player who ignores the other layer. Its layout was chosen by a search with
  the same solver, and the walls in the game are generated at compile time from the same rows, so the proof and the game
  cannot drift apart.
- **A world without a void, proved.** The ground is a height field generated in code, flat-shaded and baked into the static
  batch with the plants. Tests prove that it lies under every floor, that every walk or jump off an edge of the map (about 14600
  tried) ends in the map or is caught as a fall, and that every view down meets the map, the land or the sea.
- **A map that is walked and searched by its tests.** A test drives the real walking body through the whole map: every
  room, every shortcut, every ramp, the cathedral to its top. Another one searches it for skips: from about 11000 places to
  stand, about 410000 jumps in every direction, with and without a switch of layers in the air. Its first runs found five
  ways round the map that no play-test had found.
- **Objects that react to the switch.** An orb is in one layer: carried, it goes with the body; put down, it stays, and it
  falls through a floor that exists only in the other layer. The west wing is built on that: an orb that lays a bridge of
  the other layer, an orb that holds a door open in a world the body has left, an orb dropped through a hole that is closed
  in one world and open in the other.
- **Rules that hold in both worlds.** What exists only in A is made by people (wood, iron), what exists only in B was left
  by the storm (fallen masonry): the material says the world. Every water is fed downhill from an aqueduct that is broken
  in B, so every water is dry there; machinery turns only in A; a thing that moves exists in both worlds, only its place
  differs. Tests check each rule against the map data.
- **Layer switch as a ripple.** Both layers are rendered into their own off-screen pictures and a circle opens from the
  centre of the screen; pressed again, it turns round without a jump.
- **Water drawn as water.** Seen through and drawn after everything opaque: waves that move the light, the sky and the
  sun in it, the sea in long swells, falls between the locks, foam, the wet band a falling surface leaves, and a splash.
- **Guidance without a tutorial.** Every object of one layer leaves a pale outline in the other, brighter when the player
  stays long in a room; a column of light stands
  over every place that waits for something; where a bridge will appear is outlined before it does; a refused push
  outlines what was in the way; a beacon over the goal is seen from the first step. The only words are key prompts,
  drawn with a bitmap font written for the project, chosen by the same code that runs the key, and dimmed once learned.
- **Hand-written engine layer.** Direct3D 11 device and swap chain, depth, constant buffers, HLSL compiled at run time, a
  static batch that draws the whole fixed town of a layer, its ground and plants in one call, procedural meshes with winding tests, collision of boxes, spheres, cylinders and tori, a walking body with tuned feel
  (acceleration, variable jump height, coyote time, jump buffer, ground snap on ramps), recovery from a lost GPU device.
- **One command checks everything.** `tools/check.ps1` runs the logic tests, a full Debug and Release rebuild (and proves the
  exe under test is the one just built), a scripted game run and a repository safety check, and commits only when all of it
  passed.

## Demo

A video will be added with the first district of the new map.

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
