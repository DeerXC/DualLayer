#include "core/Window.h"

#include "core/Log.h"

namespace dl
{

namespace
{
constexpr wchar_t kClassName[] = L"DualLayerWindow";
constexpr USHORT kUsagePageGeneric = 0x01; // HID usage page "generic desktop"
constexpr USHORT kUsageMouse = 0x02;       // HID usage "mouse"
} // namespace

Window::~Window()
{
    SetMouseCaptured(false);
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
        DL_LOG_WARN("Window", "SetProcessDpiAwarenessContext failed (error %lu); the picture may be scaled by Windows",
                    GetLastError());
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

    // raw mouse input: the movement of the mouse itself, without the pointer acceleration of Windows (mouse look)
    RAWINPUTDEVICE mouse{};
    mouse.usUsagePage = kUsagePageGeneric;
    mouse.usUsage = kUsageMouse;
    mouse.dwFlags = 0;
    mouse.hwndTarget = m_hwnd;
    if (RegisterRawInputDevices(&mouse, 1, sizeof(mouse)) == FALSE)
    {
        DL_LOG_WARN("Window", "RegisterRawInputDevices failed (error %lu): no mouse look, the arrow keys still turn",
                    GetLastError());
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

void Window::ConsumeMouseDelta(long& dx, long& dy)
{
    dx = m_mouseDx;
    dy = m_mouseDy;
    m_mouseDx = 0;
    m_mouseDy = 0;
}

bool Window::CursorClient(int& x, int& y) const
{
    POINT p{};
    RECT client{};
    if (m_hwnd == nullptr || GetCursorPos(&p) == FALSE || ScreenToClient(m_hwnd, &p) == FALSE ||
        GetClientRect(m_hwnd, &client) == FALSE)
    {
        return false;
    }
    x = static_cast<int>(p.x);
    y = static_cast<int>(p.y);
    return p.x >= 0 && p.y >= 0 && p.x < client.right && p.y < client.bottom;
}

bool Window::ConsumeEscape()
{
    const bool r = m_escape;
    m_escape = false;
    return r;
}

void Window::ClipToClient()
{
    RECT client{};
    GetClientRect(m_hwnd, &client);
    POINT corners[2] = {{client.left, client.top}, {client.right, client.bottom}};
    MapWindowPoints(m_hwnd, nullptr, corners, 2); // client coordinates -> screen coordinates
    const RECT screen{corners[0].x, corners[0].y, corners[1].x, corners[1].y};
    ClipCursor(&screen);
}

void Window::SetMouseCaptured(bool captured)
{
    if (captured == m_captured || m_hwnd == nullptr)
    {
        return;
    }
    m_captured = captured;
    m_mouseDx = 0;
    m_mouseDy = 0;
    if (captured)
    {
        ClipToClient();
        ShowCursor(FALSE); // WHY once per change: ShowCursor keeps a counter, so the calls must be paired
    }
    else
    {
        ClipCursor(nullptr);
        ShowCursor(TRUE);
    }
    DL_LOG_INFO("Window", "mouse %s", captured ? "captured (Esc releases it)" : "released (click to capture)");
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
        if (m_captured)
        {
            ClipToClient();
        }
        return 0;
    case WM_MOVE:
        if (m_captured)
        {
            ClipToClient();
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
    case WM_INPUT:
    {
        RAWINPUT raw{};
        UINT size = sizeof(raw);
        if (m_captured &&
            GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, &raw, &size, sizeof(RAWINPUTHEADER)) !=
                static_cast<UINT>(-1) &&
            raw.header.dwType == RIM_TYPEMOUSE && (raw.data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE) == 0)
        {
            m_mouseDx += raw.data.mouse.lLastX;
            m_mouseDy += raw.data.mouse.lLastY;
        }
        return DefWindowProcW(hwnd, message, wParam,
                              lParam); // WHY: WM_INPUT must reach DefWindowProc so Windows can free its data
    }
    case WM_LBUTTONDOWN:
        if (m_captureAllowed)
        {
            SetMouseCaptured(true);
        }
        return 0;
    case WM_ACTIVATE:
        if (LOWORD(wParam) == WA_INACTIVE)
        {
            SetMouseCaptured(false); // Alt + Tab, or another window in front: the cursor belongs to the user again
        }
        break; // WHY not return: DefWindowProc gives the keyboard focus to the window when it becomes active
    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE)
        {
            if (m_captured)
            {
                SetMouseCaptured(false); // the first Esc gives the mouse back; the second one quits
            }
            else if (!m_escapeQuits)
            {
                m_escape = true; // the map is open: it closes on Esc
            }
            else
            {
                DestroyWindow(hwnd);
            }
        }
        else if (wParam == VK_F5 && (lParam & (1 << 30)) == 0) // bit 30: the key was already down (auto-repeat)
        {
            m_reloadKey = true;
        }
        return 0;
    case WM_DESTROY:
        SetMouseCaptured(false);
        m_hwnd = nullptr;
        PostQuitMessage(0);
        return 0;
    default:
        break;
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
}

} // namespace dl
