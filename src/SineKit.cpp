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
    std::ifstream file(input_path, std::ios::binary);
    m_WAVHeader.readWAVHeader(file);
    numChannels = m_WAVHeader.m_FMTHeader.NumChannels;
    pcmBitDepth = m_WAVHeader.m_FMTHeader.BitsPerSample;
    pcmSampleRate = m_WAVHeader.m_FMTHeader.SampleRate;
    audioType = T_PCM;
    switch (pcmBitDepth) {
        case 16: bitType = T_16I; break;
        case 24: bitType = T_24I; break;
        case 32: bitType = T_32F; break;
        case 64: bitType = T_64F; break;
    }

    numFrames = m_WAVHeader.m_WAVDataHeader.Subchunk2Size / (numChannels*(pcmBitDepth/8));

    pcmBuffer16.clear();
    pcmBuffer24.clear();
    pcmBuffer32.clear();
    pcmBuffer16.clear();
    pcmBuffer64f.clear();

    switch (bitType) {
        case T_16I: {
            std::vector<int16_t> interleaved;
            interleaved.resize(numFrames*numChannels);
            for (int i = 0; i < numFrames*2; i++) {
                int16_t sample;
                file.read(reinterpret_cast<char*>(&sample), sizeof(sample));
                interleaved.at(i) = sample;
            }
            file.close();
            pcmBuffer16.resize(numChannels, numFrames);
            for (int i = 0; i < numFrames; i++) {
                for (int j = 0; j < numChannels; j++) {
                    pcmBuffer16.channels.at(j).at(i) = interleaved.at(i*numChannels + j);
                }
            }
            break;
        }
        case T_24I: {
            {
                std::vector<int32_t> interleaved;
                interleaved.resize(numFrames * numChannels);
                for (int i = 0; i < numFrames * numChannels; i++) {
                    uint8_t bytes[3];
                    file.read(reinterpret_cast<char*>(bytes), 3);

                    // Convert 3-byte little-endian to signed 32-bit integer
                    int32_t sample = (static_cast<int32_t>(bytes[2]) << 16) |
                                     (static_cast<int32_t>(bytes[1]) << 8) |
                                     (static_cast<int32_t>(bytes[0]));
                    // Sign-extend if necessary
                    if (sample & 0x800000) {
                        sample |= 0xFF000000;
                    }
                    interleaved.at(i) = sample;
                }
                file.close();
                pcmBuffer24.resize(numChannels, numFrames);
                for (int i = 0; i < numFrames; i++) {
                    for (int j = 0; j < numChannels; j++) {
                        pcmBuffer24.channels.at(j).at(i) = interleaved.at(i * numChannels + j);
                    }
                }
                break;
            }
        }
        case T_32F : {
            std::vector<float> interleaved;
            interleaved.resize(numFrames*numChannels);
            for (int i = 0; i < numFrames*2; i++) {
                float sample;
                file.read(reinterpret_cast<char*>(&sample), sizeof(sample));
                interleaved.at(i) = sample;
            }
            file.close();
            pcmBuffer32f.resize(numChannels, numFrames);
            for (int i = 0; i < numFrames; i++) {
                for (int j = 0; j < numChannels; j++) {
                    pcmBuffer32f.channels.at(j).at(i) = interleaved.at(i*numChannels + j);
                }
            }
            break;
        }
    }
}

void sk::SineKit::writeFile(std::filesystem::path output_path) {
    std::ofstream file(output_path, std::ios::binary);
    m_WAVHeader.writeWAVHeader(file);

    switch (bitType) {
        case T_16I: {
            std::vector<int16_t> interleaved;
            interleaved.resize(pcmBuffer16.channels.at(0).size() * numChannels);
            for (int i = 0; i < pcmBuffer16.channels.at(0).size(); i++) {
                for (int j = 0; j < numChannels; j++) {
                    interleaved.at(i*numChannels + j) = pcmBuffer16.channels.at(j).at(i);
                }
            }
            for (int i = 0; i < pcmBuffer16.channels.at(0).size()*numChannels; i++) {
                file.write(reinterpret_cast<char*>(&interleaved.at(i)), 2);
            }
            file.close();
            break;
        }
        case T_24I: {
            std::vector<uint8_t> bytes;
            bytes.reserve(pcmBuffer24.channels.at(0).size() * numChannels * 3);
            for (int i = 0; i < pcmBuffer24.channels.at(0).size(); i++) {
                for (int j = 0; j < numChannels; j++) {
                    int32_t sample = pcmBuffer24.channels.at(j).at(i);
                    bytes.push_back(static_cast<uint8_t>(sample & 0xFF));
                    bytes.push_back(static_cast<uint8_t>((sample >> 8) & 0xFF));
                    bytes.push_back(static_cast<uint8_t>((sample >> 16) & 0xFF));
                }
            }
            file.write(reinterpret_cast<char*>(bytes.data()), bytes.size());
            file.close();
            break;
        }
        case T_32F: {
            std::vector<float> interleaved;
            interleaved.resize(pcmBuffer32f.channels.at(0).size() * numChannels);
            for (int i = 0; i < pcmBuffer32f.channels.at(0).size(); i++) {
                for (int j = 0; j < numChannels; j++) {
                    interleaved.at(i*numChannels + j) = pcmBuffer32f.channels.at(j).at(i);
                }
            }
            for (int i = 0; i < pcmBuffer32f.channels.at(0).size()*numChannels; i++) {
                file.write(reinterpret_cast<char*>(&interleaved.at(i)), 4);
            }
            file.close();
        }
    }
}

void sk::SineKit::convertToFormat(BitType targetBitType) {
    if (targetBitType == T_24I && bitType == T_16I) {
        {
            bitType = T_24I;
            pcmBitDepth = 24;
            pcmBuffer24.resize(pcmBuffer16.numChannels, pcmBuffer16.numFrames);
            for (int i = 0; i < pcmBuffer16.channels.at(0).size(); i++) {
                for (int j = 0; j < numChannels; j++) {
                    pcmBuffer24.channels.at(j).at(i) = static_cast<int32_t>(pcmBuffer16.channels.at(j).at(i)) << 8;
                }
            }
            pcmBuffer16.clear();
        }
    }
    if (targetBitType == T_32F && bitType == T_16I) {
        bitType = T_32F;
        pcmBitDepth = 32;
        pcmBuffer32f.resize(pcmBuffer16.numChannels, pcmBuffer16.numFrames);
        for (int i = 0; i < pcmBuffer16.channels.at(0).size(); i++) {
            for (int j = 0; j < numChannels; j++) {
                pcmBuffer32f.channels.at(j).at(i) = static_cast<float>(pcmBuffer16.channels.at(j).at(i)) / 100000.0f;
            }
        }
        pcmBuffer16.clear();
        pcmBuffer16.clear();
    }
    updateHeaders();
}

void sk::SineKit::updateHeaders() {
    m_WAVHeader.updateWAVHeader(pcmBitDepth, pcmSampleRate, numChannels, numFrames);
}




void sk::WAVHeader::readWAVHeader(std::ifstream& file) {
    m_RIFFHeader.readRIFFHeader(file);
    std::cout << m_RIFFHeader;
    m_FMTHeader.readFMTHeader(file);
    std::cout << m_FMTHeader << std::endl;
    m_WAVDataHeader.readWAVDataHeader(file);
    std::cout << m_WAVDataHeader << std::endl;
}

void sk::WAVHeader::writeWAVHeader(std::ofstream& file) {
    m_RIFFHeader.writeRIFFHeader(file);
    m_FMTHeader.writeFMTHeader(file);
    m_WAVDataHeader.writeWAVDataHeader(file);
}

void sk::WAVHeader::updateWAVHeader(unsigned int bitDepth, unsigned int sampleRate, unsigned int numChannels, unsigned int numFrames) {
    m_FMTHeader.NumChannels = numChannels;
    m_FMTHeader.SampleRate = sampleRate;
    m_FMTHeader.ByteRate = sampleRate * numChannels * bitDepth / 8;
    m_FMTHeader.BlockAlign = numChannels * bitDepth / 8;
    m_FMTHeader.BitsPerSample = bitDepth;

    m_WAVDataHeader.Subchunk2Size = numFrames * numChannels * bitDepth / 8;

}





// std::ostream& sk::operator<<(std::ostream &os, const RIFFHeader &input) {
//     os << "Chunk ID: " << input.ChunkID << std::endl;
//     os << "Chunk size: " << input.ChunkSize << std::endl;
//     os << "Format: " << input.Format << std::endl;
//     return os;
// }

std::ostream& sk::operator<<(std::ostream& os, const Tag& input) {
    for (size_t i = 0; i < 4; i++) {
        os << input.v[i];
    }
    return os;
}

void sk::RIFFHeader::readRIFFHeader(std::ifstream& file) {
    file.read(ChunkID.v, sizeof(ChunkID.v));
    file.read(reinterpret_cast<char*>(&ChunkSize), sizeof(ChunkSize));
    file.read(Format.v, sizeof(Format.v));
}

void sk::RIFFHeader::writeRIFFHeader(std::ofstream& file) {
    file.write(ChunkID.v, sizeof(ChunkID.v));
    file.write(reinterpret_cast<char*>(&ChunkSize), sizeof(ChunkSize));
    file.write(Format.v, sizeof(Format.v));
}

std::ostream& sk::operator<<(std::ostream& os, const RIFFHeader& input) {
    os << "Chunk ID: " << input.ChunkID << std::endl;
    os << "Chunk Size: " << input.ChunkSize << std::endl;
    os << "Format: " << input.Format << std::endl;
    os << std::endl;
    return os;
}


void sk::FMTHeader::readFMTHeader(std::ifstream& file) {

    file.read(Subchunk1ID.v, sizeof(Subchunk1ID.v));

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

}

void sk::FMTHeader::writeFMTHeader(std::ofstream& file) {
    file.write(Subchunk1ID.v, sizeof(Subchunk1ID.v));
    file.write(reinterpret_cast<char*>(&Subchunk1Size), sizeof(Subchunk1Size));
    file.write(reinterpret_cast<char*>(&AudioFormat), sizeof(AudioFormat));
    file.write(reinterpret_cast<char*>(&NumChannels), sizeof(NumChannels));
    file.write(reinterpret_cast<char*>(&SampleRate), sizeof(SampleRate));
    file.write(reinterpret_cast<char*>(&ByteRate), sizeof(ByteRate));
    file.write(reinterpret_cast<char*>(&BlockAlign), sizeof(BlockAlign));
    file.write(reinterpret_cast<char*>(&BitsPerSample), sizeof(BitsPerSample));

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


void sk::WAVDataHeader::readWAVDataHeader(std::ifstream& file) {
    file.read(Subchunk2ID.v, sizeof(Subchunk2ID.v));

    file.read(reinterpret_cast<char*>(&Subchunk2Size), sizeof(Subchunk2Size));
}

void sk::WAVDataHeader::writeWAVDataHeader(std::ofstream& file) {
    file.write(Subchunk2ID.v, sizeof(Subchunk2ID.v));
    file.write(reinterpret_cast<char*>(&Subchunk2Size), sizeof(Subchunk2Size));
}

std::ostream &sk::operator<<(std::ostream &os, const WAVDataHeader &input) {
    os << "Subchunk2ID: " << input.Subchunk2ID << std::endl;
    os << "Subchunk2Size: " << input.Subchunk2Size << std::endl;
    return os;
}







