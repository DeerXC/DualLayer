// ripple.hlsl - puts two finished pictures of the scene (layer images) on the screen: one outside a circle, one inside it.
// Slots:   t0 = the picture OUTSIDE the circle, t1 = the picture INSIDE it, s0 = linear sampler, b0 = Ripple.
// Colour:  the textures are sRGB views: Sample() gives LINEAR colour, and the back buffer is written through an sRGB view,
//          so no gamma curve is applied here (see docs/CONVENTIONS.md).
// Geometry: one triangle that covers the whole screen, made from SV_VertexID (no vertex buffer, no input layout).
// KEEP IN SYNC: struct RippleConstants in src/render/Models.cpp (field for field; 48 bytes).

Texture2D outerImage : register(t0);
Texture2D innerImage : register(t1);
SamplerState linearClamp : register(s0);

cbuffer Ripple : register(b0)
{
    float2 center;    // the middle of the circle, 0 .. 1 over the screen (0, 0 = top left)
    float radius;     // in units of the screen HEIGHT (so that the circle is round on a wide screen)
    float ringWidth;  // the same units: how wide the bright ring is
    float aspect;     // screen width / screen height
    float distortion; // how far the picture is bent at the ring, in the units of uv
    float2 pad0;
    float3 glowColor; // linear colour that is added at the ring
    float pad1;
};

struct VSOut
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
};

VSOut VSMain(uint id : SV_VertexID)
{
    // ids 0, 1, 2 give the uv (0, 0), (2, 0), (0, 2): one triangle that is twice as big as the screen
    VSOut output;
    output.uv = float2((id << 1) & 2, id & 2);
    output.position = float4(output.uv * float2(2.0, -2.0) + float2(-1.0, 1.0), 0.0, 1.0);
    return output;
}

float4 PSMain(VSOut input) : SV_Target
{
    // the distance from the centre, measured in screen-height units
    const float2 offset = input.uv - center;
    const float2 d = float2(offset.x * aspect, offset.y);
    const float dist = max(length(d), 1.0e-5);
    const float2 dir = d / dist;

    // a ring around the edge of the circle: 1 on the edge, 0 at ringWidth away, with a soft shape
    float ring = saturate(1.0 - abs(dist - radius) / max(ringWidth, 1.0e-4));
    ring = ring * ring * (3.0 - 2.0 * ring);

    // the picture is pushed a little along the ring: it looks like the ring is a lens
    const float2 shift = float2(dir.x / aspect, dir.y) * ring * distortion;
    const float2 uv = input.uv - shift;
    const float3 outerColor = outerImage.Sample(linearClamp, uv).rgb;
    const float3 innerColor = innerImage.Sample(linearClamp, uv).rgb;

    // 1 inside the circle, 0 outside, with a soft edge that is a part of the ring
    const float edge = max(ringWidth * 0.2, 1.0e-4);
    const float inside = 1.0 - smoothstep(radius - edge, radius + edge, dist);

    float3 color = lerp(outerColor, innerColor, inside);
    color += glowColor * ring * ring;
    return float4(color, 1.0);
}
