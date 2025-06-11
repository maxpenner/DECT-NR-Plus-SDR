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

#include "dectnrp/upper/p2p/procedure/steady_rd.hpp"

#include <cmath>

#include "dectnrp/common/prog/log.hpp"
#include "dectnrp/limits.hpp"
#include "dectnrp/sections_part4/mac_messages_and_ie/user_plane_data.hpp"

namespace dectnrp::upper::tfw::p2p {

steady_rd_t::steady_rd_t(const tpoint_config_t& tpoint_config_,
                         phy::mac_lower_t& mac_lower_,
                         rd_t& rd_)
    : tpoint_t(tpoint_config_, mac_lower_),
      rd(rd_) {
    dectnrp_assert(mac_lower.lower_ctrl_vec.size() == 1,
                   "firmware written for a single pair of physical and radio layer");

    // ##################################################
    // Radio Layer + PHY

    const uint32_t minimum_mcs_allowed = 4;

#ifdef TFW_P2P_VARIABLE_MCS
    rd.cqi_lut = phy::indicators::cqi_lut_t(
        minimum_mcs_allowed, worker_pool_config.radio_device_class.mcs_index_min, 8.0f);
#else
    rd.cqi_lut = phy::indicators::cqi_lut_t(minimum_mcs_allowed, minimum_mcs_allowed, 8.0f);
#endif

    rd.hw_simulator = dynamic_cast<radio::hw_simulator_t*>(&hw);

    // ##################################################
    // MAC Layer

    tpoint_t::hpp =
        std::make_unique<phy::harq::process_pool_t>(worker_pool_config.maximum_packet_sizes, 8, 8);

    rd.identity_ft = sp4::mac_architecture::identity_t(100, 10000000, 1000);

    // how often does the FT send beacons, how often does the PT expect beacons?
    const auto beacon_period = duration_lut.get_duration(sp3::duration_ec_t::ms001, 10);

    rd.allocation_ft = mac::allocation::allocation_ft_t(
        &duration_lut, beacon_period, duration_lut.get_duration(sp3::duration_ec_t::ms001, 2));

    rd.pll = mac::pll_t(beacon_period);

#ifdef TFW_P2P_EXPORT_PPX
    rd.ppx = mac::ppx_t(duration_lut.get_duration(sp3::duration_ec_t::s001),
                        duration_lut.get_duration(sp3::duration_ec_t::ms001, 250),
                        duration_lut.get_duration(sp3::duration_ec_t::ms001, 30),
                        beacon_period,
                        duration_lut.get_duration(sp3::duration_ec_t::ms001, 5));
#endif

    // ##################################################
    // DLC and Convergence Layer
    // -

    // ##################################################
    // Application Layer
    // -
};

phy::maclow_phy_t steady_rd_t::work_pcc(const phy::phy_maclow_t& phy_maclow) {
    ++stats.rx_pcc_success;

    const sp4::plcf_base_t* plcf_base = nullptr;

    std::optional<phy::maclow_phy_t> optret;

    // test PLCF type 1
    if ((plcf_base = phy_maclow.pcc_report.plcf_decoder.get_plcf_base(1)) != nullptr) {
        switch (plcf_base->get_HeaderFormat()) {
            case 0:
                optret = worksub_pcc_10(phy_maclow);
                break;
            case 1:
                [[fallthrough]];
            case 2:
                [[fallthrough]];
            case 3:
                [[fallthrough]];
            case 4:
                [[fallthrough]];
            case 5:
                [[fallthrough]];
            case 6:
                [[fallthrough]];
            case 7:
                optret = std::nullopt;
                break;
            default:
                dectnrp_assert_failure("undefined PLCF type 1 header format");
                optret = std::nullopt;
                break;
        }
    }

    // lambda to overwrite gain settings for JSON file export
    auto inject_currect_gain_settings = [&](phy::maclow_phy_t& maclow_phy) -> void {
        maclow_phy.hw_status.tx_power_ant_0dBFS =
            agc_tx.get_power_ant_0dBFS(phy_maclow.sync_report.fine_peak_time_64);
        maclow_phy.hw_status.rx_power_ant_0dBFS =
            agc_rx.get_power_ant_0dBFS(phy_maclow.sync_report.fine_peak_time_64);
    };

    // if decoded with valid content, return without testing for type 2
    if (optret.has_value()) {
        inject_currect_gain_settings(optret.value());
        return optret.value();
    }

    phy::maclow_phy_t ret;

    // test PLCF type 2
    if (plcf_base = phy_maclow.pcc_report.plcf_decoder.get_plcf_base(2); plcf_base != nullptr) {
        switch (plcf_base->get_HeaderFormat()) {
            case 0:
                ret = worksub_pcc_20(phy_maclow);
                break;
            case 1:
                ret = worksub_pcc_21(phy_maclow);
                break;
            case 2:
                [[fallthrough]];
            case 3:
                [[fallthrough]];
            case 4:
                [[fallthrough]];
            case 5:
                [[fallthrough]];
            case 6:
                [[fallthrough]];
            case 7:
                optret = std::nullopt;
                break;
            default:
                dectnrp_assert_failure("undefined PLCF type 1 header format");
                optret = std::nullopt;
                break;
        }
    }

    inject_currect_gain_settings(ret);

    return ret;
}

phy::machigh_phy_t steady_rd_t::work_pdc_async(const phy::phy_machigh_t& phy_machigh) {
    // ignore entire PDC if CRC is incorrect
    if (!phy_machigh.pdc_report.crc_status) {
        ++stats.rx_pdc_fail;
        return phy::machigh_phy_t();
    }

    ++stats.rx_pdc_success;

    // ignore PDC content if no proper MMIE is decoded
    if (!phy_machigh.pdc_report.mac_pdu_decoder.has_any_data()) {
        return phy::machigh_phy_t();
    }

    ++stats.rx_pdc_has_mmie;

    dectnrp_assert(
        phy_machigh.maclow_phy.get_handle_pcc2pdc() != phy::handle_pcc2pdc_t::CARDINALITY,
        "handle out of range");

    // call routine based on handle given during PCC processing
    switch (phy_machigh.maclow_phy.get_handle_pcc2pdc()) {
        using enum phy::handle_pcc2pdc_t;
        case th10:
            return worksub_pdc_10(phy_machigh);
        case th20:
            return worksub_pdc_20(phy_machigh);
        case th21:
            return worksub_pdc_21(phy_machigh);
        default:
            dectnrp_assert(false, "pcc2pdc not handled");
            break;
    }

    return phy::machigh_phy_t();
}

sp4::mac_architecture::identity_t steady_rd_t::init_identity_pt(const uint32_t firmware_id_) {
    // load identity of FT ...
    auto identity = rd.identity_ft;

    // ... and increment long and short radio device ID by at least one
    identity.LongRadioDeviceID += 1 + firmware_id_;
    identity.ShortRadioDeviceID += 1 + firmware_id_;

    return identity;
}

mac::allocation::allocation_pt_t steady_rd_t::init_allocation_pt(const uint32_t firmware_id_) {
    mac::allocation::allocation_pt_t allocation_pt =
        mac::allocation::allocation_pt_t(&duration_lut,
                                         rd.allocation_ft.get_beacon_period_as_duration(),
                                         duration_lut.get_duration(sp3::duration_ec_t::ms001, 16),
                                         duration_lut.get_duration(sp3::duration_ec_t::ms001, 11),
                                         hw.get_tmin_samples(radio::hw_t::tmin_t::turnaround));

    /* If firmware ID is larger than the number of PTs we want to support, we simply leave the
     * allocation empty. This way we can use the same code for different number of PTs, which is
     * particularly important when running a simulation.
     */
    if (rd.N_pt <= firmware_id_) {
        return allocation_pt;
    }

    // gap for better TX/RX switching and synchronization (may reduce N_resources)
    const uint32_t gap = 0;

    // frame offset to fit beacon
    const uint32_t offset_beacon = 2 + gap;

    // same for every PT
    const uint32_t ul = 2;
    const uint32_t dl = 2;
    const uint32_t ul_gap = ul + gap;
    const uint32_t dl_gap = dl + gap;
    const uint32_t ul_gap_dl_gap = ul_gap + dl_gap;
    const uint32_t stride = ul_gap_dl_gap * rd.N_pt;

#ifdef APPLICATION_INTERFACE_VNIC_OR_SOCKET
    const uint32_t N_resources = 4;
#else
    const uint32_t N_resources = 2;
#endif

    // PT specific
    const uint32_t offset = offset_beacon + firmware_id_ * ul_gap_dl_gap;

    allocation_pt.add_resource_regular(mac::allocation::direction_t::uplink,
                                       offset,
                                       ul,
                                       stride,
                                       N_resources,
                                       sp3::duration_ec_t::slot001);

    allocation_pt.add_resource_regular(mac::allocation::direction_t::downlink,
                                       offset + ul_gap,
                                       dl,
                                       stride,
                                       N_resources,
                                       sp3::duration_ec_t::slot001);

    return allocation_pt;
}

void steady_rd_t::init_packet_unicast(const uint32_t ShortRadioDeviceID_tx,
                                      const uint32_t ShortRadioDeviceID_rx,
                                      const uint32_t LongRadioDeviceID_tx,
                                      const uint32_t LongRadioDeviceID_rx) {
    // meta packet size
    sp3::packet_sizes_def_t& psdef = rd.ppmp_unicast.psdef;
    psdef.u = worker_pool_config.radio_device_class.u_min;
    psdef.b = worker_pool_config.radio_device_class.b_min;
    psdef.PacketLengthType = 1;
    psdef.PacketLength = 2;
#ifdef TFW_P2P_MIMO
    psdef.tm_mode_index = sp3::tmmode::get_single_antenna_mode(buffer_rx.nof_antennas);
#else
    psdef.tm_mode_index = 0;
#endif
    psdef.mcs_index = rd.cqi_lut.get_highest_mcs_possible(-1000.0f);
    psdef.Z = worker_pool_config.radio_device_class.Z_min;

    // define PLCFs
    sp4::plcf_21_t& plcf_21 = rd.ppmp_unicast.plcf_21;
    plcf_21.HeaderFormat = 1;
    plcf_21.PacketLengthType = psdef.PacketLengthType;
    plcf_21.set_PacketLength_m1(psdef.PacketLength);
    plcf_21.ShortNetworkID = rd.identity_ft.ShortNetworkID;
    plcf_21.TransmitterIdentity = ShortRadioDeviceID_tx;
    plcf_21.set_TransmitPower(0);
    plcf_21.DFMCS = psdef.mcs_index;
    plcf_21.ReceiverIdentity = ShortRadioDeviceID_rx;
    plcf_21.set_NumberOfSpatialStreams(1);
    plcf_21.Reserved = 0;

    // pick a feedback format
    plcf_21.FeedbackFormat = 5;
    // prepare feedback format 4
    plcf_21.feedback_info_pool.feedback_info_f4.HARQ_feedback_bitmap = 0;
    plcf_21.feedback_info_pool.feedback_info_f4.MCS = psdef.mcs_index;
    // prepare feedback format 5
    plcf_21.feedback_info_pool.feedback_info_f5.HARQ_Process_number = 0;
    plcf_21.feedback_info_pool.feedback_info_f5.Transmission_feedback =
        sp4::feedback_info_f1_t::transmission_feedback_t::ACK;
    plcf_21.feedback_info_pool.feedback_info_f5.MIMO_feedback =
        sp4::feedback_info_f1_t::mimo_feedback_t::single_layer;
    plcf_21.feedback_info_pool.feedback_info_f5.Codebook_index = 0;

    // pick one PLCF
    rd.ppmp_unicast.plcf_base_effective = &rd.ppmp_unicast.plcf_21;

    // define MAC header type
    rd.ppmp_unicast.mac_header_type.Version = sp4::mac_header_type_t::version_ec::v00;
    rd.ppmp_unicast.mac_header_type.MAC_security =
        sp4::mac_header_type_t::mac_security_ec::macsec_not_used;
    rd.ppmp_unicast.mac_header_type.MAC_header_type =
        sp4::mac_header_type_t::mac_header_type_ec::mch_empty;

    // define MAC common header
    rd.ppmp_unicast.unicast_header.Reserved = 0;
    rd.ppmp_unicast.unicast_header.Reset = 1;
    rd.ppmp_unicast.unicast_header.Sequence_number = 0;
    rd.ppmp_unicast.unicast_header.Receiver_Address = LongRadioDeviceID_rx;
    rd.ppmp_unicast.unicast_header.Transmitter_Address = LongRadioDeviceID_tx;

    // pick one MAC common header
    rd.ppmp_unicast.mch_base_effective = &rd.ppmp_unicast.mch_empty;
}

#ifdef TFW_P2P_EXPORT_PPX
void steady_rd_t::worksub_callback_ppx(const int64_t now_64,
                                       [[maybe_unused]] const size_t idx,
                                       int64_t& next_64) {
    const auto pulse_config = rd.ppx.get_ppx_imminent();

    hw.schedule_pulse_tc(pulse_config);

    dectnrp_assert(now_64 < pulse_config.rising_edge_64, "time out-of-order");
    dectnrp_assert(pulse_config.rising_edge_64 < now_64 + rd.ppx.get_ppx_period_warped(),
                   "time out-of-order");

    rd.ppx.extrapolate_next_rising_edge();

    dectnrp_assert(
        now_64 + rd.ppx.get_ppx_period_warped() < rd.ppx.get_ppx_imminent().rising_edge_64,
        "time out-of-order");

    dectnrp_assert(
        std::abs(pulse_config.rising_edge_64 - rd.ppx.get_ppx_time_advance_samples() - next_64) <
            duration_lut.get_N_samples_from_duration(sp3::duration_ec_t::ms001, 5),
        "callback adjustment time too large");

    // set time of next callback
    next_64 = pulse_config.rising_edge_64 - rd.ppx.get_ppx_time_advance_samples();
}
#endif

bool steady_rd_t::worksub_tx_unicast(phy::machigh_phy_t& machigh_phy,
                                     contact_p2p_t& contact_p2p,
                                     const mac::allocation::tx_opportunity_t& tx_opportunity) {
    // first check if there even is any data to transmit
    const auto queue_level = rd.application_server->get_queue_level_nto(
        contact_p2p.conn_idx_server, limits::max_nof_user_plane_data_per_mac_pdu);

    // if not, return immediately
    if (queue_level.N_filled == 0) {
        return false;
    }

    // define an expiration time for all instances of expiring_t
    const int64_t expiration_64 =
        tx_opportunity.tx_time_64 -
        duration_lut.get_N_samples_from_duration(sp3::duration_ec_t::ms001, 50);

    worksub_tx_unicast_psdef(contact_p2p, expiration_64);

    auto* hp_tx = hpp->get_process_tx(rd.ppmp_unicast.plcf_base_effective->get_Type(),
                                      rd.identity_ft.NetworkID,
                                      rd.ppmp_unicast.psdef,
                                      phy::harq::finalize_tx_t::reset_and_terminate);

    // every firmware has to decide how to deal with unavailable HARQ process
    if (hp_tx == nullptr) {
        dectnrp_log_wrn("HARQ process TX unavailable");
        return false;
    }

    // this is now a well-defined packet size
    const auto& packet_sizes = hp_tx->get_packet_sizes();

    // update values in PLCF header
    rd.ppmp_unicast.plcf_21.DFMCS = rd.ppmp_unicast.psdef.mcs_index;
    rd.ppmp_unicast.plcf_21.set_NumberOfSpatialStreams(packet_sizes.tm_mode.N_SS);

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

    const phy::tx_meta_t& tx_meta = {.optimal_scaling_DAC = false,
                                     .DAC_scale = agc_tx.get_ofdm_amplitude_factor(),
                                     .iq_phase_rad = 0.0f,
                                     .iq_phase_increment_s2s_post_resampling_rad = 0.0f,
                                     .GI_percentage = 5};

    const radio::buffer_tx_meta_t buffer_tx_meta = {.tx_order_id = tx_order_id,
                                                    .tx_time_64 = tx_opportunity.tx_time_64};

    ++tx_order_id;
    tx_earliest_64 = tx_opportunity.get_end();

    machigh_phy.tx_descriptor_vec.push_back(
        phy::tx_descriptor_t(*hp_tx, codebook_index, tx_meta, buffer_tx_meta));

    return true;
}

void steady_rd_t::worksub_tx_unicast_psdef(contact_p2p_t& contact_p2p,
                                           const int64_t expiration_64) {
    rd.ppmp_unicast.psdef.mcs_index =
        rd.cqi_lut.clamp_mcs(contact_p2p.mimo_csi.feedback_MCS.get_val_or_fallback(
            expiration_64, rd.cqi_lut.get_mcs_min()));

    // update transmission mode
    // ToDo
}

void steady_rd_t::worksub_tx_unicast_feedback(contact_p2p_t& contact_p2p,
                                              const int64_t expiration_64) {
    // set next feedback format in PLCF
    rd.ppmp_unicast.plcf_21.FeedbackFormat =
        contact_p2p.feedback_plan.get_current_feedback_format();

    // update respective feedback format
    switch (rd.ppmp_unicast.plcf_21.FeedbackFormat) {
        case 4:
            rd.ppmp_unicast.plcf_21.feedback_info_pool.feedback_info_f4.MCS =
                contact_p2p.mimo_csi.phy_MCS.get_val_or_fallback(expiration_64,
                                                                 rd.cqi_lut.get_mcs_min());

            break;

        case 5:
            rd.ppmp_unicast.plcf_21.feedback_info_pool.feedback_info_f5.Codebook_index =
                contact_p2p.mimo_csi.phy_codebook_index.get_val_or_fallback(expiration_64, 0);

            break;
    }
}

bool steady_rd_t::worksub_tx_unicast_mac_sdu(const contact_p2p_t& contact_p2p,
                                             const application::queue_level_t& queue_level,
                                             const sp3::packet_sizes_t& packet_sizes,
                                             phy::harq::process_tx_t& hp_tx) {
    uint32_t a_cnt_w = rd.ppmp_unicast.pack_first_3_header(hp_tx.get_a_plcf(), hp_tx.get_a_tb());

    // then attach as many user plane data MMIEs as possible
    for (uint32_t i = 0; i < queue_level.N_filled; ++i) {
        // request ...
        auto& upd = rd.mmie_pool_tx.get<sp4::user_plane_data_t>();

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
        if (rd.application_server->read_nto(contact_p2p.conn_idx_server, dst_payload) == 0) {
            break;
        }

        a_cnt_w += a_cnt_w_inc;
    }

    // in case no user plane data was written
    if (rd.ppmp_unicast.get_packed_size_mht_mch() == a_cnt_w) {
        return false;
    }

    rd.mmie_pool_tx.fill_with_padding_ies(hp_tx.get_a_tb() + a_cnt_w,
                                          packet_sizes.N_TB_byte - a_cnt_w);

    return true;
}

}  // namespace dectnrp::upper::tfw::p2p
