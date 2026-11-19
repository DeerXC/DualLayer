// models.hlsl - the cube, the sphere, the cylinder, the torus and the floor: one directional light, a little sky light.
// Slots:   b0 Frame (the camera and the light, once per frame), b1 Object (once per draw call). No textures, no samplers.
// Light:   everything is worked out in LINEAR space (common.hlsli): the colours that arrive are linear, this shader returns linear colour and the
//          back buffer is written through an sRGB view. Do not apply a gamma curve here.
//          Since M2 the light colour, the light direction and the sky colours come from the theme of the layer being drawn (themes are never mixed).
// KEEP IN SYNC: struct FrameConstants and struct ObjectConstants in src/render/Models.cpp (field for field; 144 and 160 bytes).

#include "common.hlsli"

cbuffer Frame : register(b0)
{
    float4x4 viewProjection; // View * Projection
    float3 eye;              // the camera, in the world
    float pad0;
    float3 toLight; // the direction FROM a surface TO the light (unit length)
    float ambient;  // 1 = the sky light as the theme says
    float3 lightColor;
    float pad1;
    float3 skyUp; // the sky light on a surface that looks up
    float pad2;
    float3 skyDown; // the ground light on a surface that looks down
    float pad3;
};

cbuffer Object : register(b1)
{
    float4x4 world;
    float4x4 normalMatrix; // the inverse transpose of the 3x3 of `world`: it keeps a normal at a right angle to the surface of a stretched object
    float4 color;          // rgb: linear albedo
    float4 params;         // x: 1 = a checkered floor, y: strength of the shine, z: how sharp the shine is
};

struct VSIn
{
    float3 position : POSITION;
    float3 normal : NORMAL;
};

struct VSOut
{
    float4 position : SV_Position;
    float3 worldPosition : TEXCOORD0;
    float3 worldNormal : TEXCOORD1;
};

VSOut VSMain(VSIn input)
{
    VSOut output;
    const float4 worldPosition = mul(float4(input.position, 1.0), world);
    output.position = mul(worldPosition, viewProjection);
    output.worldPosition = worldPosition.xyz;
    output.worldNormal = mul(input.normal, (float3x3)normalMatrix);
    return output;
}

float4 PSMain(VSOut input) : SV_Target
{
    const float3 n = normalize(input.worldNormal);
    const float3 l = normalize(toLight);
    const float3 v = normalize(eye - input.worldPosition);
    const float3 h = normalize(l + v);

    float3 albedo = color.rgb;
    if (params.x > 0.5)
    {
        // a floor of one-metre tiles: every second tile is darker, and a thin lighter line runs along the edge of the tiles
        const float2 p = input.worldPosition.xz;
        const float tile = fmod(abs(floor(p.x) + floor(p.y)), 2.0);
        albedo *= lerp(0.55, 1.0, tile);
        const float2 edge = min(frac(p), 1.0 - frac(p));
        const float2 width = max(fwidth(p), float2(1.0e-4, 1.0e-4));
        // (not called "line": that is a reserved word of HLSL and fxc refuses it)
        const float gridLine = 1.0 - saturate(min(edge.x / (width.x * 1.5), edge.y / (width.y * 1.5)));
        albedo = lerp(albedo, albedo * 2.2, gridLine * 0.35);
    }

    // a sky above and a ground below: a surface that looks up gets more light than one that looks down, and a face turned away from the light is dark but not black
    const float3 skyLight = lerp(skyDown, skyUp, n.y * 0.5 + 0.5) * ambient;
    const float diffuse = saturate(dot(n, l));
    const float shine = pow(saturate(dot(n, h)), max(params.z, 1.0)) * params.y * step(0.0001, diffuse);
    const float3 result = albedo * skyLight + albedo * lightColor * diffuse + lightColor * shine;
    return float4(result, 1.0);
}
