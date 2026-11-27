// Player.h - a walking body for the camera: it walks, sprints, falls, lands, jumps and stands on the solid of the town.
// Plain maths, no Windows, no Direct3D: tested without a window (tests/test_player.cpp).
// Body:      three balls of the collision radius, one above the other: at the feet, at the eye (the camera) and one
//            halfway, so that the body cannot slip between thin parts. The camera is the eye; the feet are the point on
//            the ground.
// Solid:     SetTown gives the body the solid of the town of the model in both moments, with the circle of the lantern
//            (scene/Moments.h). The moment is a parameter of every Update, so a body standing on something that is not
//            in the new moment falls at once. Without a town nothing is solid.
// Feel:      the horizontal speed follows the keys with an acceleration: quick on the ground (full speed in about 0.06
//            s, a stop in about 0.05 s), slower in the air, so a jump commits the body but can still be steered.
//            Gravity is stronger while the body falls than while it rises, so a jump goes up softly and comes down
//            firmly. Letting the jump key go while the body still rises cuts the rise (a short tap is a small hop, a
//            held key the full jump height). Coyote time: a jump still works for 0.08 s after the body left an edge;
//            jump buffer: a jump pressed 0.1 s before the landing happens at the landing.
// Method:    per small step: the speed on the ground moves toward the wish of the keys; gravity; jump; move; push the
//            three balls out of the solid. The direction of the push is the normal of what was hit: the part of the
//            speed that goes into it is removed and the rest stays, so a floor ends a fall and a slope that is too
//            steep turns the fall into a slide. The walk is taken away only by pushes that are not ground
//            (AddWallPush): a ramp keeps the walk, a wall takes the part into it, also when the floor pushes in the
//            same step. Then a probe a few centimetres below the feet tells whether the body stands on something (the
//            surface must face up: a slope limit of about 53 degrees).
// Snap:      a body that stood in the last step and did not jump is kept on the ground when the ground drops away by at
//            most kSnapDown in one step (walking down a ramp or off a low step), so that it does not fly off in small
//            hops: that was the shaking of the camera on the way down.
// Time:      Update takes the real frame time, limited to 0.1 s, and cuts it into steps that move the body by less than
//            the radius, so that nothing is jumped over.
// Falling:   a body that falls below kFallBelow goes back to the spawn point (JustRespawned tells it).
// Space:     left-handed, +Y up. With the forward direction f on the ground, right = (f.z, 0, -f.x).
// UNITS:     metres, seconds, metres per second.
// Note:      no std::min / std::max here: windows.h may define them as macros (docs/CONVENTIONS.md).
#pragma once

#include "scene/Camera.h" // Vec3
#include "scene/Moments.h"

#include <algorithm>
#include <cmath>

namespace dl
{

class Player
{
  public:
    struct Settings
    {
        float radius = 0.35f;          ///< UNITS: metres, the radius of the three balls
        float eyeHeight = 1.55f;       ///< UNITS: metres, from the feet to the eye (the camera)
        float sprintMultiplier = 1.6f; ///< ground speed multiplier when sprint is held
        float walkSpeed = 5.5f;        ///< UNITS: metres per second
        float groundAccel = 95.0f;     ///< UNITS: m/s^2 toward the wished speed on the ground (5.5 m/s in about 0.06 s)
        float groundDecel = 120.0f;    ///< UNITS: m/s^2 when the keys are let go or point back (a stop in about 0.05 s)
        float airAccel = 40.0f; ///< UNITS: m/s^2 toward the wished speed in the air (steering, not turning on the spot)
        float gravity = 30.0f;  ///< UNITS: m/s^2 while rising (a snappy jump, not the real 9.8)
        float fallGravityScale = 1.6f; ///< gravity is this many times stronger while the body falls
        float jumpHeight = 2.0f;       ///< UNITS: metres that the feet rise in a full jump
        float jumpCut =
            0.45f; ///< the part of the upward speed that is kept when the jump key is let go early (1 = no cut)
    };

    struct Input
    {
        float moveForward = 0.0f; ///< -1 .. +1
        float moveRight = 0.0f;   ///< -1 .. +1
        bool jumpPressed = false; ///< true for the frame in which the key went down
        bool sprint = false;      ///< optional, enabled by the town runtime only
        bool jumpHeld =
            true; ///< false while the jump key is up; a caller that does not track the key gets the full jump
    };

    static constexpr float kGroundFacing =
        0.6f; ///< the push of the ground points up by at least this part (about 53 degrees)
    static constexpr float kSnapDown =
        0.35f; ///< UNITS: metres the ground may drop away in one step and still hold the body
    static constexpr float kSnapStep = 0.025f; ///< UNITS: metres; the snap looks for the ground in steps this small

    /// Values that make no sense are pulled back into a range that works (NaN gets the default).
    void SetSettings(const Settings& settings)
    {
        const Settings defaults{};
        m_s.radius = Clamp(Finite(settings.radius, defaults.radius), 0.05f, 1.0f);
        m_s.eyeHeight = Clamp(Finite(settings.eyeHeight, defaults.eyeHeight), m_s.radius + 0.1f, 3.0f);
        m_s.sprintMultiplier = Clamp(Finite(settings.sprintMultiplier, defaults.sprintMultiplier), 1.0f, 2.0f);
        m_s.walkSpeed = Clamp(Finite(settings.walkSpeed, defaults.walkSpeed), 0.0f, 20.0f);
        m_s.groundAccel = Clamp(Finite(settings.groundAccel, defaults.groundAccel), 1.0f, 1000.0f);
        m_s.groundDecel = Clamp(Finite(settings.groundDecel, defaults.groundDecel), 1.0f, 1000.0f);
        m_s.airAccel = Clamp(Finite(settings.airAccel, defaults.airAccel), 0.0f, 1000.0f);
        m_s.gravity = Clamp(Finite(settings.gravity, defaults.gravity), 1.0f, 100.0f);
        m_s.fallGravityScale = Clamp(Finite(settings.fallGravityScale, defaults.fallGravityScale), 1.0f, 4.0f);
        m_s.jumpHeight = Clamp(Finite(settings.jumpHeight, defaults.jumpHeight), 0.0f, 5.0f);
        m_s.jumpCut = Clamp(Finite(settings.jumpCut, defaults.jumpCut), 0.0f, 1.0f);
    }

    const Settings& GetSettings() const
    {
        return m_s;
    }

    /// The town of the model in both moments, with the circle of the lantern (BORROWS; null: none, the default).
    void SetTown(const town::Moments* moments)
    {
        m_town = moments;
    }

    /// Where the body starts and where it goes back to after a fall (the point on the ground).
    void SetSpawn(const Vec3& feet)
    {
        if (std::isfinite(feet.x) && std::isfinite(feet.y) && std::isfinite(feet.z))
        {
            m_spawn = feet;
        }
    }

    void Respawn()
    {
        m_feet = m_spawn;
        ResetMotion();
    }

    /// Moves the body by `delta` without changing its speed: what stands on a moving platform (scene/Shapes.h, Lift) is
    /// carried with it. Call it before Update, with the move of the platform in this frame.
    void Carry(const Vec3& delta)
    {
        if (std::isfinite(delta.x) && std::isfinite(delta.y) && std::isfinite(delta.z))
        {
            m_feet.x += delta.x;
            m_feet.y += delta.y;
            m_feet.z += delta.z;
        }
    }

    /// Puts the body so that its eye is at `eye` (used when the camera flew somewhere and walking starts again).
    void Place(const Vec3& eye)
    {
        if (std::isfinite(eye.x) && std::isfinite(eye.y) && std::isfinite(eye.z))
        {
            m_feet = {eye.x, eye.y - m_s.eyeHeight, eye.z};
            ResetMotion();
        }
    }

    Vec3 Feet() const
    {
        return m_feet;
    }

    Vec3 Eye() const
    {
        return {m_feet.x, m_feet.y + m_s.eyeHeight, m_feet.z};
    }

    /// The centre of ball i (0 = at the feet, 1 = halfway, 2 = at the eye).
    Vec3 Ball(int i) const
    {
        return BallCentre(m_feet, i);
    }

    bool Grounded() const
    {
        return m_grounded;
    }

    float VerticalSpeed() const
    {
        return m_vy;
    }

    /// The speed on the ground that comes from the keys (it follows the keys with the accelerations of the settings).
    float WalkSpeedNow() const
    {
        return std::sqrt(m_moveX * m_moveX + m_moveZ * m_moveZ);
    }

    /// The speed on the ground that does not come from the keys: what a slide has gathered (0 while standing).
    float SlideSpeed() const
    {
        return std::sqrt(m_vx * m_vx + m_vz * m_vz);
    }

    /// Where the keys wanted to go in the last Update, on the ground (x, z), as a unit direction; (0, 0) when no key
    /// was held. The puzzle reads it to know whether the body pushes against the block.
    Vec3 WishDirection() const
    {
        return m_wish;
    }

    bool Jumping() const
    {
        return m_jumping && !m_grounded;
    }

    float TakeoffHeight() const
    {
        return m_takeoffY;
    }

    // The caller has already checked the entire path and confirmed support at the destination.
    void CompleteMantle(const Vec3& feet)
    {
        m_feet = feet;
        ResetMotion();
        m_grounded = true;
    }

    /// True after the Update in which the body fell too far and was put back at the spawn point.
    bool JustRespawned() const
    {
        return m_respawned;
    }

    /// True once after a jump has ended on something solid; `peak` is how high the feet rose above the take-off point.
    bool ConsumeLanding(float& peak)
    {
        if (!m_landed)
        {
            return false;
        }
        m_landed = false;
        peak = m_lastPeak;
        return true;
    }

    /// True once after the body came down on something it can stand on at more than kImpactMin; `speed` is the falling
    /// speed at the moment it hit (m/s). The camera uses it for a small dip, so a landing is felt.
    bool ConsumeImpact(float& speed)
    {
        if (!m_impactPending)
        {
            return false;
        }
        m_impactPending = false;
        speed = m_impactSpeed;
        return true;
    }

    /// lookForward: where the camera looks (only its direction on the ground counts; straight up or down gives no
    /// walking). layer: 0 = A, 1 = B, the moment whose solid the body meets.
    void Update(const Input& input, const Vec3& lookForward, int layer, float dtSeconds)
    {
        m_respawned = false;
        if (!(dtSeconds > 0.0f) || !std::isfinite(dtSeconds))
        {
            return;
        }
        const float dt = dtSeconds > kMaxFrameSeconds ? kMaxFrameSeconds : dtSeconds;
        if (layer != m_lastLayer)
        {
            // a switch: a floor that went away with the moment gives no late jump (only one that was walked off does)
            m_coyote = m_lastLayer >= 0 ? 0.0f : m_coyote;
            m_lastLayer = layer;
        }

        // ---- the wish: a speed on the ground from the keys and from where the camera looks
        float wishX = 0.0f;
        float wishZ = 0.0f;
        m_wish = {};
        const float flat = std::sqrt(lookForward.x * lookForward.x + lookForward.z * lookForward.z);
        if (std::isfinite(flat) && flat > 1.0e-4f) // (a NaN direction gives a NaN length: false)
        {
            const float fx = lookForward.x / flat;
            const float fz = lookForward.z / flat;
            const float rx = fz;
            const float rz = -fx;
            float forward = Clamp(Finite(input.moveForward, 0.0f), -1.0f, 1.0f);
            float right = Clamp(Finite(input.moveRight, 0.0f), -1.0f, 1.0f);
            const float length = std::sqrt(forward * forward + right * right);
            if (length > 1.0f) // two keys together are not faster than one
            {
                forward /= length;
                right /= length;
            }
            const float dirX = fx * forward + rx * right;
            const float dirZ = fz * forward + rz * right;
            const float dirLength = std::sqrt(dirX * dirX + dirZ * dirZ);
            if (dirLength > 1.0e-4f)
            {
                m_wish = {dirX / dirLength, 0.0f, dirZ / dirLength};
            }
            const float speed = m_s.walkSpeed * (input.sprint && m_grounded ? m_s.sprintMultiplier : 1.0f);
            wishX = dirX * speed;
            wishZ = dirZ * speed;
        }
        if (input.jumpPressed)
        {
            m_jumpBuffer = kJumpBufferSeconds;
        }
        m_jumpHeld = input.jumpHeld;

        // ---- cut the frame into steps that move the body by less than a part of the radius
        const float jumpSpeed = std::sqrt(2.0f * m_s.gravity * m_s.jumpHeight);
        float fastest = m_s.walkSpeed > jumpSpeed ? m_s.walkSpeed : jumpSpeed;
        if (input.sprint || WalkSpeedNow() > m_s.walkSpeed)
        {
            const float lateral = (std::max)(WalkSpeedNow(), m_s.walkSpeed * m_s.sprintMultiplier);
            fastest = (std::max)(fastest, lateral);
        }
        const float falling = std::fabs(m_vy) + m_s.gravity * m_s.fallGravityScale * dt;
        fastest = falling > fastest ? falling : fastest;
        int steps = static_cast<int>(std::ceil(fastest * dt / (m_s.radius * 0.7f)));
        steps = steps < 1 ? 1 : (steps > kMaxSteps ? kMaxSteps : steps);
        const float h = dt / static_cast<float>(steps);
        for (int i = 0; i < steps && !m_respawned; ++i)
        {
            Step(h, wishX, wishZ, jumpSpeed, layer);
        }
    }

  private:
    static constexpr float kMaxFrameSeconds = 0.1f;
    static constexpr float kMaxFallSpeed = 40.0f; // UNITS: metres per second
    static constexpr float kCoyoteSeconds = 0.08f;
    static constexpr float kJumpBufferSeconds = 0.10f;
    static constexpr float kProbe = 0.03f; // UNITS: metres, how far below the feet the ground is looked for
    static constexpr float kFallBelow =
        -21.0f;                              // UNITS: metres: lower than any ground a body can reach (the sea is at -8)
    static constexpr float kTouch = 1.0e-5f; // UNITS: metres: a push of less than this counts as no contact
    static constexpr float kMinAirSeconds = 0.05f; // a jump cannot end in the first 0.05 s
    static constexpr float kImpactMin = 2.0f;      // UNITS: metres per second; a softer landing is not reported
    static constexpr int kMaxSteps = 32;
    static constexpr int kBalls = 3;

    static float Finite(float v, float fallback)
    {
        return std::isfinite(v) ? v : fallback;
    }

    static float Clamp(float v, float lo, float hi)
    {
        return v < lo ? lo : (v > hi ? hi : v);
    }

    void ResetMotion()
    {
        m_moveX = 0.0f;
        m_moveZ = 0.0f;
        m_vx = 0.0f;
        m_vz = 0.0f;
        m_vy = 0.0f;
        m_grounded = false;
        m_coyote = 0.0f;
        m_jumpBuffer = 0.0f;
        m_jumping = false;
        m_cuttable = false;
        m_airSeconds = 0.0f;
        m_wish = {};
        m_impactPending = false;
    }

    /// The centre of ball i (0 = feet, 2 = eye) for a body whose feet are at `feet`.
    Vec3 BallCentre(const Vec3& feet, int i) const
    {
        const float low = m_s.radius;
        const float high = m_s.eyeHeight;
        const float t = static_cast<float>(i) / static_cast<float>(kBalls - 1);
        return {feet.x, feet.y + low + (high - low) * t, feet.z};
    }

    struct BallResult
    {
        Vec3 position;
        bool pushed = false;
    };

    /// The ball at `centre`, pushed out of the solid of `layer`.
    BallResult ResolveBall(const Vec3& centre, int layer) const
    {
        BallResult r;
        r.position = centre;
        if (m_town != nullptr && m_town->PushOut(r.position, m_s.radius, layer) > kTouch)
        {
            r.pushed = true;
        }
        return r;
    }

    /// Adds to the wall push the part of the push on the ball at `centre` that comes from walls and from slopes too
    /// steep to stand on. Each contact is asked on its own (Moments::WallPush), so that a floor and a wall touched
    /// together are told apart.
    void AddWallPush(const Vec3& centre, int layer)
    {
        if (m_town != nullptr)
        {
            m_town->WallPush(centre, m_s.radius, layer, kGroundFacing, m_wallX, m_wallZ);
        }
    }

    /// Pushes the three balls out of the solids of `layer`; each push moves the whole body. A few rounds, because a
    /// push of one ball can put another one into something. `wall`: add the wall part of every push to m_wallX /
    /// m_wallZ.
    void ResolveBody(int layer, bool wall)
    {
        for (int round = 0; round < 8; ++round)
        {
            bool any = false;
            for (int i = 0; i < kBalls; ++i)
            {
                const Vec3 centre = BallCentre(m_feet, i);
                const BallResult r = ResolveBall(centre, layer);
                if (r.pushed)
                {
                    if (wall)
                    {
                        AddWallPush(centre, layer);
                    }
                    m_feet.x += r.position.x - centre.x;
                    m_feet.y += r.position.y - centre.y;
                    m_feet.z += r.position.z - centre.z;
                    any = true;
                }
            }
            if (!any)
            {
                break;
            }
        }
    }

    /// True when something solid is just below the feet and faces up.
    bool StandsOnSomething(int layer) const
    {
        Vec3 lowered = m_feet;
        lowered.y -= kProbe;
        const Vec3 centre = BallCentre(lowered, 0);
        const BallResult r = ResolveBall(centre, layer);
        if (!r.pushed)
        {
            return false;
        }
        const float dx = r.position.x - centre.x;
        const float dy = r.position.y - centre.y;
        const float dz = r.position.z - centre.z;
        const float length = std::sqrt(dx * dx + dy * dy + dz * dz);
        return dy > 0.005f && dy > kGroundFacing * length;
    }

    /// Keeps a body that stood in the last step on the ground when the ground dropped away by at most kSnapDown: the
    /// feet are lowered straight down in small steps until the probe finds ground that faces up, then settled on it.
    /// When there is no such ground within kSnapDown, nothing changes (an edge: the body falls).
    bool SnapToGround(int layer)
    {
        const Vec3 before = m_feet;
        for (float down = kSnapStep; down <= kSnapDown + 1.0e-4f; down += kSnapStep)
        {
            m_feet.y = before.y - down;
            if (StandsOnSomething(layer))
            {
                ResolveBody(layer, false); // settle on it: the probe found it a few centimetres below
                if (StandsOnSomething(layer) && m_feet.y < before.y)
                {
                    return true;
                }
                break;
            }
        }
        m_feet = before;
        return false;
    }

    /// Moves the speed on the ground toward (targetX, targetZ) by at most the acceleration of this moment.
    void Steer(float targetX, float targetZ, float h)
    {
        float accel = m_s.airAccel;
        if (m_grounded)
        {
            const bool wantsToMove = targetX * targetX + targetZ * targetZ > 1.0e-6f;
            const bool goesOn = targetX * m_moveX + targetZ * m_moveZ >= 0.0f;
            accel = (wantsToMove && goesOn) ? m_s.groundAccel : m_s.groundDecel;
        }
        const float dx = targetX - m_moveX;
        const float dz = targetZ - m_moveZ;
        const float gap = std::sqrt(dx * dx + dz * dz);
        const float most = accel * h;
        if (gap <= most || gap < 1.0e-6f)
        {
            m_moveX = targetX;
            m_moveZ = targetZ;
            return;
        }
        m_moveX += dx * (most / gap);
        m_moveZ += dz * (most / gap);
    }

    void Step(float h, float wishX, float wishZ, float jumpSpeed, int layer)
    {
        const bool stood = m_grounded;
        bool jumpedNow = false;
        m_coyote = m_coyote > h ? m_coyote - h : 0.0f;
        m_jumpBuffer = m_jumpBuffer > h ? m_jumpBuffer - h : 0.0f;
        if (m_jumpBuffer > 0.0f && m_coyote > 0.0f && jumpSpeed > 0.0f)
        {
            m_vy = jumpSpeed;
            m_jumpBuffer = 0.0f;
            m_coyote = 0.0f;
            m_grounded = false;
            m_jumping = true;
            m_cuttable = true;
            m_airSeconds = 0.0f;
            m_takeoffY = m_feet.y;
            m_peakY = m_feet.y;
            jumpedNow = true;
        }
        if (m_jumping)
        {
            m_airSeconds += h;
        }
        // the jump key let go while the body still rises: keep only a part of the rise (once per jump; after the top
        // the speed is not upward any more, so letting go changes nothing)
        if (m_cuttable && !m_jumpHeld && m_vy > 0.0f)
        {
            m_vy *= m_s.jumpCut;
            m_cuttable = false;
        }

        Steer(wishX, wishZ, h);

        // gravity: stronger while falling; the average speed of the step, so that a jump of height H rises to H and not
        // a little less
        const float g = m_vy > 0.0f ? m_s.gravity : m_s.gravity * m_s.fallGravityScale;
        float newVy = m_vy - g * h;
        newVy = newVy < -kMaxFallSpeed ? -kMaxFallSpeed : newVy;
        m_feet.x += (m_moveX + m_vx) * h;
        m_feet.z += (m_moveZ + m_vz) * h;
        m_feet.y += 0.5f * (m_vy + newVy) * h;
        m_vy = newVy;
        const float vyBeforeContact = m_vy;

        const Vec3 moved = m_feet;
        m_wallX = 0.0f;
        m_wallZ = 0.0f;
        ResolveBody(layer, true);

        // the push points away from what was hit: take the part of the speed that goes into it away (a floor ends a
        // fall, a ceiling ends a rise, a slope turns the fall into a slide)
        const float px = m_feet.x - moved.x;
        const float py = m_feet.y - moved.y;
        const float pz = m_feet.z - moved.z;
        const float push = std::sqrt(px * px + py * py + pz * pz);
        if (push > 1.0e-5f)
        {
            const float nx = px / push;
            const float ny = py / push;
            const float nz = pz / push;
            const float into = m_vx * nx + m_vy * ny + m_vz * nz;
            if (into < 0.0f)
            {
                m_vx -= into * nx;
                m_vy -= into * ny;
                m_vz -= into * nz;
            }
        }
        // the walk: only the pushes of solids that are not ground (AddWallPush) take the part into them away
        const float wallPush = std::sqrt(m_wallX * m_wallX + m_wallZ * m_wallZ);
        if (wallPush > 1.0e-5f)
        {
            const float hx = m_wallX / wallPush;
            const float hz = m_wallZ / wallPush;
            const float walkInto = m_moveX * hx + m_moveZ * hz;
            if (walkInto < 0.0f)
            {
                m_moveX -= walkInto * hx;
                m_moveZ -= walkInto * hz;
            }
        }
        m_grounded = m_vy <= 0.01f && StandsOnSomething(layer);
        if (!m_grounded && stood && !jumpedNow && m_vy <= 0.01f && SnapToGround(layer))
        {
            m_grounded = true;
            m_vy = 0.0f; // it walked down: no fall speed is gathered, so there is no landing and no dip
        }
        if (m_grounded)
        {
            m_coyote = kCoyoteSeconds;
            m_vx = 0.0f; // standing: the friction of the ground stops a slide
            m_vz = 0.0f;
            if (vyBeforeContact < -kImpactMin &&
                !stood) // a body that already stood has no such speed: this is the moment of contact
            {
                m_impactSpeed = -vyBeforeContact;
                m_impactPending = true;
            }
        }
        if (m_jumping)
        {
            m_peakY = m_feet.y > m_peakY ? m_feet.y : m_peakY;
            if (m_grounded && m_airSeconds > kMinAirSeconds)
            {
                m_lastPeak = m_peakY - m_takeoffY;
                m_landed = true;
                m_jumping = false;
            }
        }
        if (m_feet.y < kFallBelow)
        {
            Respawn();
            m_respawned = true;
        }
    }

    const town::Moments* m_town = nullptr;
    Vec3 m_feet{0.0f, 0.0f, 0.0f};
    Vec3 m_spawn{0.0f, 0.0f, 0.0f};
    Vec3 m_wish{};
    Settings m_s;
    float m_moveX = 0.0f; // UNITS: metres per second, the speed on the ground that comes from the keys
    float m_moveZ = 0.0f;
    float m_wallX = 0.0f; // UNITS: metres, the push of this step from solids that are not ground (AddWallPush)
    float m_wallZ = 0.0f;
    float m_vx = 0.0f; // UNITS: metres per second, the slide on the ground that does not come from the keys
    float m_vy = 0.0f;
    float m_vz = 0.0f;
    float m_coyote = 0.0f;
    int m_lastLayer = -1; ///< the moment of the last Update (a switch takes the coyote time away)
    float m_jumpBuffer = 0.0f;
    float m_airSeconds = 0.0f;
    float m_takeoffY = 0.0f;
    float m_peakY = 0.0f;
    float m_lastPeak = 0.0f;
    float m_impactSpeed = 0.0f;
    bool m_grounded = false;
    bool m_jumping = false;
    bool m_cuttable = false;
    bool m_jumpHeld = true;
    bool m_landed = false;
    bool m_impactPending = false;
    bool m_respawned = false;
};

} // namespace dl
