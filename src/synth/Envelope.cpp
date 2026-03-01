#include "Envelope.h"

#include "dsp/Envelope.h"

#include <cstdint>

namespace synth::envelope {

void initEnvelope(Envelope& env, uint32_t voiceIndex, float sampleRate) {
  env.states[voiceIndex] = EnvelopeStatus::Attack;
  env.levels[voiceIndex] = 0.0f;
  env.progress[voiceIndex] = 0.0f;

  updateIncrements(env, sampleRate);
}

void triggerRelease(Envelope& env, uint32_t voiceIndex) {
  env.states[voiceIndex] = EnvelopeStatus::Release;
  env.releaseStartLevels[voiceIndex] = env.levels[voiceIndex];
  env.progress[voiceIndex] = 0.0f;
}

void updateIncrements(Envelope& env, float sampleRate) {
  env.attackIncrement = 1.0f / (env.attackMs * 0.001f * sampleRate);
  env.decayIncrement = 1.0f / (env.decayMs * 0.001f * sampleRate);
  env.releaseIncrement = 1.0f / (env.releaseMs * 0.001f * sampleRate);
}

float processEnvelope(Envelope& env, uint32_t voiceIndex) {
  float level = dsp::envelopes::processADSR(env.states[voiceIndex],
                                            env.levels[voiceIndex],
                                            env.progress[voiceIndex],
                                            env.releaseStartLevels[voiceIndex],
                                            env.attackIncrement,
                                            env.decayIncrement,
                                            env.releaseIncrement,
                                            env.sustainLevel);

  return level;
}
} // namespace synth::envelope
