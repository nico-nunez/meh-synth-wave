#include "TempTests.h"

#include "synth/Preset.h"
#include "synth/PresetSerializer.h"

#include <cstdio>
#include <fstream>
#include <sstream>

namespace synth::test {
void presetSerializer() {
  // Test 1: Serialize init preset
  auto init = preset::createInitPreset();
  std::string json = preset::serializePreset(init);
  printf("=== Serialized init preset ===\n%s\n", json.c_str());

  // Test 2: Deserialize it back
  auto result = preset::deserializePreset(json);
  if (!result.ok()) {
    printf("ERROR: %s\n", result.error.c_str());
    return;
  }
  for (const auto& w : result.warnings)
    printf("WARNING: %s\n", w.c_str());
  printf("Round-trip: OK (%zu warnings)\n", result.warnings.size());

  // Test 3: Load a factory preset from file
  std::ifstream file("presets/factory/acid_bass_303.json");
  if (file.is_open()) {
    std::ostringstream ss;
    ss << file.rdbuf();
    auto foo = ss.str();
    auto factoryResult = preset::deserializePreset(ss.str());
    if (!factoryResult.ok()) {
      printf("Factory load ERROR: %s\n", factoryResult.error.c_str());
    } else {
      printf("Factory load: OK — '%s' (%zu warnings)\n",
             factoryResult.preset.metadata.name.c_str(),
             factoryResult.warnings.size());
      for (const auto& w : factoryResult.warnings)
        printf("  WARNING: %s\n", w.c_str());
    }
  }
}
} // namespace synth::test
