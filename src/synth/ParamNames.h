#pragma once

#include "synth/Filters.h"
#include "synth/ModMatrix.h"
#include "synth/Noise.h"
#include "synth/SignalChain.h"

#include <cstring>

namespace synth::param::names {

// ============================================================
// SVFMode
// ============================================================

struct SVFModeMapping {
  const char* name;
  filters::SVFMode mode;
};

inline constexpr SVFModeMapping svfModeMappings[] = {
    {"lp", filters::SVFMode::LP},
    {"hp", filters::SVFMode::HP},
    {"bp", filters::SVFMode::BP},
    {"notch", filters::SVFMode::Notch},
};

inline filters::SVFMode parseSVFMode(const char* name) {
  for (const auto& m : svfModeMappings)
    if (std::strcmp(m.name, name) == 0)
      return m.mode;
  return filters::SVFMode::LP;
}

inline const char* svfModeToString(filters::SVFMode mode) {
  for (const auto& m : svfModeMappings)
    if (m.mode == mode)
      return m.name;
  return "lp";
}

// ============================================================
// NoiseType
// ============================================================

struct NoiseTypeMapping {
  const char* name;
  noise::NoiseType type;
};

inline constexpr NoiseTypeMapping noiseTypeMappings[] = {
    {"white", noise::NoiseType::White},
    {"pink", noise::NoiseType::Pink},
};

inline noise::NoiseType parseNoiseType(const char* name) {
  for (const auto& m : noiseTypeMappings)
    if (std::strcmp(m.name, name) == 0)
      return m.type;
  return noise::NoiseType::White;
}

inline const char* noiseTypeToString(noise::NoiseType type) {
  for (const auto& m : noiseTypeMappings)
    if (m.type == type)
      return m.name;
  return "white";
}

// ============================================================
// SignalProcessor
// ============================================================

struct SignalProcessorMapping {
  const char* name;
  signal_chain::SignalProcessor proc;
};

inline constexpr SignalProcessorMapping signalProcessorMappings[] = {
    {"svf", signal_chain::SignalProcessor::SVF},
    {"ladder", signal_chain::SignalProcessor::Ladder},
    {"saturator", signal_chain::SignalProcessor::Saturator},
};

inline signal_chain::SignalProcessor parseSignalProcessor(const char* name) {
  for (const auto& m : signalProcessorMappings)
    if (std::strcmp(m.name, name) == 0)
      return m.proc;
  return signal_chain::SignalProcessor::None;
}

inline const char* signalProcessorToString(signal_chain::SignalProcessor proc) {
  for (const auto& m : signalProcessorMappings)
    if (m.proc == proc)
      return m.name;
  return "unknown";
}

// ============================================================
// ModSrc / ModDest — public wrappers over ModMatrix.h tables
// ============================================================

inline mod_matrix::ModSrc parseModSrc(const char* name) {
  for (const auto& m : mod_matrix::modSrcMappings)
    if (std::strcmp(m.name, name) == 0)
      return m.src;
  return mod_matrix::ModSrc::NoSrc;
}

inline const char* modSrcToString(mod_matrix::ModSrc src) {
  for (const auto& m : mod_matrix::modSrcMappings)
    if (m.src == src)
      return m.name;
  return "unknown";
}

inline mod_matrix::ModDest parseModDest(const char* name) {
  for (const auto& m : mod_matrix::modDestMappings)
    if (std::strcmp(m.name, name) == 0)
      return m.dest;
  return mod_matrix::ModDest::NoDest;
}

inline const char* modDestToString(mod_matrix::ModDest dest) {
  for (const auto& m : mod_matrix::modDestMappings)
    if (m.dest == dest)
      return m.name;
  return "unknown";
}

} // namespace synth::param::names
