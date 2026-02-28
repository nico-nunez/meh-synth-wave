#pragma once

#include "Envelope.h"
#include "Filters.h"
#include "ModMatrix.h"
#include "NoiseOscillator.h"
#include "Types.h"
#include "WavetableOsc.h"

#include <cstddef>
#include <cstdint>

namespace synth::voices {
using Envelope = envelope::Envelope;

using WavetableOsc = wavetable::osc::WavetableOscillator;
using WavetableOscConfig = wavetable::osc::WavetableOscConfig;

using NoiseOsc = noise_osc::NoiseOscillator;
using NoiseOscConfig = noise_osc::NoiseOscConfig;

using ModMatrix = mod_matrix::ModMatrix;

struct VoicePoolConfig {
  WavetableOscConfig osc1{};
  WavetableOscConfig osc2{};
  WavetableOscConfig osc3{};
  WavetableOscConfig subOsc{};

  NoiseOscConfig noiseOsc{};

  float masterGain = 1.0f;
  float sampleRate = 48000.0f;
};

// VoicePool - top-level container (universal synth)
struct VoicePool {
  // ==== Oscillators (3 main + sub oscillator) ====
  WavetableOsc osc1;
  WavetableOsc osc2;
  WavetableOsc osc3;
  WavetableOsc subOsc;

  NoiseOsc noiseOsc;

  // Reduce gain for multiple oscillators
  // TODO(nico): this needs to be tide to number of active oscs
  float oscMixGain = 1.0f / 4.0;

  // TODO(nico) ==== Noise Generator ====
  // NoiseGenerator noise;

  ModMatrix modMatrix;

  // ==== Envelopes ====
  Envelope ampEnv;    // Amplitude envelope
  Envelope filterEnv; // Filter modulation
  Envelope modEnv;    // General-purpose modulation

  // ==== Filters ====
  filters::SVFilter svf;
  filters::LadderFilter ladder;

  // TODO(nico)
  // // ====  LFOs (3 for modulation) ====
  // LFO lfo1;
  // LFO lfo2;
  // LFO lfo3;

  // TODO(nico)
  // // ==== Effects ====
  // Saturator saturator;

  float masterGain = 1.0f; // range [0.0 - 2.0]
                           // range [-inf - +6DB]

  // ==== Voice metadata ====
  uint8_t midiNotes[MAX_VOICES];    // Which MIDI note (0-127)
  float velocities[MAX_VOICES];     // Note-on velocity (0.0-1.0)
  uint32_t noteOnTimes[MAX_VOICES]; // NoteOn counter ( 1 is older than 2)
  uint8_t isActive[MAX_VOICES];     // 1 = active, 0 = free

  float sampleRate;
  float invSampleRate;

  // ==== Active voice tracking ====
  uint32_t activeCount = 0;
  uint32_t activeIndices[MAX_VOICES]; // Dense array of active indices
};

// Initialize VoicePool (once upon engin creation)
void initVoicePool(VoicePool &pool, const VoicePoolConfig &config);

// updating existing Engine member
void updateVoicePoolConfig(VoicePool &pool, const VoicePoolConfig &config);

// Find free or oldest voice index for voice Initialization
uint32_t allocateVoiceIndex(VoicePool &pool);

// Initial voice state for noteOn event
void initializeVoice(VoicePool &pool, uint32_t index, uint8_t midiNote,
                     float velocity, uint32_t noteOnTime, float sampleRate);

// Trigger envelope release for voice playing midiNote
void releaseVoice(VoicePool &pool, uint8_t midiNote);

// Add newly active voice (noteOn)
void addActiveIndex(VoicePool &pool, uint32_t voiceIndex);

// Remove an inactive voice (noteOff)
void removeInactiveIndex(VoicePool &pool, uint32_t voiceIndex);

void processVoices(VoicePool &pool, float *output, size_t numSamples);

void handleNoteOn(VoicePool &pool, uint8_t midiNote, float velocity,
                  uint32_t noteOnTime, float sampleRate);

} // namespace synth::voices
