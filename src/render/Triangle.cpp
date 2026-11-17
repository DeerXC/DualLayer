#include "render/Triangle.h"

#include "core/Assert.h"
#include "core/Log.h"
#include "render/GpuDebug.h"
#include "render/Projection.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <d3dcompiler.h>

#include <cstddef>
#include <cstring>

using Microsoft::WRL::ComPtr;
using namespace DirectX;

namespace dl
{
namespace
{
struct Vertex
{
    XMFLOAT3 position; // UNITS: metres, world space (left-handed, Y up)
    XMFLOAT3 color;    // linear RGB
};

// KEEP IN SYNC: cbuffer PerObject in shaders/test_triangle.hlsl
struct PerObject
{
    XMFLOAT4X4 worldViewProj; // row-major, row vectors: v' = v * M
};
static_assert(sizeof(PerObject) % 16 == 0, "constant buffers must be a multiple of 16 bytes");
static_assert(sizeof(PerObject) == 64, "one float4x4");

constexpr char kShaderPath[] = "shaders/test_triangle.hlsl";

/// Compiles one entry point; logs the compiler's message on failure.
bool CompileShader(const char* entry, const char* target, ComPtr<ID3DBlob>& out)
{
    UINT flags = D3DCOMPILE_PACK_MATRIX_ROW_MAJOR | D3DCOMPILE_ENABLE_STRICTNESS;
#if defined(_DEBUG)
    flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
    flags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif
    wchar_t widePath[MAX_PATH];
    MultiByteToWideChar(CP_UTF8, 0, kShaderPath, -1, widePath, MAX_PATH);
    ComPtr<ID3DBlob> errors;
    const HRESULT hr = D3DCompileFromFile(widePath, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, entry, target, flags, 0, &out, &errors);
    if (FAILED(hr))
    {
        if (errors != nullptr)
        {
            DL_LOG_ERROR("Triangle", "shader %s (%s) failed:\n%s", entry, target, static_cast<const char*>(errors->GetBufferPointer()));
        }
        else
        {
            DL_LOG_ERROR("Triangle", "cannot read %s (0x%08X): is the working directory the project folder?", kShaderPath,
                         static_cast<unsigned>(hr));
        }
        return false;
    }
    return true;
}
} // namespace

bool Triangle::Init(ID3D11Device* device)
{
    ComPtr<ID3DBlob> vsBlob;
    ComPtr<ID3DBlob> psBlob;
    if (!CompileShader("VSMain", "vs_5_0", vsBlob) || !CompileShader("PSMain", "ps_5_0", psBlob))
    {
        return false;
    }
    HRESULT hr = device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &m_vertexShader);
    if (SUCCEEDED(hr))
    {
        hr = device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &m_pixelShader);
    }
    if (FAILED(hr))
    {
        DL_LOG_ERROR("Triangle", "creating the shader objects failed (0x%08X)", static_cast<unsigned>(hr));
        return false;
    }
    const D3D11_INPUT_ELEMENT_DESC layout[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Vertex, position), D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Vertex, color), D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    hr = device->CreateInputLayout(layout, ARRAYSIZE(layout), vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &m_inputLayout);
    if (FAILED(hr))
    {
        DL_LOG_ERROR("Triangle", "CreateInputLayout failed (0x%08X)", static_cast<unsigned>(hr));
        return false;
    }

    // Clockwise seen from the camera (top -> right -> left) = the FRONT face in Direct3D's default state.
    const Vertex vertices[3] = {
        {XMFLOAT3(0.0f, 0.8f, 0.0f), XMFLOAT3(0.0f, 1.0f, 0.0f)},   // top: green
        {XMFLOAT3(0.8f, -0.5f, 0.0f), XMFLOAT3(1.0f, 0.0f, 0.0f)},  // right: red
        {XMFLOAT3(-0.8f, -0.5f, 0.0f), XMFLOAT3(0.0f, 0.0f, 1.0f)}, // left: blue
    };
    D3D11_BUFFER_DESC vb{};
    vb.ByteWidth = sizeof(vertices);
    vb.Usage = D3D11_USAGE_IMMUTABLE;
    vb.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA init{};
    init.pSysMem = vertices;
    hr = device->CreateBuffer(&vb, &init, &m_vertexBuffer);
    if (FAILED(hr))
    {
        DL_LOG_ERROR("Triangle", "vertex buffer failed (0x%08X)", static_cast<unsigned>(hr));
        return false;
    }

    D3D11_BUFFER_DESC cb{};
    cb.ByteWidth = sizeof(PerObject);
    cb.Usage = D3D11_USAGE_DYNAMIC;
    cb.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cb.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    hr = device->CreateBuffer(&cb, nullptr, &m_constantBuffer);
    if (FAILED(hr))
    {
        DL_LOG_ERROR("Triangle", "constant buffer failed (0x%08X)", static_cast<unsigned>(hr));
        return false;
    }

    SetDebugName(m_vertexShader.Get(), "TestTriangle.VS");
    SetDebugName(m_pixelShader.Get(), "TestTriangle.PS");
    SetDebugName(m_inputLayout.Get(), "TestTriangle.InputLayout");
    SetDebugName(m_vertexBuffer.Get(), "TestTriangle.VertexBuffer");
    SetDebugName(m_constantBuffer.Get(), "TestTriangle.PerObjectCB");
    DL_LOG_INFO("Triangle", "ready");
    return true;
}

void Triangle::Shutdown()
{
    m_constantBuffer.Reset();
    m_vertexBuffer.Reset();
    m_inputLayout.Reset();
    m_pixelShader.Reset();
    m_vertexShader.Reset();
}

void Triangle::Draw(ID3D11DeviceContext* context, float aspect, float angleRadians, const XMMATRIX& view)
{
    DL_ASSERT(context != nullptr, "Triangle::Draw needs a context");
    DL_ASSERT(aspect > 0.0f, "aspect ratio must be positive");

    // World * View * Projection, left-handed, row vectors (docs/CONVENTIONS.md). No transpose anywhere.
    // The VIEW matrix now comes from the camera (scene/Camera.h); before this step it was a fixed camera at (0, 0, -3).
    const XMMATRIX world = XMMatrixRotationY(angleRadians);
    const XMMATRIX projection = MakeProjection(aspect); // the same lens as Models (render/Projection.h)

    PerObject data{};
    XMStoreFloat4x4(&data.worldViewProj, world * view * projection);

    D3D11_MAPPED_SUBRESOURCE mapped{};
    if (SUCCEEDED(context->Map(m_constantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
    {
        std::memcpy(mapped.pData, &data, sizeof(data));
        context->Unmap(m_constantBuffer.Get(), 0);
    }

    const UINT stride = sizeof(Vertex);
    const UINT offset = 0;
    ID3D11Buffer* vertexBuffer = m_vertexBuffer.Get();
    ID3D11Buffer* constantBuffer = m_constantBuffer.Get();
    context->IASetInputLayout(m_inputLayout.Get());
    context->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->VSSetShader(m_vertexShader.Get(), nullptr, 0);
    context->VSSetConstantBuffers(0, 1, &constantBuffer);
    context->PSSetShader(m_pixelShader.Get(), nullptr, 0);
    context->Draw(3, 0);
}
} // namespace dl

