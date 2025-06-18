#include "SineKit.h"

void sk::SineKit::clearBut(sk::BitType bitType) {
    switch (bitType) {
        case BitType::I8: {
            //Buffer8I_.clear();
            Buffer16I_.clear();
            Buffer24I_.clear();
            Buffer32I_.clear();
            Buffer32F_.clear();
            Buffer64F_.clear();
            break;
        }
        case BitType::I16: {
            Buffer8I_.clear();
            //Buffer16I_.clear();
            Buffer24I_.clear();
            Buffer32I_.clear();
            Buffer32F_.clear();
            Buffer64F_.clear();
            break;
        }
        case BitType::I24: {
            Buffer8I_.clear();
            Buffer16I_.clear();
            //Buffer24I_.clear();
            Buffer32I_.clear();
            Buffer32F_.clear();
            Buffer64F_.clear();
            break;
        }
        case BitType::F32 : {
            Buffer8I_.clear();
            Buffer16I_.clear();
            Buffer24I_.clear();
            Buffer32I_.clear();
            //Buffer32F_.clear();
            Buffer64F_.clear();
            break;
        }
        case BitType::F64: {
            Buffer8I_.clear();
            Buffer16I_.clear();
            Buffer24I_.clear();
            Buffer32I_.clear();
            Buffer32F_.clear();
            //Buffer64F_.clear();
            break;
        }
    }
}

template<typename T>
void sk::AudioBuffer<T>::clear() {
    channels.clear();
}



void sk::SineKit::updateHeaders() {
    WAVHeader_.update(static_cast<std::uint16_t>(BitType_), static_cast<uint32_t>(SampleRate_), NumChannels_, NumFrames_, (BitType_ == BitType::F32 || BitType_ == BitType::F64));
    AIFFHeader_.update(static_cast<std::uint16_t>(BitType_), static_cast<uint32_t>(SampleRate_), NumChannels_, NumFrames_,(BitType_ == BitType::F32 || BitType_ == BitType::F64));
}

template<typename T>
void sk::SineKit::readInterleaved(std::ifstream& in,
                                  AudioBuffer<T>& dst,
                                  std::size_t frames,
                                  std::size_t ch,
                                  sk::endian::Endian fileEndian, sk::BitType bitType)
{
    dst.resize(ch, frames);

    if (bitType == sk::BitType::I24) {
        std::array<std::uint8_t, 3> trip{};
        for (std::size_t f = 0; f < frames; ++f) {
            for (std::size_t c = 0; c < ch; ++c) {
                in.read(reinterpret_cast<char*>(trip.data()), 3);
                if (!in) throw std::runtime_error("PCM payload short (24‑bit read)");

                std::uint32_t v32;
                if (fileEndian == sk::endian::Endian::Little) {
                    v32 =   (static_cast<std::uint32_t>(trip[2]) << 16) |
                            (static_cast<std::uint32_t>(trip[1]) << 8) |
                            (static_cast<std::uint32_t>(trip[0]));
                } else {
                    v32 =   (static_cast<std::uint32_t>(trip[0]) << 16) |
                            (static_cast<std::uint32_t>(trip[1]) << 8) |
                            (static_cast<std::uint32_t>(trip[2]));
                }

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
                                   sk::endian::Endian fileEndian,
                                   sk::BitType bitType)
{

    if (bitType == sk::BitType::I24) {
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
            case BitType::I16: readInterleaved(file, Buffer16I_, NumFrames_, NumChannels_, sk::endian::Endian::Little, BitType_); break;
            case BitType::I24: readInterleaved(file, Buffer24I_, NumFrames_, NumChannels_, sk::endian::Endian::Little, BitType_); break;
            case BitType::F32: readInterleaved(file, Buffer32F_, NumFrames_, NumChannels_, sk::endian::Endian::Little, BitType_); break;
            case BitType::F64: readInterleaved(file, Buffer64F_, NumFrames_, NumChannels_, sk::endian::Endian::Little, BitType_); break;
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
            case BitType::I16: readInterleaved(file, Buffer16I_, NumFrames_, NumChannels_, sk::endian::Endian::Big, BitType_); break;
            case BitType::I24: readInterleaved(file, Buffer24I_, NumFrames_, NumChannels_, sk::endian::Endian::Big, BitType_); break;
            case BitType::F32: readInterleaved(file, Buffer32F_, NumFrames_, NumChannels_, sk::endian::Endian::Big, BitType_); break;
            case BitType::F64: readInterleaved(file, Buffer64F_, NumFrames_, NumChannels_, sk::endian::Endian::Big, BitType_); break;
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
            case BitType::I16: writeInterleaved(file, Buffer16I_, NumFrames_, NumChannels_, sk::endian::Endian::Little, BitType_); break;
            case BitType::I24: writeInterleaved(file, Buffer24I_, NumFrames_, NumChannels_, sk::endian::Endian::Little, BitType_); break;
            case BitType::F32: writeInterleaved(file, Buffer32F_, NumFrames_, NumChannels_, sk::endian::Endian::Little, BitType_); break;
            case BitType::F64: writeInterleaved(file, Buffer64F_, NumFrames_, NumChannels_, sk::endian::Endian::Little, BitType_); break;
            default: assert(false);
        }
    } else if (output_path.extension() == ".aiff") {
        AIFFHeader_.write(file);

        switch(BitType_) {
            case BitType::I16: writeInterleaved(file, Buffer16I_, NumFrames_, NumChannels_, sk::endian::Endian::Big, BitType_); break;
            case BitType::I24: writeInterleaved(file, Buffer24I_, NumFrames_, NumChannels_, sk::endian::Endian::Big, BitType_); break;
            case BitType::F32: writeInterleaved(file, Buffer32F_, NumFrames_, NumChannels_, sk::endian::Endian::Big, BitType_); break;
            case BitType::F64: writeInterleaved(file, Buffer64F_, NumFrames_, NumChannels_, sk::endian::Endian::Big, BitType_); break;
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
                    Buffer16I_.resize(NumChannels_, NumFrames_);
                    for (int i = 0; i < NumFrames_; i++) {
                        for (int j = 0; j < NumChannels_; j++) {
                            Buffer16I_.channels.at(j).at(i) = static_cast<int16_t>(Buffer24I_.channels.at(j).at(i) >> 8);
                        }
                    }

                    BitType_ = BitType::I16;
                    clearBut(BitType_);
                    updateHeaders();
                    break;
                }
                case BitType::F32: {
                    Buffer16I_.resize(NumChannels_, NumFrames_);
                    for (int i = 0; i < NumFrames_; i++) {
                        for (int j = 0; j < NumChannels_; j++) {
                            float sample = Buffer32F_.channels.at(j).at(i);
                            if (sample > 1.0f) sample = 1.0f;
                            if (sample < -1.0f) sample = -1.0f;
                            Buffer16I_.channels.at(j).at(i) = static_cast<int16_t>(sample * 32767.0f);
                        }
                    }
                    BitType_ = BitType::I16;
                    clearBut(BitType_);
                    updateHeaders();
                    break;
                }
                case BitType::F64: {
                    Buffer16I_.resize(NumChannels_, NumFrames_);
                    for (int i = 0; i < NumFrames_; i++) {
                        for (int j = 0; j < NumChannels_; j++) {
                            double sample = Buffer64F_.channels.at(j).at(i);
                            if (sample > 1.0) sample = 1.0;
                            if (sample < -1.0) sample = -1.0;
                            Buffer16I_.channels.at(j).at(i) = static_cast<int16_t>(sample * 32767.0);
                        }
                    }
                    BitType_ = BitType::I16;
                    clearBut(BitType_);
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
                    Buffer24I_.resize(NumChannels_, NumFrames_);
                    for (int i = 0; i < NumFrames_; i++) {
                        for (int j = 0; j < NumChannels_; j++) {
                            Buffer24I_.channels.at(j).at(i) = static_cast<int32_t>(Buffer16I_.channels.at(j).at(i)) << 8;
                        }
                    }
                    BitType_ = BitType::I24;
                    clearBut(BitType_);
                    updateHeaders();
                    break;
                }
                case BitType::F32: {
                    Buffer24I_.resize(NumChannels_, NumFrames_);
                    for (int i = 0; i < NumFrames_; i++) {
                        for (int j = 0; j < NumChannels_; j++) {
                            float sample = Buffer32F_.channels.at(j).at(i);
                            if (sample > 1.0f) sample = 1.0f;
                            if (sample < -1.0f) sample = -1.0f;
                            Buffer24I_.channels.at(j).at(i) = static_cast<int32_t>(sample * 8388607.0f);
                        }
                    }
                    BitType_ = BitType::I24;
                    clearBut(BitType_);
                    updateHeaders();
                    break;
                }
                case BitType::F64: {
                    Buffer24I_.resize(NumChannels_, NumFrames_);
                    for (int i = 0; i < NumFrames_; i++) {
                        for (int j = 0; j < NumChannels_; j++) {
                            double sample = Buffer64F_.channels.at(j).at(i);
                            if (sample > 1.0) sample = 1.0;
                            if (sample < -1.0) sample = -1.0;
                            Buffer24I_.channels.at(j).at(i) = static_cast<int32_t>(sample * 8388607.0);
                        }
                    }
                    BitType_ = BitType::I24;
                    updateHeaders();
                    clearBut(BitType_);
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
                    Buffer32F_.resize(NumChannels_, NumFrames_);
                    for (int i = 0; i < NumFrames_; i++) {
                        for (int j = 0; j < NumChannels_; j++) {
                            Buffer32F_.channels.at(j).at(i) = static_cast<float>(Buffer16I_.channels.at(j).at(i)) / 32767.0f;
                        }
                    }
                    BitType_ = BitType::F32;
                    clearBut(BitType_);
                    updateHeaders();
                    break;
                }
                case BitType::I24: {
                    Buffer32F_.resize(NumChannels_, NumFrames_);
                    for (int i = 0; i < NumFrames_; i++) {
                        for (int j = 0; j < NumChannels_; j++) {
                            Buffer32F_.channels.at(j).at(i) = static_cast<float>(Buffer24I_.channels.at(j).at(i)) / 8388607.0f;
                        }
                    }
                    BitType_ = BitType::F32;
                    clearBut(BitType_);
                    updateHeaders();
                    break;
                }
                case BitType::F64: {
                    Buffer32F_.resize(NumChannels_, NumFrames_);
                    for (int i = 0; i < NumFrames_; i++) {
                        for (int j = 0; j < NumChannels_; j++) {
                            Buffer32F_.channels.at(j).at(i) = static_cast<float>(Buffer64F_.channels.at(j).at(i));
                        }
                    }
                    BitType_ = BitType::F32;
                    clearBut(BitType_);
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
                    Buffer64F_.resize(NumChannels_, NumFrames_);
                    for (int i = 0; i < NumFrames_; i++) {
                        for (int j = 0; j < NumChannels_; j++) {
                            Buffer64F_.channels.at(j).at(i) = static_cast<double>(Buffer16I_.channels.at(j).at(i)) / 32767.0;
                        }
                    }
                    BitType_ = BitType::F64;
                    clearBut(BitType_);
                    updateHeaders();
                    break;
                }
                case BitType::I24: {
                    Buffer64F_.resize(NumChannels_, NumFrames_);
                    for (int i = 0; i < NumFrames_; i++) {
                        for (int j = 0; j < NumChannels_; j++) {
                            Buffer64F_.channels.at(j).at(i) = static_cast<double>(Buffer24I_.channels.at(j).at(i)) / 8388607.0;
                        }
                    }
                    BitType_ = BitType::F64;
                    clearBut(BitType_);
                    updateHeaders();
                    break;
                }
                case BitType::F32: {
                    Buffer64F_.resize(NumChannels_, NumFrames_);
                    for (int i = 0; i < NumFrames_; i++) {
                        for (int j = 0; j < NumChannels_; j++) {
                            Buffer64F_.channels.at(j).at(i) = static_cast<double>(Buffer32F_.channels.at(j).at(i));
                        }
                    }
                    BitType_ = BitType::F64;
                    clearBut(BitType_);
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

#ifdef USE_THREADING
void* upsampleThread(void* arg) {
    
}
#endif

template<typename T>
void sk::SineKit::upsample(std::uint8_t scale, std::uint8_t interpolation, sk::AudioBuffer<T> &buffer, sk::BitType bitType) {
    std::int64_t uFrames = NumFrames_*scale;
    AudioBuffer<T> tempBuffer;
    tempBuffer.resize(NumChannels_, uFrames);
    double clampMin = 0;
    double clampMax = 0;
    switch (bitType) {
        case BitType::I8 : clampMin = 0; clampMax = 255; break;
        case BitType::I16: clampMin = -32768; clampMax = 32767; break;
        case BitType::I24: clampMin = -8388608; clampMax = 8388607; break;
        case BitType::F32: clampMin = -10.0; clampMax = 10.0; break;
    }

    for (std::int64_t i = 0; i < NumChannels_; i++) {
        for (std::int64_t j = NumFrames_ - 1; j >= 0; j--) {
            tempBuffer.channels[i][j*scale] = buffer.channels[i][j];
        }
        for (std::int64_t j = 0; j < uFrames; j++) {
            if (j%scale != 0) {
                tempBuffer.channels[i][j] = 0;
            }
        }
    }

    switch (interpolation) {
        case 1 : {
            for (std::int64_t i = 0; i < NumChannels_; i++) {
                for (std::int64_t j = 0; j < NumFrames_ - 1; j++) {
                    long double ptAy = tempBuffer.channels.at(i).at(j * scale);
                    long double ptBy = tempBuffer.channels.at(i).at((j + 1) * scale);
                    long double delta = (ptBy - ptAy) / static_cast<long double>(scale);

                    for (int k = 1; k < scale; k++) {
                        tempBuffer.channels.at(i).at(j * scale + k) = ptAy + delta * k;

                    }
                }
            }
            break;
        }
        case 5 : {
            std::vector<double> sincLUT;
            sincLUT.resize(100001);
            for (std::int64_t k = -50000; k < 50001; k++) {
                sincLUT.at(k+50000) = (k == 0) ? 1.0 : std::sin(M_PI * k / scale) / (M_PI * k / scale);
            }
            AudioBuffer<T> bufferCache;
            bufferCache.resize(NumChannels_, uFrames);
            bufferCache = tempBuffer;
            switch (bitType) {
                case BitType::I8 : case BitType::I16 : case BitType::I24 : {
                    for (std::int64_t i = 0; i < NumChannels_; i++) {
                        for (std::int64_t j = 0; j < uFrames; j++) {
                            if (j%scale != 0) {
                                double interpolated = 0;
                                for (std::int64_t k = -50000; k < 50001; k++) {
                                    std::int64_t idx = j+k;
                                    interpolated += sincLUT.at(k+50000) * ((idx < 0 || idx >= uFrames) ? 0 : bufferCache.channels[i][idx]);
                                }
                                tempBuffer.channels[i][j] = static_cast<T>(std::clamp(std::round(interpolated), clampMin, clampMax));
                            }
                            if (j%10000 == 0) {
                                std::cout << j << "/" << uFrames << " [CH: " << i+1 << "]" << std::endl;
                            }
                        }
                    }
                    break;
                }
                case BitType::F32 : case BitType::F64 : {
                    for (std::int64_t i = 0; i < NumChannels_; i++) {
                        for (std::int64_t j = 0; j < uFrames; j++) {
                            if (j%scale != 0) {
                                double interpolated = 0;
                                for (std::int64_t k = -50000; k < 50001; k++) {
                                    std::int64_t idx = j+k;
                                    interpolated += sincLUT.at(k+50000) * ((idx < 0 || idx >= uFrames) ? 0 : bufferCache.channels[i][idx]);
                                }
                                tempBuffer.channels[i][j] = static_cast<T>(std::clamp(interpolated, clampMin, clampMax));
                            }
                            if (j%10000 == 0) {
                                std::cout << j << "/" << uFrames << " [CH: " << i+1 << "]" << std::endl;
                            }
                        }
                    }
                    break;
                }
            }
            break;
        }
    }



    buffer.resize(NumChannels_, uFrames);
    buffer = tempBuffer;
    tempBuffer.clear();
}




void sk::SineKit::toSampleRate(SampleRate sampleRate)
{
    if (sampleRate == SampleRate_)
        return;

    const std::uint32_t dst = static_cast<std::uint32_t>(sampleRate);
    const std::uint32_t src = static_cast<std::uint32_t>(SampleRate_);

    if (src == 0 || dst == 0)
        throw std::runtime_error("unsupported or undefined sample rate");

    /* At the moment we only support *up‑sampling* by an *integer* factor.
       Down‑sampling or non‑integer ratios will be added later. */
    if (dst < src || (dst % src) != 0)
        throw std::runtime_error("non‑integer or down‑sampling ratios not yet implemented");

    const std::uint8_t scale = static_cast<std::uint8_t>(dst / src);
    if (scale == 1)
        return;   // should never happen, but guard anyway.

    /* Upsample the active buffer in-place.  We only need to choose the buffer
       based on the current BitType_.  This removes a large amount of duplicated
       code and also means every new sample‑rate that is an integer multiple of
       the current one “just works.” */
    switch (BitType_) {
        case BitType::I8:   upsample(scale, 5, Buffer8I_,  BitType_); break;
        case BitType::I16:  upsample(scale, 5, Buffer16I_, BitType_); break;
        case BitType::I24:  upsample(scale, 5, Buffer24I_, BitType_); break;
        case BitType::F32:  upsample(scale, 5, Buffer32F_, BitType_); break;
        case BitType::F64:  upsample(scale, 5, Buffer64F_, BitType_); break;
        default:
            throw std::runtime_error("unsupported bit depth for resampling");
    }

    NumFrames_ *= scale;
    SampleRate_  = sampleRate;
    updateHeaders();
}

