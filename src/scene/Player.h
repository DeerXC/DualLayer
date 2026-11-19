// Player.h - a walking body for the camera: it walks, falls, lands, jumps, stands on solid objects and can fall off the floor.
// Plain maths, no Windows, no Direct3D: tested without a window (tests/test_player.cpp).
// Body:      three balls of the collision radius, one above the other: at the feet, at the eye (the camera) and one halfway, so that
//            the body cannot slip between thin parts (the tube of the torus is 0.6 m thick, the balls overlap). The camera is the
//            eye; the feet are the point on the ground.
// Layers:    only the objects of the given layer are solid (scene/Collision.h). The layer is a parameter of every Update, so a body
//            standing on an object that is not in the new layer falls at once.
// Method:    per small step: horizontal speed from the keys (no run-up: a stop is a stop), gravity, jump; move; push the three balls out
//            of the solids (ResolveSphere). The direction of the push is the normal of what was hit: the part of the falling speed that
//            goes into it is removed, the rest stays, so the body slides down a slope that is too steep and a floor ends a fall. Then
//            a probe a few centimetres below the feet tells whether the body stands on something (the surface must face up: a slope
//            limit of about 53 degrees); only a body that stands has no slide speed left.
// Feel:      coyote time (a jump still works for 0.08 s after the body left an edge) and a jump buffer (a jump pressed 0.1 s before the
//            landing happens at the landing).
// Time:      Update takes the real frame time, limited to 0.1 s, and cuts it into steps that move the body by less than the radius,
//            so that nothing is jumped over (the floor is only 0.2 m thick).
// Falling:   a body that falls kFallDepth below the floor goes back to the spawn point (JustRespawned tells it).
// Space:     left-handed, +Y up. With the forward direction f on the ground, right = (f.z, 0, -f.x).
// UNITS:     metres, seconds, metres per second.
// Note:      no std::min / std::max here: windows.h may define them as macros (docs/CONVENTIONS.md).
#pragma once

#include "scene/Camera.h" // Vec3
#include "scene/Collision.h"

#include <cmath>

namespace dl
{

class Player
{
  public:
    struct Settings
    {
        float radius = 0.35f;    ///< UNITS: metres, the radius of the three balls
        float eyeHeight = 1.55f; ///< UNITS: metres, from the feet to the eye (the camera)
        float walkSpeed = 4.0f;  ///< UNITS: metres per second
        float gravity = 30.0f;   ///< UNITS: metres per second squared (a snappy jump, not the real 9.8)
        float jumpHeight = 2.0f; ///< UNITS: metres that the feet rise in a jump (the objects of the scene are up to 1.8 m high)
    };

    struct Input
    {
        float moveForward = 0.0f; ///< -1 .. +1
        float moveRight = 0.0f;   ///< -1 .. +1
        bool jumpPressed = false; ///< true for the frame in which the key went down
    };

    /// Values that make no sense are pulled back into a range that works (NaN gets the default).
    void SetSettings(const Settings& settings)
    {
        const Settings defaults{};
        m_s.radius = Clamp(Finite(settings.radius, defaults.radius), 0.05f, 1.0f);
        m_s.eyeHeight = Clamp(Finite(settings.eyeHeight, defaults.eyeHeight), m_s.radius + 0.1f, 3.0f);
        m_s.walkSpeed = Clamp(Finite(settings.walkSpeed, defaults.walkSpeed), 0.0f, 20.0f);
        m_s.gravity = Clamp(Finite(settings.gravity, defaults.gravity), 1.0f, 100.0f);
        m_s.jumpHeight = Clamp(Finite(settings.jumpHeight, defaults.jumpHeight), 0.0f, 5.0f);
    }

    const Settings& GetSettings() const
    {
        return m_s;
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

    bool Grounded() const
    {
        return m_grounded;
    }

    float VerticalSpeed() const
    {
        return m_vy;
    }

    /// The speed on the ground that does not come from the keys: what a slide has gathered (0 while standing).
    float SlideSpeed() const
    {
        return std::sqrt(m_vx * m_vx + m_vz * m_vz);
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

    /// lookForward: where the camera looks (only its direction on the ground counts; straight up or down gives no walking).
    /// layer: 0 = A, 1 = B, the layer whose objects are solid. spinSeconds: the time that the renderer uses for the turning shapes.
    void Update(const Input& input, const Vec3& lookForward, int layer, float spinSeconds, float dtSeconds)
    {
        m_respawned = false;
        if (!(dtSeconds > 0.0f) || !std::isfinite(dtSeconds) || !std::isfinite(spinSeconds))
        {
            return;
        }
        const float dt = dtSeconds > kMaxFrameSeconds ? kMaxFrameSeconds : dtSeconds;

        // ---- the wish: a speed on the ground from the keys and from where the camera looks
        float wishX = 0.0f;
        float wishZ = 0.0f;
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
            wishX = (fx * forward + rx * right) * m_s.walkSpeed;
            wishZ = (fz * forward + rz * right) * m_s.walkSpeed;
        }
        if (input.jumpPressed)
        {
            m_jumpBuffer = kJumpBufferSeconds;
        }

        // ---- cut the frame into steps that move the body by less than a part of the radius
        const float jumpSpeed = std::sqrt(2.0f * m_s.gravity * m_s.jumpHeight);
        float fastest = m_s.walkSpeed > jumpSpeed ? m_s.walkSpeed : jumpSpeed;
        const float falling = std::fabs(m_vy) + m_s.gravity * dt;
        fastest = falling > fastest ? falling : fastest;
        int steps = static_cast<int>(std::ceil(fastest * dt / (m_s.radius * 0.7f)));
        steps = steps < 1 ? 1 : (steps > kMaxSteps ? kMaxSteps : steps);
        const float h = dt / static_cast<float>(steps);
        for (int i = 0; i < steps && !m_respawned; ++i)
        {
            Step(h, wishX, wishZ, jumpSpeed, layer, spinSeconds);
        }
    }

  private:
    static constexpr float kMaxFrameSeconds = 0.1f;
    static constexpr float kMaxFallSpeed = 40.0f;      // UNITS: metres per second
    static constexpr float kCoyoteSeconds = 0.08f;
    static constexpr float kJumpBufferSeconds = 0.10f;
    static constexpr float kProbe = 0.03f;             // UNITS: metres, how far below the feet the ground is looked for
    static constexpr float kGroundFacing = 0.6f;       // the push of the ground must point up by at least this part (0.6 = about 53 degrees)
    static constexpr float kFallDepth = 20.0f;         // UNITS: metres below the floor of the scene
    static constexpr float kMinAirSeconds = 0.05f;     // a jump cannot end in the first 0.05 s
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
        m_vx = 0.0f;
        m_vz = 0.0f;
        m_vy = 0.0f;
        m_grounded = false;
        m_coyote = 0.0f;
        m_jumpBuffer = 0.0f;
        m_jumping = false;
        m_airSeconds = 0.0f;
    }

    /// The centre of ball i (0 = feet, 2 = eye) for a body whose feet are at `feet`.
    Vec3 BallCentre(const Vec3& feet, int i) const
    {
        const float low = m_s.radius;
        const float high = m_s.eyeHeight;
        const float t = static_cast<float>(i) / static_cast<float>(kBalls - 1);
        return {feet.x, feet.y + low + (high - low) * t, feet.z};
    }

    /// Pushes the three balls out of the solids of `layer`; each push moves the whole body. A few rounds, because a push of
    /// one ball can put another one into something.
    void ResolveBody(int layer, float spinSeconds)
    {
        for (int round = 0; round < 8; ++round)
        {
            bool any = false;
            for (int i = 0; i < kBalls; ++i)
            {
                const Vec3 centre = BallCentre(m_feet, i);
                const CollisionResult r = ResolveSphere(centre, m_s.radius, layer, spinSeconds);
                if (r.pushed)
                {
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
    bool StandsOnSomething(int layer, float spinSeconds) const
    {
        Vec3 lowered = m_feet;
        lowered.y -= kProbe;
        const Vec3 centre = BallCentre(lowered, 0);
        const CollisionResult r = ResolveSphere(centre, m_s.radius, layer, spinSeconds);
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

    void Step(float h, float wishX, float wishZ, float jumpSpeed, int layer, float spinSeconds)
    {
        m_coyote = m_coyote > h ? m_coyote - h : 0.0f;
        m_jumpBuffer = m_jumpBuffer > h ? m_jumpBuffer - h : 0.0f;
        if (m_jumpBuffer > 0.0f && m_coyote > 0.0f && jumpSpeed > 0.0f)
        {
            m_vy = jumpSpeed;
            m_jumpBuffer = 0.0f;
            m_coyote = 0.0f;
            m_grounded = false;
            m_jumping = true;
            m_airSeconds = 0.0f;
            m_takeoffY = m_feet.y;
            m_peakY = m_feet.y;
        }
        if (m_jumping)
        {
            m_airSeconds += h;
        }

        // gravity: the average speed of the step, so that a jump of height H rises to H and not a little less
        float newVy = m_vy - m_s.gravity * h;
        newVy = newVy < -kMaxFallSpeed ? -kMaxFallSpeed : newVy;
        m_feet.x += (wishX + m_vx) * h;
        m_feet.z += (wishZ + m_vz) * h;
        m_feet.y += 0.5f * (m_vy + newVy) * h;
        m_vy = newVy;

        const Vec3 moved = m_feet;
        ResolveBody(layer, spinSeconds);

        // the push points away from what was hit: take the part of the speed that goes into it away (a floor ends a fall, a ceiling
        // ends a rise, a slope turns the fall into a slide)
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
        m_grounded = m_vy <= 0.01f && StandsOnSomething(layer, spinSeconds);
        if (m_grounded)
        {
            m_coyote = kCoyoteSeconds;
            m_vx = 0.0f; // standing: the friction of the ground stops a slide
            m_vz = 0.0f;
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
        if (m_feet.y < scene::kFloorY - kFallDepth)
        {
            Respawn();
            m_respawned = true;
        }
    }

    Vec3 m_feet{0.0f, scene::kFloorY, -3.0f};
    Vec3 m_spawn{0.0f, scene::kFloorY, -3.0f};
    Settings m_s;
    float m_vx = 0.0f; // UNITS: metres per second, the slide on the ground that does not come from the keys
    float m_vy = 0.0f;
    float m_vz = 0.0f;
    float m_coyote = 0.0f;
    float m_jumpBuffer = 0.0f;
    float m_airSeconds = 0.0f;
    float m_takeoffY = 0.0f;
    float m_peakY = 0.0f;
    float m_lastPeak = 0.0f;
    bool m_grounded = false;
    bool m_jumping = false;
    bool m_landed = false;
    bool m_respawned = false;
};

} // namespace dl
