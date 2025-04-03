/*
 * Copyright 2023-2025 Maxim Penner
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

#include "dectnrp/sections_part4/physical_header_field/feedback_info.hpp"

#include <string>
#include <utility>

#include "dectnrp/common/adt/bitbyte.hpp"
#include "dectnrp/common/prog/assert.hpp"

namespace dectnrp::section4 {

uint32_t feedback_info_t::mcs_2_cqi(const int32_t mcs) {
    dectnrp_assert(0 <= mcs, "MCS undefined");
    dectnrp_assert(mcs <= 11, "MCS undefined");

    return static_cast<uint32_t>(mcs + 1);
}

int32_t feedback_info_t::cqi_2_mcs(const uint32_t cqi) {
    if (cqi == 0 || 12 < cqi) {
        return MCS_out_of_range;
    }

    return static_cast<int32_t>(cqi) - 1;
}

uint32_t feedback_info_t::buffer_size_2_buffer_status(const uint32_t buffer_size) {
    if (buffer_size == 0) {
        return 0;
    } else if (0 < buffer_size && buffer_size <= 16) {
        return 1;
    } else if (16 < buffer_size && buffer_size <= 32) {
        return 2;
    } else if (32 < buffer_size && buffer_size <= 64) {
        return 3;
    } else if (64 < buffer_size && buffer_size <= 128) {
        return 4;
    } else if (128 < buffer_size && buffer_size <= 256) {
        return 5;
    } else if (256 < buffer_size && buffer_size <= 512) {
        return 6;
    } else if (512 < buffer_size && buffer_size <= 1024) {
        return 7;
    } else if (1024 < buffer_size && buffer_size <= 2048) {
        return 8;
    } else if (2048 < buffer_size && buffer_size <= 4096) {
        return 9;
    } else if (4096 < buffer_size && buffer_size <= 8192) {
        return 10;
    } else if (8192 < buffer_size && buffer_size <= 16384) {
        return 11;
    } else if (16384 < buffer_size && buffer_size <= 32768) {
        return 12;
    } else if (32768 < buffer_size && buffer_size <= 65536) {
        return 13;
    } else if (65536 < buffer_size && buffer_size <= 131072) {
        return 14;
    }

    // 131072 < buffer_size
    return 15;
}

uint32_t feedback_info_t::get_packed_size() const {
    dectnrp_assert_failure("undefined for feedback info");

    // size in bits
    return 12;
}

void feedback_info_f1_t::zero() {
    HARQ_Process_number = 0;
    Transmission_feedback = transmission_feedback_t::not_defined;
    Buffer_Size = 0;
    MCS = MCS_out_of_range;
}

bool feedback_info_f1_t::is_valid() const {
    if (common::adt::bitmask_lsb<3>() < HARQ_Process_number) {
        return false;
    }

    if (Transmission_feedback == transmission_feedback_t::not_defined) {
        return false;
    }

    if (common::adt::bitmask_lsb<4>() < buffer_size_2_buffer_status(Buffer_Size)) {
        return false;
    }

    if (common::adt::bitmask_lsb<4>() < mcs_2_cqi(MCS)) {
        return false;
    }

    return true;
}

void feedback_info_f1_t::pack(uint8_t* a_ptr) const {
    dectnrp_assert(is_valid(), "invalid");

    a_ptr[0] &= common::adt::bitmask<uint8_t, 8, 4>();

    a_ptr[0] |= HARQ_Process_number << 1;
    a_ptr[0] |= std::to_underlying(Transmission_feedback);
    a_ptr[1] = buffer_size_2_buffer_status(Buffer_Size) << 4;
    a_ptr[1] |= mcs_2_cqi(MCS);
}

bool feedback_info_f1_t::unpack(const uint8_t* a_ptr) {
    HARQ_Process_number = (a_ptr[0] >> 1) & 0b111;
    Transmission_feedback = static_cast<transmission_feedback_t>(a_ptr[0] & 0b1);
    Buffer_Size = buffer_status_2_buffer_size_lower[(a_ptr[1] >> 4) & 0b1111];
    MCS = cqi_2_mcs(a_ptr[1] & 0b1111);

    return is_valid();
}

void feedback_info_f2_t::zero() {
    Codebook_index = 0;
    MIMO_feedback = mimo_feedback_t::not_defined;
    Buffer_Size = 0;
    MCS = MCS_out_of_range;
}

bool feedback_info_f2_t::is_valid() const {
    if (common::adt::bitmask_lsb<3>() < Codebook_index) {
        return false;
    }

    if (!(MIMO_feedback == mimo_feedback_t::single_layer ||
          MIMO_feedback == mimo_feedback_t::dual_layer)) {
        return false;
    }

    if (common::adt::bitmask_lsb<4>() < buffer_size_2_buffer_status(Buffer_Size)) {
        return false;
    }

    if (common::adt::bitmask_lsb<4>() < mcs_2_cqi(MCS)) {
        return false;
    }

    return true;
}

void feedback_info_f2_t::pack(uint8_t* a_ptr) const {
    dectnrp_assert(is_valid(), "invalid");

    a_ptr[0] &= common::adt::bitmask<uint8_t, 8, 4>();

    a_ptr[0] = Codebook_index << 1;
    a_ptr[0] |= std::to_underlying(MIMO_feedback);
    a_ptr[1] = buffer_size_2_buffer_status(Buffer_Size) << 4;
    a_ptr[1] |= mcs_2_cqi(MCS);
}

bool feedback_info_f2_t::unpack(const uint8_t* a_ptr) {
    Codebook_index = (a_ptr[0] >> 1) & 0b111;
    MIMO_feedback = static_cast<mimo_feedback_t>(a_ptr[0] & 0b1);
    Buffer_Size = buffer_status_2_buffer_size_lower[(a_ptr[1] >> 4) & 0b1111];
    MCS = cqi_2_mcs(a_ptr[1] & 0b1111);

    return is_valid();
}

void feedback_info_f3_t::zero() {
    HARQ_Process_number_0 = 0;
    Transmission_feedback_0 = transmission_feedback_t::not_defined;
    HARQ_Process_number_1 = 0;
    Transmission_feedback_1 = transmission_feedback_t::not_defined;
    MCS = MCS_out_of_range;
}

bool feedback_info_f3_t::is_valid() const {
    if (common::adt::bitmask_lsb<3>() < HARQ_Process_number_0) {
        return false;
    }

    if (Transmission_feedback_0 == transmission_feedback_t::not_defined) {
        return false;
    }

    if (common::adt::bitmask_lsb<3>() < HARQ_Process_number_1) {
        return false;
    }

    if (Transmission_feedback_1 == transmission_feedback_t::not_defined) {
        return false;
    }

    if (common::adt::bitmask_lsb<4>() < mcs_2_cqi(MCS)) {
        return false;
    }

    return true;
}

void feedback_info_f3_t::pack(uint8_t* a_ptr) const {
    dectnrp_assert(is_valid(), "invalid");

    a_ptr[0] &= common::adt::bitmask<uint8_t, 8, 4>();

    a_ptr[0] |= HARQ_Process_number_0 << 1;
    a_ptr[0] |= std::to_underlying(Transmission_feedback_0);
    a_ptr[1] = HARQ_Process_number_1 << 5;
    a_ptr[1] |= std::to_underlying(Transmission_feedback_1) << 4;
    a_ptr[1] |= mcs_2_cqi(MCS);
}

bool feedback_info_f3_t::unpack(const uint8_t* a_ptr) {
    HARQ_Process_number_0 = (a_ptr[0] >> 1) & 0b111;
    Transmission_feedback_0 = static_cast<transmission_feedback_t>(a_ptr[0] & 0b1);
    HARQ_Process_number_1 = (a_ptr[1] >> 5) & 0b111;
    Transmission_feedback_1 = static_cast<transmission_feedback_t>((a_ptr[1] >> 4) & 0b1);
    MCS = cqi_2_mcs(a_ptr[1] & 0b1111);

    return is_valid();
}

void feedback_info_f4_t::zero() {
    HARQ_feedback_bitmap = 0;
    MCS = MCS_out_of_range;
}

bool feedback_info_f4_t::is_valid() const {
    if (common::adt::bitmask_lsb<8>() < HARQ_feedback_bitmap) {
        return false;
    }

    if (common::adt::bitmask_lsb<4>() < mcs_2_cqi(MCS)) {
        return false;
    }

    return true;
}

void feedback_info_f4_t::pack(uint8_t* a_ptr) const {
    dectnrp_assert(is_valid(), "invalid");

    a_ptr[0] &= common::adt::bitmask<uint8_t, 8, 4>();

    a_ptr[0] |= (HARQ_feedback_bitmap >> 4);
    a_ptr[1] = (HARQ_feedback_bitmap & 0b1111) << 4;
    a_ptr[1] |= mcs_2_cqi(MCS);
}

bool feedback_info_f4_t::unpack(const uint8_t* a_ptr) {
    HARQ_feedback_bitmap = a_ptr[0] & 0b1111;
    HARQ_feedback_bitmap = HARQ_feedback_bitmap << 4;
    HARQ_feedback_bitmap |= (a_ptr[1] >> 4) & 0b1111;
    MCS = cqi_2_mcs(a_ptr[1] & 0b1111);

    return is_valid();
}

void feedback_info_f5_t::zero() {
    HARQ_Process_number = 0;
    Transmission_feedback = transmission_feedback_t::not_defined;
    MIMO_feedback = mimo_feedback_t::not_defined;
    Codebook_index = 0;
}

bool feedback_info_f5_t::is_valid() const {
    if (common::adt::bitmask_lsb<3>() < HARQ_Process_number) {
        return false;
    }

    if (Transmission_feedback == transmission_feedback_t::not_defined) {
        return false;
    }

    if (MIMO_feedback == mimo_feedback_t::not_defined) {
        return false;
    }

    if (common::adt::bitmask_lsb<6>() < Codebook_index) {
        return false;
    }

    return true;
}

void feedback_info_f5_t::pack(uint8_t* a_ptr) const {
    dectnrp_assert(is_valid(), "invalid");

    a_ptr[0] &= common::adt::bitmask<uint8_t, 8, 4>();

    a_ptr[0] = (HARQ_Process_number << 1);
    a_ptr[0] |= std::to_underlying(Transmission_feedback);
    a_ptr[1] = std::to_underlying(MIMO_feedback) << 6;
    a_ptr[1] |= Codebook_index;
}

bool feedback_info_f5_t::unpack(const uint8_t* a_ptr) {
    HARQ_Process_number = (a_ptr[0] >> 1) & 0b111;
    Transmission_feedback = static_cast<transmission_feedback_t>(a_ptr[0] & 0b1);
    MIMO_feedback = static_cast<mimo_feedback_t>((a_ptr[1] >> 6) & 0b11);
    Codebook_index = a_ptr[1] & 0b111111;

    return is_valid();
}

void feedback_info_f6_t::zero() {
    HARQ_Process_number = 0;
    Reserved = 0;
    Buffer_Size = 0;
    MCS = MCS_out_of_range;
}

bool feedback_info_f6_t::is_valid() const {
    if (common::adt::bitmask_lsb<3>() < HARQ_Process_number) {
        return false;
    }

    if (Reserved != 0) {
        return false;
    }

    if (common::adt::bitmask_lsb<4>() < buffer_size_2_buffer_status(Buffer_Size)) {
        return false;
    }

    if (common::adt::bitmask_lsb<4>() < mcs_2_cqi(MCS)) {
        return false;
    }

    return true;
}

void feedback_info_f6_t::pack(uint8_t* a_ptr) const {
    dectnrp_assert(is_valid(), "invalid");

    a_ptr[0] &= common::adt::bitmask<uint8_t, 8, 4>();

    a_ptr[0] = (HARQ_Process_number << 1);
    a_ptr[0] |= Reserved;
    a_ptr[1] = buffer_size_2_buffer_status(Buffer_Size) << 4;
    a_ptr[1] |= mcs_2_cqi(MCS);
}

bool feedback_info_f6_t::unpack(const uint8_t* a_ptr) {
    HARQ_Process_number = (a_ptr[0] >> 1) & 0b111;
    Reserved = a_ptr[0] & 0b1;
    Buffer_Size = buffer_status_2_buffer_size_lower[(a_ptr[1] >> 4) & 0b1111];
    MCS = cqi_2_mcs(a_ptr[1] & 0b1111);

    return is_valid();
}

void feedback_info_pool_t::pack(const uint32_t feedback_format, uint8_t* a_ptr) const {
    switch (feedback_format) {
        case feedback_info_t::No_feedback:
            a_ptr[0] &= common::adt::bitmask<uint8_t, 8, 4>();
            a_ptr[1] = 0;
            break;
        case 1:
            feedback_info_f1.pack(a_ptr);
            break;
        case 2:
            feedback_info_f2.pack(a_ptr);
            break;
        case 3:
            feedback_info_f3.pack(a_ptr);
            break;
        case 4:
            feedback_info_f4.pack(a_ptr);
            break;
        case 5:
            feedback_info_f5.pack(a_ptr);
            break;
        case 6:
            feedback_info_f6.pack(a_ptr);
            break;
        default:
            dectnrp_assert_failure("unknown feedback_format");
            break;
    }
}

bool feedback_info_pool_t::unpack(const uint32_t feedback_format, const uint8_t* a_ptr) {
    switch (feedback_format) {
        case feedback_info_t::No_feedback:
            if ((a_ptr[0] & 0b1111) != 0) {
                return false;
            }

            if (a_ptr[1] != 0) {
                return false;
            }

            return true;
        case 1:
            feedback_info_f1.unpack(a_ptr);
            return true;
        case 2:
            feedback_info_f2.unpack(a_ptr);
            return true;
        case 3:
            feedback_info_f3.unpack(a_ptr);
            return true;
        case 4:
            feedback_info_f4.unpack(a_ptr);
            return true;
        case 5:
            feedback_info_f5.unpack(a_ptr);
            return true;
        case 6:
            feedback_info_f6.unpack(a_ptr);
            return true;
        default:
            return false;
    }
}

}  // namespace dectnrp::section4
