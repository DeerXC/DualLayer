// EndScreen.h - what the screen says when the goal is reached: the time, the hidden lights, the falls. The words of the
//            game stay few (docs/LEVEL_DESIGN.md, 9): this is the one screen with more than a key and a verb.
// Lines:     "CLEAR", "TIME m:ss.s", "HIDDEN LIGHTS n/N", "FALLS n", "THANK YOU FOR PLAYING". Every character is in the
//            font of the overlay (render/Font5x7.h; tests/test_overlay.cpp checks it).
// Shown:     kEndScreenDelay seconds after the clear, when the rings of the clear have spread (main.cpp).
// Plain logic, no Windows, no Direct3D. UNITS: seconds.
#pragma once

#include <cstdio>

namespace dl::endscreen
{

inline constexpr float kDelay = 1.6f; ///< UNITS: seconds after the clear
inline constexpr int kLines = 5;
inline constexpr int kLineLength = 32;

struct Screen
{
    char lines[kLines][kLineLength] = {};
    int count = 0;
};

/// The lines for a run of `seconds`, with `lights` of `lightTotal` hidden lights and `falls` falls (a fall or R).
inline Screen Make(double seconds, int lights, int lightTotal, int falls)
{
    Screen s;
    const double t = seconds < 0.0 ? 0.0 : (seconds > 5999.0 ? 5999.0 : seconds);
    const int minutes = static_cast<int>(t / 60.0);
    const double rest = t - 60.0 * minutes;
    std::snprintf(s.lines[0], kLineLength, "CLEAR");
    std::snprintf(s.lines[1], kLineLength, "TIME %d:%04.1f", minutes, rest);
    std::snprintf(s.lines[2], kLineLength, "HIDDEN LIGHTS %d/%d", lights, lightTotal);
    std::snprintf(s.lines[3], kLineLength, "FALLS %d", falls);
    std::snprintf(s.lines[4], kLineLength, "THANK YOU FOR PLAYING");
    s.count = kLines;
    return s;
}

/// The same with the title of an ending (scene/Ending.h) in the first line and, in the last, the way to see the last
/// moment again.
inline Screen Make(double seconds, int lights, int lightTotal, int falls, const char* title)
{
    Screen s = Make(seconds, lights, lightTotal, falls);
    std::snprintf(s.lines[0], kLineLength, "%s", title != nullptr ? title : "CLEAR");
    std::snprintf(s.lines[4], kLineLength, "R: THE LAST MOMENT AGAIN");
    return s;
}

} // namespace dl::endscreen
