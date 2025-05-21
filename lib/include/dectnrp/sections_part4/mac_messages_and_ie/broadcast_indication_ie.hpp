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

#include <variant>

#include "dectnrp/sections_part4/mac_messages_and_ie/mmie.hpp"

namespace dectnrp::sp4 {

class broadcast_indication_ie_t final : public mmie_packing_peeking_t {
    public:
        // Table 6.4.3.7-1, field INDICATION TYPE
        enum class indication_type_t : uint32_t {
            not_defined = common::adt::UNDEFINED_NUMERIC_32,
            paging = 0,
            random_access_response,
            upper
        };

        // Table 6.4.3.7-1, field ID TYPE
        enum class rd_id_type_t : uint32_t {
            not_defined = common::adt::UNDEFINED_NUMERIC_32,
            short_rd_id = 0,
            long_rd_id,
            upper
        };

        // Table 6.4.3.7-1, field ACK/NACK
        enum class transmission_feedback_t : uint32_t {
            not_defined = common::adt::UNDEFINED_NUMERIC_32,
            nack = 0,
            ack,
            upper
        };

        // Table 6.4.3.7-1, field FEEDBACK
        enum class feedback_type_t : uint32_t {
            not_defined = common::adt::UNDEFINED_NUMERIC_32,
            no_feedback = 0,
            mcs,
            mimo_2_antennas,
            mimo_4_antennas,
            upper
        };

        // Table 6.2.2-3
        enum class channel_quality_t : uint32_t {
            not_defined = common::adt::UNDEFINED_NUMERIC_32,
            lower = 0,
            mcs_0,
            mcs_1,
            mcs_2,
            mcs_3,
            mcs_4,
            mcs_5,
            mcs_6,
            mcs_7,
            mcs_8,
            mcs_9,
            mcs_10,
            mcs_11,
            upper
        };

        // Table 6.4.3.7-1, field MCS OR MIMO FEEDBACK
        enum class nof_layers_t : uint32_t {
            not_defined = common::adt::UNDEFINED_NUMERIC_32,
            single_layer = 0,
            dual_layer,
            four_layers,
            upper
        };

        struct mimo_feedback_t {
                nof_layers_t nof_layers;
                uint32_t codebook_index;
        };

        broadcast_indication_ie_t();

        indication_type_t indication_type;
        rd_id_type_t id_type;
        transmission_feedback_t ack_nack;
        feedback_type_t feedback;
        bool resource_allocation_ie_follows;
        uint32_t rd_id;
        std::variant<std::monostate, channel_quality_t, mimo_feedback_t> mcs_mimo_feedback;

    private:
        void zero() override;
        bool is_valid() const override;
        uint32_t get_packed_size() const override;
        void pack(uint8_t* mac_pdu_offset) const override;
        bool unpack(const uint8_t* mac_pdu_offset) override;

        constexpr uint32_t get_packed_size_min_to_peek() const override { return 1; }
        virtual peek_result_t get_packed_size_by_peeking(
            const uint8_t* mac_pdu_offset) const override;

        // Table 6.3.4-1 (ETSI TS 103 636-3)
        static constexpr uint32_t CBI_MAX_2_ANTENNAS_SINGLE_LAYER{5};

        // Table 6.3.4-3 (ETSI TS 103 636-3)
        static constexpr uint32_t CBI_MAX_2_ANTENNAS_DUAL_LAYER{2};

        // Table 6.3.4-2 (ETSI TS 103 636-3)
        static constexpr uint32_t CBI_MAX_4_ANTENNAS_SINGLE_LAYER{27};

        // Table 6.3.4-4 (ETSI TS 103 636-3)
        static constexpr uint32_t CBI_MAX_4_ANTENNAS_DUAL_LAYER{21};

        // Table 6.3.4-5 (ETSI TS 103 636-3)
        static constexpr uint32_t CBI_MAX_4_ANTENNAS_FOUR_LAYERS{4};
};

}  // namespace dectnrp::sp4
