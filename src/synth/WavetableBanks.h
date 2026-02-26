#pragma once

#include "dsp/Wavetable.h"

namespace synth::wavetable::banks {
using WavetableBank = dsp::wavetable::WavetableBank;

void initFactoryBanks();

void registerBank(WavetableBank *bank);

WavetableBank *getBankByName(const char *name);

} // namespace synth::wavetable::banks
