#include "Engine.h"
#include "ParamBindings.h"
#include "VoicePool.h"

#include "synth_io/Events.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdio>

namespace synth {
using synth_io::MIDIEvent;
using synth_io::ParamEvent;

Engine createEngine(const EngineConfig& config) {
  Engine engine{};

  voices::initVoicePool(engine.voicePool, config);

  param::bindings::initParamBindings(engine);

  engine.initMIDIBindings();

  return engine;
}

void Engine::processParamEvent(const ParamEvent& event) {
  param::bindings::setParamValueByID(*this, static_cast<ParamID>(event.id), event.value);
}

// =============================
// MIDI Event Handlers
// =============================
void Engine::initMIDIBindings() {
  using param::bindings::ParamID;

  for (auto& cc : ccTable)
    cc = ParamID::UNKOWN;

  ccTable[7] = ParamID::MASTER_GAIN;
  ccTable[74] = ParamID::SVF_CUTOFF;
  ccTable[71] = ParamID::SVF_RESONANCE;
}

void Engine::handleCC(uint8_t cc, uint8_t value) {
  using namespace param::bindings;

  if (cc == 1) {
    // handleModWheel(value);
    return;
  }
  if (cc == 64) {
    // handleSustain(value);
    return;
  }

  ParamID paramID = ccTable[cc];
  if (paramID == ParamID::UNKOWN)
    return;

  auto& binding = paramBindings[paramID];
  float denorm = binding.min + (value / 127.0f) * (binding.max - binding.min);

  setParamValueByID(*this, paramID, denorm);
}

void Engine::processMIDIEvent(const synth_io::MIDIEvent& event) {
  using Type = MIDIEvent::Type;

  switch (event.type) {
  case Type::NoteOn:
    if (event.data.noteOn.velocity > 0)
      voices::handleNoteOn(voicePool,
                           event.data.noteOn.note,
                           event.data.noteOn.velocity,
                           ++noteCount,
                           sampleRate);
    else
      voices::releaseVoice(voicePool, event.data.noteOn.note);
    break;

  case Type::NoteOff:
    voices::handleNoteOff(voicePool, event.data.noteOff.note);
    break;

  case Type::ControlChange:
    handleCC(event.data.cc.number, event.data.cc.value);
    break;

  case Type::PitchBend:
    // TODO
    break;

  // TODO(nico)...at some point
  case Type::Aftertouch:
    break;
  case Type::ChannelPressure:
    break;
  case Type::ProgramChange:
    break;

  default:
    break;
  }
}

void Engine::processAudioBlock(float** outputBuffer, size_t numChannels, size_t numFrames) {
  /* NOTE(nico): Use internal Engine block size to allow processing of
   * expensive calculation that need to occur more often than once per audio
   * buffer block but NOT on every sample either.  E.g. Modulation
   *
   * TODO(nico): mess with ENGINE_BLOCK_SIZE value (currently 64) to see
   * how it effects things
   */
  uint32_t offset = 0;
  while (offset < numFrames) {
    uint32_t blockSize = std::min(ENGINE_BLOCK_SIZE, static_cast<uint32_t>(numFrames) - offset);
    voices::processVoices(voicePool, poolBuffer + offset, blockSize);
    offset += blockSize;
  }

  for (size_t frame = 0; frame < numFrames; frame++) {
    for (size_t ch = 0; ch < numChannels; ch++) {
      outputBuffer[ch][frame] = poolBuffer[frame];
    }
  }
}

} // namespace synth
