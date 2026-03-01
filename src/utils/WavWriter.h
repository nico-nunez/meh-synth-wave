#ifndef WAV_WRITER_H
#define WAV_WRITER_H

#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

namespace WavWriter {
// Create WAV file
std::ofstream createWavFile(const std::string& filename = "output.wav");

// Write string to WAV file
void writeString(std::ofstream& file, const char* str, int length);

// Write int 32 to WAV file
void writeInt32(std::ofstream& file, int32_t value);

// Write int 16 to WAV file
void writeInt16(std::ofstream& file, int16_t value);

// Write WAV file metadata
void writeWavMetadata(std::ofstream& file, int32_t numSamples, int32_t sampleRate);

// Write WAVE file to disk
void writeWavFile(const std::string& filename, std::vector<float>& audioBuffer, int32_t sampleRate);
} // namespace WavWriter
#endif
