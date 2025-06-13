#include "SineKit.h"

void sk::FORMHeader::read(std::ifstream& file) {
    file.read(ChunkID.v, sizeof(ChunkID.v));
    ChunkSize = sk::endian::read_be<decltype(ChunkSize)>(file);
    file.read(FormType.v, sizeof(FormType.v));
}

void sk::FORMHeader::write(std::ofstream& file) const {
    file.write(ChunkID.v, sizeof(ChunkID.v));
    sk::endian::write_be<decltype(ChunkSize)>(file, ChunkSize);
    file.write(FormType.v, sizeof(FormType.v));
}
std::ostream& sk::operator<<(std::ostream& os, const sk::FORMHeader& input) {
    os << "FORM Header" << std::endl;
    os << "Chunk ID: " << input.ChunkID << std::endl;
    os << "Chunk Size: " << input.ChunkSize << std::endl;
    os << "Form Type: " << input.FormType << std::endl;
    return os;
}

void sk::COMMHeader::read(std::ifstream& file) {
    file.read(ChunkID.v, sizeof(ChunkID.v));
    ChunkSize = sk::endian::read_be<decltype(ChunkSize)>(file);
    NumChannels = sk::endian::read_be<decltype(NumChannels)>(file);
    NumSamples = sk::endian::read_be<decltype(NumSamples)>(file);
    BitDepth = sk::endian::read_be<decltype(BitDepth)>(file);
    file.read(reinterpret_cast<char*>(&SampleRate), sizeof(SampleRate));
}

void sk::COMMHeader::write(std::ofstream& file) const {
    file.write(ChunkID.v, sizeof(ChunkID.v));
    sk::endian::write_be<decltype(ChunkSize)>(file, ChunkSize);
    std::cout << ChunkSize << std::endl;
    sk::endian::write_be<decltype(NumChannels)>(file, NumChannels);
    sk::endian::write_be<decltype(NumSamples)>(file, NumSamples);
    sk::endian::write_be<decltype(BitDepth)>(file, BitDepth);
    file.write(reinterpret_cast<const char*>(&SampleRate), sizeof(SampleRate));
}
std::ostream &sk::operator<<(std::ostream &os, const COMMHeader &input) {
    os << "COMM Header" << std::endl;
    os << "Chunk ID: " << input.ChunkID << std::endl;
    os << "Chunk Size: " << input.ChunkSize << std::endl;
    os << "Num Channels: " << input.NumChannels << std::endl;
    os << "Num Samples: " << input.NumSamples << std::endl;
    os << "BitDepth: " << input.BitDepth << std::endl;
    os << "SampleRate: " << input.SampleRate << std::endl;
    return os;
}

void sk::SSNDHeader::read(std::ifstream& file) {
    file.read(ChunkID.v, sizeof(ChunkID.v));
    ChunkSize = sk::endian::read_be<decltype(ChunkSize)>(file);
    Offset = sk::endian::read_be<decltype(Offset)>(file);
    BlockSize = sk::endian::read_be<decltype(BlockSize)>(file);
}

void sk::SSNDHeader::write(std::ofstream& file) const {
    file.write(ChunkID.v, sizeof(ChunkID.v));
    sk::endian::write_be<decltype(ChunkSize)>(file, ChunkSize);
    std::cout << ChunkSize << std::endl;
    sk::endian::write_be<decltype(Offset)>(file, Offset);
    sk::endian::write_be<decltype(BlockSize)>(file, BlockSize);
}


int verifyAIFFChunk(std::ifstream& file) {
    char readData[4];
    file.read(readData, sizeof(readData));
    if (!file) return false;
    file.seekg(-4, std::ios::cur);
    if (std::memcmp(readData, "FORM", 4) == 0) return 1;
    if (std::memcmp(readData, "COMM", 4) == 0) return 2;
    if (std::memcmp(readData, "SSND", 4) == 0) return 3;
    return 0;
}

void sk::AIFFHeader::read(std::ifstream& file) {
    bool foundFORM = false;
    bool foundCOMM = false;
    bool foundSSND = false;
    while (!foundFORM || !foundCOMM || !foundSSND) {
        switch (verifyAIFFChunk(file)) {
            case 1 : {
                if (!foundFORM) {
                    foundFORM = true;
                    form.read(file);
                    std::cout << "Found FORM header" << std::endl;
                    break;
                }
                throw std::runtime_error("Multiple FORM headers found, invalid file");
            }
            case 2 : {
                if (!foundCOMM) {
                    foundCOMM = true;
                    comm.read(file);
                    std::cout << "Found COMM header" << std::endl;
                    break;
                }
                throw std::runtime_error("Multiple COMM headers found, invalid file");
            }
            case 3 : {
                if (!foundSSND) {
                    foundSSND = true;
                    ssnd.read(file);
                    std::cout << "Found SSND header" << std::endl;
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

std::ostream& sk::operator<<(std::ostream& os, const sk::AIFFHeader& aiff) {
    os << "AIFF Header" << std::endl;
    os << aiff.form;
    os << aiff.comm;
    return os;
}

void sk::AIFFHeader::write(std::ofstream& file) const {
    form.write(file);
    comm.write(file);
    ssnd.write(file);
}

void sk::SineKit::updateHeaders() {
    WAVHeader_.update(static_cast<std::uint16_t>(BitType_), static_cast<uint32_t>(SampleRate_), NumChannels_, NumFrames_, (BitType_ == BitType::F32 || BitType_ == BitType::F64));
    AIFFHeader_.update(static_cast<std::uint16_t>(BitType_), static_cast<uint32_t>(SampleRate_), NumChannels_, NumFrames_);
}

void sk::AIFFHeader::update(std::uint16_t bitDepth, std::uint32_t sampleRate, std::uint16_t numChannels, std::uint32_t numFrames) {
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

template<typename T>
void sk::SineKit::readInterleaved(std::ifstream& in,
                                  AudioBuffer<T>& dst,
                                  std::size_t frames,
                                  std::size_t ch,
                                  sk::endian::Endian fileEndian)
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

                std::uint32_t v32;
                if (fileEndian == sk::endian::Endian::Little)
                    v32 =  (static_cast<std::uint32_t>(trip[2]) << 16) |
                           (static_cast<std::uint32_t>(trip[1]) <<  8) |
                           (static_cast<std::uint32_t>(trip[0]));
                else
                    v32 =  (static_cast<std::uint32_t>(trip[0]) << 16) |
                           (static_cast<std::uint32_t>(trip[1]) <<  8) |
                           (static_cast<std::uint32_t>(trip[2]));
                // sign‑extend
                if (v32 & 0x00800000) v32 |= 0xFF000000;
                dst(c, f) = static_cast<std::int32_t>(v32);
            }
        }
    } else {
        // Other formats: read directly as tightly packed interleaved data
        std::vector<T> inter(frames * ch);
        in.read(reinterpret_cast<char*>(inter.data()),
                static_cast<std::streamsize>(inter.size() * sizeof(T)));
        if (!in)
            throw std::runtime_error("PCM payload short");

        for (auto& v : inter) {
            if constexpr (sizeof(T) != 1) {
                v = (fileEndian == sk::endian::Endian::Little)
                        ? sk::endian::le_to_host(v)
                        : sk::endian::be_to_host(v);
            }
        }

        for (std::size_t f = 0; f < frames; ++f)
            for (std::size_t c = 0; c < ch; ++c)
                dst(c, f) = inter[f * ch + c];
    }
}

template<typename T>
void sk::SineKit::writeInterleaved(std::ofstream& out,
                                   const AudioBuffer<T>& src,
                                   std::size_t frames,
                                   std::size_t ch,
                                   sk::endian::Endian fileEndian)
{
    if constexpr (std::is_same_v<T, std::int32_t>) {
        // 24‑bit PCM: write 3 bytes per sample, little‑endian
        std::array<std::uint8_t,3> trip{};
        for (std::size_t f = 0; f < frames; ++f) {
            for (std::size_t cdx = 0; cdx < ch; ++cdx) {
                auto s = static_cast<std::uint32_t>(src(cdx, f));
                if (fileEndian == sk::endian::Endian::Little) {
                    trip[0] =  s        & 0xFF;
                    trip[1] = (s >>  8) & 0xFF;
                    trip[2] = (s >> 16) & 0xFF;
                } else {
                    trip[2] =  s        & 0xFF;
                    trip[1] = (s >>  8) & 0xFF;
                    trip[0] = (s >> 16) & 0xFF;
                }
                out.write(reinterpret_cast<char*>(trip.data()), 3);
            }
        }
    } else {
        std::vector<T> inter(frames * ch);
        for (std::size_t f = 0; f < frames; ++f)
            for (std::size_t cdx = 0; cdx < ch; ++cdx)
                inter[f * ch + cdx] = src(cdx, f);

        for (auto& v : inter) {
            if constexpr (sizeof(T) != 1) {
                v = sk::endian::host_to_file(v, fileEndian);
            }
        }

        out.write(reinterpret_cast<const char*>(inter.data()),
                  static_cast<std::streamsize>(inter.size() * sizeof(T)));
    }
}

// ─── Public API ───────────────────────────────────────────────────────────
void sk::SineKit::loadFile(const std::filesystem::path& input_path)
{
    std::ifstream file(input_path, std::ios::binary);
    if(!file) throw std::runtime_error("open " + input_path.string());
    if (input_path.extension() == ".wav") {
        WAVHeader_.read(file);

        NumChannels_ = WAVHeader_.fmt.NumChannels;
        SampleRate_    = static_cast<SampleRate>(WAVHeader_.fmt.SampleRate);
        BitType_ = static_cast<BitType>(WAVHeader_.fmt.BitsPerSample);
        NumFrames_ = WAVHeader_.data.Subchunk2Size / WAVHeader_.fmt.BlockAlign;

        switch(BitType_){
            case BitType::I16: readInterleaved(file, Buffer16_, NumFrames_, NumChannels_, sk::endian::Endian::Little); break;
            case BitType::I24: readInterleaved(file, Buffer24_, NumFrames_, NumChannels_, sk::endian::Endian::Little); break;
            case BitType::F32: readInterleaved(file, Buffer32f_, NumFrames_, NumChannels_, sk::endian::Endian::Little); break;
            case BitType::F64: readInterleaved(file, Buffer64f_, NumFrames_, NumChannels_, sk::endian::Endian::Little); break;
            default: throw std::runtime_error("unsupported depth");
        }
        updateHeaders();
    } else if (input_path.extension() == ".aiff") {
        AIFFHeader_.read(file);
        std::cout << AIFFHeader_ << std::endl;

        NumChannels_ = AIFFHeader_.comm.NumChannels;
        SampleRate_ = static_cast<SampleRate>(static_cast<std::uint32_t>(AIFFHeader_.comm.SampleRate));
        BitType_ = static_cast<BitType>(AIFFHeader_.comm.BitDepth);
        NumFrames_ = AIFFHeader_.comm.NumSamples;

        switch(BitType_){
            case BitType::I16: readInterleaved(file, Buffer16_, NumFrames_, NumChannels_, sk::endian::Endian::Big); break;
            case BitType::I24: readInterleaved(file, Buffer24_, NumFrames_, NumChannels_, sk::endian::Endian::Big); break;
            case BitType::F32: readInterleaved(file, Buffer32f_, NumFrames_, NumChannels_, sk::endian::Endian::Big); break;
            case BitType::F64: readInterleaved(file, Buffer64f_, NumFrames_, NumChannels_, sk::endian::Endian::Big); break;
            default: throw std::runtime_error("unsupported depth");
        }
        updateHeaders();
    }
    file.close();
}

void sk::SineKit::writeFile(const std::filesystem::path& output_path) const
{
    std::ofstream file(output_path, std::ios::binary);
    if(!file) throw std::runtime_error("create " + output_path.string());

    if (output_path.extension() == ".wav") {
        WAVHeader_.write(file);

        switch(BitType_){
            case BitType::I16: writeInterleaved(file, Buffer16_, NumFrames_, NumChannels_, sk::endian::Endian::Little); break;
            case BitType::I24: writeInterleaved(file, Buffer24_, NumFrames_, NumChannels_, sk::endian::Endian::Little); break;
            case BitType::F32: writeInterleaved(file, Buffer32f_, NumFrames_, NumChannels_, sk::endian::Endian::Little); break;
            case BitType::F64: writeInterleaved(file, Buffer64f_, NumFrames_, NumChannels_, sk::endian::Endian::Little); break;
            default: assert(false);
        }
    } else if (output_path.extension() == ".aiff") {
        AIFFHeader_.write(file);

        switch(BitType_) {
            case BitType::I16: writeInterleaved(file, Buffer16_, NumFrames_, NumChannels_, sk::endian::Endian::Big); break;
            case BitType::I24: writeInterleaved(file, Buffer24_, NumFrames_, NumChannels_, sk::endian::Endian::Big); break;
            case BitType::F32: writeInterleaved(file, Buffer32f_, NumFrames_, NumChannels_, sk::endian::Endian::Big); break;
            case BitType::F64: writeInterleaved(file, Buffer64f_, NumFrames_, NumChannels_, sk::endian::Endian::Big); break;
            default: assert(false);
        }
    }
    file.close();
}