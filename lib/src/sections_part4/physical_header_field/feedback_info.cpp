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
    dectnrp_assert(MCS_out_of_range <= mcs && mcs <= 11, "MCS undefined");
    return static_cast<uint32_t>(mcs + 1);
}

int32_t feedback_info_t::cqi_2_mcs(const uint32_t cqi) {
    // even when PHY has a correct CRC, it can happen that the CQI is out-of-range
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

void feedback_info_f1_t::zero() {
    HARQ_Process_number = 0;
    Transmission_feedback = transmission_feedback_t::not_defined;
    Buffer_Size = 0;
    MCS = MCS_out_of_range;
}

void feedback_info_f1_t::pack(uint32_t& feedback_info) const {
    feedback_info = HARQ_Process_number << 9;
    feedback_info |= std::to_underlying(Transmission_feedback) << 8;
    feedback_info |= buffer_size_2_buffer_status(Buffer_Size) << 4;
    feedback_info |= mcs_2_cqi(MCS);
}

void feedback_info_f1_t::unpack(const uint32_t feedback_info) {
    HARQ_Process_number = (feedback_info >> 9) & 0b111;
    Transmission_feedback = static_cast<transmission_feedback_t>((feedback_info >> 8) & 0b1);
    Buffer_Size = buffer_status_2_buffer_size_lower[(feedback_info >> 4) & 0b1111];
    MCS = cqi_2_mcs(feedback_info & 0b1111);
}

void feedback_info_f2_t::zero() {
    Codebook_index = 0;
    MIMO_feedback = mimo_feedback_t::not_defined;
    Buffer_Size = 0;
    MCS = MCS_out_of_range;
}

void feedback_info_f2_t::pack(uint32_t& feedback_info) const {
    feedback_info = Codebook_index << 9;

    dectnrp_assert(std::to_underlying(MIMO_feedback) <= 1,
                   "feedback_info format supports only one bit MIMO feedback");

    feedback_info |= std::to_underlying(MIMO_feedback) << 8;
    feedback_info |= buffer_size_2_buffer_status(Buffer_Size) << 4;
    feedback_info |= mcs_2_cqi(MCS);
}

void feedback_info_f2_t::unpack(const uint32_t feedback_info) {
    Codebook_index = (feedback_info >> 9) & 0b111;
    MIMO_feedback = static_cast<mimo_feedback_t>((feedback_info >> 8) & 0b1);
    Buffer_Size = buffer_status_2_buffer_size_lower[(feedback_info >> 4) & 0b1111];
    MCS = cqi_2_mcs(feedback_info & 0b1111);
}

void feedback_info_f3_t::zero() {
    HARQ_Process_number_0 = 0;
    Transmission_feedback_0 = transmission_feedback_t::not_defined;
    HARQ_Process_number_1 = 0;
    Transmission_feedback_1 = transmission_feedback_t::not_defined;
    MCS = MCS_out_of_range;
}

void feedback_info_f3_t::pack(uint32_t& feedback_info) const {
    feedback_info = HARQ_Process_number_0 << 9;
    feedback_info |= std::to_underlying(Transmission_feedback_0) << 8;
    feedback_info |= HARQ_Process_number_1 << 5;
    feedback_info |= std::to_underlying(Transmission_feedback_1) << 4;
    feedback_info |= mcs_2_cqi(MCS);
}

void feedback_info_f3_t::unpack(const uint32_t feedback_info) {
    HARQ_Process_number_0 = (feedback_info >> 9) & 0b111;
    Transmission_feedback_0 = static_cast<transmission_feedback_t>((feedback_info >> 8) & 0b1);
    HARQ_Process_number_1 = (feedback_info >> 5) & 0b111;
    Transmission_feedback_1 = static_cast<transmission_feedback_t>((feedback_info >> 4) & 0b1);
    MCS = cqi_2_mcs(feedback_info & 0b1111);
}

void feedback_info_f4_t::zero() {
    HARQ_feedback_bitmap = 0;
    MCS = MCS_out_of_range;
}

void feedback_info_f4_t::pack(uint32_t& feedback_info) const {
    feedback_info = HARQ_feedback_bitmap << 4;
    feedback_info |= mcs_2_cqi(MCS);
}

void feedback_info_f4_t::unpack(const uint32_t feedback_info) {
    HARQ_feedback_bitmap = (feedback_info >> 4) & common::adt::bitmask<uint8_t, 8, 0>();
    MCS = cqi_2_mcs(feedback_info & 0b1111);
}

void feedback_info_f5_t::zero() {
    HARQ_Process_number = 0;
    Transmission_feedback = transmission_feedback_t::not_defined;
    MIMO_feedback = mimo_feedback_t::not_defined;
    Codebook_index = 0;
}

void feedback_info_f5_t::pack(uint32_t& feedback_info) const {
    feedback_info = (HARQ_Process_number << 9);
    feedback_info |= std::to_underlying(Transmission_feedback) << 8;
    feedback_info |= std::to_underlying(MIMO_feedback) << 6;
    feedback_info |= Codebook_index;
}

void feedback_info_f5_t::unpack(const uint32_t feedback_info) {
    HARQ_Process_number = (feedback_info >> 9) & 0b111;
    Transmission_feedback = static_cast<transmission_feedback_t>((feedback_info >> 8) & 0b1);
    MIMO_feedback = static_cast<mimo_feedback_t>((feedback_info >> 6) & 0b11);
    Codebook_index = feedback_info & 0b111111;
}

void feedback_info_pool_t::pack(const uint32_t feedback_format, uint32_t& feedback_info) {
    switch (feedback_format) {
        case feedback_info_t::No_feedback:
            feedback_info = 0;
            break;
        case 1:
            feedback_info_f1.pack(feedback_info);
            break;
        case 2:
            feedback_info_f2.pack(feedback_info);
            break;
        case 3:
            feedback_info_f3.pack(feedback_info);
            break;
        case 4:
            feedback_info_f4.pack(feedback_info);
            break;
        case 5:
            feedback_info_f5.pack(feedback_info);
            break;
        default:
            dectnrp_assert_failure("unknown feedback_format");
            break;
    }
}

bool feedback_info_pool_t::unpack(const uint32_t feedback_format, const uint32_t feedback_info) {
    switch (feedback_format) {
        case 1:
            feedback_info_f1.unpack(feedback_info);
            return true;
        case 2:
            feedback_info_f2.unpack(feedback_info);
            return true;
        case 3:
            feedback_info_f3.unpack(feedback_info);
            return true;
        case 4:
            feedback_info_f4.unpack(feedback_info);
            return true;
        case 5:
            feedback_info_f5.unpack(feedback_info);
            return true;
        default:
            return false;
    }
}

}  // namespace dectnrp::section4
