#include "VoicePool.h"

#include "synth/NoiseOscillator.h"
#include "synth/WavetableBanks.h"

#include "dsp/Effects.h"
#include "dsp/Math.h"
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
void updateVoicePoolConfig(VoicePool &pool, const VoicePoolConfig &config) {
  pool.sampleRate = config.sampleRate;
  pool.invSampleRate = 1.0f / config.sampleRate;

  pool.masterGain = config.masterGain;

  osc::updateConfig(pool.osc1, config.osc1);
  osc::updateConfig(pool.osc2, config.osc2);
  osc::updateConfig(pool.osc3, config.osc3);
  osc::updateConfig(pool.subOsc, config.subOsc);

  noise_osc::updateConfig(pool.noiseOsc, config.noiseOsc);

  int enabledCount = pool.osc1.enabled + pool.osc2.enabled + pool.osc3.enabled +
                     pool.subOsc.enabled + pool.noiseOsc.enabled;
  pool.oscMixGain =
      (enabledCount > 0) ? 1.0f / static_cast<float>(enabledCount) : 1.0f;

  filters::updateSVFCoefficients(pool.svf, pool.invSampleRate);
  filters::updateLadderCoefficient(pool.ladder, pool.invSampleRate);

  mod_matrix::addRoute(pool.modMatrix, ModSrc::FilterEnv, ModDest::SVFCutoff,
                       0.0f);
  mod_matrix::addRoute(pool.modMatrix, ModSrc::FilterEnv, ModDest::LadderCutoff,
                       0.0f);
}

void initVoicePool(VoicePool &pool, const VoicePoolConfig &config) {
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

  pool.subOsc.bank = getBankByID(BankID::Sine);
  pool.subOsc.octaveOffset = -2;
  pool.subOsc.mixLevel = 0.5f;
}

// =========================
//  Voice Allocation
// =========================
// Find free or oldest voice index for voice Initialization
uint32_t allocateVoiceIndex(VoicePool &pool) {
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

void addActiveIndex(VoicePool &pool, uint32_t voiceIndex) {
  pool.activeIndices[pool.activeCount] = voiceIndex;
  pool.activeCount++;
}

// TODO(nico): this is confusing.  there must be a better way
void removeInactiveIndex(VoicePool &pool, uint32_t voiceIndex) {
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

bool isValidActiveIndex(uint32_t index) { return index < MAX_VOICES; }

uint32_t findVoiceRelease(VoicePool &pool, uint8_t midiNote) {
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

void initializeVoice(VoicePool &pool, uint32_t voiceIndex, uint8_t midiNote,
                     float velocity, uint32_t noteOnTime, float sampleRate) {
  // ==== Set Metadata ====
  pool.isActive[voiceIndex] = 1;
  pool.midiNotes[voiceIndex] = midiNote;
  pool.noteOnTimes[voiceIndex] = noteOnTime;
  pool.velocities[voiceIndex] = velocity / 127.0f;

  pool.sampleRate = sampleRate;
  pool.invSampleRate = 1.0f / sampleRate;

  // ==== Reset Modulation Destination Values ====
  for (int d = 0; d < ModDest::DEST_COUNT; d++) {
    pool.modMatrix.prevDestValues[d][voiceIndex] = 0.0f;
    pool.modMatrix.destValues[d][voiceIndex] = 0.0f;
  }

  // ==== Initialize Oscillator 1 ====
  osc::initOscillator(pool.osc1, voiceIndex, midiNote, sampleRate);

  // ==== Initialize Oscillator 2 ====
  osc::initOscillator(pool.osc2, voiceIndex, midiNote, sampleRate);

  // ==== Initialize Oscillator 3 ====
  osc::initOscillator(pool.osc3, voiceIndex, midiNote, sampleRate);

  // ==== Initialize Sub Oscillator ====
  osc::initOscillator(pool.subOsc, voiceIndex, midiNote, sampleRate);

  // ==== Initialize Envelopes ====
  // Amp envelope
  envelope::initEnvelope(pool.ampEnv, voiceIndex, sampleRate);

  // Filter envelope
  envelope::initEnvelope(pool.filterEnv, voiceIndex, sampleRate);

  // Mod envelope
  envelope::initEnvelope(pool.modEnv, voiceIndex, sampleRate);

  // ==== Initialize Filter States ====
  filters::initSVFilter(pool.svf, voiceIndex);
  filters::initLadderFilter(pool.ladder, voiceIndex);
}

void releaseVoice(VoicePool &pool, uint8_t midiNote) {
  uint32_t voiceIndex = findVoiceRelease(pool, midiNote);

  if (!isValidActiveIndex(voiceIndex))
    return;

  envelope::triggerRelease(pool.ampEnv, voiceIndex);
  envelope::triggerRelease(pool.filterEnv, voiceIndex);
  envelope::triggerRelease(pool.modEnv, voiceIndex);
}

// Handle NoteOn Events
void handleNoteOn(VoicePool &pool, uint8_t midiNote, float velocity,
                  uint32_t noteOnTime, float sampleRate) {
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
void preProcessBlock(VoicePool &pool, size_t numSamples) {
  float invNumSamples = 1.0f / static_cast<float>(numSamples);

  mod_matrix::clearModDestSteps(pool.modMatrix);

  for (uint32_t i = pool.activeCount; i > 0; i--) {
    uint32_t voiceIndex = pool.activeIndices[i - 1];

    // Set initial modulation source values
    float modSrcs[ModSrc::SRC_COUNT] = {};

    // NOTE(nico): since this is processed in the main loop it's setting the
    // last value of the PRIOR block on first run; should be fine for now
    modSrcs[ModSrc::AmpEnv] =
        pool.ampEnv.levels[voiceIndex]; // processed in main loop

    modSrcs[ModSrc::FilterEnv] =
        envelope::processEnvelope(pool.filterEnv, voiceIndex);

    modSrcs[ModSrc::ModEnv] =
        envelope::processEnvelope(pool.modEnv, voiceIndex);

    modSrcs[ModSrc::Velocity] = pool.velocities[voiceIndex];

    // Accumulate mod destinations
    float modDests[ModDest::DEST_COUNT] = {};

    for (uint8_t r = 0; r < pool.modMatrix.count; r++) {
      const ModRoute &route = pool.modMatrix.routes[r];

      if (route.src == ModSrc::NoSrc || route.dest == ModDest::NoDest)
        continue;

      modDests[route.dest] += modSrcs[route.src] * route.amount;
    }

    // Set modulation destination values
    for (int d = 0; d < ModDest::DEST_COUNT; d++)
      pool.modMatrix.destValues[d][voiceIndex] = modDests[d];

    // Interpolation setup for fast destinations (pitch)
    mod_matrix::setModDestStep(pool.modMatrix, ModDest::Osc1Pitch, voiceIndex,
                               invNumSamples);
    mod_matrix::setModDestStep(pool.modMatrix, ModDest::Osc2Pitch, voiceIndex,
                               invNumSamples);
    mod_matrix::setModDestStep(pool.modMatrix, ModDest::Osc3Pitch, voiceIndex,
                               invNumSamples);
    mod_matrix::setModDestStep(pool.modMatrix, ModDest::SubOscPitch, voiceIndex,
                               invNumSamples);
  }
};

// Calculate interpolation for pitch increment (hot-loop)
float interpolatePitchInc(WavetableOsc &osc, ModMatrix &matrix, ModDest dest,
                          uint32_t voiceIndex, uint32_t sampleIndex) {
  // Calculate pitch modulation
  float pitchMod = matrix.prevDestValues[dest][voiceIndex] +
                   (matrix.destStepValues[dest][voiceIndex] *
                    static_cast<float>(sampleIndex));

  // Calculate and return modulated phase increment
  return osc.phaseIncrements[voiceIndex] *
         dsp::math::semitonesToFreqRatio(pitchMod);
}

// Process Oscillators with interpolation and mix (sum) values
float processAndMixOscillators(VoicePool &pool, uint32_t voiceIndex,
                               uint32_t sampleIndex) {

  // ==== Pitch: per-sample interpolation ====
  float pitchInc1 = interpolatePitchInc(
      pool.osc1, pool.modMatrix, ModDest::Osc1Pitch, voiceIndex, sampleIndex);
  float pitchInc2 = interpolatePitchInc(
      pool.osc2, pool.modMatrix, ModDest::Osc2Pitch, voiceIndex, sampleIndex);
  float pitchInc3 = interpolatePitchInc(
      pool.osc3, pool.modMatrix, ModDest::Osc3Pitch, voiceIndex, sampleIndex);
  float pitchIncSub =
      interpolatePitchInc(pool.subOsc, pool.modMatrix, ModDest::SubOscPitch,
                          voiceIndex, sampleIndex);

  // ==== Mip level - per-sample from interpolated pitch ====
  float mip1 = osc::selectMipLevel(pitchInc1 * dsp_wt::TABLE_SIZE_F);
  float mip2 = osc::selectMipLevel(pitchInc2 * dsp_wt::TABLE_SIZE_F);
  float mip3 = osc::selectMipLevel(pitchInc3 * dsp_wt::TABLE_SIZE_F);
  float mipSub = osc::selectMipLevel(pitchIncSub * dsp_wt::TABLE_SIZE_F);

  // ==== Scan position ====
  float scan1 = std::clamp(
      pool.osc1.scanPosition +
          pool.modMatrix.destValues[ModDest::Osc1ScanPos][voiceIndex],
      0.0f, 1.0f);
  float scan2 = std::clamp(
      pool.osc2.scanPosition +
          pool.modMatrix.destValues[ModDest::Osc2ScanPos][voiceIndex],
      0.0f, 1.0f);
  float scan3 = std::clamp(
      pool.osc3.scanPosition +
          pool.modMatrix.destValues[ModDest::Osc3ScanPos][voiceIndex],
      0.0f, 1.0f);
  float scanSub = std::clamp(
      pool.subOsc.scanPosition +
          pool.modMatrix.destValues[ModDest::SubOscScanPos][voiceIndex],
      0.0f, 1.0f);

  // ==== Read oscillators — fmPhaseOffset = 0.0f until Step 6 ====
  float out1 = osc::processOscillator(pool.osc1, voiceIndex, mip1, scan1, 0.0f,
                                      pitchInc1);
  float out2 = osc::processOscillator(pool.osc2, voiceIndex, mip2, scan2, 0.0f,
                                      pitchInc2);
  float out3 = osc::processOscillator(pool.osc3, voiceIndex, mip3, scan3, 0.0f,
                                      pitchInc3);
  float outSub = osc::processOscillator(pool.subOsc, voiceIndex, mipSub,
                                        scanSub, 0.0f, pitchIncSub);

  // === Mix levels — base + mod delta, clamped ≥ 0 ===
  // Block-rate values; negative mix makes no sense so clamp at 0
  float mix1 = std::max(
      0.0f, pool.osc1.mixLevel +
                pool.modMatrix.destValues[ModDest::Osc1Mix][voiceIndex]);
  float mix2 = std::max(
      0.0f, pool.osc2.mixLevel +
                pool.modMatrix.destValues[ModDest::Osc2Mix][voiceIndex]);
  float mix3 = std::max(
      0.0f, pool.osc3.mixLevel +
                pool.modMatrix.destValues[ModDest::Osc3Mix][voiceIndex]);
  float mixS = std::max(
      0.0f, pool.subOsc.mixLevel +
                pool.modMatrix.destValues[ModDest::SubOscMix][voiceIndex]);

  // ==== Noise — applies mixLevel internally ====
  float noiseOut = noise_osc::processNoise(pool.noiseOsc);

  return (out1 * mix1 + out2 * mix2 + out3 * mix3 + outSub * mixS +
          noiseOut) // no separate mix multiplier — already scaled inside
                    // processNoise
         * pool.oscMixGain;
}

/* ==== Post-block: Update prevDestValues with current value ====
 * Will be referenced at the Pre-pass of the next block
 * Active voices only
 * ============================================================== */
void postProcessBlock(VoicePool &pool) {
  for (uint32_t i = 0; i < pool.activeCount; i++) {
    uint32_t v = pool.activeIndices[i];
    pool.modMatrix.prevDestValues[ModDest::Osc1Pitch][v] =
        pool.modMatrix.destValues[ModDest::Osc1Pitch][v];

    pool.modMatrix.prevDestValues[ModDest::Osc2Pitch][v] =
        pool.modMatrix.destValues[ModDest::Osc2Pitch][v];

    pool.modMatrix.prevDestValues[ModDest::Osc3Pitch][v] =
        pool.modMatrix.destValues[ModDest::Osc3Pitch][v];

    pool.modMatrix.prevDestValues[ModDest::SubOscPitch][v] =
        pool.modMatrix.destValues[ModDest::SubOscPitch][v];
  }
}

//==== </Processing Helpers> ====
} // namespace

void processVoices(VoicePool &pool, float *output, size_t numSamples) {

  // ==== Set and process Mod Matrix values (per-block) ====
  preProcessBlock(pool, numSamples);

  // ==== Calculate each sample value (per sample) ====
  for (uint32_t sampleIndex = 0; sampleIndex < numSamples; sampleIndex++) {
    float sample = 0.0f;

    // Iterating backwards to more easily deal with swapping voices
    // that have/will become Idle/inactive after processing
    for (uint32_t i = pool.activeCount; i > 0; i--) {
      uint32_t voiceIndex = pool.activeIndices[i - 1];

      // Process osc1, osc2, osc3, and subOsc
      // interpolate modulation values and mix
      float mixedOscs = processAndMixOscillators(pool, voiceIndex, sampleIndex);

      // Process SVF with modulation
      float svfModCutoff = filters::computeEffectiveCutoff(
          pool.svf.cutoff,
          pool.modMatrix.destValues[ModDest::SVFCutoff][voiceIndex]);
      float svfModResonance =
          pool.svf.resonance +
          pool.modMatrix.destValues[ModDest::SVFResonance][voiceIndex];

      // Process SVF Filter
      float filtered = filters::processSVFilter(pool.svf, mixedOscs, voiceIndex,
                                                svfModCutoff, svfModResonance,
                                                pool.invSampleRate);

      // Process Ladder with modulation
      float ladderModCutoff = filters::computeEffectiveCutoff(
          pool.ladder.cutoff,
          pool.modMatrix.destValues[ModDest::LadderCutoff][voiceIndex]);
      float ladderModResonance =
          pool.ladder.resonance +
          pool.modMatrix.destValues[ModDest::LadderResonance][voiceIndex];
      // Process Ladder Filter
      filtered = filters::processLadderFilter(
          pool.ladder, filtered, voiceIndex, ladderModCutoff,
          ladderModResonance, pool.invSampleRate);

      // TODO(nico): Implement Saturator
      // ==== Apply saturation ====
      // filtered = processSaturator(pool.saturator, filtered);

      // Process Amp Envelope
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
