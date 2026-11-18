# Conventions

Decided once, used everywhere.

| Item | Rule | Why / check |
|---|---|---|
| Length | metres | |
| Angles | radians in code; degrees in data files (config), converted on load and marked `UNITS` | `[test_triangle] spin_deg_per_sec` |
| Coordinate system | **left-handed, +Y up, +X right, +Z forward** | DirectXMath `...LH` functions |
| Matrices | DirectXMath row-major, **row vectors**: `v' = v * M`; order `World * View * Projection` | `Triangle::Draw` |
| HLSL | `#pragma pack_matrix(row_major)` (`common.hlsli`) **and** the compile flag `D3DCOMPILE_PACK_MATRIX_ROW_MAJOR`; `mul(v, M)`; **no CPU-side transpose** | the M0 test triangle |
| Winding | clockwise = front face (Direct3D default) | the test triangle is clockwise seen from the camera |
| UV | origin top-left (Direct3D). Blender's origin is bottom-left: the exporter flips V | M1 exporter test (asymmetric "F" model) |
| Colour | textures loaded as sRGB; lighting in linear space; the back buffer is written through an sRGB view (`R8G8B8A8_UNORM_SRGB` on a `R8G8B8A8_UNORM` flip-model swap chain) | `Device::CreateBackBufferView`; every colour in `config` is linear |
| Depth | **D32_FLOAT, compare LESS, cleared to 1.0 (far)** (`Device::BeginFrame`, `Models::CreateStates`, and the depth of the two ripple pictures). Reverse-Z was considered and **not adopted** (decided 2026-11-18): the scene is small, and it would change four places. Revisit it only if far objects flicker | the models: a sphere in front of the floor hides it, behind it does not |
| Mesh vertex | position (3 floats) + normal (3 floats) = 24 bytes, no UV yet; indices are 32 bit | `MeshData.h` static_assert, input layout in `Models.cpp` |
| Mesh winding | clockwise seen from OUTSIDE; the outward normal is `cross(v1 - v0, v2 - v0)` (component formula); every shape is centred on its own origin, 1 m big | `tests/test_mesh.cpp`, every triangle of every shape |
| Normals | moved with the inverse transpose of the world matrix, not with the world matrix | `Models::DrawObjects`, `ObjectConstants.normalMatrix` |
| Layers | a layer is a label on an object, not a place in the world: a bit mask, **A = 1, B = 2, both = 3** (`SceneObject.layerMask`); a layer is numbered 0 (A) or 1 (B) in code | `kScene` in `Models.cpp`, the `Tab` key |
| Layer look | each layer has its own theme (background, light colour, light direction, sky colour); a layer is always drawn with ITS theme, themes are never mixed | `[layer_a]`, `[layer_b]` in `config/default.ini` |
| Layer pictures | during a ripple each layer is drawn complete into its own picture: colour `R8G8B8A8_UNORM_SRGB` (writing converts linear to sRGB, sampling converts back) and its own `D32_FLOAT` depth, the size of the back buffer; the ripple shader only chooses between the two pictures | `Models::DrawRipple`, `shaders/ripple.hlsl` |
| Ripple units | the circle is measured in screen HEIGHTS (so it is round on a wide screen); the centre is 0 .. 1 over the screen, (0, 0) = top left | `[ripple]` in `config/default.ini`, `FarthestCornerDistance` |
| windows.h | `min` and `max` are macros unless `NOMINMAX` is defined **before the first include that pulls in `windows.h`** (also through other headers). Where this order is not certain, do not use `std::min` / `std::max`; `std::clamp` and a plain `?:` are safe | `Models::Update` (the first M2 build failed with C2589), `LayerRipple.h` |
| Feature level | 11_0 minimum (11_1 used when present) | |
| File encoding | UTF-8, no BOM, LF line ends (`.gitattributes`, `.editorconfig`) | |
| Naming | types and functions `PascalCase`, variables `camelCase`, members `m_`, constants `kPascalCase`, `enum class` | `.clang-format` formats, it does not rename |
