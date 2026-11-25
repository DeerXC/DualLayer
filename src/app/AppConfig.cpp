#include "app/AppConfig.h"

#include "core/Log.h"

#include <string>

namespace dl
{

namespace
{

constexpr float kDefaultBodyRadius =
    0.35f; // UNITS: metres, the ball of the camera. TUNE: [collision] radius, 0.05 .. 1
constexpr float kDefaultRevealRadius = 15.0f; // UNITS: metres. TUNE: [map] reveal_radius, 6 .. 40

log::Level ParseLevel(const std::string& text)
{
    if (text == "trace")
    {
        return log::Level::Trace;
    }
    if (text == "warn")
    {
        return log::Level::Warn;
    }
    if (text == "error")
    {
        return log::Level::Error;
    }
    return log::Level::Info;
}

} // namespace

void LoadConfig(Config& config, Camera& camera)
{
    const Config::ParseResult result = config.LoadFile(kConfigPath);
    for (const std::string& error : result.errors)
    {
        DL_LOG_WARN("Config", "%s: %s", kConfigPath, error.c_str());
    }
    DL_LOG_INFO("Config", "%s: %d values", kConfigPath, result.entries);
    log::SetMinLevel(ParseLevel(config.GetString("log", "level", "info")));
    // The camera speeds come from the config file: change them, press F5, no recompiling.
    Camera::Settings settings; // the defaults live in Camera.h
    settings.moveSpeed = config.GetFloat("camera", "move_speed", settings.moveSpeed);
    settings.turnSpeed = config.GetFloat("camera", "turn_speed", settings.turnSpeed);
    camera.SetSettings(settings);
}

void ApplyPlayerConfig(const Config& config, Player& player)
{
    Player::Settings settings; // the defaults live in Player.h
    settings.radius = config.GetFloat("collision", "radius", kDefaultBodyRadius);
    settings.eyeHeight = config.GetFloat("player", "eye_height", settings.eyeHeight);
    settings.sprintMultiplier = config.GetFloat("player", "sprint_multiplier", settings.sprintMultiplier);
    settings.walkSpeed = config.GetFloat("player", "walk_speed", settings.walkSpeed);
    settings.groundAccel = config.GetFloat("player", "ground_accel", settings.groundAccel);
    settings.groundDecel = config.GetFloat("player", "ground_decel", settings.groundDecel);
    settings.airAccel = config.GetFloat("player", "air_accel", settings.airAccel);
    settings.gravity = config.GetFloat("player", "gravity", settings.gravity);
    settings.fallGravityScale = config.GetFloat("player", "fall_gravity_scale", settings.fallGravityScale);
    settings.jumpHeight = config.GetFloat("player", "jump_height", settings.jumpHeight);
    settings.jumpCut = config.GetFloat("player", "jump_cut", settings.jumpCut);
    player.SetSettings(settings);
}

town::GuidanceSettings ReadGuidanceSettings(const Config& config)
{
    town::GuidanceSettings settings; // the defaults live in scene/Guidance.h
    settings.enabled = config.GetBool("guidance", "enabled", settings.enabled);
    settings.leanDegrees = config.GetFloat("guidance", "lean_degrees", settings.leanDegrees);
    settings.leanIdleDegrees = config.GetFloat("guidance", "lean_idle_degrees", settings.leanIdleDegrees);
    settings.idleSeconds = config.GetFloat("guidance", "idle_seconds", settings.idleSeconds);
    settings.sweepSeconds = config.GetFloat("guidance", "sweep_seconds", settings.sweepSeconds);
    settings.ghostAfterSeconds = config.GetFloat("guidance", "ghost_after_seconds", settings.ghostAfterSeconds);
    settings.ghostSeconds = config.GetFloat("guidance", "ghost_seconds", settings.ghostSeconds);
    return settings;
}

float ReadMapRevealRadius(const Config& config)
{
    return config.GetFloat("map", "reveal_radius", kDefaultRevealRadius);
}

} // namespace dl
