#pragma once

#include "dsp/Effects/Chorus.h"
#include "dsp/Effects/Delay.h"
#include "dsp/Effects/Distortion.h"
#include "dsp/Effects/Phaser.h"
#include "dsp/Effects/Reverb.h"

#include <cstdint>

namespace synth::effects_chain {

using namespace dsp::effects;
using chorus::ChorusEffect;
using delay::DelayEffect;
using distortion::DistortionEffect;
using phaser::PhaserEffect;
using reverb::ReverbEffect;

constexpr uint8_t MAX_EFFECT_SLOTS = 8;

enum EffectProcessor : uint8_t {
  None = 0,
  Distortion,
  Chorus,
  Phaser,
  Delay,
  ReverbPlate, // Dattorro plate — named for future ReverbRoom (FDN) alongside it
};

struct EffectsChain {
  DistortionEffect distortion{};
  ChorusEffect chorus;
  PhaserEffect phaser;
  DelayEffect delay;
  ReverbEffect reverb;

  // Ordered processing slot array
  EffectProcessor slots[MAX_EFFECT_SLOTS];
  uint8_t length = 0;

  float sampleRate; // set at init, needed by delay/chorus/phaser
};

// --- String table for serialization (mirrors SignalProcessorMapping) ---
struct EffectProcessorMapping {
  const char* name;
  EffectProcessor proc;
};

inline constexpr EffectProcessorMapping effectProcessorMappings[] = {
    {"distortion", EffectProcessor::Distortion},
    {"chorus", EffectProcessor::Chorus},
    {"phaser", EffectProcessor::Phaser},
    {"delay", EffectProcessor::Delay},
    {"reverb", EffectProcessor::ReverbPlate},
};

EffectProcessor parseEffectProcessor(const char* name);
const char* effectProcessorToString(EffectProcessor proc);

} // namespace synth::effects_chain
