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
| Colour | textures loaded as sRGB; lighting in linear space; the back buffer is written through an sRGB view (`R8G8B8A8_UNORM_SRGB` on a `R8G8B8A8_UNORM` flip-model swap chain) | `Device::CreateBackBufferView`; clear colours in `config` are linear |
| Depth | **D32_FLOAT, compare LESS, cleared to 1.0 (far)** since M1 (`Device::BeginFrame`, `Models::CreateStates`). Reverse-Z (clear 0, compare GREATER) is still to be tested; if it wins, only those two places change, and the result is recorded in the CHANGELOG | the models: a sphere in front of the floor hides it, behind it does not |
| Mesh vertex | position (3 floats) + normal (3 floats) = 24 bytes, no UV yet; indices are 32 bit | `MeshData.h` static_assert, input layout in `Models.cpp` |
| Mesh winding | clockwise seen from OUTSIDE; the outward normal is `cross(v1 - v0, v2 - v0)` (component formula); every shape is centred on its own origin, 1 m big | `tests/test_mesh.cpp`, every triangle of every shape |
| Normals | moved with the inverse transpose of the world matrix, not with the world matrix | `Models::Draw`, `ObjectConstants.normalMatrix` |
| Feature level | 11_0 minimum (11_1 used when present) | |
| File encoding | UTF-8, no BOM, LF line ends (`.gitattributes`, `.editorconfig`) | |
| Naming | types and functions `PascalCase`, variables `camelCase`, members `m_`, constants `kPascalCase`, `enum class` | `.clang-format` formats, it does not rename |
