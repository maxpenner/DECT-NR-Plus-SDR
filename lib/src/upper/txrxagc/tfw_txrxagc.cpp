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

#include "dectnrp/upper/txrxagc/tfw_txrxagc.hpp"

#include <algorithm>
#include <cstdint>
#include <cstring>

#include "dectnrp/common/prog/log.hpp"
#include "dectnrp/phy/agc/agc_tx.hpp"
#include "dectnrp/phy/interfaces/machigh_phy.hpp"
#include "dectnrp/phy/rx/sync/irregular_report.hpp"
#include "dectnrp/radio/hw_simulator.hpp"
#include "dectnrp/sections_part3/derivative/duration_ec.hpp"

namespace dectnrp::upper::tfw::txrxagc {

const std::string tfw_txrxagc_t::firmware_name("txrxagc");

tfw_txrxagc_t::tfw_txrxagc_t(const tpoint_config_t& tpoint_config_, phy::mac_lower_t& mac_lower_)
    : tpoint_t(tpoint_config_, mac_lower_) {
    hw.set_command_time();
    hw.set_tx_power_ant_0dBFS_uniform_tc(10.0f);
    hw.set_rx_power_ant_0dBFS_uniform_tc(-50.0f);
    hw.set_freq_tc(3830.0e6);
    hw.enable_runtime_logging.store(false, std::memory_order_release);

    // loopback requires the hardware to be a simulator
    radio::hw_simulator_t* hw_simulator = dynamic_cast<radio::hw_simulator_t*>(&hw);

    if (hw_simulator != nullptr) {
        hw.set_tx_power_ant_0dBFS_uniform_tc(0.0f);
        hw.set_rx_power_ant_0dBFS_uniform_tc(0.0f);

        // called from tpoint firmware, thread-safe
        hw_simulator->set_trajectory(simulation::topology::trajectory_t(
            simulation::topology::position_t::from_cartesian(0.0f, 0.0f, 0.0f)));
        hw_simulator->set_net_bandwidth_norm(1.0f / static_cast<float>(worker_pool_config.os_min));
        hw_simulator->set_tx_into_rx_leakage_dB(0.0f);
        hw_simulator->set_rx_noise_figure_dB(0.0f);
        hw_simulator->set_rx_snr_in_net_bandwidth_norm_dB(50.0f);
    }

    psdef = {.u = worker_pool_config.radio_device_class.u_min,
             .b = worker_pool_config.radio_device_class.b_min,
             .PacketLengthType = 0,
             .PacketLength = 2,
             .tm_mode_index = 0,
             .mcs_index = 2,
             .Z = worker_pool_config.radio_device_class.Z_min};

    identity_front = sp4::mac_architecture::identity_t(100, 444, 555);

    plcf_10_front.HeaderFormat = 0;
    plcf_10_front.PacketLengthType = psdef.PacketLengthType;
    plcf_10_front.set_PacketLength_m1(psdef.PacketLength);
    plcf_10_front.ShortNetworkID = identity_front.ShortNetworkID;
    plcf_10_front.TransmitterIdentity = identity_front.ShortRadioDeviceID;
    plcf_10_front.set_TransmitPower(0);
    plcf_10_front.Reserved = 0;
    plcf_10_front.DFMCS = psdef.mcs_index;

    identity_back = sp4::mac_architecture::identity_t(identity_front.NetworkID + 1,
                                                      identity_front.LongRadioDeviceID + 1,
                                                      identity_front.ShortRadioDeviceID + 1);

    plcf_10_back.HeaderFormat = 0;
    plcf_10_back.PacketLengthType = psdef.PacketLengthType;
    plcf_10_back.set_PacketLength_m1(psdef.PacketLength);
    plcf_10_back.ShortNetworkID = identity_back.ShortNetworkID;
    plcf_10_back.TransmitterIdentity = identity_back.ShortRadioDeviceID;
    plcf_10_back.set_TransmitPower(0);
    plcf_10_back.Reserved = 0;
    plcf_10_back.DFMCS = psdef.mcs_index;
}

phy::irregular_report_t tfw_txrxagc_t::work_start(const int64_t start_time_64) {
    return phy::irregular_report_t(start_time_64 +
                                   duration_lut.get_N_samples_from_duration(
                                       sp3::duration_ec_t::ms001, measurement_spacing_ms));
}

phy::machigh_phy_t tfw_txrxagc_t::work_regular(
    [[maybe_unused]] const phy::regular_report_t& regular_report) {
    return phy::machigh_phy_t();
}

phy::machigh_phy_t tfw_txrxagc_t::work_irregular(const phy::irregular_report_t& irregular_report) {
    dectnrp_assert(
        irregular_report.call_asap_after_this_time_has_passed_64 < buffer_rx.get_rx_time_passed(),
        "time out-of-order");

    if (tpoint_config.firmware_id == 1) {
        return phy::machigh_phy_t();
    }

    phy::machigh_phy_t ret;

    tx_time_front_64 =
        buffer_rx.get_rx_time_passed() + hw.get_tmin_samples(radio::hw_t::tmin_t::turnaround);

    // toggle the AGC change to make RMS level distinguishable
    const float agc_change_dB_now = measurement_cnt_64 % 2 == 0 ? -agc_change_dB : agc_change_dB;

    // AGC settings attached to front packet, change antennas with every call
    common::ant_t adj_dB(hw.get_nof_antennas());
    for (std::size_t i = 0; i < std::min(adj_dB.get_nof_antennas(), nof_antennas_simultaneous);
         ++i) {
        adj_dB.at(i) = agc_change_dB_now;
    }

    // length of front packet
    int64_t tx_length_front_64{common::adt::UNDEFINED_EARLY_64};
    switch (agc_dut) {
        using enum agc_dut_t;
        case none:
            tx_length_front_64 = generate_packet_asap(
                ret, plcf_10_front, tx_time_front_64, std::nullopt, std::nullopt);
            break;
        case tx:
            tx_length_front_64 =
                generate_packet_asap(ret, plcf_10_front, tx_time_front_64, adj_dB, std::nullopt);
            break;
        case rx:
            tx_length_front_64 =
                generate_packet_asap(ret, plcf_10_front, tx_time_front_64, std::nullopt, adj_dB);
            break;
        case both:
            tx_length_front_64 =
                generate_packet_asap(ret, plcf_10_front, tx_time_front_64, adj_dB, adj_dB);
            break;
        case alternating:
            if (measurement_cnt_64 % 4 <= 1) {
                tx_length_front_64 = generate_packet_asap(
                    ret, plcf_10_front, tx_time_front_64, adj_dB, std::nullopt);
            } else {
                tx_length_front_64 = generate_packet_asap(
                    ret, plcf_10_front, tx_time_front_64, std::nullopt, adj_dB);
            }

            break;
    }

    // transmission time of back packet
    const int64_t tx_time_back_64 =
        tx_time_front_64 + tx_length_front_64 +
        duration_lut.get_N_samples_from_duration(sp3::duration_ec_t::subslot_u8_001,
                                                 front_back_spacing_u8_subslots);

    generate_packet_asap(ret, plcf_10_back, tx_time_back_64, std::nullopt, std::nullopt);

    // unused if logging is disabled
    [[maybe_unused]] const int64_t TX_f_TX_b = tx_time_back_64 - tx_time_front_64;
    [[maybe_unused]] const int64_t TX_f_TX_b_gap = TX_f_TX_b - tx_length_front_64;

    dectnrp_log_inf("");
    dectnrp_log_inf(
        "tx_time_front_64={} tx_length_front_64={}samples={}us TX_f_TX_b={}samples={}us TX_f_TX_b_gap={}samples={}us",
        tx_time_front_64,
        tx_length_front_64,
        duration_lut.get_N_us_from_samples(tx_length_front_64),
        TX_f_TX_b,
        duration_lut.get_N_us_from_samples(TX_f_TX_b),
        TX_f_TX_b_gap,
        duration_lut.get_N_us_from_samples(TX_f_TX_b_gap));

    ret.irregular_report =
        irregular_report.get_same_with_time_increment(duration_lut.get_N_samples_from_duration(
            sp3::duration_ec_t::ms001, measurement_spacing_ms));

    ++measurement_cnt_64;

    return ret;
}

phy::maclow_phy_t tfw_txrxagc_t::work_pcc(const phy::phy_maclow_t& phy_maclow) {
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

    // front?
    if (plcf_10_rx->ShortNetworkID == identity_front.ShortNetworkID &&
        plcf_10_rx->TransmitterIdentity == identity_front.ShortRadioDeviceID) {
        // time relative to old front packet
        const int64_t RX_f_RX_f =
            phy_maclow.sync_report.fine_peak_time_corrected_by_sto_fractional_64 - rx_time_front_64;

        // save front packet time for comparison with back packet
        rx_time_front_64 = phy_maclow.sync_report.fine_peak_time_corrected_by_sto_fractional_64;

        dectnrp_log_inf("rx_time_front_64={} rms_array={} = RX_f_RX_f={}samples={}us",
                        rx_time_front_64,
                        phy_maclow.sync_report.rms_array.get_readable_list(),
                        RX_f_RX_f,
                        duration_lut.get_N_us_from_samples(RX_f_RX_f));
    }

    // back?
    if (plcf_10_rx->ShortNetworkID == identity_back.ShortNetworkID &&
        plcf_10_rx->TransmitterIdentity == identity_back.ShortRadioDeviceID) {
        // measure times between both TX and both RX
        const int64_t TX_f_RX_f = rx_time_front_64 - tx_time_front_64;
        const int64_t RX_f_RX_b =
            phy_maclow.sync_report.fine_peak_time_corrected_by_sto_fractional_64 - rx_time_front_64;

        dectnrp_log_inf(
            "rx_time_back_64={} rms_array={} | TX_f_RX_f={}samples={}us | RX_f_RX_b={}samples={}us",
            phy_maclow.sync_report.fine_peak_time_corrected_by_sto_fractional_64,
            phy_maclow.sync_report.rms_array.get_readable_list(),
            TX_f_RX_f,
            duration_lut.get_N_us_from_samples(TX_f_RX_f),
            RX_f_RX_b,
            duration_lut.get_N_us_from_samples(RX_f_RX_b));
    }

    return phy::maclow_phy_t();
}

phy::machigh_phy_t tfw_txrxagc_t::work_pdc([[maybe_unused]] const phy::phy_machigh_t& phy_machigh) {
    return phy::machigh_phy_t();
}

phy::machigh_phy_t tfw_txrxagc_t::work_pdc_error(
    [[maybe_unused]] const phy::phy_machigh_t& phy_machigh) {
    return phy::machigh_phy_t();
}

phy::machigh_phy_t tfw_txrxagc_t::work_application(
    [[maybe_unused]] const application::application_report_t& application_report) {
    return phy::machigh_phy_t();
}

phy::machigh_phy_tx_t tfw_txrxagc_t::work_channel([[maybe_unused]] const phy::chscan_t& chscan) {
    return phy::machigh_phy_tx_t();
}

void tfw_txrxagc_t::work_stop() {}

int64_t tfw_txrxagc_t::generate_packet_asap(phy::machigh_phy_t& machigh_phy,
                                            const sp4::plcf_10_t& plcf_10,
                                            const int64_t tx_time_64,
                                            const std::optional<common::ant_t>& tx_power_adj_dB,
                                            const std::optional<common::ant_t>& rx_power_adj_dB) {
    // request harq process
    auto* hp_tx = hpp.get_process_tx(
        1, identity_front.NetworkID, psdef, phy::harq::finalize_tx_t::reset_and_terminate);

    // every firmware has to decide how to deal with unavailable HARQ process
    if (hp_tx == nullptr) {
        dectnrp_log_wrn("HARQ process TX unavailable");
        return 0;
    }

    // this is now a well-defined packet size
    const sp3::packet_sizes_t& packet_sizes = hp_tx->get_packet_sizes();

    plcf_10.pack(hp_tx->get_a_plcf());

    memset(hp_tx->get_a_tb(), 0x00, packet_sizes.N_TB_byte);

    uint32_t codebook_index = 0;

    const phy::tx_meta_t tx_meta = {.optimal_scaling_DAC = false,
                                    .DAC_scale = agc_tx.get_ofdm_amplitude_factor(),
                                    .iq_phase_rad = 0.0f,
                                    .iq_phase_increment_s2s_post_resampling_rad = 0.0f,
                                    .GI_percentage = 5};

    radio::buffer_tx_meta_t buffer_tx_meta = {.tx_order_id = tx_order_id,
                                              .tx_time_64 = tx_time_64,
                                              .tx_power_adj_dB = tx_power_adj_dB,
                                              .rx_power_adj_dB = rx_power_adj_dB};

    ++tx_order_id;
    tx_earliest_64 = buffer_tx_meta.tx_time_64 + sp3::get_N_samples_in_packet_length(
                                                     hp_tx->get_packet_sizes(), hw.get_samp_rate());

    machigh_phy.tx_descriptor_vec.push_back(
        phy::tx_descriptor_t(*hp_tx, codebook_index, tx_meta, buffer_tx_meta));

    return static_cast<int64_t>(
        sp3::get_N_samples_in_packet_length(hp_tx->get_packet_sizes(), hw.get_samp_rate()));
}

}  // namespace dectnrp::upper::tfw::txrxagc
