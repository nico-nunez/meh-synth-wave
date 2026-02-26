#include "WavetableBanks.h"

#include <cstdint>
#include <cstdio>
#include <cstring>

namespace synth::wavetable::banks {
// ================================
// Bank registry
// ================================
static constexpr uint8_t MAX_REGISTRY_BANKS = 32;
static WavetableBank *s_registry[MAX_REGISTRY_BANKS] = {};
static uint8_t s_registryCount = 0;

void registerBank(WavetableBank *bank) {
  if (!bank) {
    printf("registerBank: null bank\n");
    return;
  }
  if (s_registryCount >= MAX_REGISTRY_BANKS) {
    printf("registerBank: registry full\n");
    return;
  }
  s_registry[s_registryCount++] = bank;
}

// TODO(nico-nunez): do I need a deregisterBank

WavetableBank *getBankByName(const char *name) {
  for (int i = 0; i < s_registryCount; i++) {
    if (std::strcmp(s_registry[i]->name, name) == 0)
      return s_registry[i];
  }
  return nullptr;
}

} // namespace synth::wavetable::banks
