// Models.h - the M1 scene: a cube, a sphere, a cylinder and a torus standing on a checkered floor, lit by one light.
// Owns:      the shaders, the input layout, the two constant buffers, the render states and the vertex / index buffers
// of the
//            four shapes. The shapes come from MeshData (CPU, tested without a window); this class only puts them on
//            the GPU.
// Shader:    compiled at run time from shaders/models.hlsl (same rule as Triangle: working directory = the project
// folder). Depth:     Draw needs the depth buffer to be bound (Device::BeginFrame does it) and tests against it. State:
// Draw puts back every state it changes (rasterizer, depth, blend, input assembler, shaders, constant buffers), so
//            the code that draws before or after it does not notice that it ran.
// Keys:      main.cpp reads the keyboard (focus rules live there) and calls ToggleWireframe / CycleDetail / ToggleSpin.
// Threading: main thread only.
#pragma once

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

    /// segments: how round the sphere, the cylinder and the torus are (clamped to 6 .. 128). Returns false (and logs
    /// why) if the shader does not compile or a resource cannot be created; nothing is left half-made.
    bool Init(ID3D11Device* device, int segments);
    void Shutdown();

    void ToggleWireframe();
    /// The next level of detail: 8, 16, 32, 64 segments, then round again to 8.
    void CycleDetail();
    /// Stops / starts the turning of the shapes.
    void ToggleSpin();

    /// deltaSeconds: the time of the last frame (limited to 0.1 s: a stall must not make the shapes jump).
    void Update(float deltaSeconds);
    /// `view` and `projection` are the matrices of the camera (docs/CONVENTIONS.md: row vectors, left-handed); `eye` is
    /// the camera position in the world.
    void Draw(ID3D11DeviceContext* context, const DirectX::XMMATRIX& view, const DirectX::XMMATRIX& projection,
              const DirectX::XMFLOAT3& eye);

    /// For the title bar: "4 models | 32 segments | 5,312 triangles | solid".
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

    int m_segments = 32;
    bool m_wireframe = false;
    bool m_spinning = true;
    float m_spin = 0.0f; // seconds of turning (it does not run while the shapes are stopped)
};
} // namespace dl
