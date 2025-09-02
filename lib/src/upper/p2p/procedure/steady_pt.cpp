/*
 * Copyright 2023-present Maxim Penner
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

#include "dectnrp/upper/p2p/procedure/steady_pt.hpp"

#include "dectnrp/common/prog/assert.hpp"
#include "dectnrp/common/prog/log.hpp"
#include "dectnrp/sections_part4/mac_messages_and_ie/mmie.hpp"

namespace dectnrp::upper::tfw::p2p {

steady_pt_t::steady_pt_t(args_t& args, pt_t& pt_)
    : steady_rd_t(args),
      pt(pt_) {
    // ##################################################
    // Radio Layer + PHY
    // -

    // ##################################################
    // MAC Layer

    pt.contact_pt.sync_report = phy::sync_report_t(buffer_rx.nof_antennas);
    pt.contact_pt.identity = init_identity_pt(tpoint_config.firmware_id);
    pt.contact_pt.allocation_pt = init_allocation_pt(tpoint_config.firmware_id);
    pt.contact_pt.mimo_csi = phy::mimo_csi_t();
    pt.contact_pt.conn_idx_server = 0;
    pt.contact_pt.conn_idx_client = 0;

    // feedback format 4 for MCS, 5 for codebookindex
    pt.contact_pt.feedback_plan = mac::feedback_plan_t(std::vector<uint32_t>{4, 5});

    init_packet_unicast(pt.contact_pt.identity.ShortRadioDeviceID,
                        rd.identity_ft.ShortRadioDeviceID,
                        pt.contact_pt.identity.LongRadioDeviceID,
                        rd.identity_ft.LongRadioDeviceID);

    // ##################################################
    // DLC and Convergence Layer
    // -

    // ##################################################
    // Application Layer
    // -

    // ##################################################
    // debugging

    // -
}

phy::machigh_phy_t steady_pt_t::work_regular(
    [[maybe_unused]] const phy::regular_report_t& regular_report) {
    return phy::machigh_phy_t();
}

phy::machigh_phy_t steady_pt_t::work_irregular(const phy::irregular_report_t& irregular_report) {
    phy::machigh_phy_t ret;

    if (rd.is_shutting_down()) {
        ret.irregular_report = state_transitions_cb();
        return ret;
    }

    // update time of callbacks
    rd.callbacks.run(buffer_rx.get_rx_time_passed());

    ret.irregular_report =
        irregular_report.get_same_with_time_increment(rd.allocation_ft.get_beacon_period());

    return ret;
}

phy::machigh_phy_t steady_pt_t::work_application(
    [[maybe_unused]] const application::application_report_t& application_report) {
    phy::machigh_phy_t machigh_phy;

    worksub_tx_unicast_consecutive(machigh_phy);

    return machigh_phy;
}

phy::machigh_phy_tx_t steady_pt_t::work_channel([[maybe_unused]] const phy::chscan_t& chscan) {
    return phy::machigh_phy_tx_t();
}

phy::irregular_report_t steady_pt_t::entry() {
    const int64_t now_64 = buffer_rx.get_rx_time_passed();

    // what is the next full second after PHY becomes operational?
    const int64_t A = duration_lut.get_N_samples_at_next_full_second(now_64);

    // initialize regular callback for prints
    rd.callbacks.add_callback(
        std::bind(&steady_pt_t::worksub_callback_log, this, std::placeholders::_1),
        A + duration_lut.get_N_samples_from_duration(sp3::duration_ec_t::s001),
        duration_lut.get_N_samples_from_duration(sp3::duration_ec_t::s001,
                                                 rd.worksub_callback_log_period_sec));

    return phy::irregular_report_t(now_64 + rd.allocation_ft.get_beacon_period());
}

std::optional<phy::maclow_phy_t> steady_pt_t::worksub_pcc_10(const phy::phy_maclow_t& phy_maclow) {
    // cast guaranteed to work
    const auto* plcf_10 =
        static_cast<const sp4::plcf_10_t*>(phy_maclow.pcc_report.plcf_decoder.get_plcf_base(1));

    dectnrp_assert(plcf_10 != nullptr, "cast ill-formed");

    // is this a packet from the correct network, and from the FT?
    if (plcf_10->ShortNetworkID != pt.contact_pt.identity.ShortNetworkID ||
        plcf_10->TransmitterIdentity != rd.identity_ft.ShortRadioDeviceID) {
        return std::nullopt;
    }

    // At this point, it is practically certain that the received packet is a beacon with PLCF type
    // 1 and format 0 from the FT.

    ++stats.beacon_cnt;

    pt.contact_pt.sync_report = phy_maclow.sync_report;

    // it's the beacon, so update beacon time wherever it's needed
    pt.contact_pt.allocation_pt.set_beacon_time_last_known(
        phy_maclow.sync_report.fine_peak_time_64);
    rd.pll.provide_beacon_time(
        phy_maclow.sync_report.fine_peak_time_corrected_by_sto_fractional_64);

#ifdef TFW_P2P_EXPORT_PPX
    if (rd.ppx.has_ppx_rising_edge()) {
        rd.ppx.provide_beacon_time(
            phy_maclow.sync_report.fine_peak_time_corrected_by_sto_fractional_64);
        rd.ppx.set_ppx_period_warped(rd.pll.get_warped(rd.ppx.get_ppx_period_samples()));
    }
#endif

    if (pt.t_agc_xx_last_change_64 +
            duration_lut.get_N_samples_from_duration(sp3::duration_ec_t::ms001,
                                                     TFW_P2P_PT_AGC_MINIMUM_PERIOD_MS) <=
        phy_maclow.sync_report.fine_peak_time_64) {
        // save time
        pt.t_agc_xx_last_change_64 = phy_maclow.sync_report.fine_peak_time_64;

        // AGC change immediately before the next beacon
        const int64_t t_agc_tx_change_64 =
            pt.contact_pt.allocation_pt.get_beacon_time_last_known() +
            pt.contact_pt.allocation_pt.get_beacon_period() -
            hw.get_tmin_samples(radio::hw_t::tmin_t::gain);

#ifdef TFW_P2P_PT_AGC_CHANGE_TIME_SAME_OR_DELAYED
        const int64_t t_agc_rx_change_64 = t_agc_tx_change_64;
#else
        // immediate AGC gain change
        const int64_t t_agc_rx_change_64 =
            t_agc_tx_change_64 + pt.contact_pt.allocation_pt.get_beacon_period();
#endif

        worksub_agc(phy_maclow.sync_report, *plcf_10, t_agc_tx_change_64, t_agc_rx_change_64);
    }

    return worksub_pcc2pdc(
        phy_maclow,
        1,
        rd.identity_ft.NetworkID,
        0,
        phy::harq::finalize_rx_t::reset_and_terminate,
        phy::maclow_phy_handle_t(phy::handle_pcc2pdc_t::th10, rd.identity_ft.ShortRadioDeviceID));
}

phy::maclow_phy_t steady_pt_t::worksub_pcc_20(
    [[maybe_unused]] const phy::phy_maclow_t& phy_maclow) {
    return phy::maclow_phy_t();
}

phy::maclow_phy_t steady_pt_t::worksub_pcc_21(const phy::phy_maclow_t& phy_maclow) {
    // cast guaranteed to work
    const auto* plcf_21 =
        static_cast<const sp4::plcf_21_t*>(phy_maclow.pcc_report.plcf_decoder.get_plcf_base(2));

    dectnrp_assert(plcf_21 != nullptr, "cast ill-formed");

    // is this a packet from the correct network, from the FT and for this PT?
    if (plcf_21->ShortNetworkID != pt.contact_pt.identity.ShortNetworkID ||
        plcf_21->TransmitterIdentity != rd.identity_ft.ShortRadioDeviceID ||
        plcf_21->ReceiverIdentity != pt.contact_pt.identity.ShortRadioDeviceID) {
        return phy::maclow_phy_t();
    }

    pt.contact_pt.mimo_csi.update_from_feedback(
        plcf_21->FeedbackFormat, plcf_21->feedback_info_pool, phy_maclow.sync_report);

    return worksub_pcc2pdc(
        phy_maclow,
        2,
        rd.identity_ft.NetworkID,
        0,
        phy::harq::finalize_rx_t::reset_and_terminate,
        phy::maclow_phy_handle_t(phy::handle_pcc2pdc_t::th21, rd.identity_ft.ShortRadioDeviceID));
}

phy::machigh_phy_t steady_pt_t::worksub_pdc_10(const phy::phy_machigh_t& phy_machigh) {
    // readability
    const auto& mac_pdu_decoder = phy_machigh.pdc_report.mac_pdu_decoder;

    // request vector with base pointer to all decoded MMIEs
    const auto& mmie_decoded_vec = mac_pdu_decoder.get_mmie_decoded_vec();

    for (auto mmie : mmie_decoded_vec) {
        if (const auto* mmie_child = dynamic_cast<const sp4::cluster_beacon_message_t*>(mmie);
            mmie_child != nullptr) {
            ASSERT_MMIE_COUNT_EXACT(mac_pdu_decoder, mmie_child, 1);
            worksub_mmie_cluster_beacon_message(*mmie_child);
            continue;
        }

        if (const auto* mmie_child = dynamic_cast<const sp4::extensions::power_target_ie_t*>(mmie);
            mmie_child != nullptr) {
            ASSERT_MMIE_COUNT_EXACT(mac_pdu_decoder, mmie_child, 1);
            worksub_mmie_power_target(*mmie_child);
            continue;
        }

        if (const auto* mmie_child = dynamic_cast<const sp4::extensions::time_announce_ie_t*>(mmie);
            mmie_child != nullptr) {
            ASSERT_MMIE_COUNT_EXACT(mac_pdu_decoder, mmie_child, 1);
            worksub_mmie_time_announce(phy_machigh.phy_maclow.sync_report.fine_peak_time_64,
                                       *mmie_child);
            continue;
        }

        if (const auto* mmie_child = dynamic_cast<const sp4::resource_allocation_ie_t*>(mmie);
            mmie_child != nullptr) {
            ASSERT_MMIE_COUNT_EXACT(mac_pdu_decoder, mmie_child, 1);
            worksub_mmie_resource_allocation(*mmie_child);
            continue;
        }
    }

    pt.contact_pt.mimo_csi.update_from_phy(phy_machigh.pdc_report.mimo_report,
                                           phy_machigh.phy_maclow.sync_report);

    pt.contact_pt.mimo_csi.update_from_phy(
        rd.cqi_lut.get_highest_mcs_possible(phy_machigh.pdc_report.snr_dB),
        phy_machigh.phy_maclow.sync_report);

    phy::machigh_phy_t machigh_phy;

    // check if we can generate any uplink
    worksub_tx_unicast_consecutive(machigh_phy);

    return machigh_phy;
}

phy::machigh_phy_t steady_pt_t::worksub_pdc_20(
    [[maybe_unused]] const phy::phy_machigh_t& phy_machigh) {
    return phy::machigh_phy_t();
}

phy::machigh_phy_t steady_pt_t::worksub_pdc_21(const phy::phy_machigh_t& phy_machigh) {
    // readability
    const auto& mac_pdu_decoder = phy_machigh.pdc_report.mac_pdu_decoder;

    // request vector with base pointer to MMIE
    const auto& mmie_decoded_vec = mac_pdu_decoder.get_mmie_decoded_vec();

    // count datagrams to be forwarded
    uint32_t datagram_cnt = 0;

    for (auto mmie : mmie_decoded_vec) {
        if (const auto* mmie_child = dynamic_cast<const sp4::user_plane_data_t*>(mmie);
            mmie_child != nullptr) {
            // try submiting to application_client
            if (worksub_mmie_user_plane_data(*mmie_child)) {
                ++datagram_cnt;
            }
            continue;
        }
    }

    rd.application_client->trigger_forward_nto(datagram_cnt);

    pt.contact_pt.mimo_csi.update_from_phy(
        rd.cqi_lut.get_highest_mcs_possible(phy_machigh.pdc_report.snr_dB),
        phy_machigh.phy_maclow.sync_report);

    return phy::machigh_phy_t();
}

void steady_pt_t::worksub_tx_unicast_consecutive(phy::machigh_phy_t& machigh_phy) {
    // number of definable packets is limited
    for (uint32_t i = 0; i < rd.max_simultaneous_tx_unicast; ++i) {
        // find next transmission opportunity
        const auto tx_opportunity = pt.contact_pt.allocation_pt.get_tx_opportunity(
            mac::allocation::direction_t::ul, buffer_rx.get_rx_time_passed(), tx_earliest_64);

        // if no opportunity found, leave machigh_phy as is
        if (tx_opportunity.tx_time_64 < 0) {
            break;
        }

        // try to send a packet, may return false if no data of HARQ processes are available
        if (!worksub_tx_unicast(machigh_phy, pt.contact_pt, tx_opportunity)) {
            break;
        }
    }

    return;
}

void steady_pt_t::worksub_mmie_cluster_beacon_message(
    [[maybe_unused]] const sp4::cluster_beacon_message_t& cbm) {}

void steady_pt_t::worksub_mmie_power_target(const sp4::extensions::power_target_ie_t& ptie) {
    agc_tx.set_rx_dBm_target(ptie.get_power_target_dBm_uncoded());
}

void steady_pt_t::worksub_mmie_time_announce(
    [[maybe_unused]] const int64_t fine_peak_time_64,
    [[maybe_unused]] const sp4::extensions::time_announce_ie_t& taie) {
#ifdef TFW_P2P_EXPORT_PPX
    // is this the first time_announce_ie ever received?
    if (!rd.ppx.has_ppx_rising_edge()) {
        // initialize with first known PPX rising edge
        rd.ppx.set_ppx_rising_edge(fine_peak_time_64);

        // when is the next PPX due?
        const int64_t A = fine_peak_time_64 + rd.ppx.get_ppx_period_warped();

        rd.callbacks.add_callback(std::bind(&steady_pt_t::worksub_callback_ppx,
                                            this,
                                            std::placeholders::_1,
                                            std::placeholders::_2,
                                            std::placeholders::_3),
                                  A - rd.ppx.get_ppx_time_advance_samples(),
                                  rd.ppx.get_ppx_period_warped());
    }
#endif
}

bool steady_pt_t::worksub_mmie_user_plane_data(const sp4::user_plane_data_t& upd) {
    return rd.application_client->write_nto(
               pt.contact_pt.conn_idx_client, upd.get_data_ptr(), upd.get_data_size()) > 0;
}

void steady_pt_t::worksub_mmie_resource_allocation(
    [[maybe_unused]] const sp4::resource_allocation_ie_t& raie) {}

void steady_pt_t::worksub_callback_log([[maybe_unused]] const int64_t now_64) const {
    std::string str = "id=" + std::to_string(id) + " ";

    str += stats.get_as_string();

    str += "tx_power_ant_0dBFS=" + hw.get_tx_power_ant_0dBFS().get_readable_list() + " ";
    str += "rx_power_ant_0dBFS=" + hw.get_rx_power_ant_0dBFS().get_readable_list() + " ";
    str += "rx_rms=" + pt.contact_pt.sync_report.rms_array.get_readable_list();

    dectnrp_log_inf("{}", str);
}

}  // namespace dectnrp::upper::tfw::p2p
