#include "Unison.h"

#include "dsp/Math.h"

namespace synth::unison {

void initUnisonSubPhases(UnisonState& unison, uint32_t voiceIndex) {
  for (uint8_t u = 0; u < MAX_UNISON_VOICES; u++) {
    unison.subPhases[0][u][voiceIndex] = 0.0f;
    unison.subPhases[1][u][voiceIndex] = 0.0f;
    unison.subPhases[2][u][voiceIndex] = 0.0f;
    unison.subPhases[3][u][voiceIndex] = 0.0f;
  }
}

void updateDetuneOffsets(unison::UnisonState& u) {
  dsp::math::computeDetuneOffsets(u.detuneOffsets, u.voices, u.detune);
  for (int8_t i = 0; i < u.voices; i++) {
    u.detuneRatios[i] = dsp::math::semitonesToFreqRatio(u.detuneOffsets[i]);
  }
}

void updatePanPositions(unison::UnisonState& u) {
  dsp::math::computePanPositions(u.panPositions, u.voices, u.spread);
  for (int8_t i = 0; i < u.voices; i++) {
    dsp::math::panToLR(u.panPositions[i], u.panGainsL[i], u.panGainsR[i]);
  }
}

void updateGainComp(unison::UnisonState& u) {
  u.gainComp = dsp::math::unisonGainComp(u.voices);
}

} // namespace synth::unison
