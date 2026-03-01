#include "NoteEventQueue.h"
#include <cstddef>

namespace synth_io {

bool NoteEventQueue::push(const NoteEvent& event) {
  size_t currentIndex = writeIndex.load();
  size_t nextIndex = (currentIndex + 1) & WRAP;

  if (nextIndex == readIndex.load())
    return false;

  queue[currentIndex] = event;
  writeIndex.store(nextIndex);

  return true;
}

bool NoteEventQueue::pop(NoteEvent& event) {
  size_t currentIndex = readIndex.load();

  if (currentIndex == writeIndex.load())
    return false;

  event = queue[currentIndex];
  readIndex.store((currentIndex + 1) & WRAP);

  return true;
}

void NoteEventQueue::printEvent(NoteEvent& event) {
  printf("==== Event ====\n");
  printf("type: %d\n", (int)event.type);
  printf("midi: %d\n", event.midiNote);
  printf("velocity: %d\n", event.velocity);
}

void NoteEventQueue::printQueue() {
  size_t currentIndex = readIndex.load();
  size_t endIndex = writeIndex.load();

  // Only print events that are able to be read
  printf("======== Event Queue ========\n");
  for (; currentIndex < endIndex; currentIndex++) {
    printEvent(queue[currentIndex]);
  }
}

} // namespace synth_io
