#include "SineKit.h"

void sk::SineKit::updateHeaders() {
    WAVHeader_.update(static_cast<std::uint16_t>(BitType_), static_cast<uint32_t>(SampleRate_), NumChannels_, NumFrames_, (BitType_ == BitType::F32 || BitType_ == BitType::F64));
    AIFFHeader_.update(static_cast<std::uint16_t>(BitType_), static_cast<uint32_t>(SampleRate_), NumChannels_, NumFrames_);
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

void sk::SineKit::toBitDepth(BitType bitType) {
    if (bitType == BitType_) return;
    switch (bitType) {
        case BitType::I16: {
            switch (BitType_) {
                case BitType::I16:
                    break;
                case BitType::I24: {
                    Buffer16_.resize(NumChannels_, NumFrames_);
                    for (int i = 0; i < NumFrames_; i++) {
                        for (int j = 0; j < NumChannels_; j++) {
                            Buffer16_.channels.at(j).at(i) = static_cast<int16_t>(Buffer24_.channels.at(j).at(i) >> 8);
                        }
                    }
                    Buffer24_.channels.clear();
                    BitType_ = BitType::I16;
                    updateHeaders();
                    break;
                }
                case BitType::F32: {
                    Buffer16_.resize(NumChannels_, NumFrames_);
                    for (int i = 0; i < NumFrames_; i++) {
                        for (int j = 0; j < NumChannels_; j++) {
                            float sample = Buffer32f_.channels.at(j).at(i);
                            if (sample > 1.0f) sample = 1.0f;
                            if (sample < -1.0f) sample = -1.0f;
                            Buffer16_.channels.at(j).at(i) = static_cast<int16_t>(sample * 32767.0f);
                        }
                    }
                    Buffer32f_.channels.clear();
                    BitType_ = BitType::I16;
                    updateHeaders();
                    break;
                }
                case BitType::F64: {
                    Buffer16_.resize(NumChannels_, NumFrames_);
                    for (int i = 0; i < NumFrames_; i++) {
                        for (int j = 0; j < NumChannels_; j++) {
                            double sample = Buffer64f_.channels.at(j).at(i);
                            if (sample > 1.0) sample = 1.0;
                            if (sample < -1.0) sample = -1.0;
                            Buffer16_.channels.at(j).at(i) = static_cast<int16_t>(sample * 32767.0);
                        }
                    }
                    Buffer64f_.channels.clear();
                    BitType_ = BitType::I16;
                    updateHeaders();
                    break;
                }
                default:
                    throw std::runtime_error("unsupported bit depth conversion");
            }
            break;
        }
        case BitType::I24: {
            switch (BitType_) {
                case BitType::I24:
                    break;
                case BitType::I16: {
                    Buffer24_.resize(NumChannels_, NumFrames_);
                    for (int i = 0; i < NumFrames_; i++) {
                        for (int j = 0; j < NumChannels_; j++) {
                            Buffer24_.channels.at(j).at(i) = static_cast<int32_t>(Buffer16_.channels.at(j).at(i)) << 8;
                        }
                    }
                    Buffer16_.channels.clear();
                    BitType_ = BitType::I24;
                    updateHeaders();
                    break;
                }
                case BitType::F32: {
                    Buffer24_.resize(NumChannels_, NumFrames_);
                    for (int i = 0; i < NumFrames_; i++) {
                        for (int j = 0; j < NumChannels_; j++) {
                            float sample = Buffer32f_.channels.at(j).at(i);
                            if (sample > 1.0f) sample = 1.0f;
                            if (sample < -1.0f) sample = -1.0f;
                            Buffer24_.channels.at(j).at(i) = static_cast<int32_t>(sample * 8388607.0f);
                        }
                    }
                    Buffer32f_.channels.clear();
                    BitType_ = BitType::I24;
                    updateHeaders();
                    break;
                }
                case BitType::F64: {
                    Buffer24_.resize(NumChannels_, NumFrames_);
                    for (int i = 0; i < NumFrames_; i++) {
                        for (int j = 0; j < NumChannels_; j++) {
                            double sample = Buffer64f_.channels.at(j).at(i);
                            if (sample > 1.0) sample = 1.0;
                            if (sample < -1.0) sample = -1.0;
                            Buffer24_.channels.at(j).at(i) = static_cast<int32_t>(sample * 8388607.0);
                        }
                    }
                    Buffer64f_.channels.clear();
                    BitType_ = BitType::I24;
                    updateHeaders();
                    break;
                }
                default:
                    throw std::runtime_error("unsupported bit depth conversion");
            }
            break;
        }
        case BitType::F32: {
            switch (BitType_) {
                case BitType::F32:
                    break;
                case BitType::I16: {
                    Buffer32f_.resize(NumChannels_, NumFrames_);
                    for (int i = 0; i < NumFrames_; i++) {
                        for (int j = 0; j < NumChannels_; j++) {
                            Buffer32f_.channels.at(j).at(i) = static_cast<float>(Buffer16_.channels.at(j).at(i)) / 32767.0f;
                        }
                    }
                    Buffer16_.channels.clear();
                    BitType_ = BitType::F32;
                    updateHeaders();
                    break;
                }
                case BitType::I24: {
                    Buffer32f_.resize(NumChannels_, NumFrames_);
                    for (int i = 0; i < NumFrames_; i++) {
                        for (int j = 0; j < NumChannels_; j++) {
                            Buffer32f_.channels.at(j).at(i) = static_cast<float>(Buffer24_.channels.at(j).at(i)) / 8388607.0f;
                        }
                    }
                    Buffer24_.channels.clear();
                    BitType_ = BitType::F32;
                    updateHeaders();
                    break;
                }
                case BitType::F64: {
                    Buffer32f_.resize(NumChannels_, NumFrames_);
                    for (int i = 0; i < NumFrames_; i++) {
                        for (int j = 0; j < NumChannels_; j++) {
                            Buffer32f_.channels.at(j).at(i) = static_cast<float>(Buffer64f_.channels.at(j).at(i));
                        }
                    }
                    Buffer64f_.channels.clear();
                    BitType_ = BitType::F32;
                    updateHeaders();
                    break;
                }
                default:
                    throw std::runtime_error("unsupported bit depth conversion");
            }
            break;
        }
        case BitType::F64: {
            switch (BitType_) {
                case BitType::F64:
                    break;
                case BitType::I16: {
                    Buffer64f_.resize(NumChannels_, NumFrames_);
                    for (int i = 0; i < NumFrames_; i++) {
                        for (int j = 0; j < NumChannels_; j++) {
                            Buffer64f_.channels.at(j).at(i) = static_cast<double>(Buffer16_.channels.at(j).at(i)) / 32767.0;
                        }
                    }
                    Buffer16_.channels.clear();
                    BitType_ = BitType::F64;
                    updateHeaders();
                    break;
                }
                case BitType::I24: {
                    Buffer64f_.resize(NumChannels_, NumFrames_);
                    for (int i = 0; i < NumFrames_; i++) {
                        for (int j = 0; j < NumChannels_; j++) {
                            Buffer64f_.channels.at(j).at(i) = static_cast<double>(Buffer24_.channels.at(j).at(i)) / 8388607.0;
                        }
                    }
                    Buffer24_.channels.clear();
                    BitType_ = BitType::F64;
                    updateHeaders();
                    break;
                }
                case BitType::F32: {
                    Buffer64f_.resize(NumChannels_, NumFrames_);
                    for (int i = 0; i < NumFrames_; i++) {
                        for (int j = 0; j < NumChannels_; j++) {
                            Buffer64f_.channels.at(j).at(i) = static_cast<double>(Buffer32f_.channels.at(j).at(i));
                        }
                    }
                    Buffer32f_.channels.clear();
                    BitType_ = BitType::F64;
                    updateHeaders();
                    break;
                }
                default:
                    throw std::runtime_error("unsupported bit depth conversion");
            }
            break;
        }
        default:
            throw std::runtime_error("unsupported bit depth conversion");
    }
}
