// models.hlsl - the cube, the sphere, the cylinder, the torus and the floor: one directional light, a little sky light.
// Slots:   b0 Frame (the camera and the light, once per frame), b1 Object (once per draw call). No textures, no samplers.
// Light:   everything is worked out in LINEAR space (common.hlsli): the colours that arrive are linear, this shader returns linear colour and the
//          back buffer is written through an sRGB view. Do not apply a gamma curve here.
//          Since M2 the light colour, the light direction and the sky colours come from the theme of the layer being drawn (themes are never mixed).
// Faces:   the theme also brings fog (the colour of the background, thicker in B), the stone of the city (a tint) and its
//          weathering (uneven blocks, moss on what looks up). Only the city takes them (color.a = 1): the key objects of one
//          layer and everything that glows keep their own colours. Glowing things are seen through more of the fog.
// Batched: the fixed boxes of the map come baked into one vertex buffer per layer (render/StaticBatch.h): VSBatched takes
//          the world position, normal, colour and parameters from the vertex, VSMain from the constant buffer Object. Both
//          hand them to the same PSMain, so a baked box looks exactly like a box drawn alone.
// Water:   knobs.x from 2 on is water (src/scene/SceneData.h kMatWater ..), drawn after everything else with blending
//          (render/Models.cpp): its glow (knobs.w) is then its opacity. Still water and the sea move with waves (a sum of
//          travelling sines turns the normal of the surface), mirror the sky by the Fresnel term, catch the sun as a glint,
//          and are seen into where one looks down. Running water streaks along its flow and falls white. Foam breaks into
//          moving flecks. The wet band shows on its sides only. `time` (in Frame) drives it all.
// Lantern: the circle of the lantern set down in A (src/scene/Lantern.h) is a sphere in which B shows A. B is drawn without
//          its inside (lanternMode 1), then the town of A only inside it (lanternMode 2), with a warm rim at its edge.
// KEEP IN SYNC: struct FrameConstants and struct ObjectConstants in src/render/Models.cpp (field for field; 256 and 160 bytes),
//          struct BatchVertex in src/render/StaticBatch.h (56 bytes) and the batched input layout in Models.cpp.

#include "common.hlsli"

cbuffer Frame : register(b0)
{
    float4x4 viewProjection; // View * Projection
    float3 eye;              // the camera, in the world
    float time;              // UNITS: seconds since the start: the waves, the streaks, the foam
    float3 toLight; // the direction FROM a surface TO the light (unit length)
    float ambient;  // 1 = the sky light as the theme says
    float3 lightColor;
    float lanternMode; // 0: none; 1: B, the inside of the circle of the lantern is not drawn; 2: A, only its inside is
    float3 skyUp; // the sky light on a surface that looks up
    float pad2;
    float3 skyDown; // the ground light on a surface that looks down
    float pad3;
    float3 fogColor; // the colour of the fog (the background)
    float fogDensity; // the part of the light lost per metre
    float3 tint;      // the stone of the city in this layer
    float weathering; // 0 .. 1: uneven blocks, moss on what looks up
    float4 lantern;   // xyz: the centre of the circle of the lantern; w: its radius
    float4 skyRight;  // xyz: perspective-scaled camera right; w: extra low-mist density
    float4 skyUpRay;  // xyz: perspective-scaled camera up; w: low-mist base
    float4 skyForward; // xyz: camera forward; w: halo weight
    float4 skyZenith; // rgb: linear zenith; w: gradient enabled
};

float3 SkyGradient(float altitude)
{
    const float up = smoothstep(0.0, 0.78, altitude);
    const float down = smoothstep(0.02, 0.75, -altitude);
    return lerp(lerp(fogColor, skyZenith.rgb, up), fogColor * 0.62, down);
}

float3 AtmosphereColour(float3 ray)
{
    const float alignment = saturate(dot(ray, toLight));
    // Broad glow and a soft bright core; no noise or sky textures.
    const float halo = pow(alignment, 20.0) * 0.23 + pow(alignment, 220.0) * 0.37;
    return SkyGradient(ray.y) + lightColor * (skyForward.w * halo * smoothstep(-0.02, 0.10, ray.y));
}

float4 PSSky(float4 position : SV_Position, float2 uv : TEXCOORD0) : SV_Target
{
    const float2 ndc = uv * float2(2.0, -2.0) + float2(-1.0, 1.0);
    const float3 ray = normalize(skyForward.xyz + ndc.x * skyRight.xyz + ndc.y * skyUpRay.xyz);
    // Small ordered dither in linear light limits bands on the eight-bit target.
    const float dither = (frac(dot(floor(position.xy), float2(0.5, 0.25))) - 0.375) * 0.002;
    return float4(saturate(AtmosphereColour(ray) + dither), 1.0);
}

float LowMist(float y)
{
    return 1.0 - smoothstep(skyUpRay.w, skyUpRay.w + 19.0, y);
}

float SceneFog(float3 worldPos, out float3 colour)
{
    const float3 offset = worldPos - eye;
    const float metres = length(offset);
    float density = fogDensity;
    colour = fogColor;
    if (skyZenith.w > 0.5)
    {
        // Three midpoint samples avoid making a tall object fully fogged just because its foot is low.
        const float low = (LowMist(lerp(eye.y, worldPos.y, 1.0 / 6.0)) + LowMist(lerp(eye.y, worldPos.y, 0.5)) +
                           LowMist(lerp(eye.y, worldPos.y, 5.0 / 6.0))) / 3.0;
        density += low * skyRight.w;
        colour = lerp(SkyGradient(offset.y / max(metres, 1.0e-4)), fogColor, low * 0.45);
    }
    return 1.0 - exp(-metres * density);
}

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

struct VSBatchIn
{
    float3 position : POSITION; // world space
    float3 normal : NORMAL;     // world space
    float4 color : COLOR;
    float4 params : TEXCOORD0;
};

struct VSOut
{
    float4 position : SV_Position;
    float3 worldPosition : TEXCOORD0;
    float3 worldNormal : TEXCOORD1;
    float4 tone : TEXCOORD2;  // the colour (a: the city flag)
    float4 knobs : TEXCOORD3; // checker, shine strength, shininess, glow
};

VSOut VSMain(VSIn input)
{
    VSOut output;
    const float4 worldPosition = mul(float4(input.position, 1.0), world);
    output.position = mul(worldPosition, viewProjection);
    output.worldPosition = worldPosition.xyz;
    output.worldNormal = mul(input.normal, (float3x3)normalMatrix);
    output.tone = color;
    output.knobs = params;
    return output;
}

VSOut VSBatched(VSBatchIn input)
{
    VSOut output;
    output.position = mul(float4(input.position, 1.0), viewProjection);
    output.worldPosition = input.position;
    output.worldNormal = input.normal;
    output.tone = input.color;
    output.knobs = input.params;
    return output;
}

// ---- water

float Hash1(float2 p)
{
    return frac(sin(dot(p, float2(127.1, 311.7))) * 43758.5453);
}

// one travelling wave: its height (sin) and its slope along its direction (cos), added into h and slope
void Wave(float2 p, float2 dir, float frequency, float speed, float amplitude, inout float h, inout float2 slope)
{
    const float phase = dot(dir, p) * frequency + time * speed;
    h += amplitude * sin(phase);
    slope += dir * (amplitude * frequency * cos(phase));
}

// the surface of water at p (metres): four waves; `scale` stretches them (the sea: long swells), `strength` tilts them
void Waves(float2 p, float scale, float strength, out float h, out float2 slope)
{
    h = 0.0;
    slope = float2(0.0, 0.0);
    const float2 q = p / scale;
    Wave(q, float2(0.96, 0.28), 1.0, 1.1, 1.0, h, slope);
    Wave(q, float2(-0.37, 0.93), 1.9, 1.7, 0.5, h, slope);
    Wave(q, float2(0.66, -0.75), 3.3, 2.4, 0.25, h, slope);
    Wave(q, float2(-0.98, -0.2), 5.7, 3.1, 0.12, h, slope);
    h /= 1.87;
    slope *= strength / 1.87;
}

float4 WaterPS(VSOut input, float3 n, float3 l, float3 v)
{
    const float kind = input.knobs.x;
    const float opacity = saturate(input.knobs.w);
    const float3 p = input.worldPosition;
    const float3 albedo = input.tone.rgb;
    const float diffuse = saturate(dot(n, l));
    const float3 skyLight = lerp(skyDown, skyUp, n.y * 0.5 + 0.5) * ambient;
    float3 colour;
    float alpha;
    if (kind > 5.5)
    {
        // foam: moving flecks, thinner at their edges
        const float2 cell = floor(p.xz * 6.0 + float2(time * 0.7, time * 0.4));
        const float fleck = Hash1(cell);
        const float drift = 0.5 + 0.5 * sin(time * 3.0 + fleck * 6.2831);
        colour = albedo * (skyLight + lightColor * (0.4 + 0.6 * diffuse)) + albedo * 0.15;
        alpha = opacity * lerp(0.35, 1.0, fleck * drift);
    }
    else if (kind > 4.5)
    {
        // the wet band: on the walls only (its top and bottom are inside the water and the air); darker toward its foot
        if (abs(n.y) > 0.5)
        {
            discard;
        }
        colour = albedo * skyLight;
        alpha = opacity * 0.5;
    }
    else
    {
        const bool sea = kind > 3.5;
        const bool flow = kind > 2.5 && !sea;
        float3 nn = n;
        float crest = 0.0;
        float streak = 0.0;
        if (n.y > 0.5)
        {
            float h;
            float2 slope;
            float2 q = p.xz;
            if (flow)
            {
                // running along the longer side of the box, toward -x or -z
                const float2 along = length(world[0].xyz) >= length(world[2].xyz) ? float2(1.0, 0.0) : float2(0.0, 1.0);
                q += along * time * 2.5;
                const float lane = floor(dot(p.xz, float2(along.y, along.x)) * 5.0);
                streak = smoothstep(0.75, 1.0, frac(dot(q, along) * 0.6 + Hash1(float2(lane, 3.0))));
            }
            Waves(q, sea ? 6.0 : 1.0, sea ? 0.45 : (flow ? 0.35 : 0.18), h, slope);
            nn = normalize(float3(-slope.x, 1.0, -slope.y));
            crest = sea ? smoothstep(0.55, 0.95, h) : 0.0;
        }
        else if (flow)
        {
            // falling: white streaks run down the face, each column at its own pace
            const float column = floor((p.x + p.z) * 6.0);
            const float pace = 0.8 + 0.6 * Hash1(float2(column, 7.0));
            streak = smoothstep(0.6, 1.0, frac(p.y * 0.9 + time * 2.6 * pace + Hash1(float2(column, 1.0))));
            nn = normalize(n + 0.25 * float3(sin(p.y * 7.0 + time * 9.0), 0.0, cos(p.y * 5.0 + time * 7.0)));
        }
        // the sky in the water (Fresnel: more of it the flatter one looks), the sun as a glint, the body of the water below
        const float facing = saturate(dot(nn, v));
        const float fresnel = 0.02 + 0.98 * pow(saturate(1.0 - facing), 5.0);
        const float3 r = reflect(-v, nn);
        const float3 skyColour = lerp(fogColor, skyUp * 1.3 + fogColor * 0.4, saturate(r.y * 1.5));
        const float glint = pow(saturate(dot(r, l)), sea ? 260.0 : 480.0) * (sea ? 3.0 : 4.0);
        const float3 body = albedo * (skyLight + lightColor * (0.25 + 0.5 * diffuse)) * lerp(1.0, 0.55, facing);
        colour = lerp(body, skyColour, fresnel) + lightColor * glint;
        const float3 white = (skyLight + lightColor * 0.8) * 0.95;
        colour = lerp(colour, white, saturate(crest * 0.7 + streak * 0.8));
        // seen into where one looks down (the bed shows through), less where one looks across; the sea and falls are dense
        const float clear = sea ? 0.2 : (flow ? 0.3 : 0.55);
        alpha = opacity * saturate(lerp(1.0 - clear, 1.0, fresnel) + crest * 0.5 + streak * 0.5);
    }
    float3 haze;
    const float fog = SceneFog(p, haze);
    return float4(lerp(colour, haze, fog), alpha);
}

// The part of the light of the rim of the circle of the lantern: 1 at its edge, 0 a hand's breadth inside it. Discards
// what is on the wrong side of the circle in lanternMode 1 and 2.
float LanternClip(float3 p)
{
    if (lanternMode < 0.5)
    {
        return 0.0;
    }
    const float inside = lantern.w - length(p - lantern.xyz); // > 0 inside the circle
    if (lanternMode < 1.5)
    {
        clip(-inside); // B: nothing inside the circle
        return 0.0;
    }
    clip(inside); // A: nothing outside it
    return 1.0 - saturate(inside / 0.12);
}

float4 PSMain(VSOut input) : SV_Target
{
    const float rim = LanternClip(input.worldPosition);
    if (rim > 0.0)
    {
        return float4(1.0, 0.72, 0.36, 1.0) * (1.2 + 0.3 * sin(time * 3.0)); // the warm edge of the past
    }
    const float3 n = normalize(input.worldNormal);
    const float3 l = normalize(toLight);
    const float3 v = normalize(eye - input.worldPosition);
    const float3 h = normalize(l + v);

    if (input.knobs.x > 1.5)
    {
        return WaterPS(input, n, l, v);
    }

    float3 albedo = input.tone.rgb;
    if (input.knobs.x > 0.5)
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
    if (input.tone.a > 0.5)
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
    const float shine = pow(saturate(dot(n, h)), max(input.knobs.z, 1.0)) * input.knobs.y * step(0.0001, diffuse);
    // glow: light of its own (a lamp, a hint, an orb), the same in both layers and on every side
    const float3 shaded = albedo * skyLight + albedo * lightColor * diffuse + lightColor * shine + albedo * input.knobs.w;
    // fog: what is far melts into the background; a thing that glows is seen through more of it (a lamp in the mist)
    float3 haze;
    const float fog = SceneFog(input.worldPosition, haze) * (1.0 - 0.75 * saturate(input.knobs.w));
    return float4(lerp(shaded, haze, fog), 1.0);
}
