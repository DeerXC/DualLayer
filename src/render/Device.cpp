#include "render/Device.h"

#include "core/Log.h"
#include "render/GpuDebug.h"

#include <vector>

using Microsoft::WRL::ComPtr;

namespace dl
{
namespace
{
std::string ToUtf8(const wchar_t* wide)
{
    const int size = WideCharToMultiByte(CP_UTF8, 0, wide, -1, nullptr, 0, nullptr, nullptr);
    if (size <= 1)
    {
        return std::string();
    }
    std::string out(static_cast<std::size_t>(size - 1), '\0');
    WideCharToMultiByte(CP_UTF8, 0, wide, -1, out.data(), size, nullptr, nullptr);
    return out;
}

/// All adapters, best performance first when the OS supports IDXGIFactory6.
std::vector<ComPtr<IDXGIAdapter1>> EnumerateAdapters(IDXGIFactory2* factory)
{
    std::vector<ComPtr<IDXGIAdapter1>> adapters;
    ComPtr<IDXGIFactory6> factory6;
    if (SUCCEEDED(factory->QueryInterface(IID_PPV_ARGS(&factory6))))
    {
        for (UINT i = 0;; ++i)
        {
            ComPtr<IDXGIAdapter1> adapter;
            if (factory6->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&adapter)) ==
                DXGI_ERROR_NOT_FOUND)
            {
                break;
            }
            adapters.push_back(adapter);
        }
    }
    else
    {
        DL_LOG_WARN("Device", "IDXGIFactory6 is not available: adapters are listed in the driver's order");
        for (UINT i = 0;; ++i)
        {
            ComPtr<IDXGIAdapter1> adapter;
            if (factory->EnumAdapters1(i, &adapter) == DXGI_ERROR_NOT_FOUND)
            {
                break;
            }
            adapters.push_back(adapter);
        }
    }
    return adapters;
}
} // namespace

Device::~Device()
{
    Shutdown();
}

bool Device::Init(HWND hwnd, int width, int height, int requestedAdapter, bool vsync)
{
    m_vsync = vsync;
    m_width = width;
    m_height = height;

    // ---- factory (with the DXGI debug layer in Debug builds; it needs the "Graphics Tools" optional feature)
    UINT factoryFlags = 0;
#if defined(_DEBUG)
    factoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif
    HRESULT hr = CreateDXGIFactory2(factoryFlags, IID_PPV_ARGS(&m_factory));
    if (FAILED(hr) && factoryFlags != 0)
    {
        DL_LOG_WARN("Device", "DXGI debug factory failed (0x%08X): continuing without it", static_cast<unsigned>(hr));
        hr = CreateDXGIFactory2(0, IID_PPV_ARGS(&m_factory));
    }
    if (FAILED(hr))
    {
        DL_LOG_ERROR("Device", "CreateDXGIFactory2 failed (0x%08X)", static_cast<unsigned>(hr));
        return false;
    }

    // ---- adapters: log all of them, then choose
    const auto adapters = EnumerateAdapters(m_factory.Get());
    int firstHardware = -1;
    for (std::size_t i = 0; i < adapters.size(); ++i)
    {
        DXGI_ADAPTER_DESC1 desc{};
        adapters[i]->GetDesc1(&desc);
        const bool software = (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) != 0;
        if (!software && firstHardware < 0)
        {
            firstHardware = static_cast<int>(i);
        }
        DL_LOG_INFO("Device", "adapter [%zu] %s  vram %zu MB  vendor 0x%04X%s", i, ToUtf8(desc.Description).c_str(),
                    static_cast<std::size_t>(desc.DedicatedVideoMemory / (1024 * 1024)), desc.VendorId,
                    software ? "  (software)" : "");
    }
    int chosen = firstHardware;
    if (requestedAdapter >= 0)
    {
        if (static_cast<std::size_t>(requestedAdapter) < adapters.size())
        {
            chosen = requestedAdapter;
        }
        else
        {
            DL_LOG_WARN("Device", "adapter %d does not exist: using the first hardware adapter instead",
                        requestedAdapter);
        }
    }
    if (chosen < 0)
    {
        DL_LOG_ERROR("Device", "no hardware adapter found");
        return false;
    }
    {
        DXGI_ADAPTER_DESC1 desc{};
        adapters[static_cast<std::size_t>(chosen)]->GetDesc1(&desc);
        m_adapterName = desc.Description;
        DL_LOG_INFO("Device", "using adapter [%d] %s", chosen, ToUtf8(desc.Description).c_str());
    }

    // ---- device (feature level 11_0 is the minimum of the project)
    const D3D_FEATURE_LEVEL levels[] = {D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0};
    UINT flags = 0;
#if defined(_DEBUG)
    flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
    D3D_FEATURE_LEVEL obtained{};
    IDXGIAdapter1* adapter = adapters[static_cast<std::size_t>(chosen)].Get();
    hr = D3D11CreateDevice(adapter, D3D_DRIVER_TYPE_UNKNOWN, nullptr, flags, levels, ARRAYSIZE(levels),
                           D3D11_SDK_VERSION, &m_device, &obtained, &m_context);
    if (FAILED(hr) && (flags & D3D11_CREATE_DEVICE_DEBUG) != 0)
    {
        DL_LOG_WARN("Device",
                    "D3D11 debug layer is not available (0x%08X). Install: Settings > System > Optional features > "
                    "Graphics Tools",
                    static_cast<unsigned>(hr));
        flags &= ~static_cast<UINT>(D3D11_CREATE_DEVICE_DEBUG);
        hr = D3D11CreateDevice(adapter, D3D_DRIVER_TYPE_UNKNOWN, nullptr, flags, levels, ARRAYSIZE(levels),
                               D3D11_SDK_VERSION, &m_device, &obtained, &m_context);
    }
    if (FAILED(hr))
    {
        DL_LOG_ERROR("Device", "D3D11CreateDevice failed (0x%08X)", static_cast<unsigned>(hr));
        return false;
    }
    DL_LOG_INFO("Device", "feature level %s, debug layer %s", obtained == D3D_FEATURE_LEVEL_11_1 ? "11_1" : "11_0",
                (flags & D3D11_CREATE_DEVICE_DEBUG) != 0 ? "ON" : "off");
    SetDebugName(m_device.Get(), "Device");

    if ((flags & D3D11_CREATE_DEVICE_DEBUG) != 0)
    {
        m_device.As(&m_debug);
        ComPtr<ID3D11InfoQueue> queue;
        // WHY only with a debugger attached: a break-on-error without a debugger would kill the program.
        if (IsDebuggerPresent() != 0 && SUCCEEDED(m_device.As(&queue)))
        {
            queue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, TRUE);
            queue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, TRUE);
        }
    }
    m_context.As(&m_annotation); // may stay null on very old runtimes: GpuEvent handles that

    // ---- swap chain (flip model: the modern presentation path)
    DXGI_SWAP_CHAIN_DESC1 sd{};
    sd.Width = static_cast<UINT>(width);
    sd.Height = static_cast<UINT>(height);
    sd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.SampleDesc.Count = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.BufferCount = 2;
    sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    hr = m_factory->CreateSwapChainForHwnd(m_device.Get(), hwnd, &sd, nullptr, nullptr, &m_swapChain);
    if (FAILED(hr))
    {
        DL_LOG_ERROR("Device", "CreateSwapChainForHwnd failed (0x%08X)", static_cast<unsigned>(hr));
        return false;
    }
    m_factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER); // fullscreen switching comes later, deliberately
    return CreateBackBufferView() && CreateDepthBuffer(width, height);
}

bool Device::CreateDepthBuffer(int width, int height)
{
    // WHY: without a depth buffer the triangles are drawn in the order of the draw calls: a sphere drawn after the
    // floor is ALWAYS on top of it, even where it is behind. With one, every pixel remembers how far away the nearest
    // thing drawn so far is, and only a nearer thing replaces it. D32_FLOAT: 32 bits of float depth (Reverse-Z, which
    // would use the extra float precision close to 0, was not adopted: docs/CONVENTIONS.md, "Depth").
    D3D11_TEXTURE2D_DESC desc{};
    desc.Width = static_cast<UINT>(width);
    desc.Height = static_cast<UINT>(height);
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    // WHY typeless and readable: the ink lines (render/Models.cpp, DrawLines) read this depth in a pixel shader;
    // the depth view below still sees it as D32_FLOAT, so nothing else changes.
    desc.Format = DXGI_FORMAT_R32_TYPELESS;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
    HRESULT hr = m_device->CreateTexture2D(&desc, nullptr, &m_depthTexture);
    if (SUCCEEDED(hr))
    {
        D3D11_DEPTH_STENCIL_VIEW_DESC depthView{};
        depthView.Format = DXGI_FORMAT_D32_FLOAT;
        depthView.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
        hr = m_device->CreateDepthStencilView(m_depthTexture.Get(), &depthView, &m_depthView);
    }
    if (FAILED(hr))
    {
        DL_LOG_ERROR("Device", "the depth buffer (%d x %d) could not be created (0x%08X)", width, height,
                     static_cast<unsigned>(hr));
        m_depthView.Reset();
        m_depthTexture.Reset();
        return false;
    }
    SetDebugName(m_depthTexture.Get(), "Frame.Depth");
    SetDebugName(m_depthView.Get(), "Frame.DepthDSV");
    return true;
}

bool Device::CreateBackBufferView()
{
    ComPtr<ID3D11Texture2D> backBuffer;
    HRESULT hr = m_swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
    if (FAILED(hr))
    {
        DL_LOG_ERROR("Device", "GetBuffer failed (0x%08X)", static_cast<unsigned>(hr));
        return false;
    }
    // WHY an sRGB view on a UNORM buffer: flip-model swap chains must be UNORM, but the view may be sRGB.
    D3D11_RENDER_TARGET_VIEW_DESC rtv{};
    rtv.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    rtv.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
    hr = m_device->CreateRenderTargetView(backBuffer.Get(), &rtv, &m_backBufferRtv);
    if (FAILED(hr))
    {
        DL_LOG_ERROR("Device", "CreateRenderTargetView failed (0x%08X)", static_cast<unsigned>(hr));
        return false;
    }
    SetDebugName(backBuffer.Get(), "Frame.BackBuffer");
    SetDebugName(m_backBufferRtv.Get(), "Frame.BackBufferRTV");
    return true;
}

bool Device::Resize(int width, int height)
{
    if (width <= 0 || height <= 0 || m_swapChain == nullptr)
    {
        return true;
    }
    m_context->OMSetRenderTargets(0, nullptr, nullptr);
    m_backBufferRtv.Reset(); // all views of the back buffer must be gone before ResizeBuffers
    m_depthView.Reset();     // the depth buffer must have the size of the back buffer: it is made again below
    m_depthTexture.Reset();
    const HRESULT hr =
        m_swapChain->ResizeBuffers(0, static_cast<UINT>(width), static_cast<UINT>(height), DXGI_FORMAT_UNKNOWN, 0);
    if (FAILED(hr))
    {
        DL_LOG_ERROR("Device", "ResizeBuffers failed (0x%08X)", static_cast<unsigned>(hr));
        return false;
    }
    m_width = width;
    m_height = height;
    DL_LOG_INFO("Device", "resized to %d x %d", width, height);
    return CreateBackBufferView() && CreateDepthBuffer(width, height);
}

void Device::BeginFrame(const float linearClear[4])
{
    ID3D11RenderTargetView* rtv = m_backBufferRtv.Get();
    m_context->OMSetRenderTargets(1, &rtv, m_depthView.Get());
    D3D11_VIEWPORT vp{0.0f, 0.0f, static_cast<float>(m_width), static_cast<float>(m_height), 0.0f, 1.0f};
    m_context->RSSetViewports(1, &vp);
    m_context->ClearRenderTargetView(rtv, linearClear);
    if (m_depthView != nullptr)
    {
        m_context->ClearDepthStencilView(m_depthView.Get(), D3D11_CLEAR_DEPTH, 1.0f,
                                         0); // 1.0 = the far plane: everything is nearer than that
    }
}

void Device::EndFrame()
{
    const HRESULT hr = m_swapChain->Present(m_vsync ? 1 : 0, 0);
    if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
    {
        DL_LOG_ERROR("Device", "device lost (Present 0x%08X, reason 0x%08X)", static_cast<unsigned>(hr),
                     static_cast<unsigned>(m_device->GetDeviceRemovedReason()));
        m_lost = true; // TownSession sees Lost() and makes a new Device (Gpu::Recover); this object is not used again
    }
}

void Device::Shutdown()
{
    if (m_context != nullptr)
    {
        m_context->ClearState();
        m_context->Flush();
    }
    m_depthView.Reset();
    m_depthTexture.Reset();
    m_backBufferRtv.Reset();
    m_swapChain.Reset();
    m_annotation.Reset();
    m_context.Reset();
    if (m_debug != nullptr)
    {
        // Everything else is released: whatever is listed now (other than the device itself) is a leak.
        m_debug->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL | D3D11_RLDO_IGNORE_INTERNAL);
        m_debug.Reset();
    }
    m_device.Reset();
    m_factory.Reset();
}
} // namespace dl
