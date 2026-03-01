#include "WavWriter.h"
#include <cassert>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace WavWriter {

// Create WAV file
// NOTE: std::ofstream doesn't support std::string_view
std::ofstream createWavFile(const std::string& filename) {
  std::ofstream wavFile(filename, std::ios::binary);
  return wavFile;
}

// WAV file format uses "chunks" - blocks of data with a 4-byte ID and size
// The main structure is: RIFF chunk -> fmt chunk -> data chunk

void writeString(std::ofstream& file, const char* str, int32_t length) {
  // Write a fixed-length string to the file
  file.write(str, length);
}

void writeInt32(std::ofstream& file, int32_t value) {
  assert(value <= INT_MAX);

  // WAV format uses little-endian (least significant byte first)
  file.write(reinterpret_cast<const char*>(&value), 4);
}

void writeInt16(std::ofstream& file, int16_t value) {
  file.write(reinterpret_cast<const char*>(&value), 2);
}

void writeWavMetadata(std::ofstream& file, int32_t numSamples, int32_t sampleRate) {
  // WAV files have a specific structure. Let's build it piece by piece:

  // --- RIFF HEADER ---
  // This identifies the file as a RIFF file (Resource Interchange File
  // Format)
  writeString(file, "RIFF", 4);

  // File size minus 8 bytes (for "RIFF" and this size field itself)
  int32_t fileSize = 36 + (numSamples * 2); // 36 = header size, 2 = bytes per sample
  writeInt32(file, fileSize);

  // WAVE format identifier
  writeString(file, "WAVE", 4);

  // --- FORMAT CHUNK ---
  // Describes the audio format
  writeString(file, "fmt ", 4); // Note the space after "fmt"

  // Format chunk size (16 bytes for PCM)
  writeInt32(file, 16);

  // Audio format (1 = PCM, uncompressed)
  writeInt16(file, 1);

  // Number of channels (1 = mono, 2 = stereo)
  writeInt16(file, 1);

  // Sample rate (samples per second)
  writeInt32(file, sampleRate);

  // Byte rate (sample rate * channels * bytes per sample)
  writeInt32(file, sampleRate * 1 * 2);

  // Block align (channels * bytes per sample)
  writeInt16(file, 1 * 2);

  // Bits per sample
  writeInt16(file, 16);
}

// Write WAV file to disk
void writeWavFile(const std::string& filename,
                  std::vector<float>& audioBuffer,
                  int32_t sampleRate) {
  if (audioBuffer.empty())
    throw std::invalid_argument("Audio buffer is empty");

  if (audioBuffer.size() >= INT_MAX)
    throw std::out_of_range("Audio buffer is too large. Max:" + std::to_string(INT_MAX));

  const int32_t TOTAL_SAMPLES{static_cast<int32_t>(audioBuffer.size())};

  std::vector<int16_t> pcmData{};
  pcmData.reserve(audioBuffer.size());

  for (float sampleValue : audioBuffer) {
    // Limit to valid range (-1.0 to 1.0)
    if (sampleValue > 1.0f)
      sampleValue = 1.0f;
    if (sampleValue < -1.0f)
      sampleValue = -1.0f;

    // Convert to int16_t: -32768 to 32767
    pcmData.push_back(static_cast<int16_t>(sampleValue * 32767.0f));
  }

  std::ofstream wavFile{createWavFile(filename)};

  if (!wavFile) {
    std::cerr << "Error: Could not create output.wav\n";
  }

  std::cout << "Writing WAV file...\n";

  writeWavMetadata(wavFile, TOTAL_SAMPLES, sampleRate);

  // --- DATA CHUNK ---
  // Contains the actual audio samples
  writeString(wavFile, "data", 4);

  // Data chunk size (number of samples * bytes per sample)
  writeInt32(wavFile, TOTAL_SAMPLES * 2);

  // Write all the audio samples
  for (const auto& sample : pcmData) {
    writeInt16(wavFile, sample);
  }

  wavFile.close();

  std::cout << "Success! Created output.wav\n";
  std::cout << "Play it with any audio player to hear your sine wave.\n";
}

} // namespace WavWriter
