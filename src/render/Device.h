// Device.h - DXGI adapter choice, D3D11 device + immediate context, flip-model swap chain, back-buffer view.
// Owns:      factory, device, context, swap chain, back-buffer RTV, debug interfaces.
// Colour:    the swap chain is R8G8B8A8_UNORM; we render through an R8G8B8A8_UNORM_SRGB view, so shaders output LINEAR colour and the
//            hardware writes sRGB (see docs/CONVENTIONS.md).
// Adapters:  listed in "high performance first" order (IDXGIFactory6). On a laptop with two GPUs the first hardware adapter is the
//            discrete one. Choose another with --adapter N or [render] adapter = N.
// Depth:     (M1) a D32_FLOAT depth buffer the size of the back buffer, cleared to 1.0 (far) every frame. The nearer surface wins the depth test
//            (comparison LESS). It is made again with the back buffer on every resize. Reverse-Z was considered and not adopted
//            (docs/CONVENTIONS.md, "Depth"); the clear value and the comparison are kept in ONE place (BeginFrame, Models::CreateStates).
// Threading: main thread only (it is also the render thread).
#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <d3d11_1.h>
#include <dxgi1_6.h>
#include <wrl/client.h>

#include <string>

namespace dl
{
class Device
{
public:
    Device() = default;
    ~Device();
    Device(const Device&) = delete;
    Device& operator=(const Device&) = delete;

    /// requestedAdapter: -1 = the first hardware adapter, otherwise the index printed in the log.
    bool Init(HWND hwnd, int width, int height, int requestedAdapter, bool vsync);
    void Shutdown();

    /// Call when the client size changed. Ignores 0 x 0 (minimised).
    bool Resize(int width, int height);

    /// Binds the back buffer AND the depth buffer, sets the viewport and clears both. `linearClear` is linear RGBA. Depth is cleared to 1.0 (far).
    void BeginFrame(const float linearClear[4]);
    /// Presents. Detects a removed device: logs the reason and sets Lost(); main.cpp then builds new GPU objects (Gpu::Recover).
    void EndFrame();

    void SetVsync(bool vsync) { m_vsync = vsync; }
    bool Lost() const { return m_lost; }

    ID3D11Device* GetDevice() const { return m_device.Get(); }
    ID3D11DeviceContext* GetContext() const { return m_context.Get(); }
    ID3DUserDefinedAnnotation* GetAnnotation() const { return m_annotation.Get(); }
    const std::wstring& AdapterName() const { return m_adapterName; }
    int Width() const { return m_width; }
    int Height() const { return m_height; }

private:
    bool CreateBackBufferView();
    bool CreateDepthBuffer(int width, int height);

    Microsoft::WRL::ComPtr<IDXGIFactory2> m_factory;
    Microsoft::WRL::ComPtr<ID3D11Device> m_device;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_context;
    Microsoft::WRL::ComPtr<ID3DUserDefinedAnnotation> m_annotation;
    Microsoft::WRL::ComPtr<IDXGISwapChain1> m_swapChain;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_backBufferRtv;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> m_depthTexture;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilView> m_depthView;
    Microsoft::WRL::ComPtr<ID3D11Debug> m_debug; // only when the debug layer is on

    std::wstring m_adapterName;
    int m_width = 0;
    int m_height = 0;
    bool m_vsync = true;
    bool m_lost = false;
};
} // namespace dl
