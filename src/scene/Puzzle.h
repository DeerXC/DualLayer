// Puzzle.h - the puzzle pieces of the stage: a purple block that exists in BOTH layers and can be pushed, a red plate that
//            is pressed in layer A and opens a blue door in layer B, and the guard that refuses a layer switch that would put
//            the body inside something.
// Plain logic, no Windows, no Direct3D: tested without a window (tests/test_puzzle.cpp, tests/test_stage.cpp).
// Block:     a 1 m cube on the floor, on a 1 m grid (cell (x, z) has its centre at world (x, z)). It is solid in both layers,
//            so a push in one layer moves it in both. The first push needs kPushSeconds of walking straight at one face
//            (within about 45 degrees) from the floor; after that, as long as the body keeps walking that way, the block
//            goes on cell after cell without a stop. It slides at kSlideSpeed, a little slower than the walk, so the body
//            stays at its face and is never left behind. The next cell must be free in BOTH layers (no solid of A, no solid
//            of B, not the door, not outside the allowed cells). A refused push names what was in the way (LastBlocker), so
//            the renderer can make it flash: the thing in the way may be in the layer that is not on the screen.
// Plate:     in layer A, flat on the floor, not solid. It is pressed by the block resting on it, or by the body standing on it
//            while the body is in layer A. A body in layer B does not press it.
// Door:      in layer B, solid while closed. It is open while the plate is pressed. The question "is the door open?" depends
//            on the layer the body is in (a body that switches to B leaves the plate), so every query takes that layer.
// Solids:    Solids(layer) builds the table that the collision and the walking body use for a body in that layer: the fixed
//            objects of the stage, the block, and the door when it is closed. The table lives in the Puzzle; a pointer to it
//            stays valid until the next call.
// Guard:     BodyFits says whether the body would be free in a layer. LayerGuard turns a switch request into Switch, Refuse
//            (the ripple opens a little and turns back; the solid layer stays the old one) or Ignore.
// Space:     left-handed, +Y up, the floor of the scene at scene::kFloorY. UNITS: metres, seconds.
// Note:      no std::min / std::max here: windows.h may define them as macros (docs/CONVENTIONS.md).
#pragma once

#include "scene/Camera.h" // Vec3
#include "scene/Collision.h"
#include "scene/Player.h"
#include "scene/SceneData.h"

#include <array>
#include <cmath>
#include <cstddef>

namespace dl::puzzle
{

struct Cell
{
    int x = 0;
    int z = 0;
};

inline bool SameCell(const Cell& a, const Cell& b)
{
    return a.x == b.x && a.z == b.z;
}

/// What a stage brings: its fixed objects and where the puzzle pieces are.
struct StageDesc
{
    const scene::SceneObject* statics = nullptr; ///< BORROWED only during Init: the table is copied
    int staticCount = 0;
    Cell blockStart;         ///< where the purple block starts
    Cell plate;              ///< the red plate (layer A)
    scene::SceneObject door; ///< the blue door: its shape and place (its layer mask is set to B by Init)
    Cell minCell;            ///< the block stays inside these cells (inclusive)
    Cell maxCell;
};

/// LastBlocker: what stopped the last push.
inline constexpr int kBlockerNone = -3;
inline constexpr int kBlockerDoor = -2;
inline constexpr int kBlockerBounds = -1; ///< (0 and above: the index of a fixed object of the stage)

class Puzzle
{
  public:
    static constexpr int kMaxSolids = 160;          ///< fixed objects + the block + the door
    static constexpr float kPushSeconds = 0.15f;   ///< UNITS: seconds of walking against the block before the FIRST move
    static constexpr float kSlideSpeed = 3.6f;     ///< UNITS: metres per second; the walk is 4, so the body keeps contact
    static constexpr float kFlashSeconds = 0.6f;   ///< UNITS: seconds that a refused push shows what was in the way
    static constexpr float kBlockSize = 1.0f;      ///< UNITS: metres
    static constexpr float kFacing = 0.7f;         ///< the walk must point at the face by at least this part (about 45 degrees)

    /// Copies the stage. False (and the puzzle stays empty) when the stage has too many objects or makes no sense.
    bool Init(const StageDesc& desc)
    {
        m_staticCount = 0;
        if (desc.statics == nullptr || desc.staticCount <= 0 || desc.staticCount > kMaxSolids - 2 ||
            desc.minCell.x > desc.maxCell.x || desc.minCell.z > desc.maxCell.z || !Inside(desc.blockStart, desc))
        {
            return false;
        }
        for (int i = 0; i < desc.staticCount; ++i)
        {
            m_statics[static_cast<std::size_t>(i)] = desc.statics[i];
        }
        m_staticCount = desc.staticCount;
        m_desc = desc;
        m_desc.statics = nullptr; // never read again: the copy is used
        m_door = desc.door;
        m_door.layerMask = scene::kLayerB;
        Reset();
        return true;
    }

    /// The block back at its start, nothing pressed, nothing moving.
    void Reset()
    {
        m_cell = m_desc.blockStart;
        m_target = m_cell;
        m_slide = 0.0f;
        m_moving = false;
        m_pushTime = 0.0f;
        m_pushAxis = {};
        m_bodyOnPlate = false;
        m_flash = 0.0f;
        m_blocker = kBlockerNone;
        m_moves = 0;
    }

    /// Call once per frame after the body has moved (a push works the same in both layers: the block is in both).
    void Update(const Player& player, float dtSeconds)
    {
        if (m_staticCount == 0 || !(dtSeconds > 0.0f) || !std::isfinite(dtSeconds))
        {
            return;
        }
        const float dt = dtSeconds > 0.1f ? 0.1f : dtSeconds;
        m_flash = m_flash > dt ? m_flash - dt : 0.0f;

        // ---- the plate: the body stands on it, on the floor (whether that counts depends on the layer: see PlatePressed)
        const Vec3 feet = player.Feet();
        m_bodyOnPlate = player.Grounded() && std::fabs(feet.x - CellX(m_desc.plate)) < 0.5f &&
                        std::fabs(feet.z - CellZ(m_desc.plate)) < 0.5f && feet.y < scene::kFloorY + 0.1f;

        // ---- a move that is under way
        if (m_moving)
        {
            m_slide += dt * kSlideSpeed / kBlockSize;
            if (m_slide < 1.0f)
            {
                return;
            }
            m_cell = m_target;
            m_slide = 0.0f;
            m_moving = false;
            ++m_moves;
            // the body still walks the same way, a little behind the face: go on without a stop
            Cell axis{};
            if (Follows(player, axis) && axis.x == m_pushAxis.x && axis.z == m_pushAxis.z && TryMove(axis))
            {
                return;
            }
            m_pushTime = 0.0f;
            return;
        }

        // ---- the first push: the body on the floor, next to a face of the block, walking against it for a moment
        Cell axis{};
        if (!PushesAgainst(player, axis))
        {
            m_pushTime = 0.0f;
            return;
        }
        if (axis.x != m_pushAxis.x || axis.z != m_pushAxis.z)
        {
            m_pushTime = 0.0f; // another face: start counting again
            m_pushAxis = axis;
        }
        m_pushTime += dt;
        if (m_pushTime < kPushSeconds)
        {
            return;
        }
        m_pushTime = 0.0f;
        TryMove(axis);
    }

    /// The centre of the block in the world (between two cells while it moves).
    Vec3 BlockCentre() const
    {
        float x = CellX(m_cell);
        float z = CellZ(m_cell);
        if (m_moving)
        {
            x += (CellX(m_target) - x) * m_slide; // linear: the body walks at a constant speed behind it
            z += (CellZ(m_target) - z) * m_slide;
        }
        return {x, scene::kFloorY + 0.5f * kBlockSize, z};
    }

    Cell BlockCell() const
    {
        return m_cell;
    }

    bool BlockMoving() const
    {
        return m_moving;
    }

    /// How many moves the block has finished since Reset.
    int Moves() const
    {
        return m_moves;
    }

    /// True when the plate is pressed for a body in `layer`: by the block resting on it, or by the body standing on it in A.
    bool PlatePressed(int layer) const
    {
        const bool blockOnPlate = !m_moving && SameCell(m_cell, m_desc.plate);
        return blockOnPlate || (layer == 0 && m_bodyOnPlate);
    }

    /// The blue door is open while the plate is pressed (for a body in `layer`).
    bool DoorOpen(int layer) const
    {
        return PlatePressed(layer);
    }

    /// What stopped the last refused push (kBlockerNone, kBlockerDoor, kBlockerBounds, or the index of a fixed object), and
    /// how many seconds of its flash are left (0 = no flash).
    int LastBlocker() const
    {
        return m_blocker;
    }

    float BlockedFlash() const
    {
        return m_flash;
    }

    /// The solid table for a body in `layer`: the fixed objects, the block (both layers) and the door while it is closed.
    /// The pointer stays valid until the next call.
    const scene::SceneObject* Solids(int layer, int& count)
    {
        int n = 0;
        for (int i = 0; i < m_staticCount; ++i)
        {
            m_solids[static_cast<std::size_t>(n++)] = m_statics[static_cast<std::size_t>(i)];
        }
        m_solids[static_cast<std::size_t>(n++)] = BlockObject();
        if (!DoorOpen(layer))
        {
            m_solids[static_cast<std::size_t>(n++)] = m_door;
        }
        count = n;
        return m_solids.data();
    }

    /// The block as an object of the scene (for the renderer and the collision): a purple cube in both layers.
    scene::SceneObject BlockObject() const
    {
        scene::SceneObject o{};
        o.shape = scene::ShapeKind::Cube;
        const Vec3 c = BlockCentre();
        o.position[0] = c.x;
        o.position[1] = c.y;
        o.position[2] = c.z;
        o.scale[0] = kBlockSize;
        o.scale[1] = kBlockSize;
        o.scale[2] = kBlockSize;
        o.colorSrgb[0] = 0.62f;
        o.colorSrgb[1] = 0.32f;
        o.colorSrgb[2] = 0.86f;
        o.shineStrength = 0.30f;
        o.shininess = 40.0f;
        o.layerMask = scene::kLayerBoth;
        return o;
    }

    /// The door as an object of the scene (layer B).
    const scene::SceneObject& DoorObject() const
    {
        return m_door;
    }

    /// True when none of the three balls of the body would overlap a solid of `layer` (with the door as it would be for a
    /// body in that layer). A switch to a layer in which the body does not fit is refused, not pushed through.
    bool BodyFits(const Player& player, int layer, float spinSeconds)
    {
        int count = 0;
        const scene::SceneObject* solids = Solids(layer, count);
        const float radius = player.GetSettings().radius;
        for (int i = 0; i < 3; ++i)
        {
            const Vec3 c = player.Ball(i);
            const CollisionResult r = ResolveSphere(c, radius, layer, spinSeconds, solids, count);
            const float dx = r.position.x - c.x;
            const float dy = r.position.y - c.y;
            const float dz = r.position.z - c.z;
            if (std::sqrt(dx * dx + dy * dy + dz * dz) > kFitTolerance)
            {
                return false;
            }
        }
        return true;
    }

  private:
    static constexpr float kFitTolerance = 0.01f; // UNITS: metres a ball may be pushed and still count as fitting
    static constexpr float kContactGap = 0.06f;  // UNITS: metres between the body and the face that still count as touching
    static constexpr float kFollowGap = 0.5f;    // UNITS: metres; while the block slides, a body this close behind follows
    static constexpr float kProbeRadius = 0.24f; // UNITS: metres; with the offsets in WhatBlocks the probes reach 0.49 m

    static float CellX(const Cell& c)
    {
        return static_cast<float>(c.x);
    }

    static float CellZ(const Cell& c)
    {
        return static_cast<float>(c.z);
    }

    static bool Inside(const Cell& c, const StageDesc& d)
    {
        return c.x >= d.minCell.x && c.x <= d.maxCell.x && c.z >= d.minCell.z && c.z <= d.maxCell.z;
    }

    /// Starts a move along `axis` when the next cell is free; otherwise records what was in the way. True when it started.
    bool TryMove(const Cell& axis)
    {
        const Cell next{m_cell.x + axis.x, m_cell.z + axis.z};
        const int blocker = WhatBlocks(next);
        if (blocker != kBlockerNone)
        {
            m_blocker = blocker;
            m_flash = kFlashSeconds;
            return false;
        }
        m_pushAxis = axis;
        m_target = next;
        m_moving = true;
        m_slide = 0.0f;
        return true;
    }

    /// The face of the block the body is at, as the way the block would move, and whether the walk points along it.
    /// `gap`: how far behind the face the body may be.
    bool AtFace(const Player& player, float gap, Cell& axis) const
    {
        if (!player.Grounded())
        {
            return false;
        }
        const Vec3 feet = player.Feet();
        if (feet.y > scene::kFloorY + 0.1f)
        {
            return false; // standing on top of the block (or on anything above the floor) is not pushing
        }
        const Vec3 wish = player.WishDirection();
        const Vec3 c = BlockCentre();
        const float dx = c.x - feet.x;
        const float dz = c.z - feet.z;
        const bool alongX = std::fabs(dx) >= std::fabs(dz);
        const float along = alongX ? std::fabs(dx) : std::fabs(dz);
        const float across = alongX ? std::fabs(dz) : std::fabs(dx);
        const float half = 0.5f * kBlockSize;
        const float radius = player.GetSettings().radius;
        if (along - half - radius > gap || across > half - 0.05f)
        {
            return false; // not at a face, or at a corner
        }
        axis = alongX ? Cell{dx > 0.0f ? 1 : -1, 0} : Cell{0, dz > 0.0f ? 1 : -1};
        const float facing = wish.x * static_cast<float>(axis.x) + wish.z * static_cast<float>(axis.z);
        return facing > kFacing;
    }

    bool PushesAgainst(const Player& player, Cell& axis) const
    {
        return AtFace(player, kContactGap, axis);
    }

    bool Follows(const Player& player, Cell& axis) const
    {
        return AtFace(player, kFollowGap, axis);
    }

    /// What is in the way of the block in `cell`: kBlockerNone when the cell is free in both layers.
    int WhatBlocks(const Cell& cell) const
    {
        if (!Inside(cell, m_desc))
        {
            return kBlockerBounds;
        }
        // a few balls fill the cell (two heights, the middle and four points toward the corners), just inside the block;
        // every fixed object of EITHER layer, and the door whether it is open or not, must be clear of all of them
        const float x = CellX(cell);
        const float z = CellZ(cell);
        const float heights[2] = {scene::kFloorY + 0.30f, scene::kFloorY + 0.75f};
        const float offsets[5][2] = {{0.0f, 0.0f}, {0.25f, 0.25f}, {0.25f, -0.25f}, {-0.25f, 0.25f}, {-0.25f, -0.25f}};
        for (int i = 0; i <= m_staticCount; ++i)
        {
            const scene::SceneObject& o = i < m_staticCount ? m_statics[static_cast<std::size_t>(i)] : m_door;
            for (float y : heights)
            {
                for (const auto& off : offsets)
                {
                    Vec3 probe{x + off[0], y, z + off[1]};
                    if (collision_detail::PushOutOf(o, probe, kProbeRadius, 0.0f) > 1.0e-4f)
                    {
                        return i < m_staticCount ? i : kBlockerDoor;
                    }
                }
            }
        }
        return kBlockerNone;
    }

    std::array<scene::SceneObject, kMaxSolids> m_statics{};
    std::array<scene::SceneObject, kMaxSolids> m_solids{};
    int m_staticCount = 0;
    StageDesc m_desc{};
    scene::SceneObject m_door{};
    Cell m_cell{};
    Cell m_target{};
    Cell m_pushAxis{};
    float m_slide = 0.0f;
    float m_pushTime = 0.0f;
    float m_flash = 0.0f;
    int m_blocker = kBlockerNone;
    int m_moves = 0;
    bool m_moving = false;
    bool m_bodyOnPlate = false;
};

/// Turns a request to switch the layer into what the program should do. A switch into a place where the body does not fit
/// is not done: the ripple opens a little and turns back (Refuse), and while it does, the old layer stays the solid one.
class LayerGuard
{
  public:
    enum class Action
    {
        Switch, ///< switch as usual (ToggleLayer)
        Refuse, ///< start the ripple (ToggleLayer) and turn it back when Update returns true; keep HeldLayer solid meanwhile
        Ignore  ///< do nothing
    };

    static constexpr float kRefuseSeconds = 0.15f; ///< UNITS: seconds before a refused ripple turns back

    /// currentLayer: the layer that is solid now. rippleActive: a ripple is running. fits: the body fits in the layer the
    /// switch would go to (Puzzle::BodyFits).
    Action OnSwitchRequest(int currentLayer, bool rippleActive, bool fits)
    {
        if (m_refuse > 0.0f)
        {
            return Action::Ignore; // a refusal is playing
        }
        if (fits)
        {
            return Action::Switch;
        }
        if (rippleActive)
        {
            return Action::Ignore; // no refusal on top of a ripple: the request is dropped
        }
        m_refuse = kRefuseSeconds;
        m_held = currentLayer;
        ++m_refusals;
        return Action::Refuse;
    }

    /// True once, at the moment the refused ripple has to be turned back.
    bool Update(float dtSeconds)
    {
        if (m_refuse <= 0.0f || !(dtSeconds > 0.0f) || !std::isfinite(dtSeconds))
        {
            return false;
        }
        m_refuse -= dtSeconds;
        if (m_refuse <= 0.0f)
        {
            m_refuse = 0.0f;
            return true;
        }
        return false;
    }

    bool Refusing() const
    {
        return m_refuse > 0.0f;
    }

    /// The layer that stays solid while a refusal plays.
    int HeldLayer() const
    {
        return m_held;
    }

    /// How many switches were refused (for the log and the tests).
    int Refusals() const
    {
        return m_refusals;
    }

  private:
    float m_refuse = 0.0f;
    int m_held = 0;
    int m_refusals = 0;
};

} // namespace dl::puzzle
