#include "Noise.h"

#include "dsp/Math.h"
#include "dsp/Noise.h"

namespace synth::noise {

void updateConfig(Noise& noise, const NoiseConfig& config) {
  noise.type = config.type;
  noise.mixLevel = config.mixLevel;
  noise.enabled = config.enabled;
}

float processNoise(Noise& noise) {
  if (!noise.enabled)
    return 0.0f;

  float whiteNoise = dsp::math::randNoiseValue(); // -> [-1, 1]

  if (noise.type == NoiseType::White)
    return whiteNoise * noise.mixLevel;

  return dsp::noise::processPinkNoise(noise.pinkNoiseState, whiteNoise) * noise.mixLevel;
}

} // namespace synth::noise
