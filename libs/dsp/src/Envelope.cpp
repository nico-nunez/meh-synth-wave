#include "dsp/Envelope.h"

namespace dsp::envelopes {

float processADSR(Status& state,
                  float& amplitude,
                  float& progress,
                  float& releaseStartLevel,
                  float attackInc,
                  float decayInc,
                  float releaseInc,
                  float sustainLevel) {
  switch (state) {
  case Status::Attack:
    progress += attackInc;
    if (progress >= 1.0f) {
      state = Status::Decay;
      progress = 0.0f;
      amplitude = 1.0f;
    } else {
      amplitude = progress;
    }
    break;

  case Status::Decay:
    progress += decayInc;
    if (progress >= 1.0f) {
      state = Status::Sustain;
      amplitude = sustainLevel;
    } else {
      amplitude = 1.0f - progress * (1.0f - sustainLevel);
    }
    break;

  case Status::Sustain:
    amplitude = sustainLevel;
    break;

  case Status::Release:
    progress += releaseInc;
    if (progress >= 1.0f) {
      state = Status::Idle;
      amplitude = 0.0f;
    } else {
      amplitude = releaseStartLevel * (1.0f - progress);
    }
    break;

  case Status::Idle:
    amplitude = 0.0f;
    break;
  }

  return amplitude;
};
} // namespace dsp::envelopes
