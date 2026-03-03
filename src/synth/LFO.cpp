#include "LFO.h"
#include "dsp/Math.h"

namespace synth::lfo {

void clearContribs(LFOModState& modState) {
  for (auto& destContrib : modState.contribs)
    destContrib = 0.0f;
}

float advanceLFO(LFO& lfo, float invSampleRate, float effectiveRate, float effectiveAmplitude) {
  lfo.phase += effectiveRate * invSampleRate;

  bool wrapped = lfo.phase >= 1.0f;
  if (wrapped)
    lfo.phase -= 1.0f;

  // S&H: null bank is the sentinel — see Sample and Hold
  if (lfo.bank == nullptr) {
    if (wrapped)
      lfo.shHeld = dsp::math::randNoiseValue();
    return effectiveAmplitude * lfo.shHeld;
  }

  float tablePhase = lfo.phase * dsp::wavetable::TABLE_SIZE_F;
  return effectiveAmplitude * dsp::wavetable::readTable(lfo.bank->frames[0].mips[0], tablePhase);
}
} // namespace synth::lfo
