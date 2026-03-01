#include "audio_io/AudioIO.h"
#include "audio_io/AudioIOTypes.h"

namespace audio_io {
struct AudioSession {
  float* bufferMemory; // Actual memory buffer
  AudioBuffer buffer;  // user facing with pointers to bufferMemory

  Config userConfig;

  AudioCallback userCallback;
  void* userContext;

  void* platformContext;

  bool isValid() const { return bufferMemory != nullptr && platformContext != nullptr; }
};

using hAudioSession = AudioSession*;
} // namespace audio_io
