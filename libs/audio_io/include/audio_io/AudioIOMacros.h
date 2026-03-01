#pragma once

#define PLATFORM_SETUP(name)   int name(audio_io::hAudioSession sessionPtr)
#define PLATFORM_START(name)   int name(audio_io::hAudioSession sessionPtr)
#define PLATFORM_STOP(name)    int name(audio_io::hAudioSession sessionPtr)
#define PLATFORM_CLEANUP(name) int name(audio_io::hAudioSession sessionPtr)
