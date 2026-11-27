// AppConfig.h - what the program reads from config/default.ini outside the renderer: the log level, the camera, the
// walking body, the guidance and the map. Every value is optional; a missing or nonsensical one falls back
// (core/Config.h).
// Reload:       the same functions run again when F5 is pressed.
#pragma once

#include "core/Config.h"
#include "scene/Camera.h"
#include "scene/Guidance.h"
#include "scene/Player.h"

namespace dl
{

inline constexpr char kConfigPath[] = "config/default.ini";

/// Reads the file into `config` (a problem is logged, not fatal), sets the log level and the speeds of the camera.
void LoadConfig(Config& config, Camera& camera);

/// The numbers of the walking body: [player] and [collision]. Player pulls nonsense values into a range that works.
void ApplyPlayerConfig(const Config& config, Player& player);

/// [guidance]: enabled, lean_degrees, lean_idle_degrees, idle_seconds, sweep_seconds, ghost_after_seconds,
/// ghost_seconds.
town::GuidanceSettings ReadGuidanceSettings(const Config& config);

/// [map] reveal_radius: how far round the body the plan is seen (metres).
float ReadMapRevealRadius(const Config& config);

} // namespace dl
