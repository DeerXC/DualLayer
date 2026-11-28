// models.hlsl - the cube, the sphere, the cylinder, the torus and the floor: one directional light, a little sky light.
// Slots:   b0 Frame (the camera and the light, once per frame), b1 Object (once per draw call), b2 Lamps (the point lights of the
//          moment, once per moment); t0 and s0 the shadow map and its comparison sampler.
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
// Light:   the sun of A and the moon of B light a surface with a shadow map (t0, render/ShadowMath.h: the same shaders, VSMain and
//          VSBatched, draw into it with the matrix of the light and no pixel shader), the lamps of the moment light it too (cbuffer
//          Lamps: up to MAX_LAMPS point lights chosen by render/Lights.h), and the sky and the glow of the surface are added; the
//          sum is bent by a soft shoulder (lighting.hlsli SoftShoulder) so that no lamp turns a wall into a white patch. A haze is
//          scattered toward the sun. Glowing soft spheres (kMatHalo) are added to the picture over the lit lamps.
// Style:   the light is not photographic (lighting.hlsli, "the style of the light"; docs/DESIGN.md 6.3 and 6.10): the sun falls in hard steps, the
//          sun-less side takes a cool fill, clouds drift their shadows over the town, an edge has a rim, a lamp has rings and a warm
//          edge, rays of light stand in the air (LightShafts: a march along the ray of sight through the shadow map), and the picture is
//          graded (split tone, colour, a soft dark corner); the stone is painted with soft blotches and is damp near the water; the far
//          haze leans to a colour and the far distance has more fog (StyleFarFog). All of it comes from the eight registers at the end of
//          cbuffer Frame (gradeParams2 and the seven after it), set by
//          render/LightingSettings.h (MakeStyle); with all of them 0 the light is the plain one (the debug view "plain").
// KEEP IN SYNC: struct FrameConstants, struct ObjectConstants and struct LampConstants in src/render/Models.cpp (field for field;
//          480, 160 and 1040 bytes), MAX_LAMPS with kMaxLamps in src/render/Lights.h, struct BatchVertex in
//          src/render/StaticBatch.h (56 bytes) and the batched input layout in Models.cpp.

#include "common.hlsli"
#include "lighting.hlsli"

#define MAX_LAMPS 32

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
    float4x4 lightViewProj; // the world into the shadow map of the sun or the moon (render/ShadowMath.h)
    float4 shadowParams;    // x: one texel of the map (uv), y: depth bias (0 .. 1 along the light), z: normal offset (metres), w: strength (0: no shadow)
    float4 gradeParams;     // x: exposure, y: knee of the shoulder, z: debug view (0 normal, 1 no shadow, 2 no lamps, 3 the shadow, 4 the lamps), w: sun scatter
    float4 gradeParams2;    // x: sun in bands (0 .. 1), y: rim strength, z: saturation, w: vignette
    float4 styleCool;       // rgb: what the sky light of an unlit surface is multiplied by, a: how far
    float4 styleLow;        // rgb: the grade of the darks, a: how much
    float4 styleHigh;       // rgb: the grade of the lights, a: how much
    float4 styleLight;      // x: the shadow of the clouds, y: the rings of a lamp, z: strength of the light shafts, w: their samples
    float4 styleFar;        // rgb: what the haze is multiplied by far away, a: how far
    float4 styleMisc;       // x: the painted blotches of the stone, y: the damp near the water, z: the contrast, w: the soft edge of a step of the sun, in pixels (0: 1.5)
    float4 styleMore;       // x: the extra fog of the far distance, yzw: reserved
};

cbuffer Lamps : register(b2)
{
    float4 lampCount;           // x: how many of the lamps below are on
    float4 lampPos[MAX_LAMPS];  // xyz: where (world), w: its range (metres)
    float4 lampTint[MAX_LAMPS]; // rgb: its colour times its strength (linear)
};

Texture2D<float> shadowMap : register(t0);
SamplerComparisonState shadowSampler : register(s0);

// The style of the light of this frame (lighting.hlsli StyleParams), from the registers of cbuffer Frame.
StyleParams Style()
{
    StyleParams s;
    s.toon = gradeParams2.x;
    s.rim = gradeParams2.y;
    s.saturation = gradeParams2.z;
    s.vignette = gradeParams2.w;
    s.contrast = styleMisc.z;
    s.cool = styleCool.rgb;
    s.coolAmount = styleCool.a;
    s.low = styleLow.rgb;
    s.lowAmount = styleLow.a;
    s.high = styleHigh.rgb;
    s.highAmount = styleHigh.a;
    s.cloud = styleLight.x;
    s.lampRings = styleLight.y;
    s.shaft = styleLight.z;
    s.shaftSteps = styleLight.w;
    s.wash = styleMisc.x;
    s.damp = styleMisc.y;
    s.farTint = styleFar.rgb;
    s.farAmount = styleFar.a;
    s.farFog = styleMore.x;
    s.toonEdge = 0.0; // PSMain sets it from the change of the cosine over a pixel
    return s;
}

// What the sun or the moon leaves of a surface: 1 lit, down to 1 - strength in a shadow. `ndl`: how squarely the light falls on it.
float ShadowAt(float3 worldPos, float3 n, float ndl)
{
    if (shadowParams.w <= 0.0)
    {
        return 1.0;
    }
    // the surface is moved along its normal (more where the light grazes it) before it is compared with the map: no acne
    const float3 p = worldPos + n * (shadowParams.z * (1.5 - ndl));
    const float4 lp = mul(float4(p, 1.0), lightViewProj);
    const float2 uv = lp.xy * float2(0.5, -0.5) + float2(0.5, 0.5);
    const float fade = ShadowEdgeFade(uv.x, uv.y);
    if (fade <= 0.0 || lp.z >= 1.0 || lp.z <= 0.0)
    {
        return 1.0;
    }
    const float z = lp.z - shadowParams.y;
    float sum = 0.0;
    [unroll]
    for (int j = -1; j <= 1; ++j)
    {
        [unroll]
        for (int i = -1; i <= 1; ++i)
        {
            sum += shadowMap.SampleCmpLevelZero(shadowSampler, uv + float2(i, j) * shadowParams.x, z);
        }
    }
    const float lightAmount = lerp(1.0, sum / 9.0, fade);
    return 1.0 - shadowParams.w * (1.0 - lightAmount);
}

struct LampSum
{
    float3 diffuse;
    float3 specular;
};

// Every lamp of the moment on a surface (lighting.hlsli LampShade, added up). (Not called Lamps: that is the name of the buffer.)
LampSum LampsAt(float3 p, float3 n, float3 v, float shine, float power)
{
    LampSum sum;
    sum.diffuse = float3(0.0, 0.0, 0.0);
    sum.specular = float3(0.0, 0.0, 0.0);
    const int count = (gradeParams.z > 1.5 && gradeParams.z < 2.5) ? 0 : (int)lampCount.x;
    [loop]
    for (int i = 0; i < count; ++i)
    {
        const LampLight one = LampShadeStyled(p, n, v, lampPos[i].xyz, lampPos[i].w, lampTint[i].xyz, shine, power, styleLight.y);
        sum.diffuse += one.diffuse;
        sum.specular += one.specular;
    }
    return sum;
}

// The place of a point of the world on the screen, -1 .. 1 (for the dark corner of the grade).
float2 NdcOf(float3 worldPos)
{
    const float4 clip = mul(float4(worldPos, 1.0), viewProjection);
    return clip.xy / max(clip.w, 1.0e-4);
}

// The last step of every colour: the exposure, the grade of the style (lighting.hlsli StyleGrade), then the soft shoulder.
float3 Finish(float3 c, float2 ndc)
{
    return SoftShoulder(StyleGrade(c * gradeParams.x, Style(), ndc.x, ndc.y), gradeParams.y);
}

float LowMist(float y)
{
    return 1.0 - smoothstep(skyUpRay.w, skyUpRay.w + 19.0, y);
}

#define SHAFT_REACH 48.0 // UNITS: metres: how far along the ray of sight the light shafts are looked for (the shadow map is 60 m round)

// A number 0 .. 1 that differs from pixel to pixel (interleaved gradient noise): it moves the samples along a ray, so that the steps of
// the march do not show as bands.
float PixelNoise(float2 pixel)
{
    return frac(52.9829189 * frac(dot(pixel, float2(0.06711056, 0.00583715))));
}

// The light of the sun (or the moon) that the air scatters toward the eye along a ray of sight: the ray is walked in `steps` samples from
// the eye for `metres` (at most SHAFT_REACH), and each sample counts the light that the shadow map lets through at that point of the air.
// Only a ray that crosses the edge of a shadow shows a shaft (lighting.hlsli ShaftEdge): the rays through the gaps between the buildings;
// a ray that is all in the light is left to the fog. The air is thicker near the sea (LowMist). Points outside the shadow map count as lit. 0 when the style has no shafts, when there is no shadow map (the lantern's circle) or for
// the debug views that show the light in parts.
float3 LightShafts(float3 dir, float metres, float2 pixel)
{
    const float strength = styleLight.z;
    const bool partView = (gradeParams.z > 0.5 && gradeParams.z < 1.5) || gradeParams.z > 2.5; // the views 1, 3, 4, 5 show no shafts
    if (strength <= 0.0 || shadowParams.w <= 0.0 || partView)
    {
        return float3(0.0, 0.0, 0.0);
    }
    const int steps = clamp((int)(styleLight.w + 0.5), 1, 32);
    const float reach = min(metres, SHAFT_REACH);
    const float jitter = PixelNoise(pixel);
    float sum = 0.0;
    [loop]
    for (int i = 0; i < steps; ++i)
    {
        const float3 p = eye + dir * (reach * ((float)i + jitter) / (float)steps);
        const float4 lp = mul(float4(p, 1.0), lightViewProj);
        const float2 uv = lp.xy * float2(0.5, -0.5) + float2(0.5, 0.5);
        const float fade = ShadowEdgeFade(uv.x, uv.y);
        float passed = 1.0;
        if (fade > 0.0 && lp.z < 1.0 && lp.z > 0.0)
        {
            passed = shadowMap.SampleCmpLevelZero(shadowSampler, uv, lp.z - shadowParams.y);
        }
        const float air = (skyZenith.w > 0.5) ? lerp(0.35, 1.0, LowMist(p.y)) : 1.0;
        sum += lerp(1.0, passed, fade) * air;
    }
    const float amount = strength * ShaftPhase(dot(dir, toLight)) * ShaftEdge(sum / (float)steps) * (reach / SHAFT_REACH);
    return lightColor * amount;
}

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
    // the horizon of the sky has the hue that the far land fades to (StyleSky, 6.10 S3c): no band of the colour of the model over the pale hills
    const float3 gradient = StyleSky(SkyGradient(ray.y), ray.y, Style());
    return gradient + lightColor * (skyForward.w * halo * smoothstep(-0.02, 0.10, ray.y));
}

float4 PSSky(float4 position : SV_Position, float2 uv : TEXCOORD0) : SV_Target
{
    const float2 ndc = uv * float2(2.0, -2.0) + float2(-1.0, 1.0);
    const float3 ray = normalize(skyForward.xyz + ndc.x * skyRight.xyz + ndc.y * skyUpRay.xyz);
    // Small ordered dither in linear light limits bands on the eight-bit target.
    const float dither = (frac(dot(floor(position.xy), float2(0.5, 0.25))) - 0.375) * 0.002;
    // the shafts of light seen against the open sky (through the gaps of the town), to the reach of the march
    const float3 sky = AtmosphereColour(ray) + LightShafts(ray, SHAFT_REACH, position.xy);
    return float4(saturate(Finish(sky, ndc) + dither), 1.0);
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
    // the light of the sun scattered by the haze: the fog glows toward the sun
    colour += SunScatter(lightColor, dot(offset / max(metres, 1.0e-4), toLight), gradeParams.w);
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
    const float2 pixel = input.position.xy;
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
    const float fogOfModel = SceneFog(p, haze);
    const StyleParams waterStyle = Style();
    const float waterMetres = length(p - eye);
    haze = StyleHaze(haze, waterMetres, waterStyle);
    const float fog = StyleFog(fogOfModel, waterMetres, waterStyle);
    const float3 shafts = LightShafts((p - eye) / max(length(p - eye), 1.0e-4), length(p - eye), pixel) * (1.0 - fog);
    return float4(Finish(lerp(colour, haze, fog) + shafts, NdcOf(p)), alpha);
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

// A soft glow round a lit lamp: a sphere seen from the outside or the inside whose strength falls to 0 at its rim, added to the picture
// (no blending by alpha: the blend state adds) and lost in the fog like everything far away. Its glow (knobs.w) is its strength.
float4 HaloPS(VSOut input, float3 n, float3 v)
{
    float3 haze;
    const float fog = SceneFog(input.worldPosition, haze);
    const float3 glow = input.tone.rgb * (input.knobs.w * HaloFalloff(dot(n, v)) * (1.0 - fog));
    return float4(glow, 1.0);
}

float4 PSMain(VSOut input) : SV_Target
{
    // first, before any return: how fast the cosine to the sun changes from one pixel to the next (the soft edge of a step of the sun, 6.10)
    const float ndlWidth = fwidth(saturate(dot(normalize(input.worldNormal), normalize(toLight))));
    const float rim = LanternClip(input.worldPosition);
    if (rim > 0.0)
    {
        return float4(1.0, 0.72, 0.36, 1.0) * (1.2 + 0.3 * sin(time * 3.0)); // the warm edge of the past
    }
    const float3 n = normalize(input.worldNormal);
    const float3 l = normalize(toLight);
    const float3 v = normalize(eye - input.worldPosition);
    const float3 h = normalize(l + v);

    if (input.knobs.x > 6.5)
    {
        return HaloPS(input, n, v);
    }
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
    // the style: the stone painted by hand and damp near the water (the city only: what keeps its own colour is left as it is)
    StyleParams style = Style();
    // the edge of a step of the sun is toon_edge pixels wide; a flat face has no change of the cosine, so its step is hard
    style.toonEdge = ndlWidth * (styleMisc.w > 0.0 ? styleMisc.w : 1.5);
    if (input.tone.a > 0.5)
    {
        const float nearWater = (skyZenith.w > 0.5) ? LowMist(input.worldPosition.y) : 0.0;
        albedo = PaintAlbedo(albedo, input.worldPosition, n, nearWater, style);
    }

    // a sky above and a ground below: a surface that looks up gets more light than one that looks down, and a face turned away from the light is dark but not black
    const float3 skyLight = lerp(skyDown, skyUp, n.y * 0.5 + 0.5) * ambient;
    const float diffuse = saturate(dot(n, l));
    const float shine = pow(saturate(dot(n, h)), max(input.knobs.z, 1.0)) * input.knobs.y * step(0.0001, diffuse);
    // the shadow of the sun or the moon and of the clouds, and the lamps of the moment (the debug views of gradeParams.z leave one of them out)
    const int view = (int)(gradeParams.z + 0.5);
    float sun = 1.0;
    if (view != 1)
    {
        sun = ShadowAt(input.worldPosition, n, diffuse) * CloudShade(input.worldPosition, l, time, style.cloud);
    }
    if (view == 3)
    {
        return float4(sun, sun, sun, 1.0);
    }
    const LampSum lamps = LampsAt(input.worldPosition, n, v, input.knobs.y, input.knobs.z);
    // glow: light of its own (a lamp, a hint, an orb), the same in both layers and on every side. The city has less rim than what keeps its own colour.
    const float rimScale = input.tone.a > 0.5 ? 0.35 : 1.0;
    float3 shaded = StyleShade(albedo, n, l, v, skyLight, lightColor, sun, shine, input.knobs.w, lamps.diffuse, lamps.specular, rimScale, style);
    if (view == 4)
    {
        shaded = albedo * lamps.diffuse + lamps.specular;
    }
    // fog: what is far melts into the background; a thing that glows is seen through more of it (a lamp in the mist)
    float3 haze;
    const float fogOfModel = SceneFog(input.worldPosition, haze);
    const float3 offset = input.worldPosition - eye;
    const float metres = length(offset);
    haze = StyleHaze(haze, metres, style);
    const float fog = StyleFog(fogOfModel, metres, style) * (1.0 - 0.75 * saturate(input.knobs.w));
    // the light shafts: the light of the sun scattered by the air between the eye and this point, added in front of the fog
    const float3 shafts = LightShafts(offset / max(metres, 1.0e-4), metres, input.position.xy) * (1.0 - fog);
    return float4(Finish(lerp(shaded, haze, fog) + shafts, NdcOf(input.worldPosition)), 1.0);
}
