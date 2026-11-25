// Triangle.h - the M0 test triangle: proves the device, the shader path, the constant buffer, the matrix convention and
// the GPU markers. Owns:      vertex buffer, constant buffer, input layout, vertex and pixel shader. Shader: compiled
// at run time from shaders/test_triangle.hlsl (working directory = the project folder; the debugger setting in
//            DualLayer.vcxproj takes care of that). A compile error is logged with the compiler's message.
#pragma once

#include <DirectXMath.h>
#include <d3d11_1.h>
#include <wrl/client.h>

namespace dl
{
class Triangle
{
  public:
    /// Returns false (and logs why) if the shader does not compile or a resource cannot be created.
    bool Init(ID3D11Device* device);
    void Shutdown();

    /// angleRadians: rotation around +Y (0 = still). `view` comes from the camera (world -> camera space).
    void Draw(ID3D11DeviceContext* context, float aspect, float angleRadians, const DirectX::XMMATRIX& view);

  private:
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_vertexBuffer;
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_constantBuffer;
    Microsoft::WRL::ComPtr<ID3D11InputLayout> m_inputLayout;
    Microsoft::WRL::ComPtr<ID3D11VertexShader> m_vertexShader;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> m_pixelShader;
};
} // namespace dl
