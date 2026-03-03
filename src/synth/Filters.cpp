#include "Filters.h"

#include "dsp/Math.h"

#include <algorithm>
#include <cmath>

namespace synth::filters {

namespace {

float normalizeCutoff(float cutoffFreq, float invSampleRate) {
  // keep below Nyquist
  return std::min(cutoffFreq * invSampleRate, 0.49f);
}

} // namespace

// ==== SVF Helpers ====
void enableSVFilter(SVFilter& filter, bool enable) {
  if (enable && !filter.enabled) {
    // Reset all voice states
    for (uint32_t i = 0; i < MAX_VOICES; i++) {
      filter.voiceStates[i] = dsp::filters::SVFState{};
    }
  }
  filter.enabled = enable;
}

void initSVFilter(SVFilter& filter, size_t voiceIndex) {
  filter.voiceStates[voiceIndex] = SVFState{};
}

void updateSVFCoefficients(SVFilter& filter, float invSampleRate) {
  float Q = 0.5f + filter.resonance * 20.0f;
  float normalizedCutoff = normalizeCutoff(filter.cutoff, invSampleRate);
  filter.coeffs = dsp::filters::computeSVFCoeffs(normalizedCutoff, Q);
}

// Use when NOT passing modulation values (cutoff and/or resonance)
float processSVFilter(SVFilter& filter, float input, uint32_t voiceIndex) {
  if (!filter.enabled)
    return input;

  SVFOutputs out = dsp::filters::processSVF(input, filter.coeffs, filter.voiceStates[voiceIndex]);

  switch (filter.mode) {
  case SVFMode::MODE_COUNT:
  case SVFMode::LP:
    return out.lp;
  case SVFMode::HP:
    return out.hp;
  case SVFMode::BP:
    return out.bp;
  case SVFMode::Notch:
    return out.lp + out.hp;
  }
  return out.lp;
}

// Use when passing modulation values (cutoff and/or resonance)
float processSVFilter(SVFilter& filter,
                      float input,
                      uint32_t voiceIndex,
                      float cutoffHz,
                      float resonance,
                      float invSampleRate) {
  if (!filter.enabled)
    return input;

  bool isModulated = std::abs(filter.cutoff - cutoffHz) > 0.001f ||
                     std::abs(filter.resonance - resonance) > 0.001f;

  float normalizedCutoff = normalizeCutoff(cutoffHz, invSampleRate);

  SVFCoeffs coeffs =
      isModulated ? dsp::filters::computeSVFCoeffs(normalizedCutoff, 0.5f + resonance * 20.0f)
                  : filter.coeffs;

  SVFOutputs out = dsp::filters::processSVF(input, coeffs, filter.voiceStates[voiceIndex]);

  switch (filter.mode) {
  case SVFMode::MODE_COUNT:
  case SVFMode::LP:
    return out.lp;
  case SVFMode::HP:
    return out.hp;
  case SVFMode::BP:
    return out.bp;
  case SVFMode::Notch:
    return out.lp + out.hp;
  }
  return out.lp;
}

// ==== Ladder Helpers ====
void enableLadderFilter(LadderFilter& filter, bool enable) {
  if (enable && !filter.enabled) {
    for (uint32_t i = 0; i < MAX_VOICES; i++) {
      filter.voiceStates[i] = dsp::filters::LadderState{};
    }
  }
  filter.enabled = enable;
}

void initLadderFilter(LadderFilter& filter, size_t voiceIndex) {
  filter.voiceStates[voiceIndex] = LadderState{};
}

void updateLadderCoefficient(LadderFilter& filter, float invSampleRate) {
  float normalizedCutoff = normalizeCutoff(filter.cutoff, invSampleRate);
  filter.coeff = 2.0f * dsp::math::fastSin(dsp::math::PI_F * normalizedCutoff);
}

// Use when NOT passing modulation values (cutoff and/or resonance)
float processLadderFilter(LadderFilter& filter, float input, uint32_t voiceIndex) {
  if (!filter.enabled)
    return input;

  float res = filter.resonance * 4.0f; // map 0–1 to Ladder's 0–4 range

  return (filter.drive > 1.001f)
             ? dsp::filters::processLadderNonlinear(input,
                                                    filter.coeff,
                                                    res,
                                                    filter.drive,
                                                    filter.voiceStates[voiceIndex])
             : dsp::filters::processLadder(input,
                                           filter.coeff,
                                           res,
                                           filter.voiceStates[voiceIndex]);
}

// Use when passing modulation values (cutoff and/or resonance)
float processLadderFilter(LadderFilter& filter,
                          float input,
                          uint32_t voiceIndex,
                          float cutoffHz,
                          float resonance,
                          float invSampleRate) {
  if (!filter.enabled)
    return input;

  float normalizedCutoff = normalizeCutoff(cutoffHz, invSampleRate);
  float coeff = std::abs(filter.cutoff - cutoffHz) > 0.001f
                    ? 2.0f * dsp::math::fastSin(dsp::math::PI_F * normalizedCutoff)
                    : filter.coeff;

  float res = resonance * 4.0f; // map 0–1 to Ladder's 0–4 range

  return (filter.drive > 1.001f)
             ? dsp::filters::processLadderNonlinear(input,
                                                    coeff,
                                                    res,
                                                    filter.drive,
                                                    filter.voiceStates[voiceIndex])
             : dsp::filters::processLadder(input, coeff, res, filter.voiceStates[voiceIndex]);
}
} // namespace synth::filters
