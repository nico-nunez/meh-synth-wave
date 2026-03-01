#pragma once

#include "Types.h"

#include "dsp/Wavetable.h"

#include <cstdint>

namespace synth::wavetable::osc {

using WavetableBank = dsp::wavetable::WavetableBank;

enum class FMSource : uint8_t {
  None = 0,
  Osc1,
  Osc2,
  Osc3,
  Osc4,
};

struct WavetableOscConfig {
  const WavetableBank* bank = nullptr;
  float scanPosition = 0.0f;
  float mixLevel = 1.0f;
  float fmDepth = 0.0f;
  FMSource fmSource = FMSource::None;
  int8_t octaveOffset = 0;
  float detuneAmount = 0.0f;
  bool enabled = true;
};

struct WavetableOscillator {
  // ==== Per-voice hot data (SoA) ====
  float phases[MAX_VOICES];          // normalized [0, 1.0)
  float phaseIncrements[MAX_VOICES]; // cycles per sample (freq / sampleRate)

  // ==== Global settings for all voices in oscillator ====
  const WavetableBank* bank = nullptr;
  float scanPosition = 0.0f;
  float mixLevel = 1.0f;
  float fmDepth = 0.0f;
  FMSource fmSource = FMSource::None;
  int8_t octaveOffset = 0;
  float detuneAmount = 0.0f;
  bool enabled = true;
};

struct WavetableOscModState {
  float osc1[MAX_VOICES] = {};
  float osc2[MAX_VOICES] = {};
  float osc3[MAX_VOICES] = {};
  float osc4[MAX_VOICES] = {};
};

void initOscillator(WavetableOscillator& osc,
                    uint32_t voiceIndex,
                    uint8_t midiNote,
                    float sampleRate);

void updateConfig(WavetableOscillator& osc, const WavetableOscConfig& config);

/* Read one sample with dual-mip linear interpolation.
 * mipF: continuous mip level from selectMipLevel() — fractional part drives mip
 * crossfade. effectiveScanPos: base scanPosition + mod delta, clamped [0,1] by
 * caller.
 *
 * fmPhaseOffset: phase displacement from FM in cycles (0.0 = no FM). Any
 * magnitude, any sign — wrapping is handled internally via floorf. This is why
 * phases are normalized.
 */
float readWavetable(const WavetableOscillator& osc,
                    uint32_t voiceIndex,
                    float mipF,
                    float effectiveScanPos,
                    float fmPhaseOffset);

/* Returns a continuous float mip level for the given phase increment.
 * phaseIncrement must be in TABLE_SIZE units (table positions/sample):
 * pass osc.phaseIncrements[v] * TABLE_SIZE_F  (or the interpolated
 * equivalent) Fractional part is the blend weight between mip floor and
 * ceiling. Call per-sample from the interpolated pitch increment — not just at
 * note-on.
 */
float selectMipLevel(float phaseIncrement);

// Process oscillator (read table and increment phase)
float processOscillator(WavetableOscillator& osc,
                        uint32_t voiceIndex,
                        float mipF,
                        float effectiveScanPos,
                        float fmPhaseOffset,
                        float pitchIncrement);

void resetOscModState(WavetableOscModState& modState, uint32_t voiceIndex);

/* FM source lookup — reads previous sample's FM-affected output from
 * oscModOutputs. Lives on VoicePool, not on WavetableOscillator (see
 * ASPIRATIONAL note above). All four values are already available before any
 * processOscillator call, so processing order is irrelevant. When fmSource ==
 * FMSource::None, returns 0.0f — the subsequent multiply by fmDepth collapses
 * to zero. Vital uses an explicit fmAmount == 0 branch to skip the lookup;
 * rely on the optimizer here, or add the branch if profiling shows cost.
 */
float getFmSourceValue(WavetableOscModState& modState, uint32_t voiceIndex, FMSource src);

} // namespace synth::wavetable::osc
