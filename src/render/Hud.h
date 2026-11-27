// Hud.h - the overlay of one frame: the dot in the middle, the prompt at the bottom, the status line at the top left,
// and a tint when the body is under water. Sizes follow the height of the window, so the text is as big on a laptop as
// on a desktop screen.
// Owns:   nothing (it fills an OverlayBatch that the caller owns).
// UNITS:  pixels.
#pragma once

#include "render/OverlayBatch.h"
#include "scene/Prompts.h"

#include <algorithm>
#include <cmath>

namespace dl
{

/// Fills `hud` with the overlay of one frame. prompt: the row at the bottom ([KEY] VERB; kind Count: none). status: the
/// line at the top left. underwater: 0 .. 1, how deep the tint is. reticle: the dot.
inline void BuildHud(OverlayBatch& hud, int width, int height, const prompt::Shown& shown, const char* status,
                     float underwater, bool reticle)
{
    namespace pr = prompt;
    hud.Clear();
    const float w = static_cast<float>(width);
    const float h = static_cast<float>(height);
    // under water: the view is tinted deep green-blue and darker
    if (underwater > 0.0f)
    {
        hud.AddRect(0.0f, 0.0f, w, h, {0.03f, 0.20f, 0.26f, 0.55f * underwater});
    }
    const float unit = std::max(2.0f, std::floor(h / 320.0f)); // one font pixel, in screen pixels
    // the dot in the middle of the view: what E looks at
    const float dot = std::max(2.0f, std::floor(unit * 0.8f));
    if (reticle)
    {
        hud.AddRect(std::floor(0.5f * (w - dot)), std::floor(0.5f * (h - dot)), dot, dot, {1.0f, 1.0f, 1.0f, 0.75f});
    }
    // the status line
    const float statusUnit = std::max(2.0f, std::floor(unit * 0.6f));
    hud.AddRect(0.0f, 0.0f, OverlayBatch::TextWidth(status, statusUnit) + 4.0f * statusUnit,
                OverlayBatch::TextHeight(statusUnit) + 3.0f * statusUnit, {0.0f, 0.0f, 0.0f, 0.35f});
    hud.AddText(2.0f * statusUnit, 1.5f * statusUnit, statusUnit, status, {0.92f, 0.92f, 0.92f, 0.85f});
    // the prompt, one row at the bottom: [KEY] VERB
    if (shown.kind != pr::Kind::Count)
    {
        const float y = h - 6.0f * unit - OverlayBatch::TextHeight(unit);
        const pr::Text text = pr::TextOf(shown.kind);
        const float a = shown.alpha;
        const float pad = 2.0f * unit;
        const float keyWidth = OverlayBatch::TextWidth(text.key, unit) + 2.0f * pad;
        const float verbWidth = OverlayBatch::TextWidth(text.verb, unit);
        const float rowHeight = OverlayBatch::TextHeight(unit) + 2.0f * pad;
        const float total = keyWidth + 3.0f * unit + verbWidth;
        const float x = std::floor(0.5f * (w - total));
        hud.AddRect(x - 2.0f * unit, y - pad, total + 4.0f * unit, rowHeight, {0.0f, 0.0f, 0.0f, 0.45f * a});
        hud.AddFrame(x, y - pad, keyWidth, rowHeight, std::max(1.0f, std::floor(unit * 0.5f)), {1.0f, 0.86f, 0.45f, a});
        hud.AddText(x + pad, y, unit, text.key, {1.0f, 0.86f, 0.45f, a});
        hud.AddText(x + keyWidth + 3.0f * unit, y, unit, text.verb, {1.0f, 1.0f, 1.0f, a});
    }
}

} // namespace dl
