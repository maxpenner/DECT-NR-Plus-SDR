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

#include <array>
#include <optional>
#include <vector>

#include "dectnrp/sections_part4/mac_messages_and_ie/mmie.hpp"

namespace dectnrp::section4 {

class rd_capability_ie_t final : public mmie_packing_peeking_t {
    public:
        // Table 6.4.3.5-1, field RELEASE
        enum class release_t : uint32_t {
            not_defined = common::adt::UNDEFINED_NUMERIC_32,
            lower = 0,
            release_1,
            release_2,
            release_3,
            release_4,
            upper
        };

        // Table 6.4.3.5-1, field OPERATING MODES
        enum class operating_mode_t : uint32_t {
            not_defined = common::adt::UNDEFINED_NUMERIC_32,
            pt_operation = 0,
            ft_operation,
            ft_pt_operation,
            upper
        };

        // Table 6.4.3.5-1, field MAC SECURITY
        enum class mac_security_t : uint32_t {
            not_defined = common::adt::UNDEFINED_NUMERIC_32,
            not_supported = 0,
            mode_1,
            upper
        };

        // Table 6.4.3.5-1, field DLC SERVICE TYPE
        enum class dlc_service_type_t : uint32_t {
            not_defined = common::adt::UNDEFINED_NUMERIC_32,
            type_0 = 0,
            type_1,
            type_2,
            types_1_2_3,
            types_0_1_2_3,
            upper
        };

        // Table 6.4.3.5-1, field RD POWER CLASS
        enum class rd_power_class_t : uint32_t {
            not_defined = common::adt::UNDEFINED_NUMERIC_32,
            class_1 = 0,
            class_2,
            class_3,
            upper
        };

        // Table 6.4.3.5-1, field MAX NSS FOR RX (see also Annex B.2 of ETSI TS 103 636-3)
        enum class max_nof_spatial_streams_t : uint32_t {
            not_defined = common::adt::UNDEFINED_NUMERIC_32,
            _1_spatial_streams = 0,
            _2_spatial_streams,
            _4_spatial_streams,
            _8_spatial_streams,
            upper
        };

        // Table 6.4.3.5-1, field RX FOR TX DIVERSITY (see also Annex B.2 of ETSI TS 103 636-3)
        enum class max_nof_tx_antennas_t : uint32_t {
            not_defined = common::adt::UNDEFINED_NUMERIC_32,
            _1_antennas = 0,
            _2_antennas,
            _4_antennas,
            _8_antennas,
            upper
        };

        // Table 6.4.3.5-1, field MAX MCS (see also Annex B.2 of ETSI TS 103 636-3)
        enum class max_mcs_index_t : uint32_t {
            not_defined = common::adt::UNDEFINED_NUMERIC_32,
            _2 = 0,
            _3,
            _4,
            _5,
            _6,
            _7,
            _8,
            _9,
            _10,
            _11,
            upper
        };

        // Table 6.4.3.5-1, field SOFT-BUFFER SIZE (see also Annex B.2 of ETSI TS 103 636-3)
        enum class soft_buffer_size_t : uint32_t {
            not_defined = common::adt::UNDEFINED_NUMERIC_32,
            _16000_bytes = 0,
            _25344_bytes,
            _32000_bytes,
            _64000_bytes,
            _128000_bytes,
            _256000_bytes,
            _512000_bytes,
            _1024000_bytes,
            _2048000_bytes,
            upper
        };

        // Table 6.4.3.5-1, field NUM. OF HARQ PROCESSES (see also Annex B.2 of ETSI TS 103 636-3)
        enum class nof_harq_processes_t : uint32_t {
            not_defined = common::adt::UNDEFINED_NUMERIC_32,
            _1_processes = 0,
            _2_processes,
            _4_processes,
            _8_processes,
            upper
        };

        // Table 6.4.3.5-1, field HARQ FEEDBACK DELAY
        enum class harq_feedback_delay_t : uint32_t {
            not_defined = common::adt::UNDEFINED_NUMERIC_32,
            _0_subslots = 0,
            _1_subslots,
            _2_subslots,
            _3_subslots,
            _4_subslots,
            _5_subslots,
            _6_subslots,
            upper
        };

        // Table 6.4.3.5-1, field MU (see also Annex B.2 of ETSI TS 103 636-3)
        enum class subcarrier_width_t : uint32_t {
            not_defined = common::adt::UNDEFINED_NUMERIC_32,
            _27_kHz = 0,
            _54_kHz,
            _108_kHz,
            _216_kHz,
            upper
        };

        // Table 6.4.3.5-1, field BETA (see also Annex B.2 of ETSI TS 103 636-3)
        enum class dft_size_t : uint32_t {
            not_defined = common::adt::UNDEFINED_NUMERIC_32,
            _64 = 0,
            _128,
            _256,
            _512,
            _768,
            _1024,
            upper
        };

        struct radio_device_class_t {
                subcarrier_width_t mu;
                dft_size_t beta;
        };

        struct phy_capability_t {
                rd_power_class_t rd_power_class;
                max_nof_spatial_streams_t max_nss_for_rx;
                max_nof_tx_antennas_t rx_for_tx_diversity;

                void set_rx_gain(const int32_t rx_gain_dB) {
                    const auto lower =
                        std::lower_bound(rx_gain_table.begin(), rx_gain_table.end(), rx_gain_dB);
                    rx_gain_index = lower != rx_gain_table.end() ? lower - rx_gain_table.begin()
                                                                 : rx_gain_table.size() - 1;
                }

                std::optional<int32_t> get_rx_gain_dB() const {
                    try {
                        return rx_gain_table.at(rx_gain_index);
                    } catch (const std::exception& e) {
                        return {};
                    }
                }

                max_mcs_index_t max_mcs;
                soft_buffer_size_t soft_buffer_size;
                nof_harq_processes_t nof_harq_processes;
                harq_feedback_delay_t harq_feedback_delay;

                friend class rd_capability_ie_t;

            private:
                uint32_t rx_gain_index;

                static constexpr std::array<int32_t, 9> rx_gain_table = {
                    -10, -8, -6, -4, -2, 0, 2, 4, 6};
        };

        struct phy_capability_additional_t : radio_device_class_t, phy_capability_t {};

        rd_capability_ie_t();

        std::vector<phy_capability_additional_t> additional_phy_capabilities;
        release_t release;
        operating_mode_t operating_modes;
        bool supports_mesh_system_operation;
        bool supports_scheduled_data_transfer_service;
        mac_security_t mac_security;
        dlc_service_type_t dlc_service_type;
        phy_capability_t phy_capability;

    private:
        void zero() override;
        bool is_valid() const override;
        uint32_t get_packed_size() const override;
        void pack(uint8_t* mac_pdu_offset) const override;
        bool unpack(const uint8_t* mac_pdu_offset) override;

        constexpr uint32_t get_packed_size_min_to_peek() const override { return 1; }
        peek_result_t get_packed_size_by_peeking(const uint8_t* mac_pdu_offset) const override;
};

}  // namespace dectnrp::section4
