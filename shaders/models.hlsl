// models.hlsl - the cube, the sphere, the cylinder, the torus and the floor: one directional light, a little sky light.
// Slots:   b0 Frame (the camera and the light, once per frame), b1 Object (once per draw call). No textures, no samplers.
// Light:   everything is worked out in LINEAR space (common.hlsli): the colours that arrive are linear, this shader returns linear colour and the
//          back buffer is written through an sRGB view. Do not apply a gamma curve here.
//          Since M2 the light colour, the light direction and the sky colours come from the theme of the layer being drawn (themes are never mixed).
// Faces:   the theme also brings fog (the colour of the background, thicker in B), the stone of the city (a tint) and its
//          weathering (uneven blocks, moss on what looks up). Only the city takes them (color.a = 1): the key objects of one
//          layer and everything that glows keep their own colours. Glowing things are seen through more of the fog.
// KEEP IN SYNC: struct FrameConstants and struct ObjectConstants in src/render/Models.cpp (field for field; 176 and 160 bytes).

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
    float3 fogColor; // the colour of the fog (the background)
    float fogDensity; // the part of the light lost per metre
    float3 tint;      // the stone of the city in this layer
    float weathering; // 0 .. 1: uneven blocks, moss on what looks up
};

cbuffer Object : register(b1)
{
    float4x4 world;
    float4x4 normalMatrix; // the inverse transpose of the 3x3 of `world`: it keeps a normal at a right angle to the surface of a stretched object
    float4 color;          // rgb: linear albedo; a: 1 = part of the city (tint and weathering), 0 = keeps its own colour
    float4 params;         // x: 1 = a checkered floor, y: strength of the shine, z: how sharp the shine is, w: glow
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

    // the city: the stone of this layer, in blocks of about 0.7 m that are a little uneven, with moss on what looks up
    if (color.a > 0.5)
    {
        const float3 cell = floor(input.worldPosition * 1.4 + n * 0.01);
        const float grain = frac(sin(dot(cell, float3(12.9898, 78.233, 37.719))) * 43758.5453);
        albedo *= tint * lerp(1.0, lerp(0.62, 1.0, grain), weathering);
        const float up = saturate(n.y * 2.0 - 1.0);
        albedo = lerp(albedo, float3(0.035, 0.07, 0.03), up * weathering * 0.55 * grain);
    }

    // a sky above and a ground below: a surface that looks up gets more light than one that looks down, and a face turned away from the light is dark but not black
    const float3 skyLight = lerp(skyDown, skyUp, n.y * 0.5 + 0.5) * ambient;
    const float diffuse = saturate(dot(n, l));
    const float shine = pow(saturate(dot(n, h)), max(params.z, 1.0)) * params.y * step(0.0001, diffuse);
    // glow: light of its own (a lamp, a hint, an orb), the same in both layers and on every side
    const float3 shaded = albedo * skyLight + albedo * lightColor * diffuse + lightColor * shine + albedo * params.w;
    // fog: what is far melts into the background; a thing that glows is seen through more of it (a lamp in the mist)
    const float toEye = length(eye - input.worldPosition); // (not "distance" or "lit": names of intrinsics)
    const float fog = (1.0 - exp(-toEye * fogDensity)) * (1.0 - 0.75 * saturate(params.w));
    return float4(lerp(shaded, fogColor, fog), 1.0);
}

