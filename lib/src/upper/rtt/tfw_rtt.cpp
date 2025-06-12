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

#include "dectnrp/upper/rtt/tfw_rtt.hpp"

#include <algorithm>

#include "dectnrp/application/socket/socket_client.hpp"
#include "dectnrp/application/socket/socket_server.hpp"
#include "dectnrp/common/prog/log.hpp"
#include "dectnrp/phy/agc/agc_tx.hpp"
#include "dectnrp/upper/rtt/tfw_rtt_param.hpp"

namespace dectnrp::upper::tfw::rtt {

const std::string tfw_rtt_t::firmware_name("rtt");

tfw_rtt_t::tfw_rtt_t(const tpoint_config_t& tpoint_config_, phy::mac_lower_t& mac_lower_)
    : tpoint_t(tpoint_config_, mac_lower_) {
    // init HARQ process pool
    hpp =
        std::make_unique<phy::harq::process_pool_t>(worker_pool_config.maximum_packet_sizes, 4, 4);

    // set TX power, RX power, and frequency which should be free of any interference
    hw.set_command_time();
    hw.set_tx_power_ant_0dBFS_tc(10.0f);
    hw.set_rx_power_ant_0dBFS_uniform_tc(-30.0f);
    hw.set_freq_tc(3830.0e6);

    psdef = {.u = worker_pool_config.radio_device_class.u_min,
             .b = worker_pool_config.radio_device_class.b_min,
             .PacketLengthType = 0,
             .PacketLength = 2,
             .tm_mode_index = 0,
             .mcs_index = 2,
             .Z = worker_pool_config.radio_device_class.Z_min};

    identity_ft = sp4::mac_architecture::identity_t(100, 444, 555);
    identity_pt = identity_ft;
    ++identity_pt.LongRadioDeviceID;
    ++identity_pt.ShortRadioDeviceID;

    plcf_10.HeaderFormat = 0;
    plcf_10.PacketLengthType = psdef.PacketLengthType;
    plcf_10.set_PacketLength_m1(psdef.PacketLength);
    if (tpoint_config.firmware_id == 0) {
        plcf_10.ShortNetworkID = identity_ft.ShortNetworkID;
        plcf_10.TransmitterIdentity = identity_ft.ShortRadioDeviceID;
    } else {
        plcf_10.ShortNetworkID = identity_pt.ShortNetworkID;
        plcf_10.TransmitterIdentity = identity_pt.ShortRadioDeviceID;
    }
    plcf_10.set_TransmitPower(0);
    plcf_10.Reserved = 0;
    plcf_10.DFMCS = psdef.mcs_index;

    const application::queue_size_t queue_size = {
        .N_datagram = 4, .N_datagram_max_byte = limits::application_max_queue_datagram_byte};

    application_server = std::make_unique<application::sockets::socket_server_t>(
        id,
        tpoint_config.application_server_thread_config,
        job_queue,
        std::vector<uint32_t>{TFW_RTT_UDP_PORT_DATA, TFW_RTT_UDP_PORT_PRINT},
        queue_size);

    application_client = std::make_unique<application::sockets::socket_client_t>(
        id,
        tpoint_config.application_client_thread_config,
        job_queue,
        std::vector<uint32_t>{8050},
        queue_size);

    // client thread is not started as all data is forwarded directly from the calling worker thread
    // application_client->start_sc();

    // then start source
    application_server->start_sc();

    stage_a.resize(TFW_RTT_TX_LENGTH_MAXIMUM_BYTE);
}

phy::irregular_report_t tfw_rtt_t::work_start([[maybe_unused]] const int64_t start_time_64) {
    return phy::irregular_report_t();
}

phy::machigh_phy_t tfw_rtt_t::work_regular(
    [[maybe_unused]] const phy::regular_report_t& regular_report) {
    return phy::machigh_phy_t();
}

phy::machigh_phy_t tfw_rtt_t::work_irregular(
    [[maybe_unused]] const phy::irregular_report_t& irregular_report) {
    return phy::machigh_phy_t();
}

phy::maclow_phy_t tfw_rtt_t::work_pcc(const phy::phy_maclow_t& phy_maclow) {
    // base pointer to extract PLCF_type=1
    const auto* plcf_base = phy_maclow.pcc_report.plcf_decoder.get_plcf_base(1);

    // is this the correct PLCF type?
    if (plcf_base == nullptr) {
        return phy::maclow_phy_t();
    }

    // is this the correct header type?
    if (plcf_base->get_HeaderFormat() != 0) {
        return phy::maclow_phy_t();
    }

    // cast guaranteed to work
    const auto* plcf_10_rx =
        static_cast<const sp4::plcf_10_t*>(phy_maclow.pcc_report.plcf_decoder.get_plcf_base(1));

    dectnrp_assert(plcf_10_rx != nullptr, "cast ill-formed");

    // is this the correct short network ID?
    if (plcf_10_rx->ShortNetworkID != identity_ft.ShortNetworkID) {
        return phy::maclow_phy_t();
    }

    // expected TransmitterIdentity depends on FT or PT
    if (tpoint_config.firmware_id == 0) {
        // FT receives from PT
        if (plcf_10_rx->TransmitterIdentity != identity_pt.ShortRadioDeviceID) {
            return phy::maclow_phy_t();
        }
    } else {
        // PT receives from FT
        if (plcf_10_rx->TransmitterIdentity != identity_ft.ShortRadioDeviceID) {
            return phy::maclow_phy_t();
        }
    }

    return worksub_pcc2pdc(phy_maclow,
                           1,
                           identity_pt.NetworkID,
                           0,
                           phy::harq::finalize_rx_t::reset_and_terminate,
                           phy::maclow_phy_handle_t());
}

phy::machigh_phy_t tfw_rtt_t::work_pdc_async(const phy::phy_machigh_t& phy_machigh) {
    // ignore entire PDC if CRC is incorrect
    if (!phy_machigh.pdc_report.crc_status) {
        return phy::machigh_phy_t();
    }

    phy::machigh_phy_t machigh_phy;

    if (tpoint_config.firmware_id == 0) {
        // measure round-trip time
        const int64_t rtt = watch.get_elapsed();

        // add to statistics
        rtt_min = std::min(rtt_min, rtt);
        rtt_max = std::max(rtt_max, rtt);
        rms_max = std::max(rms_max, phy_machigh.phy_maclow.sync_report.rms_array.get_max());

        // get pointer to payload of received packet
        const auto a_raw = phy_machigh.pdc_report.mac_pdu_decoder.get_a_raw();

        // copy first few bytes so rtt can verify the content of the packet
        memcpy(&stage_a[0], a_raw.first, TFW_RTT_TX_VS_RX_VERIFICATION_LENGTH_BYTE);

        // insert measured RTT into payload so rtt_external.cpp can log it
        memcpy(&stage_a[TFW_RTT_TX_VS_RX_VERIFICATION_LENGTH_BYTE], &rtt, sizeof(rtt));

        // forward to rtt
        application_client->write_immediate(0, &stage_a[0], a_raw.second);

        ++N_measurement_rx_cnt;
    } else {
        // copy received payload onto stage
        phy_machigh.pdc_report.mac_pdu_decoder.copy_a(&stage_a[0]);

        generate_packet_asap(machigh_phy);

        // base pointer to extract PLCF_type=1
        const auto* plcf_base = phy_machigh.phy_maclow.pcc_report.plcf_decoder.get_plcf_base(1);

        // make AGC gain changes right after sending the response packet
        const int64_t t_agc_change_64 =
            machigh_phy.tx_descriptor_vec.at(0).buffer_tx_meta.tx_time_64 +
            duration_lut.get_N_samples_from_subslots(worker_pool_config.radio_device_class.u_min,
                                                     psdef.PacketLength);

        worksub_agc(phy_machigh.phy_maclow.sync_report, *plcf_base, t_agc_change_64);
    }

    return machigh_phy;
}

phy::machigh_phy_t tfw_rtt_t::work_application(
    const application::application_report_t& application_report) {
    // return immediately if PT
    if (0 < tpoint_config.firmware_id) {
        return phy::machigh_phy_t();
    }

    /* The program rtt uses payload at connection index 1 to indicate that a measurement is
     * completed and the result should be logged. All variables must be reset for a new measurement
     * run.
     */
    if (application_report.conn_idx == 1) {
        // convert rtt to us (unused if logging is turned off)
        [[maybe_unused]] const int64_t rtt_min_us = rtt_min / int64_t{1000};
        [[maybe_unused]] const int64_t rtt_max_us = rtt_max / int64_t{1000};

        // number of microseconds in our packet (unused if logging is turned off)
        [[maybe_unused]] const int64_t packet_length_us_64 =
            duration_lut.get_N_us_from_samples(duration_lut.get_N_samples_from_subslots(
                worker_pool_config.radio_device_class.u_min, psdef.PacketLength));

        dectnrp_log_inf("TX = {} RX = {} | PER = {} | packet_length = {} us",
                        N_measurement_tx_cnt,
                        N_measurement_rx_cnt,
                        1.0f - static_cast<float>(N_measurement_rx_cnt) /
                                   static_cast<float>(N_measurement_tx_cnt),
                        packet_length_us_64);
        dectnrp_log_inf("rtt_min = {} us rtt_max: = {} us", rtt_min_us, rtt_max_us);
        dectnrp_log_inf("rtt_min = {} us rtt_max: = {} us <-- RTT adjusted for packet length",
                        rtt_min_us - 2 * packet_length_us_64,
                        rtt_max_us - 2 * packet_length_us_64);
        dectnrp_log_inf("rms_max: = {}", rms_max);

        // reset measurement variables
        N_measurement_tx_cnt = 0;
        N_measurement_rx_cnt = 0;
        rtt_min = -common::adt::UNDEFINED_EARLY_64;
        rtt_max = common::adt::UNDEFINED_EARLY_64;
        rms_max = -1000.0f;

        // invalidate payload
        application_server->read_nto(1, nullptr);

        return phy::machigh_phy_t();
    }

    watch.reset();

    phy::machigh_phy_t machigh_phy;

    generate_packet_asap(machigh_phy);

    ++N_measurement_tx_cnt;

    return machigh_phy;
}

phy::machigh_phy_tx_t tfw_rtt_t::work_chscan_async([[maybe_unused]] const phy::chscan_t& chscan) {
    return phy::machigh_phy_tx_t();
}

void tfw_rtt_t::work_stop() {
    // first stop accepting new data from upper
    application_server->stop_sc();

    // data sink threads were not started
    // application_client->stop_sc();
}

void tfw_rtt_t::generate_packet_asap(phy::machigh_phy_t& machigh_phy) {
    // request harq process
    auto* hp_tx = hpp->get_process_tx(
        1, identity_ft.NetworkID, psdef, phy::harq::finalize_tx_t::reset_and_terminate);

    // every firmware has to decide how to deal with unavailable HARQ process
    if (hp_tx == nullptr) {
        dectnrp_log_wrn("HARQ process TX unavailable");
        return;
    }

    // this is now a well-defined packet size
    const sp3::packet_sizes_t& packet_sizes = hp_tx->get_packet_sizes();

    plcf_10.pack(hp_tx->get_a_plcf());

    // define MAC PDU
    if (tpoint_config.firmware_id == 0) {
        // datagram sizes for first connection
        const auto queue_level = application_server->get_queue_level_nto(0, 1);

        dectnrp_assert(queue_level.N_filled > 0, "queue empty");
        dectnrp_assert(queue_level.levels[0] <= packet_sizes.N_TB_byte,
                       "N_TB_byte is only {}",
                       packet_sizes.N_TB_byte);

        application_server->read_nto(0, hp_tx->get_a_tb());
    } else {
        memcpy(hp_tx->get_a_tb(), &stage_a[0], packet_sizes.N_TB_byte);
    }

    uint32_t codebook_index = 0;

    const phy::tx_meta_t tx_meta = {.optimal_scaling_DAC = false,
                                    .DAC_scale = agc_tx.get_ofdm_amplitude_factor(),
                                    .iq_phase_rad = 0.0f,
                                    .iq_phase_increment_s2s_post_resampling_rad = 0.0f,
                                    .GI_percentage = 5};

    radio::buffer_tx_meta_t buffer_tx_meta = {
        .tx_order_id = tx_order_id,
        .tx_time_64 = std::max(
            tx_earliest_64,
            buffer_rx.get_rx_time_passed() + hw.get_tmin_samples(radio::hw_t::tmin_t::turnaround))};

    ++tx_order_id;
    tx_earliest_64 = buffer_tx_meta.tx_time_64 + sp3::get_N_samples_in_packet_length(
                                                     hp_tx->get_packet_sizes(), hw.get_samp_rate());

    machigh_phy.tx_descriptor_vec.push_back(
        phy::tx_descriptor_t(*hp_tx, codebook_index, tx_meta, buffer_tx_meta));
}

}  // namespace dectnrp::upper::tfw::rtt
