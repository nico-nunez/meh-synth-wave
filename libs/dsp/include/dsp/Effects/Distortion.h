#pragma once

namespace dsp::effects::distortion {

struct DistortionEffect {
  float drive = 1.0f;
  float mix = 1.0f;
  bool enabled = false;
  // No internal state — waveshaper is stateless
};

} // namespace dsp::effects::distortion
