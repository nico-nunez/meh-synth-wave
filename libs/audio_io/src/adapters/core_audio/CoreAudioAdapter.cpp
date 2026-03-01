#include <AudioToolbox/AudioToolbox.h>

#include "CoreAudioAdapter.h"
#include "audio_io/AudioIOTypes.h"
#include "shared/AudioSession.h"

#include <cassert>
#include <cstddef>
#include <cstdio>

namespace CoreAudioAdapter {

// Private to this file - no one else sees this type
struct CoreAudioContext {
  AudioUnit audioUnit;
};

/* ============ (Core Audio Native Callback) ============
 * This is the function signature required by Core Audio
 * Library and user logic needs to occur in here
 *
 * TODO(nico): Figure out way to pass native buffer if buffer.format
 * matches native.  Avoids copy altogether
 */
static OSStatus nativeCallback(void* inRefCon, // ← CoreAudio gives us back what we registered
                               AudioUnitRenderActionFlags* /*ioActionFlags*/,
                               const AudioTimeStamp* /*inTimeStamp*/,
                               UInt32 /*inBusNumber*/,
                               UInt32 inNumberFrames,
                               AudioBufferList* ioData) {

  auto sessionPtr = static_cast<audio_io::hAudioSession>(inRefCon);

  /* NOTE(nico): ioData->mNumberBuffers value should match the number of buffer
   * pointers within AudioBuffer. These are determined during config/setup and
   * SHOULD NOT change.
   *
   * Non-Interleaved = mNumberBuffers == number of channelPtrs (aka numChannels)
   * Interleaved: mNumberBuffers == interleavedPtr (aka 1)
   */

#ifndef NDEBUG
  assert(sessionPtr->buffer.numFrames == inNumberFrames);

  if (sessionPtr->buffer.format == audio_io::BufferFormat::NonInterleaved) {
    assert(ioData->mNumberBuffers == sessionPtr->buffer.numChannels);
  } else {
    assert(ioData->mNumberBuffers == 1);
  }
#endif

  sessionPtr->userCallback(sessionPtr->buffer, sessionPtr->userContext);

  /* Copy (and convert if required) bufferMemory to Core Audio's buffer
   * NOTE: _src_ is user configurable but _dst_ is not at this time
   */
  if (sessionPtr->buffer.format == audio_io::BufferFormat::NonInterleaved) {
    // Non-Interleaved (src) -> Non-Interleaved (dst)
    assert(sessionPtr->buffer.channelPtrs);

    for (size_t ch = 0; ch < sessionPtr->buffer.numChannels; ch++) {
      float* dstPtr = static_cast<float*>(ioData->mBuffers[ch].mData);
      float* srcPtr = sessionPtr->buffer.channelPtrs[ch];

      assert(dstPtr);

      for (size_t i = 0; i < sessionPtr->buffer.numFrames; i++)
        dstPtr[i] = srcPtr[i];
    }
  } else {
    // Interleaved (src) -> Non-Interleaved (dst)
    assert(sessionPtr->buffer.interleavedPtr);

    size_t stride = sessionPtr->buffer.numChannels;

    for (size_t ch = 0; ch < sessionPtr->buffer.numChannels; ch++) {
      float* dstPtr = static_cast<float*>(ioData->mBuffers[ch].mData);
      float* srcPtr = sessionPtr->buffer.interleavedPtr + ch;

      assert(dstPtr);

      // Fill channel
      for (size_t i = 0; i < sessionPtr->buffer.numFrames; i++) {
        dstPtr[i] = srcPtr[i * stride];
      }
    }
  }

  return noErr;
}

/* ============ (Config Helper) ============
 * This function is for converting the user provided config
 * data to AudioStreamBasicDescription, which is required for
 * Core Audio setup
 */
AudioStreamBasicDescription configToASBD(const audio_io::Config& config) {

  // Create Core Audio stream description based on user's config
  AudioStreamBasicDescription asbd{};
  asbd.mSampleRate = config.sampleRate;
  asbd.mFormatID = kAudioFormatLinearPCM;
  asbd.mFormatFlags =
      kAudioFormatFlagIsFloat | kAudioFormatFlagIsPacked | kAudioFormatFlagIsNonInterleaved;
  asbd.mBitsPerChannel = 32;
  asbd.mChannelsPerFrame = config.numChannels;
  asbd.mFramesPerPacket = 1;                      // Always 1 for PCM
  asbd.mBytesPerFrame = asbd.mBitsPerChannel / 8; // Times nChannels if interleaved
  asbd.mBytesPerPacket = asbd.mBytesPerFrame * asbd.mFramesPerPacket;

  return asbd;
}

/* ============ (Core Audio Setup/Init) ============
 * This function contains all the logic/steps required
 * to setup and initialize Core Audio compatability and
 * will be called by AudioSession.
 *
 * TODO(nico): AudioComponentDescription should be user config
 * TODO(nico): AudioObjectGetPropertyData w\ kAudioHardwarePropertyDevices
 * TODO(later): handle config incompatability
 * TODO(later): error handling in general
 */
int coreAudioSetup(audio_io::hAudioSession sessionPtr) {
  // 1. Set audio component description values
  AudioComponentDescription acDesc{};

  // Desired hardware I/O (input or output)
  acDesc.componentType = kAudioUnitType_Output;

  // Give us whatever the user has as default for the
  // desired componentType (i.e. mic, speakers, soundcard, etc)
  acDesc.componentSubType = kAudioUnitSubType_DefaultOutput;

  // Searches plugins dir?
  acDesc.componentManufacturer = kAudioUnitManufacturer_Apple;

  // 2. Create AudioComoponent based on description
  // TODO(nico): Will this ever fail--currently?
  AudioComponent component = AudioComponentFindNext(NULL, &acDesc);
  if (!component) {
    printf("Unable to find matching audio component.\n");
    return 1;
  }

  // 3.  Create AudioUnit instance
  AudioUnit audioUnit{};
  OSStatus instanceErr = AudioComponentInstanceNew(component, &audioUnit);
  if (instanceErr) {
    printf("Error occured creating AudioComponentInstance: %i\n", instanceErr);
    return 2;
  }

  // IMPORTANT(nico-nunez):  MUST DISPOSE __audioUnit__ ON ERROR!!!!
  // WARNING(nico-nunez):  MUST DISPOSE __audioUnit__ ON ERROR!!!!

  // 4. Configure stream format
  AudioStreamBasicDescription streamDescription{configToASBD(sessionPtr->userConfig)};

  OSStatus streamDescPropErr = AudioUnitSetProperty(audioUnit,
                                                    kAudioUnitProperty_StreamFormat,
                                                    kAudioUnitScope_Input,
                                                    0,
                                                    &streamDescription,
                                                    sizeof(streamDescription));

  if (streamDescPropErr) {
    printf("Error occured with AudioUnitSetProperty: %i\n", streamDescPropErr);
    AudioComponentInstanceDispose(audioUnit);
    return 3;
  }

  // 4a. Create and set Core Audio context to Handle context
  auto* platformContext = new CoreAudioContext{};
  platformContext->audioUnit = audioUnit;

  sessionPtr->platformContext = platformContext;

  // IMPORTANT(nico-nunez):  MUST DISPOSE of __platformContext__ ON ERROR

  // 5. Set render callback
  AURenderCallbackStruct callbackStruct{};
  callbackStruct.inputProc = nativeCallback;
  callbackStruct.inputProcRefCon = sessionPtr;

  OSStatus callbackPropErr = AudioUnitSetProperty(audioUnit,
                                                  kAudioUnitProperty_SetRenderCallback,
                                                  kAudioUnitScope_Input,
                                                  0,
                                                  &callbackStruct,
                                                  sizeof(callbackStruct));

  if (callbackPropErr) {
    printf("Error occured with AudioUnitSetProperty: %i\n", callbackPropErr);
    AudioComponentInstanceDispose(audioUnit);

    delete platformContext;
    sessionPtr->platformContext = nullptr;
    return 4;
  }

  // 6. Initialize
  OSStatus initErr = AudioUnitInitialize(audioUnit);

  if (initErr) {
    printf("Failed to initialize AudioUnit: %i\n", initErr);
    AudioComponentInstanceDispose(audioUnit);
    delete platformContext;
    sessionPtr->platformContext = nullptr;
    return 5;
  }

  return 0;
}

// ============ (Core Audio Methods) ============
int coreAudioStart(audio_io::hAudioSession sessionPtr) {
  auto* ctx = static_cast<CoreAudioContext*>(sessionPtr->platformContext);
  if (!ctx) {
    printf("Unable to [start] AudioSession");
    return 1; // TODO(nico-nunez): return better error
  }
  return AudioOutputUnitStart(ctx->audioUnit);
}

int coreAudioStop(audio_io::hAudioSession sessionPtr) {
  auto* ctx = static_cast<CoreAudioContext*>(sessionPtr->platformContext);
  if (!ctx) {
    printf("Unable to [stop] AudioSession");
    return 1; // TODO(nico-nunez): return better error
  }
  return AudioOutputUnitStop(ctx->audioUnit);
}

int coreAudioCleanup(audio_io::hAudioSession sessionPtr) {
  auto* ctx = static_cast<CoreAudioContext*>(sessionPtr->platformContext);
  if (!ctx) {
    printf("Platform context does not exit [cleanup]");
    return 1; // TODO(nico-nunez): return better error
  }

  OSStatus unInitErr = AudioUnitUninitialize(ctx->audioUnit);
  if (unInitErr) {
    printf("Unable to uninitialize AudioUnit [cleanup]");
    return unInitErr;
  }

  OSStatus disposeErr = AudioComponentInstanceDispose(ctx->audioUnit);
  if (disposeErr) {
    printf("Unable to dispose of AudioUnit Instance [cleanup]");
    return disposeErr;
  }

  delete ctx;
  return 0;
}

} // namespace CoreAudioAdapter
