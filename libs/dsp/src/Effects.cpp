#include "dsp/Effects.h"

#include <algorithm>
#include <cassert>
#include <cmath>

namespace dsp::effects {
float denormalizeDrive(float drive) {
  return 1 + drive * 4.0f;
}

// Returns whatever drive is (Normalized or Denormalized)
float calcInvDrive(float drive) {
  if (drive <= 0)
    return 1.0f;

  return 1.0f / std::tanh(drive);
}

// Denormalized drive and invDrive
// mix expected to be normalized [0,1]
float softClip(float sample, float drive, float invDrive, float mix) {
  // Check for denormalized range
  assert(drive >= 0.0f);
  assert(drive <= 5.0f);

  float saturated = std::tanh(sample * drive) * invDrive;
  return sample * (1.0f - mix) + (saturated * mix);
}

// NOTE(nico): Revisit this for creative effect
// Less for protection and requires control of input levels
// Can't be too hot
inline float softClipAlt(float x) {
  if (x > 1.0f)
    return 1.0f;
  if (x < -1.0f)
    return -1.0f;
  return x - (x * x * x) / 3.0f; // x - x³/3
}

// Polynomial approximation of tanh
float softClipFast(float x) {
  return (x * (27.0f + x * x)) / (27.0f + 9.0f * x * x);
}

float hardClip(float sample, float threshold) {
  return std::clamp(sample, -threshold, threshold);
}

// NOTE(nico): drive should be denormalized, but
// bias should be normalized
float tapeSimulation(float sample, float drive, float bias) {
  // Limit max drive for less aggressive tape saturation
  if (drive > 3)
    drive = 3;

  float x = sample * drive + bias * 0.1f;

  // Asymmetric clipping (tape characteristic)
  if (x > 0.0f) {
    x = x / (1.0f + x);
  } else {
    x = x / (1.0f - 0.7f * x); // Different curve for negative
  }

  return x;
}

float dcBlock(float sample, float& state, float coefficient) {
  float output = sample - state;
  state = sample * (1.0f - coefficient) + state * coefficient;
  return output;
}

// tanh — smooth, symmetric, expensive. The "classic" sound.
float saturate_tanh(float x) {
  return std::tanh(x);
}

// Algebraic soft clip — cheaper than tanh, slightly brighter character
float saturate_soft(float x) {
  return x / (1.0f + std::abs(x));
}

// Polynomial tanh approximation — fast, tunable breakpoint
float saturate_poly(float x) {
  return x * (27.0f + x * x) / (27.0f + 9.0f * x * x);
}

// Asymmetric — different compression on positive vs negative halves
// Adds even harmonics (2nd, 4th) = "warmth", transistor-like
float saturate_asymm(float x) {
  return x > 0.0f ? std::tanh(x * 1.2f) : std::tanh(x * 0.8f);
}
} // namespace dsp::effects
