#include "SineKit.h"

// ─── RIFF helpers ─────────────────────────────────────────────────────────
void sk::RIFFHeader::read(std::ifstream& file) {
    file.read(ChunkID.v, sizeof(ChunkID.v));
    file.read(reinterpret_cast<char*>(&ChunkSize), sizeof(ChunkSize));
    file.read(Format.v, sizeof(Format.v));
    if(!file) throw std::runtime_error("RIFF header read failed");
}

void sk::RIFFHeader::write(std::ofstream& file) const {
    file.write(ChunkID.v, sizeof(ChunkID.v));
    file.write(reinterpret_cast<const char*>(&ChunkSize), sizeof(ChunkSize));
    file.write(Format.v, sizeof(Format.v));
}


// ─── FMT helpers ──────────────────────────────────────────────────────────
void sk::FMTHeader::read(std::ifstream& file) {
    file.read(Subchunk1ID.v, sizeof(Subchunk1ID.v));
    file.read(reinterpret_cast<char*>(&Subchunk1Size), sizeof(Subchunk1Size));
    file.read(reinterpret_cast<char*>(&AudioFormat), sizeof(AudioFormat));
    file.read(reinterpret_cast<char*>(&NumChannels), sizeof(NumChannels));
    file.read(reinterpret_cast<char*>(&SampleRate), sizeof(SampleRate));
    file.read(reinterpret_cast<char*>(&ByteRate), sizeof(ByteRate));
    file.read(reinterpret_cast<char*>(&BlockAlign), sizeof(BlockAlign));
    file.read(reinterpret_cast<char*>(&BitsPerSample), sizeof(BitsPerSample));
    if (!file) throw std::runtime_error("FMTHeader header read failed");
}

void sk::FMTHeader::write(std::ofstream& file) const {
    file.write(Subchunk1ID.v, sizeof(Subchunk1ID.v));
    file.write(reinterpret_cast<const char*>(&Subchunk1Size), sizeof(Subchunk1Size));
    file.write(reinterpret_cast<const char*>(&AudioFormat), sizeof(AudioFormat));
    file.write(reinterpret_cast<const char*>(&NumChannels), sizeof(NumChannels));
    file.write(reinterpret_cast<const char*>(&SampleRate), sizeof(SampleRate));
    file.write(reinterpret_cast<const char*>(&ByteRate), sizeof(ByteRate));
    file.write(reinterpret_cast<const char*>(&BlockAlign), sizeof(BlockAlign));
    file.write(reinterpret_cast<const char*>(&BitsPerSample), sizeof(BitsPerSample));
}


void sk::FACTHeader::read(std::ifstream& file) {
    file.read(ChunkID.v, sizeof(ChunkID.v));
    file.read(reinterpret_cast<char*>(&ChunkSize), sizeof(ChunkSize));
    file.read(reinterpret_cast<char*>(&NumSamples), sizeof(NumSamples));
    if (!file) throw std::runtime_error("FACTHeader header read failed");
}

void sk::FACTHeader::write(std::ofstream& file) const {
    file.write(ChunkID.v, sizeof(ChunkID.v));
    file.write(reinterpret_cast<const char*>(&ChunkSize), sizeof(ChunkSize));
    file.write(reinterpret_cast<const char*>(&NumSamples), sizeof(NumSamples));
}


// ─── WAV DATA helpers ─────────────────────────────────────────────────────
void sk::WAVDataHeader::read(std::ifstream& file) {
    file.read(Subchunk2ID.v, sizeof(Subchunk2ID.v));
    file.read(reinterpret_cast<char*>(&Subchunk2Size), sizeof(Subchunk2Size));
    if (!file) throw std::runtime_error("WAV DATA header read failed");
}

void sk::WAVDataHeader::write(std::ofstream& file) const {
    file.write(Subchunk2ID.v, sizeof(Subchunk2ID.v));
    file.write(reinterpret_cast<const char*>(&Subchunk2Size), sizeof(Subchunk2Size));
}

bool verifyFMT(std::ifstream& file) {
    char readData[4];
    file.read(readData, sizeof(readData));
    if (!file) return false;
    file.seekg(-4, std::ios::cur);
    for (int i = 0; i < 4; i++) {
        std::cout << readData[i];
    }
    std::cout << std::endl;
    return readData[0] == 'f' && readData[1] == 'm' &&
           readData[2] == 't' && readData[3] == ' ';
}

bool verifyFact(std::ifstream& file) {
    char readData[4];
    file.read(readData, sizeof(readData));
    if (!file) return false;
    file.seekg(-4, std::ios::cur);
    for (int i = 0; i < 4; i++) {
        std::cout << readData[i];
    }
    std::cout << std::endl;
    return readData[0] == 'f' && readData[1] == 'a' &&
           readData[2] == 'c' && readData[3] == 't';
}

bool verifyData(std::ifstream& file) {
    char readData[4];
    file.read(readData, sizeof(readData));
    if (!file) return false;
    file.seekg(-4, std::ios::cur);
    for (int i = 0; i < 4; i++) {
        std::cout << readData[i];
    }
    std::cout << std::endl;
    return readData[0] == 'd' && readData[1] == 'a' &&
           readData[2] == 't' && readData[3] == 'a';
}

// ─── WAV HEADER helpers ───────────────────────────────────────────────────
void sk::WAVHeader::read(std::ifstream& file) {
    riff.read(file);
    while (!verifyFMT(file)) {
        file.seekg(1, std::ios::cur);
    }
    std::cout << "Broken FMT" << std::endl;
    fmt.read(file);
    while (!verifyFact(file) && !verifyData(file)) {
        file.seekg(1, std::ios::cur);
    }
    std::cout << "Broken Fact/Data" << std::endl;
    if (verifyFact(file)) {
        fact.read(file);
    }
    while (!verifyData(file)) {
        file.seekg(1, std::ios::cur);
    }
    std::cout << "Broken Data" << std::endl;
    data.read(file);
}

void sk::WAVHeader::write(std::ofstream& file) const {
    riff.write(file);
    fmt.write(file);
    if (fmt.AudioFormat == 3) {
        fact.write(file);
    }
    data.write(file);
}

void sk::SineKit::updateHeaders() {
    WAVHeader_.update(static_cast<std::uint16_t>(BitType_), static_cast<uint32_t>(SampleRate_), NumChannels_, NumFrames_, (BitType_ == BitType::F32 || BitType_ == BitType::F64));
}

void sk::WAVHeader::update(std::uint16_t bitDepth, std::uint32_t sampleRate, std::uint16_t numChannels, std::uint32_t numFrames, bool isFloat) {
    fmt.AudioFormat = isFloat ? 3 : 1;
    fmt.NumChannels = numChannels;
    fmt.SampleRate = sampleRate;
    fmt.BlockAlign = numChannels * bitDepth / 8;
    fmt.ByteRate = sampleRate * fmt.BlockAlign;
    fmt.BitsPerSample = bitDepth;

    fact.NumSamples = numFrames;

    data.Subchunk2Size = numFrames * fmt.BlockAlign;
    riff.ChunkSize = data.Subchunk2Size + data.Subchunk2Size;

}

template<typename T>
void sk::SineKit::readInterleaved(std::ifstream& in, AudioBuffer<T>& dst,
                              std::size_t frames, std::size_t ch)
{
    dst.resize(ch, frames);

    if constexpr (std::is_same_v<T, std::int32_t>) {
        // 24‑bit PCM → 32‑bit container (3‑byte little‑endian to signed 4‑byte)
        std::array<std::uint8_t, 3> trip{};
        for (std::size_t f = 0; f < frames; ++f) {
            for (std::size_t c = 0; c < ch; ++c) {
                in.read(reinterpret_cast<char*>(trip.data()), 3);
                if (!in)
                    throw std::runtime_error("PCM payload short (24‑bit read)");

                std::int32_t v = (static_cast<std::int32_t>(trip[2]) << 16) |
                                 (static_cast<std::int32_t>(trip[1]) << 8)  |
                                 (static_cast<std::int32_t>(trip[0]));

                // Sign‑extend 24‑bit value to 32‑bit
                if (v & 0x00800000) v |= 0xFF000000;

                dst(c, f) = v;
            }
        }
    } else {
        // Other formats: read directly as tightly packed interleaved data
        std::vector<T> inter(frames * ch);
        in.read(reinterpret_cast<char*>(inter.data()),
                static_cast<std::streamsize>(inter.size() * sizeof(T)));
        if (!in)
            throw std::runtime_error("PCM payload short");

        for (std::size_t f = 0; f < frames; ++f)
            for (std::size_t c = 0; c < ch; ++c)
                dst(c, f) = inter[f * ch + c];
    }
}

template<typename T>
void sk::SineKit::writeInterleaved(std::ofstream& out, const AudioBuffer<T>& src,
                               std::size_t frames, std::size_t ch)
{
    if constexpr (std::is_same_v<T, std::int32_t>) {
        // 24‑bit PCM: write 3 bytes per sample, little‑endian
        std::array<std::uint8_t,3> trip{};
        for (std::size_t f = 0; f < frames; ++f) {
            for (std::size_t cdx = 0; cdx < ch; ++cdx) {
                auto s = static_cast<std::uint32_t>(src(cdx, f));
                trip[0] =  s        & 0xFF;
                trip[1] = (s >>  8) & 0xFF;
                trip[2] = (s >> 16) & 0xFF;
                out.write(reinterpret_cast<char*>(trip.data()), 3);
            }
        }
    } else {
        std::vector<T> inter(frames * ch);
        for (std::size_t f = 0; f < frames; ++f)
            for (std::size_t cdx = 0; cdx < ch; ++cdx)
                inter[f * ch + cdx] = src(cdx, f);

        out.write(reinterpret_cast<const char*>(inter.data()),
                  static_cast<std::streamsize>(inter.size() * sizeof(T)));
    }
}

// ─── Public API ───────────────────────────────────────────────────────────
void sk::SineKit::loadFile(const std::filesystem::path& input_path)
{
    std::ifstream file(input_path, std::ios::binary);
    if(!file) throw std::runtime_error("open " + input_path.string());

    WAVHeader_.read(file);

    NumChannels_ = WAVHeader_.fmt.NumChannels;
    SampleRate_    = static_cast<SampleRate>(WAVHeader_.fmt.SampleRate);
    BitType_ = static_cast<BitType>(WAVHeader_.fmt.BitsPerSample);
    NumFrames_ = WAVHeader_.data.Subchunk2Size / WAVHeader_.fmt.BlockAlign;

    switch(BitType_){
        case BitType::I16: readInterleaved(file, Buffer16_, NumFrames_, NumChannels_); break;
        case BitType::I24: readInterleaved(file, Buffer24_, NumFrames_, NumChannels_); break;
        case BitType::F32: readInterleaved(file, Buffer32f_, NumFrames_, NumChannels_); break;
        case BitType::F64: readInterleaved(file, Buffer64f_, NumFrames_, NumChannels_); break;
        default: throw std::runtime_error("unsupported depth");
    }
}

void sk::SineKit::writeFile(const std::filesystem::path& output_path) const
{
    std::ofstream file(output_path, std::ios::binary);
    if(!file) throw std::runtime_error("create " + output_path.string());

    WAVHeader_.write(file);

    switch(BitType_){
        case BitType::I16: writeInterleaved(file, Buffer16_, NumFrames_, NumChannels_); break;
        case BitType::I24: writeInterleaved(file, Buffer24_, NumFrames_, NumChannels_); break;
        case BitType::F32: writeInterleaved(file, Buffer32f_, NumFrames_, NumChannels_); break;
        case BitType::F64: writeInterleaved(file, Buffer64f_, NumFrames_, NumChannels_); break;
        default: assert(false);
    }
}