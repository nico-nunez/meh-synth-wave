#pragma once

#include <cstdint>

namespace dsp::wavetable {
inline constexpr uint32_t TABLE_SIZE = 2048; // must be a power of 2
inline constexpr float TABLE_SIZE_F = static_cast<float>(TABLE_SIZE);

// 2× resolution (may get tossed)
inline constexpr uint32_t TABLE_SIZE_HI_RES = 4096;
inline constexpr float TABLE_SIZE_HI_RES_F = static_cast<float>(TABLE_SIZE_HI_RES);

// covers ~11 octaves (MIDI 0–127 is ~10.5)
inline constexpr uint8_t MAX_MIP_LEVELS = 11;

inline constexpr uint32_t MAX_FRAMES = 256;
inline constexpr uint8_t MAX_BANK_NAME_LEN = 64;

// Index wrap mask — used in readTable to wrap 2047 → 0
inline constexpr uint32_t TABLE_MASK = TABLE_SIZE - 1; // 0x7FF

// One single-cycle waveform at all mip levels
struct WavetableFrame {
  float mips[MAX_MIP_LEVELS][TABLE_SIZE];
};

struct WavetableBank {
  WavetableFrame* frames;  // heap-allocated: frameCount frames
  uint32_t frameCount = 1; // must be non-zero
  char name[MAX_BANK_NAME_LEN];
};

// Lifecycle
WavetableBank* createWavetableBank(uint32_t frameCount, const char* name);
bool destroyWavetableBank(WavetableBank* bank);

// Linear table lookup — phase is float in [0, TABLE_SIZE_F)
float readTable(const float* table, float phase);

} // namespace dsp::wavetable
