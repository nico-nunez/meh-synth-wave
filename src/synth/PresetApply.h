#pragma once

#include "synth/Preset.h"

#include <sstream>
#include <string>
#include <vector>

namespace synth {
struct Engine;
}

namespace synth::preset {

// ============================================================
// Apply: Preset → Engine (VoicePool + ParamRouter)
// ============================================================

struct ApplyResult {
  std::vector<std::string> warnings;
};

ApplyResult applyPreset(const Preset& preset, Engine& engine);

// ============================================================
// Capture: Engine → Preset
// ============================================================

Preset capturePreset(const Engine& engine);

// ============================================================
// Process Preset Input Command (terminal) Helper
// ============================================================
void processPresetCmd(std::istringstream& iss, Engine& engine);

} // namespace synth::preset
