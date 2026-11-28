// lines_blend.hlsli - the ink of docs/DESIGN.md 6.10 (S2b) as a blend: the pass of shaders/lines.hlsl adds the line over the
//                     picture without reading it. Include shaders/lines.hlsli first (this file uses nothing but lerp and saturate).
// Why:                with c = coverage * amount, lines.hlsli LineComposite is
//                       scene * (1 - c + c * 0.3 * localMix) + c * (1 - localMix) * lineColor,
//                     and the blend state ONE, INV_SRC_ALPHA (result = source.rgb + picture * (1 - source.a)) gives exactly that
//                     for source.rgb = LineBlendColor and source.a = LineBlendAlpha. tests/test_style.cpp checks it for any picture,
//                     coverage and master amount. No copy of the picture is needed.
#ifndef DL_LINES_BLEND_HLSLI
#define DL_LINES_BLEND_HLSLI

// The colour that the pass returns (premultiplied: the part of the line colour that is added).
float3 LineBlendColor(float coverage, float3 lineColor, float localMix, float amount)
{
    const float c = saturate(coverage * amount);
    return lineColor * (c * (1.0f - localMix));
}

// The alpha that the pass returns: how much of the picture stays under the line.
float LineBlendAlpha(float coverage, float localMix, float amount)
{
    return saturate(coverage * amount) * (1.0f - 0.3f * localMix);
}

#endif
