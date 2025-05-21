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

#include "dectnrp/sections_part4/mac_messages_and_ie/mmie_pool_tx.hpp"

#include "dectnrp/sections_part4/mac_messages_and_ie/association_release_message.hpp"
#include "dectnrp/sections_part4/mac_messages_and_ie/association_request_message.hpp"
#include "dectnrp/sections_part4/mac_messages_and_ie/association_response_message.hpp"
#include "dectnrp/sections_part4/mac_messages_and_ie/broadcast_indication_ie.hpp"
#include "dectnrp/sections_part4/mac_messages_and_ie/cluster_beacon_message.hpp"
#include "dectnrp/sections_part4/mac_messages_and_ie/configuration_request_ie.hpp"
#include "dectnrp/sections_part4/mac_messages_and_ie/group_assignment_ie.hpp"
#include "dectnrp/sections_part4/mac_messages_and_ie/higher_layer_signalling.hpp"
#include "dectnrp/sections_part4/mac_messages_and_ie/load_info_ie.hpp"
#include "dectnrp/sections_part4/mac_messages_and_ie/mac_security_info_ie.hpp"
#include "dectnrp/sections_part4/mac_messages_and_ie/measurement_report_ie.hpp"
#include "dectnrp/sections_part4/mac_messages_and_ie/neighbouring_ie.hpp"
#include "dectnrp/sections_part4/mac_messages_and_ie/network_beacon_message.hpp"
#include "dectnrp/sections_part4/mac_messages_and_ie/padding_ie.hpp"
#include "dectnrp/sections_part4/mac_messages_and_ie/radio_device_status_ie.hpp"
#include "dectnrp/sections_part4/mac_messages_and_ie/random_access_resource_ie.hpp"
#include "dectnrp/sections_part4/mac_messages_and_ie/rd_capability_ie.hpp"
#include "dectnrp/sections_part4/mac_messages_and_ie/reconfiguration_request_message.hpp"
#include "dectnrp/sections_part4/mac_messages_and_ie/reconfiguration_response_message.hpp"
#include "dectnrp/sections_part4/mac_messages_and_ie/resource_allocation_ie.hpp"
#include "dectnrp/sections_part4/mac_messages_and_ie/route_info_ie.hpp"
#include "dectnrp/sections_part4/mac_messages_and_ie/user_plane_data.hpp"

// separated as not standard-compliant
#include "dectnrp/sections_part4/mac_messages_and_ie/extensions/forward_to_ie.hpp"
#include "dectnrp/sections_part4/mac_messages_and_ie/extensions/power_target_ie.hpp"
#include "dectnrp/sections_part4/mac_messages_and_ie/extensions/time_announce_ie.hpp"

namespace dectnrp::sp4 {

mmie_pool_tx_t::mmie_pool_tx_t() {
    /* Each MMIE that is already implemented should be contained in the pool at least once. Same
     * order as in Table 6.3.4-2, Table 6.3.4-3 and Table 6.3.4-4.
     */
    // set_nof_elements<padding_ie_t>(1);
    set_nof_elements<higher_layer_signalling_t>(1);
    set_nof_elements<user_plane_data_t>(1);
    // reserved
    set_nof_elements<network_beacon_message_t>(1);
    set_nof_elements<cluster_beacon_message_t>(1);
    set_nof_elements<association_request_message_t>(1);
    set_nof_elements<association_response_message_t>(1);
    set_nof_elements<association_release_message_t>(1);
    set_nof_elements<reconfiguration_request_message_t>(1);
    set_nof_elements<reconfiguration_response_message_t>(1);
    // additional MAC messages
    set_nof_elements<mac_security_info_ie_t>(1);
    set_nof_elements<route_info_ie_t>(1);
    set_nof_elements<resource_allocation_ie_t>(1);
    set_nof_elements<random_access_resource_ie_t>(1);
    set_nof_elements<rd_capability_ie_t>(1);
    set_nof_elements<neighbouring_ie_t>(1);
    set_nof_elements<broadcast_indication_ie_t>(1);
    set_nof_elements<group_assignment_ie_t>(1);
    set_nof_elements<load_info_ie_t>(1);
    set_nof_elements<measurement_report_ie_t>(1);
    // reserved
    // escape
    // IE type extension
    set_nof_elements<configuration_request_ie_t>(1);
    set_nof_elements<radio_device_status_ie_t>(1);
    /*
     * The following MMIEs are not part of the standard, and exist only in this project.
     */
    set_nof_elements<extensions::forward_to_ie_t>(1);
    set_nof_elements<extensions::power_target_ie_t>(1);
    set_nof_elements<extensions::time_announce_ie_t>(1);
}

void mmie_pool_tx_t::fill_with_padding_ies(uint8_t* mac_pdu_offset, uint32_t N_bytes_to_fill) {
    // get padding IE from pool
    padding_ie_t padding_ie;

    // iterate until there are no more padding bytes left
    for (uint32_t N; N_bytes_to_fill;) {
        // set number of bytes to pad in current iteration
        if (N_bytes_to_fill > padding_ie_t::N_PADDING_BYTES_MAX) {
            N = padding_ie_t::N_PADDING_BYTES_MAX;
        } else {
            N = N_bytes_to_fill;
        }

        // configure padding IE and attach to MAC PDU
        padding_ie.set_nof_padding_bytes(N);
        padding_ie.pack_mmh_sdu(mac_pdu_offset);

        const uint32_t mmh_size = padding_ie.get_packed_size_of_mmh_sdu();

        // set MMH payload to zero to avoid retransmission of old data
        std::memset(mac_pdu_offset + mmh_size, 0x0, N - mmh_size);

        // update write pointer and number of bytes left to pad
        mac_pdu_offset += N;
        N_bytes_to_fill -= N;
    }
}

}  // namespace dectnrp::sp4
