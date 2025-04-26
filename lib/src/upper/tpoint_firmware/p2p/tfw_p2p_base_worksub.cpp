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

#include <cmath>

#include "dectnrp/common/prog/log.hpp"
#include "dectnrp/limits.hpp"
#include "dectnrp/sections_part3/beamforming_and_antenna_port_mapping.hpp"

namespace dectnrp::upper::tfw::p2p {

#ifdef TFW_P2P_EXPORT_PPX
void tfw_p2p_base_t::worksub_callback_ppx(const int64_t now_64,
                                          const size_t idx,
                                          int64_t& next_64) {
    const auto pulse_config = ppx.get_ppx_imminent();

    hw.schedule_pulse_tc(pulse_config);

    dectnrp_assert(now_64 < pulse_config.rising_edge_64, "time out-of-order");
    dectnrp_assert(pulse_config.rising_edge_64 < now_64 + ppx.get_ppx_period_warped(),
                   "time out-of-order");

    ppx.extrapolate_next_rising_edge();

    dectnrp_assert(now_64 + ppx.get_ppx_period_warped() < ppx.get_ppx_imminent().rising_edge_64,
                   "time out-of-order");

    dectnrp_assert(
        std::abs(pulse_config.rising_edge_64 - ppx.get_ppx_time_advance_samples() - next_64) <
            duration_lut.get_N_samples_from_duration(section3::duration_ec_t::ms001, 5),
        "callback adjustment time too large");

    // set time of next callback
    next_64 = pulse_config.rising_edge_64 - ppx.get_ppx_time_advance_samples();
}
#endif

bool tfw_p2p_base_t::worksub_tx_unicast(phy::machigh_phy_t& machigh_phy,
                                        contact_p2p_t& contact_p2p,
                                        const mac::allocation::tx_opportunity_t& tx_opportunity) {
    // first check if there even is any data to transmit
    const auto queue_level = app_server->get_queue_level_nto(
        contact_p2p.conn_idx_server, limits::max_nof_user_plane_data_per_mac_pdu);

    // if not, return immediately
    if (queue_level.N_filled == 0) {
        return false;
    }

    // define an expiration time for all instances of expiring_t
    const int64_t expiration_64 =
        tx_opportunity.tx_time_64 -
        duration_lut.get_N_samples_from_duration(section3::duration_ec_t::ms001, 50);

    worksub_tx_unicast_psdef(contact_p2p, expiration_64);

    auto* hp_tx = hpp->get_process_tx(ppmp_unicast.plcf_base_effective->get_Type(),
                                      identity_ft.NetworkID,
                                      ppmp_unicast.psdef,
                                      phy::harq::finalize_tx_t::reset_and_terminate);

    // every firmware has to decide how to deal with unavailable HARQ process
    if (hp_tx == nullptr) {
        dectnrp_log_wrn("HARQ process TX unavailable");
        return false;
    }

    // this is now a well-defined packet size
    const auto& packet_sizes = hp_tx->get_packet_sizes();

    // update values in PLCF header
    ppmp_unicast.plcf_21.DFMCS = ppmp_unicast.psdef.mcs_index;
    ppmp_unicast.plcf_21.set_NumberOfSpatialStreams(packet_sizes.tm_mode.N_SS);

    worksub_tx_unicast_feedback(contact_p2p, expiration_64);

    if (!worksub_tx_unicast_mac_sdu(contact_p2p, queue_level, packet_sizes, *hp_tx)) {
        // no data written to the HARQ buffer, terminate the process and return without sending
        hp_tx->finalize();

        return false;
    }

    contact_p2p.feedback_plan.set_next_feedback_format();

#ifdef TFW_P2P_MIMO
    const uint32_t codebook_index =
        contact_p2p.mimo_csi.codebook_index.get_val_or_fallback(expiration_64, 0);
#else
    const uint32_t codebook_index = 0;
#endif

    // PHY meta
    const phy::tx_meta_t& tx_meta = {.optimal_scaling_DAC = false,
                                     .DAC_scale = agc_tx.get_ofdm_amplitude_factor(),
                                     .iq_phase_rad = 0.0f,
                                     .iq_phase_increment_s2s_post_resampling_rad = 0.0f,
                                     .GI_percentage = 5};

    // radio meta
    const radio::buffer_tx_meta_t buffer_tx_meta = {.tx_order_id = tx_order_id,
                                                    .tx_time_64 = tx_opportunity.tx_time_64};

    ++tx_order_id;
    tx_earliest_64 = tx_opportunity.get_end();

    // add to transmit vector
    machigh_phy.tx_descriptor_vec.push_back(
        phy::tx_descriptor_t(*hp_tx, codebook_index, tx_meta, buffer_tx_meta));

    return true;
}

void tfw_p2p_base_t::worksub_tx_unicast_psdef(contact_p2p_t& contact_p2p,
                                              const int64_t expiration_64) {
    ppmp_unicast.psdef.mcs_index =
        cqi_lut.clamp_mcs(contact_p2p.mimo_csi.feedback_MCS.get_val_or_fallback(
            expiration_64, cqi_lut.get_mcs_min()));

    // update transmission mode
    // ToDo
}

void tfw_p2p_base_t::worksub_tx_unicast_feedback(contact_p2p_t& contact_p2p,
                                                 const int64_t expiration_64) {
    // set next feedback format in PLCF
    ppmp_unicast.plcf_21.FeedbackFormat = contact_p2p.feedback_plan.get_current_feedback_format();

    // update respective feedback format
    switch (ppmp_unicast.plcf_21.FeedbackFormat) {
        case 4:
            ppmp_unicast.plcf_21.feedback_info_pool.feedback_info_f4.MCS =
                contact_p2p.mimo_csi.phy_MCS.get_val_or_fallback(expiration_64,
                                                                 cqi_lut.get_mcs_min());

            break;

        case 5:
            ppmp_unicast.plcf_21.feedback_info_pool.feedback_info_f5.Codebook_index =
                contact_p2p.mimo_csi.phy_codebook_index.get_val_or_fallback(expiration_64, 0);

            break;
    }
}

bool tfw_p2p_base_t::worksub_tx_unicast_mac_sdu(const contact_p2p_t& contact_p2p,
                                                const application::queue_level_t& queue_level,
                                                const section3::packet_sizes_t& packet_sizes,
                                                phy::harq::process_tx_t& hp_tx) {
    uint32_t a_cnt_w = ppmp_unicast.pack_first_3_header(hp_tx.get_a_plcf(), hp_tx.get_a_tb());

    // then attach as many user plane data MMIEs as possible
    for (uint32_t i = 0; i < queue_level.N_filled; ++i) {
        // request ...
        auto& upd = mmie_pool_tx.get<section4::user_plane_data_t>();

        // ... and configure user plane data MMIE
        upd.set_flow_id(1);
        upd.set_data_size(queue_level.levels[i]);

        // make sure user plane data still fits into the transport block
        if (packet_sizes.N_TB_byte <= a_cnt_w + upd.get_packed_size_of_mmh_sdu()) {
            break;
        }

        upd.pack_mmh_sdu(hp_tx.get_a_tb() + a_cnt_w);

        const uint32_t a_cnt_w_inc = upd.get_packed_size_of_mmh_sdu();

        // request payload pointer and ...
        uint8_t* const dst_payload = upd.get_data_ptr();

        dectnrp_assert(
            dst_payload - hp_tx.get_a_tb() + queue_level.levels[i] <= packet_sizes.N_TB_byte,
            "MAC PDU too large");

        // ... try reading data from upper layer to MMIE
        if (app_server->read_nto(contact_p2p.conn_idx_server, dst_payload) == 0) {
            break;
        }

        a_cnt_w += a_cnt_w_inc;
    }

    // in case no user plane data was written
    if (ppmp_unicast.get_packed_size_mht_mch() == a_cnt_w) {
        return false;
    }

    mmie_pool_tx.fill_with_padding_ies(hp_tx.get_a_tb() + a_cnt_w,
                                       packet_sizes.N_TB_byte - a_cnt_w);

    return true;
}

}  // namespace dectnrp::upper::tfw::p2p
