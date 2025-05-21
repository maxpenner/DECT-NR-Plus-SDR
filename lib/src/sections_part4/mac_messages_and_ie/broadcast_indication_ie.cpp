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

#include "dectnrp/sections_part4/mac_messages_and_ie/broadcast_indication_ie.hpp"

#include "dectnrp/common/adt/bitbyte.hpp"
#include "dectnrp/common/adt/enumeration.hpp"
#include "dectnrp/common/prog/assert.hpp"

namespace dectnrp::sp4 {

broadcast_indication_ie_t::broadcast_indication_ie_t() {
    mac_mux_header.zero();
    mac_mux_header.mac_ext = mac_multiplexing_header_t::mac_ext_t::No_Length_Field;
    mac_mux_header.ie_type.mac_ext_00_01_10 =
        mac_multiplexing_header_t::ie_type_mac_ext_00_01_10_t::Broadcast_Indication_IE;

    zero();

    dectnrp_assert(check_validity_at_runtime(this), "mmie invalid");
}

void broadcast_indication_ie_t::zero() {
    indication_type = indication_type_t::not_defined;
    id_type = rd_id_type_t::not_defined;
    ack_nack = transmission_feedback_t::not_defined;
    feedback = feedback_type_t::not_defined;
    resource_allocation_ie_follows = false;
    rd_id = common::adt::UNDEFINED_NUMERIC_32;
    mcs_mimo_feedback = std::monostate{};
}

bool broadcast_indication_ie_t::is_valid() const {
    if (!common::adt::is_valid(indication_type) || !common::adt::is_valid(id_type)) {
        return false;
    }

    if (id_type == rd_id_type_t::short_rd_id && rd_id > common::adt::bitmask_lsb<16>()) {
        return false;
    }

    if (indication_type == indication_type_t::random_access_response) {
        // for random access response the short RD-ID shall be used
        // ACK/NACK field is present when INDICATION TYPE is random access response
        if (id_type != rd_id_type_t::short_rd_id || !common::adt::is_valid(ack_nack)) {
            return false;
        }

        switch (feedback) {
            using enum feedback_type_t;

            case no_feedback:
                break;

            case mcs:
                if (auto mcs_feedback = std::get_if<channel_quality_t>(&mcs_mimo_feedback);
                    mcs_feedback != nullptr && common::adt::is_valid(*mcs_feedback)) {
                    break;
                }
                return false;

            case mimo_2_antennas:
                if (auto mimo_feedback = std::get_if<mimo_feedback_t>(&mcs_mimo_feedback);
                    mimo_feedback == nullptr) {
                    return false;
                } else if (mimo_feedback->nof_layers == nof_layers_t::single_layer &&
                           mimo_feedback->codebook_index <= CBI_MAX_2_ANTENNAS_SINGLE_LAYER) {
                    break;
                } else if (mimo_feedback->nof_layers == nof_layers_t::dual_layer &&
                           mimo_feedback->codebook_index <= CBI_MAX_2_ANTENNAS_DUAL_LAYER) {
                    break;
                }
                return false;

            case mimo_4_antennas:
                if (auto mimo_feedback = std::get_if<mimo_feedback_t>(&mcs_mimo_feedback);
                    mimo_feedback == nullptr) {
                    return false;
                } else if (mimo_feedback->nof_layers == nof_layers_t::single_layer &&
                           mimo_feedback->codebook_index <= CBI_MAX_4_ANTENNAS_SINGLE_LAYER) {
                    break;
                } else if (mimo_feedback->nof_layers == nof_layers_t::dual_layer &&
                           mimo_feedback->codebook_index <= CBI_MAX_4_ANTENNAS_DUAL_LAYER) {
                    break;
                } else if (mimo_feedback->nof_layers == nof_layers_t::four_layers &&
                           mimo_feedback->codebook_index <= CBI_MAX_4_ANTENNAS_FOUR_LAYERS) {
                    break;
                }
                return false;

            // FEEDBACK field is present when INDICATION TYPE is random access response
            default:
                return false;
        }
    }

    return true;
}

uint32_t broadcast_indication_ie_t::get_packed_size() const {
    dectnrp_assert(is_valid(), "Broadcast Indication IE is not valid");
    uint32_t packed_size = id_type == rd_id_type_t::short_rd_id ? 3 : 5;

    if (indication_type == indication_type_t::random_access_response &&
        feedback != feedback_type_t::no_feedback) {
        ++packed_size;
    }

    return packed_size;
}

void broadcast_indication_ie_t::pack(uint8_t* mac_pdu_offset) const {
    // assert that Broadcast Indication IE is valid before packing
    dectnrp_assert(is_valid(), "Broadcast Indication IE is not valid");

    // pack mandatory fields in octet 0 of IE
    mac_pdu_offset[0] = std::to_underlying(indication_type) << 5;
    mac_pdu_offset[0] |= std::to_underlying(id_type) << 4;
    mac_pdu_offset[0] |= resource_allocation_ie_follows;

    // pack LONG/SHORT RD-ID in octets 1 (and 2) of IE
    uint32_t N_bytes = id_type == rd_id_type_t::short_rd_id ? 2 : 4;
    common::adt::l2b_lower(mac_pdu_offset + 1, rd_id, N_bytes);
    ++N_bytes;

    if (indication_type == indication_type_t::random_access_response) {
        // pack optional fields in octet 0 of IE
        mac_pdu_offset[0] |= std::to_underlying(ack_nack) << 3;
        mac_pdu_offset[0] |= std::to_underlying(feedback) << 1;

        // pack optional MCS OR MIMO FEEDBACK field (if present)
        switch (feedback) {
            using enum feedback_type_t;

            default:
            case no_feedback:
                break;

            case mcs:
                {
                    const auto& channel_quality = std::get<channel_quality_t>(mcs_mimo_feedback);
                    mac_pdu_offset[N_bytes] = std::to_underlying(channel_quality);
                    ++N_bytes;
                    break;
                }

            case mimo_2_antennas:
            case mimo_4_antennas:
                {
                    const auto& mimo_feedback = std::get<mimo_feedback_t>(mcs_mimo_feedback);
                    mac_pdu_offset[N_bytes] = std::to_underlying(mimo_feedback.nof_layers)
                                              << (feedback == mimo_2_antennas ? 3 : 6);
                    mac_pdu_offset[N_bytes] |= mimo_feedback.codebook_index;
                    ++N_bytes;
                    break;
                }
        }
    }

    dectnrp_assert(get_packed_size_by_peeking(mac_pdu_offset).has_value(), "peeking failed");
    dectnrp_assert(get_packed_size_by_peeking(mac_pdu_offset).value() == N_bytes,
                   "lengths do not match");
}

bool broadcast_indication_ie_t::unpack(const uint8_t* mac_pdu_offset) {
    zero();

    // unpack non-optional fields in octet 0 of IE
    indication_type = common::adt::from_coded_value<indication_type_t>(mac_pdu_offset[0] >> 5);
    id_type = common::adt::from_coded_value<rd_id_type_t>((mac_pdu_offset[0] >> 4) & 1);
    resource_allocation_ie_follows = mac_pdu_offset[0] & 1;

    // unpack LONG/SHORT RD-ID in octets 1 (and 2) of IE
    uint32_t N_bytes = id_type == rd_id_type_t::short_rd_id ? 2 : 4;
    rd_id = common::adt::b2l_lower(mac_pdu_offset + 1, N_bytes);
    ++N_bytes;

    if (indication_type == indication_type_t::random_access_response) {
        // unpack optional fields in octet 0 of IE
        ack_nack =
            common::adt::from_coded_value<transmission_feedback_t>((mac_pdu_offset[0] >> 3) & 1);
        feedback = common::adt::from_coded_value<feedback_type_t>(mac_pdu_offset[0] & 0b110);

        // unpack optional MCS OR MIMO FEEDBACK field (if present)
        switch (mimo_feedback_t mimo_feedback; feedback) {
            using enum feedback_type_t;

            default:
            case no_feedback:
                break;

            case mcs:
                mcs_mimo_feedback =
                    common::adt::from_coded_value<channel_quality_t>(mac_pdu_offset[N_bytes] & 0xf);
                ++N_bytes;
                break;

            case mimo_2_antennas:
                mimo_feedback.nof_layers =
                    common::adt::from_coded_value<nof_layers_t>((mac_pdu_offset[N_bytes] >> 3) & 1);
                mimo_feedback.codebook_index = mac_pdu_offset[N_bytes] & 0b111;
                mcs_mimo_feedback = mimo_feedback;
                ++N_bytes;
                break;

            case mimo_4_antennas:
                mimo_feedback.nof_layers =
                    common::adt::from_coded_value<nof_layers_t>(mac_pdu_offset[N_bytes] >> 6);
                mimo_feedback.codebook_index =
                    mac_pdu_offset[N_bytes] & common::adt::bitmask_lsb<6>();
                mcs_mimo_feedback = mimo_feedback;
                ++N_bytes;
                break;
        }
    }

    dectnrp_assert(get_packed_size() == N_bytes, "lengths do not match");

    return is_valid();
}

mmie_packing_peeking_t::peek_result_t broadcast_indication_ie_t::get_packed_size_by_peeking(
    const uint8_t* mac_pdu_offset) const {
    uint32_t packed_size =
        ((mac_pdu_offset[0] >> 4) & 1) == std::to_underlying(rd_id_type_t::short_rd_id) ? 3 : 5;

    // check whether INDICATION TYPE field is set to a value that is not reserved
    if ((mac_pdu_offset[0] >> 5) >= std::to_underlying(indication_type_t::upper)) {
        return common::adt::UNDEFINED_NUMERIC_32;
    }

    // check whether MCS OR MIMO FEEDBACK field is present
    if ((mac_pdu_offset[0] >> 5) == std::to_underlying(indication_type_t::random_access_response) &&
        (mac_pdu_offset[0] & 0b110) != std::to_underlying(feedback_type_t::no_feedback)) {
        ++packed_size;
    }

    return packed_size;
}

}  // namespace dectnrp::sp4
