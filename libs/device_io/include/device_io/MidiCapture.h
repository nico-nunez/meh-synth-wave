#pragma once

#include <CoreMIDI/CoreMIDI.h>

#include <cstddef>
#include <cstdint>

/*
 * IMPORTANT NOTE(nico):
 * *MIDI* == Apple defined
 * *Midi* == Internally defined (aka defined by me)
 */

namespace device_io {

struct MidiConfig {
  // --- Current ---
  // (none yet - using defaults)

  // --- Future possibilities ---
  // uint8_t channelFilter = 0xFF;        // Bitmask: which channels to accept
  // (0xFF = all) bool ignoreActiveSensing = true;     // Filter out active
  // sensing messages bool ignoreClock = true;             // Filter out MIDI
  // clock messages bool useInternalThread = false;      // false = piggyback,
  // true = standalone mode const char* clientName = "device_io"; // Name shown
  // in Audio MIDI Setup
};

struct MidiSource {
  MIDIUniqueID uniqueID = 0;
  char displayName[256] = "";

  // --- Future possibilities ---
  // char manufacturer[128] = "";         // e.g., "Arturia"
  // char model[128] = "";                // e.g., "KeyStep 37"
  // reconnects bool isOnline = false;               // Currently connected?
  // bool isVirtual = false;              // Virtual port (software) vs hardware
};

struct MidiEvent {
  enum class Type : uint8_t {
    NoteOn,
    NoteOff,
    ControlChange,
    PitchBend,
    ProgramChange,
    Aftertouch,      // Polyphonic (per-note)
    ChannelPressure, // Monophonic (whole channel)
    // System messages (if not filtered)
    Clock,
    Start,
    Stop,
    Continue,
    Unknown
  };

  Type type = Type::Unknown;
  uint8_t channel = 0; // 0-15

  // Interpretation depends on type:
  //   NoteOn/NoteOff:    data1 = note (0-127), data2 = velocity (0-127)
  //   ControlChange:     data1 = CC number (0-127), data2 = value (0-127)
  //   ProgramChange:     data1 = program (0-127), data2 = unused
  //   Aftertouch:        data1 = note (0-127), data2 = pressure (0-127)
  //   ChannelPressure:   data1 = pressure (0-127), data2 = unused
  //   PitchBend:         (use pitchBendValue instead)
  uint8_t data1 = 0;
  uint8_t data2 = 0;

  // Pitch bend as signed value: -8192 to +8191 (0 = center)
  // Only valid when type == PitchBend
  int16_t pitchBendValue = 0;

  // CoreMIDI timestamp (mach_absolute_time units)
  // Can convert to seconds or compare for ordering
  uint64_t timestamp = 0;

  // --- Future possibilities ---
  // uint32_t sourceUniqueID = 0;    // Which device this came from
  // (multi-device setups) float normalizedVelocity = 0;   // velocity / 127.0f
  // (convenience) float normalizedCC = 0;         // CC value / 127.0f
  // (convenience) float pitchBendNormalized = 0;  // -1.0 to +1.0
};

using MidiCallback = void (*)(MidiEvent event, void* userData);

struct MidiSession;
using hMidiSession = MidiSession*;

// ==== API ====

// Query available sources (can call anytime, doesn't need a session)
/*!
        @function		getMidiSources

        @abstract 		Get available MIDI sources

        @param		  srcBuffer
            MidiSource source buffer to be filled
        @param		  srcBufferCount
                                                Size (count) of source buffer
        @result			Number of sources found.  Last source buffer
   item is equal to return value - 1
*/
size_t getMidiSources(MidiSource* buffer, size_t bufferSize);

// Create session with callback
hMidiSession setupMidiSession(const MidiConfig& config, MidiCallback callback, void* userContext);

// Connect/disconnect sources (can call before or after start)
int connectMidiSource(hMidiSession session, MIDIUniqueID uniqueID);

int disconnectMidiSource(hMidiSession session, MIDIUniqueID uniqueID);
int disconnectAllMidiSources(hMidiSession session);

// Lifecycle
int startMidiSession(hMidiSession session);
int stopMidiSession(hMidiSession session);
int cleanupMidiSession(hMidiSession session);

} // namespace device_io
