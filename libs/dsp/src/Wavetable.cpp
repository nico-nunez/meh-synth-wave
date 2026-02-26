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

/* ===================================
 * Table lookup — linear interpolation
 * ===================================
 * _phase_ is in table-space [0, TABLE_SIZE_F). The caller (readWavetable)
 * converts normalized [0, 1.0) phase to table-space by multiplying by
 * TABLE_SIZE_F before calling here. This keeps the multiply out of any inner
 * loop that calls readTable multiple times (e.g. dual-mip reads 2–4 times per
 * sample).
 *
 * TABLE_MASK wraps iB at 2047 → 0. iA is already in [0, TABLE_SIZE-1] by
 * construction (truncation of a value in [0, TABLE_SIZE_F)). Bitmask is valid
 * because TABLE_SIZE is a power of 2.*/

float readTable(const float *table, float phase) {
  uint32_t iA = static_cast<uint32_t>(phase);
  uint32_t iB = (iA + 1) & TABLE_MASK;         // wrap 2047 → 0
  float frac = phase - static_cast<float>(iA); // fractional part [0, 1)

  return table[iA] + frac * (table[iB] - table[iA]);
}

} // namespace dsp::wavetable
