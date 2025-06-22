//
// Created by Hector van der Aa on 6/14/25.
//

#ifndef DSFHEADERS_H
#define DSFHEADERS_H

#include "HeaderTags.h"
#include <cstdint>

namespace sk::headers::DSF {
struct DSDHeader {
    headers::Tag ChunkID = {{'D', 'S', 'D', ' '}};
    std::uint64_t ChunkSize = 28;
    std::uint64_t FileSize = 0;
    std::uint64_t MetaPtr = 0;
    void read(std::ifstream &file);
    void write(std::ofstream &file) const;
};

struct FMTHeader {
    headers::Tag ChunkID = {{'f', 'm', 't', ' '}};
    std::uint64_t ChunkSize = 52;
    std::uint32_t Format = 1;
    std::uint32_t FormatID = 0;
    std::uint32_t ChanType = 2;
    std::uint32_t ChanNum = 2;
    std::uint32_t SampleRate = 0;
    std::uint32_t BitsPerSample = 1;
    std::uint64_t NumSamples = 0;
    std::uint32_t BlockSize = 4096;
    std::uint32_t Reserved = 0;
    void read(std::ifstream &file);
    void write(std::ofstream &file) const;
};
} // namespace sk::headers::DSF

#endif // DSFHEADERS_H
