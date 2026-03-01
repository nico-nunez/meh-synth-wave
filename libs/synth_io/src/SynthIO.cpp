#include "synth_io/SynthIO.h"

#include "NoteEventQueue.h"
#include "ParamEventQueue.h"

#include "audio_io/AudioIO.h"
#include "audio_io/AudioIOTypes.h"
#include "audio_io/AudioIOTypesFwd.h"

#include <cstdint>
#include <cstdio>

namespace synth_io {
using AudioBuffer = audio_io::AudioBuffer;
using hAudioSession = audio_io::hAudioSession;

struct SynthSession {
  NoteEventQueue noteEventQueue{};
  ParamEventQueue paramEventQueue{};

  AudioBufferHandler processAudioBlock;

  NoteEventHandler processNoteEvent;
  ParamEventHandler processParamEvent;

  hAudioSession audioSession;
  void* userContext;
};
using hSynthSession = SynthSession*;

static void audioCallback(AudioBuffer buffer, void* context) {
  auto* ctx = static_cast<SynthSession*>(context);

  if (ctx->processParamEvent) {
    ParamEvent paramEvent;
    while (ctx->paramEventQueue.pop(paramEvent)) {
      ctx->processParamEvent(paramEvent, ctx->userContext);
    }
  }

  if (ctx->processNoteEvent) {
    NoteEvent noteEvent;
    while (ctx->noteEventQueue.pop(noteEvent)) {
      ctx->processNoteEvent(noteEvent, ctx->userContext);
    }
  }

  if (ctx->processAudioBlock) {
    ctx->processAudioBlock(buffer.channelPtrs,
                           buffer.numChannels,
                           buffer.numFrames,
                           ctx->userContext);
  }
}

// ==== PUBLIC APIS ====

// ==== Session Handlers ====
hSynthSession initSession(SessionConfig userConfig,
                          SynthCallbacks userCallbacks,
                          void* userContext) {

  hSynthSession sessionPtr = new SynthSession();
  sessionPtr->processParamEvent = userCallbacks.processParamEvent;
  sessionPtr->processNoteEvent = userCallbacks.processNoteEvent;
  sessionPtr->processAudioBlock = userCallbacks.processAudioBlock;
  sessionPtr->userContext = userContext;

  // 2. Setup audio_io
  audio_io::Config config{};
  config.sampleRate = userConfig.sampleRate;
  config.numChannels = userConfig.numChannels;
  config.numFrames = userConfig.numFrames;
  config.bufferFormat = static_cast<audio_io::BufferFormat>(userConfig.bufferFormat);

  sessionPtr->audioSession = audio_io::setupAudioSession(config, audioCallback, sessionPtr);

  return sessionPtr;
};

int startSession(hSynthSession sessionPtr) {
  return audio_io::startAudioSession(sessionPtr->audioSession);
}

int stopSession(hSynthSession sessionPtr) {
  return audio_io::stopAudioSession(sessionPtr->audioSession);
}

int disposeSession(hSynthSession sessionPtr) {
  int status = audio_io::cleanupAudioSession(sessionPtr->audioSession);
  if (status != 0) {
    printf("Unable to cleanup Audio Session");
    return 1;
  }

  delete sessionPtr;

  return 0;
}

// ==== Note Event Handlers ====
bool noteOn(hSynthSession sessionPtr, uint8_t midiNote, uint8_t velocity) {
  // TODO(nico): replicate emplace_back() to reduce copy;
  return sessionPtr->noteEventQueue.push({NoteEventType::NoteOn, midiNote, velocity});
}

bool noteOff(hSynthSession sessionPtr, uint8_t midiNote, uint8_t velocity) {
  // TODO(nico): replicate emplace_back() to reduce copy;
  return sessionPtr->noteEventQueue.push({NoteEventType::NoteOff, midiNote, velocity});
}

// ==== Parameter Event Handlers ====
bool setParam(hSynthSession sessionPtr, uint8_t id, float value) {
  // TODO(nico): replicate emplace_back() to reduce copy;
  return sessionPtr->paramEventQueue.push({id, value});
}

} // namespace synth_io
