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

#ifdef TFW_P2P_EXPORT_1PPS
void tfw_p2p_pt_t::worksub_callback_pps(const int64_t now_64, const size_t idx, int64_t& next_64) {
    // when is the next PPS due?
    const auto pulse_config = ppx_pll.get_ppx_imminent(now_64);

    // send commands to hardware
    hw.schedule_pulse_tc(pulse_config);

    ppx_pll.set_ppx_time_extrapolation(now_64);

    // adjust time of next callback
    next_64 = pulse_config.rising_edge_64 - ppx_pll.get_ppx_time_advance_samples();
}

void tfw_p2p_pt_t::worksub_pps_first_beacon(const int64_t fine_peak_time_64) {
    // when is the next time alignment beacon due?
    const int64_t A = fine_peak_time_64 + ppx_pll.get_ppx_period_samples();

    // the callback for the PPS has to be called slightly earlier to queue up the GPIO commands
    const int64_t B = A - ppx_pll.get_ppx_time_advance_samples();

    // put callback in queue
    callbacks.add_callback(std::bind(&tfw_p2p_pt_t::worksub_callback_pps,
                                     this,
                                     std::placeholders::_1,
                                     std::placeholders::_2,
                                     std::placeholders::_3),
                           B,
                           duration_lut.get_N_samples_from_duration(section3::duration_ec_t::s001));

    // init PLL
    ppx_pll.set_ppx_time(fine_peak_time_64);
}
#endif

std::optional<phy::maclow_phy_t> tfw_p2p_pt_t::worksub_pcc_10(const phy::phy_maclow_t& phy_maclow) {
    // cast guaranteed to work
    const auto* plcf_10 = static_cast<const section4::plcf_10_t*>(
        phy_maclow.pcc_report.plcf_decoder.get_plcf_base(1));

    dectnrp_assert(plcf_10 != nullptr, "cast ill-formed");

    // must have the correct network, and a known TransmitterIdentity
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
    // apply AGC change for RX and TX just before the next beacon
    const int64_t t_agc_change_64 =
        allocation_pt.get_beacon_time_last_known() + allocation_pt.get_beacon_period() -
        duration_lut.get_N_samples_from_duration(section3::duration_ec_t::settling_time_gain_us);
#else
    // immediate AGC gain change
    const int64_t t_agc_change_64 = common::adt::UNDEFINED_EARLY_64;
#endif

    // make AGC settings based on the received beacon
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

std::optional<phy::maclow_phy_t> tfw_p2p_pt_t::worksub_pcc_11(const phy::phy_maclow_t& phy_maclow) {
    return std::nullopt;
}

phy::maclow_phy_t tfw_p2p_pt_t::worksub_pcc_20(const phy::phy_maclow_t& phy_maclow) {
    return phy::maclow_phy_t();
}

phy::maclow_phy_t tfw_p2p_pt_t::worksub_pcc_21(const phy::phy_maclow_t& phy_maclow) {
    // cast guaranteed to work
    const auto* plcf_21 = static_cast<const section4::plcf_21_t*>(
        phy_maclow.pcc_report.plcf_decoder.get_plcf_base(2));

    dectnrp_assert(plcf_21 != nullptr, "cast ill-formed");

    // must have the correct ReceiverIdentity, and a known TransmitterIdentity
    if (plcf_21->ReceiverIdentity != identity_pt.ShortRadioDeviceID) {
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

    // request vector with base pointer to MMIE
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

#ifdef TFW_P2P_EXPORT_1PPS
    if (stats.beacon_cnt > 1) [[likely]] {
        ppx_pll.set_ppx_time_in_raster(phy_machigh.phy_maclow.sync_report.fine_peak_time_64);
    } else [[unlikely]] {
        dectnrp_assert(stats.beacon_cnt == 1, "must be first beacon received");

        // for now, we assume the first ever received beacon is the only master beacon
        worksub_pps_first_beacon(phy_machigh.phy_maclow.sync_report.fine_peak_time_64);
    }
#endif

    // check if we can generate any uplink
    phy::machigh_phy_t machigh_phy;

    worksub_tx_unicast_consecutive(machigh_phy);

    return machigh_phy;
}

phy::machigh_phy_t tfw_p2p_pt_t::worksub_pdc_11(const phy::phy_machigh_t& phy_machigh) {
    return phy::machigh_phy_t();
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

        // we have an opportunity, but no more HARQ processes or data
        if (!worksub_tx_unicast(machigh_phy, tx_opportunity, 0)) {
            break;
        }
    }

    return;
}

void tfw_p2p_pt_t::worksub_mmie_cluster_beacon_message(
    const phy::phy_machigh_t& phy_machigh,
    const section4::cluster_beacon_message_t& cluster_beacon_message) {
    //
}

void tfw_p2p_pt_t::worksub_mmie_time_announce(
    const phy::phy_machigh_t& phy_machigh,
    const section4::extensions::time_announce_ie_t& time_announce_ie) {
    //
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
