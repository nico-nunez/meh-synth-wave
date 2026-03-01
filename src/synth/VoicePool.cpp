#include "VoicePool.h"

#include "synth/NoiseOscillator.h"
#include "synth/WavetableBanks.h"

#include "dsp/Effects.h"
#include "dsp/Math.h"
#include "synth/ParamRanges.h"
#include "synth/WavetableOsc.h"

#include <cstddef>
#include <cstdint>

namespace synth::voices {
using ModSrc = mod_matrix::ModSrc;
using ModDest = mod_matrix::ModDest;
using ModDest2D = mod_matrix::ModDest2D;
using ModRoute = mod_matrix::ModRoute;

namespace osc = wavetable::osc;

// =========================
// VoicePool Configuration
// =========================
void updateVoicePoolConfig(VoicePool& pool, const VoicePoolConfig& config) {
  pool.sampleRate = config.sampleRate;
  pool.invSampleRate = 1.0f / config.sampleRate;

  pool.masterGain = config.masterGain;

  osc::updateConfig(pool.osc1, config.osc1);
  osc::updateConfig(pool.osc2, config.osc2);
  osc::updateConfig(pool.osc3, config.osc3);
  osc::updateConfig(pool.osc4, config.osc4);

  noise_osc::updateConfig(pool.noiseOsc, config.noiseOsc);

  int enabledCount = pool.osc1.enabled + pool.osc2.enabled + pool.osc3.enabled + pool.osc4.enabled +
                     pool.noiseOsc.enabled;
  pool.oscMixGain = (enabledCount > 0) ? 1.0f / static_cast<float>(enabledCount) : 1.0f;

  filters::updateSVFCoefficients(pool.svf, pool.invSampleRate);
  filters::updateLadderCoefficient(pool.ladder, pool.invSampleRate);

  mod_matrix::addRoute(pool.modMatrix, ModSrc::FilterEnv, ModDest::SVFCutoff, 0.0f);
  mod_matrix::addRoute(pool.modMatrix, ModSrc::FilterEnv, ModDest::LadderCutoff, 0.0f);
}

void initVoicePool(VoicePool& pool, const VoicePoolConfig& config) {
  using namespace wavetable::banks;
  initFactoryBanks();

  updateVoicePoolConfig(pool, config);

  // TODO(nico-nunez): find a better way!!!
  pool.osc1.bank = getBankByID(BankID::Saw);
  pool.osc1.detuneAmount = 10.0f;

  pool.osc2.bank = getBankByID(BankID::Saw);
  pool.osc2.mixLevel = 0.7f;
  pool.osc2.octaveOffset = -1;
  pool.osc2.detuneAmount = -10.0f;

  pool.osc3.bank = getBankByID(BankID::Sine);
  pool.osc3.mixLevel = 0.5f;

  pool.osc4.bank = getBankByID(BankID::Sine);
  pool.osc4.octaveOffset = -2;
  pool.osc4.mixLevel = 0.5f;
}

// =========================
//  Voice Allocation
// =========================
// Find free or oldest voice index for voice Initialization
uint32_t allocateVoiceIndex(VoicePool& pool) {
  uint32_t oldestIndex = MAX_VOICES; // out of range
  uint32_t oldestNoteOnTime = UINT32_MAX;

  for (uint32_t i = 0; i < MAX_VOICES; i++) {
    if (!pool.isActive[i]) {
      return i; // return available voice index
    } else {
      if (pool.noteOnTimes[i] < oldestNoteOnTime) {
        oldestNoteOnTime = pool.noteOnTimes[i];
        oldestIndex = i;
      }
    }
  }

  // Need to cleanup otherwise it'll play twice
  // since it'll be added again after initializing voice
  removeInactiveIndex(pool, oldestIndex);

  return oldestIndex;
}

void addActiveIndex(VoicePool& pool, uint32_t voiceIndex) {
  pool.activeIndices[pool.activeCount] = voiceIndex;
  pool.activeCount++;
}

// TODO(nico): this is confusing.  there must be a better way
void removeInactiveIndex(VoicePool& pool, uint32_t voiceIndex) {
  uint32_t removeIndex = pool.activeCount; // out of range of active

  for (uint32_t i = 0; i < pool.activeCount; i++) {
    if (pool.activeIndices[i] == voiceIndex) {
      removeIndex = i;
      break;
    }
  }

  if (removeIndex == pool.activeCount)
    return;

  // Swap current inactive with most recent active
  pool.activeCount--;
  pool.activeIndices[removeIndex] = pool.activeIndices[pool.activeCount];
  pool.isActive[voiceIndex] = 0;
}

// =========================
// Voice Initialization
// =========================

// ==== <Initialization Helpers> ====
namespace {

bool isValidActiveIndex(uint32_t index) {
  return index < MAX_VOICES;
}

uint32_t findVoiceRelease(VoicePool& pool, uint8_t midiNote) {
  for (uint32_t i = 0; i < pool.activeCount; i++) {
    uint32_t voiceIndex = pool.activeIndices[i];
    if (pool.midiNotes[voiceIndex] == midiNote &&
        pool.ampEnv.states[voiceIndex] != envelope::EnvelopeStatus::Release &&
        pool.ampEnv.states[voiceIndex] != envelope::EnvelopeStatus::Idle) {

      return voiceIndex;
    }
  }
  return MAX_VOICES;
}

} // namespace
// ==== </Initialization Helpers> ====

void initializeVoice(VoicePool& pool,
                     uint32_t voiceIndex,
                     uint8_t midiNote,
                     float velocity,
                     uint32_t noteOnTime,
                     float sampleRate) {

  pool.isActive[voiceIndex] = 1;
  pool.midiNotes[voiceIndex] = midiNote;
  pool.noteOnTimes[voiceIndex] = noteOnTime;
  pool.velocities[voiceIndex] = velocity / 127.0f;

  pool.sampleRate = sampleRate;
  pool.invSampleRate = 1.0f / sampleRate;

  for (int d = 0; d < ModDest::DEST_COUNT; d++) {
    pool.modMatrix.prevDestValues[d][voiceIndex] = 0.0f;
    pool.modMatrix.destValues[d][voiceIndex] = 0.0f;
  }
  osc::resetOscModState(pool.oscModState, voiceIndex);

  osc::initOscillator(pool.osc1, voiceIndex, midiNote, sampleRate);
  osc::initOscillator(pool.osc2, voiceIndex, midiNote, sampleRate);
  osc::initOscillator(pool.osc3, voiceIndex, midiNote, sampleRate);
  osc::initOscillator(pool.osc4, voiceIndex, midiNote, sampleRate);

  envelope::initEnvelope(pool.ampEnv, voiceIndex, sampleRate);
  envelope::initEnvelope(pool.filterEnv, voiceIndex, sampleRate);
  envelope::initEnvelope(pool.modEnv, voiceIndex, sampleRate);

  filters::initSVFilter(pool.svf, voiceIndex);
  filters::initLadderFilter(pool.ladder, voiceIndex);
}

void releaseVoice(VoicePool& pool, uint8_t midiNote) {
  uint32_t voiceIndex = findVoiceRelease(pool, midiNote);

  if (!isValidActiveIndex(voiceIndex))
    return;

  envelope::triggerRelease(pool.ampEnv, voiceIndex);
  envelope::triggerRelease(pool.filterEnv, voiceIndex);
  envelope::triggerRelease(pool.modEnv, voiceIndex);
}

// Handle NoteOn Events
void handleNoteOn(VoicePool& pool,
                  uint8_t midiNote,
                  float velocity,
                  uint32_t noteOnTime,
                  float sampleRate) {
  uint32_t voiceIndex = allocateVoiceIndex(pool);

  initializeVoice(pool, voiceIndex, midiNote, velocity, noteOnTime, sampleRate);

  addActiveIndex(pool, voiceIndex);
}

// ===========================
// Voice Processing
// ===========================

namespace {
namespace osc = wavetable::osc;
namespace dsp_wt = dsp::wavetable;

// ==== <Processing Helpers> ====

/* ==== Pre-pass: once per block, once per active voice ====
 * Advance block-rate envelopes (filterEnv, modEnv).
 * ampEnv is NOT advanced here; it runs per-sample in the hot loop below.
 * ==================================================================== */
void preProcessBlock(VoicePool& pool, size_t numSamples) {
  float invNumSamples = 1.0f / static_cast<float>(numSamples);

  mod_matrix::clearModDestSteps(pool.modMatrix);

  for (uint32_t i = pool.activeCount; i > 0; i--) {
    uint32_t voiceIndex = pool.activeIndices[i - 1];

    float modSrcs[ModSrc::SRC_COUNT] = {};

    // NOTE(nico): since this is processed in the main loop it's setting the
    // last value of the PRIOR block on first run; should be fine for now
    modSrcs[ModSrc::AmpEnv] = pool.ampEnv.levels[voiceIndex]; // processed in main loop
    modSrcs[ModSrc::FilterEnv] = envelope::processEnvelope(pool.filterEnv, voiceIndex);
    modSrcs[ModSrc::ModEnv] = envelope::processEnvelope(pool.modEnv, voiceIndex);
    modSrcs[ModSrc::Velocity] = pool.velocities[voiceIndex];

    float modDests[ModDest::DEST_COUNT] = {};
    for (uint8_t r = 0; r < pool.modMatrix.count; r++) {
      const ModRoute& route = pool.modMatrix.routes[r];

      if (route.src == ModSrc::NoSrc || route.dest == ModDest::NoDest)
        continue;

      modDests[route.dest] += modSrcs[route.src] * route.amount;
    }

    // Set modulation destination values
    for (int d = 0; d < ModDest::DEST_COUNT; d++)
      pool.modMatrix.destValues[d][voiceIndex] = modDests[d];

    // Interpolation setup for fast destinations (pitch)
    mod_matrix::setModDestStep(pool.modMatrix, ModDest::Osc1Pitch, voiceIndex, invNumSamples);
    mod_matrix::setModDestStep(pool.modMatrix, ModDest::Osc2Pitch, voiceIndex, invNumSamples);
    mod_matrix::setModDestStep(pool.modMatrix, ModDest::Osc3Pitch, voiceIndex, invNumSamples);
    mod_matrix::setModDestStep(pool.modMatrix, ModDest::Osc4Pitch, voiceIndex, invNumSamples);
  }
};

// Calculate interpolation for pitch increment (hot-loop)
float interpolatePitchInc(WavetableOsc& osc,
                          ModMatrix& matrix,
                          ModDest dest,
                          uint32_t voiceIndex,
                          uint32_t sampleIndex) {
  uint32_t& v = voiceIndex;
  uint32_t& s = sampleIndex;

  float pitchMod =
      matrix.prevDestValues[dest][v] + (matrix.destStepValues[dest][v] * static_cast<float>(s));

  // Calculate and return modulated phase increment
  return osc.phaseIncrements[v] * dsp::math::semitonesToFreqRatio(pitchMod);
}

// Process Oscillators with interpolation and mix (sum) values
float processAndMixOscillators(VoicePool& pool, uint32_t voiceIndex, uint32_t sampleIndex) {
  using param::ranges::osc::clampScanPos;
  using std::max;

  const uint32_t& v = voiceIndex;
  const uint32_t& s = sampleIndex;

  auto& modMatrix = pool.modMatrix;
  auto& destValues = pool.modMatrix.destValues;

  auto& osc1 = pool.osc1;
  auto& osc2 = pool.osc2;
  auto& osc3 = pool.osc3;
  auto& osc4 = pool.osc4;
  auto& oscModState = pool.oscModState;

  float pitchInc1 = interpolatePitchInc(osc1, modMatrix, ModDest::Osc1Pitch, v, s);
  float pitchInc2 = interpolatePitchInc(osc2, modMatrix, ModDest::Osc2Pitch, v, s);
  float pitchInc3 = interpolatePitchInc(osc3, modMatrix, ModDest::Osc3Pitch, v, s);
  float pitchInc4 = interpolatePitchInc(osc4, modMatrix, ModDest::Osc4Pitch, v, s);

  float mip1 = osc::selectMipLevel(pitchInc1 * dsp_wt::TABLE_SIZE_F);
  float mip2 = osc::selectMipLevel(pitchInc2 * dsp_wt::TABLE_SIZE_F);
  float mip3 = osc::selectMipLevel(pitchInc3 * dsp_wt::TABLE_SIZE_F);
  float mip4 = osc::selectMipLevel(pitchInc4 * dsp_wt::TABLE_SIZE_F);

  float scan1 = clampScanPos(osc1.scanPosition + destValues[ModDest::Osc1ScanPos][v]);
  float scan2 = clampScanPos(osc2.scanPosition + destValues[ModDest::Osc2ScanPos][v]);
  float scan3 = clampScanPos(osc3.scanPosition + destValues[ModDest::Osc3ScanPos][v]);
  float scan4 = clampScanPos(osc4.scanPosition + destValues[ModDest::Osc4ScanPos][v]);

  float fmDepth1 = max(0.0f, osc1.fmDepth + destValues[ModDest::Osc1FMDepth][v]);
  float fmDepth2 = max(0.0f, osc2.fmDepth + destValues[ModDest::Osc2FMDepth][v]);
  float fmDepth3 = max(0.0f, osc3.fmDepth + destValues[ModDest::Osc3FMDepth][v]);
  float fmDepth4 = max(0.0f, osc4.fmDepth + destValues[ModDest::Osc4FMDepth][v]);

  float fm1 = osc::getFmSourceValue(oscModState, v, osc1.fmSource) * fmDepth1;
  float fm2 = osc::getFmSourceValue(oscModState, v, osc2.fmSource) * fmDepth2;
  float fm3 = osc::getFmSourceValue(oscModState, v, osc3.fmSource) * fmDepth3;
  float fm4 = osc::getFmSourceValue(oscModState, v, osc4.fmSource) * fmDepth4;

  float out1 = osc::processOscillator(osc1, v, mip1, scan1, fm1, pitchInc1);
  float out2 = osc::processOscillator(osc2, v, mip2, scan2, fm2, pitchInc2);
  float out3 = osc::processOscillator(osc3, v, mip3, scan3, fm3, pitchInc3);
  float out4 = osc::processOscillator(osc4, v, mip4, scan4, fm4, pitchInc4);

  oscModState.osc1[v] = out1;
  oscModState.osc2[v] = out2;
  oscModState.osc3[v] = out3;
  oscModState.osc4[v] = out4;

  // === Mix levels — base + mod delta, clamped ≥ 0 ===
  // Block-rate values; negative mix makes no sense so clamp at 0
  float mix1 = max(0.0f, osc1.mixLevel + destValues[ModDest::Osc1Mix][v]);
  float mix2 = max(0.0f, osc2.mixLevel + destValues[ModDest::Osc2Mix][v]);
  float mix3 = max(0.0f, osc3.mixLevel + destValues[ModDest::Osc3Mix][v]);
  float mix4 = max(0.0f, osc4.mixLevel + destValues[ModDest::Osc4Mix][v]);

  float noiseOut = noise_osc::processNoise(pool.noiseOsc);

  return (out1 * mix1 + out2 * mix2 + out3 * mix3 + out4 * mix4 + noiseOut) * pool.oscMixGain;
  // no separate mix multiplier — already scaled inside processNoise
}

/* ==== Post-block: Update prevDestValues with current value ====
 * Will be referenced at the Pre-pass of the next block
 * Active voices only
 * ============================================================== */
void postProcessBlock(VoicePool& pool) {
  auto& prevDestValues = pool.modMatrix.prevDestValues;
  auto& destValues = pool.modMatrix.destValues;

  for (uint32_t i = 0; i < pool.activeCount; i++) {
    uint32_t v = pool.activeIndices[i]; // voiceIndex

    prevDestValues[ModDest::Osc1Pitch][v] = destValues[ModDest::Osc1Pitch][v];
    prevDestValues[ModDest::Osc2Pitch][v] = destValues[ModDest::Osc2Pitch][v];
    prevDestValues[ModDest::Osc3Pitch][v] = destValues[ModDest::Osc3Pitch][v];
    prevDestValues[ModDest::Osc4Pitch][v] = destValues[ModDest::Osc4Pitch][v];
  }
}

//==== </Processing Helpers> ====
} // namespace

void processVoices(VoicePool& pool, float* output, size_t numSamples) {

  // ==== Set and process Mod Matrix values (block-rate) ====
  preProcessBlock(pool, numSamples);

  // ==== Calculate each sample value (audio-rate) ====
  for (uint32_t sampleIndex = 0; sampleIndex < numSamples; sampleIndex++) {
    float sample = 0.0f;

    // Iterating backwards to more easily deal with swapping voices
    // that have/will become Idle/inactive after processing
    for (uint32_t i = pool.activeCount; i > 0; i--) {
      uint32_t voiceIndex = pool.activeIndices[i - 1];

      float mixedOscs = processAndMixOscillators(pool, voiceIndex, sampleIndex);

      float svfModCutoff =
          filters::computeEffectiveCutoff(pool.svf.cutoff,
                                          pool.modMatrix
                                              .destValues[ModDest::SVFCutoff][voiceIndex]);
      float svfModResonance =
          pool.svf.resonance + pool.modMatrix.destValues[ModDest::SVFResonance][voiceIndex];

      float filtered = filters::processSVFilter(pool.svf,
                                                mixedOscs,
                                                voiceIndex,
                                                svfModCutoff,
                                                svfModResonance,
                                                pool.invSampleRate);
      float ladderModCutoff =
          filters::computeEffectiveCutoff(pool.ladder.cutoff,
                                          pool.modMatrix
                                              .destValues[ModDest::LadderCutoff][voiceIndex]);
      float ladderModResonance =
          pool.ladder.resonance + pool.modMatrix.destValues[ModDest::LadderResonance][voiceIndex];

      filtered = filters::processLadderFilter(pool.ladder,
                                              filtered,
                                              voiceIndex,
                                              ladderModCutoff,
                                              ladderModResonance,
                                              pool.invSampleRate);

      // TODO(nico): Implement Saturator
      // ==== Apply saturation ====
      // filtered = processSaturator(pool.saturator, filtered);

      float ampEnv = envelope::processEnvelope(pool.ampEnv, voiceIndex);

      // Check if amplitude envelope completed - remove immediately
      if (pool.ampEnv.states[voiceIndex] == envelope::EnvelopeStatus::Idle) {
        removeInactiveIndex(pool, voiceIndex);
        // No index adjustment needed - iterating backwards
      }

      sample += filtered * ampEnv * pool.velocities[voiceIndex] * VOICE_GAIN;
    }

    // TODO(nico): Basic soft clip for now.
    // Mainly for protection and not as an effect
    output[sampleIndex] = dsp::effects::softClipFast(sample * pool.masterGain);
  }

  // Increment modulation phases
  postProcessBlock(pool);
}

} // namespace synth::voices
