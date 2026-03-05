#pragma once

#include <cstdint>

namespace synth_io {

struct MIDIEvent {
  enum class Type : uint8_t {
    NoteOn,
    NoteOff,
    ControlChange,
    PitchBend,
    ProgramChange,
    Aftertouch,
    ChannelPressure,
    Unknown
  };

  Type type = Type::Unknown;
  uint8_t channel = 0;
  uint64_t timestamp = 0;

  union {
    struct {
      uint8_t note;
      uint8_t velocity;
    } noteOn;
    struct {
      uint8_t note;
      uint8_t velocity;
    } noteOff;
    struct {
      uint8_t number;
      uint8_t value;
    } cc;
    struct {
      int16_t value;
    } pitchBend;
    struct {
      uint8_t number;
    } programChange;
    struct {
      uint8_t note;
      uint8_t pressure;
    } aftertouch;
    struct {
      uint8_t pressure;
    } channelPressure;
  } data;
};

struct ParamEvent {
  uint8_t id = 0;
  float value = 0.0f; // Normalized [0, 1]
};

} // namespace synth_io
