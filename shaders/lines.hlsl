// lines.hlsl - the ink lines of docs/DESIGN.md 6.10 (S2b): one full-screen pass over the depth buffer of a finished picture, after
//              everything opaque and before the water (so that the water is blended over the lines, and the lines of the sea bed
//              are as faint as the bed is).
// Slots:       t1 = the depth buffer of the picture (a typeless depth texture with a shader view: render/Device.cpp, render/Models.cpp),
//              b0 = Lines. t0 is left alone: it is the shadow map of the scene.
// Colour:      the picture is not read. The ink is added with the blend state ONE, INV_SRC_ALPHA (render/Models.cpp, m_lineBlend):
//              result = source.rgb + picture * (1 - source.a), which is lines.hlsli LineComposite exactly (lines_blend.hlsli). The
//              back buffer is written through an sRGB view, so the blend is done in linear light.
// Geometry:    one triangle that covers the whole screen, made from SV_VertexID: the vertex shader of ripple.hlsl is used (no vertex
//              buffer, no input layout), this file has the pixel shader only.
// The maths:   lines.hlsli (tested without a GPU in tests/test_style.cpp). Here: reading the depth, the square of pixels round the one
//              that is drawn, and the blend. tests/test_style.cpp PassCoverage is this pass written again in C++.
// KEEP IN SYNC: struct LineConstants in src/render/LineSettings.h (field for field; 80 bytes).
#include "lines.hlsli"
#include "lines_blend.hlsli"

cbuffer Lines : register(b0)
{
    float4 lineDepth;  // x: near plane, y: far plane (metres), z: PixelSlope, w: jump (relative inverse depth)
    float4 lineLevels; // x: silhouette level, y: crease start, z: crease full (1 - cos of the turn), w: master amount
    float4 lineFade;   // x: crease fade near, y: crease fade far (metres), z: far alpha, w: local mix
    float4 lineWidth;  // x: outline width, y: inner edge width (pixels), z: tan of half the view across, w: up
    float4 lineInk;    // rgb: the colour of the line (linear)
};

Texture2D<float> depthTexture : register(t1);

// The inverse depth (1 / metres) at a pixel; a pixel outside the picture is the nearest one inside it.
float InverseDepthAt(int2 pixel, int2 size)
{
    const int2 inside = clamp(pixel, int2(0, 0), size - int2(1, 1));
    return InverseViewDepth(depthTexture.Load(int3(inside, 0)), lineDepth.x, lineDepth.y);
}

// The edge at a pixel (lines.hlsli LineEdge); none on the outermost row and column of the picture.
float3 EdgeAt(int2 pixel, int2 size)
{
    if (pixel.x < 1 || pixel.y < 1 || pixel.x > size.x - 2 || pixel.y > size.y - 2)
    {
        return float3(0.0, 0.0, 1000.0);
    }
    const float sx = ((float(pixel.x) + 0.5) / float(size.x) * 2.0 - 1.0) * lineWidth.z;
    const float sy = (1.0 - (float(pixel.y) + 0.5) / float(size.y) * 2.0) * lineWidth.w;
    return LineEdge(sx, sy, lineDepth.z, InverseDepthAt(pixel, size), InverseDepthAt(pixel + int2(-1, 0), size),
                    InverseDepthAt(pixel + int2(1, 0), size), InverseDepthAt(pixel + int2(0, -1), size),
                    InverseDepthAt(pixel + int2(0, 1), size), lineLevels.x, lineLevels.y, lineLevels.z, lineDepth.w);
}

struct VSOut
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
};

float4 PSMain(VSOut input) : SV_Target
{
    uint width;
    uint height;
    depthTexture.GetDimensions(width, height);
    const int2 size = int2(int(width), int(height));
    const int2 pixel = int2(input.position.xy);
    // a line `w` pixels wide reaches ceil(w / 2 + 0.5) - 1 pixels beyond the middle of an edge pixel (lines.hlsli InkReach); the square
    // that is looked at is that many pixels round the one that is drawn (at least 1, at most 4)
    const float widest = max(lineWidth.x, lineWidth.y);
    const int reach = int(min(max(ceil(0.5 * widest + 0.5) - 1.0, 1.0), 4.0));
    float coverage = 0.0;
    [loop]
    for (int j = -reach; j <= reach; ++j)
    {
        [loop]
        for (int i = -reach; i <= reach; ++i)
        {
            const float3 edge = EdgeAt(pixel + int2(i, j), size);
            if (edge.x > 0.0 || edge.y > 0.0)
            {
                coverage = max(coverage, LineCoverage(edge, length(float2(float(i), float(j))), lineWidth.x, lineWidth.y,
                                                      lineFade.x, lineFade.y, lineFade.z));
            }
        }
    }
    if (coverage <= 0.0)
    {
        discard; // nothing to add: the picture stays as it is
    }
    return float4(LineBlendColor(coverage, lineInk.rgb, lineFade.w, lineLevels.w),
                  LineBlendAlpha(coverage, lineFade.w, lineLevels.w));
}
