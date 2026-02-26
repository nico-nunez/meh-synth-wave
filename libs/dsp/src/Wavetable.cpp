#include "dsp/Wavetable.h"

#include <cstdint>
#include <cstdio>
#include <cstring>

namespace dsp::wavetable {
// ================================
// Lifecycle
// ================================

WavetableBank *createWavetableBank(uint32_t frameCount, const char *name) {
  if (frameCount == 0 || frameCount > MAX_FRAMES) {
    printf("createWavetableBank: invalid frameCount %u\n", frameCount);
    return nullptr;
  }

  WavetableBank *bank = new WavetableBank();
  bank->frames = new WavetableFrame[frameCount];
  bank->frameCount = frameCount;

  std::strncpy(bank->name, name, MAX_BANK_NAME_LEN - 1);
  bank->name[MAX_BANK_NAME_LEN - 1] = '\0';

  return bank;
}

bool destroyWavetableBank(WavetableBank *bank) {
  if (!bank) {
    printf("destroyWavetableBank: null bank\n");
    return false;
  }
  delete[] bank->frames;
  delete bank;
  return true;
}

// ======================================================
// Table lookup — linear interpolation, fixed-point phase
// ======================================================

float readTable(const float *table, uint32_t phase) {
  uint32_t iA = phase >> PHASE_SHIFT;
  uint32_t iB = (iA + 1) & TABLE_MASK;
  float frac = float(phase & FRAC_MASK) * FRAC_SCALE;

  return table[iA] + frac * (table[iB] - table[iA]);
}

} // namespace dsp::wavetable
