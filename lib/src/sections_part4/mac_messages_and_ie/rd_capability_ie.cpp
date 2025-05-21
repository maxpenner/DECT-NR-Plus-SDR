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

#include "dectnrp/sections_part4/mac_messages_and_ie/rd_capability_ie.hpp"

#include <utility>

#include "dectnrp/common/adt/bitbyte.hpp"
#include "dectnrp/common/adt/enumeration.hpp"

namespace dectnrp::sp4 {

rd_capability_ie_t::rd_capability_ie_t() {
    mac_mux_header.zero();
    mac_mux_header.mac_ext = mac_multiplexing_header_t::mac_ext_t::No_Length_Field;
    mac_mux_header.ie_type.mac_ext_00_01_10 =
        mac_multiplexing_header_t::ie_type_mac_ext_00_01_10_t::RD_Capability_IE;

    zero();

    dectnrp_assert(check_validity_at_runtime(this), "mmie invalid");
}

void rd_capability_ie_t::zero() {
    additional_phy_capabilities.clear();
    release = release_t::not_defined;

    operating_modes = operating_mode_t::not_defined;
    supports_mesh_system_operation = false;
    supports_scheduled_data_transfer_service = false;

    mac_security = mac_security_t::not_defined;
    dlc_service_type = dlc_service_type_t::not_defined;

    phy_capability.rd_power_class = rd_power_class_t::not_defined;
    phy_capability.max_nss_for_rx = max_nof_spatial_streams_t::not_defined;
    phy_capability.rx_for_tx_diversity = max_nof_tx_antennas_t::not_defined;

    phy_capability.rx_gain_index = common::adt::UNDEFINED_NUMERIC_32;
    phy_capability.max_mcs = max_mcs_index_t::not_defined;

    phy_capability.soft_buffer_size = soft_buffer_size_t::not_defined;
    phy_capability.nof_harq_processes = nof_harq_processes_t::not_defined;

    phy_capability.harq_feedback_delay = harq_feedback_delay_t::not_defined;
}

bool rd_capability_ie_t::is_valid() const {
    auto is_phy_capability_valid = [](const phy_capability_t& capability) -> bool {
        return common::adt::is_valid(capability.rd_power_class) &&
               common::adt::is_valid(capability.max_nss_for_rx) &&
               common::adt::is_valid(capability.rx_for_tx_diversity) &&
               capability.get_rx_gain_dB() && common::adt::is_valid(capability.max_mcs) &&
               common::adt::is_valid(capability.soft_buffer_size) &&
               common::adt::is_valid(capability.nof_harq_processes) &&
               common::adt::is_valid(capability.harq_feedback_delay);
    };

    if (additional_phy_capabilities.size() > 0b111 ||
        !std::all_of(additional_phy_capabilities.cbegin(),
                     additional_phy_capabilities.cend(),
                     [&is_phy_capability_valid](const auto& capability) {
                         return common::adt::is_valid(capability.mu) &&
                                common::adt::is_valid(capability.beta) &&
                                is_phy_capability_valid(capability);
                     })) {
        return false;
    }

    if (!common::adt::is_valid(release) || !common::adt::is_valid(operating_modes) ||
        !common::adt::is_valid(mac_security) || !common::adt::is_valid(dlc_service_type) ||
        !is_phy_capability_valid(phy_capability)) {
        return false;
    }

    return true;
}

uint32_t rd_capability_ie_t::get_packed_size() const {
    dectnrp_assert(is_valid(), "RD Capability IE is not valid");
    return 7 + additional_phy_capabilities.size() * 5;
}

void rd_capability_ie_t::pack(uint8_t* mac_pdu_offset) const {
    // assert that RD Capability IE is valid before packing
    dectnrp_assert(is_valid(), "RD Capability IE is not valid");

    // set required fields in octet 0
    mac_pdu_offset[0] = additional_phy_capabilities.size() << 5;
    mac_pdu_offset[0] |= std::to_underlying(release);

    // set required fields in octet 1
    mac_pdu_offset[1] = std::to_underlying(operating_modes) << 2;
    mac_pdu_offset[1] |= supports_mesh_system_operation << 1;
    mac_pdu_offset[1] |= supports_scheduled_data_transfer_service;

    // set required fields in octet 2
    mac_pdu_offset[2] = std::to_underlying(mac_security) << 5;
    mac_pdu_offset[2] |= std::to_underlying(dlc_service_type) << 2;

    auto pack_phy_capability = [](const phy_capability_t& src, uint8_t* const dst) -> uint32_t {
        *dst = std::to_underlying(src.rd_power_class) << 4;
        *dst |= std::to_underlying(src.max_nss_for_rx) << 2;
        *dst |= std::to_underlying(src.rx_for_tx_diversity);

        dst[1] = src.rx_gain_index << 4;
        dst[1] |= std::to_underlying(src.max_mcs);

        dst[2] = std::to_underlying(src.soft_buffer_size) << 4;
        dst[2] |= std::to_underlying(src.nof_harq_processes) << 2;

        dst[3] = std::to_underlying(src.harq_feedback_delay) << 4;

        return 4;
    };

    // set required fields in octets 3-6
    pack_phy_capability(phy_capability, mac_pdu_offset + 3);

    uint32_t N_bytes = 7;

    // pack PHY capabilities for additional physical layer numerologies (if included)
    for (const auto& capability : additional_phy_capabilities) {
        mac_pdu_offset[N_bytes] = std::to_underlying(capability.mu) << 5;
        mac_pdu_offset[N_bytes] |= std::to_underlying(capability.beta) << 1;
        ++N_bytes;

        N_bytes += pack_phy_capability(capability, mac_pdu_offset + N_bytes);
    }

    dectnrp_assert(get_packed_size_by_peeking(mac_pdu_offset).has_value(), "peeking failed");
    dectnrp_assert(get_packed_size_by_peeking(mac_pdu_offset).value() == N_bytes,
                   "lengths do not match");
}

bool rd_capability_ie_t::unpack(const uint8_t* mac_pdu_offset) {
    zero();

    // unpack RELEASE field in octet 0
    release =
        common::adt::from_coded_value<release_t>(mac_pdu_offset[0] & common::adt::bitmask_lsb<5>());

    // unpack non-optional fields in octet 1
    operating_modes =
        common::adt::from_coded_value<operating_mode_t>((mac_pdu_offset[1] >> 2) & 0b11);
    supports_mesh_system_operation = mac_pdu_offset[1] & 0b10;
    supports_scheduled_data_transfer_service = mac_pdu_offset[1] & 1;

    // unpack non-optional fields in octet 2
    mac_security = common::adt::from_coded_value<mac_security_t>(mac_pdu_offset[2] >> 5);
    dlc_service_type =
        common::adt::from_coded_value<dlc_service_type_t>((mac_pdu_offset[2] >> 2) & 0b111);

    auto unpack_phy_capability = [](const uint8_t* src, phy_capability_t& dst) -> uint32_t {
        dst.rd_power_class = common::adt::from_coded_value<rd_power_class_t>((*src >> 4) & 0b111);
        dst.max_nss_for_rx =
            common::adt::from_coded_value<max_nof_spatial_streams_t>((*src >> 2) & 0b11);
        dst.rx_for_tx_diversity = common::adt::from_coded_value<max_nof_tx_antennas_t>(*src & 0b11);

        dst.rx_gain_index = src[1] >> 4;
        dst.max_mcs = common::adt::from_coded_value<max_mcs_index_t>(src[1] & 0xf);

        dst.soft_buffer_size = common::adt::from_coded_value<soft_buffer_size_t>(src[2] >> 4);
        dst.nof_harq_processes =
            common::adt::from_coded_value<nof_harq_processes_t>((src[2] >> 2) & 0b11);

        dst.harq_feedback_delay = common::adt::from_coded_value<harq_feedback_delay_t>(src[3] >> 4);

        return 4;
    };

    // unpack non-optional PHY capability in octets 3-6
    unpack_phy_capability(mac_pdu_offset + 3, phy_capability);

    uint32_t N_bytes = 7;

    // unpack PHY capabilities for additional physical layer numerologies (if included)
    for (phy_capability_additional_t capability;
         // ToDo: add check before calling .value(), otherwise undefined behaviour
         N_bytes < get_packed_size_by_peeking(mac_pdu_offset).value();
         additional_phy_capabilities.push_back(capability)) {
        capability.mu =
            common::adt::from_coded_value<subcarrier_width_t>(mac_pdu_offset[N_bytes] >> 5);
        capability.beta =
            common::adt::from_coded_value<dft_size_t>((mac_pdu_offset[N_bytes] >> 1) & 0xf);
        ++N_bytes;

        N_bytes += unpack_phy_capability(mac_pdu_offset + N_bytes, capability);
    }

    dectnrp_assert(get_packed_size() == N_bytes, "lengths do not match");

    return is_valid();
}

mmie_packing_peeking_t::peek_result_t rd_capability_ie_t::get_packed_size_by_peeking(
    const uint8_t* mac_pdu_offset) const {
    return 7 + (mac_pdu_offset[0] >> 5) * 5;
}

}  // namespace dectnrp::sp4
