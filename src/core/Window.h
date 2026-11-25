// Window.h - one Win32 window. Declares per-monitor DPI awareness before creating it.
// Owns:      the HWND, and the state of the mouse capture.
// Units:     all sizes are PHYSICAL pixels of the client area (at 150% scaling a 1600 px window is about 1067 logical
// px wide). Mouse:     raw mouse input (WM_INPUT) is read while the mouse is captured: a click into the window captures
// it (the cursor is
//            hidden and kept inside the window), Esc or leaving the window (Alt + Tab) releases it. The movement is
//            added up in counts of the mouse, not in pixels, so it does not stop at the edge of the screen and ignores
//            the pointer speed setting of Windows. Esc while the mouse is free closes the window, unless the program
//            asks to be told of it instead (SetEscapeQuits: the map closes on Esc). While the map is open a click does
//            not capture the mouse (SetCaptureAllowed), and CursorClient says where the pointer is.
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

    HWND Handle() const
    {
        return m_hwnd;
    }
    int Width() const
    {
        return m_width;
    }
    int Height() const
    {
        return m_height;
    }
    bool IsMinimized() const
    {
        return m_minimized;
    }

    /// True once after the client size changed (resize, DPI change, restore).
    bool ConsumeResized();
    /// True once after F5 was pressed.
    bool ConsumeReloadKey();

    /// The mouse movement since the last call, in counts of the mouse (+x right, +y toward the user); 0 while not
    /// captured.
    void ConsumeMouseDelta(long& dx, long& dy);
    bool MouseCaptured() const
    {
        return m_captured;
    }
    /// Hides the cursor and keeps it inside the window (true), or gives it back (false).
    void SetMouseCaptured(bool captured);

    /// The pointer in client pixels (0, 0 = top left); false while it is outside the client area.
    bool CursorClient(int& x, int& y) const;

    /// True (the default): Esc with the mouse free closes the window. False: it is only remembered (ConsumeEscape).
    void SetEscapeQuits(bool quits)
    {
        m_escapeQuits = quits;
    }

    /// True once after Esc was pressed with the mouse free and SetEscapeQuits(false).
    bool ConsumeEscape();

    /// True (the default): a click into the window captures the mouse. False: the pointer stays free (the map is open).
    void SetCaptureAllowed(bool allowed)
    {
        m_captureAllowed = allowed;
    }

  private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    void ClipToClient();

    HWND m_hwnd = nullptr;
    int m_width = 0;
    int m_height = 0;
    long m_mouseDx = 0;
    long m_mouseDy = 0;
    bool m_minimized = false;
    bool m_resized = false;
    bool m_reloadKey = false;
    bool m_quit = false;
    bool m_captured = false;
    bool m_escapeQuits = true;
    bool m_escape = false;
    bool m_captureAllowed = true;
};

} // namespace dl
