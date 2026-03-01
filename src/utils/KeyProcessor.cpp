#include "KeyProcessor.h"
#include "Logger.h"

#include "synth_io/Events.h"
#include "synth_io/SynthIO.h"

#include "device_io/KeyCapture.h"
#include "device_io/MidiCapture.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <iostream>

namespace synth::utils {
using MidiEvent = device_io::MidiEvent;

using NoteEvent = synth_io::NoteEvent;
using NoteEventType = synth_io::NoteEventType;

// Handle MIDI device events
static void midiCallback(MidiEvent midiEvent, void* context) {
  auto sessionPtr = static_cast<hSynthSession>(context);

  // TODO(nico): handle more than just note on/off events
  switch (midiEvent.type) {
  case MidiEvent::Type::NoteOn:
    synth_io::noteOn(sessionPtr, midiEvent.data1, midiEvent.data2);
    break;
  case MidiEvent::Type::NoteOff:
    synth_io::noteOff(sessionPtr, midiEvent.data1, midiEvent.data2);
    break;

  default:
    break;
  }
}

// Handle keyboard events
static void keyEventCallback(device_io::KeyEvent event, void* userContext) {
  auto sessionPtr = static_cast<hSynthSession>(userContext);

  // Currently 'z' & 'x' control octive up/down
  // Need to ignore keyup (note off) for now
  if ((event.character == 120 || event.character == 122) &&
      event.type == device_io::KeyEventType::KeyUp) {

    return;

    // Note "ON" event
  } else if (event.type == device_io::KeyEventType::KeyDown) {

    synth_io::noteOn(sessionPtr, asciiToMidi(event.character), 127);

    // Note "OFF" event
  } else if (event.type == device_io::KeyEventType::KeyUp) {

    synth_io::noteOff(sessionPtr, asciiToMidi(event.character), 127);
  }

  // "ESC" to quit
  if (event.type == device_io::KeyEventType::KeyDown && event.keyCode == 53) {
    printf("ESC pressed, stopping...\n");
    device_io::terminateKeyCaptureLoop();
  }
}

hMidiSession initMidiSession(hSynthSession sessionPtr) {
  // 1a. Setup MIDI on this thread's run loop for now
  constexpr size_t MAX_MIDI_DEVICES = 16;
  device_io::MidiSource midiSourceBuffer[MAX_MIDI_DEVICES];
  size_t numMidiDevices = device_io::getMidiSources(midiSourceBuffer, MAX_MIDI_DEVICES);

  hMidiSession midiSession = nullptr;

  if (numMidiDevices) {
    // Display MIDI source options
    for (size_t i = 0; i < numMidiDevices; i++) {
      printf("%ld. %s\n", i, midiSourceBuffer[i].displayName);
    }

    uint32_t srcIndex;
    LogF("Enter midi device number: ");
    std::cin >> srcIndex;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    if (srcIndex < 0 || srcIndex >= numMidiDevices) {
      printf("Invalid option selected: %d", srcIndex);
      return midiSession;
    }

    midiSession = device_io::setupMidiSession({}, midiCallback, sessionPtr);

    device_io::connectMidiSource(midiSession, midiSourceBuffer[srcIndex].uniqueID);

    device_io::startMidiSession(midiSession);

  } else {
    synth::utils::LogF("No MIDI devices found\n");
  }

  return midiSession;
}

int startKeyInputCapture(hSynthSession sessionPtr, hMidiSession midiSessionPtr) {
  printf("KeyCapture Example\n");
  printf("------------------\n");
  printf("Press keys to see events. ESC to quit.\n\n");

  // 1. Initialize Cocoa app
  device_io::initKeyCaptureApp();

  // 2. Create a minimal window (required for local capture without permissions)
  device_io::WindowConfig config = device_io::defaultWindowConfig();
  config.title = "Meh Synth";
  config.width = 800;
  config.height = 500;

  if (!createCaptureWindow(config)) {
    printf("Failed to create window\n");
    return 1;
  }

  // 3. Start capturing with local mode (no permissions needed when window
  // focused)
  //    Change to CaptureMode::Global if you need capture when not focused
  //    Change to CaptureMode::Both if you want both behaviors
  if (!startKeyCapture(keyEventCallback, sessionPtr, device_io::CaptureMode::Local)) {
    printf("Failed to start key capture\n");
    return 1;
  }

  const char* windowText = "Super Synth\n\n"
                           "Press 'z' to go down an octive and 'c' to go up an octive\n\n"
                           "================= Keyboard Layout ================\n"
                           "|    |   |   |   |   |   |   |   |   |   |   |   |\n"
                           "|    |   |   |   |   |   |   |   |   |   |   |   |\n"
                           "|    | w |   | E |   |   | T |   | Y |   | U |   |\n"
                           "|    |___|   |___|   |   |___|   |___|   |___|   |\n"
                           "|      |       |     |     |       |       |     |\n"
                           "|      |       |     |     |       |       |     |\n"
                           "|  A   |   S   |  D  |  F  |   G   |   H   |  J  |\n"
                           "|______|_______|_____|_____|_______|_______|_____|\n\n"
                           "Press keys... (ESC to quit)\n";

  // Update window text
  device_io::setWindowText(windowText);

  // 4. Run the event loop (blocks until stopKeyCaptureLoop() called)
  device_io::runKeyCaptureLoop();

  // 5. Cleanup
  device_io::stopKeyCapture();

  if (midiSessionPtr) {
    device_io::stopMidiSession(midiSessionPtr);
    device_io::cleanupMidiSession(midiSessionPtr);
  }

  printf("Done.\n");
  return 0;
}

uint8_t asciiToMidi(char key) {
  static constexpr uint8_t SEMITONES = 12;
  static uint8_t octiveOffset = 0;

  uint8_t midiKey = 0;

  // Change Octive
  if (key == 122) { // ('z')
    --octiveOffset;
  }

  if (key == 120) { // ('x')
    ++octiveOffset;
  }

  // Change Velocity
  // 99  // ('c')
  // 118 // ('v')

  switch (key) {
  case 97: //  ('a') "C"  64
    midiKey = 64;
    break;
  case 119: // ('w') "C#" 65
    midiKey = 65;
    break;
  case 115: // ('s') "D"  66
    midiKey = 66;
    break;
  case 101: // ('e') "D#" 67
    midiKey = 67;
    break;
  case 100: // ('d') "E"  68
    midiKey = 68;
    break;
  case 102: // ('f') "F"  69
    midiKey = 69;
    break;
  case 116: // ('t') "F#" 70
    midiKey = 70;
    break;
  case 103: // ('g') "G"  71
    midiKey = 71;
    break;
  case 121: // ('y') "G#" 72
    midiKey = 72;
    break;
  case 104: // ('h') "A"  73
    midiKey = 73;
    break;
  case 117: // ('u') "A#" 74
    midiKey = 74;
    break;
  case 106: // ('j') "B"  75
    midiKey = 75;
    break;
  case 107: // ('k') "C"  76
    midiKey = 76;
    break;
  case 111: // ('o') "C#" 77
    midiKey = 77;
    break;
  case 108: // ('l') "D"  78
    midiKey = 78;
    break;
  case 112: // ('p') "D#" 79
    midiKey = 79;
    break;

  default:
    return 0; // unmapped key
  }

  return midiKey + (octiveOffset * SEMITONES);
}

} // namespace synth::utils
