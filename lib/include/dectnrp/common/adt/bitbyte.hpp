/*
 * Copyright 2023-2024 Maxim Penner, Alexander Poets
 * Copyright 2025-2025 Maxim Penner
 *
 * This file is part of DECTNRP.
 *
 * DECTNRP is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * DECTNRP is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Affero General Public License for more details.
 *
 * A copy of the GNU Affero General Public License can be found in
 * the LICENSE file in the top-level directory of this distribution
 * and at http://www.gnu.org/licenses/.
 */

#pragma once

#include <concepts>
#include <cstdint>
#include <cstring>

namespace dectnrp::common::adt {

/// creates a bitmask and sets the bits in the range [lsb, msb)
template <std::unsigned_integral T, std::size_t msb, std::size_t lsb>
    requires(msb >= lsb && msb <= sizeof(T) * 8)
static constexpr T bitmask() {
    T ret{0};
    for (size_t i = lsb; i < msb; ++i) {
        ret |= 1 << i;
    }
    return ret;
}

/// creates a bitmask and sets the n least significant bits
template <std::size_t n, typename T = uint32_t>
static constexpr T bitmask_lsb() {
    return bitmask<T, n, 0>();
}

/// creates a bitmask and sets the n most significant bits
template <std::size_t n, typename T = uint32_t>
static constexpr T bitmask_msb() {
    return bitmask<T, sizeof(T) * 8, sizeof(T) * 8 - n>();
}

/**
 * \brief Conversion from little to big endian and copy of lower bytes. Assume we have an uint32_t
 * xyz=0x0A0B0C0D. On a little endian machine it is saved as:
 *
 *  0D              0C              0B              0A
 *
 *  address n       address n+1     address n+2     address n+3
 *
 * The least significant byte is stored at the lowest byte address. A pointer uint32_t* ptr32=&xyz
 * would point to address n, same is true for a pointer uint8_t* ptr8=(uint8_t*)&xyz. The function
 * l2b_lower() internally first inverts the byte order to ...
 *
 *  0A              0B              0C              0D
 *
 *  address n       address n+1     address n+2     address n+3
 *
 * ... and then copies the "n_byte" on the right. For instance, if n_byte=3, dst would ultimately
 * become 0B0C0D.
 *
 * \param dst
 * \param src
 * \param n_byte
 */
template <std::unsigned_integral T>
inline void l2b_lower(uint8_t* dst, const T src, const size_t n_byte) {
    T src_swapped{0};

    if constexpr (std::is_same_v<T, uint32_t>) {
        src_swapped = __builtin_bswap32(src);
    } else {
        static_assert(std::is_same_v<T, uint64_t>, "unsupported unsigned integral");
        src_swapped = __builtin_bswap64(src);
    }

    std::memcpy(dst, (uint8_t*)&src_swapped + sizeof(T) - n_byte, n_byte);
}

/// reverses the operation l2b_lower() and makes sure that undefined bytes are set to zero
template <std::unsigned_integral T = uint32_t>
inline T b2l_lower(const uint8_t* src, const size_t n_byte) {
    T dst_swapped = 0;

    std::memcpy((uint8_t*)&dst_swapped + sizeof(T) - n_byte, src, n_byte);

    if constexpr (std::is_same_v<T, uint32_t>) {
        return __builtin_bswap32(dst_swapped);
    } else {
        static_assert(std::is_same_v<T, uint64_t>, "unsupported unsigned integral");
        return __builtin_bswap64(dst_swapped);
    }
}

/**
 * \brief Conversion from little to big endian and copy of upper bytes. Assume we have an uint32_t
 * xyz=0x0A0B0C0D. On a little endian machine it is saved as:
 *
 *  0D              0C              0B              0A
 *
 *  address n       address n+1     address n+2     address n+3
 *
 * The least significant byte is stored at the lowest byte address. A pointer uint32_t* ptr32=&xyz
 * would point to address n, same is true for a pointer uint8_t* ptr8=(uint8_t*)&xyz. The function
 * l2b_upper() internally first inverts the byte order to ...
 *
 *  0A              0B              0C              0D
 *
 *  address n       address n+1     address n+2     address n+3
 *
 * ... and then copies the "n_byte" on the left. For instance, if n_byte=3, dst would ultimately
 * become 0A0B0C.
 *
 * \param dst
 * \param src
 * \param n_byte
 */
template <std::unsigned_integral T>
inline void l2b_upper(uint8_t* dst, const T src, const size_t n_byte) {
    T src_swapped{0};

    if constexpr (std::is_same_v<T, uint32_t>) {
        src_swapped = __builtin_bswap32(src);
    } else {
        static_assert(std::is_same_v<T, uint64_t>, "unsupported unsigned integral");
        src_swapped = __builtin_bswap64(src);
    }

    std::memcpy(dst, (uint8_t*)&src_swapped, n_byte);
}

/// reverses the operation l2b_upper() and makes sure that undefined bytes are set to zero
template <std::unsigned_integral T = uint32_t>
inline T b2l_upper(const uint8_t* src, const size_t n_byte) {
    T dst_swapped = 0;

    std::memcpy((uint8_t*)&dst_swapped, src, n_byte);

    if constexpr (std::is_same_v<T, uint32_t>) {
        return __builtin_bswap32(dst_swapped);
    } else {
        static_assert(std::is_same_v<T, uint64_t>, "unsupported unsigned integral");
        return __builtin_bswap64(dst_swapped);
    }
}

}  // namespace dectnrp::common::adt
