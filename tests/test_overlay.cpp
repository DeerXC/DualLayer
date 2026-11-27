// test_overlay.cpp - tests of the parts of the screen overlay that need no window and no GPU: the font
// (src/render/Font5x7.h), the batch of rectangles and text (src/render/OverlayBatch.h) and the choice of prompts
// (src/scene/Prompts.h).
//   g++ -std=c++20 -Wall -Wextra -I src tests/test_overlay.cpp -o test_overlay
//   MSVC (Developer PowerShell):  .\tests\run_tests.ps1 -Only overlay
// Header-only: there is no .cpp to add to the project. What is checked:
//   1. the font: every letter, digit and sign that the game writes has its own glyph; a small letter is drawn as its
//      capital; an unknown character is a box; some glyphs pixel by pixel ('E', 'T', '1')
//   2. the batch: a rectangle is six vertices with the right corners and colour; a frame is four rectangles round the
//      inside of its box; text covers exactly the lit pixels of its glyphs (the area is counted), its width is n * 6 -
//      1 font pixels; a full batch drops whole rectangles and says so; nothing for a size that is not above 0
//   3. the prompts: nothing unless the body walks; a shut thing in reach in the past wins and says its verb, then the
//      lantern (call it back, take it, put it down); nothing for a thing in the present; every prompt text can be drawn
//      by the font
// Returns 1 when a check fails.

#include "check.h"
#include "render/OverlayBatch.h"
#include "scene/Prompts.h"

#include <cmath>
#include <cstdio>
#include <cstring>

namespace
{

using dl::OverlayBatch;
using dl::OverlayColor;
namespace font = dl::font;
namespace prompt = dl::prompt;

bool Near(float a, float b, float eps = 1e-4f)
{
    return std::fabs(a - b) <= eps;
}

int LitPixels(const font::Glyph& g)
{
    int n = 0;
    for (int y = 0; y < font::kGlyphHeight; ++y)
    {
        for (int x = 0; x < font::kGlyphWidth; ++x)
        {
            n += font::Lit(g, x, y) ? 1 : 0;
        }
    }
    return n;
}

/// The area covered by the triangles of the batch (each triangle by the shoelace formula).
float Area(const OverlayBatch& b, int first = 0)
{
    float area = 0.0f;
    const dl::OverlayVertex* v = b.Data();
    for (int i = first; i + 2 < b.Count(); i += 3)
    {
        area += 0.5f * std::fabs((v[i + 1].x - v[i].x) * (v[i + 2].y - v[i].y) -
                                 (v[i + 2].x - v[i].x) * (v[i + 1].y - v[i].y));
    }
    return area;
}

void TestFont()
{
    const char* needed = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 :/.,-+=()!?'<>*";
    for (const char* p = needed; *p != '\0'; ++p)
    {
        CHECK(&font::Find(*p) != &font::kMissing);
    }
    CHECK(&font::Find('e') == &font::Find('E') && &font::Find('z') == &font::Find('Z'));
    CHECK(&font::Find('~') == &font::kMissing && LitPixels(font::kMissing) == 5 + 5 + 5 * 2);
    CHECK(LitPixels(font::Find(' ')) == 0);
    const font::Glyph& e =
        font::Find('E'); // the top row is full, the middle row is four from the left, the left column is full
    for (int x = 0; x < 5; ++x)
    {
        CHECK(font::Lit(e, x, 0) && font::Lit(e, x, 6));
    }
    CHECK(font::Lit(e, 3, 3) && !font::Lit(e, 4, 3));
    for (int y = 0; y < 7; ++y)
    {
        CHECK(font::Lit(e, 0, y));
    }
    const font::Glyph& t = font::Find('T');
    CHECK(LitPixels(t) == 5 + 6);
    const font::Glyph& one = font::Find('1');
    CHECK(font::Lit(one, 2, 0) && font::Lit(one, 1, 1) && LitPixels(one) == 1 + 2 + 4 + 3);
    CHECK(!font::Lit(e, -1, 0) && !font::Lit(e, 5, 0) && !font::Lit(e, 0, 7)); // outside: not lit
}

void TestBatch()
{
    static OverlayBatch b;
    const OverlayColor red{1.0f, 0.0f, 0.0f, 0.5f};
    b.AddRect(10.0f, 20.0f, 30.0f, 40.0f, red);
    CHECK(b.Count() == 6 && Near(Area(b), 1200.0f));
    float minX = 1e9f;
    float maxX = -1e9f;
    float minY = 1e9f;
    float maxY = -1e9f;
    for (int i = 0; i < 6; ++i)
    {
        const dl::OverlayVertex& v = b.Data()[i];
        minX = std::fmin(minX, v.x);
        maxX = std::fmax(maxX, v.x);
        minY = std::fmin(minY, v.y);
        maxY = std::fmax(maxY, v.y);
        CHECK(v.r == 1.0f && v.g == 0.0f && v.a == 0.5f);
    }
    CHECK(minX == 10.0f && maxX == 40.0f && minY == 20.0f && maxY == 60.0f);
    b.AddRect(0.0f, 0.0f, 0.0f, 5.0f, red); // nothing for a size that is not above 0
    b.AddRect(0.0f, 0.0f, 5.0f, -1.0f, red);
    CHECK(b.Count() == 6);
    b.Clear();
    b.AddFrame(0.0f, 0.0f, 100.0f, 50.0f, 4.0f, red); // the ring inside the box
    CHECK(b.Count() == 24 && Near(Area(b), 100.0f * 50.0f - 92.0f * 42.0f));
    b.Clear();
    const float scale = 3.0f;
    const char* text = "E TAKE";
    const float width = b.AddText(5.0f, 7.0f, scale, text, red);
    CHECK(Near(width, static_cast<float>(6 * 6 - 1) * scale) && Near(width, OverlayBatch::TextWidth(text, scale)));
    int lit = 0;
    for (const char* p = text; *p != '\0'; ++p)
    {
        lit += LitPixels(font::Find(*p));
    }
    CHECK(Near(Area(b), static_cast<float>(lit) * scale * scale, 1e-2f)); // exactly the lit pixels, none twice
    bool inside = true;
    for (int i = 0; i < b.Count(); ++i)
    {
        const dl::OverlayVertex& v = b.Data()[i];
        inside = inside && v.x >= 5.0f && v.x <= 5.0f + width && v.y >= 7.0f &&
                 v.y <= 7.0f + OverlayBatch::TextHeight(scale);
    }
    CHECK(inside);
    CHECK(b.Count() < lit * 6); // runs of pixels are merged: fewer rectangles than lit pixels
    CHECK(OverlayBatch::TextWidth("", 2.0f) == 0.0f && OverlayBatch::TextWidth(nullptr, 2.0f) == 0.0f);
    CHECK(b.AddText(0.0f, 0.0f, 0.0f, "A", red) == 0.0f);
    // a full batch drops whole rectangles and says so
    static OverlayBatch full;
    for (int i = 0; i < OverlayBatch::kMaxVertices / 6 + 10; ++i)
    {
        full.AddRect(0.0f, 0.0f, 1.0f, 1.0f, red);
    }
    CHECK(full.Count() == OverlayBatch::kMaxVertices && full.Full());
    full.Clear();
    CHECK(full.Count() == 0 && !full.Full());
    // a layer kept from an earlier frame is added whole; one that does not fit is dropped whole and the batch says so
    static OverlayBatch kept;
    static OverlayBatch target;
    kept.Clear();
    target.Clear();
    kept.AddRect(0.0f, 0.0f, 10.0f, 10.0f, {1.0f, 0.0f, 0.0f, 1.0f});
    kept.AddTriangle(0.0f, 0.0f, 5.0f, 0.0f, 0.0f, 5.0f, {0.0f, 1.0f, 0.0f, 1.0f});
    target.AddRect(1.0f, 1.0f, 2.0f, 2.0f, {0.0f, 0.0f, 1.0f, 1.0f});
    target.Append(kept);
    CHECK(target.Count() == 6 + 9 && !target.Full());
    CHECK(Near(target.Data()[6].r, 1.0f) && Near(target.Data()[12].g, 1.0f));
    static OverlayBatch nothing; // (static: a batch is too large for the stack)
    target.Append(nothing);
    CHECK(target.Count() == 15);
    while (!target.Full())
    {
        target.AddRect(0.0f, 0.0f, 1.0f, 1.0f, {1.0f, 1.0f, 1.0f, 1.0f});
    }
    const int before = target.Count();
    target.Append(kept);
    CHECK(target.Count() == before && target.Full());
}

void TestShapes()
{
    static OverlayBatch b;
    const OverlayColor c{0.2f, 0.4f, 0.6f, 0.8f};
    b.AddTriangle(0.0f, 0.0f, 10.0f, 0.0f, 0.0f, 10.0f, c);
    CHECK(b.Count() == 3 && Near(Area(b), 50.0f) && b.Data()[1].x == 10.0f && b.Data()[2].g == 0.4f);
    b.Clear();
    b.AddLine(0.0f, 0.0f, 10.0f, 0.0f, 2.0f, c); // a line of 10 x 2
    CHECK(b.Count() == 6 && Near(Area(b), 20.0f, 1e-3f));
    b.Clear();
    b.AddLine(0.0f, 0.0f, 6.0f, 8.0f, 2.0f, c); // any direction: the same area (length 10)
    CHECK(Near(Area(b), 20.0f, 1e-3f));
    float minY = 1e9f;
    for (int i = 0; i < b.Count(); ++i)
    {
        minY = std::fmin(minY, b.Data()[i].y);
    }
    CHECK(minY < 0.0f); // the thickness is on both sides of the line
    b.Clear();
    b.AddLine(1.0f, 1.0f, 1.0f, 1.0f, 2.0f, c); // no length, no thickness: nothing
    b.AddLine(0.0f, 0.0f, 5.0f, 0.0f, 0.0f, c);
    b.AddLine(0.0f, 0.0f, 5.0f, 0.0f, -1.0f, c);
    CHECK(b.Count() == 0);
    // a full batch drops whole shapes, triangles and lines too
    for (int i = 0; i < OverlayBatch::kMaxVertices / 6; ++i)
    {
        b.AddRect(0.0f, 0.0f, 1.0f, 1.0f, c);
    }
    CHECK(b.Count() == OverlayBatch::kMaxVertices - OverlayBatch::kMaxVertices % 6 && !b.Full());
    b.AddLine(0.0f, 0.0f, 5.0f, 0.0f, 1.0f, c);
    b.AddTriangle(0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, c);
    b.AddRect(0.0f, 0.0f, 1.0f, 1.0f, c);
    CHECK(b.Count() <= OverlayBatch::kMaxVertices && b.Full());
    b.Clear();
    CHECK(b.Count() == 0 && !b.Full());
}

void TestPrompts()
{
    prompt::Input in;
    CHECK(prompt::Choose(in).kind == prompt::Kind::Count); // not active: nothing
    in.active = true;
    in.lanternInReach = true;
    CHECK(prompt::Choose(in).kind == prompt::Kind::Take && Near(prompt::Choose(in).alpha, 1.0f));
    in.lanternCarried = true; // carried: nothing to take; on the ground in the past it can be put down
    CHECK(prompt::Choose(in).kind == prompt::Kind::Count);
    in.grounded = true;
    CHECK(prompt::Choose(in).kind == prompt::Kind::PutDown && Near(prompt::Choose(in).alpha, prompt::kPutDownAlpha));
    in.layer = 1; // in the present the lantern is not set down
    CHECK(prompt::Choose(in).kind == prompt::Kind::Count);
    in = prompt::Input{};
    in.active = true;
    in.lanternFar = true;
    CHECK(prompt::Choose(in).kind == prompt::Kind::Recall && Near(prompt::Choose(in).alpha, prompt::kRecallAlpha));
    in.lanternInReach = true; // a lantern out of reach wins over a take
    CHECK(prompt::Choose(in).kind == prompt::Kind::Recall);
    // a shut thing in reach wins over the lantern, in the past only, with the prompt of its verb
    const prompt::Kind verbs[5] = {prompt::Kind::Open, prompt::Kind::Push, prompt::Kind::Lay, prompt::Kind::Ring,
                                   prompt::Kind::Lower};
    for (int v = 0; v < 5; ++v)
    {
        in.shutThingVerb = v;
        in.layer = 0;
        CHECK(prompt::Choose(in).kind == verbs[v]);
        in.layer = 1;
        CHECK(prompt::Choose(in).kind == prompt::Kind::Recall);
    }
    in.layer = 0;
    in.shutThingVerb = 99; // a verb that is not known counts as OPEN
    CHECK(prompt::Choose(in).kind == prompt::Kind::Open);
    in.active = false; // sinking or flying: E means nothing
    CHECK(prompt::Choose(in).kind == prompt::Kind::Count);
    // TAB is offered last, to a player who has not switched yet: anything for E comes first, nothing when not active
    in = prompt::Input{};
    in.active = true;
    in.offerSwitch = true;
    CHECK(prompt::Choose(in).kind == prompt::Kind::Switch && Near(prompt::Choose(in).alpha, 1.0f));
    in.lanternInReach = true;
    CHECK(prompt::Choose(in).kind == prompt::Kind::Take);
    in.lanternInReach = false;
    in.shutThingVerb = 0;
    CHECK(prompt::Choose(in).kind == prompt::Kind::Open);
    in.active = false;
    CHECK(prompt::Choose(in).kind == prompt::Kind::Count);
    // the map is offered last of all: anything else comes first, nothing when not active
    in = prompt::Input{};
    in.active = true;
    in.offerMap = true;
    CHECK(prompt::Choose(in).kind == prompt::Kind::Map && Near(prompt::Choose(in).alpha, prompt::kMapAlpha));
    in.offerSwitch = true;
    CHECK(prompt::Choose(in).kind == prompt::Kind::Switch);
    in.offerSwitch = false;
    in.lanternInReach = true;
    CHECK(prompt::Choose(in).kind == prompt::Kind::Take);
    in.active = false;
    CHECK(prompt::Choose(in).kind == prompt::Kind::Count);
    for (int k = 0; k < prompt::kKindCount; ++k) // every text can be drawn
    {
        const prompt::Text text = prompt::TextOf(static_cast<prompt::Kind>(k));
        CHECK(std::strlen(text.key) > 0 && std::strlen(text.verb) > 0);
        for (const char* p = text.verb; *p != '\0'; ++p)
        {
            CHECK(&font::Find(*p) != &font::kMissing);
        }
        for (const char* p = text.key; *p != '\0'; ++p)
        {
            CHECK(&font::Find(*p) != &font::kMissing);
        }
    }
}

} // namespace

int main()
{
    TestFont();
    TestBatch();
    TestShapes();
    TestPrompts();
    std::printf("%d checks passed, %d failed\n", g_passed, g_failed);
    return g_failed == 0 ? 0 : 1;
}
