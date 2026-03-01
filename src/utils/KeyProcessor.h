#pragma once

#include <cstdint>

namespace device_io {
struct MidiSession;
}
namespace synth_io {
struct NoteEventQueue;
struct SynthSession;
} // namespace synth_io

namespace synth::utils {
using hMidiSession = device_io::MidiSession*;
using hSynthSession = synth_io::SynthSession*;

hMidiSession initMidiSession(hSynthSession);

int startKeyInputCapture(hSynthSession, hMidiSession);

uint8_t asciiToMidi(char key);
} // namespace synth::utils
