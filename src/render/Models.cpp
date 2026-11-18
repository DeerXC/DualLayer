#include "render/Models.h"

#include "core/Assert.h"
#include "core/Log.h"
#include "render/GpuDebug.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <d3dcompiler.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <utility>

using Microsoft::WRL::ComPtr;
using namespace DirectX;

namespace dl
{

namespace
{

constexpr char kModelsShaderPath[] = "shaders/models.hlsl";
constexpr char kRippleShaderPath[] = "shaders/ripple.hlsl";
constexpr int kDetailLevels[] = {8, 16, 32, 64};

// KEEP IN SYNC: cbuffer Frame and cbuffer Object in shaders/models.hlsl (field for field).
struct FrameConstants
{
    XMFLOAT4X4 viewProjection; // row-major, row vectors: v' = v * M
    XMFLOAT3 eye;              // UNITS: metres, world space
    float pad0;
    XMFLOAT3 toLight; // unit vector from a surface TO the light
    float ambient;
    XMFLOAT3 lightColor; // linear
    float pad1;
    XMFLOAT3 skyUp; // linear: the sky light on a surface that looks up
    float pad2;
    XMFLOAT3 skyDown; // linear: the ground light on a surface that looks down
    float pad3;
};

struct ObjectConstants
{
    XMFLOAT4X4 world;
    XMFLOAT4X4 normalMatrix; // inverse transpose of the 3x3 of `world`
    XMFLOAT4 color;          // linear albedo
    XMFLOAT4 params;         // x: checkered floor, y: shine strength, z: shininess
};

// KEEP IN SYNC: cbuffer Ripple in shaders/ripple.hlsl (field for field).
struct RippleConstants
{
    XMFLOAT2 center; // 0 .. 1 over the screen
    float radius;    // UNITS: screen heights
    float ringWidth; // UNITS: screen heights
    float aspect;    // width / height
    float distortion;
    float pad0[2];
    XMFLOAT3 glowColor; // linear
    float pad1;
};

static_assert(sizeof(FrameConstants) == 144, "cbuffer Frame is 9 registers of 16 bytes");
static_assert(sizeof(ObjectConstants) == 160, "cbuffer Object is 10 registers of 16 bytes");
static_assert(sizeof(RippleConstants) == 48, "cbuffer Ripple is 3 registers of 16 bytes");

// ---- the scene
// ---------------------------------------------------------------------------------------------------------- The floor
// is a cube squashed flat: its TOP is at y = kFloorY. Every object stands on it.
constexpr float kFloorY = -1.0f;
// The camera starts at (0, 0, -3) looking along +Z (scene/Camera.h): the row of shapes is 6 metres in front of it.
constexpr float kRowZ = 3.0f;
constexpr float kTorusMajor = 0.8f; // UNITS: metres, the middle of the tube
constexpr float kTorusMinor = 0.3f; // UNITS: metres, the radius of the tube

// ---- the layers (M2). A layer is a LABEL on an object, a bit of a mask: A = bit 0, B = bit 1, both = 3.
constexpr unsigned kLayerA = 1u << 0;
constexpr unsigned kLayerB = 1u << 1;
constexpr unsigned kLayerBoth = kLayerA | kLayerB;

// ---- the look of the layers (M2). The built-in values; [layer_a] / [layer_b] in config/default.ini change them.
// A is a warm world (a low red sun), B is a cool world (a blue moon from the other side). All colours are linear.
constexpr float kSkyDownScale = 0.28f; // the ground light is this part of the sky light (the same ratio as in M1)

// ---- the ripple (M2). The built-in values; [ripple] in config/default.ini changes them.
constexpr float kDefaultRippleSeconds = 0.9f;     // UNITS: seconds for the circle to cover the screen
constexpr float kDefaultRippleWidth = 0.07f;      // UNITS: screen heights
constexpr float kDefaultRippleDistortion = 0.012f; // UNITS: uv (a part of the screen)
constexpr float kRippleGlow = 0.5f;               // the ring adds this much of the light colour of the layer it opens
constexpr float kRippleOvershoot = 1.5f;          // the circle grows this many ring widths past the corner, so the ring leaves the screen

std::array<Models::Theme, 2> DefaultThemes()
{
    Models::Theme a;
    a.toLight = XMFLOAT3(-0.45f, 0.80f, -0.55f); // above, left of and behind the camera (the light of M1)
    a.lightColor = XMFLOAT3(1.25f, 1.02f, 0.78f);
    a.sky = XMFLOAT3(0.19f, 0.12f, 0.09f);
    a.clear = XMFLOAT3(0.035f, 0.012f, 0.008f);
    Models::Theme b;
    b.toLight = XMFLOAT3(0.55f, 0.65f, -0.45f); // above, right of and behind the camera: the shadows turn round
    b.lightColor = XMFLOAT3(0.78f, 0.95f, 1.25f);
    b.sky = XMFLOAT3(0.08f, 0.13f, 0.25f);
    b.clear = XMFLOAT3(0.006f, 0.014f, 0.040f);
    return {{a, b}};
}

struct SceneObject
{
    Models::Shape shape;
    float position[3]; // UNITS: metres, the centre of the object
    float scale[3];    // UNITS: metres: a shape is 1 metre big (a sphere: across), this stretches it
    float tilt;        // radians, round X, before the turn
    float spinSpeed;   // radians per second, round Y (negative: the other way)
    float colorSrgb[3];
    float checker; // 1 = the checkered floor
    float shineStrength;
    float shininess;
    unsigned layerMask = kLayerBoth; // which layers show this object (not written = both)
};

// THE SCENE. Edit it to move or to change something. Every object stands on the floor: y = kFloorY + half of its height
// after the scale (the torus: half of its tilted extent, 0.8 * sin(tilt) + 0.3).
// A members must be written in the order of the struct (C++20 designated initialisers): layerMask comes last.
using Shape = Models::Shape;
const SceneObject kScene[] = {
    // the floor: a cube of 24 x 0.2 x 24 metres, its top at y = -1; it is in both layers
    {.shape = Shape::Cube,
     .position = {0.0f, kFloorY - 0.1f, kRowZ},
     .scale = {24.0f, 0.2f, 24.0f},
     .tilt = 0.0f,
     .spinSpeed = 0.0f,
     .colorSrgb = {0.30f, 0.33f, 0.40f},
     .checker = 1.0f,
     .shineStrength = 0.04f,
     .shininess = 16.0f,
     .layerMask = kLayerBoth},
    // the cube: 1.6 metres, red, layer A
    {.shape = Shape::Cube,
     .position = {-3.3f, kFloorY + 0.8f, kRowZ},
     .scale = {1.6f, 1.6f, 1.6f},
     .tilt = 0.0f,
     .spinSpeed = 0.55f,
     .colorSrgb = {0.86f, 0.24f, 0.22f},
     .checker = 0.0f,
     .shineStrength = 0.18f,
     .shininess = 32.0f,
     .layerMask = kLayerA},
    // the sphere: 1.8 metres across, blue, layer B
    {.shape = Shape::Sphere,
     .position = {-1.1f, kFloorY + 0.9f, kRowZ},
     .scale = {1.8f, 1.8f, 1.8f},
     .tilt = 0.0f,
     .spinSpeed = 0.80f,
     .colorSrgb = {0.20f, 0.46f, 0.92f},
     .checker = 0.0f,
     .shineStrength = 0.45f,
     .shininess = 64.0f,
     .layerMask = kLayerB},
    // the cylinder: 1.3 metres across, 1.8 high, light grey, layer A
    {.shape = Shape::Cylinder,
     .position = {1.1f, kFloorY + 0.9f, kRowZ},
     .scale = {1.3f, 1.8f, 1.3f},
     .tilt = 0.0f,
     .spinSpeed = -0.65f,
     .colorSrgb = {0.82f, 0.83f, 0.86f},
     .checker = 0.0f,
     .shineStrength = 0.30f,
     .shininess = 48.0f,
     .layerMask = kLayerA},
    // the torus: a ring of 0.8 metres (to the middle of the tube) and a tube of 0.3, tilted so that the hole shows; layer B
    {.shape = Shape::Torus,
     .position = {3.3f, kFloorY + 0.75f, kRowZ},
     .scale = {1.0f, 1.0f, 1.0f},
     .tilt = 0.6f,
     .spinSpeed = 0.90f,
     .colorSrgb = {0.96f, 0.76f, 0.24f},
     .checker = 0.0f,
     .shineStrength = 0.55f,
     .shininess = 80.0f,
     .layerMask = kLayerB},
};
constexpr int kSceneCount = static_cast<int>(sizeof(kScene) / sizeof(kScene[0]));

// the colours of the scene are written as the eye knows them (sRGB); the light works in linear space
float SrgbToLinear(float c)
{
    return c <= 0.04045f ? c / 12.92f : std::pow((c + 0.055f) / 1.055f, 2.4f);
}

/// Compiles one entry point of a shader file; logs the compiler's message on failure.
bool CompileShader(const char* path, const char* entry, const char* target, ComPtr<ID3DBlob>& out)
{
    UINT flags = D3DCOMPILE_PACK_MATRIX_ROW_MAJOR |
                 D3DCOMPILE_ENABLE_STRICTNESS; // docs/CONVENTIONS.md: row-major, as common.hlsli
#if defined(_DEBUG)
    flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
    flags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif
    wchar_t widePath[MAX_PATH];
    MultiByteToWideChar(CP_UTF8, 0, path, -1, widePath, MAX_PATH);
    ComPtr<ID3DBlob> errors;
    const HRESULT hr = D3DCompileFromFile(widePath, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, entry, target, flags, 0,
                                          &out, &errors);
    if (FAILED(hr))
    {
        if (errors != nullptr)
        {
            DL_LOG_ERROR("Models", "shader %s (%s) failed:\n%s", entry, target,
                         static_cast<const char*>(errors->GetBufferPointer()));
        }
        else
        {
            DL_LOG_ERROR("Models", "cannot read %s (0x%08X): is the working directory the project folder?", path,
                         static_cast<unsigned>(hr));
        }
        return false;
    }
    return true;
}

/// Every state that Models::Draw changes is put back when this goes out of scope. WHY: Triangle does not set the
/// rasterizer state, so a wireframe state left behind would make the test triangle a wireframe too. The ripple also
/// changes the render target and the viewport, and binds two textures that must not stay bound (a texture that is bound
/// for reading cannot be drawn into: the debug layer would warn on the next ripple).
class StateGuard
{
  public:
    explicit StateGuard(ID3D11DeviceContext* context) : m_context(context)
    {
        context->RSGetState(&m_rasterizer);
        context->OMGetDepthStencilState(&m_depthStencil, &m_stencilRef);
        context->OMGetBlendState(&m_blend, m_blendFactor, &m_sampleMask);
        context->OMGetRenderTargets(1, &m_renderView, &m_depthView);
        m_viewportCount = 1;
        context->RSGetViewports(&m_viewportCount, &m_viewport);
        context->IAGetInputLayout(&m_layout);
        context->IAGetPrimitiveTopology(&m_topology);
        context->IAGetVertexBuffers(0, 1, &m_vertexBuffer, &m_stride, &m_offset);
        context->IAGetIndexBuffer(&m_indexBuffer, &m_indexFormat, &m_indexOffset);
        context->VSGetShader(&m_vertexShader, nullptr, nullptr);
        context->PSGetShader(&m_pixelShader, nullptr, nullptr);
        context->VSGetConstantBuffers(0, 2, m_vsConstants);
        context->PSGetConstantBuffers(0, 2, m_psConstants);
        context->PSGetShaderResources(0, 2, m_resources);
        context->PSGetSamplers(0, 1, &m_sampler);
    }

    ~StateGuard()
    {
        // the render target first: the textures that the ripple drew into are bound for reading until the last lines
        ID3D11RenderTargetView* renderView = m_renderView.Get();
        m_context->OMSetRenderTargets(1, &renderView, m_depthView.Get());
        if (m_viewportCount > 0)
        {
            m_context->RSSetViewports(m_viewportCount, &m_viewport);
        }
        m_context->RSSetState(m_rasterizer.Get());
        m_context->OMSetDepthStencilState(m_depthStencil.Get(), m_stencilRef);
        m_context->OMSetBlendState(m_blend.Get(), m_blendFactor, m_sampleMask);
        m_context->IASetInputLayout(m_layout.Get());
        m_context->IASetPrimitiveTopology(m_topology);
        ID3D11Buffer* vertexBuffer = m_vertexBuffer.Get();
        m_context->IASetVertexBuffers(0, 1, &vertexBuffer, &m_stride, &m_offset);
        m_context->IASetIndexBuffer(m_indexBuffer.Get(), m_indexFormat, m_indexOffset);
        m_context->VSSetShader(m_vertexShader.Get(), nullptr, 0);
        m_context->PSSetShader(m_pixelShader.Get(), nullptr, 0);
        m_context->VSSetConstantBuffers(0, 2, m_vsConstants);
        m_context->PSSetConstantBuffers(0, 2, m_psConstants);
        m_context->PSSetShaderResources(0, 2, m_resources);
        ID3D11SamplerState* sampler = m_sampler.Get();
        m_context->PSSetSamplers(0, 1, &sampler);
        for (ID3D11Buffer*& buffer : m_vsConstants) // the Get calls added a reference to each; give them back
        {
            if (buffer != nullptr)
            {
                buffer->Release();
                buffer = nullptr;
            }
        }
        for (ID3D11Buffer*& buffer : m_psConstants)
        {
            if (buffer != nullptr)
            {
                buffer->Release();
                buffer = nullptr;
            }
        }
        for (ID3D11ShaderResourceView*& view : m_resources)
        {
            if (view != nullptr)
            {
                view->Release();
                view = nullptr;
            }
        }
    }

    StateGuard(const StateGuard&) = delete;
    StateGuard& operator=(const StateGuard&) = delete;

  private:
    ID3D11DeviceContext* m_context; // BORROWS
    ComPtr<ID3D11RasterizerState> m_rasterizer;
    ComPtr<ID3D11DepthStencilState> m_depthStencil;
    UINT m_stencilRef = 0;
    ComPtr<ID3D11BlendState> m_blend;
    float m_blendFactor[4] = {};
    UINT m_sampleMask = 0xffffffff;
    ComPtr<ID3D11RenderTargetView> m_renderView;
    ComPtr<ID3D11DepthStencilView> m_depthView;
    D3D11_VIEWPORT m_viewport{};
    UINT m_viewportCount = 0;
    ComPtr<ID3D11InputLayout> m_layout;
    D3D11_PRIMITIVE_TOPOLOGY m_topology = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
    ComPtr<ID3D11Buffer> m_vertexBuffer;
    UINT m_stride = 0;
    UINT m_offset = 0;
    ComPtr<ID3D11Buffer> m_indexBuffer;
    DXGI_FORMAT m_indexFormat = DXGI_FORMAT_UNKNOWN;
    UINT m_indexOffset = 0;
    ComPtr<ID3D11VertexShader> m_vertexShader;
    ComPtr<ID3D11PixelShader> m_pixelShader;
    ID3D11Buffer* m_vsConstants[2] = {};
    ID3D11Buffer* m_psConstants[2] = {};
    ID3D11ShaderResourceView* m_resources[2] = {};
    ComPtr<ID3D11SamplerState> m_sampler;
};

} // namespace

// ---------------------------------------------------------------------------------------------------------------------
// Init
// ---------------------------------------------------------------------------------------------------------------------

bool Models::Init(ID3D11Device* device, int segments)
{
    DL_ASSERT(device != nullptr, "Models::Init needs a device");
    Shutdown();
    if (device == nullptr)
    {
        return false;
    }
    m_device = device;
    m_segments = std::clamp(segments, 6, 128);
    m_themes = DefaultThemes(); // LoadThemes may change them from the config afterwards
    m_ripple.SetSeconds(kDefaultRippleSeconds);
    m_rippleWidth = kDefaultRippleWidth;
    m_rippleDistortion = kDefaultRippleDistortion;
    m_rippleCenter = XMFLOAT2(0.5f, 0.5f);
    ComPtr<ID3DBlob> vsBlob;
    ComPtr<ID3DBlob> psBlob;
    if (!CompileShader(kModelsShaderPath, "VSMain", "vs_5_0", vsBlob) ||
        !CompileShader(kModelsShaderPath, "PSMain", "ps_5_0", psBlob))
    {
        Shutdown();
        return false;
    }
    HRESULT hr =
        m_device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &m_vertexShader);
    if (SUCCEEDED(hr))
    {
        hr = m_device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &m_pixelShader);
    }
    if (FAILED(hr))
    {
        DL_LOG_ERROR("Models", "creating the shader objects failed (0x%08X)", static_cast<unsigned>(hr));
        Shutdown();
        return false;
    }
    // a vertex is MeshVertex: a position (12 bytes), then a normal (12 bytes)
    const D3D11_INPUT_ELEMENT_DESC layout[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(MeshVertex, position), D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(MeshVertex, normal), D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    hr = m_device->CreateInputLayout(layout, ARRAYSIZE(layout), vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(),
                                     &m_inputLayout);
    if (FAILED(hr))
    {
        DL_LOG_ERROR("Models",
                     "CreateInputLayout failed (0x%08X): the vertex of the shader and MeshVertex do not match",
                     static_cast<unsigned>(hr));
        Shutdown();
        return false;
    }
    // the two constant buffers are written every frame (DYNAMIC + WRITE_DISCARD)
    D3D11_BUFFER_DESC cb{};
    cb.Usage = D3D11_USAGE_DYNAMIC;
    cb.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cb.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    cb.ByteWidth = sizeof(FrameConstants);
    hr = m_device->CreateBuffer(&cb, nullptr, &m_frameBuffer);
    if (SUCCEEDED(hr))
    {
        cb.ByteWidth = sizeof(ObjectConstants);
        hr = m_device->CreateBuffer(&cb, nullptr, &m_objectBuffer);
    }
    if (FAILED(hr))
    {
        DL_LOG_ERROR("Models", "constant buffers failed (0x%08X)", static_cast<unsigned>(hr));
        Shutdown();
        return false;
    }
    SetDebugName(m_vertexShader.Get(), "Models.VS");
    SetDebugName(m_pixelShader.Get(), "Models.PS");
    SetDebugName(m_inputLayout.Get(), "Models.InputLayout");
    SetDebugName(m_frameBuffer.Get(), "Models.FrameCB");
    SetDebugName(m_objectBuffer.Get(), "Models.ObjectCB");
    if (!CreateStates() || !CreateRippleResources() || !CreateMeshes(m_segments))
    {
        Shutdown();
        return false;
    }
    DL_LOG_INFO("Models", "ready: %d objects, %d segments, %d triangles", kSceneCount, m_segments, TriangleTotal());
    return true;
}

void Models::Shutdown()
{
    for (GpuMesh& mesh : m_meshes)
    {
        mesh = GpuMesh{};
    }
    for (LayerTarget& target : m_targets)
    {
        target = LayerTarget{};
    }
    m_targetWidth = 0;
    m_targetHeight = 0;
    m_noDepthState.Reset();
    m_noCullState.Reset();
    m_sampler.Reset();
    m_rippleBuffer.Reset();
    m_ripplePixelShader.Reset();
    m_rippleVertexShader.Reset();
    m_blendState.Reset();
    m_depthState.Reset();
    m_wireState.Reset();
    m_solidState.Reset();
    m_objectBuffer.Reset();
    m_frameBuffer.Reset();
    m_inputLayout.Reset();
    m_pixelShader.Reset();
    m_vertexShader.Reset();
    m_device.Reset();
}

bool Models::CreateStates()
{
    // docs/CONVENTIONS.md: clockwise = front face (the Direct3D default), so FrontCounterClockwise = FALSE; back faces
    // are not drawn.
    D3D11_RASTERIZER_DESC rasterizer{};
    rasterizer.FillMode = D3D11_FILL_SOLID;
    rasterizer.CullMode = D3D11_CULL_BACK;
    rasterizer.FrontCounterClockwise = FALSE;
    rasterizer.DepthClipEnable = TRUE;
    HRESULT hr = m_device->CreateRasterizerState(&rasterizer, &m_solidState);
    if (SUCCEEDED(hr))
    {
        rasterizer.FillMode = D3D11_FILL_WIREFRAME;
        hr = m_device->CreateRasterizerState(&rasterizer, &m_wireState);
    }
    if (SUCCEEDED(hr))
    {
        // the ripple triangle: both faces are drawn, so that its winding can never make it disappear
        rasterizer.FillMode = D3D11_FILL_SOLID;
        rasterizer.CullMode = D3D11_CULL_NONE;
        hr = m_device->CreateRasterizerState(&rasterizer, &m_noCullState);
    }
    // the depth test: a pixel is written only if it is NEARER than what is there (Device::BeginFrame clears the depth
    // to 1.0 = far)
    D3D11_DEPTH_STENCIL_DESC depth{};
    depth.DepthEnable = TRUE;
    depth.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    depth.DepthFunc = D3D11_COMPARISON_LESS;
    if (SUCCEEDED(hr))
    {
        hr = m_device->CreateDepthStencilState(&depth, &m_depthState);
    }
    if (SUCCEEDED(hr))
    {
        depth.DepthEnable = FALSE; // the ripple covers the whole screen: no test, no write
        depth.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
        hr = m_device->CreateDepthStencilState(&depth, &m_noDepthState);
    }
    D3D11_BLEND_DESC blend{};
    blend.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL; // no blending: the models are opaque
    if (SUCCEEDED(hr))
    {
        hr = m_device->CreateBlendState(&blend, &m_blendState);
    }
    D3D11_SAMPLER_DESC sampler{};
    sampler.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampler.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP; // the ring bends the picture a little past the edge: clamp, do not wrap
    sampler.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampler.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampler.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sampler.MaxLOD = D3D11_FLOAT32_MAX;
    if (SUCCEEDED(hr))
    {
        hr = m_device->CreateSamplerState(&sampler, &m_sampler);
    }
    if (FAILED(hr))
    {
        DL_LOG_ERROR("Models", "creating the render states failed (0x%08X)", static_cast<unsigned>(hr));
        return false;
    }
    SetDebugName(m_solidState.Get(), "Models.Rasterizer.Solid");
    SetDebugName(m_wireState.Get(), "Models.Rasterizer.Wire");
    SetDebugName(m_noCullState.Get(), "Models.Ripple.Rasterizer");
    SetDebugName(m_depthState.Get(), "Models.DepthState");
    SetDebugName(m_noDepthState.Get(), "Models.Ripple.DepthState");
    SetDebugName(m_blendState.Get(), "Models.BlendState");
    SetDebugName(m_sampler.Get(), "Models.Ripple.Sampler");
    return true;
}

bool Models::CreateRippleResources()
{
    ComPtr<ID3DBlob> vsBlob;
    ComPtr<ID3DBlob> psBlob;
    if (!CompileShader(kRippleShaderPath, "VSMain", "vs_5_0", vsBlob) ||
        !CompileShader(kRippleShaderPath, "PSMain", "ps_5_0", psBlob))
    {
        return false;
    }
    HRESULT hr = m_device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr,
                                              &m_rippleVertexShader);
    if (SUCCEEDED(hr))
    {
        hr = m_device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr,
                                         &m_ripplePixelShader);
    }
    if (SUCCEEDED(hr))
    {
        D3D11_BUFFER_DESC cb{};
        cb.Usage = D3D11_USAGE_DYNAMIC;
        cb.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        cb.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        cb.ByteWidth = sizeof(RippleConstants);
        hr = m_device->CreateBuffer(&cb, nullptr, &m_rippleBuffer);
    }
    if (FAILED(hr))
    {
        DL_LOG_ERROR("Models", "creating the ripple shader objects failed (0x%08X)", static_cast<unsigned>(hr));
        return false;
    }
    SetDebugName(m_rippleVertexShader.Get(), "Models.Ripple.VS");
    SetDebugName(m_ripplePixelShader.Get(), "Models.Ripple.PS");
    SetDebugName(m_rippleBuffer.Get(), "Models.Ripple.CB");
    return true;
}

// The two pictures are made the first time a ripple needs them, and again when the window size changes. The new ones
// are made first and put in place only when all of them worked, so a failure keeps the old ones.
bool Models::EnsureLayerTargets(UINT width, UINT height)
{
    if (width == 0 || height == 0)
    {
        return false;
    }
    if (width == m_targetWidth && height == m_targetHeight && m_targets[0].renderView && m_targets[1].renderView)
    {
        return true;
    }
    static const char* const kNames[2][5] = {
        {"Models.Ripple.Outer.Color", "Models.Ripple.Outer.RTV", "Models.Ripple.Outer.SRV", "Models.Ripple.Outer.Depth",
         "Models.Ripple.Outer.DSV"},
        {"Models.Ripple.Inner.Color", "Models.Ripple.Inner.RTV", "Models.Ripple.Inner.SRV", "Models.Ripple.Inner.Depth",
         "Models.Ripple.Inner.DSV"},
    };
    std::array<LayerTarget, 2> fresh;
    for (std::size_t i = 0; i < fresh.size(); ++i)
    {
        LayerTarget& target = fresh[i];
        // colour: an sRGB format, so that writing converts linear to sRGB and sampling converts it back
        D3D11_TEXTURE2D_DESC desc{};
        desc.Width = width;
        desc.Height = height;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
        HRESULT hr = m_device->CreateTexture2D(&desc, nullptr, &target.color);
        if (SUCCEEDED(hr))
        {
            hr = m_device->CreateRenderTargetView(target.color.Get(), nullptr, &target.renderView);
        }
        if (SUCCEEDED(hr))
        {
            hr = m_device->CreateShaderResourceView(target.color.Get(), nullptr, &target.shaderView);
        }
        if (SUCCEEDED(hr))
        {
            desc.Format = DXGI_FORMAT_D32_FLOAT; // the same depth format as Device (docs/CONVENTIONS.md)
            desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
            hr = m_device->CreateTexture2D(&desc, nullptr, &target.depth);
        }
        if (SUCCEEDED(hr))
        {
            hr = m_device->CreateDepthStencilView(target.depth.Get(), nullptr, &target.depthView);
        }
        if (FAILED(hr))
        {
            DL_LOG_ERROR("Models", "the ripple pictures (%u x %u) could not be made (0x%08X)", width, height,
                         static_cast<unsigned>(hr));
            return false;
        }
        SetDebugName(target.color.Get(), kNames[i][0]);
        SetDebugName(target.renderView.Get(), kNames[i][1]);
        SetDebugName(target.shaderView.Get(), kNames[i][2]);
        SetDebugName(target.depth.Get(), kNames[i][3]);
        SetDebugName(target.depthView.Get(), kNames[i][4]);
    }
    m_targets = std::move(fresh);
    m_targetWidth = width;
    m_targetHeight = height;
    DL_LOG_INFO("Models", "ripple pictures: 2 x %u x %u", width, height);
    return true;
}

bool Models::CreateMeshes(int segments)
{
    return CreateGpuMesh(Shape::Cube, MakeCube()) && CreateGpuMesh(Shape::Sphere, MakeSphere(segments)) &&
           CreateGpuMesh(Shape::Cylinder, MakeCylinder(segments)) &&
           CreateGpuMesh(Shape::Torus, MakeTorus(segments, kTorusMajor, kTorusMinor));
}

// A mesh that never changes is IMMUTABLE: the GPU gets the data once and the CPU can never touch it again.
// The new buffers are made first and put in place only when both worked, so a failure keeps the old shape.
bool Models::CreateGpuMesh(Shape shape, const MeshData& data)
{
    static const char* const kNames[kShapeCount][2] = {
        {"Models.Cube.VB", "Models.Cube.IB"},
        {"Models.Sphere.VB", "Models.Sphere.IB"},
        {"Models.Cylinder.VB", "Models.Cylinder.IB"},
        {"Models.Torus.VB", "Models.Torus.IB"},
    };
    if (data.vertices.empty() || data.indices.empty())
    {
        DL_LOG_ERROR("Models", "a shape has no vertices");
        return false;
    }
    GpuMesh fresh;
    D3D11_BUFFER_DESC desc{};
    desc.Usage = D3D11_USAGE_IMMUTABLE;
    desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    desc.ByteWidth = static_cast<UINT>(data.vertices.size() * sizeof(MeshVertex));
    D3D11_SUBRESOURCE_DATA initial{};
    initial.pSysMem = data.vertices.data();
    HRESULT hr = m_device->CreateBuffer(&desc, &initial, &fresh.vertexBuffer);
    if (SUCCEEDED(hr))
    {
        desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        desc.ByteWidth = static_cast<UINT>(data.indices.size() * sizeof(std::uint32_t));
        initial.pSysMem = data.indices.data();
        hr = m_device->CreateBuffer(&desc, &initial, &fresh.indexBuffer);
    }
    if (FAILED(hr))
    {
        DL_LOG_ERROR("Models", "buffers of a shape failed (0x%08X)", static_cast<unsigned>(hr));
        return false;
    }
    fresh.indexCount = static_cast<UINT>(data.indices.size());
    const auto index = static_cast<std::size_t>(shape);
    SetDebugName(fresh.vertexBuffer.Get(), kNames[index][0]);
    SetDebugName(fresh.indexBuffer.Get(), kNames[index][1]);
    m_meshes[index] = std::move(fresh);
    return true;
}

// ---------------------------------------------------------------------------------------------------------------------
// the look of the layers and the ripple
// ---------------------------------------------------------------------------------------------------------------------

void Models::LoadThemes(const Config& config)
{
    static const char* const kSections[2] = {"layer_a", "layer_b"};
    const std::array<Theme, 2> defaults = DefaultThemes();
    for (std::size_t i = 0; i < 2; ++i)
    {
        const char* s = kSections[i];
        const Theme& d = defaults[i];
        Theme& t = m_themes[i];
        t.toLight = XMFLOAT3(config.GetFloat(s, "light_x", d.toLight.x), config.GetFloat(s, "light_y", d.toLight.y),
                             config.GetFloat(s, "light_z", d.toLight.z));
        t.lightColor = XMFLOAT3(config.GetFloat(s, "light_r", d.lightColor.x),
                                config.GetFloat(s, "light_g", d.lightColor.y),
                                config.GetFloat(s, "light_b", d.lightColor.z));
        t.sky = XMFLOAT3(config.GetFloat(s, "sky_r", d.sky.x), config.GetFloat(s, "sky_g", d.sky.y),
                         config.GetFloat(s, "sky_b", d.sky.z));
        t.clear = XMFLOAT3(config.GetFloat(s, "clear_r", d.clear.x), config.GetFloat(s, "clear_g", d.clear.y),
                           config.GetFloat(s, "clear_b", d.clear.z));
        // a light with no direction cannot be normalised: keep the built-in one
        const float length2 = t.toLight.x * t.toLight.x + t.toLight.y * t.toLight.y + t.toLight.z * t.toLight.z;
        if (!(length2 > 1.0e-6f))
        {
            DL_LOG_WARN("Models", "[%s] light_x/y/z is a zero vector: using the built-in direction", s);
            t.toLight = d.toLight;
        }
    }
    // the ripple: a value that makes no sense is pulled back into a range that works
    m_ripple.SetSeconds(config.GetFloat("ripple", "seconds", kDefaultRippleSeconds));
    m_rippleWidth = std::clamp(config.GetFloat("ripple", "width", kDefaultRippleWidth), 0.005f, 0.5f);
    m_rippleDistortion = std::clamp(config.GetFloat("ripple", "distortion", kDefaultRippleDistortion), 0.0f, 0.1f);
    m_rippleCenter = XMFLOAT2(std::clamp(config.GetFloat("ripple", "center_x", 0.5f), 0.0f, 1.0f),
                              std::clamp(config.GetFloat("ripple", "center_y", 0.5f), 0.0f, 1.0f));
    DL_LOG_INFO("Models", "layer themes loaded, ripple %.2f s", m_ripple.Seconds());
}

void Models::ClearColor(float out[4]) const
{
    const XMFLOAT3& clear = m_themes[static_cast<std::size_t>(m_ripple.Outer())].clear;
    out[0] = clear.x;
    out[1] = clear.y;
    out[2] = clear.z;
    out[3] = 1.0f;
}

// ---------------------------------------------------------------------------------------------------------------------
// the keys and the clock
// ---------------------------------------------------------------------------------------------------------------------

void Models::ToggleWireframe()
{
    m_wireframe = !m_wireframe;
}

void Models::ToggleSpin()
{
    m_spinning = !m_spinning;
}

void Models::ToggleLayer()
{
    m_ripple.Toggle();
    DL_LOG_INFO("Models", "layer: going to %s", m_ripple.Target() == 0 ? "A" : "B");
}

void Models::CycleDetail()
{
    if (!m_device)
    {
        return;
    }
    int next = kDetailLevels[0];
    for (int level : kDetailLevels)
    {
        if (level > m_segments)
        {
            next = level;
            break;
        }
    }
    if (CreateMeshes(next))
    {
        m_segments = next;
        DL_LOG_INFO("Models", "detail: %d segments, %d triangles", m_segments, TriangleTotal());
    }
    else
    {
        DL_LOG_WARN("Models", "detail %d could not be made: staying at %d", next, m_segments);
    }
}

void Models::Update(float deltaSeconds)
{
    const float dt = std::clamp(deltaSeconds, 0.0f, 0.1f);
    if (m_spinning)
    {
        m_spin += dt;
    }
    m_ripple.Update(dt); // runs even while the shapes are stopped: the layer must still change
}

// ---------------------------------------------------------------------------------------------------------------------
// Draw
// ---------------------------------------------------------------------------------------------------------------------

void Models::BindScenePipeline(ID3D11DeviceContext* context)
{
    context->OMSetDepthStencilState(m_depthState.Get(), 0);
    context->OMSetBlendState(m_blendState.Get(), nullptr, 0xffffffff);
    context->RSSetState(m_wireframe ? m_wireState.Get() : m_solidState.Get());
    context->IASetInputLayout(m_inputLayout.Get());
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->VSSetShader(m_vertexShader.Get(), nullptr, 0);
    context->PSSetShader(m_pixelShader.Get(), nullptr, 0);
    ID3D11Buffer* constants[2] = {m_frameBuffer.Get(), m_objectBuffer.Get()};
    context->VSSetConstantBuffers(0, 2, constants);
    context->PSSetConstantBuffers(0, 2, constants);
}

void Models::DrawObjects(ID3D11DeviceContext* context, const XMMATRIX& view, const XMMATRIX& projection,
                         const XMFLOAT3& eye, int layer)
{
    // ---- the frame: the camera and the light of this layer. View * Projection is worked out once
    // (docs/CONVENTIONS.md: row vectors, no transpose).
    const Theme& theme = m_themes[static_cast<std::size_t>(layer)];
    FrameConstants frame{};
    XMStoreFloat4x4(&frame.viewProjection, view * projection);
    frame.eye = eye;
    XMStoreFloat3(&frame.toLight, XMVector3Normalize(XMLoadFloat3(&theme.toLight)));
    frame.ambient = 1.0f;
    frame.lightColor = theme.lightColor;
    frame.skyUp = theme.sky;
    frame.skyDown = XMFLOAT3(theme.sky.x * kSkyDownScale, theme.sky.y * kSkyDownScale, theme.sky.z * kSkyDownScale);
    D3D11_MAPPED_SUBRESOURCE mapped{};
    if (FAILED(context->Map(m_frameBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
    {
        return;
    }
    std::memcpy(mapped.pData, &frame, sizeof(frame));
    context->Unmap(m_frameBuffer.Get(), 0);

    // ---- one draw call per object: only the objects of this layer
    const unsigned layerBit = 1u << layer;
    for (const SceneObject& object : kScene)
    {
        if ((object.layerMask & layerBit) == 0)
        {
            continue; // not on the layer we are drawing
        }
        const GpuMesh& mesh = m_meshes[static_cast<std::size_t>(object.shape)];
        if (!mesh.vertexBuffer || !mesh.indexBuffer)
        {
            continue;
        }
        // World = Scale * Rotation * Translation: first the size, then the tilt and the turn, then the place (row
        // vectors: read left to right)
        const XMMATRIX world = XMMatrixScaling(object.scale[0], object.scale[1], object.scale[2]) *
                               XMMatrixRotationX(object.tilt) * XMMatrixRotationY(object.spinSpeed * m_spin) *
                               XMMatrixTranslation(object.position[0], object.position[1], object.position[2]);
        ObjectConstants data{};
        XMStoreFloat4x4(&data.world, world);
        // A normal is not moved like a point: on a stretched object it must be turned the other way (the inverse
        // transpose), or the light is wrong. For this affine matrix the 3x3 part of the result is the inverse transpose
        // of the 3x3 of `world`.
        XMStoreFloat4x4(&data.normalMatrix, XMMatrixTranspose(XMMatrixInverse(nullptr, world)));
        data.color = XMFLOAT4(SrgbToLinear(object.colorSrgb[0]), SrgbToLinear(object.colorSrgb[1]),
                              SrgbToLinear(object.colorSrgb[2]), 1.0f);
        data.params = XMFLOAT4(object.checker, object.shineStrength, object.shininess, 0.0f);
        if (FAILED(context->Map(m_objectBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
        {
            continue;
        }
        std::memcpy(mapped.pData, &data, sizeof(data));
        context->Unmap(m_objectBuffer.Get(), 0);
        ID3D11Buffer* vertexBuffer = mesh.vertexBuffer.Get();
        const UINT stride = sizeof(MeshVertex);
        const UINT offset = 0;
        context->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
        context->IASetIndexBuffer(mesh.indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
        context->DrawIndexed(mesh.indexCount, 0, 0);
    }
}

bool Models::DrawRipple(ID3D11DeviceContext* context, const XMMATRIX& view, const XMMATRIX& projection,
                        const XMFLOAT3& eye)
{
    // ---- what is on the screen now: the ripple draws somewhere else first and must come back to it
    ComPtr<ID3D11RenderTargetView> screenView;
    ComPtr<ID3D11DepthStencilView> screenDepth;
    context->OMGetRenderTargets(1, screenView.GetAddressOf(), screenDepth.GetAddressOf());
    if (!screenView)
    {
        return false;
    }
    ComPtr<ID3D11Resource> screenResource;
    screenView->GetResource(&screenResource);
    ComPtr<ID3D11Texture2D> screenTexture;
    if (FAILED(screenResource.As(&screenTexture)))
    {
        return false;
    }
    D3D11_TEXTURE2D_DESC screenDesc{};
    screenTexture->GetDesc(&screenDesc);
    D3D11_VIEWPORT viewport{};
    UINT viewportCount = 1;
    context->RSGetViewports(&viewportCount, &viewport);
    if (viewportCount == 0 || !(viewport.Height > 0.0f) || !EnsureLayerTargets(screenDesc.Width, screenDesc.Height))
    {
        return false; // nothing has been changed yet
    }

    // ---- pass 1 and 2: each layer, complete, into its own picture ([0] outside the circle, [1] inside it)
    const int layers[2] = {m_ripple.Outer(), m_ripple.Inner()};
    for (std::size_t i = 0; i < 2; ++i)
    {
        const LayerTarget& target = m_targets[i];
        const XMFLOAT3& clearColor = m_themes[static_cast<std::size_t>(layers[i])].clear;
        const float clear[4] = {clearColor.x, clearColor.y, clearColor.z, 1.0f}; // linear: written as sRGB
        context->ClearRenderTargetView(target.renderView.Get(), clear);
        context->ClearDepthStencilView(target.depthView.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
        ID3D11RenderTargetView* renderView = target.renderView.Get();
        context->OMSetRenderTargets(1, &renderView, target.depthView.Get());
        context->RSSetViewports(1, &viewport);
        BindScenePipeline(context);
        DrawObjects(context, view, projection, eye, layers[i]);
    }

    // ---- pass 3: back to the screen, one triangle that covers it, the circle decides which picture each pixel takes
    ID3D11RenderTargetView* screen = screenView.Get();
    context->OMSetRenderTargets(1, &screen, screenDepth.Get());
    context->RSSetViewports(1, &viewport);

    const float aspect = viewport.Width / viewport.Height;
    RippleConstants ripple{};
    ripple.center = m_rippleCenter;
    ripple.radius = m_ripple.Opened() *
                    (FarthestCornerDistance(m_rippleCenter.x, m_rippleCenter.y, aspect) + kRippleOvershoot * m_rippleWidth);
    ripple.ringWidth = m_rippleWidth;
    ripple.aspect = aspect;
    ripple.distortion = m_rippleDistortion;
    const XMFLOAT3& glow = m_themes[static_cast<std::size_t>(m_ripple.Inner())].lightColor;
    ripple.glowColor = XMFLOAT3(glow.x * kRippleGlow, glow.y * kRippleGlow, glow.z * kRippleGlow);
    D3D11_MAPPED_SUBRESOURCE mapped{};
    if (FAILED(context->Map(m_rippleBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
    {
        return true; // the pictures were drawn; the StateGuard of Draw puts the state back
    }
    std::memcpy(mapped.pData, &ripple, sizeof(ripple));
    context->Unmap(m_rippleBuffer.Get(), 0);

    context->OMSetDepthStencilState(m_noDepthState.Get(), 0);
    context->OMSetBlendState(m_blendState.Get(), nullptr, 0xffffffff);
    context->RSSetState(m_noCullState.Get());
    context->IASetInputLayout(nullptr); // the vertex shader makes its own positions from SV_VertexID
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->VSSetShader(m_rippleVertexShader.Get(), nullptr, 0);
    context->PSSetShader(m_ripplePixelShader.Get(), nullptr, 0);
    ID3D11Buffer* constants[1] = {m_rippleBuffer.Get()};
    context->PSSetConstantBuffers(0, 1, constants);
    ID3D11ShaderResourceView* pictures[2] = {m_targets[0].shaderView.Get(), m_targets[1].shaderView.Get()};
    context->PSSetShaderResources(0, 2, pictures);
    ID3D11SamplerState* sampler = m_sampler.Get();
    context->PSSetSamplers(0, 1, &sampler);
    context->Draw(3, 0);

    // the pictures must not stay bound: next frame they are drawn into again
    ID3D11ShaderResourceView* none[2] = {};
    context->PSSetShaderResources(0, 2, none);
    return true;
}

void Models::Draw(ID3D11DeviceContext* context, const XMMATRIX& view, const XMMATRIX& projection, const XMFLOAT3& eye)
{
    DL_ASSERT(context != nullptr, "Models::Draw needs a context");
    if (context == nullptr || !m_vertexShader || !m_pixelShader)
    {
        return; // Init did not work: the log has said so; the program goes on without the models
    }
    const StateGuard guard(context);

    if (m_ripple.Active() && DrawRipple(context, view, projection, eye))
    {
        return;
    }
    // idle (or the pictures of the ripple could not be made): one layer, straight to the screen
    BindScenePipeline(context);
    DrawObjects(context, view, projection, eye, m_ripple.Active() ? m_ripple.Target() : m_ripple.Outer());
}

// The numbers of the title bar follow the layer the screen is going to show.
int Models::TriangleTotal() const
{
    const unsigned layerBit = 1u << m_ripple.Target();
    int total = 0;
    for (const SceneObject& object : kScene)
    {
        if ((object.layerMask & layerBit) != 0)
        {
            total += static_cast<int>(m_meshes[static_cast<std::size_t>(object.shape)].indexCount / 3);
        }
    }
    return total;
}

int Models::VisibleModelCount() const
{
    const unsigned layerBit = 1u << m_ripple.Target();
    int count = 0;
    for (const SceneObject& object : kScene)
    {
        if ((object.layerMask & layerBit) != 0 && object.checker < 0.5f) // (the floor is not a "model")
        {
            ++count;
        }
    }
    return count;
}

std::wstring Models::StatusLine() const
{
    wchar_t text[160];
    swprintf_s(text, L"%d models | %d segments | %d triangles | %s%s | layer %s", VisibleModelCount(), m_segments,
               TriangleTotal(), m_wireframe ? L"wireframe" : L"solid", m_spinning ? L"" : L" | stopped",
               m_ripple.Target() == 0 ? L"A" : L"B");
    return text;
}

} // namespace dl
