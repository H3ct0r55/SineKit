#pragma once
#ifndef SINEKIT_LIBRARY_H
#define SINEKIT_LIBRARY_H

#include <fstream>
#include <filesystem>
#include <iostream>
#include <vector>
#include <cassert>
#include "EndianHelpers.h"
#include <bit>
#include <type_traits>
#include <cstring>
#include <string>
#include <float.h>
#include "lib/CustomFloat.h"
#include <cstdint>
#include "headers/WAVHeaders.h"


namespace sk {
    struct Tag {char v[4];};
    std::ostream& operator<<(std::ostream& os, const Tag& input);

    struct RIFFHeader {
        Tag             ChunkID         {{'R','I','F','F'}};
        std::uint32_t   ChunkSize       {0};
        Tag             Format          {{'W','A','V','E'}};

        void read(std::ifstream& file);
        void write(std::ofstream& file) const;
    };
    std::ostream& operator<<(std::ostream& os, const RIFFHeader& input);

    struct FMTHeader {
        Tag             Subchunk1ID     {{'f','m','t',' '}};
        std::uint32_t   Subchunk1Size   {16};
        std::uint16_t   AudioFormat     {1};
        std::uint16_t   NumChannels     {0};
        std::uint32_t   SampleRate      {0};
        std::uint32_t   ByteRate        {0};
        std::uint16_t   BlockAlign      {0};
        std::uint16_t   BitsPerSample   {0};
        void read(std::ifstream& file);
        void write(std::ofstream& file) const;
    };
    std::ostream& operator<<(std::ostream& os, const FMTHeader& input);

    struct FACTHeader {
        Tag             ChunkID         {{'f','a','c','t'}};
        std::uint32_t   ChunkSize       {4};
        std::uint32_t   NumSamples       {0};
        void read(std::ifstream& file);
        void write(std::ofstream& file) const;
    };

    struct WAVDataHeader {
        Tag             Subchunk2ID     {{'d','a','t','a'}};
        std::uint32_t   Subchunk2Size   {0};
        void read(std::ifstream& file);
        void write(std::ofstream& file) const;
    };
    std::ostream& operator<<(std::ostream& os, const WAVDataHeader& input);

    struct WAVHeader {
        RIFFHeader      riff;
        FMTHeader       fmt;
        FACTHeader       fact;
        WAVDataHeader   data;
        void read(std::ifstream& file);
        void write(std::ofstream& file) const;
        void update(std::uint16_t bitDepth, std::uint32_t sampleRate, std::uint16_t numChannels, std::uint32_t numFrames, bool isFloat);
    };

    struct FORMHeader {
        Tag             ChunkID     {{'F','O','R','M'}};
        std::uint32_t   ChunkSize   {0};
        Tag             FormType    {{'A','I','F','F'}};
        void read(std::ifstream& file);
        void write(std::ofstream& file) const;
    };
    std::ostream& operator<<(std::ostream& os, const FORMHeader& input);

    struct COMMHeader {
        Tag             ChunkID     {{'C','O','M','M'}};
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
        Tag             ChunkID     {{'S','S','N','D'}};
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

    enum class AudioType {Undefined, PCM, DSD};

    enum class BitType : std::uint16_t { Undefined = 0, I16 = 16, I24 = 24, F32 = 32, F64 = 64 };

    enum class SampleRate : std::uint32_t {
        Undefined   =   0,
        P22K05      =   22050,
        P32K        =   32000,
        P44K1       =   44100,
        P48K        =   48000,
        P88K2       =   88200,
        P96K        =   96000,
        P176K4      =   176400,
        P192K       =   192000,
        DSD64       =   2822400,
        DSD128      =   5644800,
        DSD256      =   11289600,
        DSD512      =   22579200
    };

    template<typename T>
    struct AudioBuffer {
        std::vector<std::vector<T>> channels;

        void resize(std::size_t numChannels, size_t numFrames) {
            channels.assign(numChannels, std::vector<T>(numFrames));
        }
        [[nodiscard]] std::size_t numChannels() const { return channels.size();};
        [[nodiscard]] std::size_t numFrames() const { return channels.is_empty() ? 0 : channels.front.size();};

        T&       operator()(std::size_t c, std::size_t f)       { return channels[c][f]; }
        const T& operator()(std::size_t c, std::size_t f) const { return channels[c][f]; }
    };

    class SineKit {
    private:
        WAVHeader               WAVHeader_;
        AIFFHeader              AIFFHeader_;
        AudioType               AudioType_      {AudioType::Undefined};
        BitType                 BitType_        {BitType::Undefined};
        SampleRate              SampleRate_     {SampleRate::Undefined};
        std::uint16_t           NumChannels_    {0};
        std::uint32_t           NumFrames_      {0};
        AudioBuffer<int16_t>    Buffer16_;
        AudioBuffer<int32_t>    Buffer24_;
        AudioBuffer<float>      Buffer32f_;
        AudioBuffer<double>     Buffer64f_;

        template<typename T>
        static void readInterleaved (std::ifstream&, AudioBuffer<T>&, std::size_t frames, std::size_t ch, sk::endian::Endian fileEndian);

        template<typename T>
        static void writeInterleaved(std::ofstream&, const AudioBuffer<T>&, std::size_t frames, std::size_t ch, sk::endian::Endian fileEndian);

    public:

        void loadFile(const std::filesystem::path& input_path);
        void writeFile(const std::filesystem::path& output_path) const;
        void updateHeaders();
    };

}

#endif //SINEKIT_LIBRARY_H
