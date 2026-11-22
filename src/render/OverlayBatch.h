// OverlayBatch.h - what the screen overlay draws in one frame, as a list of coloured triangles in screen pixels: rectangles,
//            frames and text (render/Font5x7.h). render/Overlay.h puts the list on the GPU; this part has no Windows and
//            no Direct3D, so the layout is tested without a window (tests/test_overlay.cpp).
// Space:     screen pixels, (0, 0) = the top-left corner of the client area, +x to the right, +y down.
// Colour:    sRGB with alpha (0 .. 1), as a designer writes it; the overlay converts it to linear when it is drawn (the back
//            buffer is written through an sRGB view, docs/CONVENTIONS.md).
// Text:      every row of a glyph is cut into runs of lit pixels, and each run is one rectangle (a letter is a few
//            rectangles, not 35). `scale` is the size of a font pixel in screen pixels.
// Limits:    at most kMaxVertices; what does not fit is dropped whole (a rectangle is never half drawn), and Full() says so.
#pragma once

#include "render/Font5x7.h"

#include <array>
#include <cstddef>

namespace dl
{

struct OverlayVertex
{
    float x;
    float y;
    float r;
    float g;
    float b;
    float a;
};
static_assert(sizeof(OverlayVertex) == 24, "the input layout of the overlay (render/Overlay.cpp) assumes 24 bytes per vertex");

struct OverlayColor
{
    float r = 1.0f;
    float g = 1.0f;
    float b = 1.0f;
    float a = 1.0f;
};

class OverlayBatch
{
  public:
    static constexpr int kMaxVertices = 12000; ///< 2000 rectangles

    void Clear()
    {
        m_count = 0;
        m_full = false;
    }

    /// A filled rectangle. Nothing for a width or height that is not above 0.
    void AddRect(float x, float y, float w, float h, const OverlayColor& c)
    {
        if (!(w > 0.0f) || !(h > 0.0f))
        {
            return;
        }
        if (m_count + 6 > kMaxVertices)
        {
            m_full = true;
            return;
        }
        // two triangles, clockwise on the screen (y down): (x0 y0) (x1 y0) (x1 y1), (x0 y0) (x1 y1) (x0 y1)
        const float x1 = x + w;
        const float y1 = y + h;
        Put(x, y, c);
        Put(x1, y, c);
        Put(x1, y1, c);
        Put(x, y, c);
        Put(x1, y1, c);
        Put(x, y1, c);
    }

    /// A frame of `thickness` inside the rectangle.
    void AddFrame(float x, float y, float w, float h, float thickness, const OverlayColor& c)
    {
        AddRect(x, y, w, thickness, c);
        AddRect(x, y + h - thickness, w, thickness, c);
        AddRect(x, y + thickness, thickness, h - 2.0f * thickness, c);
        AddRect(x + w - thickness, y + thickness, thickness, h - 2.0f * thickness, c);
    }

    /// Text with its top-left corner at (x, y). Returns its width in screen pixels.
    float AddText(float x, float y, float scale, const char* text, const OverlayColor& c)
    {
        if (text == nullptr || !(scale > 0.0f))
        {
            return 0.0f;
        }
        float pen = x;
        for (const char* p = text; *p != '\0'; ++p)
        {
            const font::Glyph& g = font::Find(*p);
            for (int row = 0; row < font::kGlyphHeight; ++row)
            {
                int col = 0;
                while (col < font::kGlyphWidth)
                {
                    if (!font::Lit(g, col, row))
                    {
                        ++col;
                        continue;
                    }
                    int end = col;
                    while (end < font::kGlyphWidth && font::Lit(g, end, row))
                    {
                        ++end;
                    }
                    AddRect(pen + static_cast<float>(col) * scale, y + static_cast<float>(row) * scale,
                            static_cast<float>(end - col) * scale, scale, c);
                    col = end;
                }
            }
            pen += static_cast<float>(font::kAdvance) * scale;
        }
        return TextWidth(text, scale);
    }

    /// The width of `text` at `scale`, without the gap after the last character.
    static float TextWidth(const char* text, float scale)
    {
        int n = 0;
        for (const char* p = text; p != nullptr && *p != '\0'; ++p)
        {
            ++n;
        }
        return n == 0 ? 0.0f : static_cast<float>(n * font::kAdvance - (font::kAdvance - font::kGlyphWidth)) * scale;
    }

    static float TextHeight(float scale)
    {
        return static_cast<float>(font::kGlyphHeight) * scale;
    }

    int Count() const
    {
        return m_count;
    }

    bool Full() const
    {
        return m_full;
    }

    const OverlayVertex* Data() const
    {
        return m_vertices.data();
    }

  private:
    void Put(float x, float y, const OverlayColor& c)
    {
        m_vertices[static_cast<std::size_t>(m_count++)] = OverlayVertex{x, y, c.r, c.g, c.b, c.a};
    }

    std::array<OverlayVertex, kMaxVertices> m_vertices{};
    int m_count = 0;
    bool m_full = false;
};

} // namespace dl
