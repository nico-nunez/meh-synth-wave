#pragma once

#include <cstdint>

namespace dsp::wavetable {
inline constexpr uint32_t TABLE_SIZE = 2048; // must be a power of 2
inline constexpr float TABLE_SIZE_F = float(TABLE_SIZE);

// 2× resolution (may get tossed)
inline constexpr uint32_t TABLE_SIZE_HI_RES = 4096;
inline constexpr float TABLE_SIZE_HI_RES_F = float(TABLE_SIZE_HI_RES);

// covers ~11 octaves (MIDI 0–127 is ~10.5)
inline constexpr uint8_t MAX_MIP_LEVELS = 11;

inline constexpr uint8_t MAX_BANK_NAME_LEN = 64;
inline constexpr uint32_t MAX_FRAMES = 256; // per bank

// Fixed-point phase constants (32-bit phase, upper 11 bits = table index)
inline constexpr uint32_t PHASE_SHIFT = 21; // 32 - log2(TABLE_SIZE)
inline constexpr uint32_t TABLE_MASK = TABLE_SIZE - 1;

// 0x1FFFFF or 2097151 — fraction bits (21 consecutive 1s)
inline constexpr uint32_t FRAC_MASK = (1u << PHASE_SHIFT) - 1;
inline constexpr float FRAC_SCALE = 1.0f / float(1u << PHASE_SHIFT);

// Convert a float phase increment (table positions/sample) to fixed-point
// Use double to preserve precision — called at note-on, not in the hot loop
inline uint32_t toFixedPhaseInc(float phaseIncrement) {
  return static_cast<uint32_t>(static_cast<double>(phaseIncrement) /
                               TABLE_SIZE * 4294967296.0);
}

// One single-cycle waveform at all mip levels
struct WavetableFrame {
  float mips[MAX_MIP_LEVELS][TABLE_SIZE];
};

struct WavetableBank {
  WavetableFrame *frames;  // heap-allocated: frameCount frames
  uint32_t frameCount = 1; // must be non-zero
  char name[MAX_BANK_NAME_LEN];
};

// Lifecycle
WavetableBank *createWavetableBank(uint8_t frameCount, const char *name);
bool destroyWavetableBank(WavetableBank *bank);

// Linear table lookup — phase is fixed-point uint32_t
float readTable(const float *table, uint32_t phase);

} // namespace dsp::wavetable
