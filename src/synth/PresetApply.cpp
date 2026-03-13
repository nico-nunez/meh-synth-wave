#include "PresetApply.h"

#include "synth/Engine.h"
#include "synth/ModMatrix.h"
#include "synth/ParamBindings.h"
#include "synth/ParamDefs.h"
#include "synth/ParamRanges.h"
#include "synth/SignalChain.h"
#include "synth/WavetableBanks.h"
#include "synth/WavetableOsc.h"

namespace synth::preset {

namespace pb = param::bindings;

namespace osc = wavetable::osc;
namespace banks = wavetable::banks;

using param::ParamID;

// ============================================================
// Helpers (anonymous)
// ============================================================
namespace {

// Clamp mod route amount based on destination type.
float clampModAmount(mod_matrix::ModDest dest, float amount) {
  using namespace mod_matrix;
  using namespace param::ranges::mod;

  switch (dest) {
  case SVFCutoff:
  case LadderCutoff:
    return clampCutoffMod(amount);
  case SVFResonance:
  case LadderResonance:
    return clampResonanceMod(amount);
  case Osc1Pitch:
  case Osc2Pitch:
  case Osc3Pitch:
  case Osc4Pitch:
    return clampPitchMod(amount);
  case Osc1Mix:
  case Osc2Mix:
  case Osc3Mix:
  case Osc4Mix:
    return clampMixLevelMod(amount);
  case Osc1ScanPos:
  case Osc2ScanPos:
  case Osc3ScanPos:
  case Osc4ScanPos:
    return clampScanPosMod(amount);
  case Osc1FMDepth:
  case Osc2FMDepth:
  case Osc3FMDepth:
  case Osc4FMDepth:
    return clampFMDepthMod(amount);
  case LFO1Rate:
  case LFO2Rate:
  case LFO3Rate:
    return clampLFORateMod(amount);
  case LFO1Amplitude:
  case LFO2Amplitude:
  case LFO3Amplitude:
    return clampLFOAmplitudeMod(amount);
  default:
    return amount; // unknown dest — pass through
  }
}

// Resolve osc bank strings → WavetableBank pointers, fmSource strings → FMSource enums.
void applyOscStrings(const PresetStrings& strings,
                     voices::VoicePool& pool,
                     std::vector<std::string>& warnings) {
  osc::WavetableOsc* oscs[] = {&pool.osc1, &pool.osc2, &pool.osc3, &pool.osc4};
  const char* names[] = {"osc1", "osc2", "osc3", "osc4"};

  for (int i = 0; i < 4; i++) {
    // Bank
    auto* bank = banks::getBankByName(strings.oscBanks[i].c_str());
    if (bank) {
      oscs[i]->bank = bank;
    } else {
      oscs[i]->bank = banks::getBankByID(banks::BankID::Sine); // default
      warnings.push_back(std::string(names[i]) + ".bank: unknown '" + strings.oscBanks[i] +
                         "', using sine");
    }

    // FM Source
    oscs[i]->fmSource = osc::parseFMSource(strings.oscFmSources[i].c_str());
  }
}

void applyNoiseType(const std::string& typeStr,
                    voices::VoicePool& pool,
                    std::vector<std::string>& warnings) {
  pool.noise.type = noise::parseNoiseType(typeStr.c_str());
}

// SVF mode is already written as a float by the param loop (SVF_MODE param).
// But the binding writes to the SVFMode enum via the FilterMode binding type,
// so the param loop handles it. Nothing extra needed here unless the
// string→enum parse produces a different value than what's in paramValues[].
//
// ARCHITECTURAL NOTE: SVF mode flows through two paths:
//   1. paramValues[SVF_MODE] → setParamValueByID → writes SVFMode enum via FilterMode binding
//   2. strings.svfMode → parseSVFMode → (would write SVFMode enum directly)
// The param path (1) is authoritative. The string field exists for JSON
// round-tripping. On apply, path (1) handles it. On capture, we read the
// enum and convert to string for the preset.

void applyLFOBanks(const PresetStrings& strings,
                   voices::VoicePool& pool,
                   std::vector<std::string>& warnings) {
  lfo::LFO* lfos[] = {&pool.lfo1, &pool.lfo2, &pool.lfo3};
  const char* names[] = {"lfo1", "lfo2", "lfo3"};

  for (int i = 0; i < 3; i++) {
    if (strings.lfoBanks[i] == "sah") {
      lfos[i]->bank = nullptr; // S&H sentinel
    } else {
      auto* bank = banks::getBankByName(strings.lfoBanks[i].c_str());
      if (bank) {
        lfos[i]->bank = bank;
      } else {
        lfos[i]->bank = banks::getBankByID(banks::BankID::Sine); // default
        warnings.push_back(std::string(names[i]) + ".bank: unknown '" + strings.lfoBanks[i] +
                           "', using sine");
      }
    }
  }
}

// Apply a single oscillator's direct-write fields (bank, fmSource).
// Returns warning string if bank not found, empty otherwise.
void applyOscDirect(const OscPreset& src,
                    osc::WavetableOsc& dest,
                    const char* oscName,
                    std::vector<std::string>& warnings) {
  // Bank
  auto* bank = getBankByName(src.bank.c_str());
  if (bank) {
    dest.bank = bank;
  } else {
    dest.bank = getBankByName("sine");
    warnings.push_back(std::string(oscName) + ".bank: unknown '" + src.bank + "', using sine");
  }

  // FM Source
  dest.fmSource = osc::parseFMSource(src.fmSource.c_str());
}

// Apply a single oscillator's bound params.
void applyOscBound(pb::ParamRouter& router,
                   voices::VoicePool& pool,
                   const pb::OscParamIDs ids,
                   const OscPreset& src) {
  pb::setParamValueByID(router, pool, ids.mixLevel, src.mixLevel);
  pb::setParamValueByID(router, pool, ids.detune, src.detuneAmount);
  pb::setParamValueByID(router, pool, ids.octave, static_cast<float>(src.octaveOffset));
  pb::setParamValueByID(router, pool, ids.scanPos, src.scanPos);
  pb::setParamValueByID(router, pool, ids.fmDepth, src.fmDepth);
  pb::setParamValueByID(router, pool, ids.fmRatio, src.fmRatio);
  pb::setParamValueByID(router, pool, ids.enabled, src.enabled ? 1.0f : 0.0f);
}

// Apply an envelope's bound params.
void applyEnvBound(pb::ParamRouter& router,
                   voices::VoicePool& pool,
                   const pb::EnvParamIDs ids,
                   const EnvelopePreset& src) {
  pb::setParamValueByID(router, pool, ids.attack, src.attackMs);
  pb::setParamValueByID(router, pool, ids.decay, src.decayMs);
  pb::setParamValueByID(router, pool, ids.sustain, src.sustainLevel);
  pb::setParamValueByID(router, pool, ids.release, src.releaseMs);
  pb::setParamValueByID(router, pool, ids.attackCurve, src.attackCurve);
  pb::setParamValueByID(router, pool, ids.decayCurve, src.decayCurve);
  pb::setParamValueByID(router, pool, ids.releaseCurve, src.releaseCurve);
}

// Apply a single LFO's direct-write fields (bank).
void applyLFODirect(const LFOPreset& src,
                    lfo::LFO& dest,
                    const char* lfoName,
                    std::vector<std::string>& warnings) {
  if (src.bank == "sah") {
    dest.bank = nullptr; // S&H sentinel
  } else {
    auto* bank = getBankByName(src.bank.c_str());
    if (bank) {
      dest.bank = bank;
    } else {
      dest.bank = getBankByName("sine");
      warnings.push_back(std::string(lfoName) + ".bank: unknown '" + src.bank + "', using sine");
    }
  }
}

// Apply a single LFO's bound params.
void applyLFOBound(pb::ParamRouter& router,
                   voices::VoicePool& pool,
                   const pb::LFOParamIDs ids,
                   const LFOPreset& src) {
  pb::setParamValueByID(router, pool, ids.rate, src.rate);
  pb::setParamValueByID(router, pool, ids.amplitude, src.amplitude);
  pb::setParamValueByID(router, pool, ids.retrigger, src.retrigger ? 1.0f : 0.0f);
}

} // namespace

// ============================================================
// Apply Preset
// ============================================================
ApplyResult applyPreset(const Preset& preset, Engine& engine) {
  ApplyResult result;
  auto& pool = engine.voicePool;
  auto& router = engine.paramRouter;

  // ==== All bound params in one loop ====
  for (int i = 0; i < param::PARAM_COUNT - 1; i++) {
    auto id = static_cast<param::ParamID>(i);
    pb::setParamValueByID(router, pool, id, preset.paramValues[i]);
  }

  // ==== String fields — direct-write (bank/mode/type resolution) ====
  applyOscStrings(preset.strings, pool, result.warnings);
  applyNoiseType(preset.strings.noiseType, pool, result.warnings);
  applySVFMode(preset.strings.svfMode, pool); // already applied as param, just need enum
  applyLFOBanks(preset.strings, pool, result.warnings);

  // ==== Mod matrix: rebuild from scratch ====
  mod_matrix::clearRoutes(pool.modMatrix);
  for (const auto& route : preset.modMatrix) {
    // ... same validation + clampModAmount logic as today ...
  }
  mod_matrix::clearPrevModDests(pool.modMatrix);

  // ==== Signal chain ====
  // ... same string→enum parsing as today ...

  return result;
}

// ============================================================
// Helpers for capture
// ============================================================

namespace {

OscPreset captureOsc(const osc::WavetableOsc& osc) {
  OscPreset p;
  p.bank = osc.bank ? osc.bank->name : "sine";
  p.scanPos = osc.scanPos;
  p.mixLevel = osc.mixLevel;
  p.fmDepth = osc.fmDepth;
  p.fmRatio = osc.fmRatio;
  p.fmSource = osc::fmSourceToString(osc.fmSource);
  p.octaveOffset = osc.octaveOffset;
  p.detuneAmount = osc.detuneAmount;
  p.enabled = osc.enabled;
  return p;
}

EnvelopePreset captureEnv(const envelope::Envelope& env) {
  EnvelopePreset p;
  p.attackMs = env.attackMs;
  p.decayMs = env.decayMs;
  p.sustainLevel = env.sustainLevel;
  p.releaseMs = env.releaseMs;
  p.attackCurve = env.attackCurveParam;
  p.decayCurve = env.decayCurveParam;
  p.releaseCurve = env.releaseCurveParam;
  return p;
}

LFOPreset captureLFO(const lfo::LFO& lfo) {
  LFOPreset p;
  p.bank = lfo.bank ? lfo.bank->name : "sah";
  p.rate = lfo.rate;
  p.amplitude = lfo.amplitude;
  p.retrigger = lfo.retrigger;
  return p;
}

} // namespace

// ============================================================
// capturePreset
// ============================================================

Preset capturePreset(const Engine& engine) {
  const auto& pool = engine.voicePool;

  Preset p;
  p.version = CURRENT_PRESET_VERSION;

  // Metadata: leave as defaults (caller sets name before saving)

  // ==== Oscillators ====
  p.osc1 = captureOsc(pool.osc1);
  p.osc2 = captureOsc(pool.osc2);
  p.osc3 = captureOsc(pool.osc3);
  p.osc4 = captureOsc(pool.osc4);

  // ==== Noise ====
  p.noise.mixLevel = pool.noise.mixLevel;
  p.noise.type = noise::noiseTypeToString(pool.noise.type);
  p.noise.enabled = pool.noise.enabled;

  // ==== Envelopes ====
  p.ampEnv = captureEnv(pool.ampEnv);
  p.filterEnv = captureEnv(pool.filterEnv);
  p.modEnv = captureEnv(pool.modEnv);

  // ==== SVF ====
  p.svf.mode = param::names::svfModeToString(pool.svf.mode);
  p.svf.cutoff = pool.svf.cutoff;
  p.svf.resonance = pool.svf.resonance;
  p.svf.enabled = pool.svf.enabled;

  // ==== Ladder ====
  p.ladder.cutoff = pool.ladder.cutoff;
  p.ladder.resonance = pool.ladder.resonance;
  p.ladder.drive = pool.ladder.drive;
  p.ladder.enabled = pool.ladder.enabled;

  // ==== Saturator ====
  p.saturator.drive = pool.saturator.drive;
  p.saturator.mix = pool.saturator.mix;
  p.saturator.enabled = pool.saturator.enabled;

  // ==== LFOs ====
  p.lfo1 = captureLFO(pool.lfo1);
  p.lfo2 = captureLFO(pool.lfo2);
  p.lfo3 = captureLFO(pool.lfo3);

  // ==== Mod matrix ====
  p.modMatrix.clear();
  for (uint8_t i = 0; i < pool.modMatrix.count; i++) {
    const auto& route = pool.modMatrix.routes[i];
    ModRoutePreset rp;
    rp.source = param::names::modSrcToString(route.src);
    rp.destination = param::names::modDestToString(route.dest);
    rp.amount = route.amount;
    p.modMatrix.push_back(rp);
  }

  // ==== Signal chain ====
  p.signalChain.clear();
  for (uint8_t i = 0; i < pool.signalChain.length; i++) {
    const char* name = param::names::signalProcessorToString(pool.signalChain.slots[i]);
    if (std::strcmp(name, "unknown") != 0) {
      p.signalChain.push_back(name);
    }
  }

  // ==== Voice modes ====
  p.mono.enabled = pool.mono.enabled;
  p.mono.legato = pool.mono.legato;

  p.porta.time = pool.porta.time;
  p.porta.legato = pool.porta.legato;
  p.porta.enabled = pool.porta.enabled;

  p.unison.voices = pool.unison.voices;
  p.unison.detune = pool.unison.detune;
  p.unison.spread = pool.unison.spread;
  p.unison.enabled = pool.unison.enabled;

  // ==== Global ====
  p.global.pitchBendRange = pool.pitchBend.range;

  return p;
}
} // namespace synth::preset
