// Anchors.h - the anchor lamps (docs/LEVEL_DESIGN.md, 14.5): where a body comes back after a fall or R.
// Rule:      a lamp is lit the first time a ball of the body comes within kAnchorLightReach of its pedestal, in either layer,
//            and stays lit for the run. A fall or R in a room with a lit lamp brings the body back beside that lamp
//            (AnchorDesc::stand); in a room without one, to the entry of the room (marked by a carved tile, Stage::EntryTiles).
// Look:      the pedestal is a box of the map (Stage::AnchorPedestal, solid, the same in both layers); the lamp on it is drawn
//            here: dim until lit, then a warm light. A respawn shows a ring that spreads from where the body comes back
//            (AddRespawnRing), the same in both layers.
// Plain data and plain logic, no Windows, no Direct3D: tested without a window (tests/test_stage.cpp, TestAnchors).
// UNITS: metres, seconds.
#pragma once

#include "scene/Camera.h" // Vec3
#include "scene/SceneData.h"
#include "scene/Stage.h"

namespace dl::stage
{

inline constexpr float kAnchorLightReach = 2.5f; ///< UNITS: metres between a ball of the body and the top of the pedestal
inline constexpr float kRespawnRingSeconds = 0.8f;

class AnchorSet
{
  public:
    /// Lights every lamp a ball of `body` comes near. `Body` needs Ball(i) (scene/Player.h). Returns a lamp lit now, or -1.
    template <class Body>
    int Update(const Body& body)
    {
        int now = -1;
        for (int i = 0; i < kAnchorCount; ++i)
        {
            if (m_lit[i])
            {
                continue;
            }
            const Vec3 top{kAnchors[i].base.x, kAnchors[i].base.y + kAnchorHeight, kAnchors[i].base.z};
            for (int b = 0; b < 3; ++b)
            {
                const Vec3 p = body.Ball(b);
                const float dx = p.x - top.x;
                const float dy = p.y - top.y;
                const float dz = p.z - top.z;
                if (dx * dx + dy * dy + dz * dz < kAnchorLightReach * kAnchorLightReach)
                {
                    m_lit[i] = true;
                    now = now < 0 ? i : now;
                    break;
                }
            }
        }
        return now;
    }

    bool Lit(int i) const
    {
        return i >= 0 && i < kAnchorCount && m_lit[i];
    }

    /// The lamp of `room`, or -1.
    static int AnchorOf(int room)
    {
        for (int i = 0; i < kAnchorCount; ++i)
        {
            if (RoomIndex(kAnchors[i].room) == room)
            {
                return i;
            }
        }
        return -1;
    }

    /// Where a body of `room` comes back: beside the lit lamp of the room, else at its entry.
    Vec3 RespawnPoint(int room) const
    {
        if (room < 0 || room >= kRoomCount)
        {
            return kRooms[kStartRoom].entry;
        }
        const int a = AnchorOf(room);
        return Lit(a) ? kAnchors[a].stand : kRooms[room].entry;
    }

    /// Adds the lamps (both layers, on their pedestals) to a DRAWN table. Returns the new count.
    int AddDraw(SceneObject* out, int count, int capacity) const
    {
        for (int i = 0; i < kAnchorCount && out != nullptr && count >= 0 && count < capacity; ++i)
        {
            const Vec3 b = kAnchors[i].base;
            const float y = b.y + kAnchorHeight;
            SceneObject lamp = Span(b.x - 0.16f, y, b.z - 0.16f, b.x + 0.16f, y + 0.32f, b.z + 0.16f, m_lit[i] ? Rgb{1.0f, 0.86f, 0.55f}
                                                                                                         : Rgb{0.55f, 0.52f, 0.46f},
                                    kLayerBoth);
            lamp.glow = m_lit[i] ? 1.3f : 0.1f;
            out[count++] = lamp;
        }
        return count;
    }

  private:
    bool m_lit[kAnchorCount] = {};
};

/// Adds, for kRespawnRingSeconds after a respawn (`since` >= 0), a ring that spreads on the floor from `at`. Returns the new count.
inline int AddRespawnRing(const Vec3& at, float since, SceneObject* out, int count, int capacity)
{
    if (out == nullptr || count < 0 || count >= capacity || since < 0.0f || since >= kRespawnRingSeconds)
    {
        return count;
    }
    const float t = since / kRespawnRingSeconds;
    SceneObject o{};
    o.shape = ShapeKind::Torus;
    o.position[0] = at.x;
    o.position[1] = at.y + 0.1f;
    o.position[2] = at.z;
    o.scale[0] = 0.6f + 4.0f * t;
    o.scale[1] = 0.15f;
    o.scale[2] = 0.6f + 4.0f * t;
    o.colorSrgb[0] = 0.85f;
    o.colorSrgb[1] = 0.80f;
    o.colorSrgb[2] = 1.00f;
    o.glow = 1.2f * (1.0f - t);
    o.layerMask = kLayerBoth;
    out[count++] = o;
    return count;
}

} // namespace dl::stage
