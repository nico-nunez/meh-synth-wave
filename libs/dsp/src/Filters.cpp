#include "dsp/Filters.h"
#include "dsp/Math.h"

#include <algorithm>
#include <cmath>

namespace dsp::filters {

// ==== Chamberlin (artifacts as it nears Nyquist) ====
// NOTE(nico): consider calculating per block instead of per sample
void updateFilterCoefficients(float& f, float& q, float cutoff, float resonance, float sampleRate) {
  // Clamp parameters to stable range
  cutoff = std::clamp(cutoff, 20.0f, sampleRate * 0.45f);
  resonance = std::clamp(resonance, 0.0f, 0.99f);

  // Calculate filter coefficients
  f = 2.0f * std::sin(math::PI_F * cutoff / sampleRate);
  q = 1.0f - resonance;
}

void processSVF(float sample, float f, float q, SVFOutputs& state) {
  // Process state-variable filter
  state.lp += f * state.bp;
  state.hp = sample - state.lp - q * state.bp;
  state.bp += f * state.hp;
}

float getLowpass(const SVFOutputs& state) {
  return state.lp;
}

float getHighpass(const SVFOutputs& state) {
  return state.hp;
}

float getBandpass(const SVFOutputs& state) {
  return state.bp;
}

// ==== Cytomic/TPT Form (fixes issue with Chamberlin) ====

// Call when cutoff or resonance changes — NOT per sample
SVFCoeffs computeSVFCoeffs(float cutoff, float Q, float invSampleRate) {
  /* TODO(nico): optimize this
   * - std::tan() polynomial approximation?
   */
  float g = std::tan(math::PI_F * cutoff * invSampleRate); // pre-warped frequency
  float k = 1.0f / Q;
  float a1 = 1.0f / (1.0f + g * (g + k));
  float a2 = g * a1;
  float a3 = g * a2;
  return {a1, a2, a3, k};
}

SVFOutputs processSVF(float input, const SVFCoeffs& c, SVFState& s) {
  float v3 = input - s.ic2;
  float v1 = c.a1 * s.ic1 + c.a2 * v3;
  float v2 = s.ic2 + c.a2 * s.ic1 + c.a3 * v3;

  s.ic1 = 2.0f * v1 - s.ic1;
  s.ic2 = 2.0f * v2 - s.ic2;

  return {v2, v1, input - c.k * v1 - v2};
}

// ==== Ladder Filter (Moog Style) ====
// resonance: 0 (no resonance) to 4 (self-oscillation)
// f: 2 * std::sin(M_PI * cutoff / sampleRate)
float processLadder(float input, float f, float resonance, LadderState& st) {
  float feedback = resonance * st.s[3];
  float x = input - feedback;

  st.s[0] += f * (x - st.s[0]);
  st.s[1] += f * (st.s[0] - st.s[1]);
  st.s[2] += f * (st.s[1] - st.s[2]);
  st.s[3] += f * (st.s[2] - st.s[3]);

  return st.s[3];
}

float processLadderNonlinear(float input, float f, float resonance, float drive, LadderState& st) {
  // Nonlinear feedback — tanh prevents harsh blowup at high resonance
  float feedback = resonance * std::tanh(st.s[3]);

  // Drive into the input — saturates before the filter stages
  float x = std::tanh(drive * input - feedback);

  st.s[0] += f * (x - st.s[0]);
  st.s[1] += f * (st.s[0] - st.s[1]);
  st.s[2] += f * (st.s[1] - st.s[2]);
  st.s[3] += f * (st.s[2] - st.s[3]);

  return st.s[3];
}
} // namespace dsp::filters
