// Prompts.h - which key prompt the screen shows now, and how strongly (docs/LEVEL_DESIGN.md, 9: guidance without words; the
//            prompt is the one place with words, and it names only a key and a verb).
// Rules:     at most one prompt for E and one for another key at a time.
//            E: holding an orb -> PLACE when it would go onto a socket, a plate or a hole (Mechanisms::SpotAhead), else PUT
//            DOWN; holding nothing -> TAKE when an orb is in reach (Mechanisms::OrbInReach).
//            TAB: SWITCH WORLD until the switch has been used kLearnedAfter times (the first lesson of the game), and again
//            whenever a held orb is in front of a socket or plate of the other world (it fits only in its own world).
//            R: RESET ROOM in a room with a puzzle after kStuckSeconds in it without leaving.
// Fading:    a prompt that has been followed kLearnedAfter times is drawn dimmer (kLearnedAlpha): it is still there when it
//            is needed, but it no longer shouts. TAB is not shown at all once learned.
// Plain logic, no Windows, no Direct3D: tested without a window (tests/test_overlay.cpp). UNITS: seconds.
#pragma once

#include <array>
#include <cstddef>

namespace dl::prompt
{

enum class Kind
{
    Take,
    PutDown,
    Place,
    SwitchWorld,
    ResetRoom,
    Open, ///< a thing that changes, at its handle in A (scene/Things.h)
    Push,
    Lay,
    Ring,
    Lower,
    Recall, ///< the lantern, set down far away: back to the hand
    Count
};

inline constexpr int kKindCount = static_cast<int>(Kind::Count);

struct Text
{
    const char* key;
    const char* verb;
};

/// The key and the verb of each prompt (the font has capitals only).
constexpr Text TextOf(Kind k)
{
    switch (k)
    {
    case Kind::Take:
        return {"E", "TAKE"};
    case Kind::PutDown:
        return {"E", "PUT DOWN"};
    case Kind::Place:
        return {"E", "PLACE"};
    case Kind::SwitchWorld:
        return {"TAB", "SWITCH WORLD"};
    case Kind::ResetRoom:
        return {"R", "RESET ROOM"};
    case Kind::Open:
        return {"E", "OPEN"};
    case Kind::Push:
        return {"E", "PUSH"};
    case Kind::Lay:
        return {"E", "LAY ACROSS"};
    case Kind::Ring:
        return {"E", "RING"};
    case Kind::Lower:
        return {"E", "LOWER"};
    case Kind::Recall:
        return {"E", "CALL THE LANTERN"};
    case Kind::Count:
        break;
    }
    return {"", ""};
}

inline constexpr int kLearnedAfter = 3;        ///< uses after which a prompt is dimmed (TAB: hidden)
inline constexpr float kLearnedAlpha = 0.45f;  ///< how strong a learned prompt is drawn
inline constexpr float kStuckSeconds = 90.0f;  ///< UNITS: seconds in a puzzle room before R is offered

/// What the game knows this frame.
struct Input
{
    bool walking = true;     ///< E and R only mean something while walking
    bool holding = false;    ///< an orb is held
    bool wouldPlace = false; ///< holding: the put-down would go onto a socket, a plate or a hole
    bool orbInReach = false; ///< not holding: E would take an orb
    bool otherWorldSpot = false; ///< holding: a socket or plate of the other world is in front
    bool puzzleRoom = false; ///< the room has something that R puts back
    float secondsInRoom = 0.0f;
};

struct Shown
{
    Kind kind = Kind::Count; ///< Kind::Count = nothing
    float alpha = 0.0f;
};

/// Counts how often each prompt was followed, and chooses what to show.
class Tracker
{
  public:
    /// The player pressed the key of `k` (and it did something).
    void Used(Kind k)
    {
        if (k != Kind::Count)
        {
            ++m_uses[static_cast<std::size_t>(k)];
        }
    }

    int Uses(Kind k) const
    {
        return k == Kind::Count ? 0 : m_uses[static_cast<std::size_t>(k)];
    }

    float Alpha(Kind k) const
    {
        return Uses(k) >= kLearnedAfter ? kLearnedAlpha : 1.0f;
    }

    /// The prompt for E (first) and the other prompt (second); either may be nothing.
    std::array<Shown, 2> Choose(const Input& in) const
    {
        std::array<Shown, 2> out{};
        if (in.walking)
        {
            Kind e = Kind::Count;
            if (in.holding)
            {
                e = in.wouldPlace ? Kind::Place : Kind::PutDown;
            }
            else if (in.orbInReach)
            {
                e = Kind::Take;
            }
            if (e != Kind::Count)
            {
                out[0] = {e, Alpha(e)};
            }
        }
        if (Uses(Kind::SwitchWorld) < kLearnedAfter || (in.walking && in.holding && in.otherWorldSpot && !in.wouldPlace))
        {
            out[1] = {Kind::SwitchWorld, 1.0f};
        }
        else if (in.walking && in.puzzleRoom && in.secondsInRoom >= kStuckSeconds)
        {
            out[1] = {Kind::ResetRoom, Alpha(Kind::ResetRoom)};
        }
        return out;
    }

  private:
    std::array<int, kKindCount> m_uses{};
};

} // namespace dl::prompt
