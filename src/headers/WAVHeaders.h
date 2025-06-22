//
// Created by Hector van der Aa on 6/13/25.
//

#ifndef WAVHEADERS_H
#define WAVHEADERS_H

#include "../lib/EndianHelpers.h"
#include "HeaderTags.h"
#include <fstream>
#include <iostream>

namespace sk::headers::WAV {
struct RIFFHeader {
    Tag ChunkID{{'R', 'I', 'F', 'F'}};
    std::uint32_t ChunkSize{0};
    Tag Format{{'W', 'A', 'V', 'E'}};

    void read(std::ifstream &file);
    void write(std::ofstream &file) const;
};
std::ostream &operator<<(std::ostream &os, const RIFFHeader &input);

struct FMTHeader {
    Tag Subchunk1ID{{'f', 'm', 't', ' '}};
    std::uint32_t Subchunk1Size{16};
    std::uint16_t AudioFormat{1};
    std::uint16_t NumChannels{0};
    std::uint32_t SampleRate{0};
    std::uint32_t ByteRate{0};
    std::uint16_t BlockAlign{0};
    std::uint16_t BitsPerSample{0};
    void read(std::ifstream &file);
    void write(std::ofstream &file) const;
};
std::ostream &operator<<(std::ostream &os, const FMTHeader &input);

struct FACTHeader {
    Tag ChunkID{{'f', 'a', 'c', 't'}};
    std::uint32_t ChunkSize{4};
    std::uint32_t NumSamples{0};
    void read(std::ifstream &file);
    void write(std::ofstream &file) const;
};

struct WAVDataHeader {
    Tag Subchunk2ID{{'d', 'a', 't', 'a'}};
    std::uint32_t Subchunk2Size{0};
    void read(std::ifstream &file);
    void write(std::ofstream &file) const;
};
std::ostream &operator<<(std::ostream &os, const WAVDataHeader &input);

struct WAVHeader {
    RIFFHeader riff;
    FMTHeader fmt;
    FACTHeader fact;
    WAVDataHeader data;
    void read(std::ifstream &file);
    void write(std::ofstream &file) const;
    void update(std::uint16_t bitDepth, std::uint32_t sampleRate,
                std::uint16_t numChannels, std::uint32_t numFrames,
                bool isFloat);
};
} // namespace sk::headers::WAV

#endif // WAVHEADERS_H
