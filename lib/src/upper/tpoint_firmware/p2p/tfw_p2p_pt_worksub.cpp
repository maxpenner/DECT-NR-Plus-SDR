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

#include "dectnrp/upper/tpoint_firmware/p2p/tfw_p2p_pt.hpp"
//

#include <span>
#include <utility>

#include "dectnrp/common/adt/miscellaneous.hpp"
#include "dectnrp/common/prog/assert.hpp"
#include "dectnrp/common/prog/log.hpp"

namespace dectnrp::upper::tfw::p2p {

std::optional<phy::maclow_phy_t> tfw_p2p_pt_t::worksub_pcc_10(const phy::phy_maclow_t& phy_maclow) {
    // cast guaranteed to work
    const auto* plcf_10 = static_cast<const section4::plcf_10_t*>(
        phy_maclow.pcc_report.plcf_decoder.get_plcf_base(1));

    dectnrp_assert(plcf_10 != nullptr, "cast ill-formed");

    // is this a packet from the correct network, and from the FT?
    if (plcf_10->ShortNetworkID != identity_pt.ShortNetworkID ||
        plcf_10->TransmitterIdentity != identity_ft.ShortRadioDeviceID) {
        return std::nullopt;
    }

    // At this point, it is practically certain that the received packet is a beacon with PLCF type
    // 1 and format 0 from the FT.

    ++stats.beacon_cnt;

    // it's the beacon, so update beacon time
    allocation_pt.set_beacon_time_last_known(phy_maclow.sync_report.fine_peak_time_64);

#ifdef TFW_P2P_PT_AGC_ENABLED
#ifdef TFW_P2P_PT_AGC_CHANGE_TIMED_OR_IMMEDIATE_PT
    // apply AGC change for RX and TX immediately before the next beacon
    const int64_t t_agc_change_64 = allocation_pt.get_beacon_time_last_known() +
                                    allocation_pt.get_beacon_period() -
                                    hw.get_tmin_samples(radio::hw_t::tmin_t::gain);
#else
    // immediate AGC gain change
    const int64_t t_agc_change_64 = common::adt::UNDEFINED_EARLY_64;
#endif

    // apply AGC settings
    worksub_agc(phy_maclow.sync_report, *plcf_10, t_agc_change_64);
#endif

    return worksub_pcc2pdc(
        phy_maclow,
        1,
        identity_ft.NetworkID,
        0,
        phy::harq::finalize_rx_t::reset_and_terminate,
        phy::maclow_phy_handle_t(phy::handle_pcc2pdc_t::th10, identity_ft.ShortRadioDeviceID));
}

phy::maclow_phy_t tfw_p2p_pt_t::worksub_pcc_20(const phy::phy_maclow_t& phy_maclow) {
    return phy::maclow_phy_t();
}

phy::maclow_phy_t tfw_p2p_pt_t::worksub_pcc_21(const phy::phy_maclow_t& phy_maclow) {
    // cast guaranteed to work
    const auto* plcf_21 = static_cast<const section4::plcf_21_t*>(
        phy_maclow.pcc_report.plcf_decoder.get_plcf_base(2));

    dectnrp_assert(plcf_21 != nullptr, "cast ill-formed");

    // is this a packet from the correct network, from the FT and for this PT?
    if (plcf_21->ShortNetworkID != identity_pt.ShortNetworkID ||
        plcf_21->TransmitterIdentity != identity_ft.ShortRadioDeviceID ||
        plcf_21->ReceiverIdentity != identity_pt.ShortRadioDeviceID) {
        return phy::maclow_phy_t();
    }

    return worksub_pcc2pdc(
        phy_maclow,
        2,
        identity_ft.NetworkID,
        0,
        phy::harq::finalize_rx_t::reset_and_terminate,
        phy::maclow_phy_handle_t(phy::handle_pcc2pdc_t::th21, identity_ft.ShortRadioDeviceID));
}

phy::machigh_phy_t tfw_p2p_pt_t::worksub_pdc_10(const phy::phy_machigh_t& phy_machigh) {
    // readability
    const auto& mac_pdu_decoder = phy_machigh.pdc_report.mac_pdu_decoder;

    // request vector with base pointer to all decoded MMIEs
    const auto& mmie_decoded_vec = mac_pdu_decoder.get_mmie_decoded_vec();

    // go over each MMIE
    for (auto mmie : mmie_decoded_vec) {
        if (const auto* mmie_child = dynamic_cast<const section4::cluster_beacon_message_t*>(mmie);
            mmie_child != nullptr) {
            ASSERT_MMIE_COUNT_EXACT(mac_pdu_decoder, mmie_child, 1);
            worksub_mmie_cluster_beacon_message(phy_machigh, *mmie_child);
            continue;
        }

        if (const auto* mmie_child =
                dynamic_cast<const section4::extensions::time_announce_ie_t*>(mmie);
            mmie_child != nullptr) {
            ASSERT_MMIE_COUNT_EXACT(mac_pdu_decoder, mmie_child, 1);
            worksub_mmie_time_announce(phy_machigh, *mmie_child);
            continue;
        }
    }

#ifdef TFW_P2P_EXPORT_PPX
    if (ppx.has_ppx_rising_edge()) {
        ppx.provide_beacon_time(phy_machigh.phy_maclow.sync_report.fine_peak_time_64);
    }
#endif

    // update MCS sent to FT as feedback
    ppmp_unicast.plcf_21.feedback_info_pool.feedback_info_f4.MCS =
        cqi_lut.get_highest_mcs_possible(phy_machigh.pdc_report.snr_dB);

    // update codebook index sent to FT as feedback
    ppmp_unicast.plcf_21.feedback_info_pool.feedback_info_f5.Codebook_index =
        phy_machigh.pdc_report.mimo_report.tm_3_7_beamforming_idx;

    // convert MIMO report to CSI
    mimo_csi.update(phy_machigh.pdc_report.mimo_report, phy_machigh.phy_maclow.sync_report);

    // check if we can generate any uplink
    phy::machigh_phy_t machigh_phy;

    worksub_tx_unicast_consecutive(machigh_phy);

    return machigh_phy;
}

phy::machigh_phy_t tfw_p2p_pt_t::worksub_pdc_20(const phy::phy_machigh_t& phy_machigh) {
    return phy::machigh_phy_t();
}

phy::machigh_phy_t tfw_p2p_pt_t::worksub_pdc_21(const phy::phy_machigh_t& phy_machigh) {
    // readability
    const auto& mac_pdu_decoder = phy_machigh.pdc_report.mac_pdu_decoder;

    // request vector with base pointer to MMIE
    const auto& mmie_decoded_vec = mac_pdu_decoder.get_mmie_decoded_vec();

    // count datagrams to be forwarded
    uint32_t datagram_cnt = 0;

    for (auto mmie : mmie_decoded_vec) {
        if (const auto* mmie_child = dynamic_cast<const section4::user_plane_data_t*>(mmie);
            mmie_child != nullptr) {
            // submit to app_client
            if (app_client->write_try(0, mmie_child->get_data_ptr(), mmie_child->get_data_size()) >
                0) {
                ++datagram_cnt;
            }

            continue;
        }

        dectnrp_log_wrn("MMIE not user plane data");
    }

    app_client->trigger_forward_nto(datagram_cnt);

    return phy::machigh_phy_t();
}

void tfw_p2p_pt_t::worksub_tx_unicast_consecutive(phy::machigh_phy_t& machigh_phy) {
    // number of definable packets is limited
    for (uint32_t i = 0; i < max_simultaneous_tx_unicast; ++i) {
        // find next transmission opportunity
        const auto tx_opportunity = allocation_pt.get_tx_opportunity(
            mac::allocation::direction_t::uplink, buffer_rx.get_rx_time_passed(), tx_earliest_64);

        // if no opportunity found, leave machigh_phy as is
        if (tx_opportunity.tx_time_64 < 0) {
            break;
        }

        // change feedback info in PLCF
        ppmp_unicast.plcf_21.FeedbackFormat = ppmp_unicast.plcf_21.FeedbackFormat == 4 ? 5 : 4;

        // try to send a packet, may return false if no data of HARQ processes are available
        if (!worksub_tx_unicast(machigh_phy, tx_opportunity, mimo_csi, 0)) {
            break;
        }
    }

    return;
}

void tfw_p2p_pt_t::worksub_mmie_cluster_beacon_message(
    const phy::phy_machigh_t& phy_machigh,
    const section4::cluster_beacon_message_t& cluster_beacon_message) {
    // ToDo
}

void tfw_p2p_pt_t::worksub_mmie_time_announce(
    const phy::phy_machigh_t& phy_machigh,
    const section4::extensions::time_announce_ie_t& time_announce_ie) {
#ifdef TFW_P2P_EXPORT_PPX
    // is this the first time_announce_ie ever received?
    if (!ppx.has_ppx_rising_edge()) {
        // initialize with first known PPX rising edge
        ppx.set_ppx_rising_edge(phy_machigh.phy_maclow.sync_report.fine_peak_time_64);

        // when is the next PPX due?
        const int64_t A =
            phy_machigh.phy_maclow.sync_report.fine_peak_time_64 + ppx.get_ppx_period_samples();

        callbacks.add_callback(std::bind(&tfw_p2p_pt_t::worksub_callback_ppx,
                                         this,
                                         std::placeholders::_1,
                                         std::placeholders::_2,
                                         std::placeholders::_3),
                               A - ppx.get_ppx_time_advance_samples(),
                               ppx.get_ppx_period_samples());
    }
#endif
}

void tfw_p2p_pt_t::worksub_callback_log(const int64_t now_64) const {
    std::string str = "id=" + std::to_string(id) + " ";

    str += stats.get_as_string();

    str += "tx_power_ant_0dBFS=" + std::to_string(agc_tx.get_power_ant_0dBFS(now_64)) + " ";
    str += "rx_power_ant_0dBFS=" + agc_rx.get_power_ant_0dBFS(now_64).get_readable_list() + " ";
    str += "rx_rms=" + agc_rx.get_rms_measured_last_known().get_readable_list();

    dectnrp_log_inf("{}", str);
}

}  // namespace dectnrp::upper::tfw::p2p
