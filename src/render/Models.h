// Models.h - the scene of M1 and M2: a cube, a sphere, a cylinder and a torus standing on a checkered floor, lit by one
//            light, split into two layers (A and B) that look like two different worlds.
// Owns:      the shaders, the input layout, the constant buffers, the render states, the vertex / index buffers of the four
//            shapes, the look (theme) of each layer, and the ripple that switches between the layers. The shapes come from
//            MeshData (CPU, tested without a window); this class only puts them on the GPU.
// Shader:    compiled at run time from shaders/models.hlsl and shaders/ripple.hlsl (working directory = the project folder).
// Depth:     Draw needs the depth buffer to be bound (Device::BeginFrame does it) and tests against it.
// State:     Draw puts back every state it changes (render targets, viewport, rasterizer, depth, blend, input assembler,
//            shaders, constant buffers, shader resources, sampler), so the code that draws before or after it does not
//            notice that it ran.
// Layers:    every object carries a mask (A = 1, B = 2, both = 3). When one layer is on the screen, only its objects are
//            drawn, straight to the back buffer. ToggleLayer starts a ripple (LayerRipple.h): for about a second BOTH
//            layers are drawn into two off-screen pictures, and a circle that opens from the centre of the screen shows the
//            new layer inside and the old one outside.
// Targets:   the two off-screen pictures are made the first time a ripple needs them and again when the window size
//            changes; they live until Shutdown.
// Keys:      main.cpp reads the keyboard (focus rules live there) and calls ToggleWireframe / CycleDetail / ToggleSpin /
//            ToggleLayer.
// Threading: main thread only.
#pragma once

#include "core/Config.h"
#include "render/LayerRipple.h"
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
    /// why) if a shader does not compile or a resource cannot be created; nothing is left half-made.
    /// The layer that is shown and a running ripple are NOT reset: after a lost device the scene comes back as it was.
    bool Init(ID3D11Device* device, int segments);
    void Shutdown();

    /// Reads [layer_a], [layer_b] and [ripple] from the config. A missing key keeps the built-in value, so the program
    /// works without any of them. Call it after Init and again when F5 reloads the config.
    void LoadThemes(const Config& config);

    void ToggleWireframe();
    /// The next level of detail: 8, 16, 32, 64 segments, then round again to 8.
    void CycleDetail();
    /// Stops / starts the turning of the shapes.
    void ToggleSpin();
    /// Goes to the other layer (A <-> B) with a ripple. Pressed again while the ripple runs, the ripple turns round.
    void ToggleLayer();

    /// deltaSeconds: the time of the last frame (limited to 0.1 s: a stall must not make the shapes jump).
    void Update(float deltaSeconds);

    /// The background colour for Device::BeginFrame: the layer on the screen (during a ripple the circle covers it).
    /// Linear RGBA, alpha = 1.
    void ClearColor(float out[4]) const;

    /// `view` and `projection` are the matrices of the camera (docs/CONVENTIONS.md: row vectors, left-handed); `eye` is
    /// the camera position in the world.
    void Draw(ID3D11DeviceContext* context, const DirectX::XMMATRIX& view, const DirectX::XMMATRIX& projection,
              const DirectX::XMFLOAT3& eye);

    /// For the title bar: "2 models | 32 segments | 1,234 triangles | solid | layer A". The layer is the one the screen
    /// is going to show.
    std::wstring StatusLine() const;

  private:
    static constexpr int kShapeCount = static_cast<int>(Shape::Count);

    struct GpuMesh
    {
        Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer;
        Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer;
        UINT indexCount = 0;
    };

    /// One off-screen picture of a layer: colour (sRGB, so that the lighting stays in linear space) and its own depth.
    struct LayerTarget
    {
        Microsoft::WRL::ComPtr<ID3D11Texture2D> color;
        Microsoft::WRL::ComPtr<ID3D11RenderTargetView> renderView;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> shaderView;
        Microsoft::WRL::ComPtr<ID3D11Texture2D> depth;
        Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depthView;
    };

    bool CreateMeshes(int segments);
    bool CreateGpuMesh(Shape shape, const MeshData& data);
    bool CreateStates();
    bool CreateRippleResources();
    bool EnsureLayerTargets(UINT width, UINT height);
    int TriangleTotal() const;
    int VisibleModelCount() const;

    /// Sets everything the objects of the scene need (not the render target).
    void BindScenePipeline(ID3D11DeviceContext* context);
    /// Writes the frame constants of `layer` and draws the objects that belong to it.
    void DrawObjects(ID3D11DeviceContext* context, const DirectX::XMMATRIX& view, const DirectX::XMMATRIX& projection,
                     const DirectX::XMFLOAT3& eye, int layer);
    /// Both layers into the two off-screen pictures, then the circle onto the back buffer. False (and nothing changed)
    /// if the pictures cannot be made: Draw then shows the layer without a ripple.
    bool DrawRipple(ID3D11DeviceContext* context, const DirectX::XMMATRIX& view, const DirectX::XMMATRIX& projection,
                    const DirectX::XMFLOAT3& eye);

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
    float m_spin = 0.0f; // seconds of turning (it does not run while the shapes are stopped)

    // ---- the ripple
    LayerRipple m_ripple;
    Microsoft::WRL::ComPtr<ID3D11VertexShader> m_rippleVertexShader;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> m_ripplePixelShader;
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_rippleBuffer;
    Microsoft::WRL::ComPtr<ID3D11SamplerState> m_sampler;
    Microsoft::WRL::ComPtr<ID3D11RasterizerState> m_noCullState;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> m_noDepthState;
    std::array<LayerTarget, 2> m_targets; // [0] = the picture outside the circle, [1] = the picture inside it
    UINT m_targetWidth = 0;
    UINT m_targetHeight = 0;
    float m_rippleWidth = 0.07f;      // UNITS: screen heights, the width of the bright ring
    float m_rippleDistortion = 0.012f; // UNITS: screen (uv), how far the ring bends the picture
    DirectX::XMFLOAT2 m_rippleCenter = DirectX::XMFLOAT2(0.5f, 0.5f); // 0 .. 1 over the screen
};

} // namespace dl
