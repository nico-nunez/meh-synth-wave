#include "WavetableOsc.h"

#include "dsp/Math.h"
#include "utils/Utils.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace synth::wavetable::osc {
namespace dsp_wt = dsp::wavetable;

// ================================
// Initialization
// ================================
void initOscillator(WavetableOscillator& osc,
                    uint32_t voiceIndex,
                    uint8_t midiNote,
                    float sampleRate) {
  float offsetExp = static_cast<float>(osc.octaveOffset) + (osc.detuneAmount / 1200.0f);
  float freq = utils::midiToFrequency(midiNote) * std::exp2f(offsetExp);

  osc.phases[voiceIndex] = 0.0f;
  osc.phaseIncrements[voiceIndex] = freq / sampleRate;
}

// ================================
// Configuration
// ================================
void updateConfig(WavetableOscillator& osc, const WavetableOscConfig& config) {
  osc.bank = config.bank;
  osc.scanPos = config.scanPos;
  osc.mixLevel = config.mixLevel;
  osc.fmDepth = config.fmDepth;
  osc.fmSource = config.fmSource;
  osc.octaveOffset = config.octaveOffset;
  osc.detuneAmount = config.detuneAmount;
  osc.enabled = config.enabled;
}

/* ================================
 * Mip selection
 * ================================
 * Returns a continuous float mip level. The integer part selects mip table A;
 * integer+1 selects mip table B. The fractional part is the blend weight.
 *
 * Relationship: one octave up = phase increment doubles = mip level +1.
 * So mip = log2(phaseIncrement) gives a continuous float that tracks pitch.
 * fastLog2 gives this in O(1) with ~0.01 error — well within audible tolerance.
 *
 * MUST be in TABLE_SIZE units (table positions/sample)
 */
float selectMipLevel(float phaseIncrement) {
  if (phaseIncrement <= 1.0f)
    return 0.0f;

  float mip = dsp::math::fastLog2(phaseIncrement);

  // Clamped to MAX_MIP_LEVELS-2 so mipB = floor(mip)+1 never goes out of bounds
  return std::clamp(mip, 0.0f, float(dsp_wt::MAX_MIP_LEVELS - 2));
}

/* ================================
 * Table read — dual-mip linear interpolation
 * ================================
 *
 * Two reads per mip level (linear interp within table) × two mip levels = 4
 * reads/sample. Same cost as cubic at a single mip, but correct under pitch
 * modulation and free of mip transition artifacts. See Design Decisions for the
 * full trade-off explanation.
 *
 * FM phase offset: float in cycles (same units as osc.phases[v]). Added to the
 * normalized phase and wrapped via floorf — handles any magnitude and sign in a
 * single instruction.
 *
 * Frame interpolation: scanF maps [0,1] onto [0, frameCount-1]. Linear blend
 * between frameA and frameB is sufficient — morphing is perceptually smooth at
 * block-rate.
 */
float readWavetable(const WavetableOscillator& osc,
                    uint32_t voiceIndex,
                    float mipF,
                    float effectiveScanPos,
                    float fmPhaseOffset) {
  if (!osc.enabled || osc.bank == nullptr)
    return 0.0f;

  // Apply FM offset and wrap to [0, 1.0).
  // floorf handles any magnitude and sign — the reason phases are normalized.
  // Convert to table-space once here; all readTable calls below use tablePhase.
  float rawPhase = osc.phases[voiceIndex] + fmPhaseOffset;
  float wrappedPhase = rawPhase - floorf(rawPhase);

  float tablePhase = wrappedPhase * dsp_wt::TABLE_SIZE_F; // [0, TABLE_SIZE)

  // Mip blend
  int mA = int(mipF);
  int mB = mA + 1;
  float mFrac = mipF - float(mA);

  // Single-frame fast path
  if (osc.bank->frameCount == 1) {
    float sA = dsp_wt::readTable(osc.bank->frames[0].mips[mA], tablePhase);
    float sB = dsp_wt::readTable(osc.bank->frames[0].mips[mB], tablePhase);
    return sA + mFrac * (sB - sA);
  }

  // Multi-frame: interpolate between adjacent frames
  float scanF = effectiveScanPos * float(osc.bank->frameCount - 1);

  int frameA = std::min(static_cast<int>(scanF), static_cast<int>(osc.bank->frameCount) - 2);
  int frameB = frameA + 1;
  float fFrac = scanF - float(frameA);

  // 4 table reads — (2 frames) × (2 mip levels), then blend along both axes
  float fAmA = dsp_wt::readTable(osc.bank->frames[frameA].mips[mA], tablePhase);
  float fBmA = dsp_wt::readTable(osc.bank->frames[frameB].mips[mA], tablePhase);
  float fAmB = dsp_wt::readTable(osc.bank->frames[frameA].mips[mB], tablePhase);
  float fBmB = dsp_wt::readTable(osc.bank->frames[frameB].mips[mB], tablePhase);

  float sA = fAmA + fFrac * (fBmA - fAmA); // frame lerp at mip A
  float sB = fAmB + fFrac * (fBmB - fAmB); // frame lerp at mip B
  return sA + mFrac * (sB - sA);           // mip lerp
}

float processOscillator(WavetableOscillator& osc,
                        uint32_t voiceIndex,
                        float mipF,
                        float effectiveScanPos,
                        float fmPhaseOffset,
                        float pitchIncrement) {
  float sample = readWavetable(osc, voiceIndex, mipF, effectiveScanPos, fmPhaseOffset);

  osc.phases[voiceIndex] += pitchIncrement;
  osc.phases[voiceIndex] -= floorf(osc.phases[voiceIndex]);

  return sample;
}

// =========================
// FM Modulation
// =========================

void resetOscModState(WavetableOscModState& modState, uint32_t voiceIndex) {
  modState.osc1[voiceIndex] = 0.0f;
  modState.osc2[voiceIndex] = 0.0f;
  modState.osc3[voiceIndex] = 0.0f;
  modState.osc4[voiceIndex] = 0.0f;

  modState.osc1Feedback[voiceIndex] = 0.0f;
  modState.osc2Feedback[voiceIndex] = 0.0f;
  modState.osc3Feedback[voiceIndex] = 0.0f;
  modState.osc4Feedback[voiceIndex] = 0.0f;
}

float getFmInputValue(WavetableOscModState& modState,
                      uint32_t voiceIndex,
                      FMSource src,
                      FMSource carrier) {
  switch (src) {
  case FMSource::Osc1: {
    if (carrier == FMSource::Osc1) {
      float fb = 0.5f * (modState.osc1[voiceIndex] + modState.osc1Feedback[voiceIndex]);
      modState.osc1Feedback[voiceIndex] = modState.osc1[voiceIndex];
      return fb;
    }
    return modState.osc1[voiceIndex];
  }
  case FMSource::Osc2: {
    if (carrier == FMSource::Osc2) {
      float fb = 0.5f * (modState.osc2[voiceIndex] + modState.osc2Feedback[voiceIndex]);
      modState.osc2Feedback[voiceIndex] = modState.osc2[voiceIndex];
      return fb;
    }
    return modState.osc2[voiceIndex];
  }
  case FMSource::Osc3: {
    if (carrier == FMSource::Osc3) {
      float fb = 0.5f * (modState.osc3[voiceIndex] + modState.osc3Feedback[voiceIndex]);
      modState.osc3Feedback[voiceIndex] = modState.osc3[voiceIndex];
      return fb;
    }
    return modState.osc3[voiceIndex];
  }
  case FMSource::Osc4: {
    if (carrier == FMSource::Osc4) {
      float fb = 0.5f * (modState.osc4[voiceIndex] + modState.osc4Feedback[voiceIndex]);
      modState.osc4Feedback[voiceIndex] = modState.osc4[voiceIndex];
      return fb;
    }
    return modState.osc4[voiceIndex];
  }
  default:
    return 0.0f;
  }
}
} // namespace synth::wavetable::osc
