#pragma once

#include "dsp/WaveTable.h"

namespace synth::wavetable::banks {
using WavetableBank = dsp::wavetable::WavetableBank;

void registerBank(WavetableBank *bank);

WavetableBank *getBankByName(const char *name);

} // namespace synth::wavetable::banks
