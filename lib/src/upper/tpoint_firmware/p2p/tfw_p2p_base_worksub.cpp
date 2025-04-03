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

#include "dectnrp/upper/tpoint_firmware/p2p/tfw_p2p_base.hpp"
//

#include "dectnrp/common/prog/log.hpp"
#include "dectnrp/limits.hpp"

namespace dectnrp::upper::tfw::p2p {

bool tfw_p2p_base_t::worksub_tx_unicast(phy::machigh_phy_t& machigh_phy,
                                        const mac::allocation::tx_opportunity_t& tx_opportunity,
                                        const uint32_t conn_idx) {
    // first check if there even is any data to transmit
    const auto items_level_report = app_server->get_items_level_report_try(
        conn_idx, limits::max_nof_user_plane_data_per_mac_pdu);

    // if not, return immediately
    if (items_level_report.N_filled == 0) {
        return false;
    }

    // OPTIONAL: change packet dimensions of unicast (psdef)
    // -

    // request harq process
    auto* hp_tx = hpp->get_process_tx(plcf_mht_mch_unicast.plcf_base_effective->get_Type(),
                                      identity_ft.NetworkID,
                                      plcf_mht_mch_unicast.psdef,
                                      phy::harq::finalize_tx_t::reset_and_terminate);

    // every firmware has to decide how to deal with unavailable HARQ process
    if (hp_tx == nullptr) {
        dectnrp_log_wrn("HARQ process TX unavailable");
        return false;
    }

    // this is now a well-defined packet size
    const section3::packet_sizes_t& packet_sizes = hp_tx->get_packet_sizes();

    // pack first headers for DEC NR+ packet
    uint32_t a_cnt_w =
        plcf_mht_mch_unicast.pack_first_3_header(hp_tx->get_a_plcf(), hp_tx->get_a_tb());

    // then attach as many user plane data MMIEs as possible
    for (uint32_t i = 0; i < items_level_report.N_filled; ++i) {
        // request ...
        auto& upd = mmie_pool_tx.get<section4::user_plane_data_t>();

        // ... and configure user plane data MMIE
        upd.set_flow_id(1);
        upd.set_data_size(items_level_report.levels[i]);

        // make sure user plane data still fits into the transport block
        if (packet_sizes.N_TB_byte <= a_cnt_w + upd.get_packed_size_of_mmh_sdu()) {
            break;
        }

        // pack header and ...
        upd.pack_mmh_sdu(hp_tx->get_a_tb() + a_cnt_w);

        // ... advance pointer by size of packed MAC PDU and ...
        const uint32_t a_cnt_w_inc = upd.get_packed_size_of_mmh_sdu();

        // ... request payload pointer and ...
        uint8_t* const dst_payload = upd.get_data_ptr();

        dectnrp_assert(dst_payload - hp_tx->get_a_tb() + items_level_report.levels[i] <=
                           packet_sizes.N_TB_byte,
                       "MAC PDU too large");

        // ... try reading data from upper layer to MMIE
        if (app_server->read_try(conn_idx, dst_payload) == 0) {
            break;
        }

        // increment counter only if we actually wrote data to the MMIE
        a_cnt_w += a_cnt_w_inc;
    }

    // in case not user plane data was written
    if (plcf_mht_mch_unicast.get_packed_size_mht_mch() == a_cnt_w) {
        hp_tx->finalize();
        return false;
    }

    // fill up with padding IEs
    mmie_pool_tx.fill_with_padding_ies(hp_tx->get_a_tb() + a_cnt_w,
                                       packet_sizes.N_TB_byte - a_cnt_w);

    // pick beamforming codebook index, for beacon usually 0 to be used for channel sounding
    const uint32_t codebook_index = 0;

    // required meta data on radio layer
    const radio::buffer_tx_meta_t buffer_tx_meta = {.tx_order_id = tx_order_id,
                                                    .tx_time_64 = tx_opportunity.tx_time_64};

    // add to transmit vector
    machigh_phy.tx_descriptor_vec.push_back(
        phy::tx_descriptor_t(*hp_tx, codebook_index, plcf_mht_mch_unicast.tx_meta, buffer_tx_meta));

    // increase tx order id
    ++tx_order_id;

    // set end of packet as earliest next transmission time
    tx_earliest_64 = tx_opportunity.get_end();

    return true;
}

}  // namespace dectnrp::upper::tfw::p2p
