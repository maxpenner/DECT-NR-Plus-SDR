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

#include "dectnrp/upper/p2p/procedure/steady_ft.hpp"

#include "dectnrp/common/prog/assert.hpp"
#include "dectnrp/common/prog/log.hpp"
#include "dectnrp/sections_part4/mac_messages_and_ie/cluster_beacon_message.hpp"
#include "dectnrp/sections_part4/mac_messages_and_ie/extensions//time_announce_ie.hpp"
#include "dectnrp/sections_part4/mac_messages_and_ie/user_plane_data.hpp"

namespace dectnrp::upper::tfw::p2p {

steady_ft_t::steady_ft_t(args_t& args, ft_t& ft_)
    : steady_rd_t(args),
      ft(ft_) {
#ifdef TFW_P2P_MIMO
    dectnrp_assert(
        1 < buffer_rx.nof_antennas,
        "MIMO requires an FT able to transmit at least two transmit streams. Change to a radio device class with N_TX larger one.");
#endif

    // ##################################################
    // Radio Layer + PHY
    // -

    // ##################################################
    // MAC Layer

    ft.contact_list.reserve(10);

    // init contact list
    for (uint32_t firmware_id_pt = 0; firmware_id_pt < rd.N_pt; ++firmware_id_pt) {
        // load identity of one PT
        const auto identity_pt = init_identity_pt(firmware_id_pt);

        // connection index is the firmware ID
        const uint32_t conn_idx_server{firmware_id_pt};
        const uint32_t conn_idx_client{firmware_id_pt};

        // add PT as new contact
        ft.contact_list.add_new_contact_and_setup_indexing(
            identity_pt, conn_idx_server, conn_idx_client);

        auto& contact = ft.contact_list.get_contact(identity_pt.LongRadioDeviceID);

        contact.sync_report = phy::sync_report_t(buffer_rx.nof_antennas);
        contact.identity = identity_pt;
        contact.allocation_pt = init_allocation_pt(firmware_id_pt);
        contact.mimo_csi = phy::mimo_csi_t();
        contact.conn_idx_server = conn_idx_server;
        contact.conn_idx_client = conn_idx_client;

        // feedback format 4 to send the MCS in the downlink
        contact.feedback_plan = mac::feedback_plan_t(std::vector<uint32_t>{4});

        dectnrp_assert(contact.identity.NetworkID == identity_pt.NetworkID, "id not equal");
        dectnrp_assert(contact.identity.ShortNetworkID == identity_pt.ShortNetworkID,
                       "id not equal");
        dectnrp_assert(contact.identity.LongRadioDeviceID == identity_pt.LongRadioDeviceID,
                       "id not equal");
        dectnrp_assert(contact.identity.ShortRadioDeviceID == identity_pt.ShortRadioDeviceID,
                       "id not equal");
    }

    init_packet_beacon();
    init_packet_unicast(rd.identity_ft.ShortRadioDeviceID,
                        sp4::mac_architecture::identity_t::ShortRadioDeviceID_reserved,
                        rd.identity_ft.LongRadioDeviceID,
                        sp4::mac_architecture::identity_t::LongRadioDeviceID_reserved);

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

phy::machigh_phy_t steady_ft_t::work_regular(
    [[maybe_unused]] const phy::regular_report_t& regular_report) {
    return phy::machigh_phy_t();
}

phy::machigh_phy_t steady_ft_t::work_irregular(
    [[maybe_unused]] const phy::irregular_report_t& irregular_report) {
    phy::machigh_phy_t ret;

    if (rd.is_shutting_down()) {
        ret.irregular_report = state_transitions_cb();
        return ret;
    }

    dectnrp_assert(irregular_report.call_asap_after_this_time_has_passed_64 <
                       rd.allocation_ft.get_beacon_time_scheduled(),
                   "too late");

    dectnrp_assert(0 < irregular_report.get_recognition_delay(), "time out-of-order");

    /*
    dectnrp_assert(irregular_report.get_recognition_delay() <
                       duration_lut.get_N_samples_from_duration(sp3::duration_ec_t::slot001, 1),
                   "too late");
     */

    // try defining beacon transmission
    if (!worksub_tx_beacon(ret)) {
        dectnrp_assert_failure("beacon not transmitted");
    }

    worksub_tx_unicast_consecutive(ret);

    ret.irregular_report = phy::irregular_report_t(
        rd.allocation_ft.get_beacon_time_scheduled_minus_prepare_duration());

    rd.callbacks.run(buffer_rx.get_rx_time_passed());

    return ret;
}

phy::machigh_phy_t steady_ft_t::work_application(
    [[maybe_unused]] const application::application_report_t& application_report) {
    phy::machigh_phy_t machigh_phy;

    worksub_tx_unicast_consecutive(machigh_phy);

    return machigh_phy;
}

phy::machigh_phy_tx_t steady_ft_t::work_chscan_async([[maybe_unused]] const phy::chscan_t& chscan) {
    return phy::machigh_phy_tx_t();
}

phy::irregular_report_t steady_ft_t::entry() {
    // what is the next full second after start_time_64?
    const int64_t A =
        duration_lut.get_N_samples_at_next_full_second(buffer_rx.get_rx_time_passed());

#ifdef TFW_P2P_FT_ALIGN_BEACON_START_TO_FULL_SECOND_OR_CORRECT_OFFSET
    // time first beacon is transmitted
    const int64_t B = A + duration_lut.get_N_samples_from_duration(sp3::duration_ec_t::s001);
#else
    // time first beacon is transmitted
    const int64_t B = A + duration_lut.get_N_samples_from_duration(sp3::duration_ec_t::s001) +
                      hw.get_pps_to_full_second_measured_samples();
#endif

    // set first beacon transmission time, beacon is aligned with full second
    rd.allocation_ft.set_beacon_time_scheduled(B);

#ifdef TFW_P2P_EXPORT_PPX
    dectnrp_assert(B - duration_lut.get_N_samples_from_duration(sp3::duration_ec_t::s001) > 0,
                   "time out-of-order");

    // set virtual time of first rising edge, next edge is then aligned with the first beacon
    rd.ppx.set_ppx_rising_edge(B -
                               duration_lut.get_N_samples_from_duration(sp3::duration_ec_t::s001));
#endif

    // initialize regular callback for logs
    rd.callbacks.add_callback(
        std::bind(&steady_ft_t::worksub_callback_log, this, std::placeholders::_1),
        A + duration_lut.get_N_samples_from_duration(sp3::duration_ec_t::ms001, 500),
        duration_lut.get_N_samples_from_duration(sp3::duration_ec_t::s001,
                                                 rd.worksub_callback_log_period_sec));

#ifdef TFW_P2P_EXPORT_PPX
    rd.callbacks.add_callback(std::bind(&steady_ft_t::worksub_callback_ppx,
                                        this,
                                        std::placeholders::_1,
                                        std::placeholders::_2,
                                        std::placeholders::_3),
                              B - rd.ppx.get_ppx_time_advance_samples(),
                              rd.ppx.get_ppx_period_warped());
#endif

    return phy::irregular_report_t(
        rd.allocation_ft.get_beacon_time_scheduled_minus_prepare_duration());
}

void steady_ft_t::init_packet_beacon() {
    // meta packet size
    sp3::packet_sizes_def_t& psdef = rd.ppmp_beacon.psdef;
    psdef.u = worker_pool_config.radio_device_class.u_min;
    psdef.b = worker_pool_config.radio_device_class.b_min;
    psdef.PacketLengthType = 1;
    psdef.PacketLength = 2;
#ifdef TFW_P2P_MIMO
    psdef.tm_mode_index = sp3::tmmode::get_tx_div_mode(buffer_rx.nof_antennas);
#else
    psdef.tm_mode_index = 0;
#endif
    psdef.mcs_index = 2;
    psdef.Z = worker_pool_config.radio_device_class.Z_min;

    // define PLCFs
    sp4::plcf_10_t& plcf_10 = rd.ppmp_beacon.plcf_10;
    plcf_10.HeaderFormat = 0;
    plcf_10.PacketLengthType = psdef.PacketLengthType;
    plcf_10.set_PacketLength_m1(psdef.PacketLength);
    plcf_10.ShortNetworkID = rd.identity_ft.ShortNetworkID;
    plcf_10.TransmitterIdentity = rd.identity_ft.ShortRadioDeviceID;
    plcf_10.set_TransmitPower(ft.TransmitPower_dBm_fixed);
    plcf_10.Reserved = 0;
    plcf_10.DFMCS = psdef.mcs_index;

    // pick one PLCF
    rd.ppmp_beacon.plcf_base_effective = &rd.ppmp_beacon.plcf_10;

    // define MAC header type
    rd.ppmp_beacon.mac_header_type.Version = sp4::mac_header_type_t::version_ec::v00;
    rd.ppmp_beacon.mac_header_type.MAC_security =
        sp4::mac_header_type_t::mac_security_ec::macsec_not_used;
    rd.ppmp_beacon.mac_header_type.MAC_header_type =
        sp4::mac_header_type_t::mac_header_type_ec::Beacon;

    // define MAC common header
    rd.ppmp_beacon.beacon_header.set_Network_ID_3_lsb(rd.identity_ft.NetworkID);
    rd.ppmp_beacon.beacon_header.Transmitter_Address = rd.identity_ft.LongRadioDeviceID;

    // pick one MAC common header
    rd.ppmp_beacon.mch_base_effective = &rd.ppmp_beacon.beacon_header;

    // set values in cluster beacon IE
    auto& cbm = rd.mmie_pool_tx.get<sp4::cluster_beacon_message_t>();
    cbm.system_frame_number = 0;
    cbm.clusters_max_tx_power = sp4::network_beacon_message_t::clusters_max_tx_power_t::_19_dBm;
    cbm.has_power_constraints = true;
    cbm.frame_offset.reset();
    cbm.next_cluster_channel.reset();
    cbm.time_to_next.reset();
    cbm.network_beacon_period = sp4::network_beacon_message_t::network_beacon_period_t::_100_ms;
    cbm.cluster_beacon_period = sp4::network_beacon_message_t::cluster_beacon_period_t::_10_ms;
    cbm.count_to_trigger = sp4::cluster_beacon_message_t::count_to_trigger_t::_1_times;
    cbm.rel_quality = sp4::cluster_beacon_message_t::quality_threshold_t::_9_dB;
    cbm.min_quality = sp4::cluster_beacon_message_t::quality_threshold_t::_9_dB;
}

std::optional<phy::maclow_phy_t> steady_ft_t::worksub_pcc_10(
    [[maybe_unused]] const phy::phy_maclow_t& phy_maclow) {
    return std::nullopt;
}

phy::maclow_phy_t steady_ft_t::worksub_pcc_20(
    [[maybe_unused]] const phy::phy_maclow_t& phy_maclow) {
    return phy::maclow_phy_t();
}

phy::maclow_phy_t steady_ft_t::worksub_pcc_21(const phy::phy_maclow_t& phy_maclow) {
    // cast guaranteed to work
    const auto* plcf_21 =
        static_cast<const sp4::plcf_21_t*>(phy_maclow.pcc_report.plcf_decoder.get_plcf_base(2));

    dectnrp_assert(plcf_21 != nullptr, "cast ill-formed");

    // is this a packet from the correct network, from a known PT, and for this FT?
    if (plcf_21->ShortNetworkID != rd.identity_ft.ShortNetworkID ||
        !ft.contact_list.is_srdid_known(plcf_21->TransmitterIdentity) ||
        plcf_21->ReceiverIdentity != rd.identity_ft.ShortRadioDeviceID) {
        return phy::maclow_phy_t();
    }

    // load long radio device ID of sending PT
    const auto lrdid = ft.contact_list.get_lrdid_from_srdid(plcf_21->TransmitterIdentity);

    // load contact information of PT
    auto& contact = ft.contact_list.get_contact(lrdid);

    contact.sync_report = phy_maclow.sync_report;

    contact.mimo_csi.update_from_feedback(
        plcf_21->FeedbackFormat, plcf_21->feedback_info_pool, phy_maclow.sync_report);

    return worksub_pcc2pdc(phy_maclow,
                           2,
                           rd.identity_ft.NetworkID,
                           0,
                           phy::harq::finalize_rx_t::reset_and_terminate,
                           phy::maclow_phy_handle_t(phy::handle_pcc2pdc_t::th21, lrdid));
}

phy::machigh_phy_t steady_ft_t::worksub_pdc_10(
    [[maybe_unused]] const phy::phy_machigh_t& phy_machigh) {
    return phy::machigh_phy_t();
}

phy::machigh_phy_t steady_ft_t::worksub_pdc_20(
    [[maybe_unused]] const phy::phy_machigh_t& phy_machigh) {
    return phy::machigh_phy_t();
}

phy::machigh_phy_t steady_ft_t::worksub_pdc_21(const phy::phy_machigh_t& phy_machigh) {
    dectnrp_assert(phy_machigh.pdc_report.crc_status,
                   "worksub should only be called with valid CRC");

    // long radio device ID used as key
    const auto lrdid = phy_machigh.maclow_phy.get_handle_lrdid();

    auto& contact = ft.contact_list.get_contact(lrdid);

    // request vector of base pointers to MMIEs
    const auto& mmie_decoded_vec = phy_machigh.pdc_report.mac_pdu_decoder.get_mmie_decoded_vec();

    // count datagrams to be forwarded
    uint32_t datagram_cnt = 0;

    for (auto mmie : mmie_decoded_vec) {
        if (dynamic_cast<sp4::user_plane_data_t*>(mmie) == nullptr) {
            dectnrp_log_wrn("MMIE not user plane data");
            continue;
        }

        const sp4::user_plane_data_t* upd = static_cast<sp4::user_plane_data_t*>(mmie);

        if (rd.application_client->write_nto(
                contact.conn_idx_client, upd->get_data_ptr(), upd->get_data_size()) > 0) {
            ++datagram_cnt;
        }
    }

    rd.application_client->trigger_forward_nto(datagram_cnt);

    contact.mimo_csi.update_from_phy(
        rd.cqi_lut.get_highest_mcs_possible(phy_machigh.pdc_report.snr_dB),
        phy_machigh.phy_maclow.sync_report);

    return phy::machigh_phy_t();
}

bool steady_ft_t::worksub_tx_beacon(phy::machigh_phy_t& machigh_phy) {
    // OPTIONAL: change MIMO mode and dimensions of PLCF and MAC PDU (psdef)
    // -

    // request harq process
    auto* hp_tx = hpp.get_process_tx(rd.ppmp_beacon.plcf_base_effective->get_Type(),
                                     rd.identity_ft.NetworkID,
                                     rd.ppmp_beacon.psdef,
                                     phy::harq::finalize_tx_t::reset_and_terminate);

    // every firmware has to decide how to deal with unavailable HARQ process
    if (hp_tx == nullptr) {
        dectnrp_log_wrn("HARQ process TX unavailable.");
        return false;
    }

    // this is now a well-defined packet size
    const sp3::packet_sizes_t& packet_sizes = hp_tx->get_packet_sizes();

    // OPTIONAL: change content of PLCF, MAC header type and MAC common header
    // -

    uint32_t a_cnt_w = rd.ppmp_beacon.pack_first_3_header(hp_tx->get_a_plcf(), hp_tx->get_a_tb());

    // change content of cluster_beacon_message_t
    auto& cbm = rd.mmie_pool_tx.get<sp4::cluster_beacon_message_t>();
    cbm.pack_mmh_sdu(hp_tx->get_a_tb() + a_cnt_w);
    a_cnt_w += cbm.get_packed_size_of_mmh_sdu();

    // one time_announce_ie_t per second
    if (stats.beacon_cnt % rd.allocation_ft.get_N_beacons_per_second() == 0) {
        // set values in time_announce_ie_t
        auto& taie = rd.mmie_pool_tx.get<sp4::extensions::time_announce_ie_t>();
        taie.set_time(sp4::extensions::time_announce_ie_t::time_type_t::LOCAL, 0, 0);
        taie.pack_mmh_sdu(hp_tx->get_a_tb() + a_cnt_w);
        a_cnt_w += taie.get_packed_size_of_mmh_sdu();
    }

    // fill up with padding IEs
    rd.mmie_pool_tx.fill_with_padding_ies(hp_tx->get_a_tb() + a_cnt_w,
                                          packet_sizes.N_TB_byte - a_cnt_w);

    uint32_t codebook_index = 0;

    const phy::tx_meta_t& tx_meta = {.optimal_scaling_DAC = false,
                                     .DAC_scale = agc_tx.get_ofdm_amplitude_factor(),
                                     .iq_phase_rad = 0.0f,
                                     .iq_phase_increment_s2s_post_resampling_rad = 0.0f,
                                     .GI_percentage = 5};

    radio::buffer_tx_meta_t buffer_tx_meta = {
        .tx_order_id = tx_order_id, .tx_time_64 = rd.allocation_ft.get_beacon_time_scheduled()};

    ++tx_order_id;
    tx_earliest_64 = rd.allocation_ft.get_beacon_time_scheduled();

    machigh_phy.tx_descriptor_vec.push_back(
        phy::tx_descriptor_t(*hp_tx, codebook_index, tx_meta, buffer_tx_meta));

    // set tranmission time of next beacon
    rd.allocation_ft.set_beacon_time_next();

    ++stats.beacon_cnt;

    return true;
}

void steady_ft_t::worksub_tx_unicast_consecutive(phy::machigh_phy_t& machigh_phy) {
    // number of definable packets is limited
    for (uint32_t i = 0; i < rd.max_simultaneous_tx_unicast; ++i) {
        // go over the connection indexes which represent different devices
        for (auto& contact : ft.contact_list.get_contacts_vec()) {
            contact.allocation_pt.set_beacon_time_last_known(
                rd.allocation_ft.get_beacon_time_transmitted());

            const auto tx_opportunity =
                contact.allocation_pt.get_tx_opportunity(mac::allocation::direction_t::downlink,
                                                         buffer_rx.get_rx_time_passed(),
                                                         tx_earliest_64);

            // if no opportunity found, leave machigh_phy as is
            if (tx_opportunity.tx_time_64 < 0) {
                return;
            }

            // change content of headers
            rd.ppmp_unicast.plcf_21.ReceiverIdentity = contact.identity.ShortRadioDeviceID;
            rd.ppmp_unicast.unicast_header.Receiver_Address = contact.identity.LongRadioDeviceID;

            // change feedback info in PLCF
            rd.ppmp_unicast.plcf_21.FeedbackFormat = sp4::feedback_info_t::No_feedback;

            // try to send a packet, may return false if no data of HARQ processes are available
            if (!worksub_tx_unicast(machigh_phy, contact, tx_opportunity)) {
                // break;
            }
        }
    }
}

void steady_ft_t::worksub_callback_log(const int64_t now_64) const {
    std::string str = "id=" + std::to_string(id) + " ";

    str += stats.get_as_string();

    str += "tx_power_ant_0dBFS=" + std::to_string(agc_tx.get_power_ant_0dBFS(now_64)) + " ";
    str += "rx_power_ant_0dBFS=" + agc_rx.get_power_ant_0dBFS(now_64).get_readable_list() + " ";

    for (const auto& contact : ft.contact_list.get_contacts_vec()) {
        str += "rx_rms=[";
        str += std::to_string(contact.identity.LongRadioDeviceID) + "]" +
               contact.sync_report.rms_array.get_readable_list() + " ";
    }

    dectnrp_log_inf("{}", str);
}

}  // namespace dectnrp::upper::tfw::p2p
