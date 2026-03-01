#pragma once

namespace dsp::filters {
struct SVFOutputs {
  float lp, bp, hp;
};

// ==== Chamberlin ====
// NOTE(nico): consider calculating per block instead of per sample
// TODO(nico): probably remove this
void updateFilterCoefficients(float& f, float& q, float cutoff, float resonance, float sampleRate);

void processSVF(float input, float cutoff, float resonance, SVFOutputs& state, float sampleRate);

// Access outputs from SVFState (maybe...)
float getLowpass(const SVFOutputs& state);
float getHighpass(const SVFOutputs& state);
float getBandpass(const SVFOutputs& state);

// ==== Cytomic / TPT Form ====
struct SVFState {
  float ic1 = 0.0f; // integrator 1 capacitor state
  float ic2 = 0.0f; // integrator 2 capacitor state
};

struct SVFCoeffs {
  float a1, a2, a3;
  float k; // damping = 1/Q
};

SVFCoeffs computeSVFCoeffs(float cutoff, float Q, float invSampleRate);
SVFOutputs processSVF(float input, const SVFCoeffs& c, SVFState& s);

// ==== Ladder Filter (Moog style) ====
struct LadderState {
  float s[4] = {0, 0, 0, 0};
};

float processLadder(float input, float f, float resonance, LadderState& st);
float processLadderNonlinear(float input, float f, float resonance, float drive, LadderState& st);
} // namespace dsp::filters
