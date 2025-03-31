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

#include "dectnrp/sections_part4/physical_header_field/plcf_decoder.hpp"

#include "dectnrp/common/adt/miscellaneous.hpp"
#include "dectnrp/common/prog/assert.hpp"

namespace dectnrp::section4 {

plcf_decoder_t::plcf_decoder_t(uint32_t PacketLength_max_,
                               uint32_t mcs_index_max_,
                               uint32_t N_SS_max_)
    : PacketLength_max(PacketLength_max_),
      mcs_index_max(mcs_index_max_),
      N_SS_max(N_SS_max_) {
    set_configuration();
}

void plcf_decoder_t::set_configuration() {
    HeaderFormat_type1 = common::adt::UNDEFINED_NUMERIC_32;
    HeaderFormat_type2 = common::adt::UNDEFINED_NUMERIC_32;
}

section3::fec_cfg_t& plcf_decoder_t::get_fec_cfg(const uint32_t PLCF_type) {
    dectnrp_assert(PLCF_type == 1 || PLCF_type == 2, "unknown PLCF type");
    return PLCF_type == 1 ? fec_cfg_type1 : fec_cfg_type2;
}

void plcf_decoder_t::decode_and_rdc_check(const uint32_t PLCF_type, const uint8_t* plcf_front) {
    // extract header format
    const uint32_t HeaderFormat_candidate = (plcf_front[0] >> 5) & 0b111;

    // lambda to check limits of radio device class (rdc)
    auto check_rdc_limits = [this](plcf_base_t* plcf_base, const uint8_t* plcf_front_) -> bool {
        *plcf_base << plcf_front_;

        // check radio class limits
        if (plcf_base->get_PacketLength() > PacketLength_max || plcf_base->DFMCS > mcs_index_max ||
            plcf_base->get_N_SS() > N_SS_max) {
            return false;
        }

        return true;
    };

    dectnrp_assert(PLCF_type == 1 || PLCF_type == 2, "unknown PLCF type");

    switch (PLCF_type) {
        case 1:
            dectnrp_assert(HeaderFormat_type1 == common::adt::UNDEFINED_NUMERIC_32,
                           "Header format already defined. Reset called?");

            // validity condition 0: PLCF must be implemented, but this is a non-fatal error as the
            // bits can be flipped despite correct CRC
            if (HeaderFormat_candidate >= array_plcf_type1.size()) {
                return;
            }

            // validity condition 1: PLCF fields must stay within radio device class
            if (!check_rdc_limits(array_plcf_type1.at(HeaderFormat_candidate), plcf_front)) {
                return;
            }

            // save header format
            HeaderFormat_type1 = HeaderFormat_candidate;

            break;

        case 2:
            dectnrp_assert(HeaderFormat_type2 == common::adt::UNDEFINED_NUMERIC_32,
                           "Header format already defined. Reset called?");

            // validity condition 0: PLCF must be implemented, but this is a non-fatal error as the
            // bits can be flipped despite correct CRC
            if (HeaderFormat_candidate >= array_plcf_type2.size()) {
                return;
            }

            // validity condition 2: PLCF fields must stay within radio device class
            if (!check_rdc_limits(array_plcf_type2.at(HeaderFormat_candidate), plcf_front)) {
                return;
            }

            // save header format
            HeaderFormat_type2 = HeaderFormat_candidate;

            break;
    }
}

uint32_t plcf_decoder_t::has_any_plcf() const {
    uint32_t ret = 0;

    if (HeaderFormat_type1 != common::adt::UNDEFINED_NUMERIC_32) {
        ret += 1;
    }

    if (HeaderFormat_type2 != common::adt::UNDEFINED_NUMERIC_32) {
        ret += 2;
    }

    return ret;
}

const plcf_base_t* plcf_decoder_t::get_plcf_base(const uint32_t PLCF_type) const {
    dectnrp_assert(PLCF_type == 1 || PLCF_type == 2, "unknown PLCF type");

    switch (PLCF_type) {
        case 1:
            if (HeaderFormat_type1 != common::adt::UNDEFINED_NUMERIC_32) {
                dectnrp_assert(HeaderFormat_type1 ==
                                   array_plcf_type1.at(HeaderFormat_type1)->get_HeaderFormat(),
                               "not the same header format");

                return array_plcf_type1.at(HeaderFormat_type1);
            }
            break;

        case 2:
            if (HeaderFormat_type2 != common::adt::UNDEFINED_NUMERIC_32) {
                dectnrp_assert(HeaderFormat_type2 ==
                                   array_plcf_type2.at(HeaderFormat_type2)->get_HeaderFormat(),
                               "not the same header format");

                return array_plcf_type2.at(HeaderFormat_type2);
            }
    }

    return nullptr;
}

uint32_t plcf_decoder_t::get_headerformat(const uint32_t PLCF_type) const {
    dectnrp_assert(PLCF_type == 1 || PLCF_type == 2, "unknown PLCF type");
    return PLCF_type == 1 ? HeaderFormat_type1 : HeaderFormat_type2;
}

}  // namespace dectnrp::section4
