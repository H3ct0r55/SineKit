// Extended80.h  (new header)
#pragma once
#include <array>
#include <cstdint>
#include <bit>
#include <limits>
#include <cmath>

namespace sk
{
    /** In-memory representation of the 80-bit IEEE-754 “extended” format.
     *
     *  * 16-bit sign|exponent word followed by a 64-bit fraction.
     *  * Both sub-words live in **host endianness** so the struct is
     *    trivially copyable and you can memcmp / memset it.
     *  * Size is guaranteed to be 10 bytes (checked at compile time).
     *
     *  The type is deliberately *dumb*: it owns only the raw bits plus
     *  <-> conversion helpers.  All stream I/O is supplied separately
     *  (see §3 below) so that `EndianHelpers.h` keeps ownership of the
     *  “write_be / write_le” interface.
     */
    struct Extended80
    {
        std::array<std::uint8_t, 10> raw{}; ///< big‑endian 80‑bit value (sign|exp + 64‑bit frac)

        /// Return the sign‑exponent word in host endianness.
        std::uint16_t se() const noexcept
        {
            return (static_cast<std::uint16_t>(raw[0]) << 8) | raw[1];
        }

        /// Return the 63‑bit fraction in host endianness.
        std::uint64_t frac() const noexcept
        {
            std::uint64_t f = 0;
            for (int i = 2; i < 10; ++i)
                f = (f << 8) | raw[i];
            return f;
        }

        /* ---------- conversion helpers ---------- */

        /// Create from any floating-point value (double is plenty here)
        template <typename FP,
                  std::enable_if_t<std::is_floating_point_v<FP>, int> = 0>
        static Extended80 from(FP val) noexcept
        {
            Extended80 out;
            if (val == 0.0)
                return out;                      // all‑zero already OK

            int sign = std::signbit(val);
            FP absVal = std::fabs(val);

            int exp2;                            // base‑2 exponent
            FP mant = std::frexp(absVal, &exp2); // returns 0.5 ≤ mant < 1

            // Shift mantissa into [1,2) so that bit63 is the MSB
            mant *= 2.0;
            exp2  -= 1;

            // 64‑bit integer fraction with *explicit* integer bit
            //   mant is now in [1,2), so its top bit becomes bit‑63.
            std::uint64_t f =
                static_cast<std::uint64_t>(std::ldexp(mant, 63) + 0.5); // round‑to‑nearest

            std::uint16_t exp = static_cast<std::uint16_t>(exp2 + 16383);
            std::uint16_t se  = static_cast<std::uint16_t>((sign << 15) | exp);

            // Store in big‑endian order
            out.raw[0] = static_cast<std::uint8_t>(se >> 8);
            out.raw[1] = static_cast<std::uint8_t>(se & 0xFF);
            for (int i = 0; i < 8; ++i)
                out.raw[2 + i] = static_cast<std::uint8_t>(f >> (56 - 8 * i));

            return out;
        }

        /// Convert back to double
        double to_double() const noexcept
        {
            std::uint16_t se_be = se();
            std::uint64_t f     = frac();

            if (se_be == 0 && f == 0) return 0.0;

            bool sign = se_be & 0x8000u;
            int  exp  = static_cast<int>(se_be & 0x7FFFu) - 16383;

            double mant =
                static_cast<double>(f) /
                static_cast<double>(UINT64_C(1) << 63);

            double val = std::ldexp(mant, exp);
            return sign ? -val : val;
        }
    };

    static_assert(sizeof(Extended80) == 10, "Extended80 must be 80 bits (10 bytes)");
}