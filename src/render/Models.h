// Models.h - the scene of M1 and M2: a cube, a sphere, a cylinder and a torus standing on a checkered floor, lit by one
//            light, split into two layers (A and B) that look like two different worlds.
// Owns:      the shaders, the input layout, the constant buffers, the render states, the vertex / index buffers of the
// four
//            shapes, the look (theme) of each layer, and the ripple that switches between the layers. The shapes come
//            from MeshData (CPU, tested without a window); this class only puts them on the GPU.
// Shader:    compiled at run time from shaders/models.hlsl and shaders/ripple.hlsl (working directory = the project
// folder). Depth:     Draw needs the depth buffer to be bound (Device::BeginFrame does it) and tests against it. State:
// Draw puts back every state it changes (render targets, viewport, rasterizer, depth, blend, input assembler,
//            shaders, constant buffers, shader resources, sampler), so the code that draws before or after it does not
//            notice that it ran.
// Layers:    every object carries a mask (A = 1, B = 2, both = 3). When one layer is on the screen, only its objects
// are
//            drawn, straight to the back buffer. ToggleLayer starts a ripple (LayerRipple.h): for about a second BOTH
//            layers are drawn into two off-screen pictures, and a circle that opens from the centre of the screen shows
//            the new layer inside and the old one outside.
// Targets:   the two off-screen pictures are made the first time a ripple needs them and again when the window size
//            changes; they live until Shutdown.
// Objects:   a table of scene::SceneObject (scene/SceneData.h), given every frame by SetObjects and drawn one by one.
// Static:    the fixed town is baked by its owner (scene/Town.h) into one vertex buffer per layer
// (render/StaticBatch.h),
//            given once by SetStaticTown / SetStaticWater and drawn with one call per layer. The baked lists are kept
//            on the CPU, so a new device after a lost one makes the buffers again by itself.
// Keys:      main.cpp reads the keyboard (focus rules live there) and calls ToggleWireframe / CycleDetail /
// ToggleLayer. Threading: main thread only.
#pragma once

#include "core/Config.h"
#include "render/LayerRipple.h"
#include "render/MeshData.h"
#include "render/StaticBatch.h"
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
    /// The kinds of shape of the objects (scene/SceneData.h).
    using Shape = scene::ShapeKind;

    /// The look of one layer. All colours are LINEAR (docs/CONVENTIONS.md).
    struct Theme
    {
        DirectX::XMFLOAT3 toLight =
            DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f); ///< from a surface TO the light (made unit length when drawn)
        DirectX::XMFLOAT3 lightColor = DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f); ///< colour of the light
        DirectX::XMFLOAT3 sky =
            DirectX::XMFLOAT3(0.1f, 0.1f, 0.1f); ///< colour of the sky light on a surface that looks up
        DirectX::XMFLOAT3 clear = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f); ///< the background, and the colour of the fog
        float fogDensity = 0.0f; ///< UNITS: 1 / metres: the part of the light lost per metre
        DirectX::XMFLOAT3 tint =
            DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f); ///< the stone of the city in this layer: multiplies its albedo
        DirectX::XMFLOAT3 zenith{};              ///< linear sky colour overhead
        DirectX::XMFLOAT3 groundFill{};          ///< linear bounce from the ground
        float mistBase = 0.0f;                   ///< UNITS: metres, world height at which low mist starts to thin
        float mistDensity = 0.0f;                ///< UNITS: 1 / metres, additional low mist
        bool gradientSky = false;                ///< town only; the legacy test map retains its solid backdrop
        float weathering =
            0.0f; ///< 0 .. 1: how uneven the stone blocks are and how much moss grows on surfaces that look up
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

    /// The objects that are drawn (BORROWS: the table must stay alive until the next call). The default is the scene
    /// table of the program; a null table or a count of 0 goes back to it. The stage gives a new table every frame (its
    /// block moves, its door opens and closes, a blocker flashes).
    void SetObjects(const scene::SceneObject* objects, int count)
    {
        if (objects != nullptr && count >= 0)
        {
            m_objects = objects;
            m_objectCount = count;
        }
        else
        {
            m_objects = nullptr;
            m_objectCount = 0;
        }
    }

    /// Fixed geometry that is not made of boxes (the polygons of the town: scene/Town.h), baked by its owner, one mesh
    /// per layer (copied): joined to the static batch of that layer. Call it once (it survives a lost device).
    void SetStaticTown(const BatchMesh& layerA, const BatchMesh& layerB);

    /// Water that does not move, baked by its owner, one mesh per layer (copied; scene/Town.h BakeWater): drawn in the
    /// water pass, blended over what is behind it. Call it once (it survives a lost device).
    void SetStaticWater(const BatchMesh& layerA, const BatchMesh& layerB);

    /// The looks of both layers at once (the town of the model brings its own: scene/Town.h). LoadThemes sets them from
    /// the config again.
    void SetThemes(const Theme& layerA, const Theme& layerB)
    {
        m_themes[0] = layerA;
        m_themes[1] = layerB;
    }

    /// The circle of the lantern (scene/Lantern.h): inside it, B is drawn as A. radius <= 0: none.
    void SetLantern(const DirectX::XMFLOAT3& centre, float radius)
    {
        m_lantern = DirectX::XMFLOAT4(centre.x, centre.y, centre.z, radius > 0.0f ? radius : 0.0f);
    }

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
    bool UploadBatch(const BatchMesh& baked, GpuMesh& target, const char* vertexName, const char* indexName);
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
    Microsoft::WRL::ComPtr<ID3D11BlendState> m_waterBlend;        // water: blended over what is behind it
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> m_waterDepth; // water: tested against the depth, not written
    std::vector<const scene::SceneObject*> m_water;               // the water of a frame, far to near (reused)
    std::array<GpuMesh, kShapeCount> m_meshes;
    // ---- the static batch: one mesh per layer
    std::array<GpuMesh, 2> m_batch;
    Microsoft::WRL::ComPtr<ID3D11VertexShader> m_batchVertexShader;
    Microsoft::WRL::ComPtr<ID3D11InputLayout> m_batchLayout;
    int m_drawCalls = 0;
    std::array<Theme, 2> m_themes; // [0] = layer A, [1] = layer B (set by Init, changed by LoadThemes)
    DirectX::XMFLOAT4 m_lantern = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f); // the circle of the lantern; w 0: none
    std::array<BatchMesh, 2> m_townMesh;  // the polygons of the town of each layer (SetStaticTown)
    std::array<BatchMesh, 2> m_waterMesh; // the water that does not move (SetStaticWater)
    std::array<GpuMesh, 2> m_waterBatch;
    /// The theme a layer is drawn with now: B as it is, A blended toward its night.
    Theme ThemeOf(int layer) const;
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
    const scene::SceneObject* m_objects = nullptr; // BORROWED: see SetObjects
    int m_objectCount = 0;
    bool m_wireframe = false;
    float m_time = 0.0f; // seconds since the start (always runs): the waves of the water

    // ---- the ripple
    LayerRipple m_ripple;
    Microsoft::WRL::ComPtr<ID3D11VertexShader> m_rippleVertexShader;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> m_ripplePixelShader;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> m_skyPixelShader;
    bool m_townAtmosphere = true;
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_rippleBuffer;
    Microsoft::WRL::ComPtr<ID3D11SamplerState> m_sampler;
    Microsoft::WRL::ComPtr<ID3D11RasterizerState> m_noCullState;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> m_noDepthState;
    std::array<LayerTarget, 2> m_targets; // [0] = the picture outside the circle, [1] = the picture inside it
    UINT m_targetWidth = 0;
    UINT m_targetHeight = 0;
    float m_rippleWidth = 0.07f;       // UNITS: screen heights, the width of the bright ring
    float m_rippleDistortion = 0.012f; // UNITS: screen (uv), how far the ring bends the picture
    DirectX::XMFLOAT2 m_rippleCenter = DirectX::XMFLOAT2(0.5f, 0.5f); // 0 .. 1 over the screen
};

} // namespace dl
