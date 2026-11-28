// rain.hlsl - the rain of B (docs/DESIGN.md 6.11, stage R1): one full-screen pass over the depth buffer of a finished picture of B, after
//             everything else (the water and the halos included), so that the rain falls over all of it.
// Slots:      t1 = the depth buffer of the picture (a typeless depth texture with a shader view: render/Device.cpp, render/Models.cpp),
//             b0 = Rain. t0 is left alone: it is the shadow map of the scene.
// Colour:     the picture is not read. The rain is added with the blend state ONE, INV_SRC_ALPHA (render/Models.cpp, m_rainBlend):
//             result = source.rgb + picture * (1 - source.a), so the source is the colour of the rain times its coverage and the alpha
//             is the coverage. The back buffer is written through an sRGB view, so the blend is done in linear light.
// Geometry:   one triangle that covers the whole screen, made from SV_VertexID: the vertex shader of ripple.hlsl is used (no vertex
//             buffer, no input layout), this file has the pixel shader only.
// The maths:  rain.hlsli (tested without a GPU in tests/test_style.cpp, TestRain). Here: reading the depth, the three layers of streaks,
//             the rings on the ground and the blend.
// KEEP IN SYNC: struct RainConstants in src/render/RainSettings.h (field for field; 240 bytes).
#include "lines.hlsli"
#include "rain.hlsli"

cbuffer Rain : register(b0)
{
    float4 rainView;     // x: tan of half the field of view across, y: up, z: near plane, w: far plane (metres)
    float4 rainClock;    // x: seconds, y: the size of a pixel of the screen in pixels of 1080 high, z: the master amount, w: the density of the rings
    float4 rainFall;     // x, y: the fall direction on the screen (unit, y down), z: how far the rings reach (m), w: the size of a pixel at 1 m
    float4 rainLayerA;   // the near layer: x: cell width, y: cell height (pixels of 1080), z: density, w: speed (pixels of 1080 a second)
    float4 rainLayerB;   // the middle layer
    float4 rainLayerC;   // the far layer
    float4 rainDepths;   // x, y, z: the distance of the three layers (m), w: the cell of the rings (m)
    float4 rainLook;     // x: the length of a streak (of a cell), y: its width at the near layer (pixels of 1080), z: the strength of the
                         // streaks, w: the strength of the rings
    float4 rainTint;     // rgb: the colour of the rain (linear), a: the radius of a ring (m)
    float4 rainEye;      // xyz: the camera in the world, w: the rate of the rings (cycles a second)
    float4 rainRight;    // xyz: the right of the camera (world, unit)
    float4 rainUp;       // xyz: the up of the camera
    float4 rainForward;  // xyz: the forward of the camera
    float4 rainGround;   // xyz: the world up, in the view space of the camera (for the rings)
    float4 rainLantern;  // xyz: the centre of the dry circle, w: its radius (0: none)
};

Texture2D<float> depthTexture : register(t1);

struct VSOut
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
};

// One layer of streaks at the pixel: `layer` is 0, 1 or 2; the strength falls with the layer.
float LayerAt(float2 p, float4 cells, float widthScale, float seed, float layerDepth, float strength, float sceneDepth, float3 rayDir)
{
    const float px = rainClock.y;
    const float2 fall = rainFall.xy;
    const float u = dot(p, float2(-fall.y, fall.x));
    const float v = dot(p, fall);
    const float width = max(rainLook.y * widthScale * px, 0.9);
    const float streak = RainStreak(u, v, cells.x * px, cells.y * px, cells.z, cells.w * px, rainClock.x, rainLook.x, width, seed);
    if (streak <= 0.0)
    {
        return 0.0;
    }
    // each streak stands at a distance of its own (0.4 to 1.6 of the layer's): in front of the near ground as well as of the far
    const float depth = layerDepth * RainDepthFactor(u, v, cells.x * px, cells.y * px, cells.w * px, rainClock.x, seed);
    const float3 at = rainEye.xyz + rayDir * depth; // where the streak is, along this pixel's ray
    const float dry = RainDry(at.x, at.y, at.z, rainLantern.x, rainLantern.y, rainLantern.z, rainLantern.w, 1.2);
    return RainLayerCoverage(streak, sceneDepth, depth, dry, strength);
}

float4 PSMain(VSOut input) : SV_Target
{
    uint widthPixels;
    uint heightPixels;
    depthTexture.GetDimensions(widthPixels, heightPixels);
    const float2 pixel = input.position.xy; // the middle of the pixel
    const float depth = depthTexture.Load(int3(int2(pixel), 0));
    const float metres = 1.0 / InverseViewDepth(depth, rainView.z, rainView.w); // the depth of view of what is there (the far plane for the sky)
    const float sx = (pixel.x / float(widthPixels) * 2.0 - 1.0) * rainView.x;
    const float sy = (1.0 - pixel.y / float(heightPixels) * 2.0) * rainView.y;
    const float3 view = float3(sx * metres, sy * metres, metres); // the point in the view space of the camera
    // the normal of the surface, turned to the eye (these two derivatives must be taken before anything returns)
    float3 normal = normalize(cross(ddx(view), ddy(view)));
    if (dot(normal, view) > 0.0)
    {
        normal = -normal;
    }
    const float3 rayDir = rainRight.xyz * sx + rainUp.xyz * sy + rainForward.xyz; // along this pixel, with a depth of view of 1

    float near0 = LayerAt(pixel, rainLayerA, 1.0, 1.0, rainDepths.x, rainLook.z, metres, rayDir);
    float mid = LayerAt(pixel, rainLayerB, 0.75, 2.0, rainDepths.y, rainLook.z * 0.75, metres, rayDir);
    float far0 = LayerAt(pixel, rainLayerC, 0.55, 3.0, rainDepths.z, rainLook.z * 0.5, metres, rayDir);

    float ring = 0.0;
    const float ground = RainGround(dot(normal, rainGround.xyz));
    if (ground > 0.01 && metres < rainFall.z)
    {
        const float3 world = rainEye.xyz + rayDir * metres;
        const float dry = RainDry(world.x, world.y, world.z, rainLantern.x, rainLantern.y, rainLantern.z, rainLantern.w, 0.6);
        const float width = RainRingWidth(metres, rainFall.w);
        ring = RainRing(world.x, world.z, rainClock.x, rainClock.w, rainDepths.w, rainTint.a, width, rainEye.w) * ground *
               RainRingReach(metres, rainFall.z) * dry * rainLook.w;
    }
    const float coverage = RainCombine(near0, mid, far0, ring) * rainClock.z;
    if (coverage <= 0.002)
    {
        discard; // nothing to add: the picture stays as it is
    }
    return float4(rainTint.rgb * coverage, coverage);
}
