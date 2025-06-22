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

#include "dectnrp/sections_part4/mac_pdu/mac_multiplexing_header.hpp"

#include "dectnrp/common/adt/enumeration.hpp"
#include "dectnrp/common/prog/assert.hpp"
#include "dectnrp/sections_part4/mac_messages_and_ie/cluster_beacon_message.hpp"
#include "dectnrp/sections_part4/mac_messages_and_ie/extensions/power_target_ie.hpp"
#include "dectnrp/sections_part4/mac_messages_and_ie/padding_ie.hpp"
#include "dectnrp/sections_part4/mac_messages_and_ie/user_plane_data.hpp"

// separated as not standard-compliant
#include "dectnrp/sections_part4/mac_messages_and_ie/extensions/time_announce_ie.hpp"

/* The following header allows activating and deactivating individual MMIEs at the receiver. Even
 * when received, the MMIE decoder ignores the respective MMIEs as they are not recognized by the
 * MAC multiplexing header.
 */
#include "dectnrp/sections_part4/mac_messages_and_ie/mmie_activate.hpp"

namespace dectnrp::sp4 {

void mac_multiplexing_header_t::zero() {
    mac_ext = mac_ext_t::not_defined;
    ie_type.mac_ext_00_01_10 = ie_type_mac_ext_00_01_10_t::not_defined;
    length = common::adt::UNDEFINED_NUMERIC_32;
    tinfo = nullptr;
}

bool mac_multiplexing_header_t::is_valid() const {
    switch (mac_ext) {
        using enum mac_ext_t;

        case Length_Field_1_Bit:
            switch (length) {
                case 0:
                    return common::adt::is_valid(ie_type.mac_ext_11_len_0);
                case 1:
                    return common::adt::is_valid(ie_type.mac_ext_11_len_1);
            }
            return false;

        case Length_Field_8_Bit:
            if (length > common::adt::bitmask_lsb<8>()) {
                return false;
            }
            [[fallthrough]];

        case Length_Field_16_Bit:
            if (length > common::adt::bitmask_lsb<16>()) {
                return false;
            }
            [[fallthrough]];

        case No_Length_Field:
            return common::adt::is_valid(ie_type.mac_ext_00_01_10);

        default:
            return false;
    }
}

uint32_t mac_multiplexing_header_t::get_packed_size() const {
    dectnrp_assert(common::adt::is_valid(mac_ext), "mac_ext not valid");

    switch (mac_ext) {
        using enum mac_ext_t;

        case Length_Field_8_Bit:
            return packed_size_min_to_peek + 1;

        case Length_Field_16_Bit:
            return packed_size_min_to_peek + 2;

        default:
            return packed_size_min_to_peek;
    }
}

void mac_multiplexing_header_t::pack(uint8_t* mac_pdu_offset) const {
    dectnrp_assert(is_valid(), "MAC multiplexing header is not valid");

    // set MAC Extension field
    mac_pdu_offset[0] = std::to_underlying(mac_ext) << 6;

    switch (mac_ext) {
        using enum mac_ext_t;

        // Options a) and b) in Figure 6.3.4-1
        case Length_Field_1_Bit:
            // set Length field
            mac_pdu_offset[0] |= length << 5;
            // set IE Type field
            mac_pdu_offset[0] |= length ? std::to_underlying(ie_type.mac_ext_11_len_1)
                                        : std::to_underlying(ie_type.mac_ext_11_len_0);
            break;

        // Option c) in Figure 6.3.4-1
        case No_Length_Field:
            // set IE Type field
            mac_pdu_offset[0] |= std::to_underlying(ie_type.mac_ext_00_01_10);
            break;

        // Options d) and f) in Figure 6.3.4-1
        case Length_Field_8_Bit:
            // set IE Type field
            mac_pdu_offset[0] |= std::to_underlying(ie_type.mac_ext_00_01_10);
            // set Length field
            mac_pdu_offset[1] = length & common::adt::bitmask_lsb<8>();
            break;

        // Options e) and f) in Figure 6.3.4-1
        case Length_Field_16_Bit:
        default:
            // set IE Type field
            mac_pdu_offset[0] |= std::to_underlying(ie_type.mac_ext_00_01_10);
            // set Length field
            common::adt::l2b_lower(&mac_pdu_offset[1], length, 2);
            break;
    }
}

bool mac_multiplexing_header_t::unpack([[maybe_unused]] const uint8_t* mac_pdu_offset) {
    dectnrp_assert_failure("unpack is split in two functions");
    return false;
}

bool mac_multiplexing_header_t::unpack_mac_ext_ie_type(const uint8_t* mac_pdu_offset) {
    dectnrp_assert(mac_ext == mac_ext_t::not_defined &&
                       length == common::adt::UNDEFINED_NUMERIC_32 && tinfo == nullptr,
                   "fields not zero");

    mac_ext = common::adt::from_coded_value<mac_ext_t>(mac_pdu_offset[0] >> 6);

    // unpack MMIE type and make sure combination is valid and implemented
    switch (mac_ext) {
        using enum mac_ext_t;

        default:
            break;

        case No_Length_Field:
            {
                // unpack IE Type field
                const uint32_t A = mac_pdu_offset[0] & common::adt::bitmask_lsb<6>();
                ie_type.mac_ext_00_01_10 =
                    common::adt::from_coded_value<ie_type_mac_ext_00_01_10_t>(A);

                // check if IE type can occur in this combination
                switch (ie_type.mac_ext_00_01_10) {
                    using enum ie_type_mac_ext_00_01_10_t;

#ifdef ACTIVATE_Padding_IE
                    case Padding_IE:
                        tinfo = &typeid(padding_ie_t);
                        break;
#endif

#ifdef ACTIVATE_Network_Beacon_Message
                    case Network_Beacon_Message:
                        tinfo = &typeid(network_beacon_message_t);
                        break;
#endif

#ifdef ACTIVATE_Cluster_Beacon_Message
                    case Cluster_Beacon_Message:
                        tinfo = &typeid(cluster_beacon_message_t);
                        break;
#endif

#ifdef ACTIVATE_Association_Request_Message
                    case Association_Request_Message:
                        tinfo = &typeid(association_request_message_t);
                        break;
#endif

#ifdef ACTIVATE_Association_Response_Message
                    case Association_Response_Message:
                        tinfo = &typeid(association_response_message_t);
                        break;
#endif

#ifdef ACTIVATE_Association_Release_Message
                    case Association_Release_Message:
                        tinfo = &typeid(association_release_message_t);
                        break;
#endif

#ifdef ACTIVATE_Reconfiguration_Request_Message
                    case Reconfiguration_Request_Message:
                        tinfo = &typeid(reconfiguration_request_message_t);
                        break;
#endif

#ifdef ACTIVATE_Reconfiguration_Response_Message
                    case Reconfiguration_Response_Message:
                        tinfo = &typeid(reconfiguration_response_message_t);
                        break;
#endif

#ifdef ACTIVATE_MAC_Security_Info_IE
                    case Security_Info_IE:
                        tinfo = &typeid(mac_security_info_ie_t);
                        break;
#endif

#ifdef ACTIVATE_Route_Info_IE
                    case Route_Info_IE:
                        tinfo = &typeid(route_info_ie_t);
                        break;
#endif

#ifdef ACTIVATE_Resource_Allocation_IE
                    case Resource_Allocation_IE:
                        tinfo = &typeid(resource_allocation_ie_t);
                        break;
#endif

#ifdef ACTIVATE_Random_Access_Resource_IE
                    case Random_Access_Resource_IE:
                        tinfo = &typeid(random_access_resource_ie_t);
                        break;
#endif

#ifdef ACTIVATE_RD_Capability_IE
                    case RD_Capability_IE:
                        tinfo = &typeid(rd_capability_ie_t);
                        break;
#endif

#ifdef ACTIVATE_Neighbouring_IE
                    case Neighbouring_IE:
                        tinfo = &typeid(neighbouring_ie_t);
                        break;
#endif

#ifdef ACTIVATE_Broadcast_Indication_IE
                    case Broadcast_Indication_IE:
                        tinfo = &typeid(broadcast_indication_ie_t);
                        break;
#endif

#ifdef ACTIVATE_Load_Info_IE
                    case Load_Info_IE:
                        tinfo = &typeid(load_info_ie_t);
                        break;
#endif

#ifdef ACTIVATE_Measurement_Report_IE
                    case Measurement_Report_IE:
                        tinfo = &typeid(measurement_report_ie_t);
                        break;
#endif

#ifdef ACTIVATE_Power_Target_IE
                    case Power_Target_IE:
                        tinfo = &typeid(extensions::power_target_ie_t);
                        break;
#endif

#ifdef ACTIVATE_Time_Announce_IE
                    case Time_Announce_IE:
                        tinfo = &typeid(extensions::time_announce_ie_t);
                        break;
#endif

                    default:
                        break;
                }
            }

            break;

        case Length_Field_8_Bit:
            {
                // unpack IE Type field
                const uint32_t A = mac_pdu_offset[0] & common::adt::bitmask_lsb<6>();
                ie_type.mac_ext_00_01_10 =
                    common::adt::from_coded_value<ie_type_mac_ext_00_01_10_t>(A);

                // check if IE type can occur in this combination
                switch (ie_type.mac_ext_00_01_10) {
                    using enum ie_type_mac_ext_00_01_10_t;

#ifdef ACTIVATE_Padding_IE
                    case Padding_IE:
                        tinfo = &typeid(padding_ie_t);
                        break;
#endif

#ifdef ACTIVATE_Higher_Layer_Signalling
                    case Higher_Layer_Signalling_Flow_1:
                    case Higher_Layer_Signalling_Flow_2:
                        tinfo = &typeid(higher_layer_signalling_t);
                        break;
#endif

#ifdef ACTIVATE_User_Plane_Data
                    case User_Plane_Data_Flow_1:
                    case User_Plane_Data_Flow_2:
                    case User_Plane_Data_Flow_3:
                    case User_Plane_Data_Flow_4:
                        tinfo = &typeid(user_plane_data_t);
                        break;
#endif

#ifdef ACTIVATE_Group_Assignment_IE
                    case Group_Assignment_IE:
                        tinfo = &typeid(group_assignment_ie_t);
                        break;
#endif

                    default:
                        break;
                }
            }

            break;

        case Length_Field_16_Bit:
            {
                // unpack IE Type field
                const uint32_t A = mac_pdu_offset[0] & common::adt::bitmask_lsb<6>();
                ie_type.mac_ext_00_01_10 =
                    common::adt::from_coded_value<ie_type_mac_ext_00_01_10_t>(A);

                // check if IE type can occur in this combination
                switch (ie_type.mac_ext_00_01_10) {
                    using enum ie_type_mac_ext_00_01_10_t;

#ifdef ACTIVATE_Higher_Layer_Signalling
                    case Higher_Layer_Signalling_Flow_1:
                    case Higher_Layer_Signalling_Flow_2:
                        tinfo = &typeid(higher_layer_signalling_t);
                        break;
#endif

#ifdef ACTIVATE_User_Plane_Data
                    case User_Plane_Data_Flow_1:
                    case User_Plane_Data_Flow_2:
                    case User_Plane_Data_Flow_3:
                    case User_Plane_Data_Flow_4:
                        tinfo = &typeid(user_plane_data_t);
                        break;
#endif

#ifdef ACTIVATE_Group_Assignment_IE
                    case Group_Assignment_IE:
                        tinfo = &typeid(group_assignment_ie_t);
                        break;
#endif

                    default:
                        break;
                }
            }

            break;

        case Length_Field_1_Bit:
            {
                // unpack Length field which is part of the first byte
                const uint32_t length_local = (mac_pdu_offset[0] >> 5) & 1;

                // unpack coded IE Type field
                const uint32_t A = mac_pdu_offset[0] & common::adt::bitmask_lsb<5>();

                // check if IE type can occur in this combination
                switch (length_local) {
                    case 0:

                        ie_type.mac_ext_11_len_0 =
                            common::adt::from_coded_value<ie_type_mac_ext_11_len_0_t>(A);

                        switch (ie_type.mac_ext_11_len_0) {
                            using enum ie_type_mac_ext_11_len_0_t;

#ifdef ACTIVATE_Padding_IE
                            case Padding_IE:
                                tinfo = &typeid(padding_ie_t);
                                break;
#endif

#ifdef ACTIVATE_Configuration_Request_IE
                            case Configuration_Request_IE:
                                tinfo = &typeid(configuration_request_ie_t);
                                break;
#endif

                            default:
                                break;
                        }

                        break;

                    case 1:

                        ie_type.mac_ext_11_len_1 =
                            common::adt::from_coded_value<ie_type_mac_ext_11_len_1_t>(A);

                        switch (common::adt::from_coded_value<ie_type_mac_ext_11_len_1_t>(A)) {
                            using enum ie_type_mac_ext_11_len_1_t;

#ifdef ACTIVATE_Padding_IE
                            case Padding_IE:
                                tinfo = &typeid(padding_ie_t);
                                break;
#endif

#ifdef ACTIVATE_Radio_Device_Status_IE
                            case Radio_Device_Status_IE:
                                tinfo = &typeid(radio_device_status_ie_t);
                                break;
#endif

                            default:
                                break;
                        }

                        break;

                    default:
                        dectnrp_assert_failure("length decoded not to 0 or 1");
                        break;
                }
            }

            break;
    }

    return tinfo != nullptr;
}

void mac_multiplexing_header_t::unpack_length(const uint8_t* mac_pdu_offset) {
    dectnrp_assert(length == common::adt::UNDEFINED_NUMERIC_32, "length not zero");

    switch (mac_ext) {
        using enum mac_ext_t;

        default:
            dectnrp_assert_failure("mac_ext not valid");
            break;

        case No_Length_Field:
            length = common::adt::UNDEFINED_NUMERIC_32;
            break;

        case Length_Field_8_Bit:
            length = mac_pdu_offset[1];
            break;

        case Length_Field_16_Bit:
            length = common::adt::b2l_lower(&mac_pdu_offset[1], 2);
            break;

        case Length_Field_1_Bit:
            length = (mac_pdu_offset[0] >> 5) & 1;
            break;
    }
}

}  // namespace dectnrp::sp4
