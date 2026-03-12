#include "PresetApply.h"

#include "synth/Engine.h"
#include "synth/ModMatrix.h"
#include "synth/ParamBindings.h"
#include "synth/ParamDefs.h"
#include "synth/ParamNames.h"
#include "synth/ParamRanges.h"
#include "synth/SignalChain.h"
#include "synth/WavetableBanks.h"
#include "synth/WavetableOsc.h"

namespace synth::preset {

namespace pb = param::bindings;
namespace pn = param::names;

namespace osc = wavetable::osc;

using param::ParamID;
using wavetable::banks::getBankByName;

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

  // ==== Oscillators: direct-write fields ====
  applyOscDirect(preset.osc1, pool.osc1, "osc1", result.warnings);
  applyOscDirect(preset.osc2, pool.osc2, "osc2", result.warnings);
  applyOscDirect(preset.osc3, pool.osc3, "osc3", result.warnings);
  applyOscDirect(preset.osc4, pool.osc4, "osc4", result.warnings);

  // ==== Oscillators: bound params ====
  applyOscBound(router, pool, pb::OSC_PARAM_IDS[0], preset.osc1);
  applyOscBound(router, pool, pb::OSC_PARAM_IDS[1], preset.osc2);
  applyOscBound(router, pool, pb::OSC_PARAM_IDS[2], preset.osc3);
  applyOscBound(router, pool, pb::OSC_PARAM_IDS[3], preset.osc4);

  // ==== Noise: direct-write + bound ====
  pool.noise.type = pn::parseNoiseType(preset.noise.type.c_str());
  pb::setParamValueByID(router, pool, ParamID::NOISE_MIX_LEVEL, preset.noise.mixLevel);
  pb::setParamValueByID(router, pool, ParamID::NOISE_ENABLED, preset.noise.enabled ? 1.0f : 0.0f);

  // ==== Envelopes ====
  applyEnvBound(router, pool, pb::ENV_PARAM_IDS[0], preset.ampEnv);
  applyEnvBound(router, pool, pb::ENV_PARAM_IDS[1], preset.filterEnv);
  applyEnvBound(router, pool, pb::ENV_PARAM_IDS[2], preset.modEnv);

  // ==== SVF ====
  pb::setParamValueByID(router,
                        pool,
                        ParamID::SVF_MODE,
                        static_cast<float>(pn::parseSVFMode(preset.svf.mode.c_str())));
  pb::setParamValueByID(router, pool, ParamID::SVF_CUTOFF, preset.svf.cutoff);
  pb::setParamValueByID(router, pool, ParamID::SVF_RESONANCE, preset.svf.resonance);
  pb::setParamValueByID(router, pool, ParamID::SVF_ENABLED, preset.svf.enabled ? 1.0f : 0.0f);

  // ==== Ladder ====
  pb::setParamValueByID(router, pool, ParamID::LADDER_CUTOFF, preset.ladder.cutoff);
  pb::setParamValueByID(router, pool, ParamID::LADDER_RESONANCE, preset.ladder.resonance);
  pb::setParamValueByID(router, pool, ParamID::LADDER_DRIVE, preset.ladder.drive);
  pb::setParamValueByID(router, pool, ParamID::LADDER_ENABLED, preset.ladder.enabled ? 1.0f : 0.0f);

  // ==== Saturator ====
  pb::setParamValueByID(router, pool, ParamID::SATURATOR_DRIVE, preset.saturator.drive);
  pb::setParamValueByID(router, pool, ParamID::SATURATOR_MIX, preset.saturator.mix);
  pb::setParamValueByID(router,
                        pool,
                        ParamID::SATURATOR_ENABLED,
                        preset.saturator.enabled ? 1.0f : 0.0f);

  // ==== LFOs: direct-write + bound ====
  applyLFODirect(preset.lfo1, pool.lfo1, "lfo1", result.warnings);
  applyLFODirect(preset.lfo2, pool.lfo2, "lfo2", result.warnings);
  applyLFODirect(preset.lfo3, pool.lfo3, "lfo3", result.warnings);

  applyLFOBound(router, pool, pb::LFO_PARAM_IDS[0], preset.lfo1);
  applyLFOBound(router, pool, pb::LFO_PARAM_IDS[1], preset.lfo2);
  applyLFOBound(router, pool, pb::LFO_PARAM_IDS[2], preset.lfo3);

  // ==== Global ====
  pb::setParamValueByID(router, pool, ParamID::PITCH_BEND_RANGE, preset.global.pitchBendRange);

  // ==== Voice modes ====
  // Apply mono BEFORE porta — mono.enabled triggers voice release in onParamUpdate
  pb::setParamValueByID(router, pool, ParamID::MONO_ENABLED, preset.mono.enabled ? 1.0f : 0.0f);
  pb::setParamValueByID(router, pool, ParamID::MONO_LEGATO, preset.mono.legato ? 1.0f : 0.0f);

  pb::setParamValueByID(router, pool, ParamID::PORTA_TIME, preset.porta.time);
  pb::setParamValueByID(router, pool, ParamID::PORTA_LEGATO, preset.porta.legato ? 1.0f : 0.0f);
  pb::setParamValueByID(router, pool, ParamID::PORTA_ENABLED, preset.porta.enabled ? 1.0f : 0.0f);

  pb::setParamValueByID(router,
                        pool,
                        ParamID::UNISON_VOICES,
                        static_cast<float>(preset.unison.voices));
  pb::setParamValueByID(router, pool, ParamID::UNISON_DETUNE, preset.unison.detune);
  pb::setParamValueByID(router, pool, ParamID::UNISON_SPREAD, preset.unison.spread);
  pb::setParamValueByID(router, pool, ParamID::UNISON_ENABLED, preset.unison.enabled ? 1.0f : 0.0f);

  // ==== Mod matrix: rebuild from scratch ====
  mod_matrix::clearRoutes(pool.modMatrix);

  for (const auto& route : preset.modMatrix) {
    auto src = pn::parseModSrc(route.source.c_str());
    auto dest = pn::parseModDest(route.destination.c_str());

    if (src == mod_matrix::ModSrc::NoSrc) {
      result.warnings.push_back("mod route: unknown source '" + route.source + "', skipping");
      continue;
    }
    if (dest == mod_matrix::ModDest::NoDest) {
      result.warnings.push_back("mod route: unknown destination '" + route.destination +
                                "', skipping");
      continue;
    }

    float clampedAmount = clampModAmount(dest, route.amount);
    if (!mod_matrix::addRoute(pool.modMatrix, src, dest, clampedAmount)) {
      result.warnings.push_back("mod route: matrix full, skipping " + route.source + " → " +
                                route.destination);
      break;
    }
  }

  // Zero interpolation state so first block doesn't lerp from stale values
  mod_matrix::clearPrevModDests(pool.modMatrix);

  // ==== Signal chain: rebuild ====
  signal_chain::SignalProcessor procs[signal_chain::MAX_CHAIN_SLOTS];
  uint8_t procCount = 0;

  for (const auto& name : preset.signalChain) {
    auto proc = param::names::parseSignalProcessor(name.c_str());
    if (proc != signal_chain::SignalProcessor::None) {
      procs[procCount++] = proc;
    } else {
      result.warnings.push_back("signalChain: unknown processor '" + name + "', skipping");
    }
  }
  signal_chain::setChain(pool.signalChain, procs, procCount);

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
  p.noise.type = param::names::noiseTypeToString(pool.noise.type);
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
