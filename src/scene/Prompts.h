// Prompts.h - which key prompt the screen shows now: the key and a verb, the only words of the game.
// Rule:      at most one prompt. For E, in order of importance: a thing of the past within reach that is still shut
//            (OPEN, PUSH, LAY ACROSS, RING or LOWER, as its verb says; only in the past, because in the present a
//            handle does nothing); the lantern set down far away (CALL THE LANTERN, dimmed: a reminder); the lantern
//            within reach (TAKE); the lantern in hand on the ground in the past (PUT DOWN, dimmed). Last, and only when
//            there is nothing for E: TAB SWITCH WORLD, offered to a player who has not switched the moment yet (the
//            first lesson), and after it M MAP, offered to a player who has walked a while and has not opened the map
//            yet. Nothing while the body does not walk or is sinking.
// Plain logic, no Windows, no Direct3D: tested without a window (tests/test_overlay.cpp). The caller reads the state of
// the game into an Input; this file knows nothing of the town.
#pragma once

namespace dl::prompt
{

enum class Kind
{
    Take,
    PutDown,
    Open, ///< a thing that changes, at its handle in A (scene/Things.h)
    Push,
    Lay,
    Ring,
    Lower,
    Recall, ///< the lantern, set down far away: back to the hand
    Switch, ///< TAB: the first lesson of the game
    Map,    ///< M: the plan on two sheets of tracing paper
    Count
};

inline constexpr int kKindCount = static_cast<int>(Kind::Count);

inline constexpr float kRecallAlpha = 0.45f; ///< how strong the reminder to call the lantern is drawn
inline constexpr float kPutDownAlpha = 0.6f; ///< how strong the offer to put the lantern down is drawn
inline constexpr float kMapAlpha = 0.55f;    ///< how strong the offer of the map is drawn

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
    case Kind::Switch:
        return {"TAB", "SWITCH WORLD"};
    case Kind::Map:
        return {"M", "MAP"};
    case Kind::Count:
        break;
    }
    return {"", ""};
}

/// The prompt of a verb of a thing (the order of the verbs in data/town.dlt: OPEN, PUSH, LAY ACROSS, RING, LOWER);
/// anything else counts as OPEN.
constexpr Kind KindOfVerb(int verb)
{
    constexpr Kind kOfVerb[5] = {Kind::Open, Kind::Push, Kind::Lay, Kind::Ring, Kind::Lower};
    return kOfVerb[verb >= 0 && verb < 5 ? verb : 0];
}

/// What the game knows this frame.
struct Input
{
    bool active = false;     ///< the body walks and is not sinking: E means something
    int layer = 0;           ///< the moment on the screen: 0 = A (the past), 1 = B
    int shutThingVerb = -1;  ///< the verb of a still shut thing within reach, or -1 for none
    bool lanternFar = false; ///< the lantern is set down, out of reach but not out of call
    bool lanternInReach = false;
    bool lanternCarried = false;
    bool grounded = false;    ///< the body stands on something
    bool offerSwitch = false; ///< the player has not switched the moment yet and has had time to find out how
    bool offerMap = false;    ///< the player has walked a while and has never opened the map
};

struct Shown
{
    Kind kind = Kind::Count; ///< Kind::Count = nothing
    float alpha = 0.0f;
};

/// The prompt for E, or nothing.
constexpr Shown Choose(const Input& in)
{
    if (!in.active)
    {
        return {};
    }
    if (in.shutThingVerb >= 0 && in.layer == 0)
    {
        return {KindOfVerb(in.shutThingVerb), 1.0f};
    }
    if (in.lanternFar)
    {
        return {Kind::Recall, kRecallAlpha};
    }
    if (!in.lanternCarried && in.lanternInReach)
    {
        return {Kind::Take, 1.0f};
    }
    if (in.lanternCarried && in.layer == 0 && in.grounded)
    {
        return {Kind::PutDown, kPutDownAlpha};
    }
    if (in.offerSwitch)
    {
        return {Kind::Switch, 1.0f};
    }
    if (in.offerMap)
    {
        return {Kind::Map, kMapAlpha};
    }
    return {};
}

} // namespace dl::prompt
