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
// Scene:     the table of objects is scene/SceneData.h (shared with the collision, scene/Collision.h).
// Static:    SetStatic gives the boxes that never move (the fixed city); they are baked into one vertex buffer per layer
//            (render/StaticBatch.h) and drawn with one call per layer. SetObjects gives the rest, drawn one by one. The baked
//            list is kept on the CPU, so a new device after a lost one bakes it again by itself.
// Keys:      main.cpp reads the keyboard (focus rules live there) and calls ToggleWireframe / CycleDetail / ToggleSpin /
//            ToggleLayer.
// Threading: main thread only.
#pragma once

#include "core/Config.h"
#include "render/LayerRipple.h"
#include "render/MeshData.h"
#include "scene/SceneData.h"

#include <DirectXMath.h>
#include <d3d11_1.h>
#include <wrl/client.h>

#include <array>
#include <string>
#include <vector>

namespace dl
{

class Models
{
  public:
    /// The kinds of shape of the scene (scene/SceneData.h: the table that the collision reads too).
    using Shape = scene::ShapeKind;

    /// The look of one layer. All colours are LINEAR (docs/CONVENTIONS.md).
    struct Theme
    {
        DirectX::XMFLOAT3 toLight = DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f);   ///< from a surface TO the light (made unit length when drawn)
        DirectX::XMFLOAT3 lightColor = DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f); ///< colour of the light
        DirectX::XMFLOAT3 sky = DirectX::XMFLOAT3(0.1f, 0.1f, 0.1f);        ///< colour of the sky light on a surface that looks up
        DirectX::XMFLOAT3 clear = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);      ///< the background, and the colour of the fog
        float fogDensity = 0.0f;                                            ///< UNITS: 1 / metres: the part of the light lost per metre
        DirectX::XMFLOAT3 tint = DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f);       ///< the stone of the city in this layer: multiplies its albedo
        float weathering = 0.0f; ///< 0 .. 1: how uneven the stone blocks are and how much moss grows on surfaces that look up
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

    /// The objects that are drawn (BORROWS: the table must stay alive until the next call). The default is the scene table
    /// of the program; a null table or a count of 0 goes back to it. The stage gives a new table every frame (its block
    /// moves, its door opens and closes, a blocker flashes).
    void SetObjects(const scene::SceneObject* objects, int count)
    {
        if (objects != nullptr && count >= 0) // (0: everything is in the static batch)
        {
            m_objects = objects;
            m_objectCount = count;
        }
        else
        {
            m_objects = scene::kScene;
            m_objectCount = scene::kSceneCount;
        }
    }

    /// The fixed boxes of the map (copied): every cube of them that does not turn is baked and drawn in one call per layer; the
    /// rest of them should be given to SetObjects. Call it once after Init (it survives a lost device).
    void SetStatic(const scene::SceneObject* objects, int count);

    /// How many draw calls the last Draw made (for the title bar and the log).
    int DrawCalls() const
    {
        return m_drawCalls;
    }

    /// True while a ripple runs (the switch guard does not refuse on top of a ripple).
    bool LayerSwitching() const
    {
        return m_ripple.Active();
    }

    /// The time that the turning shapes use (seconds; it stops while the shapes are stopped). The collision needs the
    /// same time, or what is solid and what is drawn drift apart.
    float SpinSeconds() const
    {
        return m_spin;
    }

    /// The layer whose objects are solid: the one the screen is going to show (0 = A, 1 = B). During a ripple that is
    /// the NEW layer, so the world that is opening up is solid at once.
    int CollisionLayer() const
    {
        return m_ripple.Target();
    }

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
    bool CreateBatchBuffers();
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
    // ---- the static batch: one mesh per layer
    std::vector<scene::SceneObject> m_staticObjects; // the CPU copy (kept through Shutdown: a new device bakes it again)
    std::array<GpuMesh, 2> m_batch;
    Microsoft::WRL::ComPtr<ID3D11VertexShader> m_batchVertexShader;
    Microsoft::WRL::ComPtr<ID3D11InputLayout> m_batchLayout;
    int m_drawCalls = 0;
    std::array<Theme, 2> m_themes; // [0] = layer A, [1] = layer B (set by Init, changed by LoadThemes)
    int m_segments = 32;
    /// The drawn table as a range, so that a loop reads "for (const SceneObject& object : Objects())".
    struct ObjectRange
    {
        const scene::SceneObject* first;
        const scene::SceneObject* last;
        const scene::SceneObject* begin() const
        {
            return first;
        }
        const scene::SceneObject* end() const
        {
            return last;
        }
    };
    ObjectRange Objects() const
    {
        return {m_objects, m_objects + m_objectCount};
    }
    const scene::SceneObject* m_objects = scene::kScene; // BORROWED: see SetObjects
    int m_objectCount = scene::kSceneCount;
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
