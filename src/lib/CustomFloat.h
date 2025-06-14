//
// Created by Hector van der Aa on 6/13/25.
//

#pragma once
#include <cstdint>
#include <cmath>
#include <bit>
#include <algorithm>   // for std::fill
#include <ostream>
#include <iomanip>
#include <type_traits>

struct Float80 {
private:
    std::uint8_t bits[10] = {0,0,0,0,0,0,0,0,0,0};
public:
    Float80() {
        std::fill(std::begin(bits), std::end(bits), 0);
    }
    Float80 operator+(const Float80& other) const noexcept {
        bool signA = bits[0] & 0x80;
        std::uint16_t expA = ((bits[0] & 0x7F) << 8) | bits[1];
        std::uint64_t mantissaA = 0;
        for (int i = 2; i < 10; ++i) {
            mantissaA = (mantissaA << 8) | bits[i];
        }
        mantissaA |= 1ULL << 63;

        bool signB = other.bits[0] & 0x80;
        std::uint16_t expB = ((other.bits[0] & 0x7F) << 8) | other.bits[1];
        std::uint64_t mantissaB = 0;
        for (int i = 2; i < 10; ++i) {
            mantissaB = (mantissaB << 8) | other.bits[i];
        }
        mantissaB |= 1ULL << 63;

        std::uint16_t exponentResult;
        if (expA >= expB) {
            std::uint16_t expDiff = expA - expB;
            mantissaB >>= expDiff;
            exponentResult = expA;
        } else {
            std::uint16_t expDiff = expB - expA;
            mantissaA >>= expDiff;
            exponentResult = expB;
        }

        std::uint64_t resultMantissa;
        bool resultSign;

        if (signA == signB) {
            resultMantissa = mantissaA + mantissaB;
            resultSign = signA;
        } else {
            if (mantissaA > mantissaB) {
                resultMantissa = mantissaA - mantissaB;
                resultSign = signA;
            } else {
                resultMantissa = mantissaB - mantissaA;
                resultSign = signB;
            }
        }

        while ((resultMantissa & (1ULL << 63)) == 0 && resultMantissa != 0) {
            resultMantissa <<= 1;
            exponentResult--;
        }

        Float80 result;

        result.bits[0] = (resultSign << 7) | ((exponentResult >> 8) & 0x7F);
        result.bits[1] = exponentResult & 0xFF;

        for (int i = 9; i >= 2; --i) {
            result.bits[i] = static_cast<uint8_t>(resultMantissa & 0xFF);
            resultMantissa >>= 8;
        }

        return result;
    }

    Float80 operator-(const Float80& other) const noexcept {
        Float80 neg = other;
        neg.bits[0] ^= 0x80;
        return *this + neg;
    }

    explicit operator long double() const noexcept {
        bool sign = bits[0] & 0x80;
        uint16_t exponent = ((bits[0] & 0x7F) << 8) | bits[1];
        uint64_t mantissa = 0;
        for (int i = 2; i < 10; ++i)
            mantissa = (mantissa << 8) | bits[i];

        std::int64_t realExp = exponent - 16383;

        static constexpr long double kTwo63 = 0x1p63L;   // 2^63 as long double
        long double frac = static_cast<long double>(mantissa) / kTwo63;
        long double result = std::ldexp(frac, realExp);
        return sign ? -result : result;
    }

    // Generic conversion to *any* arithmetic type via long double
    template<typename T,
             typename = std::enable_if_t<
                 std::is_arithmetic_v<T> && !std::is_same_v<T, long double>>>
    explicit operator T() const noexcept {
        return static_cast<T>(static_cast<long double>(*this));
    }

    explicit Float80(long double value) noexcept {
        bool sign = std::signbit(value);
        if (sign) value = -value;

        int exp;
        long double frac = std::frexp(value, &exp); // 0.5 ≤ frac < 1
        // ── Canonicalise for IEEE‑754 extended‑precision ───────────────
        exp   -= 1;    // compensate for the ×2 below
        frac  *= 2.0L; // now 1 ≤ frac < 2 so the explicit integer‑bit is 1

        static constexpr long double kTwo63 = 0x1p63L;
        // Scale to a 64‑bit significand (integer‑bit + 63 fraction bits)
        std::uint64_t mantissa = static_cast<std::uint64_t>(frac * kTwo63);

        // Rare case: rounding made mantissa wrap to 0 (i.e. 2^64).  Re‑normalise.
        if (mantissa == 0) {
            mantissa = 0x8000000000000000ULL; // 1 << 63
            ++exp;                            // bump exponent back up
        }

        std::uint16_t biasedExp = static_cast<std::uint16_t>(exp + 16383);

        bits[0] = (sign << 7) | ((biasedExp >> 8) & 0x7F);
        bits[1] = biasedExp & 0xFF;

        for (int i = 9; i >= 2; --i) {
            bits[i] = static_cast<uint8_t>(mantissa & 0xFF);
            mantissa >>= 8;
        }
    }


    // Assignment from any arithmetic type (e.g., int, double, long double)
    template<typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
    Float80& operator=(T value) {
        Float80 tmp(static_cast<long double>(value));
        std::copy(std::begin(tmp.bits), std::end(tmp.bits), std::begin(bits));
        return *this;
    }

    // Stream output: prints the numeric value followed by raw hex bytes
    friend std::ostream& operator<<(std::ostream& os, const Float80& f) {
        // save stream state
        std::ios_base::fmtflags old_flags = os.flags();
        char old_fill = os.fill();

        // numeric value
        os << static_cast<long double>(f) << " (0x";

        // raw bytes in big‑endian order
        os << std::hex << std::uppercase << std::setfill('0');
        for (int i = 0; i < 10; ++i) {
            os << std::setw(2) << static_cast<int>(f.bits[i]);
        }

        // restore stream state
        os.flags(old_flags);
        os.fill(old_fill);
        os << ')';
        return os;
    }
};

static_assert(sizeof(Float80) == 10, "Float80 must be exactly 10 bytes.");
