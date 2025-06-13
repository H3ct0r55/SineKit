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