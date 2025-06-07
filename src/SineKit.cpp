#include "SineKit.h"

#include <iostream>

sk::SineKit::SineKit() : audioType(T_UDEF), pcmBitDepth(0),
                         pcmSampleRate(0), dsdOversamplingRate(0),
                         numChannels(0){}

sk::SineKit::SineKit(AudioType type, unsigned int number_channels) :
                        audioType(type), pcmBitDepth(0),
                        pcmSampleRate(0), dsdOversamplingRate(0),
                        numChannels(number_channels){}

void sk::SineKit::loadFile(std::filesystem::path input_path) {
    WAVHeader.readWAVHeader(input_path);
}

sk::WAVHeader::WAVHeader() : m_RIFFHeader(RIFFHeader()), m_FMTHeader(FMTHeader()), m_WAVDataHeader(WAVDataHeader()) {}
sk::WAVHeader::~WAVHeader() {
}

void sk::WAVHeader::readWAVHeader(std::filesystem::path input_path) {
    m_RIFFHeader.readRIFFHeader(input_path);
    std::cout << m_RIFFHeader << std::endl;
    m_FMTHeader.readFMTHeader(input_path);
    std::cout << m_FMTHeader << std::endl;
    m_WAVDataHeader.readWAVDataHeader(input_path, m_FMTHeader.getSubchunk1Size());
    std::cout << m_WAVDataHeader << std::endl;
}







sk::RIFFHeader::RIFFHeader() : ChunkID(nullptr), ChunkSize(0), Format(nullptr)  {}
sk::RIFFHeader::~RIFFHeader() {
    delete[] ChunkID;
    delete[] Format;
}

void sk::RIFFHeader::readRIFFHeader(std::filesystem::path input_path) {
    std::fstream file(input_path, std::ios::in | std::ios::binary);

    if (!file.is_open()) {
        std::cerr << "Could not open file " << input_path << std::endl;
        return;
    }

    ChunkID = new char[5];
    file.read(ChunkID, 4);
    ChunkID[4] = '\0';

    file.read(reinterpret_cast<char*>(&ChunkSize), 4);

    Format = new char[5];
    file.read(Format, 4);
    Format[4] = '\0';
    file.close();
}

uint32_t sk::FMTHeader::getSubchunk1Size() {
    return Subchunk1Size;
}


std::ostream& sk::operator<<(std::ostream &os, const RIFFHeader &input) {
    os << "Chunk ID: " << input.ChunkID << std::endl;
    os << "Chunk size: " << input.ChunkSize << std::endl;
    os << "Format: " << input.Format << std::endl;
    return os;
}







sk::FMTHeader::FMTHeader() : Subchunk1ID(nullptr), Subchunk1Size(0) {}
sk::FMTHeader::~FMTHeader() {
    delete[] Subchunk1ID;
}

void sk::FMTHeader::readFMTHeader(std::filesystem::path input_path) {
    std::fstream file(input_path, std::ios::in | std::ios::binary);

    if (!file.is_open()) {
        std::cerr << "Could not open file " << input_path << std::endl;
        return;
    }

    file.ignore(12);

    Subchunk1ID = new char[5];
    file.read(Subchunk1ID, 4);
    Subchunk1ID[4] = '\0';

    file.read(reinterpret_cast<char*>(&Subchunk1Size), sizeof(Subchunk1Size));

    int OffsetCounter = Subchunk1Size;

    file.read(reinterpret_cast<char*>(&AudioFormat), sizeof(AudioFormat));
    OffsetCounter -= sizeof(AudioFormat);

    file.read(reinterpret_cast<char*>(&NumChannels), sizeof(NumChannels));
    OffsetCounter -= sizeof(NumChannels);

    file.read(reinterpret_cast<char*>(&SampleRate), sizeof(SampleRate));
    OffsetCounter -= sizeof(SampleRate);

    file.read(reinterpret_cast<char*>(&ByteRate), sizeof(ByteRate));
    OffsetCounter -= sizeof(ByteRate);

    file.read(reinterpret_cast<char*>(&BlockAlign), sizeof(BlockAlign));
    OffsetCounter -= sizeof(BlockAlign);

    file.read(reinterpret_cast<char*>(&BitsPerSample), sizeof(BitsPerSample));
    OffsetCounter -= sizeof(BitsPerSample);

    file.ignore(OffsetCounter);

    file.close();
}

std::ostream &sk::operator<<(std::ostream &os, const FMTHeader &input) {
    os << "Subchunk1ID: " << input.Subchunk1ID << std::endl;
    os << "Subchunk1Size: " << input.Subchunk1Size << std::endl;
    os << "AudioFormat: " << input.AudioFormat << std::endl;
    os << "NumChannels: " << input.NumChannels << std::endl;
    os << "SampleRate: " << input.SampleRate << std::endl;
    os << "ByteRate: " << input.ByteRate << std::endl;
    os << "BlockAlign: " << input.BlockAlign << std::endl;
    os << "BitsPerSample: " << input.BitsPerSample << std::endl;
    return os;
}






sk::WAVDataHeader::WAVDataHeader() : Subchunk2ID(nullptr), Subchunk2Size(0) {}
sk::WAVDataHeader::~WAVDataHeader() {
    delete[] Subchunk2ID;
}

void sk::WAVDataHeader::readWAVDataHeader(std::filesystem::path input_path, uint32_t Subchunk1Size) {
    std::fstream file(input_path, std::ios::in | std::ios::binary);

    if (!file.is_open()) {
        std::cerr << "Could not open file " << input_path << std::endl;
        return;
    }

    file.ignore(12 + 8 + Subchunk1Size);

    Subchunk2ID = new char[5];
    file.read(Subchunk2ID, 4);
    Subchunk2ID[4] = '\0';

    file.read(reinterpret_cast<char*>(&Subchunk2Size), sizeof(Subchunk2Size));

    file.close();
}

std::ostream &sk::operator<<(std::ostream &os, const WAVDataHeader &input) {
    os << "Subchunk2ID: " << input.Subchunk2ID << std::endl;
    os << "Subchunk2Size: " << input.Subchunk2Size << std::endl;
    return os;
}







