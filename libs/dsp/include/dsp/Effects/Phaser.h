#pragma once

#include <cstdint>

namespace dsp::effects::phaser {

struct PhaserState {
  // todo...
};

struct PhaserEffect {
  int8_t stages = 4; // allpass stage count
  float rate = 0.5f;
  float depth = 1.0f;
  float feedback = 0.5f;
  float mix = 0.5f;
  bool enabled = false;
  PhaserState state;
};

} // namespace dsp::effects::phaser
