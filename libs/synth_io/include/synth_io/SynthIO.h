#pragma once

#include "Events.h"

#include <cstddef>
#include <cstdint>

namespace synth_io {
struct SynthSession;
using hSynthSession = SynthSession*;

// --- Constants ---
inline constexpr uint32_t DEFAULT_SAMPLE_RATE = 48000;
inline constexpr uint32_t DEFAULT_FRAMES = 512;
inline constexpr uint16_t DEFAULT_CHANNELS = 2;

enum class BufferFormat {
  NonInterleaved, // channels in separate arrays [LLLL] [RRRR]
  Interleaved,    // channels interwoven in single array [LRLRLRLR]
};

struct SessionConfig {
  uint32_t sampleRate = DEFAULT_SAMPLE_RATE;
  uint32_t numFrames = DEFAULT_FRAMES;
  uint16_t numChannels = DEFAULT_CHANNELS;
  BufferFormat bufferFormat = BufferFormat::NonInterleaved;
};

typedef void (*NoteEventHandler)(NoteEvent noteEvent, void* userContext);
typedef void (*AudioBufferHandler)(float** outputBuffer,
                                   size_t numChannels,
                                   size_t numFrames,
                                   void* userContext);

typedef void (*ParamEventHandler)(ParamEvent paramEvent, void* userContext);

struct SynthCallbacks {
  ParamEventHandler processParamEvent = nullptr;
  NoteEventHandler processNoteEvent = nullptr;
  AudioBufferHandler processAudioBlock = nullptr;
};

// ==== Session Handlers ====
hSynthSession initSession(SessionConfig userConfig,
                          SynthCallbacks userCallbacks,
                          void* userContext = NULL);

int startSession(hSynthSession sessionPtr);
int stopSession(hSynthSession sessionPtr);
int disposeSession(hSynthSession sessionPtr);

// ==== Note Event Handlers ====
bool noteOn(hSynthSession sessionPtr, uint8_t midiNote, uint8_t velocity);
bool noteOff(hSynthSession sessionPtr, uint8_t midiNote, uint8_t velocity);

// ==== Parameter Event Handlers ====
bool setParam(hSynthSession sessionPtr, uint8_t id, float value);

} // namespace synth_io
