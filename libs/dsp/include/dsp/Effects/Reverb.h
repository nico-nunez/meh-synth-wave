#pragma once

namespace dsp::effects::reverb {
struct DattorroReverb {
  // todo...
};

struct ReverbEffect {
  float preDelay = 0.0f; // ms
  float decay = 0.75f;   // T60 decay time
  float damping = 0.5f;
  float bandwidth = 0.75f; // input bandwidth
  float mix = 0.5f;
  bool enabled = false;
  DattorroReverb state; // full plate network, fixed topology
};

} // namespace dsp::effects::reverb
