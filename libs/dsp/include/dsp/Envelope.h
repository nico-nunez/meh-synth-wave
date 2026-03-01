#pragma once

namespace dsp::envelopes {

enum class Status { Idle, Attack, Decay, Sustain, Release };

float processADSR(Status& state,
                  float& amplitude,
                  float& progress,
                  float& releaseStartLevel,
                  float attackInc,
                  float decayInc,
                  float releaseInc,
                  float sustainLevel);

} // namespace dsp::envelopes
