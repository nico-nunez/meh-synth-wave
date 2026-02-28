#include "WavetableBanks.h"
#include "dsp/Math.h"
#include "dsp/Wavetable.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>

namespace synth::wavetable::banks {
namespace dsp_wt = dsp::wavetable;
using WavetableFrame = dsp::wavetable::WavetableFrame;

namespace {
float invTableSize = 1.0f / dsp_wt::TABLE_SIZE_F;

inline float sineAt(uint32_t harmonic, uint32_t sampleIndex) {
  return sinf(dsp::math::TWO_PI_F * static_cast<float>(harmonic) *
              static_cast<float>(sampleIndex) * invTableSize);
}

WavetableBank *createSineBank() {
  WavetableBank *bank = dsp_wt::createWavetableBank(1, "sine");
  WavetableFrame *frame = &bank->frames[0];

  // Mip Level
  for (uint32_t mipLevel = 0; mipLevel < dsp_wt::MAX_MIP_LEVELS; mipLevel++) {

    // Sample Level
    for (uint32_t sampleIndex = 0; sampleIndex < dsp_wt::TABLE_SIZE;
         sampleIndex++) {

      // Pure sine (1 harmonics)
      frame->mips[mipLevel][sampleIndex] = sineAt(1, sampleIndex);
    }
  }

  registerBank(BankID::Sine, bank);
  return bank;
}

WavetableBank *createSawBank() {
  WavetableBank *bank = dsp_wt::createWavetableBank(1, "saw");
  WavetableFrame *frame = &bank->frames[0];

  uint32_t tableLimit = dsp_wt::TABLE_SIZE / 2;

  // Mip Level
  for (uint32_t mipLevel = 0; mipLevel < dsp_wt::MAX_MIP_LEVELS; mipLevel++) {
    uint32_t mipLimit = std::max(1u, tableLimit >> mipLevel);

    // Sample Level
    for (uint32_t sampleIndex = 0; sampleIndex < dsp_wt::TABLE_SIZE;
         sampleIndex++) {
      float sample = 0.0f;

      // Harminic Level (odd & even)
      for (uint32_t harmonic = 1; harmonic <= mipLimit; harmonic++) {
        float sign = ((harmonic & 1) == 1) ? 1.0f : -1.0f; // is odd
        sample +=
            sign * sineAt(harmonic, sampleIndex) / static_cast<float>(harmonic);
      }

      frame->mips[mipLevel][sampleIndex] =
          sample * (2.0f * dsp::math::INV_PI_F);
    }
  }

  registerBank(BankID::Saw, bank);
  return bank;
}

WavetableBank *createSquareBank() {
  WavetableBank *bank = dsp_wt::createWavetableBank(1, "square");
  WavetableFrame *frame = &bank->frames[0];

  uint32_t tableLimit = dsp_wt::TABLE_SIZE / 2;

  // Mip Level
  for (uint32_t mipLevel = 0; mipLevel < dsp_wt::MAX_MIP_LEVELS; mipLevel++) {
    uint32_t mipLimit = std::max(1u, tableLimit >> mipLevel);

    // Sample Level
    for (uint32_t sampleIndex = 0; sampleIndex < dsp_wt::TABLE_SIZE;
         sampleIndex++) {
      float sample = 0.0f;

      // Harminic Level (only odd)
      for (uint32_t harmonic = 1; harmonic <= mipLimit; harmonic += 2) {
        sample += sineAt(harmonic, sampleIndex) / static_cast<float>(harmonic);
      }

      frame->mips[mipLevel][sampleIndex] =
          sample * (4.0f * dsp::math::INV_PI_F);
    }
  }

  registerBank(BankID::Square, bank);
  return bank;
}

WavetableBank *createTriangleBank() {
  WavetableBank *bank = dsp_wt::createWavetableBank(1, "triangle");
  WavetableFrame *frame = &bank->frames[0];

  uint32_t tableLimit = dsp_wt::TABLE_SIZE / 2;

  // Mip Level
  for (uint32_t mipLevel = 0; mipLevel < dsp_wt::MAX_MIP_LEVELS; mipLevel++) {
    uint32_t mipLimit = std::max(1u, tableLimit >> mipLevel);

    // Sample Level
    for (uint32_t sampleIndex = 0; sampleIndex < dsp_wt::TABLE_SIZE;
         sampleIndex++) {
      float sample = 0.0f;

      // Harminic Level (only odd)
      for (uint32_t harmonic = 1; harmonic <= mipLimit; harmonic += 2) {
        // Convert odd increments into counter (1,3,5,7...) -> (0,1,2,3...)
        float sign = ((((harmonic - 1) / 2) & 1) == 0) ? 1.0f : -1.0f;

        sample += sign * sineAt(harmonic, sampleIndex) /
                  (static_cast<float>(harmonic) * static_cast<float>(harmonic));
      }

      frame->mips[mipLevel][sampleIndex] =
          sample * (8.0f * (dsp::math::INV_PI_F * dsp::math::INV_PI_F));
    }
  }

  registerBank(BankID::Triangle, bank);
  return bank;
}

WavetableBank *createSineToSawBank() {
  const uint32_t NUM_FRAMES = 8;
  const uint32_t tableLimit = dsp_wt::TABLE_SIZE / 2;

  WavetableBank *bank = dsp_wt::createWavetableBank(NUM_FRAMES, "sine_to_saw");

  // Frame Level
  for (uint32_t frameIndex = 0; frameIndex < NUM_FRAMES; frameIndex++) {
    WavetableFrame *frame = &bank->frames[frameIndex];

    // Increase morph (sine -> saw) per frame (0 -> 1)
    // Frame 0 = no morph (0) and pure sine
    // Frame 7 = full morph (1) and pure saw
    float morphAmount =
        static_cast<float>(frameIndex) / static_cast<float>(NUM_FRAMES - 1);

    uint32_t frameHarmonics = std::max(
        1u, static_cast<uint32_t>(1.0f + morphAmount * (tableLimit - 1)));

    // Mip Level
    for (uint32_t mipLevel = 0; mipLevel < dsp_wt::MAX_MIP_LEVELS; mipLevel++) {
      uint32_t mipHarmonics = std::max(1u, tableLimit >> mipLevel);
      uint32_t mipLimit = std::min(frameHarmonics, mipHarmonics);

      // Sample Level
      for (uint32_t sampleIndex = 0; sampleIndex < dsp_wt::TABLE_SIZE;
           sampleIndex++) {
        float sample = 0.0f;

        // Harmonic Level
        for (uint32_t harmonic = 1; harmonic <= mipLimit; harmonic++) {
          float sign = ((harmonic & 1) == 1) ? 1.0f : -1.0f;

          sample += sign * sineAt(harmonic, sampleIndex) /
                    static_cast<float>(harmonic);
        }

        frame->mips[mipLevel][sampleIndex] =
            sample * (2.0f * dsp::math::INV_PI_F);
      }
    }
  }

  registerBank(BankID::SineToSaw, bank);
  return bank;
}

} // namespace

// ================================
// Bank registry
// ================================
static constexpr uint8_t MAX_REGISTRY_BANKS = 32;
static WavetableBank *s_registry[MAX_REGISTRY_BANKS] = {};
static uint8_t s_registryCount = 0;

void registerBank(BankID id, WavetableBank *bank) {
  if (!bank) {
    printf("registerBank: null bank\n");
    return;
  }
  if (s_registryCount >= MAX_REGISTRY_BANKS) {
    printf("registerBank: registry full\n");
    return;
  }
  s_registry[id] = bank;
  s_registryCount++;
}

// TODO(nico-nunez): deregiter is needed once wav importing is implemented

WavetableBank *getBankByID(BankID id) { return s_registry[id]; }

WavetableBank *getBankByName(const char *name) {
  for (int i = 0; i < s_registryCount; i++) {
    if (std::strcmp(s_registry[i]->name, name) == 0)
      return s_registry[i];
  }
  return nullptr;
}

// ================================
// Bank Initialization
// ================================
void initFactoryBanks() {
  createSineBank();
  createSawBank();
  createSquareBank();
  createTriangleBank();
  createSineToSawBank();
}
} // namespace synth::wavetable::banks
