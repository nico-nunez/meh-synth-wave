#include "synth/PresetIO.h"
#include <cstdio>

namespace synth::test {

void presetIO() {
  // Ensure dirs
  synth::preset::ensurePresetDirs();

  // Load a factory preset by name (dev fallback)
  auto result = synth::preset::loadPresetByName("init");
  if (!result.ok()) {
    printf("Load error: %s\n", result.error.c_str());
    return;
  }
  printf("Loaded: %s from %s\n", result.preset.metadata.name.c_str(), result.filePath.c_str());

  for (const auto& w : result.warnings)
    printf("  WARNING: %s\n", w.c_str());

  // Save to user dir
  std::string savePath = synth::preset::getUserPresetsDir() + "/test_save.json";
  std::string err = synth::preset::savePreset(result.preset, savePath);
  if (!err.empty()) {
    printf("Save error: %s\n", err.c_str());
    return;
  }
  printf("Saved to: %s\n", savePath.c_str());

  // Reload and verify
  auto reloaded = synth::preset::loadPreset(savePath);
  printf("Reloaded: %s (%s)\n",
         reloaded.preset.metadata.name.c_str(),
         reloaded.ok() ? "OK" : reloaded.error.c_str());

  // List all presets
  auto presets = synth::preset::listPresets();
  printf("\nAvailable presets (%zu):\n", presets.size());
  for (const auto& entry : presets) {
    printf("  %s [%s] — %s\n",
           entry.name.c_str(),
           entry.isFactory ? "factory" : "user",
           entry.path.c_str());
  }
}
} // namespace synth::test
