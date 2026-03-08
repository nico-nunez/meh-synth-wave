#pragma once

#include "audio_io/AudioIOMacros.h"
#include "audio_io/AudioIOTypes.h"
#include "audio_io/AudioIOTypesFwd.h"

#include "AudioToolbox/AudioToolbox.h"

namespace CoreAudioAdapter {

audio_io::DeviceInfo queryDefaultOutputDevice();

AudioStreamBasicDescription configToASBD(const audio_io::Config& config);

PLATFORM_SETUP(coreAudioSetup);
PLATFORM_START(coreAudioStart);
PLATFORM_STOP(coreAudioStop);
PLATFORM_CLEANUP(coreAudioCleanup);

} // namespace CoreAudioAdapter
