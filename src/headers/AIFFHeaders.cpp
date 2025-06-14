//
// Created by Hector van der Aa on 6/13/25.
//

#include "AIFFHeaders.h"
#include "../lib/EndianHelpers.h"

void sk::headers::FORMHeader::read(std::ifstream& file) {
    file.read(ChunkID.v, sizeof(ChunkID.v));
    ChunkSize = sk::endian::read_be<decltype(ChunkSize)>(file);
    file.read(FormType.v, sizeof(FormType.v));
}

void sk::headers::FORMHeader::write(std::ofstream& file) const {
    file.write(ChunkID.v, sizeof(ChunkID.v));
    sk::endian::write_be<decltype(ChunkSize)>(file, ChunkSize);
    file.write(FormType.v, sizeof(FormType.v));
}
std::ostream& sk::headers::operator<<(std::ostream& os, const sk::headers::FORMHeader& input) {
    os << "FORM Header" << std::endl;
    os << "Chunk ID: " << input.ChunkID << std::endl;
    os << "Chunk Size: " << input.ChunkSize << std::endl;
    os << "Form Type: " << input.FormType << std::endl;
    return os;
}

void sk::headers::COMMHeader::read(std::ifstream& file) {
    file.read(ChunkID.v, sizeof(ChunkID.v));
    ChunkSize = sk::endian::read_be<decltype(ChunkSize)>(file);
    NumChannels = sk::endian::read_be<decltype(NumChannels)>(file);
    NumSamples = sk::endian::read_be<decltype(NumSamples)>(file);
    BitDepth = sk::endian::read_be<decltype(BitDepth)>(file);
    file.read(reinterpret_cast<char*>(&SampleRate), sizeof(SampleRate));
}

void sk::headers::COMMHeader::write(std::ofstream& file) const {
    file.write(ChunkID.v, sizeof(ChunkID.v));
    sk::endian::write_be<decltype(ChunkSize)>(file, ChunkSize);
    sk::endian::write_be<decltype(NumChannels)>(file, NumChannels);
    sk::endian::write_be<decltype(NumSamples)>(file, NumSamples);
    sk::endian::write_be<decltype(BitDepth)>(file, BitDepth);
    file.write(reinterpret_cast<const char*>(&SampleRate), sizeof(SampleRate));
}
std::ostream &sk::headers::operator<<(std::ostream &os, const headers::COMMHeader &input) {
    os << "COMM Header" << std::endl;
    os << "Chunk ID: " << input.ChunkID << std::endl;
    os << "Chunk Size: " << input.ChunkSize << std::endl;
    os << "Num Channels: " << input.NumChannels << std::endl;
    os << "Num Samples: " << input.NumSamples << std::endl;
    os << "BitDepth: " << input.BitDepth << std::endl;
    os << "SampleRate: " << input.SampleRate << std::endl;
    return os;
}

void sk::headers::SSNDHeader::read(std::ifstream& file) {
    file.read(ChunkID.v, sizeof(ChunkID.v));
    ChunkSize = sk::endian::read_be<decltype(ChunkSize)>(file);
    Offset = sk::endian::read_be<decltype(Offset)>(file);
    BlockSize = sk::endian::read_be<decltype(BlockSize)>(file);
}

void sk::headers::SSNDHeader::write(std::ofstream& file) const {
    file.write(ChunkID.v, sizeof(ChunkID.v));
    sk::endian::write_be<decltype(ChunkSize)>(file, ChunkSize);
    sk::endian::write_be<decltype(Offset)>(file, Offset);
    sk::endian::write_be<decltype(BlockSize)>(file, BlockSize);
}


int verifyAIFFChunk(std::ifstream& file) {
    char readData[4];
    file.read(readData, sizeof(readData));
    if (!file) return false;
    file.seekg(-4, std::ios::cur);
    if (std::strncmp(readData, "FORM", 4) == 0) return 1;
    if (std::strncmp(readData, "COMM", 4) == 0) return 2;
    if (std::strncmp(readData, "SSND", 4) == 0) return 3;
    return 0;
}

void sk::headers::AIFFHeader::read(std::ifstream& file) {
    bool foundFORM = false;
    bool foundCOMM = false;
    bool foundSSND = false;
    while (!foundFORM || !foundCOMM || !foundSSND) {
        switch (verifyAIFFChunk(file)) {
            case 1 : {
                if (!foundFORM) {
                    foundFORM = true;
                    form.read(file);
                    break;
                }
                throw std::runtime_error("Multiple FORM headers found, invalid file");
            }
            case 2 : {
                if (!foundCOMM) {
                    foundCOMM = true;
                    comm.read(file);
                    break;
                }
                throw std::runtime_error("Multiple COMM headers found, invalid file");
            }
            case 3 : {
                if (!foundSSND) {
                    foundSSND = true;
                    ssnd.read(file);
                    break;
                }
                throw std::runtime_error("Multiple SSND headers found, invalid file");
            }
            default: {
                file.seekg(2, std::ios::cur);
            }
        }
    }
}

std::ostream& sk::headers::operator<<(std::ostream& os, const sk::headers::AIFFHeader& aiff) {
    os << "AIFF Header" << std::endl;
    os << aiff.form;
    os << aiff.comm;
    return os;
}

void sk::headers::AIFFHeader::write(std::ofstream& file) const {
    form.write(file);
    comm.write(file);
    ssnd.write(file);
}

void sk::headers::AIFFHeader::update(std::uint16_t bitDepth, std::uint32_t sampleRate, std::uint16_t numChannels, std::uint32_t numFrames) {
    // ─── COMM chunk ───────────────────────────────────────────────
    comm.BitDepth    = static_cast<std::int16_t>(bitDepth);
    comm.SampleRate  = static_cast<Float80>(sampleRate);      // already 10 bytes (static‑asserted elsewhere)
    comm.NumChannels = static_cast<std::int16_t>(numChannels);
    comm.NumSamples  = numFrames;

    // Uncompressed PCM ⇒ COMM payload is fixed at 18 bytes.
    comm.ChunkSize = 18;
    comm.ChunkID   = {{'C','O','M','M'}};

    // ─── FORM chunk ───────────────────────────────────────────────
    // Audio data size in bytes.
    const std::uint64_t soundBytes = static_cast<std::uint64_t>(bitDepth) * numChannels * numFrames / 8;

    // SSND payload = 8‑byte (<Offset><BlockSize>) prefix + raw samples.
    const std::uint64_t ssndPayloadSize = 8 /*Offset+BlockSize*/ + soundBytes;

    // FORM size = "AIFF" tag (4) + COMM (8 header + payload) + SSND (8 header + payload)
    const std::uint64_t formSize =
        4 +
        (8 + comm.ChunkSize) +
        (8 + ssndPayloadSize);

    form.ChunkID  = {{'F','O','R','M'}};
    form.FormType = {{'A','I','F','F'}};
    form.ChunkSize = static_cast<std::uint32_t>(formSize);

    ssnd.ChunkID  = {{'S','S','N','D'}};
    ssnd.ChunkSize = soundBytes;
}