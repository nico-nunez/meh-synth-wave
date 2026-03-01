#pragma once

#include "Types.h"

#include "dsp/Envelope.h"

#include <cstdint>

namespace synth::envelope {
using EnvelopeStatus = dsp::envelopes::Status;

struct Envelope {
  // === Per-voice state (hot data) ===
  EnvelopeStatus states[MAX_VOICES];    // Current stage
  float levels[MAX_VOICES];             // Current output (0.0-1.0)
  float progress[MAX_VOICES];           // Progress in stage (0.0-1.0)
  float releaseStartLevels[MAX_VOICES]; // Captured on release

  // === Global ADSR settings (cold data) ===
  float attackMs = 10.0f;
  float decayMs = 100.0f;
  float sustainLevel = 0.7f;
  float releaseMs = 200.0f;

  // Pre-calculated increments (updated when UI changes)
  float attackIncrement = 0.0f; // 1.0 / (attackMs * 0.001 * sampleRate)
  float decayIncrement = 0.0f;
  float releaseIncrement = 0.0f;
};

void initEnvelope(Envelope& env, uint32_t voiceIndex, float sampleRate);

void triggerRelease(Envelope& env, uint32_t voiceIndex);

// Helper to recalculate increments when ADSR changes
void updateIncrements(Envelope& env, float sampleRate);

float processEnvelope(Envelope& env, uint32_t voiceIndex);

} // namespace synth::envelope
