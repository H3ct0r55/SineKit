#ifndef SINEKIT_LIBRARY_H
#define SINEKIT_LIBRARY_H

#include <fstream>
#include <filesystem>
#include <iostream>

namespace sk {
    class RIFFHeader {
        char* ChunkID;
        uint32_t ChunkSize;
        char* Format;
    public:
        RIFFHeader();
        ~RIFFHeader();
        void readRIFFHeader(std::filesystem::path input_path);
        friend std::ostream& operator<<(std::ostream& os, const RIFFHeader& input);
    };

    class FMTHeader {
        char* Subchunk1ID;
        uint32_t Subchunk1Size;
        uint16_t AudioFormat;
        uint16_t NumChannels;
        uint32_t SampleRate;
        uint32_t ByteRate;
        uint16_t BlockAlign;
        uint16_t BitsPerSample;
    public:
        FMTHeader();
        ~FMTHeader();
        void readFMTHeader(std::filesystem::path input_path);
        uint32_t getSubchunk1Size();
        friend std::ostream& operator<<(std::ostream& os, const FMTHeader& input);
    };

    class WAVDataHeader {
        char* Subchunk2ID;
        uint32_t Subchunk2Size;
    public:
        WAVDataHeader();
        ~WAVDataHeader();
        void readWAVDataHeader(std::filesystem::path input_path, uint32_t Subchunk1Size);
        friend std::ostream& operator<<(std::ostream& os, const WAVDataHeader& input);
    };

    class WAVHeader {
        RIFFHeader m_RIFFHeader;
        FMTHeader m_FMTHeader;
        WAVDataHeader m_WAVDataHeader;
    public:
        WAVHeader();
        ~WAVHeader();
        void readWAVHeader(std::filesystem::path input_path);
    };

    enum AudioType {
        T_UDEF,
        T_PCM,
        T_DSD
    };

    class SineKit {
        WAVHeader WAVHeader;
        AudioType audioType;
        unsigned int pcmBitDepth;
        unsigned int pcmSampleRate;
        unsigned int dsdOversamplingRate;
        unsigned int numChannels;
        //auto** audioData = nullptr;

    public:
        SineKit();
        SineKit(AudioType type, unsigned int number_channels);

        void loadFile(std::filesystem::path input_path);
    };
}

#endif //SINEKIT_LIBRARY_H
