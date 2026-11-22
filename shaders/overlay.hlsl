// overlay.hlsl - flat, see-through shapes over the finished picture: the prompts and the status line (render/Overlay.cpp).
// Input:   the position is already in clip space (Overlay::Draw converts the pixels), the colour is linear with alpha.
// Colour:  returned as it comes; the back buffer is written through an sRGB view and the blend state mixes by alpha.
// KEEP IN SYNC: struct OverlayVertex in src/render/OverlayBatch.h (2 + 4 floats = 24 bytes) and the input layout in Overlay.cpp.
#include "common.hlsli"

struct VSIn
{
    float2 position : POSITION;
    float4 color : COLOR;
};

struct VSOut
{
    float4 position : SV_Position;
    float4 color : COLOR;
};

VSOut VSMain(VSIn input)
{
    VSOut output;
    output.position = float4(input.position, 0.0, 1.0);
    output.color = input.color;
    return output;
}

float4 PSMain(VSOut input) : SV_Target
{
    return input.color;
}
