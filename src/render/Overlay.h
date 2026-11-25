// Overlay.h - draws an OverlayBatch (render/OverlayBatch.h) over the finished picture: flat, see-through rectangles and
// text
//            in screen pixels, for the prompts and the small status line of the game.
// Owns:      the shaders (shaders/overlay.hlsl), the input layout, one dynamic vertex buffer of
// OverlayBatch::kMaxVertices,
//            the blend state (alpha), the rasterizer state (no culling) and the depth state (no test, no write).
// Order:     drawn last in the frame, after the models (and the test triangle), into the bound back buffer.
// State:     Draw sets everything it needs and, at the end, puts back the default blend, depth and rasterizer states,
// so the
//            next pass of the next frame starts from the defaults.
// Colour:    the batch holds sRGB colours; they are converted to linear when they are copied (the back buffer view is
// sRGB). Threading: main thread only.
#pragma once

#include "render/OverlayBatch.h"

#include <d3d11_1.h>
#include <wrl/client.h>

namespace dl
{

class Overlay
{
  public:
    /// Returns false (and logs why) if the shader does not compile or a resource cannot be created; nothing is left
    /// half-made.
    bool Init(ID3D11Device* device);
    void Shutdown();

    /// `width` and `height`: the size of the bound render target in pixels.
    void Draw(ID3D11DeviceContext* context, const OverlayBatch& batch, int width, int height);

  private:
    Microsoft::WRL::ComPtr<ID3D11VertexShader> m_vertexShader;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> m_pixelShader;
    Microsoft::WRL::ComPtr<ID3D11InputLayout> m_inputLayout;
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_vertexBuffer;
    Microsoft::WRL::ComPtr<ID3D11BlendState> m_blendState;
    Microsoft::WRL::ComPtr<ID3D11RasterizerState> m_rasterizerState;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> m_depthState;
};

} // namespace dl
