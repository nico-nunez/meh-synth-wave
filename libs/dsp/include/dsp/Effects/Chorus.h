#pragma once

namespace dsp::effects::chorus {

struct ChorusState {
  // todo....
};

struct ChorusEffect {
  float rate = 1.0f;  // LFO rate, Hz
  float depth = 0.5f; // modulation depth
  float mix = 0.5f;
  bool enabled = false;
  ChorusState state; // delay lines + LFO phase
};

} // namespace dsp::effects::chorus
