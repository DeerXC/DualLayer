// Window.h - one Win32 window. Declares per-monitor DPI awareness before creating it.
// Owns:      the HWND.
// Units:     all sizes are PHYSICAL pixels of the client area (at 150% scaling a 1600 px window is about 1067 logical px wide).
// Threading: main thread only (messages are pumped by PumpMessages()).
#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

namespace dl
{
class Window
{
public:
    Window() = default;
    ~Window();
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    /// Sets DPI awareness, registers the class and creates the window with the given CLIENT size.
    bool Create(HINSTANCE instance, const wchar_t* title, int clientWidth, int clientHeight);

    /// Dispatches pending messages. Returns false once the window was closed.
    bool PumpMessages();

    void SetTitle(const wchar_t* title);

    HWND Handle() const { return m_hwnd; }
    int Width() const { return m_width; }
    int Height() const { return m_height; }
    bool IsMinimized() const { return m_minimized; }

    /// True once after the client size changed (resize, DPI change, restore).
    bool ConsumeResized();
    /// True once after F5 was pressed.
    bool ConsumeReloadKey();

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

    HWND m_hwnd = nullptr;
    int m_width = 0;
    int m_height = 0;
    bool m_minimized = false;
    bool m_resized = false;
    bool m_reloadKey = false;
    bool m_quit = false;
};
} // namespace dl
