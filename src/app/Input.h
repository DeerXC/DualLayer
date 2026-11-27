// Input.h - the keyboard as the game reads it: polled every frame, with the edge of a press found by the caller's own
// memory.
// Focus:   every function takes `focused` or is called only when the window is in front, so that a key pressed in
//          another program never moves the body.
// Windows: this is the only place of the game loop that asks Windows for a key.
#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include "scene/Camera.h"

namespace dl::input
{

/// True while the key is held down right now.
inline bool IsDown(int virtualKey)
{
    return (GetAsyncKeyState(virtualKey) & 0x8000) != 0;
}

/// True once per press of a key (a key held down counts once). `wasDown` remembers the last frame.
inline bool PressedOnce(int virtualKey, bool& wasDown, bool focused)
{
    const bool down = focused && IsDown(virtualKey);
    const bool pressed = down && !wasDown;
    wasDown = down;
    return pressed;
}

/// How much faster Shift makes the flying camera (debug fly and noclip; walking has its own sprint).
inline constexpr float kFlyBoost = 4.0f;

/// Reads the keyboard: W/S forward/back, D/A right/left, E or Space / Q up/down, Shift boosts, arrow keys turn. Each value is -1, 0
/// or +1 (boost: 1 or kFlyBoost).
inline CameraInput ReadCameraKeys()
{
    CameraInput input;
    input.moveForward = (IsDown('W') ? 1.0f : 0.0f) - (IsDown('S') ? 1.0f : 0.0f);
    input.moveRight = (IsDown('D') ? 1.0f : 0.0f) - (IsDown('A') ? 1.0f : 0.0f);
    input.moveUp = ((IsDown('E') || IsDown(VK_SPACE)) ? 1.0f : 0.0f) - (IsDown('Q') ? 1.0f : 0.0f);
    input.boost = IsDown(VK_SHIFT) ? kFlyBoost : 1.0f;
    input.lookRight = (IsDown(VK_RIGHT) ? 1.0f : 0.0f) - (IsDown(VK_LEFT) ? 1.0f : 0.0f);
    input.lookUp = (IsDown(VK_UP) ? 1.0f : 0.0f) - (IsDown(VK_DOWN) ? 1.0f : 0.0f);
    return input;
}

} // namespace dl::input
