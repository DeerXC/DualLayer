// Models.h - the scene of M1 and M2: a cube, a sphere, a cylinder and a torus standing on a checkered floor, lit by one
//            light, split into two layers (A and B) that look like two different worlds.
// Owns:      the shaders, the input layout, the two constant buffers, the render states, the vertex / index buffers of
//            the four shapes, and the look (theme) of each layer. The shapes come from MeshData (CPU, tested without a
//            window); this class only puts them on the GPU.
// Shader:    compiled at run time from shaders/models.hlsl (same rule as Triangle: working directory = the project folder).
// Depth:     Draw needs the depth buffer to be bound (Device::BeginFrame does it) and tests against it.
// State:     Draw puts back every state it changes (rasterizer, depth, blend, input assembler, shaders, constant buffers),
//            so the code that draws before or after it does not notice that it ran.
// Layers:    every object carries a mask (A = 1, B = 2, both = 3). Only the objects of the visible layer are drawn.
//            ToggleLayer starts a short fade (fade_seconds in the config): the light and the background change colour, and
//            the objects swap when the fade is half way.
// Keys:      main.cpp reads the keyboard (focus rules live there) and calls ToggleWireframe / CycleDetail / ToggleSpin /
//            ToggleLayer.
// Threading: main thread only.
#pragma once

#include "core/Config.h"
#include "render/MeshData.h"

#include <DirectXMath.h>
#include <d3d11_1.h>
#include <wrl/client.h>

#include <array>
#include <string>

namespace dl
{

class Models
{
  public:
    enum class Shape
    {
        Cube,
        Sphere,
        Cylinder,
        Torus,
        Count
    };

    /// The look of one layer. All colours are LINEAR (docs/CONVENTIONS.md).
    struct Theme
    {
        DirectX::XMFLOAT3 toLight = DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f);   ///< from a surface TO the light (made unit length when drawn)
        DirectX::XMFLOAT3 lightColor = DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f); ///< colour of the light
        DirectX::XMFLOAT3 sky = DirectX::XMFLOAT3(0.1f, 0.1f, 0.1f);        ///< colour of the sky light on a surface that looks up
        DirectX::XMFLOAT3 clear = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);      ///< the background
    };

    /// segments: how round the sphere, the cylinder and the torus are (clamped to 6 .. 128). Returns false (and logs
    /// why) if the shader does not compile or a resource cannot be created; nothing is left half-made.
    bool Init(ID3D11Device* device, int segments);
    void Shutdown();

    /// Reads [layer_a], [layer_b] and [layers] from the config. A missing key keeps the built-in value, so the program
    /// works without any of them. Call it after Init and again when F5 reloads the config.
    void LoadThemes(const Config& config);

    void ToggleWireframe();
    /// The next level of detail: 8, 16, 32, 64 segments, then round again to 8.
    void CycleDetail();
    /// Stops / starts the turning of the shapes.
    void ToggleSpin();
    /// Switches to the other layer (A <-> B). The change is a short fade, not a cut.
    void ToggleLayer();

    /// deltaSeconds: the time of the last frame (limited to 0.1 s: a stall must not make the shapes jump).
    void Update(float deltaSeconds);

    /// The background colour of this moment (the two layer backgrounds, mixed during a fade). Linear RGBA, alpha = 1.
    void ClearColor(float out[4]) const;

    /// `view` and `projection` are the matrices of the camera (docs/CONVENTIONS.md: row vectors, left-handed); `eye` is
    /// the camera position in the world.
    void Draw(ID3D11DeviceContext* context, const DirectX::XMMATRIX& view, const DirectX::XMMATRIX& projection,
              const DirectX::XMFLOAT3& eye);

    /// For the title bar: "2 models | 32 segments | 1,234 triangles | solid | layer A".
    std::wstring StatusLine() const;

  private:
    static constexpr int kShapeCount = static_cast<int>(Shape::Count);

    struct GpuMesh
    {
        Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer;
        Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer;
        UINT indexCount = 0;
    };

    bool CreateMeshes(int segments);
    bool CreateGpuMesh(Shape shape, const MeshData& data);
    bool CreateStates();
    int TriangleTotal() const;
    int VisibleModelCount() const;
    unsigned VisibleLayer() const; ///< 0 = A, 1 = B: the layer whose objects are drawn right now
    Theme BlendedTheme() const;    ///< the two themes mixed by the fade

    Microsoft::WRL::ComPtr<ID3D11Device> m_device;
    Microsoft::WRL::ComPtr<ID3D11VertexShader> m_vertexShader;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> m_pixelShader;
    Microsoft::WRL::ComPtr<ID3D11InputLayout> m_inputLayout;
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_frameBuffer;
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_objectBuffer;
    Microsoft::WRL::ComPtr<ID3D11RasterizerState> m_solidState;
    Microsoft::WRL::ComPtr<ID3D11RasterizerState> m_wireState;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> m_depthState;
    Microsoft::WRL::ComPtr<ID3D11BlendState> m_blendState;
    std::array<GpuMesh, kShapeCount> m_meshes;
    std::array<Theme, 2> m_themes; // [0] = layer A, [1] = layer B (set by Init, changed by LoadThemes)
    int m_segments = 32;
    bool m_wireframe = false;
    bool m_spinning = true;
    float m_spin = 0.0f;       // seconds of turning (it does not run while the shapes are stopped)
    unsigned m_layer = 0;      // the layer we are going to (0 = A, 1 = B)
    float m_layerBlend = 0.0f; // 0 = fully A, 1 = fully B; it moves toward m_layer
    float m_fadeSeconds = 0.35f; // UNITS: seconds for a full A <-> B change; 0 = a cut
};

} // namespace dl
