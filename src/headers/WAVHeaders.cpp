//
// Created by Hector van der Aa on 6/13/25.
//

#include "WAVHeaders.h"

// ─── RIFF helpers ─────────────────────────────────────────────────────────
void sk::headers::RIFFHeader::read(std::ifstream& file) {
    file.read(ChunkID.v, sizeof(ChunkID.v));
    ChunkSize = sk::endian::read_le<decltype(ChunkSize)>(file);
    file.read(Format.v, sizeof(Format.v));
    if(!file) throw std::runtime_error("RIFF header read failed");
}

void sk::headers::RIFFHeader::write(std::ofstream& file) const {
    file.write(ChunkID.v, sizeof(ChunkID.v));
    sk::endian::write_le<decltype(ChunkSize)>(file, ChunkSize);
    file.write(Format.v, sizeof(Format.v));
}


// ─── FMT helpers ──────────────────────────────────────────────────────────
void sk::headers::FMTHeader::read(std::ifstream& file) {
    file.read(Subchunk1ID.v, sizeof(Subchunk1ID.v));

    Subchunk1Size = sk::endian::read_le<decltype(Subchunk1Size)>(file);
    AudioFormat = sk::endian::read_le<decltype(AudioFormat)>(file);
    NumChannels = sk::endian::read_le<decltype(NumChannels)>(file);
    SampleRate = sk::endian::read_le<decltype(SampleRate)>(file);
    ByteRate = sk::endian::read_le<decltype(ByteRate)>(file);
    BlockAlign = sk::endian::read_le<decltype(BlockAlign)>(file);
    BitsPerSample = sk::endian::read_le<decltype(BitsPerSample)>(file);

    if (!file) throw std::runtime_error("FMTHeader header read failed");
}

void sk::headers::FMTHeader::write(std::ofstream& file) const {
    file.write(Subchunk1ID.v, sizeof(Subchunk1ID.v));
    sk::endian::write_le<decltype(Subchunk1Size)>(file, Subchunk1Size);
    sk::endian::write_le<decltype(AudioFormat)>(file, AudioFormat);
    sk::endian::write_le<decltype(NumChannels)>(file, NumChannels);
    sk::endian::write_le<decltype(SampleRate)>(file, SampleRate);
    sk::endian::write_le<decltype(ByteRate)>(file, ByteRate);
    sk::endian::write_le<decltype(BlockAlign)>(file, BlockAlign);
    sk::endian::write_le<decltype(BitsPerSample)>(file, BitsPerSample);
}


void sk::headers::FACTHeader::read(std::ifstream& file) {
    file.read(ChunkID.v, sizeof(ChunkID.v));
    ChunkSize = sk::endian::read_le<decltype(ChunkSize)>(file);
    NumSamples = sk::endian::read_le<decltype(NumSamples)>(file);
    if (!file) throw std::runtime_error("FACTHeader header read failed");
}

void sk::headers::FACTHeader::write(std::ofstream& file) const {
    file.write(ChunkID.v, sizeof(ChunkID.v));
    sk::endian::write_le<decltype(ChunkSize)>(file, ChunkSize);
    sk::endian::write_le<decltype(NumSamples)>(file, NumSamples);
}


// ─── WAV DATA helpers ─────────────────────────────────────────────────────
void sk::headers::WAVDataHeader::read(std::ifstream& file) {
    file.read(Subchunk2ID.v, sizeof(Subchunk2ID.v));
    Subchunk2Size = sk::endian::read_le<decltype(Subchunk2Size)>(file);
    if (!file) throw std::runtime_error("WAV DATA header read failed");
}

void sk::headers::WAVDataHeader::write(std::ofstream& file) const {
    file.write(Subchunk2ID.v, sizeof(Subchunk2ID.v));
    sk::endian::write_le<decltype(Subchunk2Size)>(file, Subchunk2Size);
}

int verifyWAVChunk(std::ifstream& file) {
    char readData[4];
    file.read(readData, sizeof(readData));
    if (!file) return false;
    file.seekg(-4, std::ios::cur);
    if (std::strncmp(readData, "RIFF", 4) == 0) return 1;
    if (std::strncmp(readData, "fmt ", 4) == 0) return 2;
    if (std::strncmp(readData, "fact", 4) == 0) return 3;
    if (std::strncmp(readData, "data", 4) == 0) return 4;
    return 0;
}
// ─── WAV HEADER helpers ───────────────────────────────────────────────────
void sk::headers::WAVHeader::read(std::ifstream& file) {
    bool foundRIFF = false;
    bool foundFMT = false;
    bool foundFact = false;
    bool foundData = false;
    while (!foundRIFF || !foundFMT || !foundFact || !foundData) {
        switch (verifyWAVChunk(file)) {
            case 1 : {
                if (!foundRIFF) {
                    foundRIFF = true;
                    riff.read(file);
                    std::cout << "Found RIFF header" << std::endl;
                    break;
                }
                throw std::runtime_error("Multiple RIFF headers found, invalid file");
            }
            case 2 : {
                if (!foundFMT) {
                    foundFMT = true;
                    fmt.read(file);
                    std::cout << "Found FMT header" << std::endl;
                    break;
                }
                throw std::runtime_error("Multiple FMT headers found, invalid file");
            }
            case 3 : {
                if (!foundFact) {
                    foundFact = true;
                    fact.read(file);
                    std::cout << "Found Fact header" << std::endl;
                    break;
                }
                throw std::runtime_error("Multiple FACT headers found, invalid file");
            }
            case 4 : {
                if (!foundData) {
                    foundData = true;
                    data.read(file);
                    std::cout << "Found data header" << std::endl;
                    break;
                }
                throw std::runtime_error("Multiple DATA headers found, invalid file");
            }
            default: {
                file.seekg(2, std::ios::cur);
                break;
            }
        }

        if (foundRIFF && foundFMT && foundData && fmt.AudioFormat != 3) {
            foundFact = true;
            std::cout << "Skipping Fact Header" << std::endl;
        }
    }
}


void sk::headers::WAVHeader::write(std::ofstream& file) const {
    riff.write(file);
    fmt.write(file);
    if (fmt.AudioFormat == 3) {
        fact.write(file);
    }
    data.write(file);
}

void sk::headers::WAVHeader::update(std::uint16_t bitDepth, std::uint32_t sampleRate, std::uint16_t numChannels, std::uint32_t numFrames, bool isFloat) {
    fmt.AudioFormat = isFloat ? 3 : 1;
    std::cout << fmt.AudioFormat << std::endl;
    fmt.NumChannels = numChannels;
    fmt.SampleRate = sampleRate;
    fmt.BlockAlign = numChannels * bitDepth / 8;
    fmt.ByteRate = sampleRate * fmt.BlockAlign;
    fmt.BitsPerSample = bitDepth;

    fact.NumSamples = numFrames;

    data.Subchunk2Size = numFrames * fmt.BlockAlign;
    riff.ChunkSize = 4 + (8 + fmt.Subchunk1Size) + (8 + data.Subchunk2Size) + (fmt.AudioFormat == 3 ? 12 : 0);

}
