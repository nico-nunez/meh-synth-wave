#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace synth::preset {

inline constexpr uint32_t CURRENT_PRESET_VERSION = 1;

// ============================================================
// Sub-structs
// ============================================================

struct OscPreset {
  std::string bank = "sine";
  float scanPos = 0.0f;
  float mixLevel = 1.0f;
  float fmDepth = 0.0f;
  float fmRatio = 1.0f;
  std::string fmSource = "none";
  int8_t octaveOffset = 0;
  float detuneAmount = 0.0f;
  bool enabled = true;
};

struct NoisePreset {
  float mixLevel = 0.0f;
  std::string type = "white";
  bool enabled = false;
};

struct EnvelopePreset {
  float attackMs = 10.0f;
  float decayMs = 100.0f;
  float sustainLevel = 0.7f;
  float releaseMs = 200.0f;
  float attackCurve = -5.0f;
  float decayCurve = -5.0f;
  float releaseCurve = -5.0f;
};

struct SVFPreset {
  std::string mode = "lp";
  float cutoff = 1000.0f;
  float resonance = 0.5f;
  bool enabled = false;
};

struct LadderPreset {
  float cutoff = 1000.0f;
  float resonance = 0.3f;
  float drive = 1.0f;
  bool enabled = false;
};

struct SaturatorPreset {
  float drive = 1.0f;
  float mix = 1.0f;
  bool enabled = false;
};

struct LFOPreset {
  std::string bank = "sine"; // "sah" = Sample & Hold
  float rate = 1.0f;
  float amplitude = 1.0f;
  bool retrigger = false;
};

struct ModRoutePreset {
  std::string source;
  std::string destination;
  float amount = 0.0f;
};

struct MonoPreset {
  bool enabled = false;
  bool legato = true;
};

struct PortaPreset {
  float time = 50.0f;
  bool legato = true;
  bool enabled = false;
};

struct UnisonPreset {
  int8_t voices = 4;
  float detune = 20.0f;
  float spread = 0.5f;
  bool enabled = false;
};

struct GlobalPreset {
  float pitchBendRange = 2.0f;
};

struct PresetMetadata {
  std::string name = "Init";
  std::string author;
  std::string category;
  std::string description;
};

// ============================================================
// Top-level Preset
// ============================================================

struct Preset {
  uint32_t version = CURRENT_PRESET_VERSION;
  PresetMetadata metadata;

  OscPreset osc1, osc2, osc3, osc4;
  NoisePreset noise;
  EnvelopePreset ampEnv, filterEnv, modEnv;
  SVFPreset svf;
  LadderPreset ladder;
  SaturatorPreset saturator;
  LFOPreset lfo1, lfo2, lfo3;
  std::vector<ModRoutePreset> modMatrix;
  std::vector<std::string> signalChain = {"svf", "ladder", "saturator"};
  MonoPreset mono;
  PortaPreset porta;
  UnisonPreset unison;
  GlobalPreset global;
};

// ============================================================
// Init preset — hardcoded clean starting point
// ============================================================

inline Preset createInitPreset() {
  Preset p;
  p.metadata.name = "Init";
  p.metadata.category = "Init";
  p.metadata.description = "Clean starting point";

  // osc1: sine, full level (struct defaults)
  p.osc1 = OscPreset{};

  // osc2-4: disabled
  p.osc2 = OscPreset{};
  p.osc2.enabled = false;
  p.osc2.mixLevel = 0.0f;
  p.osc3 = OscPreset{};
  p.osc3.enabled = false;
  p.osc3.mixLevel = 0.0f;
  p.osc4 = OscPreset{};
  p.osc4.enabled = false;
  p.osc4.mixLevel = 0.0f;

  // Everything else: struct defaults (filters disabled, no mod routes, etc.)
  return p;
}

} // namespace synth::preset
