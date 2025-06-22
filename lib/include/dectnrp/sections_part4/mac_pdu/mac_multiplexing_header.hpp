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

#include <utility>

#include "dectnrp/common/adt/miscellaneous.hpp"
#include "dectnrp/common/serdes/packing.hpp"

namespace dectnrp::sp4 {

class mac_multiplexing_header_t final : public common::serdes::packing_t {
    public:
        /// MAC extension field encoding (Table 6.3.4-1).
        enum class mac_ext_t : uint32_t {
            not_defined = common::adt::UNDEFINED_NUMERIC_32,
            No_Length_Field = 0,  // self-contained
            Length_Field_8_Bit,   // up to 2 + 255 = 257 bytes
            Length_Field_16_Bit,  // up tp 3 + 65535 bytes
            Length_Field_1_Bit,   // 1 or 2 bytes
            upper
        };

        /// IE type field encoding for MAC extension field encoding 00, 01, 10 (Table 6.3.4-2).
        enum class ie_type_mac_ext_00_01_10_t : uint32_t {
            not_defined = common::adt::UNDEFINED_NUMERIC_32,
            Padding_IE = 0,
            Higher_Layer_Signalling_Flow_1 = 0b1,
            Higher_Layer_Signalling_Flow_2 = 0b10,
            User_Plane_Data_Flow_1 = 0b11,
            User_Plane_Data_Flow_2 = 0b100,
            User_Plane_Data_Flow_3 = 0b101,
            User_Plane_Data_Flow_4 = 0b110,
            // Reserved
            Network_Beacon_Message = 0b1000,
            Cluster_Beacon_Message = 0b1001,
            Association_Request_Message = 0b1010,
            Association_Response_Message = 0b1011,
            Association_Release_Message = 0b1100,
            Reconfiguration_Request_Message = 0b1101,
            Reconfiguration_Response_Message = 0b1110,
            Additional_MAC_Messages = 0b1111,
            Security_Info_IE = 0b10000,
            Route_Info_IE = 0b10001,
            Resource_Allocation_IE = 0b10010,
            Random_Access_Resource_IE = 0b10011,
            RD_Capability_IE = 0b10100,
            Neighbouring_IE = 0b10101,
            Broadcast_Indication_IE = 0b10110,
            Group_Assignment_IE = 0b10111,
            Load_Info_IE = 0b11000,
            Measurement_Report_IE = 0b11001,
            // Source_Routing_IE = 0b11010,
            // Joining_Beacon_Message = 0b11011,
            // Joining_Information_IE = 0b11100,
            // Reserved
            Escape = 0b111110,
            IE_Type_Extension = 0b111111,
            /*
             * The following MMIEs are not part of the standard, and exist only in this project.
             */
            Power_Target_IE = 0b11101,
            Time_Announce_IE = 0b11110
        };

        ///  IE type field encoding for MAC extension field encoding 11 and payload length 0 byte
        ///  (Table 6.3.4-3).
        enum class ie_type_mac_ext_11_len_0_t : uint32_t {
            not_defined = common::adt::UNDEFINED_NUMERIC_32,
            Padding_IE = 0,
            Configuration_Request_IE = 0b1,
            // Keep_alive_IE = 0b10,
            // Reserved
            MAC_Security_Info_IE = 0b10000,
            // Reserved
            Escape = 0b11110
            // Reserved
        };

        /// IE type field encoding for MAC extension field encoding 11 and payload length of 1 byte
        /// (Table 6.3.4-4).
        enum class ie_type_mac_ext_11_len_1_t : uint32_t {
            not_defined = common::adt::UNDEFINED_NUMERIC_32,
            Padding_IE = 0,
            Radio_Device_Status_IE = 0b1,
            // Radio_capability_short_IE = 0b10,
            // Association_Control_IE = 0b11,
            // Reserved
            Escape = 0b11110
            // Reserved
        };

        union ie_type_t {
                ie_type_mac_ext_00_01_10_t mac_ext_00_01_10;
                ie_type_mac_ext_11_len_0_t mac_ext_11_len_0;
                ie_type_mac_ext_11_len_1_t mac_ext_11_len_1;
        };

        [[nodiscard]] mac_multiplexing_header_t() { zero(); };

        void zero() override;
        [[nodiscard]] bool is_valid() const override final;
        [[nodiscard]] uint32_t get_packed_size() const override;
        void pack(uint8_t* mac_pdu_offset) const override;
        [[nodiscard]] bool unpack(const uint8_t* mac_pdu_offset) override;

        /// first byte of any MAC multiplexing header is enough to determine the full packed size
        static constexpr uint32_t packed_size_min_to_peek{1U};

        /// when containing two byte for length
        static constexpr uint32_t packet_size_max{3U};

        [[nodiscard]] bool unpack_mac_ext_ie_type(const uint8_t* mac_pdu_offset);
        void unpack_length(const uint8_t* mac_pdu_offset);

        mac_ext_t mac_ext;
        ie_type_t ie_type;
        uint32_t length;

        /// during RX, we also save the type of the MMIE
        const std::type_info* tinfo;
};

}  // namespace dectnrp::sp4

namespace dectnrp::common::adt {

template <std::same_as<sp4::mac_multiplexing_header_t::ie_type_mac_ext_00_01_10_t> E>
[[nodiscard]] E from_coded_value(const std::underlying_type_t<E> value) {
    switch (value) {
        case std::to_underlying(E::Padding_IE):
        case std::to_underlying(E::Higher_Layer_Signalling_Flow_1):
        case std::to_underlying(E::Higher_Layer_Signalling_Flow_2):
        case std::to_underlying(E::User_Plane_Data_Flow_1):
        case std::to_underlying(E::User_Plane_Data_Flow_2):
        case std::to_underlying(E::User_Plane_Data_Flow_3):
        case std::to_underlying(E::User_Plane_Data_Flow_4):
        case std::to_underlying(E::Network_Beacon_Message):
        case std::to_underlying(E::Cluster_Beacon_Message):
        case std::to_underlying(E::Association_Request_Message):
        case std::to_underlying(E::Association_Response_Message):
        case std::to_underlying(E::Association_Release_Message):
        case std::to_underlying(E::Reconfiguration_Request_Message):
        case std::to_underlying(E::Reconfiguration_Response_Message):
        case std::to_underlying(E::Additional_MAC_Messages):
        case std::to_underlying(E::Security_Info_IE):
        case std::to_underlying(E::Route_Info_IE):
        case std::to_underlying(E::Resource_Allocation_IE):
        case std::to_underlying(E::Random_Access_Resource_IE):
        case std::to_underlying(E::RD_Capability_IE):
        case std::to_underlying(E::Neighbouring_IE):
        case std::to_underlying(E::Broadcast_Indication_IE):
        case std::to_underlying(E::Group_Assignment_IE):
        case std::to_underlying(E::Load_Info_IE):
        case std::to_underlying(E::Measurement_Report_IE):
        case std::to_underlying(E::Escape):
        case std::to_underlying(E::IE_Type_Extension):
            /*
             * The following MMIEs are not part of the standard, and exist only in this project.
             */
        case std::to_underlying(E::Power_Target_IE):
        case std::to_underlying(E::Time_Announce_IE):
            return static_cast<E>(value);

        default:
            return E::not_defined;
    }
}

template <std::same_as<sp4::mac_multiplexing_header_t::ie_type_mac_ext_11_len_0_t> E>
[[nodiscard]] E from_coded_value(const std::underlying_type_t<E> value) {
    switch (value) {
        case std::to_underlying(E::Padding_IE):
        case std::to_underlying(E::Configuration_Request_IE):
        case std::to_underlying(E::MAC_Security_Info_IE):
        case std::to_underlying(E::Escape):
            return static_cast<E>(value);

        default:
            return E::not_defined;
    }
}

template <std::same_as<sp4::mac_multiplexing_header_t::ie_type_mac_ext_11_len_1_t> E>
[[nodiscard]] E from_coded_value(const std::underlying_type_t<E> value) {
    switch (value) {
        case std::to_underlying(E::Padding_IE):
        case std::to_underlying(E::Radio_Device_Status_IE):
        case std::to_underlying(E::Escape):
            return static_cast<E>(value);

        default:
            return E::not_defined;
    }
}

}  // namespace dectnrp::common::adt
