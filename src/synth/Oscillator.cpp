#include "Oscillator.h"

#include "synth/ParamRanges.h"
#include "utils/Utils.h"

#include <cmath>
#include <cstdint>

namespace synth::oscillator {

// Create a new Oscillator with passed config
Oscillator createOscillator(const OscConfig& config) {
  Oscillator osc{};
  updateConfig(osc, config);

  return osc;
}

// =================================
// Initialization and Configuration
// =================================
void initOscillator(Oscillator& osc, uint32_t voiceIndex, uint8_t midiNote, float sampleRate) {

  float freq = utils::midiToFrequency(midiNote) *
               std::pow(2.0f, static_cast<float>(osc.octaveOffset)) *
               std::pow(2.0f, osc.detuneAmount / 1200.0f);

  osc.phases[voiceIndex] = 0.0f;
  osc.phaseIncrements[voiceIndex] = freq / sampleRate;
}

// Helper for updating global settings
void updateConfig(Oscillator& osc, const OscConfig& config) {
  if (osc.detuneAmount != config.detuneAmount)
    osc.detuneAmount = config.detuneAmount;

  if (osc.enabled != config.enabled)
    osc.enabled = config.enabled;

  if (osc.mixLevel != config.mixLevel)
    osc.mixLevel = config.mixLevel;

  if (osc.octaveOffset != config.octaveOffset)
    osc.octaveOffset = config.octaveOffset;

  if (osc.waveform != config.waveform)
    osc.waveform = config.waveform;
}

// TODO(nico): are the following even necessary
// given it's up to the caller to recalculate?
void setWaveformType(Oscillator& osc, WaveformType newType) {
  osc.waveform = newType;
  // NOTE(nico): don't reset active voices or update anything else
  // Caller can do this if they want but shouldn't on active voices (clicks)
}

void setMixLevel(Oscillator& osc, float newLevel) {
  // Ensure values falls within valid range (0 - 4.0)
  if (newLevel < 0.0f) {
    newLevel = 0.0f;
  } else if (newLevel > 4.0f) {
    newLevel = 4.0f;
  }

  osc.mixLevel = newLevel;
}

void setOctiveOffset(Oscillator& osc, int8_t newOffest) {
  osc.octaveOffset = newOffest;
  // NOTE(nico): recalculation needs to be triggerd by caller to update active
  // voices
}

void setDetuneAmount(Oscillator& osc, float newDetuneAmount) {
  osc.detuneAmount = newDetuneAmount;
  // NOTE(nico): recalculation needs to be triggerd by caller to update active
  // voices
}

void toggleEnabled(Oscillator& osc, bool isEnabled) {
  osc.enabled = isEnabled;
}

// =================================
// Processing
// =================================
void incrementPhase(Oscillator& osc, uint32_t voiceIndex) {
  osc.phases[voiceIndex] += osc.phaseIncrements[voiceIndex];

  // Wrap index
  if (osc.phases[voiceIndex] >= 1.0f)
    osc.phases[voiceIndex] -= 1.0f;
}

// Use this if NOT passing pitch and/or mix modulation
float processOscillator(Oscillator& osc, uint32_t voiceIndex) {
  float sample = dsp::waveforms::processWaveform(osc.waveform,
                                                 osc.phases[voiceIndex],
                                                 osc.phaseIncrements[voiceIndex]) *
                 osc.mixLevel;

  incrementPhase(osc, voiceIndex);

  return sample;
}

// Use this if passing pitch and/or mix modulation values
// NOTE(nico): values are expected to be calculated by caller
float processOscillator(Oscillator& osc,
                        uint32_t voiceIndex,
                        float phaseIncrement,
                        float mixLevel) {
  float sample =
      dsp::waveforms::processWaveform(osc.waveform, osc.phases[voiceIndex], phaseIncrement) *
      param::ranges::osc::clampMixLevel(mixLevel);

  // Advance using the modulated increment, not the stored one
  osc.phases[voiceIndex] += phaseIncrement;
  if (osc.phases[voiceIndex] >= 1.0f)
    osc.phases[voiceIndex] -= 1.0f;

  return sample;
}

} // namespace synth::oscillator
