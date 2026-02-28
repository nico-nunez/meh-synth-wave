#pragma once

#include "synth/Types.h"

#include <cstdint>
#include <sstream>

namespace synth::mod_matrix {
// ==== Modulation Sources ====
enum ModSrc {
  NoSrc = 0,

  // Envelopes — all three, whether or not they're "doing anything" currently
  AmpEnv,    // 0.0–1.0, always running per voice
  FilterEnv, // 0.0–1.0, always running per voice
  ModEnv,    // 0.0–1.0, general-purpose, not hardwired to anything

  LFO1,
  LFO2,
  LFO3,

  // Per-note values — available from the moment a voice is allocated
  Velocity, // 0.0–1.0 from MIDI note-on velocity
  Noise,

  SRC_COUNT // used to size arrays, not a valid source
};

// ==== Modulation Destinations ====
enum ModDest {
  NoDest = 0,

  // Filter cutoff — units are octaves (bipolar ±4)
  SVFCutoff,
  LadderCutoff,

  // Filter resonance — units are linear ±1.0
  SVFResonance,
  LadderResonance,

  // Oscillator pitch — units are semitones (bipolar ±24)
  Osc1Pitch,
  Osc2Pitch,
  Osc3Pitch,
  SubOscPitch,

  // Oscillator mix level — units are linear ±1.0
  Osc1Mix,
  Osc2Mix,
  Osc3Mix,
  SubOscMix,

  // Wavetable scan position — 0.0–1.0 offset
  Osc1ScanPos,
  Osc2ScanPos,
  Osc3ScanPos,
  SubOscScanPos,

  // FM depth modulation — DX7-style "envelope controls FM intensity"
  Osc1FMDepth,
  Osc2FMDepth,
  Osc3FMDepth,
  SubOscFMDepth,

  DEST_COUNT // used to size arrays, not a valid dest
};

// ==== Modulation Routing ====
constexpr int MAX_MOD_ROUTES = 16;

using ModDest2D = float[ModDest::DEST_COUNT][MAX_VOICES];

struct ModRoute {
  ModSrc src = ModSrc::NoSrc;
  ModDest dest = ModDest::NoDest;
  float amount = 0.0f;
};

struct ModMatrix {
  ModRoute routes[MAX_MOD_ROUTES];
  uint8_t count = 0;

  // engine block-rate output of pre-pass
  ModDest2D destValues = {};

  // interpolation state, persists between engine blocks
  ModDest2D prevDestValues = {};

  // Stack-local inside processVoices
  // Need to zero out each loop if I keep it here
  ModDest2D destStepValues = {};
};

bool addRoute(ModMatrix &matrix, ModSrc src, ModDest dest, float amount);
bool addRoute(ModMatrix &matrix, const ModRoute &route);
bool removeRoute(ModMatrix &matrix, uint8_t index);
bool clearRoutes(ModMatrix &matrix);

void clearPrevModDests(ModMatrix &matrix);
void clearModDestSteps(ModMatrix &matrix);
void setModDestStep(ModMatrix &matrix, ModDest dest, uint32_t voiceIndex,
                    float invNumSamples);

// ==== Parsing Helpers ====
struct ModSrcMapping {
  const char *name;
  const ModSrc src;
};

inline constexpr ModSrcMapping
    modSrcMappings[ModSrc::SRC_COUNT - 1] = // Not including "NoSrc"
    {
        // Envelopes
        {"ampEnv", ModSrc::AmpEnv},
        {"filterEnv", ModSrc::FilterEnv},
        {"modEnv", ModSrc::ModEnv},

        // LFOs
        {"lfo1", ModSrc::LFO1},
        {"lfo2", ModSrc::LFO2},
        {"lfo3", ModSrc::LFO3},

        // Per-note
        {"velocity", ModSrc::Velocity},
        {"noise", ModSrc::Noise},
};

struct ModDestMapping {
  const char *name;
  const ModDest dest;
};
inline constexpr ModDestMapping
    modDestMappings[ModDest::DEST_COUNT - 1] = // Not including "NoDest"
    {
        // Filter Cutoff
        {"svf.cutoff", ModDest::SVFCutoff},
        {"ladder.cutoff", ModDest::LadderCutoff},

        // Filter Resonance
        {"svf.resonance", ModDest::SVFResonance},
        {"ladder.resonance", ModDest::LadderResonance},

        // Oscillator Pitch
        {"osc1.pitch", ModDest::Osc1Pitch},
        {"osc2.pitch", ModDest::Osc2Pitch},
        {"osc3.pitch", ModDest::Osc3Pitch},
        {"subOsc.pitch", ModDest::SubOscPitch},

        // Oscillator Mix
        {"osc1.mixLevel", ModDest::Osc1Mix},
        {"osc2.mixLevel", ModDest::Osc2Mix},
        {"osc3.mixLevel", ModDest::Osc3Mix},
        {"subOsc.mixLevel", ModDest::SubOscMix},

        // Oscillator Scan Position
        {"osc1.scanPos", ModDest::Osc1ScanPos},
        {"osc2.scanPos", ModDest::Osc2ScanPos},
        {"osc3.scanPos", ModDest::Osc3ScanPos},
        {"subOsc.scanPos", ModDest::SubOscScanPos},

        // Oscillator FM Depth
        {"osc1.fmDepth", ModDest::Osc1FMDepth},
        {"osc2.fmDepth", ModDest::Osc2FMDepth},
        {"osc3.fmDepth", ModDest::Osc3FMDepth},
        {"subOsc.fmDepth", ModDest::SubOscFMDepth},
};

void parseModCommand(std::istringstream &iss, ModMatrix &modMatrix);
} // namespace synth::mod_matrix
