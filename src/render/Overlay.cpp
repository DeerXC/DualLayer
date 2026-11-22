#include "render/Overlay.h"

#include "core/Log.h"
#include "render/GpuDebug.h"
#include "render/ShaderCompiler.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <d3dcompiler.h>

#include <cmath>
#include <cstddef>

using Microsoft::WRL::ComPtr;

namespace dl
{

namespace
{

constexpr char kShaderPath[] = "shaders/overlay.hlsl";

float SrgbToLinear(float c)
{
    return c <= 0.04045f ? c / 12.92f : std::pow((c + 0.055f) / 1.055f, 2.4f);
}

} // namespace

bool Overlay::Init(ID3D11Device* device)
{
    Shutdown();
    if (device == nullptr)
    {
        return false;
    }
    ComPtr<ID3DBlob> vsBlob;
    ComPtr<ID3DBlob> psBlob;
    if (!CompileShader(kShaderPath, "VSMain", "vs_5_0", vsBlob, "Overlay") || !CompileShader(kShaderPath, "PSMain", "ps_5_0", psBlob, "Overlay"))
    {
        return false;
    }
    HRESULT hr = device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &m_vertexShader);
    if (SUCCEEDED(hr))
    {
        hr = device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &m_pixelShader);
    }
    // a vertex is OverlayVertex: a position in clip space after the copy (8 bytes), then a linear colour with alpha (16 bytes)
    const D3D11_INPUT_ELEMENT_DESC layout[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(OverlayVertex, x), D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(OverlayVertex, r), D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    if (SUCCEEDED(hr))
    {
        hr = device->CreateInputLayout(layout, ARRAYSIZE(layout), vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &m_inputLayout);
    }
    if (SUCCEEDED(hr))
    {
        D3D11_BUFFER_DESC vb{};
        vb.ByteWidth = static_cast<UINT>(sizeof(OverlayVertex) * OverlayBatch::kMaxVertices);
        vb.Usage = D3D11_USAGE_DYNAMIC;
        vb.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        vb.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        hr = device->CreateBuffer(&vb, nullptr, &m_vertexBuffer);
    }
    if (SUCCEEDED(hr))
    {
        D3D11_BLEND_DESC blend{}; // over: colour * alpha + what is there * (1 - alpha)
        blend.RenderTarget[0].BlendEnable = TRUE;
        blend.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
        blend.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
        blend.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
        blend.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
        blend.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
        blend.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        blend.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        hr = device->CreateBlendState(&blend, &m_blendState);
    }
    if (SUCCEEDED(hr))
    {
        D3D11_RASTERIZER_DESC rasterizer{};
        rasterizer.FillMode = D3D11_FILL_SOLID;
        rasterizer.CullMode = D3D11_CULL_NONE; // flat 2D shapes: their winding must never hide them
        rasterizer.DepthClipEnable = TRUE;
        hr = device->CreateRasterizerState(&rasterizer, &m_rasterizerState);
    }
    if (SUCCEEDED(hr))
    {
        D3D11_DEPTH_STENCIL_DESC depth{};
        depth.DepthEnable = FALSE; // over everything, and nothing is written into the depth buffer
        depth.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
        depth.DepthFunc = D3D11_COMPARISON_ALWAYS;
        hr = device->CreateDepthStencilState(&depth, &m_depthState);
    }
    if (FAILED(hr))
    {
        DL_LOG_ERROR("Overlay", "creating the overlay objects failed (0x%08X)", static_cast<unsigned>(hr));
        Shutdown();
        return false;
    }
    SetDebugName(m_vertexShader.Get(), "Overlay.VS");
    SetDebugName(m_pixelShader.Get(), "Overlay.PS");
    SetDebugName(m_inputLayout.Get(), "Overlay.InputLayout");
    SetDebugName(m_vertexBuffer.Get(), "Overlay.VB");
    SetDebugName(m_blendState.Get(), "Overlay.Blend");
    SetDebugName(m_rasterizerState.Get(), "Overlay.Rasterizer");
    SetDebugName(m_depthState.Get(), "Overlay.Depth");
    DL_LOG_INFO("Overlay", "ready");
    return true;
}

void Overlay::Shutdown()
{
    m_depthState.Reset();
    m_rasterizerState.Reset();
    m_blendState.Reset();
    m_vertexBuffer.Reset();
    m_inputLayout.Reset();
    m_pixelShader.Reset();
    m_vertexShader.Reset();
}

void Overlay::Draw(ID3D11DeviceContext* context, const OverlayBatch& batch, int width, int height)
{
    if (context == nullptr || !m_vertexBuffer || batch.Count() <= 0 || width <= 0 || height <= 0)
    {
        return;
    }
    D3D11_MAPPED_SUBRESOURCE mapped{};
    if (FAILED(context->Map(m_vertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
    {
        return;
    }
    // pixels (top left, y down) to clip space (centre, y up); sRGB to linear
    auto* out = static_cast<OverlayVertex*>(mapped.pData);
    const float sx = 2.0f / static_cast<float>(width);
    const float sy = 2.0f / static_cast<float>(height);
    const OverlayVertex* in = batch.Data();
    for (int i = 0; i < batch.Count(); ++i)
    {
        out[i] = OverlayVertex{in[i].x * sx - 1.0f, 1.0f - in[i].y * sy, SrgbToLinear(in[i].r), SrgbToLinear(in[i].g), SrgbToLinear(in[i].b), in[i].a};
    }
    context->Unmap(m_vertexBuffer.Get(), 0);

    const float factor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    context->OMSetBlendState(m_blendState.Get(), factor, 0xffffffff);
    context->OMSetDepthStencilState(m_depthState.Get(), 0);
    context->RSSetState(m_rasterizerState.Get());
    context->IASetInputLayout(m_inputLayout.Get());
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ID3D11Buffer* vertexBuffer = m_vertexBuffer.Get();
    const UINT stride = sizeof(OverlayVertex);
    const UINT offset = 0;
    context->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
    context->VSSetShader(m_vertexShader.Get(), nullptr, 0);
    context->PSSetShader(m_pixelShader.Get(), nullptr, 0);
    context->Draw(static_cast<UINT>(batch.Count()), 0);

    // back to the defaults for whatever is drawn next
    context->OMSetBlendState(nullptr, nullptr, 0xffffffff);
    context->OMSetDepthStencilState(nullptr, 0);
    context->RSSetState(nullptr);
}

} // namespace dl
