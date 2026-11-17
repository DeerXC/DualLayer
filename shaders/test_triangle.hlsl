// test_triangle.hlsl - one coloured triangle that checks the coordinate and matrix conventions.
// Slots:   b0 PerObject (worldViewProj).  No textures, no samplers.
// Expect:  with the camera at (0, 0, -3) looking along +Z, RED is on the RIGHT, GREEN on top, BLUE on the LEFT.
//          Red on the left = the X axis is mirrored (a right-handed matrix was used). Nothing visible = wrong winding or a transposed matrix.
// KEEP IN SYNC: struct PerObject in src/render/Triangle.cpp (one float4x4 = 64 bytes).
#include "common.hlsli"

cbuffer PerObject : register(b0)
{
    float4x4 worldViewProj;
};

struct VSIn
{
    float3 position : POSITION;
    float3 color : COLOR;
};

struct VSOut
{
    float4 position : SV_Position;
    float3 color : COLOR;
};

VSOut VSMain(VSIn input)
{
    VSOut output;
    output.position = mul(float4(input.position, 1.0), worldViewProj);
    output.color = input.color;
    return output;
}

float4 PSMain(VSOut input) : SV_Target
{
    return float4(input.color, 1.0);
}
