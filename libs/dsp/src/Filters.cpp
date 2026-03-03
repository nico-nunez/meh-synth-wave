#include "dsp/Filters.h"
#include "dsp/Math.h"

#include <cassert>

namespace dsp::filters {

// ==== Cytomic/TPT Form (fixes issue with Chamberlin) ====
//
float modulateCutoff(float baseCutoff, float modulationOctaves) {
  return baseCutoff * dsp::math::fastExp2(modulationOctaves);
}

// Call when cutoff or resonance changes — NOT per sample
SVFCoeffs computeSVFCoeffs(float normalizedFreq, float Q) {
  assert(normalizedFreq < 0.5f);

  float g = math::fastTan(math::PI_F * normalizedFreq);
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
// f: 2 * math::fastSin(M_PI * normalizedFreq)
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
  float feedback = resonance * math::fastTanh(st.s[3]);

  // Drive into the input — saturates before the filter stages
  float x = math::fastTanh(drive * input - feedback);

  st.s[0] += f * (x - st.s[0]);
  st.s[1] += f * (st.s[0] - st.s[1]);
  st.s[2] += f * (st.s[1] - st.s[2]);
  st.s[3] += f * (st.s[2] - st.s[3]);

  return st.s[3];
}

float dcBlock(float sample, float& state, float coefficient) {
  float output = sample - state;
  state = sample * (1.0f - coefficient) + state * coefficient;
  return output;
}

} // namespace dsp::filters
