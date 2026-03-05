#pragma once

#include "synth_io/Events.h"

#include <atomic>
#include <cstddef>
#include <cstdio>

namespace synth_io {

struct MIDIEventQueue {
  // NOTE(nico): SIZE value need to be power of to use bitmasking for wrapping
  // Alternative is modulo (%) which is more expensive
  static constexpr size_t SIZE{256};
  static constexpr size_t WRAP{SIZE - 1};

  MIDIEvent queue[SIZE];

  std::atomic<size_t> readIndex{0};
  std::atomic<size_t> writeIndex{0};

  bool push(const MIDIEvent& event);
  bool pop(MIDIEvent& event);
};

} // namespace synth_io
