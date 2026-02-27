#include "dsp/Math.h"
#include "dsp/Wavetable.h"

#include <cmath>
#include <cstdint>
#include <cstring>

namespace dsp::math {
// NOTE(nico): Modern compilers often auto-vectorize std::exp2f using SIMD
// instructions (like AVX or SSE), which can sometimes outperform manual
// bit-hacks on modern CPUs.
float fastExp2(float x) {
  int32_t xi = static_cast<int32_t>(x);
  float xf = x - static_cast<float>(xi);

  float p =
      1.0f + xf * (0.6931472f +
                   xf * (0.2402265f + xf * (0.0555041f + xf * 0.0096181f)));

  int32_t bits;
  std::memcpy(&bits, &p, 4);

  bits += xi << 23;

  std::memcpy(&p, &bits, 4);
  return p;
}

float semitonesToFreqRatio(float x) { return fastExp2(x / 12); }

// Fast approximation of log2(x)
float fastLog2(float val) {
  uint32_t i;
  std::memcpy(&i, &val, sizeof(i));

  float y = float(i) * 1.19209290e-7f; // 1 / 2^23

  return y - 126.94269504f;
}

// White noise PRNG
static uint32_t s_seed = 2463534242u;

uint32_t xorshift32() {
  s_seed ^= s_seed << 13;
  s_seed ^= s_seed >> 17;
  s_seed ^= s_seed << 5;
  return s_seed;
}

float randNoiseValue() {
  return static_cast<float>(static_cast<int32_t>(xorshift32())) *
         4.6566129e-10f; // float → [-1, 1]
}

float sineAt(int harmonic, int sampleIndex) {
  return sinf(dsp::math::TWO_PI_F * static_cast<float>(harmonic) *
              static_cast<float>(sampleIndex) / wavetable::TABLE_SIZE_F);
}

} // namespace dsp::math
