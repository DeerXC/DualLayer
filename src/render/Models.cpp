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

constexpr char kShaderPath[] = "shaders/models.hlsl";
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

static_assert(sizeof(FrameConstants) == 144, "cbuffer Frame is 9 registers of 16 bytes");
static_assert(sizeof(ObjectConstants) == 160, "cbuffer Object is 10 registers of 16 bytes");

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
constexpr float kDefaultFadeSeconds = 0.35f; // UNITS: seconds
constexpr float kSkyDownScale = 0.28f;       // the ground light is this part of the sky light (the same ratio as in M1)

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

/// Compiles one entry point of models.hlsl; logs the compiler's message on failure.
bool CompileShader(const char* entry, const char* target, ComPtr<ID3DBlob>& out)
{
    UINT flags = D3DCOMPILE_PACK_MATRIX_ROW_MAJOR |
                 D3DCOMPILE_ENABLE_STRICTNESS; // docs/CONVENTIONS.md: row-major, as common.hlsli
#if defined(_DEBUG)
    flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
    flags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif
    wchar_t widePath[MAX_PATH];
    MultiByteToWideChar(CP_UTF8, 0, kShaderPath, -1, widePath, MAX_PATH);
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
            DL_LOG_ERROR("Models", "cannot read %s (0x%08X): is the working directory the project folder?", kShaderPath,
                         static_cast<unsigned>(hr));
        }
        return false;
    }
    return true;
}

/// Every state that Models::Draw changes is put back when this goes out of scope. WHY: Triangle does not set the
/// rasterizer state, so a wireframe state left behind would make the test triangle a wireframe too.
class StateGuard
{
  public:
    explicit StateGuard(ID3D11DeviceContext* context) : m_context(context)
    {
        context->RSGetState(&m_rasterizer);
        context->OMGetDepthStencilState(&m_depthStencil, &m_stencilRef);
        context->OMGetBlendState(&m_blend, m_blendFactor, &m_sampleMask);
        context->IAGetInputLayout(&m_layout);
        context->IAGetPrimitiveTopology(&m_topology);
        context->IAGetVertexBuffers(0, 1, &m_vertexBuffer, &m_stride, &m_offset);
        context->IAGetIndexBuffer(&m_indexBuffer, &m_indexFormat, &m_indexOffset);
        context->VSGetShader(&m_vertexShader, nullptr, nullptr);
        context->PSGetShader(&m_pixelShader, nullptr, nullptr);
        context->VSGetConstantBuffers(0, 2, m_vsConstants);
        context->PSGetConstantBuffers(0, 2, m_psConstants);
    }

    ~StateGuard()
    {
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
    m_fadeSeconds = kDefaultFadeSeconds;
    ComPtr<ID3DBlob> vsBlob;
    ComPtr<ID3DBlob> psBlob;
    if (!CompileShader("VSMain", "vs_5_0", vsBlob) || !CompileShader("PSMain", "ps_5_0", psBlob))
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
    if (!CreateStates() || !CreateMeshes(m_segments))
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
    D3D11_BLEND_DESC blend{};
    blend.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL; // no blending: the models are opaque
    if (SUCCEEDED(hr))
    {
        hr = m_device->CreateBlendState(&blend, &m_blendState);
    }
    if (FAILED(hr))
    {
        DL_LOG_ERROR("Models", "creating the render states failed (0x%08X)", static_cast<unsigned>(hr));
        return false;
    }
    SetDebugName(m_solidState.Get(), "Models.Rasterizer.Solid");
    SetDebugName(m_wireState.Get(), "Models.Rasterizer.Wire");
    SetDebugName(m_depthState.Get(), "Models.DepthState");
    SetDebugName(m_blendState.Get(), "Models.BlendState");
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
// the look of the layers
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
    m_fadeSeconds = std::clamp(config.GetFloat("layers", "fade_seconds", kDefaultFadeSeconds), 0.0f, 5.0f);
    DL_LOG_INFO("Models", "layer themes loaded, fade %.2f s", m_fadeSeconds);
}

Models::Theme Models::BlendedTheme() const
{
    // smoothstep: the fade starts and ends gently (a straight line would look like a mechanical wipe)
    const float b = m_layerBlend;
    const float t = b * b * (3.0f - 2.0f * b);
    const auto mix = [t](const XMFLOAT3& a, const XMFLOAT3& c)
    { return XMFLOAT3(a.x + (c.x - a.x) * t, a.y + (c.y - a.y) * t, a.z + (c.z - a.z) * t); };
    Theme result;
    result.toLight = mix(m_themes[0].toLight, m_themes[1].toLight);
    result.lightColor = mix(m_themes[0].lightColor, m_themes[1].lightColor);
    result.sky = mix(m_themes[0].sky, m_themes[1].sky);
    result.clear = mix(m_themes[0].clear, m_themes[1].clear);
    return result;
}

unsigned Models::VisibleLayer() const
{
    // the objects swap when the fade is half way: the picture is then in between the two colours
    return m_layerBlend >= 0.5f ? 1u : 0u;
}

void Models::ClearColor(float out[4]) const
{
    const Theme theme = BlendedTheme();
    out[0] = theme.clear.x;
    out[1] = theme.clear.y;
    out[2] = theme.clear.z;
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
    m_layer = 1u - m_layer; // 0 <-> 1
    DL_LOG_INFO("Models", "layer: going to %s", m_layer == 0 ? "A" : "B");
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
    // the fade runs even while the shapes are stopped: the layer must still change
    const float target = m_layer == 0 ? 0.0f : 1.0f;
    if (m_fadeSeconds <= 0.0f)
    {
        m_layerBlend = target;
    }
    else
    {
        const float step = dt / m_fadeSeconds;
        const float direction = m_layer == 0 ? -1.0f : 1.0f;
        m_layerBlend = std::clamp(m_layerBlend + direction * step, 0.0f, 1.0f);
    }
}

// ---------------------------------------------------------------------------------------------------------------------
// Draw
// ---------------------------------------------------------------------------------------------------------------------

void Models::Draw(ID3D11DeviceContext* context, const XMMATRIX& view, const XMMATRIX& projection, const XMFLOAT3& eye)
{
    DL_ASSERT(context != nullptr, "Models::Draw needs a context");
    if (context == nullptr || !m_vertexShader || !m_pixelShader)
    {
        return; // Init did not work: the log has said so; the program goes on without the models
    }
    const StateGuard guard(context);

    // ---- the frame: the camera and the light. View * Projection is worked out once (docs/CONVENTIONS.md: row vectors,
    // no transpose). The light and the sky are the mix of the two layer themes.
    const Theme theme = BlendedTheme();
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

    // ---- the state of the pass
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

    // ---- one draw call per object: only the objects of the visible layer
    const unsigned visibleBit = 1u << VisibleLayer();
    for (const SceneObject& object : kScene)
    {
        if ((object.layerMask & visibleBit) == 0)
        {
            continue; // not on the layer we are looking at
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

int Models::TriangleTotal() const
{
    const unsigned visibleBit = 1u << VisibleLayer();
    int total = 0;
    for (const SceneObject& object : kScene)
    {
        if ((object.layerMask & visibleBit) != 0)
        {
            total += static_cast<int>(m_meshes[static_cast<std::size_t>(object.shape)].indexCount / 3);
        }
    }
    return total;
}

int Models::VisibleModelCount() const
{
    const unsigned visibleBit = 1u << VisibleLayer();
    int count = 0;
    for (const SceneObject& object : kScene)
    {
        if ((object.layerMask & visibleBit) != 0 && object.checker < 0.5f) // (the floor is not a "model")
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
               m_layer == 0 ? L"A" : L"B");
    return text;
}

} // namespace dl
