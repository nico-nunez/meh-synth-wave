
#pragma once

#include "dsp/Tempo.h"

namespace dsp::effects::delay {
using tempo::Subdivision;

struct DelayState {
  // todo....
};

struct DelayEffect {
  float time = 0.5f; // seconds, used when !tempoSync
  Subdivision subdivision = Subdivision::Quarter;
  bool tempoSync = true;
  float feedback = 0.4f;
  bool pingPong = false;
  float mix = 0.5f;
  bool enabled = false;
  uint32_t delaySamples = 0; // precomputed; hot path reads this
  DelayState state;          // circular buffer, read/write heads
};

} // namespace dsp::effects::delay
