#pragma once
#include <bit>
#include <bitset>
#include <cstdint>
#include <concepts>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <ostream>

namespace sk::endian {

// ── Host endianness at compile time ───────────────────────────────────────
inline constexpr bool kHostIsLE =
    (std::endian::native == std::endian::little);

enum class Endian { Little, Big };

// ── Concept: any 1/2/4/8‑byte integral **or** floating‑point type ─────────
template<typename T>
concept IntWord =
    (std::integral<T> || std::floating_point<T>) &&
    (sizeof(T) == 1 || sizeof(T) == 2 || sizeof(T) == 4 || sizeof(T) == 8);

// ── byteswap — handles both ints and IEEE floats via std::bit_cast ───────
template<IntWord T>
[[nodiscard]] constexpr T byteswap(T v) noexcept
{
    if constexpr (sizeof(T) == 1)            // nothing to do
        return v;

    if constexpr (std::integral<T>)
        return std::byteswap(v);             // C++23 intrinsic

    else if constexpr (sizeof(T) == 4) {     // float → uint32 → swap
        auto tmp = std::byteswap(std::bit_cast<std::uint32_t>(v));
        return std::bit_cast<T>(tmp);
    }
    else {                                   // 8‑byte: double / int64
        static_assert(sizeof(T) == 8);
        auto tmp = std::byteswap(std::bit_cast<std::uint64_t>(v));
        return std::bit_cast<T>(tmp);
    }
}

// ── Convert from file‑order LE / BE to host‑order ─────────────────────────
template<IntWord T>
[[nodiscard]] constexpr T le_to_host(T v) noexcept
{
    return kHostIsLE ? v : byteswap(v);
}

template<IntWord T>
[[nodiscard]] constexpr T be_to_host(T v) noexcept
{
    return kHostIsLE ? byteswap(v) : v;
}

// ── Convert from host‑order to file‑order LE / BE ─────────────────────────
template<IntWord T>
[[nodiscard]] constexpr T host_to_le(T v) noexcept
{
    return kHostIsLE ? v : byteswap(v);
}

template<IntWord T>
[[nodiscard]] constexpr T host_to_be(T v) noexcept
{
    return kHostIsLE ? byteswap(v) : v;
}

// ── Streaming helpers (UNCHANGED SIGNATURES) ─────────────────────────────
//               write N‑byte integer/float in given endianness
template<IntWord T>
inline void write_le(std::ofstream& os, T v)
{
    v = host_to_le(v);
    os.write(reinterpret_cast<const char*>(&v), sizeof v);
}
template<IntWord T>
inline void write_be(std::ofstream& os, T v)
{
    v = host_to_be(v);
    os.write(reinterpret_cast<const char*>(&v), sizeof v);
}

//               read N‑byte integer/float in given endianness
template<IntWord T>
[[nodiscard]] inline T read_le(std::ifstream& is)
{
    T v{};
    is.read(reinterpret_cast<char*>(&v), sizeof v);
    return le_to_host(v);
}
template<IntWord T>
[[nodiscard]] inline T read_be(std::ifstream& is)
{
    T v{};
    is.read(reinterpret_cast<char*>(&v), sizeof v);
    return be_to_host(v);
}

// ── Adapters for quick per‑field use in structs ───────────────────────────
template<IntWord T>
[[nodiscard]] constexpr T swap_if_needed(T raw, Endian fileEndian) noexcept
{
    if constexpr (sizeof(T) == 1) return raw;        // never swap single byte
    return (fileEndian == Endian::Little) ? le_to_host(raw)
                                          : be_to_host(raw);
}
template<IntWord T>
[[nodiscard]] constexpr T host_to_file(T v, Endian fileEndian) noexcept
{
    return (fileEndian == Endian::Little) ? host_to_le(v)
                                          : host_to_be(v);
}

} // namespace sk::endian
