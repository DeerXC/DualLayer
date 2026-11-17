// common.hlsli - conventions shared by every shader of the project (see docs/CONVENTIONS.md).
// Space:   world space is LEFT-handed, +Y up, +X right, +Z forward.
// Matrices: row-major, ROW vectors: clip = mul(float4(position, 1), worldViewProj). C++ stores DirectXMath matrices as they are (no transpose).
// Colour:  shaders output LINEAR colour; the back buffer is written through an sRGB view.
// KEEP IN SYNC: src/render/Triangle.cpp compiles with D3DCOMPILE_PACK_MATRIX_ROW_MAJOR; this pragma says the same thing in the source.
#pragma pack_matrix(row_major)
