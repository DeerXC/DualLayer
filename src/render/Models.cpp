#include "render/Models.h"

#include "core/Assert.h"
#include "core/Log.h"
#include "render/GpuDebug.h"
#include "render/Lights.h"
#include "render/Projection.h"
#include "render/ShaderCompiler.h"
#include "render/ShadowMath.h"
#include "render/StaticBatch.h"
#include "scene/SceneData.h"

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
constexpr char kLinesShaderPath[] = "shaders/lines.hlsl";
constexpr char kRainShaderPath[] = "shaders/rain.hlsl";
constexpr int kDetailLevels[] = {8, 16, 32, 64};
constexpr float kShadowHalfDepth =
    160.0f; // UNITS: metres: the shadow box reaches this far toward the light and away from it
constexpr float kShadowAhead =
    0.35f; // the box is centred this part of its radius ahead of the camera, on the ground plane

// KEEP IN SYNC: cbuffer Frame and cbuffer Object in shaders/models.hlsl (field for field).
struct FrameConstants
{
    XMFLOAT4X4 viewProjection; // row-major, row vectors: v' = v * M
    XMFLOAT3 eye;              // UNITS: metres, world space
    float time;                // UNITS: seconds since the start (the waves of the water)
    XMFLOAT3 toLight;          // unit vector from a surface TO the light
    float ambient;
    XMFLOAT3 lightColor; // linear
    float lanternMode; // 0: none; 1: this is B, what is inside the circle of the lantern is not drawn; 2: this is the A
                       // inside the circle, what is outside it is not drawn (scene/Lantern.h)
    XMFLOAT3 skyUp;    // linear: the sky light on a surface that looks up
    float pad2;
    XMFLOAT3 skyDown; // linear: the ground light on a surface that looks down
    float pad3;
    XMFLOAT3 fogColor;        // linear: the colour of the fog (= the background)
    float fogDensity;         // UNITS: 1 / metres
    XMFLOAT3 tint;            // the stone of the city in this layer (multiplies the albedo of the city)
    float weathering;         // 0 .. 1
    XMFLOAT4 lantern;         // xyz: the centre of the circle of the lantern (world); w: its radius (metres)
    XMFLOAT4 skyRight;        // xyz: camera right / projection x scale; w: additional near-ground fog density
    XMFLOAT4 skyUpRay;        // xyz: camera up / projection y scale; w: near-ground fog base height
    XMFLOAT4 skyForward;      // xyz: camera forward; w: solar / lunar halo weight
    XMFLOAT4 skyZenith;       // rgb: linear colour; w: gradient enabled
    XMFLOAT4X4 lightViewProj; // the world into the shadow map of the sun or the moon (render/ShadowMath.h)
    XMFLOAT4 shadow;    // x: one texel of the map (uv), y: depth bias, z: normal offset (metres), w: strength (0: none)
    XMFLOAT4 grade;     // x: exposure, y: knee of the shoulder, z: debug view, w: sun scatter
    XMFLOAT4 grade2;    // x: the sun in bands, y: the rim, z: saturation, w: vignette (the style of the light)
    XMFLOAT4 styleCool; // rgb: the multiplier of the sky light of a face the sun does not reach, a: how far
    XMFLOAT4 styleLow;  // rgb: the grade of the darks, a: how much
    XMFLOAT4 styleHigh; // rgb: the grade of the lights, a: how much
    XMFLOAT4 styleLight; // x: the shadow of the clouds, y: the rings of a lamp, z: the light shafts, w: their samples
    XMFLOAT4 styleFar;   // rgb: what the haze is multiplied by far away, a: how far
    XMFLOAT4 styleMisc;  // x: the painted blotches, y: the damp near the water, z: the contrast, w: the edge (pixels)
    XMFLOAT4 styleMore;  // x: far fog, y: a bright light to grey, z: wet stone, w: overcast sky
};

struct ObjectConstants
{
    XMFLOAT4X4 world;
    XMFLOAT4X4 normalMatrix; // inverse transpose of the 3x3 of `world`
    XMFLOAT4 color;  // rgb: linear albedo; a: 1 = part of the city (takes the tint and the weathering of the layer)
    XMFLOAT4 params; // x: checkered floor, y: shine strength, z: shininess, w: glow (its own light)
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

// KEEP IN SYNC: cbuffer Lamps in shaders/models.hlsl and kMaxLamps in render/Lights.h.
struct LampConstants
{
    float count[4];           // x: how many lamps are on
    float pos[kMaxLamps][4];  // xyz: where (world), w: range (metres)
    float tint[kMaxLamps][4]; // rgb: colour times strength (linear)
};

static_assert(sizeof(FrameConstants) == 480, "cbuffer Frame is 30 registers of 16 bytes");
static_assert(sizeof(LampConstants) == 1040, "cbuffer Lamps is 65 registers of 16 bytes");
static_assert(sizeof(ObjectConstants) == 160, "cbuffer Object is 10 registers of 16 bytes");
static_assert(sizeof(RippleConstants) == 48, "cbuffer Ripple is 3 registers of 16 bytes");

// ---- the objects (what is drawn one by one): scene/SceneData.h.
using scene::kTorusMajor;
using scene::kTorusMinor;
using scene::SceneObject;

// ---- the look of the layers (M2). The built-in values; [layer_a] / [layer_b] in config/default.ini change them.
// A is a warm world (a low red sun), B is a cool world (a blue moon from the other side). All colours are linear.
constexpr float kSkyDownScale = 0.28f; // the ground light is this part of the sky light (the same ratio as in M1)

// ---- the ripple (M2). The built-in values; [ripple] in config/default.ini changes them.
constexpr float kDefaultRippleSeconds = 0.9f;      // UNITS: seconds for the circle to cover the screen
constexpr float kDefaultRippleWidth = 0.07f;       // UNITS: screen heights
constexpr float kDefaultRippleDistortion = 0.012f; // UNITS: uv (a part of the screen)
constexpr float kRippleGlow = 0.5f;                // the ring adds this much of the light colour of the layer it opens
constexpr float kRippleOvershoot =
    1.5f; // the circle grows this many ring widths past the corner, so the ring leaves the screen

/// The built-in looks of the two moments, used until the town gives its own (ApplyTownThemes in app/Gpu.cpp). A, the
/// whole city: a low evening sun, warm sandstone, a light haze, a bright sky behind it. B, the city after the storm: a
/// cold light from above, wet dark stone with moss on what looks up, a thick fog that hides what is far. They differ in
/// every channel at once, so that a single glance tells them apart.
std::array<Models::Theme, 2> DefaultThemes()
{
    Models::Theme a;
    a.toLight = XMFLOAT3(-0.60f, 0.45f, -0.65f); // a low sun, left of and behind the camera at the start: long shadows
    a.lightColor = XMFLOAT3(1.45f, 1.05f, 0.70f);
    a.sky = XMFLOAT3(0.24f, 0.16f, 0.12f);
    a.clear = XMFLOAT3(0.42f, 0.22f, 0.10f); // an evening sky
    a.fogDensity = 0.006f;
    a.tint = XMFLOAT3(1.25f, 1.00f, 0.74f); // sandstone
    a.weathering = 0.15f;
    Models::Theme b;
    b.toLight = XMFLOAT3(0.20f, 0.95f, -0.25f); // a grey light from high above: short shadows
    b.lightColor = XMFLOAT3(0.55f, 0.68f, 0.85f);
    b.sky = XMFLOAT3(0.10f, 0.14f, 0.20f);
    b.clear = XMFLOAT3(0.045f, 0.065f, 0.09f); // a storm
    b.fogDensity = 0.022f;
    b.tint = XMFLOAT3(0.62f, 0.70f, 0.78f); // wet dark stone
    b.weathering = 0.85f;
    return {{a, b}};
}

// the colours of the scene are written as the eye knows them (sRGB); the light works in linear space
float SrgbToLinear(float c)
{
    return c <= 0.04045f ? c / 12.92f : std::pow((c + 0.055f) / 1.055f, 2.4f);
}

/// Every state that Models::Draw changes is put back when this goes out of scope. WHY: some passes do not set the
/// rasterizer state, so a wireframe state left behind would stay on for whatever is drawn next. The ripple also
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
        context->VSGetConstantBuffers(0, 3, m_vsConstants);
        context->PSGetConstantBuffers(0, 3, m_psConstants);
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
        m_context->VSSetConstantBuffers(0, 3, m_vsConstants);
        m_context->PSSetConstantBuffers(0, 3, m_psConstants);
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
    ID3D11Buffer* m_vsConstants[3] = {};
    ID3D11Buffer* m_psConstants[3] = {};
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
    m_themes = DefaultThemes();      // LoadThemes may change them from the config afterwards
    m_lighting = LightingSettings{}; // LoadLighting may change them from the config afterwards
    m_lines = LineSettings{};
    m_rain = RainSettings{};
    m_ripple.SetSeconds(kDefaultRippleSeconds);
    m_rippleWidth = kDefaultRippleWidth;
    m_rippleDistortion = kDefaultRippleDistortion;
    m_rippleCenter = XMFLOAT2(0.5f, 0.5f);
    ComPtr<ID3DBlob> vsBlob;
    ComPtr<ID3DBlob> psBlob;
    ComPtr<ID3DBlob> batchBlob;
    ComPtr<ID3DBlob> skyBlob;
    if (!CompileShader(kModelsShaderPath, "VSMain", "vs_5_0", vsBlob) ||
        !CompileShader(kModelsShaderPath, "PSMain", "ps_5_0", psBlob) ||
        !CompileShader(kModelsShaderPath, "VSBatched", "vs_5_0", batchBlob) ||
        !CompileShader(kModelsShaderPath, "PSSky", "ps_5_0", skyBlob))
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
    hr = m_device->CreatePixelShader(skyBlob->GetBufferPointer(), skyBlob->GetBufferSize(), nullptr, &m_skyPixelShader);
    if (FAILED(hr))
    {
        DL_LOG_ERROR("Models", "creating the sky shader failed (0x%08X)", static_cast<unsigned>(hr));
        Shutdown();
        return false;
    }
    SetDebugName(m_skyPixelShader.Get(), "Models.Sky.PS");
    // a vertex is MeshVertex: a position (12 bytes), then a normal (12 bytes)
    const D3D11_INPUT_ELEMENT_DESC layout[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(MeshVertex, position), D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(MeshVertex, normal), D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    hr = m_device->CreateInputLayout(layout, ARRAYSIZE(layout), vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(),
                                     &m_inputLayout);
    if (SUCCEEDED(hr))
    {
        hr = m_device->CreateVertexShader(batchBlob->GetBufferPointer(), batchBlob->GetBufferSize(), nullptr,
                                          &m_batchVertexShader);
    }
    // a baked vertex is BatchVertex: position (12 bytes), normal (12), colour (16), parameters (16)
    const D3D11_INPUT_ELEMENT_DESC batchLayout[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(BatchVertex, position), D3D11_INPUT_PER_VERTEX_DATA,
         0},
        {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(BatchVertex, normal), D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(BatchVertex, color), D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(BatchVertex, params), D3D11_INPUT_PER_VERTEX_DATA,
         0},
    };
    if (SUCCEEDED(hr))
    {
        hr = m_device->CreateInputLayout(batchLayout, ARRAYSIZE(batchLayout), batchBlob->GetBufferPointer(),
                                         batchBlob->GetBufferSize(), &m_batchLayout);
    }
    if (FAILED(hr))
    {
        DL_LOG_ERROR(
            "Models",
            "CreateInputLayout failed (0x%08X): the vertex of the shader and MeshVertex / BatchVertex do not match",
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
    if (SUCCEEDED(hr))
    {
        cb.ByteWidth = sizeof(LampConstants);
        hr = m_device->CreateBuffer(&cb, nullptr, &m_lampBuffer);
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
    SetDebugName(m_batchVertexShader.Get(), "Models.Batch.VS");
    SetDebugName(m_batchLayout.Get(), "Models.Batch.InputLayout");
    SetDebugName(m_frameBuffer.Get(), "Models.FrameCB");
    SetDebugName(m_objectBuffer.Get(), "Models.ObjectCB");
    SetDebugName(m_lampBuffer.Get(), "Models.LampsCB");
    if (!CreateStates() || !CreateRippleResources() || !CreateMeshes(m_segments) || !CreateBatchBuffers())
    {
        Shutdown();
        return false;
    }
    // the ink lines: a pass that cannot be made is not an error either: the log says so and the picture has no lines
    CreateLineResources();
    // the rain of B: the same
    CreateRainResources();
    // the shadow map: a map that cannot be made is not an error, the shadows are off and the log says so
    if (m_lighting.shadows && !CreateShadowMap(m_lighting.shadowSize))
    {
        m_lighting.shadows = false;
    }
    DL_LOG_INFO("Models", "ready: %d segments, %d triangles, shadows %s", m_segments, TriangleTotal(),
                m_lighting.shadows ? "on" : "off");
    return true;
}

void Models::Shutdown()
{
    for (GpuMesh& mesh : m_meshes)
    {
        mesh = GpuMesh{};
    }
    for (GpuMesh& mesh : m_batch)
    {
        mesh = GpuMesh{};
    }
    for (GpuMesh& mesh : m_waterBatch)
    {
        mesh = GpuMesh{};
    }
    m_batchLayout.Reset();
    m_batchVertexShader.Reset();
    for (LayerTarget& target : m_targets)
    {
        target = LayerTarget{};
    }
    m_targetWidth = 0;
    m_targetHeight = 0;
    ReleaseShadowMap();
    m_shadowSampler.Reset();
    m_shadowRaster.Reset();
    m_additiveBlend.Reset();
    m_lampBuffer.Reset();
    m_screenDepthView.Reset();
    m_screenDepth.Reset();
    m_lineBlend.Reset();
    m_lineBuffer.Reset();
    m_linePixelShader.Reset();
    m_rainBlend.Reset();
    m_rainBuffer.Reset();
    m_rainPixelShader.Reset();
    m_noDepthState.Reset();
    m_noCullState.Reset();
    m_sampler.Reset();
    m_rippleBuffer.Reset();
    m_ripplePixelShader.Reset();
    m_skyPixelShader.Reset();
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
    // water: blended over what is behind it (its alpha says how much of it is seen), tested against the depth but not
    // written, so that what is under it still shows and one water does not hide another
    D3D11_BLEND_DESC waterBlend{};
    waterBlend.RenderTarget[0].BlendEnable = TRUE;
    waterBlend.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    waterBlend.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    waterBlend.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    waterBlend.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    waterBlend.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
    waterBlend.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    waterBlend.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    if (SUCCEEDED(hr))
    {
        hr = m_device->CreateBlendState(&waterBlend, &m_waterBlend);
    }
    D3D11_DEPTH_STENCIL_DESC waterDepth{};
    waterDepth.DepthEnable = TRUE;
    waterDepth.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    waterDepth.DepthFunc = D3D11_COMPARISON_LESS;
    if (SUCCEEDED(hr))
    {
        hr = m_device->CreateDepthStencilState(&waterDepth, &m_waterDepth);
    }
    D3D11_SAMPLER_DESC sampler{};
    sampler.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampler.AddressU =
        D3D11_TEXTURE_ADDRESS_CLAMP; // the ring bends the picture a little past the edge: clamp, do not wrap
    sampler.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampler.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampler.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sampler.MaxLOD = D3D11_FLOAT32_MAX;
    if (SUCCEEDED(hr))
    {
        hr = m_device->CreateSamplerState(&sampler, &m_sampler);
    }
    // light: the shadow map is compared in the hardware (and smoothed by the bilinear filter); outside the map
    // everything is lit
    D3D11_SAMPLER_DESC shadowSampler{};
    shadowSampler.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
    shadowSampler.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
    shadowSampler.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
    shadowSampler.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
    shadowSampler.BorderColor[0] = shadowSampler.BorderColor[1] = shadowSampler.BorderColor[2] =
        shadowSampler.BorderColor[3] = 1.0f;
    shadowSampler.ComparisonFunc = D3D11_COMPARISON_LESS_EQUAL;
    shadowSampler.MaxLOD = D3D11_FLOAT32_MAX;
    if (SUCCEEDED(hr))
    {
        hr = m_device->CreateSamplerState(&shadowSampler, &m_shadowSampler);
    }
    // the shadow pass draws both sides (the polygons of the town have no fixed winding), a little biased away from the
    // light, and is not clipped by the near plane of the box: what stands between the light and the box still casts its
    // shadow into it
    D3D11_RASTERIZER_DESC shadowRaster{};
    shadowRaster.FillMode = D3D11_FILL_SOLID;
    shadowRaster.CullMode = D3D11_CULL_NONE;
    shadowRaster.FrontCounterClockwise = FALSE;
    shadowRaster.SlopeScaledDepthBias = 1.5f;
    shadowRaster.DepthClipEnable = FALSE;
    if (SUCCEEDED(hr))
    {
        hr = m_device->CreateRasterizerState(&shadowRaster, &m_shadowRaster);
    }
    // halos: added to what is behind them
    D3D11_BLEND_DESC additive{};
    additive.RenderTarget[0].BlendEnable = TRUE;
    additive.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
    additive.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
    additive.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    additive.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
    additive.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
    additive.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    additive.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    if (SUCCEEDED(hr))
    {
        hr = m_device->CreateBlendState(&additive, &m_additiveBlend);
    }
    if (FAILED(hr))
    {
        DL_LOG_ERROR("Models", "creating the render states failed (0x%08X)", static_cast<unsigned>(hr));
        return false;
    }
    SetDebugName(m_shadowSampler.Get(), "Models.Shadow.Sampler");
    SetDebugName(m_shadowRaster.Get(), "Models.Shadow.Rasterizer");
    SetDebugName(m_additiveBlend.Get(), "Models.Halo.BlendState");
    SetDebugName(m_solidState.Get(), "Models.Rasterizer.Solid");
    SetDebugName(m_wireState.Get(), "Models.Rasterizer.Wire");
    SetDebugName(m_noCullState.Get(), "Models.Ripple.Rasterizer");
    SetDebugName(m_depthState.Get(), "Models.DepthState");
    SetDebugName(m_noDepthState.Get(), "Models.Ripple.DepthState");
    SetDebugName(m_blendState.Get(), "Models.BlendState");
    SetDebugName(m_waterBlend.Get(), "Models.Water.BlendState");
    SetDebugName(m_waterDepth.Get(), "Models.Water.DepthState");
    SetDebugName(m_sampler.Get(), "Models.Ripple.Sampler");
    return true;
}

// The shadow map: one 32-bit depth texture that is drawn into (as a depth buffer) and then read by the model shader (as
// a texture): typeless, with a depth view and a shader view of its own.
bool Models::CreateShadowMap(int size)
{
    ReleaseShadowMap();
    D3D11_TEXTURE2D_DESC desc{};
    desc.Width = static_cast<UINT>(size);
    desc.Height = static_cast<UINT>(size);
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R32_TYPELESS;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
    HRESULT hr = m_device->CreateTexture2D(&desc, nullptr, &m_shadowTexture);
    if (SUCCEEDED(hr))
    {
        D3D11_DEPTH_STENCIL_VIEW_DESC depthView{};
        depthView.Format = DXGI_FORMAT_D32_FLOAT;
        depthView.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
        depthView.Texture2D.MipSlice = 0;
        hr = m_device->CreateDepthStencilView(m_shadowTexture.Get(), &depthView, &m_shadowDsv);
    }
    if (SUCCEEDED(hr))
    {
        D3D11_SHADER_RESOURCE_VIEW_DESC shaderView{};
        shaderView.Format = DXGI_FORMAT_R32_FLOAT;
        shaderView.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        shaderView.Texture2D.MostDetailedMip = 0;
        shaderView.Texture2D.MipLevels = 1;
        hr = m_device->CreateShaderResourceView(m_shadowTexture.Get(), &shaderView, &m_shadowSrv);
    }
    if (FAILED(hr))
    {
        DL_LOG_WARN("Models", "the shadow map (%d x %d) could not be made (0x%08X): no shadows", size, size,
                    static_cast<unsigned>(hr));
        ReleaseShadowMap();
        return false;
    }
    SetDebugName(m_shadowTexture.Get(), "Models.Shadow.Depth");
    SetDebugName(m_shadowDsv.Get(), "Models.Shadow.DSV");
    SetDebugName(m_shadowSrv.Get(), "Models.Shadow.SRV");
    m_shadowSize = size;
    DL_LOG_INFO("Models", "shadow map: %d x %d", size, size);
    return true;
}

void Models::ReleaseShadowMap()
{
    m_shadowSrv.Reset();
    m_shadowDsv.Reset();
    m_shadowTexture.Reset();
    m_shadowSize = 0;
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

// The ink lines (6.10): the pixel shader of shaders/lines.hlsl (the vertex shader is the one of the ripple), its
// constants and the blend that lays the ink over the picture: result = source.rgb + picture * (1 - source.a). Nothing
// else needs them: when they cannot be made the lines are off and the log says why.
bool Models::CreateLineResources()
{
    m_linePixelShader.Reset();
    m_lineBuffer.Reset();
    m_lineBlend.Reset();
    ComPtr<ID3DBlob> psBlob;
    if (!CompileShader(kLinesShaderPath, "PSMain", "ps_5_0", psBlob))
    {
        DL_LOG_ERROR("Models", "the ink lines are off: %s did not compile", kLinesShaderPath);
        return false;
    }
    HRESULT hr =
        m_device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &m_linePixelShader);
    if (SUCCEEDED(hr))
    {
        D3D11_BUFFER_DESC cb{};
        cb.Usage = D3D11_USAGE_DYNAMIC;
        cb.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        cb.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        cb.ByteWidth = sizeof(LineConstants);
        hr = m_device->CreateBuffer(&cb, nullptr, &m_lineBuffer);
    }
    if (SUCCEEDED(hr))
    {
        D3D11_BLEND_DESC blend{};
        blend.RenderTarget[0].BlendEnable = TRUE;
        blend.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
        blend.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
        blend.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
        blend.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
        blend.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
        blend.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        blend.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        hr = m_device->CreateBlendState(&blend, &m_lineBlend);
    }
    if (FAILED(hr))
    {
        DL_LOG_ERROR("Models", "the ink lines are off: their shader objects failed (0x%08X)",
                     static_cast<unsigned>(hr));
        m_linePixelShader.Reset();
        m_lineBuffer.Reset();
        m_lineBlend.Reset();
        return false;
    }
    SetDebugName(m_linePixelShader.Get(), "Models.Lines.PS");
    SetDebugName(m_lineBuffer.Get(), "Models.Lines.CB");
    SetDebugName(m_lineBlend.Get(), "Models.Lines.BlendState");
    return true;
}

// The shader view of the depth texture of a picture: the two ripple pictures have theirs; the one of the screen (the
// depth of Device) is made the first time and kept while that texture lives. nullptr when it cannot be read.
ID3D11ShaderResourceView* Models::DepthViewOf(ID3D11Resource* depth)
{
    for (const LayerTarget& target : m_targets)
    {
        if (target.depth.Get() == depth && target.depthShaderView)
        {
            return target.depthShaderView.Get();
        }
    }
    if (m_screenDepth.Get() == depth && m_screenDepthView)
    {
        return m_screenDepthView.Get();
    }
    m_screenDepthView.Reset();
    m_screenDepth.Reset();
    ComPtr<ID3D11Texture2D> texture;
    if (FAILED(depth->QueryInterface(IID_PPV_ARGS(&texture))))
    {
        return nullptr;
    }
    D3D11_TEXTURE2D_DESC desc{};
    texture->GetDesc(&desc);
    if (desc.Format != DXGI_FORMAT_R32_TYPELESS || (desc.BindFlags & D3D11_BIND_SHADER_RESOURCE) == 0)
    {
        return nullptr; // a depth buffer of another kind (Device.cpp makes the one of the screen typeless and readable)
    }
    D3D11_SHADER_RESOURCE_VIEW_DESC read{};
    read.Format = DXGI_FORMAT_R32_FLOAT;
    read.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    read.Texture2D.MostDetailedMip = 0;
    read.Texture2D.MipLevels = 1;
    if (FAILED(m_device->CreateShaderResourceView(depth, &read, &m_screenDepthView)))
    {
        return nullptr;
    }
    m_screenDepth = depth;
    SetDebugName(m_screenDepthView.Get(), "Models.Lines.ScreenDepth.SRV");
    return m_screenDepthView.Get();
}

void Models::DrawLines(ID3D11DeviceContext* context, int layer, const XMMATRIX& projection)
{
    const float master = m_debugView == kDebugViewPlain ? 0.0f : m_lighting.style; // the plain view has no line
    if (!m_linePixelShader || !m_lineBuffer || !m_lineBlend || !(master > 0.0f) || !(m_lines.amount > 0.0f))
    {
        return;
    }
    const GpuScope linesTime(m_timer, context, GpuTimer::kLines);
    ComPtr<ID3D11RenderTargetView> target;
    ComPtr<ID3D11DepthStencilView> depthView;
    context->OMGetRenderTargets(1, target.GetAddressOf(), depthView.GetAddressOf());
    if (!target || !depthView)
    {
        return;
    }
    ComPtr<ID3D11Resource> depthResource;
    depthView->GetResource(&depthResource);
    ID3D11ShaderResourceView* depthRead = DepthViewOf(depthResource.Get());
    ComPtr<ID3D11Texture2D> depthTexture;
    if (depthRead == nullptr || FAILED(depthResource.As(&depthTexture)))
    {
        if (!m_lineDepthWarned)
        {
            m_lineDepthWarned = true;
            DL_LOG_WARN("Models", "the ink lines are off: the depth buffer cannot be read in a shader");
        }
        return;
    }
    D3D11_TEXTURE2D_DESC depthDesc{};
    depthTexture->GetDesc(&depthDesc);
    const LineConstants constants =
        MakeLines(m_lines, layer, master, XMVectorGetY(projection.r[1]), static_cast<float>(depthDesc.Height),
                  kNearPlane, kFarPlane, XMVectorGetX(projection.r[0]));
    D3D11_MAPPED_SUBRESOURCE mapped{};
    if (FAILED(context->Map(m_lineBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
    {
        return;
    }
    std::memcpy(mapped.pData, &constants, sizeof(constants));
    context->Unmap(m_lineBuffer.Get(), 0);
    // the depth cannot be read while it is bound for writing: the picture is bound without it for this pass
    ID3D11RenderTargetView* colour = target.Get();
    context->OMSetRenderTargets(1, &colour, nullptr);
    context->OMSetBlendState(m_lineBlend.Get(), nullptr, 0xffffffff);
    context->OMSetDepthStencilState(m_noDepthState.Get(), 0);
    context->RSSetState(m_noCullState.Get());
    context->IASetInputLayout(nullptr); // the vertex shader makes its positions from SV_VertexID
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->VSSetShader(m_rippleVertexShader.Get(), nullptr, 0);
    context->PSSetShader(m_linePixelShader.Get(), nullptr, 0);
    ID3D11Buffer* lineConstants = m_lineBuffer.Get();
    context->PSSetConstantBuffers(0, 1, &lineConstants);
    context->PSSetShaderResources(1, 1, &depthRead); // t0 is the shadow map of the scene: left alone
    context->Draw(3, 0);
    ID3D11ShaderResourceView* none[1] = {}; // the depth must not stay bound for reading: it is written again
    context->PSSetShaderResources(1, 1, none);
    context->OMSetRenderTargets(1, &colour, depthView.Get());
    BindScenePipeline(context); // the shaders, the states and the buffers of the scene again
    ++m_drawCalls;
}

// The rain of B (6.11, R1): the pixel shader of shaders/rain.hlsl (the vertex shader is the one of the ripple), its
// constants and the blend that lays the rain over the picture: result = source.rgb + picture * (1 - source.a). Nothing
// else needs them: when they cannot be made the rain is off and the log says why.
bool Models::CreateRainResources()
{
    m_rainPixelShader.Reset();
    m_rainBuffer.Reset();
    m_rainBlend.Reset();
    ComPtr<ID3DBlob> psBlob;
    if (!CompileShader(kRainShaderPath, "PSMain", "ps_5_0", psBlob))
    {
        DL_LOG_ERROR("Models", "the rain is off: %s did not compile", kRainShaderPath);
        return false;
    }
    HRESULT hr =
        m_device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &m_rainPixelShader);
    if (SUCCEEDED(hr))
    {
        D3D11_BUFFER_DESC cb{};
        cb.Usage = D3D11_USAGE_DYNAMIC;
        cb.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        cb.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        cb.ByteWidth = sizeof(RainConstants);
        hr = m_device->CreateBuffer(&cb, nullptr, &m_rainBuffer);
    }
    if (SUCCEEDED(hr))
    {
        D3D11_BLEND_DESC blend{};
        blend.RenderTarget[0].BlendEnable = TRUE;
        blend.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
        blend.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
        blend.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
        blend.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
        blend.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
        blend.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        blend.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        hr = m_device->CreateBlendState(&blend, &m_rainBlend);
    }
    if (FAILED(hr))
    {
        DL_LOG_ERROR("Models", "the rain is off: its shader objects failed (0x%08X)", static_cast<unsigned>(hr));
        m_rainPixelShader.Reset();
        m_rainBuffer.Reset();
        m_rainBlend.Reset();
        return false;
    }
    SetDebugName(m_rainPixelShader.Get(), "Models.Rain.PS");
    SetDebugName(m_rainBuffer.Get(), "Models.Rain.CB");
    SetDebugName(m_rainBlend.Get(), "Models.Rain.BlendState");
    return true;
}

void Models::DrawRain(ID3D11DeviceContext* context, int layer, const XMMATRIX& view, const XMMATRIX& projection,
                      const XMFLOAT3& eye)
{
    const bool plain = m_debugView == kDebugViewPlain;
    if (layer != 1 || plain || !m_rainPixelShader || !m_rainBuffer || !m_rainBlend || !(m_lighting.style > 0.0f) ||
        !(m_rain.amount > 0.0f))
    {
        return; // the rain is for B, and not in the plain view
    }
    ComPtr<ID3D11RenderTargetView> target;
    ComPtr<ID3D11DepthStencilView> depthView;
    context->OMGetRenderTargets(1, target.GetAddressOf(), depthView.GetAddressOf());
    if (!target || !depthView)
    {
        return;
    }
    ComPtr<ID3D11Resource> depthResource;
    depthView->GetResource(&depthResource);
    ID3D11ShaderResourceView* depthRead = DepthViewOf(depthResource.Get());
    ComPtr<ID3D11Texture2D> depthTexture;
    if (depthRead == nullptr || FAILED(depthResource.As(&depthTexture)))
    {
        if (!m_rainDepthWarned)
        {
            m_rainDepthWarned = true;
            DL_LOG_WARN("Models", "the rain is off: the depth buffer cannot be read in a shader");
        }
        return;
    }
    const GpuScope rainTime(m_timer, context, GpuTimer::kRain);
    D3D11_TEXTURE2D_DESC depthDesc{};
    depthTexture->GetDesc(&depthDesc);
    // the camera: its axes are the rows of the transposed view (docs/CONVENTIONS.md), unit length
    const XMMATRIX axes = XMMatrixTranspose(view);
    RainView rv;
    rv.tanAcross = 1.0f / XMVectorGetX(projection.r[0]);
    rv.tanUp = 1.0f / XMVectorGetY(projection.r[1]);
    rv.nearZ = kNearPlane;
    rv.farZ = kFarPlane;
    rv.heightPixels = static_cast<float>(depthDesc.Height);
    rv.time = m_time;
    XMFLOAT3 axis{};
    XMStoreFloat3(&axis, axes.r[0]);
    rv.right[0] = axis.x;
    rv.right[1] = axis.y;
    rv.right[2] = axis.z;
    XMStoreFloat3(&axis, axes.r[1]);
    rv.up[0] = axis.x;
    rv.up[1] = axis.y;
    rv.up[2] = axis.z;
    XMStoreFloat3(&axis, axes.r[2]);
    rv.forward[0] = axis.x;
    rv.forward[1] = axis.y;
    rv.forward[2] = axis.z;
    rv.eye[0] = eye.x;
    rv.eye[1] = eye.y;
    rv.eye[2] = eye.z;
    // the rain falls the way the moonlight travels (6.11): from the moon to the ground
    const Theme moon = ThemeOf(1);
    XMFLOAT3 toMoon{};
    XMStoreFloat3(&toMoon, XMVector3Normalize(XMLoadFloat3(&moon.toLight)));
    rv.travel[0] = -toMoon.x;
    rv.travel[1] = -toMoon.y;
    rv.travel[2] = -toMoon.z;
    rv.lantern[0] = m_lantern.x;
    rv.lantern[1] = m_lantern.y;
    rv.lantern[2] = m_lantern.z;
    rv.lantern[3] = m_lantern.w;
    const RainConstants constants = MakeRain(m_rain, layer, plain, rv);
    D3D11_MAPPED_SUBRESOURCE mapped{};
    if (FAILED(context->Map(m_rainBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
    {
        return;
    }
    std::memcpy(mapped.pData, &constants, sizeof(constants));
    context->Unmap(m_rainBuffer.Get(), 0);
    // the depth cannot be read while it is bound for writing: the picture is bound without it for this pass
    ID3D11RenderTargetView* colour = target.Get();
    context->OMSetRenderTargets(1, &colour, nullptr);
    context->OMSetBlendState(m_rainBlend.Get(), nullptr, 0xffffffff);
    context->OMSetDepthStencilState(m_noDepthState.Get(), 0);
    context->RSSetState(m_noCullState.Get());
    context->IASetInputLayout(nullptr); // the vertex shader makes its positions from SV_VertexID
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->VSSetShader(m_rippleVertexShader.Get(), nullptr, 0);
    context->PSSetShader(m_rainPixelShader.Get(), nullptr, 0);
    ID3D11Buffer* rainConstants = m_rainBuffer.Get();
    context->PSSetConstantBuffers(0, 1, &rainConstants);
    context->PSSetShaderResources(1, 1, &depthRead); // t0 is the shadow map of the scene: left alone
    context->Draw(3, 0);
    ID3D11ShaderResourceView* none[1] = {}; // the depth must not stay bound for reading: it is written again
    context->PSSetShaderResources(1, 1, none);
    context->OMSetRenderTargets(1, &colour, depthView.Get());
    BindScenePipeline(context); // the shaders, the states and the buffers of the scene again
    ++m_drawCalls;
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
            // the same depth as Device (D32_FLOAT, docs/CONVENTIONS.md), typeless so that the ink lines can read it
            desc.Format = DXGI_FORMAT_R32_TYPELESS;
            desc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
            hr = m_device->CreateTexture2D(&desc, nullptr, &target.depth);
        }
        if (SUCCEEDED(hr))
        {
            D3D11_DEPTH_STENCIL_VIEW_DESC depthDesc{};
            depthDesc.Format = DXGI_FORMAT_D32_FLOAT;
            depthDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
            hr = m_device->CreateDepthStencilView(target.depth.Get(), &depthDesc, &target.depthView);
        }
        if (SUCCEEDED(hr))
        {
            D3D11_SHADER_RESOURCE_VIEW_DESC readDesc{};
            readDesc.Format = DXGI_FORMAT_R32_FLOAT;
            readDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            readDesc.Texture2D.MostDetailedMip = 0;
            readDesc.Texture2D.MipLevels = 1;
            hr = m_device->CreateShaderResourceView(target.depth.Get(), &readDesc, &target.depthShaderView);
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

Models::Theme Models::ThemeOf(int layer) const
{
    return m_themes[static_cast<std::size_t>(layer == 0 ? 0 : 1)];
}

void Models::SetStaticTown(const BatchMesh& layerA, const BatchMesh& layerB)
{
    m_townMesh[0] = layerA;
    m_townMesh[1] = layerB;
    if (m_device)
    {
        CreateBatchBuffers();
    }
}

void Models::SetStaticWater(const BatchMesh& layerA, const BatchMesh& layerB)
{
    m_waterMesh[0] = layerA;
    m_waterMesh[1] = layerB;
    if (m_device)
    {
        CreateBatchBuffers();
    }
}

// One baked mesh into an immutable vertex and index buffer. An empty mesh leaves `target` empty (not an error).
bool Models::UploadBatch(const BatchMesh& baked, GpuMesh& target, const char* vertexName, const char* indexName)
{
    target = GpuMesh{};
    if (baked.indices.empty())
    {
        return true;
    }
    D3D11_BUFFER_DESC desc{};
    desc.Usage = D3D11_USAGE_IMMUTABLE;
    desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    desc.ByteWidth = static_cast<UINT>(baked.vertices.size() * sizeof(BatchVertex));
    D3D11_SUBRESOURCE_DATA initial{};
    initial.pSysMem = baked.vertices.data();
    HRESULT hr = m_device->CreateBuffer(&desc, &initial, &target.vertexBuffer);
    if (SUCCEEDED(hr))
    {
        desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        desc.ByteWidth = static_cast<UINT>(baked.indices.size() * sizeof(std::uint32_t));
        initial.pSysMem = baked.indices.data();
        hr = m_device->CreateBuffer(&desc, &initial, &target.indexBuffer);
    }
    if (FAILED(hr))
    {
        DL_LOG_ERROR("Models", "%s could not be made (0x%08X)", vertexName, static_cast<unsigned>(hr));
        target = GpuMesh{};
        return false;
    }
    target.indexCount = static_cast<UINT>(baked.indices.size());
    SetDebugName(target.vertexBuffer.Get(), vertexName);
    SetDebugName(target.indexBuffer.Get(), indexName);
    return true;
}

// The fixed boxes of each layer, baked into one immutable vertex and index buffer; the still water of each layer into
// its own. Nothing to bake is not an error.
bool Models::CreateBatchBuffers()
{
    static const char* const kNames[2][4] = {
        {"Models.Batch.A.VB", "Models.Batch.A.IB", "Models.Water.A.VB", "Models.Water.A.IB"},
        {"Models.Batch.B.VB", "Models.Batch.B.IB", "Models.Water.B.VB", "Models.Water.B.IB"}};
    for (int layer = 0; layer < 2; ++layer)
    {
        const std::size_t l = static_cast<std::size_t>(layer);
        const BatchMesh& baked = m_townMesh[l]; // the polygons of the town
        if (!UploadBatch(baked, m_batch[l], kNames[layer][0], kNames[layer][1]) ||
            !UploadBatch(m_waterMesh[l], m_waterBatch[l], kNames[layer][2], kNames[layer][3]))
        {
            return false;
        }
        if (!baked.indices.empty())
        {
            DL_LOG_INFO("Models", "static batch %s: %d polygons, %zu triangles in one draw call",
                        layer == 0 ? "A" : "B", baked.objects, baked.indices.size() / 3);
        }
    }
    return true;
}

// ---------------------------------------------------------------------------------------------------------------------
// the look of the layers and the ripple
// ---------------------------------------------------------------------------------------------------------------------

void Models::LoadThemes(const Config& config)
{
    m_townAtmosphere = config.GetBool("render", "town_atmosphere", true);
    static const char* const kSections[2] = {"layer_a", "layer_b"};
    const std::array<Theme, 2> defaults = DefaultThemes();
    for (std::size_t i = 0; i < 2; ++i)
    {
        const char* s = kSections[i];
        const Theme& d = defaults[i];
        Theme& t = m_themes[i];
        t.toLight = XMFLOAT3(config.GetFloat(s, "light_x", d.toLight.x), config.GetFloat(s, "light_y", d.toLight.y),
                             config.GetFloat(s, "light_z", d.toLight.z));
        t.lightColor =
            XMFLOAT3(config.GetFloat(s, "light_r", d.lightColor.x), config.GetFloat(s, "light_g", d.lightColor.y),
                     config.GetFloat(s, "light_b", d.lightColor.z));
        t.sky = XMFLOAT3(config.GetFloat(s, "sky_r", d.sky.x), config.GetFloat(s, "sky_g", d.sky.y),
                         config.GetFloat(s, "sky_b", d.sky.z));
        t.clear = XMFLOAT3(config.GetFloat(s, "clear_r", d.clear.x), config.GetFloat(s, "clear_g", d.clear.y),
                           config.GetFloat(s, "clear_b", d.clear.z));
        t.fogDensity = std::clamp(config.GetFloat(s, "fog_density", d.fogDensity), 0.0f, 0.2f);
        t.tint = XMFLOAT3(std::clamp(config.GetFloat(s, "tint_r", d.tint.x), 0.0f, 4.0f),
                          std::clamp(config.GetFloat(s, "tint_g", d.tint.y), 0.0f, 4.0f),
                          std::clamp(config.GetFloat(s, "tint_b", d.tint.z), 0.0f, 4.0f));
        t.weathering = std::clamp(config.GetFloat(s, "weathering", d.weathering), 0.0f, 1.0f);
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

void Models::LoadLighting(const Config& config)
{
    SetLighting(ReadLightingSettings(config));
    m_lines = ReadLineSettings(config);
    m_rain = ReadRainSettings(config);
    DL_LOG_INFO("Models", "lighting: shadows %s %d (radius %.0f m), lamps %s, exposure %.2f, knee %.2f",
                m_lighting.shadows ? "on" : "off", m_lighting.shadowSize, static_cast<double>(m_lighting.shadowRadius),
                m_lighting.pointLights ? "on" : "off", static_cast<double>(m_lighting.exposure),
                static_cast<double>(m_lighting.knee));
}

void Models::SetLighting(const LightingSettings& settings)
{
    m_lighting = Sanitize(settings);
    if (!m_device)
    {
        return;
    }
    if (!m_lighting.shadows)
    {
        ReleaseShadowMap(); // no shadows: the memory goes back
    }
    else if (m_shadowSize != m_lighting.shadowSize || !m_shadowDsv)
    {
        if (!CreateShadowMap(m_lighting.shadowSize))
        {
            m_lighting.shadows = false;
        }
    }
}

void Models::ClearColor(float out[4]) const
{
    const XMFLOAT3 clear = ThemeOf(m_ripple.Outer()).clear;
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
    m_ripple.Update(dt);
    m_time = std::fmod(m_time + dt, 3600.0f); // the water moves on (wrapped once an hour: a float keeps its precision)
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
    ID3D11Buffer* constants[3] = {m_frameBuffer.Get(), m_objectBuffer.Get(), m_lampBuffer.Get()};
    context->VSSetConstantBuffers(0, 3, constants);
    context->PSSetConstantBuffers(0, 3, constants);
    ID3D11SamplerState* compare =
        m_shadowSampler.Get(); // the shader declares the shadow sampler even when there are no shadows
    context->PSSetSamplers(0, 1, &compare);
}

// One object: its constants, then its shape. Used by the shadow pass (which only needs its place) and the normal pass
// alike.
void Models::DrawObject(ID3D11DeviceContext* context, const SceneObject& object, int& drawCounter)
{
    const GpuMesh& mesh = m_meshes[static_cast<std::size_t>(object.shape)];
    if (!mesh.vertexBuffer || !mesh.indexBuffer)
    {
        return;
    }
    // World = Scale * Rotation * Translation: first the size, then the tilt and the turn, then the place (row vectors:
    // read left to right)
    const XMMATRIX world = XMMatrixScaling(object.scale[0], object.scale[1], object.scale[2]) *
                           XMMatrixRotationX(object.tilt) * XMMatrixRotationY(object.yaw) *
                           XMMatrixTranslation(object.position[0], object.position[1], object.position[2]);
    ObjectConstants data{};
    XMStoreFloat4x4(&data.world, world);
    // A normal is not moved like a point: on a stretched object it must be turned the other way (the inverse
    // transpose), or the light is wrong. For this affine matrix the 3x3 part of the result is the inverse transpose of
    // the 3x3 of `world`.
    XMStoreFloat4x4(&data.normalMatrix, XMMatrixTranspose(XMMatrixInverse(nullptr, world)));
    // the city (an object of both layers that does not shine by itself) takes the stone of the layer; the key objects
    // of one layer and everything that glows keep their own colours, so that red, blue and gold always mean the same
    const float city = (object.layerMask == scene::kLayerBoth && object.glow <= 0.0f) ? 1.0f : 0.0f;
    data.color = XMFLOAT4(SrgbToLinear(object.colorSrgb[0]), SrgbToLinear(object.colorSrgb[1]),
                          SrgbToLinear(object.colorSrgb[2]), city);
    data.params = XMFLOAT4(object.checker, object.shineStrength, object.shininess, object.glow);
    D3D11_MAPPED_SUBRESOURCE objectMap{};
    if (FAILED(context->Map(m_objectBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &objectMap)))
    {
        return;
    }
    std::memcpy(objectMap.pData, &data, sizeof(data));
    context->Unmap(m_objectBuffer.Get(), 0);
    ID3D11Buffer* vertexBuffer = mesh.vertexBuffer.Get();
    const UINT stride = sizeof(MeshVertex);
    const UINT offset = 0;
    context->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
    context->IASetIndexBuffer(mesh.indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
    context->DrawIndexed(mesh.indexCount, 0, 0);
    ++drawCounter;
}

void Models::DrawObjects(ID3D11DeviceContext* context, const XMMATRIX& view, const XMMATRIX& projection,
                         const XMFLOAT3& eye, int layer)
{
    // ---- the camera axes (for the sky) and the light of this moment
    const XMMATRIX cameraAxes = XMMatrixTranspose(view);
    XMFLOAT3 right{}, up{}, forward{};
    XMStoreFloat3(&right, XMVectorScale(cameraAxes.r[0], 1.0f / XMVectorGetX(projection.r[0])));
    XMStoreFloat3(&up, XMVectorScale(cameraAxes.r[1], 1.0f / XMVectorGetY(projection.r[1])));
    XMStoreFloat3(&forward, cameraAxes.r[2]);

    // ---- the shadow box of the sun or the moon: round a point a little ahead of the camera, on the texel grid of the
    // map
    const Theme sunTheme = ThemeOf(layer);
    XMFLOAT3 toLight{};
    XMStoreFloat3(&toLight, XMVector3Normalize(XMLoadFloat3(&sunTheme.toLight)));
    const bool wantShadow = m_lighting.shadows && m_shadowDsv && m_shadowSrv && toLight.y > 0.05f;
    ShadowFit fit;
    if (wantShadow)
    {
        const float flat = std::sqrt(forward.x * forward.x + forward.z * forward.z);
        const float ahead = m_lighting.shadowRadius * kShadowAhead;
        const float centre[3] = {eye.x + (flat > 1.0e-4f ? forward.x / flat : 0.0f) * ahead, eye.y - 2.0f,
                                 eye.z + (flat > 1.0e-4f ? forward.z / flat : 1.0f) * ahead};
        const float lightDir[3] = {toLight.x, toLight.y, toLight.z};
        fit = FitSunShadow(lightDir, centre, m_lighting.shadowRadius, kShadowHalfDepth, m_shadowSize);
    }
    bool shadowOn = false; // true once the map of this moment is drawn: then the model shader looks it up

    // ---- the frame constants of a theme (the layer drawn can borrow the theme of the other: the circle of the
    // lantern)
    const auto makeFrame = [&](int themeLayer, float mode)
    {
        const Theme theme = ThemeOf(themeLayer);
        const float sunScale = themeLayer == 0 ? m_lighting.sunA : m_lighting.sunB;
        const float skyScale = themeLayer == 0 ? m_lighting.ambientA : m_lighting.ambientB;
        FrameConstants frame{};
        XMStoreFloat4x4(&frame.viewProjection, view * projection);
        frame.eye = eye;
        frame.time = m_time;
        XMStoreFloat3(&frame.toLight, XMVector3Normalize(XMLoadFloat3(&theme.toLight)));
        frame.ambient = 1.0f;
        frame.lightColor =
            XMFLOAT3(theme.lightColor.x * sunScale, theme.lightColor.y * sunScale, theme.lightColor.z * sunScale);
        frame.lanternMode = mode;
        frame.skyUp = XMFLOAT3(theme.sky.x * skyScale, theme.sky.y * skyScale, theme.sky.z * skyScale);
        frame.skyDown = XMFLOAT3(theme.sky.x * kSkyDownScale * skyScale, theme.sky.y * kSkyDownScale * skyScale,
                                 theme.sky.z * kSkyDownScale * skyScale);
        frame.fogColor =
            theme.clear; // the fog is the colour of the sky behind it: what is far melts into the background
        frame.fogDensity = theme.fogDensity;
        frame.tint = theme.tint;
        frame.weathering = theme.weathering;
        frame.lantern = m_lantern;
        if (m_townAtmosphere && theme.gradientSky)
        {
            frame.skyDown =
                XMFLOAT3(theme.groundFill.x * skyScale, theme.groundFill.y * skyScale, theme.groundFill.z * skyScale);
            frame.skyRight = XMFLOAT4(right.x, right.y, right.z, theme.mistDensity);
            frame.skyUpRay = XMFLOAT4(up.x, up.y, up.z, theme.mistBase);
            frame.skyForward = XMFLOAT4(forward.x, forward.y, forward.z, themeLayer == 0 ? 1.0f : 0.42f);
            frame.skyZenith = XMFLOAT4(theme.zenith.x, theme.zenith.y, theme.zenith.z, 1.0f);
        }
        // the shadow: only in the moment drawn on its own and in the part of B that is B (mode 2, the circle of the
        // lantern, shows A)
        const float texel = 1.0f / static_cast<float>(m_shadowSize > 0 ? m_shadowSize : 1);
        if (wantShadow)
        {
            std::memcpy(&frame.lightViewProj, fit.viewProj, sizeof(fit.viewProj));
        }
        else
        {
            XMStoreFloat4x4(&frame.lightViewProj,
                            XMMatrixTranslation(0.0f, 0.0f, 0.0f)); // (the shader does not look at it)
        }
        const float depthBias = wantShadow ? m_lighting.shadowBias * fit.texelWorld / fit.depthRange : 0.0f;
        const float normalOffset = wantShadow ? m_lighting.shadowNormal * fit.texelWorld : 0.0f;
        frame.shadow =
            XMFLOAT4(texel, depthBias, normalOffset, shadowOn && mode < 1.5f ? m_lighting.shadowStrength : 0.0f);
        frame.grade =
            XMFLOAT4(m_lighting.exposure, m_lighting.knee, static_cast<float>(m_debugView), m_lighting.sunScatter);
        // the style of the light of this theme (render/LightingSettings.h); the debug view "plain" leaves it out
        const StyleConstants style = MakeStyle(m_lighting, themeLayer, m_debugView == kDebugViewPlain);
        std::memcpy(&frame.grade2, style.grade2, sizeof(style.grade2));
        std::memcpy(&frame.styleCool, style.cool, sizeof(style.cool));
        std::memcpy(&frame.styleLow, style.low, sizeof(style.low));
        std::memcpy(&frame.styleHigh, style.high, sizeof(style.high));
        std::memcpy(&frame.styleLight, style.light, sizeof(style.light));
        std::memcpy(&frame.styleFar, style.hazeTint, sizeof(style.hazeTint));
        std::memcpy(&frame.styleMisc, style.misc, sizeof(style.misc));
        std::memcpy(&frame.styleMore, style.more, sizeof(style.more));
        // the rain of B (6.11): the stone is wet, the sky overcast and the fog thicker; A and the plain view are as
        // they were, and so is the circle of the lantern in B (drawn with the frame of A)
        const RainWet wet = MakeWet(m_rain, themeLayer, m_debugView == kDebugViewPlain || !(m_lighting.style > 0.0f));
        frame.styleMore.z = wet.wet;
        frame.styleMore.w = wet.overcast;
        frame.fogDensity *= wet.fogScale;
        return frame;
    };
    const auto writeFrame = [&](const FrameConstants& frame)
    {
        D3D11_MAPPED_SUBRESOURCE mapped{};
        if (FAILED(context->Map(m_frameBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
        {
            return false;
        }
        std::memcpy(mapped.pData, &frame, sizeof(frame));
        context->Unmap(m_frameBuffer.Get(), 0);
        return true;
    };
    const auto setFrame = [&](int themeLayer, float mode) { return writeFrame(makeFrame(themeLayer, mode)); };
    // one baked mesh in one draw call (render/StaticBatch.h)
    const auto drawBatch = [&](const GpuMesh& batch, int& counter)
    {
        if (batch.indexCount == 0)
        {
            return;
        }
        context->IASetInputLayout(m_batchLayout.Get());
        context->VSSetShader(m_batchVertexShader.Get(), nullptr, 0);
        ID3D11Buffer* batchBuffer = batch.vertexBuffer.Get();
        const UINT batchStride = sizeof(BatchVertex);
        const UINT batchOffset = 0;
        context->IASetVertexBuffers(0, 1, &batchBuffer, &batchStride, &batchOffset);
        context->IASetIndexBuffer(batch.indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
        context->DrawIndexed(batch.indexCount, 0, 0);
        ++counter;
        context->IASetInputLayout(m_inputLayout.Get()); // back to the objects drawn one by one
        context->VSSetShader(m_vertexShader.Get(), nullptr, 0);
    };
    const unsigned layerBit = 1u << layer;

    // ---- the lamps of this moment: the nearest and strongest, into the constant buffer b2
    {
        LampBatch lamps;
        if (m_lighting.pointLights)
        {
            const float eyeAt[3] = {eye.x, eye.y, eye.z};
            SelectLamps(m_fixedLamps, m_movingLamps, layer, eyeAt, layer == 0 ? m_lighting.lampsA : m_lighting.lampsB,
                        m_lighting.lantern, m_time, lamps);
        }
        LampConstants constants{};
        constants.count[0] = static_cast<float>(lamps.count);
        std::memcpy(constants.pos, lamps.pos, sizeof(constants.pos));
        std::memcpy(constants.tint, lamps.color, sizeof(constants.tint));
        D3D11_MAPPED_SUBRESOURCE mapped{};
        if (SUCCEEDED(context->Map(m_lampBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
        {
            std::memcpy(mapped.pData, &constants, sizeof(constants));
            context->Unmap(m_lampBuffer.Get(), 0);
        }
    }

    // ---- the shadow pass: the casters of this moment seen from the light, into the shadow map (depth only)
    if (wantShadow)
    {
        const GpuScope shadowTime(m_timer, context, GpuTimer::kShadow);
        ComPtr<ID3D11RenderTargetView> savedView;
        ComPtr<ID3D11DepthStencilView> savedDepth;
        context->OMGetRenderTargets(1, savedView.GetAddressOf(), savedDepth.GetAddressOf());
        D3D11_VIEWPORT savedViewport{};
        UINT savedViewports = 1;
        context->RSGetViewports(&savedViewports, &savedViewport);
        ID3D11ShaderResourceView* noView[1] = {}; // the map cannot be read while it is drawn into
        context->PSSetShaderResources(0, 1, noView);
        context->OMSetRenderTargets(0, nullptr, m_shadowDsv.Get());
        const D3D11_VIEWPORT mapViewport{0.0f, 0.0f, static_cast<float>(m_shadowSize), static_cast<float>(m_shadowSize),
                                         0.0f, 1.0f};
        context->RSSetViewports(1, &mapViewport);
        context->ClearDepthStencilView(m_shadowDsv.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
        BindScenePipeline(context);
        context->RSSetState(m_shadowRaster.Get());
        context->PSSetShader(nullptr, nullptr, 0); // depth only
        FrameConstants lightFrame = makeFrame(layer, 0.0f);
        std::memcpy(&lightFrame.viewProjection, fit.viewProj,
                    sizeof(fit.viewProj)); // the same shaders, seen from the light
        if (writeFrame(lightFrame))
        {
            drawBatch(m_batch[static_cast<std::size_t>(layer)], m_shadowDraws);
            for (const SceneObject& object : Objects())
            {
                // what casts: the objects of this moment that are solid things, not water, not glow, not a halo
                if ((object.layerMask & layerBit) != 0 && !scene::IsWaterMaterial(object.checker) &&
                    !scene::IsHalo(object.checker) && object.glow <= 0.05f)
                {
                    DrawObject(context, object, m_shadowDraws);
                }
            }
            shadowOn = true;
        }
        ID3D11RenderTargetView* renderView = savedView.Get();
        context->OMSetRenderTargets(1, &renderView, savedDepth.Get());
        if (savedViewports > 0)
        {
            context->RSSetViewports(1, &savedViewport);
        }
        BindScenePipeline(context); // the shaders, the states and the buffers of the picture again
        if (shadowOn)
        {
            ID3D11ShaderResourceView* map = m_shadowSrv.Get();
            context->PSSetShaderResources(0, 1, &map);
            ID3D11SamplerState* compare = m_shadowSampler.Get();
            context->PSSetSamplers(0, 1, &compare);
        }
    }

    // the lantern set down in A keeps a circle of A standing in B (scene/Lantern.h): B is drawn without the inside of
    // the circle, then the fixed town of A, in the light of A, only inside it
    const bool circle = layer == 1 && m_lantern.w > 0.0f;
    // ---- the fixed city of this layer: one draw call
    if (!setFrame(layer, circle ? 1.0f : 0.0f))
    {
        return;
    }
    if (m_townAtmosphere && ThemeOf(layer).gradientSky)
    {
        // The background belongs to the whole layer, never to the local lantern patch.
        context->OMSetDepthStencilState(m_noDepthState.Get(), 0);
        context->RSSetState(m_noCullState.Get());
        context->IASetInputLayout(nullptr);
        context->VSSetShader(m_rippleVertexShader.Get(), nullptr, 0); // SV_VertexID fullscreen triangle
        context->PSSetShader(m_skyPixelShader.Get(), nullptr, 0);
        context->Draw(3, 0);
        ++m_drawCalls;
        BindScenePipeline(context);
    }
    drawBatch(m_batch[static_cast<std::size_t>(layer)], m_drawCalls);
    if (circle)
    {
        setFrame(0, 2.0f);
        drawBatch(m_batch[0], m_drawCalls);
        setFrame(layer, 0.0f); // the objects drawn one by one are not clipped
    }
    // ---- one draw call per object: only the objects of this layer; the water (scene::IsWaterMaterial) waits for its
    // own pass, the halos for theirs
    m_water.clear();
    m_halos.clear();
    for (const SceneObject& object : Objects())
    {
        if ((object.layerMask & layerBit) == 0)
        {
            continue; // not on the layer we are drawing
        }
        if (scene::IsWaterMaterial(object.checker))
        {
            m_water.push_back(&object);
            continue;
        }
        if (scene::IsHalo(object.checker))
        {
            m_halos.push_back(&object);
            continue;
        }
        DrawObject(context, object, m_drawCalls);
    }
    // ---- the ink lines (6.10): over everything opaque of this picture, under the water (which is blended over them)
    DrawLines(context, layer, projection);
    // ---- the water: after everything opaque, blended; the still water of the static batch first, then the water
    // objects far to near (the sea first: it is under everything else)
    const GpuMesh& stillWater = m_waterBatch[static_cast<std::size_t>(layer)];
    const bool waterOfA = circle && m_waterBatch[0].indexCount > 0; // the water of A inside the circle
    if (stillWater.indexCount > 0 || waterOfA)
    {
        context->OMSetBlendState(m_waterBlend.Get(), nullptr, 0xffffffff);
        context->OMSetDepthStencilState(m_waterDepth.Get(), 0);
        if (stillWater.indexCount > 0)
        {
            setFrame(layer, circle ? 1.0f : 0.0f);
            drawBatch(stillWater, m_drawCalls);
        }
        if (waterOfA)
        {
            setFrame(0, 2.0f);
            drawBatch(m_waterBatch[0], m_drawCalls);
        }
        setFrame(layer, 0.0f);
        context->OMSetBlendState(m_blendState.Get(), nullptr, 0xffffffff);
        context->OMSetDepthStencilState(m_depthState.Get(), 0);
    }
    if (!m_water.empty())
    {
        const auto farness = [&](const SceneObject* o)
        {
            if (o->checker > scene::kMatSea - 0.5f && o->checker < scene::kMatSea + 0.5f)
            {
                return 1.0e12f;
            }
            const float dx = o->position[0] - eye.x;
            const float dy = o->position[1] - eye.y;
            const float dz = o->position[2] - eye.z;
            return dx * dx + dy * dy + dz * dz;
        };
        std::sort(m_water.begin(), m_water.end(),
                  [&](const SceneObject* a, const SceneObject* b) { return farness(a) > farness(b); });
        context->OMSetBlendState(m_waterBlend.Get(), nullptr, 0xffffffff);
        context->OMSetDepthStencilState(m_waterDepth.Get(), 0);
        for (const SceneObject* object : m_water)
        {
            DrawObject(context, *object, m_drawCalls);
        }
        context->OMSetBlendState(m_blendState.Get(), nullptr, 0xffffffff);
        context->OMSetDepthStencilState(m_depthState.Get(), 0);
    }
    // ---- the halos: added over everything, tested against the depth (a wall hides them) but not written, seen from
    // inside as well
    if (!m_halos.empty())
    {
        context->OMSetBlendState(m_additiveBlend.Get(), nullptr, 0xffffffff);
        context->OMSetDepthStencilState(m_waterDepth.Get(), 0);
        context->RSSetState(m_noCullState.Get());
        for (const SceneObject* object : m_halos)
        {
            DrawObject(context, *object, m_drawCalls);
        }
        context->OMSetBlendState(m_blendState.Get(), nullptr, 0xffffffff);
        context->OMSetDepthStencilState(m_depthState.Get(), 0);
        context->RSSetState(m_wireframe ? m_wireState.Get() : m_solidState.Get());
    }
    // ---- the rain of B (6.11): over everything of the picture, the water and the halos too
    DrawRain(context, layer, view, projection, eye);
    if (shadowOn)
    {
        ID3D11ShaderResourceView* none[1] =
            {}; // the map is drawn into again for the next moment: it must not stay bound
        context->PSSetShaderResources(0, 1, none);
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
        const XMFLOAT3 clearColor = ThemeOf(layers[i]).clear;
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
    ripple.radius = m_ripple.Opened() * (FarthestCornerDistance(m_rippleCenter.x, m_rippleCenter.y, aspect) +
                                         kRippleOvershoot * m_rippleWidth);
    ripple.ringWidth = m_rippleWidth;
    ripple.aspect = aspect;
    ripple.distortion = m_rippleDistortion;
    const XMFLOAT3 glow = ThemeOf(m_ripple.Inner()).lightColor;
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
    const GpuScope sceneTime(m_timer, context, GpuTimer::kScene);
    m_drawCalls = 0;
    m_shadowDraws = 0;

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
    int total = static_cast<int>(m_batch[static_cast<std::size_t>(m_ripple.Target())].indexCount / 3);
    for (const SceneObject& object : Objects())
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
    for (const SceneObject& object : Objects())
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
    swprintf_s(text, L"%d models | %d draws | %d segments | %d triangles | %s | layer %s", VisibleModelCount(),
               m_drawCalls, m_segments, TriangleTotal(), m_wireframe ? L"wireframe" : L"solid",
               m_ripple.Target() == 0 ? L"A" : L"B");
    return text;
}

} // namespace dl
