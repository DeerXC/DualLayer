#include "core/Window.h"

#include "core/Log.h"

namespace dl
{
namespace
{
constexpr wchar_t kClassName[] = L"DualLayerWindow";
}

Window::~Window()
{
    if (m_hwnd != nullptr)
    {
        DestroyWindow(m_hwnd);
    }
}

bool Window::Create(HINSTANCE instance, const wchar_t* title, int clientWidth, int clientHeight)
{
    // WHY: without this Windows scales the whole window bitmap at 125 / 150 % and the picture is blurry.
    // It must be called before any window exists. Failure (already set, or an old Windows) is not fatal.
    if (!SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2))
    {
        DL_LOG_WARN("Window", "SetProcessDpiAwarenessContext failed (error %lu); the picture may be scaled by Windows", GetLastError());
    }

    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = &Window::WndProc;
    wc.hInstance = instance;
    wc.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512)); // 32512 = IDC_ARROW
    wc.lpszClassName = kClassName;
    if (RegisterClassExW(&wc) == 0)
    {
        DL_LOG_ERROR("Window", "RegisterClassEx failed (error %lu)", GetLastError());
        return false;
    }

    RECT rect{0, 0, clientWidth, clientHeight};
    const DWORD style = WS_OVERLAPPEDWINDOW;
    AdjustWindowRect(&rect, style, FALSE); // the requested size is the CLIENT size
    m_hwnd = CreateWindowExW(0, kClassName, title, style, CW_USEDEFAULT, CW_USEDEFAULT, rect.right - rect.left,
                             rect.bottom - rect.top, nullptr, nullptr, instance, this);
    if (m_hwnd == nullptr)
    {
        DL_LOG_ERROR("Window", "CreateWindowEx failed (error %lu)", GetLastError());
        return false;
    }
    RECT client{};
    GetClientRect(m_hwnd, &client);
    m_width = client.right - client.left;
    m_height = client.bottom - client.top;
    ShowWindow(m_hwnd, SW_SHOW);
    DL_LOG_INFO("Window", "created: client %d x %d physical px, DPI %u", m_width, m_height, GetDpiForWindow(m_hwnd));
    return true;
}

bool Window::PumpMessages()
{
    MSG msg{};
    while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE) != 0)
    {
        if (msg.message == WM_QUIT)
        {
            m_quit = true;
        }
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return !m_quit;
}

void Window::SetTitle(const wchar_t* title)
{
    SetWindowTextW(m_hwnd, title);
}

bool Window::ConsumeResized()
{
    const bool r = m_resized;
    m_resized = false;
    return r;
}

bool Window::ConsumeReloadKey()
{
    const bool r = m_reloadKey;
    m_reloadKey = false;
    return r;
}

LRESULT CALLBACK Window::WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == WM_NCCREATE)
    {
        const auto* create = reinterpret_cast<const CREATESTRUCTW*>(lParam);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(create->lpCreateParams));
    }
    auto* self = reinterpret_cast<Window*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (self != nullptr)
    {
        return self->HandleMessage(hwnd, message, wParam, lParam);
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
}

LRESULT Window::HandleMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_SIZE:
        m_minimized = (wParam == SIZE_MINIMIZED);
        if (!m_minimized)
        {
            m_width = static_cast<int>(LOWORD(lParam));
            m_height = static_cast<int>(HIWORD(lParam));
            m_resized = true;
        }
        return 0;
    case WM_DPICHANGED:
    {
        // WHY: Windows suggests a new window rectangle when the window is dragged to a monitor with another scale.
        const RECT* suggested = reinterpret_cast<const RECT*>(lParam);
        SetWindowPos(hwnd, nullptr, suggested->left, suggested->top, suggested->right - suggested->left,
                     suggested->bottom - suggested->top, SWP_NOZORDER | SWP_NOACTIVATE);
        return 0;
    }
    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE)
        {
            DestroyWindow(hwnd);
        }
        else if (wParam == VK_F5 && (lParam & (1 << 30)) == 0) // bit 30: the key was already down (auto-repeat)
        {
            m_reloadKey = true;
        }
        return 0;
    case WM_DESTROY:
        m_hwnd = nullptr;
        PostQuitMessage(0);
        return 0;
    default:
        break;
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
}
} // namespace dl
