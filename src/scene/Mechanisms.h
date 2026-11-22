// Mechanisms.h - the moving parts of a wing that react to the layer switch: orbs that are carried between the layers,
//            sockets and plates that an orb (or the body) works, levers, and gates (bridges, doors, bars) that open and close
//            with them (docs/LEVEL_DESIGN.md, section 4).
// Worlds:    one rule for every part: a thing works only with what is in its own world. An orb is drawn in the colour of
//            the world it is in (warm red-gold in A, cold blue in B), like the pads and the columns of light of that world,
//            so "the same colour fits" is seen before it is understood. A held orb is placed only on a socket or a plate of
//            the layer the body is in; in front of one of the other world it is put down as anywhere else, and the prompt
//            offers the switch of worlds (SpotAhead reports it).
// Orb:       a ball of kOrbRadius that is in ONE layer. Carried (E), it is in the layer of the body and switches with it. Put
//            down (E), it is placed one step ahead of the body, in the layer of the body, and stays there; near a spot (a
//            socket, a plate, a hole: kSnap) it is placed over the spot. With nothing under it in its own layer it falls
//            straight down to the first surface of that layer (no roll, no bounce); below the fall line it goes back to its
//            start. It is never solid for the body. A put-down through a solid (a door, bars, a wall) is refused.
// Socket:    active while an orb of its layer rests on it.
// Plate:     active while an orb of its layer rests on it, or while the body stands on it in its layer.
// Lever:     active from the moment a ball of the body touches it, in either layer, for ever.
// Latch:     a trigger may latch: once active, it stays active (a lever always does).
// Gate:      a box that is there while its trigger (or a second one) is active (a bridge), or while neither is (a door,
//            bars). It is solid and drawn in the layers of its mask, like any box of the map.
// Guidance:  OrbInReach and SpotAhead answer the questions of the prompts (scene/Prompts.h): can E take something, and would
//            the held orb be placed on a socket, a plate or a hole. AddBeacons draws a thin column of light over every socket
//            and plate that is not worked yet and every lever not pulled, in both layers, coloured by the layer it works in:
//            a place to bring something is seen from across the room.
// Reset:     ResetRoom(room) puts the orbs that start in that room back at their starts (R in docs/LEVEL_DESIGN.md, 9). A
//            latched trigger stays latched: what is done for good stays done.
// Plain data and plain logic, no Windows, no Direct3D: tested without a window (tests/test_mechanisms.cpp, and the walk
// through the west wing in tests/test_stage.cpp). UNITS: metres, seconds. Layers: 0 = A, 1 = B.
// Note:      no std::min / std::max here: windows.h may define them as macros (docs/CONVENTIONS.md).
#pragma once

#include "scene/Camera.h" // Vec3
#include "scene/Collision.h"
#include "scene/Player.h"
#include "scene/SceneData.h"

#include <array>
#include <cmath>
#include <cstddef>

namespace dl::mech
{

enum class TriggerKind
{
    Socket,
    Plate,
    Lever,
};

struct Trigger
{
    TriggerKind kind;
    Vec3 at;          ///< the centre of its top (a socket, a plate: on the floor) or the centre of the lever
    int layer;        ///< 0 = A, 1 = B: the layer in which it is worked (a lever: both)
    bool latch;       ///< once active, active for ever (a lever always latches)
    const char* name; ///< for the log
};

struct Gate
{
    scene::SceneObject box; ///< its shape, colour and layer mask
    int trigger;            ///< the trigger that opens it
    int orTrigger;          ///< a second one (-1: none): it counts as open when either is active
    bool presentWhenOpen;   ///< true: a bridge (there while open); false: a door, bars (there while closed)
    const char* name;       ///< for the log
};

struct OrbStart
{
    Vec3 centre; ///< where it starts (it falls onto what is under it)
    int layer;   ///< the layer it starts in
    int room;    ///< the room (scene/Stage.h kRooms) whose R puts it back
};

/// What a wing brings. Sockets and plates are spots by themselves; `spots` adds places such as holes.
struct WingDesc
{
    const Trigger* triggers = nullptr;
    int triggerCount = 0;
    const Gate* gates = nullptr;
    int gateCount = 0;
    const OrbStart* orbs = nullptr;
    int orbCount = 0;
    const Vec3* spots = nullptr;
    int spotCount = 0;
    float fallY = -100.0f; ///< an orb whose centre falls below this goes back to its start
};

inline constexpr float kOrbRadius = 0.25f;  ///< UNITS: metres
inline constexpr float kSnap = 0.9f;        ///< UNITS: metres: an orb put down this close to a spot (on the ground) is placed on it
inline constexpr float kPutAhead = 1.0f;    ///< UNITS: metres in front of the feet
inline constexpr float kReach = 1.4f;       ///< UNITS: metres: an orb this close to the feet (on the ground) can be taken
inline constexpr float kRestTolerance = 0.35f; ///< UNITS: metres: an orb resting this close to a socket or plate works it
inline constexpr float kLeverReach = 0.75f; ///< UNITS: metres between a ball of the body and the lever
inline constexpr int kMaxTriggers = 16;
inline constexpr int kMaxGates = 16;
inline constexpr int kMaxOrbs = 8;

/// What Interact did.
enum class Action
{
    Nothing, ///< no orb held and none in reach
    Took,    ///< an orb is now held
    Put,     ///< the held orb was put down
    Refused, ///< the put-down was refused (a solid in the way)
};

class MechanismSet
{
  public:
    struct Orb
    {
        Vec3 centre;
        int layer = 0;
        bool held = false;
        bool resting = false;
        float vy = 0.0f;
    };

    /// Copies the wing. False (and the set stays empty) when it does not fit.
    bool Init(const WingDesc& desc)
    {
        m_triggerCount = 0;
        m_gateCount = 0;
        m_orbCount = 0;
        m_spotCount = 0;
        if (desc.triggerCount < 0 || desc.triggerCount > kMaxTriggers || desc.gateCount < 0 || desc.gateCount > kMaxGates ||
            desc.orbCount < 0 || desc.orbCount > kMaxOrbs || desc.spotCount < 0 || desc.spotCount > kMaxTriggers)
        {
            return false;
        }
        for (int i = 0; i < desc.gateCount; ++i)
        {
            const Gate& g = desc.gates[i];
            if (g.trigger < 0 || g.trigger >= desc.triggerCount || g.orTrigger >= desc.triggerCount)
            {
                return false;
            }
        }
        for (int i = 0; i < desc.triggerCount; ++i)
        {
            m_triggers[static_cast<std::size_t>(i)] = desc.triggers[i];
        }
        for (int i = 0; i < desc.gateCount; ++i)
        {
            m_gates[static_cast<std::size_t>(i)] = desc.gates[i];
        }
        for (int i = 0; i < desc.orbCount; ++i)
        {
            m_starts[static_cast<std::size_t>(i)] = desc.orbs[i];
        }
        for (int i = 0; i < desc.spotCount; ++i)
        {
            m_spots[static_cast<std::size_t>(i)] = desc.spots[i];
        }
        m_triggerCount = desc.triggerCount;
        m_gateCount = desc.gateCount;
        m_orbCount = desc.orbCount;
        m_spotCount = desc.spotCount;
        m_fallY = desc.fallY;
        Reset();
        return true;
    }

    /// Everything back as at the start: every orb at its start, no trigger active or latched.
    void Reset()
    {
        for (int i = 0; i < m_orbCount; ++i)
        {
            PutBack(i);
        }
        for (int i = 0; i < m_triggerCount; ++i)
        {
            m_active[static_cast<std::size_t>(i)] = false;
            m_latched[static_cast<std::size_t>(i)] = false;
            m_changed[static_cast<std::size_t>(i)] = false;
        }
    }

    /// R in `room`: the orbs that start in it go back to their starts (also a held one). Latched triggers stay.
    void ResetRoom(int room)
    {
        for (int i = 0; i < m_orbCount; ++i)
        {
            if (m_starts[static_cast<std::size_t>(i)].room == room)
            {
                PutBack(i);
            }
        }
    }

    /// E: takes the nearest orb in reach (in the layer of the body), or puts the held one down. `solids` is the solid table
    /// of the layer of the body. `which` gets the orb.
    Action Interact(const Player& body, int bodyLayer, const Vec3& look, const scene::SceneObject* solids, int count, int& which)
    {
        which = HeldOrb();
        if (which >= 0)
        {
            return PutDown(which, body, bodyLayer, look, solids, count) ? Action::Put : Action::Refused;
        }
        which = OrbInReach(body, bodyLayer, solids, count);
        if (which < 0)
        {
            return Action::Nothing;
        }
        Orb& o = m_orbs[static_cast<std::size_t>(which)];
        o.held = true;
        o.resting = false;
        o.vy = 0.0f;
        return Action::Took;
    }

    /// Once per frame, after the body has moved. `solidsA` / `solidsB`: the solid table of each layer (the orbs fall in their
    /// own layer). The held orb follows the body and its layer.
    void Update(const Player& body, int bodyLayer, const scene::SceneObject* solidsA, int countA, const scene::SceneObject* solidsB,
                int countB, float dtSeconds)
    {
        if (!(dtSeconds > 0.0f) || !std::isfinite(dtSeconds))
        {
            return;
        }
        const float dt = dtSeconds > 0.1f ? 0.1f : dtSeconds;
        for (int i = 0; i < m_orbCount; ++i)
        {
            Orb& o = m_orbs[static_cast<std::size_t>(i)];
            if (o.held)
            {
                o.layer = bodyLayer;
                o.centre = HeldCentre(body);
                continue;
            }
            const scene::SceneObject* solids = o.layer == 0 ? solidsA : solidsB;
            const int count = o.layer == 0 ? countA : countB;
            if (o.resting && !Supported(o, solids, count))
            {
                o.resting = false; // what it lay on is gone (a bridge, a lid of the other layer does not count)
            }
            if (!o.resting)
            {
                Fall(i, solids, count, dt);
            }
        }
        for (int i = 0; i < m_triggerCount; ++i)
        {
            const bool was = m_active[static_cast<std::size_t>(i)];
            bool now = m_latched[static_cast<std::size_t>(i)] || Worked(i, body, bodyLayer);
            const Trigger& t = m_triggers[static_cast<std::size_t>(i)];
            if (now && (t.latch || t.kind == TriggerKind::Lever))
            {
                m_latched[static_cast<std::size_t>(i)] = true;
            }
            m_active[static_cast<std::size_t>(i)] = now;
            m_changed[static_cast<std::size_t>(i)] = now != was;
        }
    }

    bool Active(int trigger) const
    {
        return trigger >= 0 && trigger < m_triggerCount && m_active[static_cast<std::size_t>(trigger)];
    }

    /// True in the frame in which the trigger turned on or off (for the log).
    bool Changed(int trigger) const
    {
        return trigger >= 0 && trigger < m_triggerCount && m_changed[static_cast<std::size_t>(trigger)];
    }

    bool GateOpen(int gate) const
    {
        const Gate& g = m_gates[static_cast<std::size_t>(gate)];
        return Active(g.trigger) || Active(g.orTrigger);
    }

    bool GatePresent(int gate) const
    {
        const Gate& g = m_gates[static_cast<std::size_t>(gate)];
        return g.presentWhenOpen ? GateOpen(gate) : !GateOpen(gate);
    }

    int TriggerCount() const
    {
        return m_triggerCount;
    }

    int GateCount() const
    {
        return m_gateCount;
    }

    int OrbCount() const
    {
        return m_orbCount;
    }

    const Trigger& TriggerAt(int i) const
    {
        return m_triggers[static_cast<std::size_t>(i)];
    }

    const Gate& GateAt(int i) const
    {
        return m_gates[static_cast<std::size_t>(i)];
    }

    const Orb& OrbAt(int i) const
    {
        return m_orbs[static_cast<std::size_t>(i)];
    }

    /// The held orb, or -1.
    int HeldOrb() const
    {
        for (int i = 0; i < m_orbCount; ++i)
        {
            if (m_orbs[static_cast<std::size_t>(i)].held)
            {
                return i;
            }
        }
        return -1;
    }

    /// The orb E would take now (in reach, in the layer of the body, not behind a solid), or -1. `solids`: the solid table of
    /// the layer of the body.
    int OrbInReach(const Player& body, int bodyLayer, const scene::SceneObject* solids, int count) const
    {
        const Vec3 f = body.Feet();
        const Vec3 chest{f.x, f.y + 1.0f, f.z};
        int best = -1;
        float bestDistance = kReach;
        for (int i = 0; i < m_orbCount; ++i)
        {
            const Orb& o = m_orbs[static_cast<std::size_t>(i)];
            if (o.held || o.layer != bodyLayer || o.centre.y < f.y - 0.6f || o.centre.y > f.y + 2.0f)
            {
                continue;
            }
            const float dx = o.centre.x - f.x;
            const float dz = o.centre.z - f.z;
            const float d = std::sqrt(dx * dx + dz * dz);
            if (d < bestDistance && PathClear(chest, o.centre, 0.08f, bodyLayer, solids, count)) // not through a door or bars
            {
                best = i;
                bestDistance = d;
            }
        }
        return best;
    }

    /// Where a put-down now would go for a body in `layer`: the trigger it would be placed on (a socket or a plate of that
    /// layer), kSpotExtra for an extra spot (a hole, in either layer), or kSpotNone (one step ahead). `snapped` gets the
    /// place. `otherWorld` (when given) gets a socket or plate of the OTHER layer that would be in reach, or kSpotNone.
    static constexpr int kSpotNone = -1;
    static constexpr int kSpotExtra = -2;
    int SpotAhead(const Player& body, int layer, const Vec3& look, Vec3& snapped, int* otherWorld = nullptr) const
    {
        const Vec3 f = body.Feet();
        const float flat = std::sqrt(look.x * look.x + look.z * look.z);
        const float fx = flat > 1.0e-4f ? look.x / flat : 0.0f;
        const float fz = flat > 1.0e-4f ? look.z / flat : 1.0f;
        const Vec3 ahead{f.x + kPutAhead * fx, f.y + 0.6f, f.z + kPutAhead * fz};
        snapped = ahead;
        int which = kSpotNone;
        float best = kSnap; // the nearest spot on about the level of the feet, within kSnap
        const auto consider = [&](const Vec3& spot, int id) {
            const float dx = spot.x - ahead.x;
            const float dz = spot.z - ahead.z;
            const float d = std::sqrt(dx * dx + dz * dz);
            if (d < best && std::fabs(spot.y - f.y) < 0.5f)
            {
                best = d;
                snapped = {spot.x, ahead.y, spot.z};
                which = id;
            }
        };
        if (otherWorld != nullptr)
        {
            *otherWorld = kSpotNone;
        }
        for (int t = 0; t < m_triggerCount; ++t)
        {
            const Trigger& tr = m_triggers[static_cast<std::size_t>(t)];
            if (tr.kind == TriggerKind::Lever)
            {
                continue;
            }
            if (tr.layer == layer)
            {
                consider(tr.at, t);
            }
            else if (otherWorld != nullptr && *otherWorld == kSpotNone)
            {
                const float dx = tr.at.x - ahead.x;
                const float dz = tr.at.z - ahead.z;
                if (dx * dx + dz * dz < kSnap * kSnap && std::fabs(tr.at.y - f.y) < 0.5f)
                {
                    *otherWorld = t;
                }
            }
        }
        for (int s = 0; s < m_spotCount; ++s)
        {
            consider(m_spots[static_cast<std::size_t>(s)], kSpotExtra);
        }
        return which;
    }

    /// Adds to a DRAWN table a thin column of light over every socket and plate that is not worked and every lever that is
    /// not pulled (both layers; red for a trigger of A, blue for B, gold for a lever). Returns the new count.
    int AddBeacons(scene::SceneObject* out, int count, int capacity) const
    {
        for (int i = 0; i < m_triggerCount && out != nullptr && count >= 0 && count < capacity; ++i)
        {
            if (m_active[static_cast<std::size_t>(i)])
            {
                continue;
            }
            const Trigger& t = m_triggers[static_cast<std::size_t>(i)];
            const bool lever = t.kind == TriggerKind::Lever;
            const Colour w = WorldColour(t.layer);
            const float red = lever ? 1.0f : w.r;
            const float green = lever ? 0.82f : w.g;
            const float blue = lever ? 0.36f : w.b;
            const float bottom = lever ? t.at.y + 0.2f : t.at.y + 0.05f;
            scene::SceneObject o = Flat(t.at.x, bottom, t.at.z, kBeaconWidth, kBeaconHeight, red, green, blue, scene::kLayerBoth);
            o.shineStrength = 0.0f;
            o.glow = 1.6f;
            out[count++] = o;
        }
        return count;
    }

    struct Colour
    {
        float r;
        float g;
        float b;
    };

    /// The colour of a world: an orb, a pad and a column of light of layer A are warm red-gold, of layer B cold blue.
    static constexpr Colour WorldColour(int layer)
    {
        return layer == 0 ? Colour{1.00f, 0.50f, 0.28f} : Colour{0.35f, 0.65f, 1.00f};
    }

    static constexpr float kBeaconWidth = 0.06f;  ///< UNITS: metres
    static constexpr float kBeaconHeight = 2.4f; ///< UNITS: metres: seen over a body and over the low walls of a room

    /// Adds the gates that are there to a SOLID table of `layer`. Returns the new count.
    int AddSolids(int layer, scene::SceneObject* out, int count, int capacity) const
    {
        const unsigned bit = 1u << layer;
        for (int i = 0; i < m_gateCount && out != nullptr && count >= 0 && count < capacity; ++i)
        {
            const Gate& g = m_gates[static_cast<std::size_t>(i)];
            if (GatePresent(i) && (g.box.layerMask & bit) != 0u)
            {
                out[count++] = g.box;
            }
        }
        return count;
    }

    /// Adds to a DRAWN table the gates that are there and the pads of the triggers (a socket, a plate: flat, in their layer,
    /// brighter while active; a lever: in both layers, gold once pulled). Single-layer boxes among them can be outlined in the
    /// other layer by the caller (scene/StageHints.h). Returns the new count.
    int AddFixedDraw(scene::SceneObject* out, int count, int capacity) const
    {
        for (int i = 0; i < m_gateCount && out != nullptr && count >= 0 && count < capacity; ++i)
        {
            if (GatePresent(i))
            {
                out[count++] = m_gates[static_cast<std::size_t>(i)].box;
            }
        }
        for (int i = 0; i < m_triggerCount && out != nullptr && count >= 0 && count < capacity; ++i)
        {
            out[count++] = PadOf(i);
        }
        return count;
    }

    /// Adds to a DRAWN table the orbs (each in its own layer; the held one in front of the eye) and, under each orb that lies
    /// somewhere, a small flat mark in the other layer, so that an orb left in one layer is seen from the other. Returns the
    /// new count.
    int AddOrbDraw(const Player& body, const Vec3& look, scene::SceneObject* out, int count, int capacity) const
    {
        for (int i = 0; i < m_orbCount && out != nullptr && count >= 0 && count < capacity; ++i)
        {
            const Orb& o = m_orbs[static_cast<std::size_t>(i)];
            Vec3 c = o.centre;
            if (o.held)
            {
                c = ShownHeld(body, look);
            }
            const Colour w = WorldColour(o.layer);
            scene::SceneObject ball = Ball(c, kOrbRadius, w.r, w.g, w.b, 1u << o.layer, 1.4f);
            ball.glow = 0.8f; // an orb is a light: seen in a dark corner and from across a room
            out[count++] = ball;
            if (!o.held && count < capacity)
            {
                const unsigned other = o.layer == 0 ? scene::kLayerB : scene::kLayerA;
                const float y = c.y - kOrbRadius;
                out[count++] = Flat(c.x, y + 0.02f, c.z, 0.5f, 0.02f, w.r, w.g, w.b, other); // seen from the other world, in its own colour
            }
        }
        return count;
    }

  private:
    static constexpr float kFallGravity = 20.0f; // UNITS: m/s^2
    static constexpr float kFallMax = 15.0f;     // UNITS: m/s
    static constexpr float kFallStep = 0.1f;     // UNITS: metres: a fall is cut into steps this short (floors are 0.2 m)

    void PutBack(int i)
    {
        const OrbStart& s = m_starts[static_cast<std::size_t>(i)];
        Orb& o = m_orbs[static_cast<std::size_t>(i)];
        o.centre = s.centre;
        o.layer = s.layer;
        o.held = false;
        o.resting = false;
        o.vy = 0.0f;
    }

    static Vec3 HeldCentre(const Player& body)
    {
        const Vec3 f = body.Feet();
        return {f.x, f.y + 1.05f, f.z};
    }

    static Vec3 ShownHeld(const Player& body, const Vec3& look)
    {
        const Vec3 eye = body.Eye();
        const float flat = std::sqrt(look.x * look.x + look.z * look.z);
        const float fx = flat > 1.0e-4f ? look.x / flat : 0.0f;
        const float fz = flat > 1.0e-4f ? look.z / flat : 1.0f;
        return {eye.x + 0.6f * fx, eye.y - 0.45f, eye.z + 0.6f * fz};
    }

    static scene::SceneObject Ball(const Vec3& c, float r, float red, float green, float blue, unsigned mask, float spin)
    {
        scene::SceneObject o{};
        o.shape = scene::ShapeKind::Sphere;
        o.position[0] = c.x;
        o.position[1] = c.y;
        o.position[2] = c.z;
        o.scale[0] = 2.0f * r;
        o.scale[1] = 2.0f * r;
        o.scale[2] = 2.0f * r;
        o.spinSpeed = spin;
        o.colorSrgb[0] = red;
        o.colorSrgb[1] = green;
        o.colorSrgb[2] = blue;
        o.shineStrength = 1.0f;
        o.shininess = 120.0f;
        o.layerMask = mask;
        return o;
    }

    /// A flat box centred at (x, z) whose BOTTOM is at y.
    static scene::SceneObject Flat(float x, float y, float z, float size, float height, float red, float green, float blue, unsigned mask)
    {
        scene::SceneObject o{};
        o.shape = scene::ShapeKind::Cube;
        o.position[0] = x;
        o.position[1] = y + 0.5f * height;
        o.position[2] = z;
        o.scale[0] = size;
        o.scale[1] = height;
        o.scale[2] = size;
        o.colorSrgb[0] = red;
        o.colorSrgb[1] = green;
        o.colorSrgb[2] = blue;
        o.shineStrength = 0.2f;
        o.shininess = 30.0f;
        o.layerMask = mask;
        return o;
    }

    scene::SceneObject PadOf(int i) const
    {
        const Trigger& t = m_triggers[static_cast<std::size_t>(i)];
        const bool on = m_active[static_cast<std::size_t>(i)];
        if (t.kind == TriggerKind::Lever)
        {
            scene::SceneObject o = Flat(t.at.x, t.at.y - 0.2f, t.at.z, 0.4f, 0.4f, on ? 1.0f : 0.45f, on ? 0.82f : 0.45f, on ? 0.36f : 0.50f,
                                        scene::kLayerBoth);
            o.shineStrength = 0.8f;
            o.glow = on ? 0.6f : 0.4f;
            return o;
        }
        const float size = t.kind == TriggerKind::Socket ? 0.7f : 0.9f;
        const float k = on ? 1.0f : 0.0f;
        const float red = t.layer == 0 ? 0.70f + 0.30f * k : 0.15f + 0.45f * k;
        const float green = t.layer == 0 ? 0.18f + 0.40f * k : 0.35f + 0.45f * k;
        const float blue = t.layer == 0 ? 0.16f + 0.30f * k : 0.80f + 0.20f * k;
        scene::SceneObject pad = Flat(t.at.x, t.at.y, t.at.z, size, 0.04f, red, green, blue, 1u << t.layer);
        pad.glow = on ? 0.8f : 0.25f;
        return pad;
    }

    /// True when the trigger is worked right now (before latching).
    bool Worked(int i, const Player& body, int bodyLayer) const
    {
        const Trigger& t = m_triggers[static_cast<std::size_t>(i)];
        if (t.kind == TriggerKind::Lever)
        {
            for (int b = 0; b < 3; ++b)
            {
                const Vec3 p = body.Ball(b);
                const float dx = p.x - t.at.x;
                const float dy = p.y - t.at.y;
                const float dz = p.z - t.at.z;
                if (dx * dx + dy * dy + dz * dz < kLeverReach * kLeverReach)
                {
                    return true;
                }
            }
            return false;
        }
        for (int k = 0; k < m_orbCount; ++k)
        {
            const Orb& o = m_orbs[static_cast<std::size_t>(k)];
            if (!o.held && o.resting && o.layer == t.layer && std::fabs(o.centre.x - t.at.x) < kRestTolerance &&
                std::fabs(o.centre.z - t.at.z) < kRestTolerance && std::fabs(o.centre.y - kOrbRadius - t.at.y) < 0.2f)
            {
                return true;
            }
        }
        if (t.kind == TriggerKind::Plate && bodyLayer == t.layer && body.Grounded())
        {
            const Vec3 f = body.Feet();
            return std::fabs(f.x - t.at.x) < 0.45f && std::fabs(f.z - t.at.z) < 0.45f && std::fabs(f.y - t.at.y) < 0.15f;
        }
        return false;
    }

    /// True when the sphere of the orb is pushed up by something just under it in its layer.
    static bool Supported(const Orb& o, const scene::SceneObject* solids, int count)
    {
        const Vec3 probe{o.centre.x, o.centre.y - 0.03f, o.centre.z};
        const CollisionResult r = ResolveSphere(probe, kOrbRadius, o.layer, 0.0f, solids, count);
        return r.pushed && r.position.y - probe.y > 0.005f;
    }

    void Fall(int i, const scene::SceneObject* solids, int count, float dt)
    {
        Orb& o = m_orbs[static_cast<std::size_t>(i)];
        o.vy -= kFallGravity * dt;
        o.vy = o.vy < -kFallMax ? -kFallMax : o.vy;
        float drop = -o.vy * dt;
        while (drop > 0.0f)
        {
            const float step = drop < kFallStep ? drop : kFallStep;
            drop -= step;
            const Vec3 moved{o.centre.x, o.centre.y - step, o.centre.z};
            const CollisionResult r = ResolveSphere(moved, kOrbRadius, o.layer, 0.0f, solids, count);
            o.centre = r.position;
            if (r.pushed && r.position.y - moved.y > 0.001f)
            {
                o.resting = true;
                o.vy = 0.0f;
                return;
            }
            if (o.centre.y < m_fallY)
            {
                PutBack(i); // fell out of the map: back to its start
                return;
            }
        }
    }

    /// True when a sphere of `radius` moved in a straight line from `a` to `b` meets no solid of `layer`.
    static bool PathClear(const Vec3& a, const Vec3& b, float radius, int layer, const scene::SceneObject* solids, int count)
    {
        const int samples = 10;
        for (int s = 0; s <= samples; ++s)
        {
            const float t = static_cast<float>(s) / static_cast<float>(samples);
            const Vec3 p{a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t};
            const CollisionResult r = ResolveSphere(p, radius, layer, 0.0f, solids, count);
            if (r.pushed && std::fabs(r.position.x - p.x) + std::fabs(r.position.y - p.y) + std::fabs(r.position.z - p.z) > 0.005f)
            {
                return false;
            }
        }
        return true;
    }

    bool PutDown(int i, const Player& body, int bodyLayer, const Vec3& look, const scene::SceneObject* solids, int count)
    {
        const Vec3 f = body.Feet();
        Vec3 target{};
        SpotAhead(body, bodyLayer, look, target);
        const Vec3 start{f.x, f.y + 0.6f, f.z};
        if (!PathClear(start, target, kOrbRadius, bodyLayer, solids, count))
        {
            return false;
        }
        Orb& o = m_orbs[static_cast<std::size_t>(i)];
        o.held = false;
        o.resting = false;
        o.vy = 0.0f;
        o.layer = bodyLayer;
        o.centre = target;
        return true;
    }

    std::array<Trigger, kMaxTriggers> m_triggers{};
    std::array<Gate, kMaxGates> m_gates{};
    std::array<OrbStart, kMaxOrbs> m_starts{};
    std::array<Orb, kMaxOrbs> m_orbs{};
    std::array<Vec3, kMaxTriggers> m_spots{};
    std::array<bool, kMaxTriggers> m_active{};
    std::array<bool, kMaxTriggers> m_latched{};
    std::array<bool, kMaxTriggers> m_changed{};
    int m_triggerCount = 0;
    int m_gateCount = 0;
    int m_orbCount = 0;
    int m_spotCount = 0;
    float m_fallY = -100.0f;
};

} // namespace dl::mech

