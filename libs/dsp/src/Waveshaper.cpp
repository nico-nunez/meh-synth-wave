#include "dsp/Waveshaper.h"
#include "dsp/Math.h"

#include <algorithm>
#include <cassert>
#include <cmath>

namespace dsp::waveshaper {
// Denormalized drive and invDrive
// mix expected to be normalized [0,1]
float softClip(float sample, float drive, float invDrive, float mix) {
  // Check for denormalized range
  assert(drive >= 0.0f);

  float saturated = math::fastTanh(sample * drive) * invDrive;
  return sample * (1.0f - mix) + (saturated * mix);
}

// NOTE(nico): Revisit this for creative effect
// Less for protection and requires control of input levels
// Can't be too hot
float softClipAlt(float x) {
  if (x > 1.0f)
    return 1.0f;
  if (x < -1.0f)
    return -1.0f;
  return x - (x * x * x) / 3.0f; // x - x³/3
}

float hardClip(float sample, float threshold) {
  return std::clamp(sample, -threshold, threshold);
}

// NOTE(nico): drive should be denormalized, but
// bias should be normalized
// Callers responsibility to ensure drive is not too much
float tapeSimulation(float sample, float drive, float bias) {
  float x = sample * drive + bias * 0.1f;

  // Asymmetric clipping (tape characteristic)
  if (x > 0.0f) {
    x = x / (1.0f + x);
  } else {
    x = x / (1.0f - 0.7f * x); // Different curve for negative
  }

  return x;
}

// Algebraic soft clip — cheaper than tanh, slightly brighter character
float saturate_soft(float x) {
  return x / (1.0f + std::abs(x));
}

// Asymmetric — different compression on positive vs negative halves
// Adds even harmonics (2nd, 4th) = "warmth", transistor-like
float saturate_asymm(float x) {
  return x > 0.0f ? math::fastTanh(x * 1.2f) : math::fastTanh(x * 0.8f);
}
} // namespace dsp::waveshaper
