#include "NoiseOscillator.h"
#include "dsp/Math.h"

namespace synth::noise_osc {
float processNoise(NoiseOscillator &noise) {
  if (!noise.enabled)
    return 0.0f;

  float noiseValue = dsp::math::randNoiseValue(); // -> [-1, 1]

  if (noise.noiseType == NoiseType::White)
    return noiseValue * noise.mixLevel;

  // Paul Kellet pink noise approximation
  noise.b0 = 0.99886f * noise.b0 + noiseValue * 0.0555179f;
  noise.b1 = 0.99332f * noise.b1 + noiseValue * 0.0750759f;
  noise.b2 = 0.96900f * noise.b2 + noiseValue * 0.1538520f;
  float pink = (noise.b0 + noise.b1 + noise.b2 + noiseValue * 0.5362f) * 0.11f;

  return pink * noise.mixLevel;
}
} // namespace synth::noise_osc
