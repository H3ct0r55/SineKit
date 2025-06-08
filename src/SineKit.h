#ifndef SINEKIT_LIBRARY_H
#define SINEKIT_LIBRARY_H


#include <stdlib.h>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <vector>

namespace sk {
    struct Tag {
        char v[4];
    };
    std::ostream& operator<<(std::ostream& os, const Tag& input);

    struct RIFFHeader {
        Tag ChunkID;
        uint32_t ChunkSize;
        Tag Format;

        void readRIFFHeader(std::ifstream& file);
        void writeRIFFHeader(std::ofstream& file);
    };
    std::ostream& operator<<(std::ostream& os, const RIFFHeader& input);

    struct FMTHeader {
        Tag Subchunk1ID;
        uint32_t Subchunk1Size;
        uint16_t AudioFormat;
        uint16_t NumChannels;
        uint32_t SampleRate;
        uint32_t ByteRate;
        uint16_t BlockAlign;
        uint16_t BitsPerSample;
        void readFMTHeader(std::ifstream& file);
        void writeFMTHeader(std::ofstream& file);
    };
    std::ostream& operator<<(std::ostream& os, const FMTHeader& input);

    struct WAVDataHeader {
        Tag Subchunk2ID;
        uint32_t Subchunk2Size;
        void readWAVDataHeader(std::ifstream& file);
        void writeWAVDataHeader(std::ofstream& file);
    };
    std::ostream& operator<<(std::ostream& os, const WAVDataHeader& input);

    struct WAVHeader {
        RIFFHeader m_RIFFHeader;
        FMTHeader m_FMTHeader;
        WAVDataHeader m_WAVDataHeader;
        void readWAVHeader(std::ifstream& file);
        void writeWAVHeader(std::ofstream& file);
        void updateWAVHeader(unsigned int bitDepth, unsigned int sampleRate, unsigned int numChannels, unsigned int numFrames);
    };

    enum AudioType {
        T_UDEF,
        T_PCM,
        T_DSD
    };

    enum BitType {
        T_16I,
        T_24I,
        T_32I,
        T_32F,
        T_64F
    };

    template<typename SampleType>
    struct AudioBuffer {
        size_t numFrames;
        size_t numChannels;

        std::vector<std::vector<SampleType>> channels;

        void resize(size_t numCh, size_t frames) {
            numFrames = frames;
            numChannels = numCh;

            channels.resize(numChannels);
            for (auto &ch : channels)
                ch.resize(numFrames);

        }

        void clear() {
            for (auto &ch : channels)
                ch.clear();
            channels.clear();
            numFrames = 0;
            numChannels = 0;
        }

        SampleType& sample(size_t ch, size_t frame) {
            return channels[ch][frame];
        }
    };

    class SineKit {
        WAVHeader m_WAVHeader;
        AudioType audioType;
        BitType bitType;
        unsigned int pcmBitDepth;
        unsigned int pcmSampleRate;
        unsigned int dsdOversamplingRate;
        unsigned int numChannels;
        unsigned int numFrames;
        AudioBuffer<int16_t> pcmBuffer16;
        AudioBuffer<int32_t> pcmBuffer24;
        AudioBuffer<int32_t> pcmBuffer32;
        AudioBuffer<float> pcmBuffer32f;
        AudioBuffer<double> pcmBuffer64f;

    public:
        SineKit();
        SineKit(AudioType type, unsigned int number_channels);

        void loadFile(std::filesystem::path input_path);
        void writeFile(std::filesystem::path output_path);
        void convertToFormat(BitType bitType);
        void updateHeaders();
    };

}

#endif //SINEKIT_LIBRARY_H
