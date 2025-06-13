//
// Created by Hector van der Aa on 6/13/25.
//

#ifndef AIFFHEADERS_H
#define AIFFHEADERS_H

#include "HeaderTags.h"
#include "../lib/CustomFloat.h"

namespace sk::headers {
    struct FORMHeader {
        headers::Tag             ChunkID     {{'F','O','R','M'}};
        std::uint32_t   ChunkSize   {0};
        headers::Tag             FormType    {{'A','I','F','F'}};
        void read(std::ifstream& file);
        void write(std::ofstream& file) const;
    };
    std::ostream& operator<<(std::ostream& os, const FORMHeader& input);

    struct COMMHeader {
        headers::Tag             ChunkID     {{'C','O','M','M'}};
        std::int32_t    ChunkSize   {0};
        std::int16_t    NumChannels {0};
        std::uint32_t   NumSamples  {0};
        std::int16_t    BitDepth    {0};
        Float80         SampleRate  {0};
        void read(std::ifstream& file);
        void write(std::ofstream& file) const;
    };
    std::ostream& operator<<(std::ostream& os, const COMMHeader& input);

    struct SSNDHeader {
        headers::Tag             ChunkID     {{'S','S','N','D'}};
        std::uint32_t   ChunkSize   {0};
        std::uint32_t   Offset      {0};
        std::uint32_t   BlockSize   {0};
        void read(std::ifstream& file);
        void write(std::ofstream& file) const;
    };

    struct AIFFHeader {
        FORMHeader      form;
        COMMHeader      comm;
        SSNDHeader      ssnd;
        void read(std::ifstream& file);
        void write(std::ofstream& file) const;
        void update(std::uint16_t bitDepth, std::uint32_t sampleRate, std::uint16_t numChannels, std::uint32_t numFrames);
    };
    std::ostream& operator<<(std::ostream& os, const AIFFHeader& input);
}

#endif //AIFFHEADERS_H
