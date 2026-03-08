#include "audio_io/AudioIO.h"
#include "adapters/core_audio/CoreAudioAdapter.h"
#include "audio_io/AudioIOTypes.h"
#include "shared/AudioSession.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>

// TODO(nico): This is where platform choice needs to occur

namespace audio_io {

// =============================
// Audio Device Negotiation
// =============================

DeviceInfo queryDefaultDevice() {
  return CoreAudioAdapter::queryDefaultOutputDevice();
}

// =============================
// Audio Session Initialization
// =============================

hAudioSession setupAudioSession(const Config& userConfig,
                                AudioCallback userCallback,
                                void* userContext) {

  // Create a new AudioSession
  hAudioSession sessionPtr = new AudioSession();

  // Set user provided data
  sessionPtr->userConfig = userConfig;
  sessionPtr->userCallback = userCallback;
  sessionPtr->userContext = userContext;

  // Get/Create platform context (CoreAudio for now.  more to come...)
  int errCode = CoreAudioAdapter::coreAudioSetup(sessionPtr);
  if (errCode) {
    printf("Platform setup failed: %d", errCode);
    delete sessionPtr;
    return nullptr;
  }

  // NOTE: Doing buffer stuff here since config values may change due
  // to platform compatability
  uint32_t numFrames = sessionPtr->userConfig.numFrames;
  uint16_t numChannels = sessionPtr->userConfig.numChannels;
  BufferFormat bufferFormat = sessionPtr->userConfig.bufferFormat;

  // Allocate bufferMemory
  sessionPtr->bufferMemory = new float[numFrames * numChannels];

  // Create AudioBuffer (config details + pointer(s) to bufferMemory)
  AudioBuffer buffer{};
  buffer.format = bufferFormat;
  buffer.numChannels = numChannels;
  buffer.numFrames = numFrames;

  // Set bufferMemory pointer(s) based on bufferFormat;
  if (bufferFormat == BufferFormat::NonInterleaved) {

    // Non-Interleaved == buffer per channel (Narrow Frame)
    buffer.channelPtrs = new float*[numChannels];

    for (size_t i = 0; i < numChannels; i++)
      buffer.channelPtrs[i] = sessionPtr->bufferMemory + (numFrames * i);
  } else {

    // Interleaved == single buffer with interwoven channels (Wide Frame)
    buffer.interleavedPtr = sessionPtr->bufferMemory;
  }

  sessionPtr->buffer = buffer;

  return sessionPtr;
}

int startAudioSession(hAudioSession sessionPtr) {
  int errCode = CoreAudioAdapter::coreAudioStart(sessionPtr);
  if (errCode) {
    printf("Platform audio start failed: %d", errCode);
    return errCode;
  }
  return 0;
}

int stopAudioSession(hAudioSession sessionPtr) {
  int errCode = CoreAudioAdapter::coreAudioStop(sessionPtr);
  if (errCode) {
    printf("Platform audio stop failed: %d", errCode);
    return errCode;
  }
  return 0;
}

int cleanupAudioSession(hAudioSession sessionPtr) {
  int errCode = CoreAudioAdapter::coreAudioCleanup(sessionPtr);
  if (errCode) {
    printf("Platform cleanup failed: %d", errCode);
    return errCode;
  }

  if (sessionPtr->buffer.format == BufferFormat::NonInterleaved)
    delete[] sessionPtr->buffer.channelPtrs;

  delete[] sessionPtr->bufferMemory;
  delete sessionPtr;
  return 0;
}

} // namespace audio_io
