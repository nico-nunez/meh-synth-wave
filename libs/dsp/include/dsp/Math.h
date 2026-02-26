#pragma once

#include <cstdint>
namespace dsp::math {

inline constexpr float PI_F = 3.1415927f;
inline constexpr double PI_DOUBLE = 3.141592653589793;
inline constexpr float TWO_PI_F = 2 * PI_F;

inline constexpr float SEMITONE_PER_OCTAVE = 1.0f / 12.0f;

// Pre-caluated value of 2^(1/12)
inline constexpr float SEMITONE_RATIO = 1.059463094f;

float fastExp2(float x);
float semitonesToFreqRatio(float x);

float fastLog2(float val);

// White noise PRNG
uint32_t xorshift32();
float randNoiseValue();

} // namespace dsp::math
