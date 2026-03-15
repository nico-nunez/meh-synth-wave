#pragma once

#include "dsp/Tempo.h"

namespace synth::tempo {
using dsp::tempo::parseSubdivision;
using dsp::tempo::Subdivision;
using dsp::tempo::subdivisionPeriodSeconds;
using dsp::tempo::subdivisionToString;

struct TempoState {
  float bpm = 120.0f;
};

inline float calcEffectiveRate(Subdivision sub, float bpm) {
  return 1.0f / dsp::tempo::subdivisionPeriodSeconds(sub, bpm);
}

} // namespace synth::tempo
