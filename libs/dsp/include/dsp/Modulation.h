#pragma once

#include "Waveforms.h"

namespace dsp::modulation {
// Generate LFO waveform at given phase
float processLFO(float phase, waveforms::WaveformType type, float pulseWidth = 0.5f);

// Modulation curves (for velocity, mod wheel, etc.)
float exponentialCurve(float linear); // 0-1 → exponential curve
float logarithmicCurve(float linear); // 0-1 → logarithmic curve

} // namespace dsp::modulation
