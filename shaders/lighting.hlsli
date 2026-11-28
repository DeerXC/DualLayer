// lighting.hlsli - the maths of the light that needs no texture and no constant buffer: the fall-off of a lamp, the shading of one
//                  lamp, the shoulder that bends a light that is too bright, the fade at the edge of the shadow map, the glow round a
//                  lamp and the haze toward the sun; and, since the style of the light (docs/DESIGN.md 6.3 and 6.10), the hard steps
//                  of the sun, the cool fill of a shadow, the shadows of the clouds, the rim of an edge, the rings and the colour of a
//                  lamp, the grade of the picture (split tone, saturation, vignette) and the phase of the light shafts.
// Why a file of its own: this is written in the part of HLSL that is also C++ (no swizzles, no out parameters, no textures), so
//                  tests/test_lighting.cpp includes it through a stand-in for the vector types (tests/hlsl_shim.h) and checks the
//                  numbers with the compiler of the tests: the shader file itself cannot be run without a GPU.
// Units:           metres; colours are linear (docs/CONVENTIONS.md).
// KEEP IN SYNC:    nothing in C++ depends on the numbers here except the defaults of render/LightingSettings.h (a lamp of strength 18
//                  under a street lamp, 3.4 m over the ground, lights it with 1.4).
#ifndef DL_LIGHTING_HLSLI
#define DL_LIGHTING_HLSLI

// How much of its strength a lamp gives at `dist` metres: the inverse square, softened near the lamp (+ 1.5 m^2) and multiplied by a
// smooth window that is exactly 0 at `range` (saturate(1 - (d / r)^4)^2: no hard edge to the circle of light).
float LampAttenuation(float dist, float range)
{
    const float x = saturate(dist / max(range, 0.001f));
    const float x2 = x * x;
    const float window = saturate(1.0f - x2 * x2);
    return window * window / (dist * dist + 1.5f);
}

struct LampLight
{
    float3 diffuse;  // what it adds to a surface that has an albedo (multiply by it)
    float3 specular; // what it adds as a highlight (already has the strength of the shine)
};

// The light of a lamp in soft rings instead of one smooth slope: `att` (LampAttenuation) is taken to the power 0.45 (about what the
// eye sees), cut into four steps whose edges are smooth (smoothstep over 0.22 .. 0.78 of each step) and taken back. Continuous, only
// rising with `att`, 0 at 0; `amount` 0 leaves `att` as it is, 1 is the full rings.
float LampRings(float att, float amount)
{
    const float p = pow(saturate(att * 1.5f), 0.45f);
    const float t = p * 4.0f;
    const float whole = floor(t);
    const float q = (whole + smoothstep(0.22f, 0.78f, t - whole)) * 0.25f;
    const float back = pow(q, 1.0f / 0.45f) / 1.5f;
    return lerp(att, back, amount);
}

// The colour of a lamp over its pool: the core a little whiter, the edge a deeper amber, as a flame is. `att` is the fall-off
// (LampAttenuation); `amount` 0 leaves the colour as it is.
float3 LampWarmth(float3 colour, float att, float amount)
{
    const float core = saturate(att * 3.0f);
    const float3 edge = float3(1.0f, 0.80f, 0.58f);
    const float3 hot = float3(1.0f, 0.93f, 0.80f);
    return colour * lerp(float3(1.0f, 1.0f, 1.0f), lerp(edge, hot, core), amount);
}

// One lamp at `lp` with the range `lr` and the colour `lc` (colour times strength), on a surface at `p` with the normal `n`, seen from
// the direction `v` (to the eye). `shine`: strength of the highlight, `power`: how sharp it is (params y and z of a surface). `style`
// 0 .. 1: how much of the rings and the warmth of the flame the pool has (0: the smooth light of LampShade).
LampLight LampShadeStyled(float3 p, float3 n, float3 v, float3 lp, float lr, float3 lc, float shine, float power, float style)
{
    LampLight r;
    r.diffuse = float3(0.0f, 0.0f, 0.0f);
    r.specular = float3(0.0f, 0.0f, 0.0f);
    const float3 d = lp - p;
    const float dist = length(d);
    if (dist >= lr)
    {
        return r;
    }
    const float3 l = d / max(dist, 1.0e-4f);
    const float ndl = saturate(dot(n, l));
    const float att = LampAttenuation(dist, lr);
    const float3 received = LampWarmth(lc, att, style) * LampRings(att, style);
    r.diffuse = received * ndl;
    if (shine > 0.0f)
    {
        const float3 h = normalize(l + v);
        r.specular = received * (pow(saturate(dot(n, h)), max(power, 1.0f)) * shine * step(0.0001f, ndl));
    }
    return r;
}

// The smooth lamp of the first light (no rings, no warmth): LampShadeStyled with a style of 0.
LampLight LampShade(float3 p, float3 n, float3 v, float3 lp, float lr, float3 lc, float shine, float power)
{
    return LampShadeStyled(p, n, v, lp, lr, lc, shine, power, 0.0f);
}

// A light that is brighter than `knee` is bent toward 1 (it never reaches it) instead of being cut off at 1 by the screen; one that
// is not is left as it is. The three colours are scaled together, so the hue of a bright lamp stays. The slope at the knee is 1:
// there is no visible start of the shoulder.
float3 SoftShoulder(float3 c, float knee)
{
    const float peak = max(c.x, max(c.y, c.z));
    if (peak <= knee)
    {
        return c;
    }
    const float room = 1.0f - knee;
    const float over = peak - knee;
    const float bent = knee + room * (over / (over + room));
    return c * (bent / peak);
}

// 1 inside the shadow map, falling to 0 over the last tenth of its width: the shadow ends softly at the edge of the box that follows
// the camera instead of with a line. (u, v): where the point is in the map, 0 .. 1.
float ShadowEdgeFade(float u, float v)
{
    const float edge = min(min(u, 1.0f - u), min(v, 1.0f - v));
    return saturate(edge * 10.0f);
}

// The glow round a lamp, from how squarely the soft sphere that carries it is seen (|n . v|, 0 .. 1): 1 in the middle, 0 at the rim.
// A broad soft skirt (the cube of the facing) with a brighter heart (its eighth power): the glow has no visible edge and the
// flame seems to be in the middle of it. Exactly 1 at 1 and 0 at 0, only rising.
float HaloFalloff(float facing)
{
    const float a = saturate(abs(facing));
    const float broad = a * a * a;
    const float heart = pow(a, 8.0f);
    return broad + (heart - broad) * 0.25f;
}

// The light of the sun scattered by the haze toward the sun: `cosine` is the cosine of the angle between the ray of sight and the
// direction to the sun; `amount` how strong the effect is.
float3 SunScatter(float3 sun, float cosine, float amount)
{
    return sun * (pow(saturate(cosine), 6.0f) * amount);
}

// ---- the style of the light --------------------------------------------------------------------------------------------------
// The light of the town is not photographic. The sun falls on a face in hard steps (the faces of a low-poly town read as separate
// planes, and a flat fill in three tones is the look of docs/DESIGN.md 6.10), the sun-less side of everything takes a cool fill against
// the warm light, clouds drift their shadows over the stone, an edge
// that faces the light has a rim, a lamp lights its pool in rings, rays of light stand in the air, and the picture is graded (the
// darks leaning one way, the lights the other, a little more colour, a soft dark corner). Every amount below is 0 .. 1 and 0 is the
// plain light that was before: the master `style` and the debug view "plain" of the game (key L) set all of them to 0.

struct StyleParams
{
    float toon;       // 0 .. 1: how much of the sun falls in hard steps (ToonRampEdge)
    float rim;        // the strength of the rim of an edge (RimTerm); 0: none
    float saturation; // 1 = as it is; the grade (StyleGrade)
    float vignette;   // 0 .. 1: how much darker the corners of the picture are
    float contrast;   // 1 = as it is: the light is raised to this power round the middle grey (StyleGrade)
    float3 cool;      // what the sky light is multiplied by where the sun does not reach (a colour with a mean near 1)
    float coolAmount; // 0 .. 1: how far toward `cool`
    float3 low;       // the grade of the darks (a multiplier)
    float lowAmount;
    float3 high;      // the grade of the lights (a multiplier)
    float highAmount;
    float cloud;      // 0 .. 1: how much of the sun a cloud takes (CloudShade)
    float lampRings;  // 0 .. 1: the rings and the warmth of a lamp (LampShadeStyled)
    float shaft;      // the strength of the light shafts; 0: none
    float shaftSteps; // how many samples along a ray of sight
    float wash;       // 0 .. 1: the painted blotches of the stone (PaintAlbedo)
    float damp;       // 0 .. 1: how much darker and cooler the stone is near the water (PaintAlbedo)
    float3 farTint;   // the colour whose hue the haze leans to far away (StyleHaze); only its hue counts
    float farAmount;  // 0 .. 1: how far
    float farFog;     // 0 .. 1: the extra fog of the far distance (StyleFarFog): the far hills and the sea go pale
    float toonEdge;   // UNITS: cosine: the soft edge of a step of the sun (ToonRampEdge); 0: hard. The pixel shader sets it from fwidth
};

// A step: 0 below `t`, 1 above it, and between t - edge / 2 and t + edge / 2 a smooth rise. `edge` is the width of that rise (0: a hard step;
// the narrowest that is divided by is 0.0001).
float ToonStep(float x, float t, float edge)
{
    const float k = saturate((x - t) / max(edge, 1.0e-4f) + 0.5f);
    return k * k * (3.0f - 2.0f * k);
}

// How much of a surface's cosine to the sun remains after the hard steps of the sun (docs/DESIGN.md 6.10): three tones. None of the sun below a
// cosine of 0.14 (the face is turned from it and has only the sky), 0.46 of it from 0.14 (a roof or the ground under a low sun), all of it from
// 0.52 (a wall that faces the sun). Each edge is `edge` wide, in cosine units: the pixel shader passes the change of the cosine over one or two
// pixels, so that the edge of a step on a round thing is as soft as the edge of a drawn line and a flat face (which has no change) is flat.
// `amount` 0 leaves the cosine as it is. 0 at 0, 1 at 1 (to a rounding), only rising.
float ToonRampEdge(float x, float amount, float edge)
{
    const float banded = 0.46f * ToonStep(x, 0.14f, edge) + 0.54f * ToonStep(x, 0.52f, edge);
    return lerp(x, banded, amount);
}

// The same with hard steps (the reference of the tests).
float ToonRamp(float x, float amount)
{
    return ToonRampEdge(x, amount, 0.0f);
}

// A hash of a point of a grid, 0 .. 1.
float Hash21(float x, float z)
{
    return frac(sin(x * 127.1f + z * 311.7f) * 43758.5453f);
}

// Smooth value noise, 0 .. 1.
float CloudNoise(float x, float z)
{
    const float ix = floor(x);
    const float iz = floor(z);
    const float fx = x - ix;
    const float fz = z - iz;
    const float ux = fx * fx * (3.0f - 2.0f * fx);
    const float uz = fz * fz * (3.0f - 2.0f * fz);
    const float a = Hash21(ix, iz);
    const float b = Hash21(ix + 1.0f, iz);
    const float c = Hash21(ix, iz + 1.0f);
    const float d = Hash21(ix + 1.0f, iz + 1.0f);
    return lerp(lerp(a, b, ux), lerp(c, d, ux), uz);
}

// 1 where a cloud stands over the point (x, z) of the plane of the clouds, 0 where the sky is open; the edge is soft but short, like a
// painted one. Patches of about 25 m that drift by 0.5 m a second.
float CloudCover(float x, float z, float time)
{
    const float u = x * 0.045f + time * 0.022f;
    const float w = z * 0.045f + time * 0.009f;
    const float n = CloudNoise(u, w) * 0.65f + CloudNoise(u * 2.3f + 7.1f, w * 2.3f + 3.7f) * 0.35f;
    return smoothstep(0.48f, 0.64f, n);
}

// What is left of the sun at the point `p` when the clouds take `amount` of it: the point is moved along the light to the plane of the
// clouds (y = 0, with a light that is never lower than 0.3 of vertical so that a low sun does not stretch the patches without end).
float CloudShade(float3 p, float3 toLight, float time, float amount)
{
    const float k = p.y / max(toLight.y, 0.3f);
    return 1.0f - amount * CloudCover(p.x - toLight.x * k, p.z - toLight.z * k, time);
}

// The rim of an edge: 1 where the surface is seen edge-on (`ndv`, the cosine to the eye, near 0), 0 where it is seen squarely; the
// more where the light falls on it (`ndl`) and none on a floor (`ny`, the y of the normal: a floor seen from afar is all edge).
float RimTerm(float ndv, float ndl, float ny)
{
    const float edge = pow(saturate(1.0f - ndv), 3.0f);
    const float towardLight = 0.3f + 0.7f * saturate(ndl);
    const float notFloor = 1.0f - saturate(abs(ny) * 1.25f);
    return edge * towardLight * notFloor;
}

// The stone of the city painted by hand: large soft blotches in value and in hue (patches of about 10 m; on a wall they are tall, like
// stains where the rain runs), and the damp of the foot of the town (`lowMist`, 0 .. 1: how near the water the point is, from the
// shader's LowMist): darker and cooler. Returns the new albedo; a `wash` and a `damp` of 0 give it back as it was.
float3 PaintAlbedo(float3 albedo, float3 p, float3 n, float lowMist, StyleParams s)
{
    const bool floorLike = abs(n.y) > 0.6f;
    const float u = floorLike ? p.x : p.x + p.z;
    const float w = floorLike ? p.z : p.y * 0.5f;
    const float raw = CloudNoise(u * 0.09f, w * 0.09f) * 0.65f + CloudNoise(u * 0.31f + 11.3f, w * 0.31f + 5.1f) * 0.35f;
    const float big = smoothstep(0.30f, 0.70f, raw); // value noise stays near 0.5: stretch it so that the blotches can be seen
    const float hue = smoothstep(0.25f, 0.75f, CloudNoise(u * 0.06f + 3.7f, w * 0.06f + 9.2f)) - 0.5f;
    const float value = lerp(1.0f, lerp(0.80f, 1.14f, big), s.wash);
    const float3 shift = float3(1.0f + 0.22f * hue * s.wash, 1.0f, 1.0f - 0.22f * hue * s.wash);
    const float wet = lowMist * s.damp;
    const float3 damp = lerp(float3(1.0f, 1.0f, 1.0f), float3(0.62f, 0.74f, 0.86f), wet);
    return albedo * shift * damp * value;
}

// The haze toward the horizon, leaning toward the HUE of `farTint` the farther it is: nothing within 24 m, all of it from 200 m (6.10: the
// middle distance, the town seen from the air, is already cooler than what is near, and the far hills are lavender and not orange). The
// brightness of the haze is kept: only its hue moves. (A multiplier cannot do it: an orange horizon would stay orange.)
float3 StyleHaze(float3 haze, float metres, StyleParams s)
{
    const float3 grey = float3(0.2126f, 0.7152f, 0.0722f);
    const float amount = smoothstep(24.0f, 200.0f, metres) * s.farAmount;
    const float3 leaning = s.farTint * (dot(haze, grey) / max(dot(s.farTint, grey), 1.0e-4f));
    return lerp(haze, leaning, amount);
}

// More fog far away than the fog of the model has (6.10, aerial perspective): none within 100 m, all of `farFog` from 280 m. The distance
// goes paler, as in the pictures that the look follows; the near and the middle are left as they are.
float StyleFarFog(float metres, StyleParams s)
{
    return smoothstep(100.0f, 280.0f, metres) * s.farFog;
}

// The fog of a point: the fog of the model and the far fog of the style, one over the other (a part of the light is lost in each).
float StyleFog(float fog, float metres, StyleParams s)
{
    return 1.0f - (1.0f - fog) * (1.0f - StyleFarFog(metres, s));
}

// What a surface looks like in the style: the light that is the sum of the plain one (the sky light, the sun in hard steps, the highlight, the
// glow of the surface and the lamps), the cool fill of the shadow and the rim. `sun`: what the shadow and the clouds leave of the sun (0 ..
// 1); `shine`: the highlight (already with its strength); `glow`: the light of its own; `lampDiffuse` and `lampSpecular`: the lamps
// (LampShadeStyled); `rimScale`: 1 for the things that keep their own colour, less for the stone of the city.
float3 StyleShade(float3 albedo, float3 n, float3 l, float3 v, float3 skyLight, float3 lightColor, float sun, float shine, float glow,
                  float3 lampDiffuse, float3 lampSpecular, float rimScale, StyleParams s)
{
    const float diffuse = saturate(dot(n, l));
    const float litAmount = ToonRampEdge(diffuse, s.toon, s.toonEdge) * sun;
    // the sky light of a surface that the sun does not reach leans toward the cool colour
    const float3 sky = skyLight * lerp(float3(1.0f, 1.0f, 1.0f), s.cool, (1.0f - litAmount) * s.coolAmount);
    const float3 plain = albedo * sky + albedo * lightColor * litAmount + lightColor * (shine * sun) + albedo * glow +
                         albedo * lampDiffuse + lampSpecular;
    const float rim = RimTerm(saturate(dot(n, v)), diffuse, n.y) * s.rim * rimScale * (0.4f + 0.6f * sun);
    return plain + lightColor * rim;
}

// The contrast of the picture: the colour is raised to the power `contrast` round the middle grey of 0.18 (linear), so that what is
// darker than that gets darker and what is lighter gets lighter; 1 leaves it as it is. Per channel, so that a colour keeps its hue.
float3 StyleContrast(float3 c, float contrast)
{
    const float k = 0.18f;
    return float3(pow(max(c.x, 0.0f) / k, contrast) * k, pow(max(c.y, 0.0f) / k, contrast) * k, pow(max(c.z, 0.0f) / k, contrast) * k);
}

// The grade of the picture: the contrast, the darks multiplied toward `low`, the lights toward `high` (by the luminance), the colour pushed away from
// grey by `saturation`, and the corners darkened by `vignette` (ndcX and ndcY: the place on the screen, -1 .. 1). With the defaults
// of a plain light (amounts 0, saturation 1, vignette 0) the colour is returned as it is.
float3 StyleGrade(float3 c0, StyleParams s, float ndcX, float ndcY)
{
    const float3 grey = float3(0.2126f, 0.7152f, 0.0722f);
    const float3 c = StyleContrast(c0, s.contrast);
    const float lum = dot(c, grey);
    const float lowWeight = 1.0f - smoothstep(0.0f, 0.30f, lum);
    const float highWeight = smoothstep(0.30f, 1.1f, lum);
    float3 g = c * lerp(float3(1.0f, 1.0f, 1.0f), s.low, lowWeight * s.lowAmount);
    g = g * lerp(float3(1.0f, 1.0f, 1.0f), s.high, highWeight * s.highAmount);
    const float after = dot(g, grey);
    g = lerp(float3(after, after, after), g, s.saturation);
    g = float3(max(g.x, 0.0f), max(g.y, 0.0f), max(g.z, 0.0f));
    const float corner = smoothstep(0.55f, 1.45f, sqrt(ndcX * ndcX + ndcY * ndcY));
    return g * (1.0f - s.vignette * corner);
}

// How much of the light the air scatters toward the eye, by the cosine between the ray of sight and the direction to the light: a floor
// of 0.04 all round (a shaft is hardly seen away from the light), up to 1 straight toward it.
float ShaftPhase(float cosine)
{
    return 0.04f + 0.96f * pow(saturate(cosine), 5.0f);
}

// What of a ray of sight is a shaft: `litShare` (0 .. 1) is the part of the samples along it that the light reaches. A ray that is all
// in the light is plain haze (the fog has that already: the sky and the distance keep their gradient), a ray that is all in the
// shadow of a building has nothing, and a ray that crosses the edge of a shadow is where a shaft is seen: 4 m (1 - m), 1 at one half,
// 0 at both ends.
float ShaftEdge(float litShare)
{
    const float m = saturate(litShare);
    return 4.0f * m * (1.0f - m);
}

#endif
