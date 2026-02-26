#pragma once

#include <cstdint>
namespace synth::noise_osc {

enum class NoiseType : uint8_t {
  White = 0,
  Pink,
};

struct NoiseOscConfig {
  float mixLevel = 0.0f;
  NoiseType noiseType = NoiseType::White;
  bool enabled = false;
};

struct NoiseOscillator {
  float mixLevel = 0.0f;
  NoiseType noiseType = NoiseType::White;
  bool enabled = false;

  // Pink noise filter state (Kellet 3-stage IIR)
  float b0 = 0.0f;
  float b1 = 0.0f;
  float b2 = 0.0f;
};

float processNoise(NoiseOscillator &noise);
// Returns noise in [-1, 1] scaled by mixLevel.
// White: flat spectrum. Pink: -3dB/octave via Kellet approximation.
// Uses xorshift32 PRNG — do not use std::rand().

} // namespace synth::noise_osc
