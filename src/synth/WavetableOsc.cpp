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
void initOscillator(WavetableOscillator &osc, uint32_t voiceIndex,
                    uint8_t midiNote, float sampleRate) {
  float offsetExp =
      static_cast<float>(osc.octaveOffset) + (osc.detuneAmount / 1200.0f);

  float freq = utils::midiToFrequency(midiNote) * std::exp2f(offsetExp);

  osc.phases[voiceIndex] = 0u;
  osc.phaseIncrements[voiceIndex] =
      static_cast<float>(dsp_wt::TABLE_SIZE) * freq / sampleRate;
}

// ================================
// Configuration
// ================================
void updateConfig(WavetableOscillator &osc, const WavetableOscConfig &config) {
  osc.bank = config.bank;
  osc.scanPosition = config.scanPosition;
  osc.mixLevel = config.mixLevel;
  osc.fmDepth = config.fmDepth;
  osc.fmSource = config.fmSource;
  osc.octaveOffset = config.octaveOffset;
  osc.detuneAmount = config.detuneAmount;
  osc.enabled = config.enabled;
}

// ================================
// Mip selection
// ================================
//
// Returns a continuous float mip level. The integer part selects mip table A;
// integer+1 selects mip table B. The fractional part is the blend weight.
//
// Relationship: one octave up = phase increment doubles = mip level +1.
// So mip = log2(phaseIncrement) gives a continuous float that tracks pitch.
// fastLog2 gives this in O(1) with ~0.01 error — well within audible tolerance.

float selectMipLevel(float phaseIncrement) {
  if (phaseIncrement <= 1.0f)
    return 0.0f;

  float mip = dsp::math::fastLog2(phaseIncrement);

  // Clamped to MAX_MIP_LEVELS-2 so mipB = floor(mip)+1 never goes out of bounds
  return std::clamp(mip, 0.0f, float(dsp_wt::MAX_MIP_LEVELS - 2));
}

// ================================
// Table read — dual-mip linear interpolation
// ================================
//
// Two reads per mip level (linear interp within table) × two mip levels = 4
// reads/sample. Same cost as cubic at a single mip, but correct under pitch
// modulation and free of mip transition artifacts. See Design Decisions for the
// full trade-off explanation.
//
// FM phase offset: fixed-point uint32_t added directly to phase — wraps via
// unsigned overflow, handles negative displacements correctly via two's
// complement.
//
// Frame interpolation: scanF maps [0,1] onto [0, frameCount-1]. Linear blend
// between frameA and frameB is sufficient — morphing is perceptually smooth at
// block-rate.

float readWavetable(const WavetableOscillator &osc, uint32_t voiceIndex,
                    float mipF, float effectiveScanPos,
                    uint32_t fmPhaseOffset) {
  if (!osc.enabled || osc.bank == nullptr)
    return 0.0f;

  // Apply FM phase offset — unsigned add wraps automatically
  uint32_t readPhase = osc.phases[voiceIndex] + fmPhaseOffset;

  // Mip blend
  int mA = int(mipF);
  int mB = mA + 1;
  float mFrac = mipF - float(mA);

  // Single-frame fast path
  if (osc.bank->frameCount == 1) {
    float sA = dsp_wt::readTable(osc.bank->frames[0].mips[mA], readPhase);
    float sB = dsp_wt::readTable(osc.bank->frames[0].mips[mB], readPhase);
    return sA + mFrac * (sB - sA);
  }

  // Multi-frame: interpolate between adjacent frames
  float scanF = effectiveScanPos * float(osc.bank->frameCount - 1);
  uint32_t frameA =
      std::min(static_cast<uint32_t>(scanF), osc.bank->frameCount - 2);
  uint32_t frameB = frameA + 1;
  float fFrac = scanF - float(frameA);

  // 4 table reads — (2 frames) × (2 mip levels), then blend along both axes
  float fAmA = dsp_wt::readTable(osc.bank->frames[frameA].mips[mA], readPhase);
  float fBmA = dsp_wt::readTable(osc.bank->frames[frameB].mips[mA], readPhase);
  float fAmB = dsp_wt::readTable(osc.bank->frames[frameA].mips[mB], readPhase);
  float fBmB = dsp_wt::readTable(osc.bank->frames[frameB].mips[mB], readPhase);

  float sA = fAmA + fFrac * (fBmA - fAmA); // frame lerp at mip A
  float sB = fAmB + fFrac * (fBmB - fAmB); // frame lerp at mip B
  return sA + mFrac * (sB - sA);           // mip lerp
}

} // namespace synth::wavetable::osc
