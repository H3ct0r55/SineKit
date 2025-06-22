#pragma once
#ifndef SINEKIT_LIBRARY_H
#define SINEKIT_LIBRARY_H

#include <fstream>
#include <filesystem>
#include <iostream>
#include <vector>
#include <cassert>
#include "lib/EndianHelpers.h"
#include <bit>
#include <type_traits>
#include <cstring>
#include <string>
#include <float.h>
#include "lib/CustomFloat.h"
#include <cstdint>
#include "headers/WAVHeaders.h"
#include "headers/AIFFHeaders.h"
#include "headers/HeaderTags.h"
#include <functional>
#include <cmath>
#include <boost/math/special_functions/bessel.hpp>

#ifdef USE_THREADING
#include <pthread.h>
#endif

namespace sk {


    enum class AudioType {Undefined, PCM, DSD};

    enum class BitType : std::uint16_t { Undefined = 0, I8 = 8, I16 = 16, I24 = 24, F32 = 32, F64 = 64 };

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

    enum class WindowType : std::uint32_t {
        RECTANGULAR = 0,
        HAMMING = 1,
        HANNING = 2,
        BLACKMAN = 3,
        KAISER = 4,
    };

    enum class InterpolationOrder : std::uint8_t { Default = 3, Linear = 1, Quadratic = 2, Cubic = 3, Quartic = 4, Sinc = 5};
    enum class DitherAmount {None, Low, Medium, High};

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
        void clear();

    };


    class SineKit {
    private:
        headers::WAV::WAVHeader               WAVHeader_;
        headers::AIFF::AIFFHeader              AIFFHeader_;
        AudioType               AudioType_      {AudioType::Undefined};
        BitType                 BitType_        {BitType::Undefined};
        SampleRate              SampleRate_     {SampleRate::Undefined};
        std::uint16_t           NumChannels_    {0};
        std::uint32_t           NumFrames_      {0};
        AudioBuffer<std::uint8_t>    Buffer8I_;
        AudioBuffer<std::int16_t>    Buffer16I_;
        AudioBuffer<std::int32_t>    Buffer24I_;
        AudioBuffer<std::int32_t>    Buffer32I_;
        AudioBuffer<float>      Buffer32F_;
        AudioBuffer<double>     Buffer64F_;

        template<typename T>
        static void readInterleaved (std::ifstream&, AudioBuffer<T>&, std::size_t frames, std::size_t ch, sk::endian::Endian fileEndian, sk::BitType bitType);

        template<typename T>
        static void writeInterleaved(std::ofstream&, const AudioBuffer<T>&, std::size_t frames, std::size_t ch, sk::endian::Endian fileEndian, sk::BitType bitType);
        void clearBut(sk::BitType bitType);
        void updateHeaders();

        template<typename T>
        void upsample(std::uint8_t scale, std::uint8_t interpolation, sk::AudioBuffer<T> &buffer, sk::BitType bitType, std::uint64_t windowSize, sk::WindowType windowType);

        template<typename T>
        void upsampleNonInt(std::int8_t interpolation, std::int64_t base, std::int64_t target, sk::AudioBuffer<T> &buffer, sk::BitType bitType, std::uint64_t windowSize, sk::WindowType windowType);

    public:

        void loadFile(const std::filesystem::path& input_path);
        void writeFile(const std::filesystem::path& output_path) const;
        void toBitDepth(BitType bitType);
        void toSampleRate(SampleRate sampleRate);
    };

}

#endif //SINEKIT_LIBRARY_H
