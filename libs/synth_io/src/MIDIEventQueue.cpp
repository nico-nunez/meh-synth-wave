#include "MIDIEventQueue.h"
#include <cstddef>

namespace synth_io {

bool MIDIEventQueue::push(const MIDIEvent& event) {
  size_t currentIndex = writeIndex.load();
  size_t nextIndex = (currentIndex + 1) & WRAP;

  if (nextIndex == readIndex.load())
    return false;

  queue[currentIndex] = event;
  writeIndex.store(nextIndex);

  return true;
}

bool MIDIEventQueue::pop(MIDIEvent& event) {
  size_t currentIndex = readIndex.load();

  if (currentIndex == writeIndex.load())
    return false;

  event = queue[currentIndex];
  readIndex.store((currentIndex + 1) & WRAP);

  return true;
}

} // namespace synth_io
