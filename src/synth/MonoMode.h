#pragma once

#include "synth/Types.h"

#include <cstdint>

namespace synth::mono {

struct MonoState {
  bool enabled = false;
  bool legato = true;

  uint32_t voiceIndex = synth::MAX_VOICES;

  bool heldNotes[128]{};
  uint8_t noteStack[16]{};
  uint8_t stackDepth = 0;
};

void pushNoteToStack(MonoState& mono, uint8_t midiNote);
void removeNoteFromStack(MonoState& state, uint8_t midiNote);

} // namespace synth::mono
