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

#include "dectnrp/upper/loopback/tfw_loopback.hpp"

#include <cmath>
#include <cstdlib>
#include <ctime>
#include <limits>

#include "dectnrp/common/adt/freq_shift.hpp"
#include "dectnrp/common/adt/miscellaneous.hpp"
#include "dectnrp/common/prog/assert.hpp"
#include "dectnrp/common/prog/log.hpp"
#include "dectnrp/constants.hpp"
#include "dectnrp/phy/rx/sync/irregular_report.hpp"
#include "dectnrp/phy/rx/sync/sync_param.hpp"

namespace dectnrp::upper::tfw::loopback {

tfw_loopback_t::tfw_loopback_t(const tpoint_config_t& tpoint_config_, phy::mac_lower_t& mac_lower_)
    : tpoint_t(tpoint_config_, mac_lower_) {
    dectnrp_assert(
        1e3 <= RX_SYNC_PARAM_AUTOCORRELATOR_DETECTION_RMS_THRESHOLD_MAX_SP,
        "This is an intentional error. Loopback firmware requires large RMS limit. Set RX_SYNC_PARAM_AUTOCORRELATOR_DETECTION_RMS_THRESHOLD_MAX_SP to 1e3 and recompile.");

    dectnrp_assert(!hw.hw_config.simulator_clip_and_quantize,
                   "For loopback firmware, clipping and quantization must not be applied.");

    // set frequency, TX and RX power
    hw.set_command_time();
    hw.set_freq_tc(0.0);
    hw.set_tx_power_ant_0dBFS_tc(0.0f);
    hw.set_rx_power_ant_0dBFS_uniform_tc(0.0f);

    // loopback requires the hardware to be a simulator
    hw_simulator = dynamic_cast<radio::hw_simulator_t*>(&hw);

    dectnrp_assert(hw_simulator != nullptr, "hw not simulator");

    // called from tpoint firmware, thread-safe
    hw_simulator->set_trajectory(simulation::topology::trajectory_t(
        simulation::topology::position_t::from_cartesian(0.0f, 0.0f, 0.0f)));
    hw_simulator->set_net_bandwidth_norm(1.0f / static_cast<float>(worker_pool_config.os_min));
    hw_simulator->set_tx_into_rx_leakage_dB(0.0f);
    hw_simulator->set_rx_noise_figure_dB(0.0f);
    hw_simulator->set_rx_snr_in_net_bandwidth_norm_dB(0.0f);

    state = STATE_t::A_SET_CHANNEL_SNR;

    stt.x_to_A_64 = duration_lut.get_N_samples_from_duration(sp3::duration_ec_t::ms001, 20);
    stt.A_to_B_64 = duration_lut.get_N_samples_from_duration(sp3::duration_ec_t::ms001, 5);
    stt.B_to_C_64 = duration_lut.get_N_samples_from_duration(sp3::duration_ec_t::ms001, 5);
    stt.C_to_B_64 = duration_lut.get_N_samples_from_duration(sp3::duration_ec_t::ms001, 15);
    stt.C_to_D_64 = duration_lut.get_N_samples_from_duration(sp3::duration_ec_t::ms001, 15);

    state_time_reference_64 = common::adt::UNDEFINED_EARLY_64;

    parameter_cnt = 0;

    // init SNR vector to test
    for (float snr = -2.0; snr < 20.0f; snr += 1.0f) {
        snr_vec.push_back(snr);
    }

    snr_cnt = 0;

    nof_experiment_per_snr = 10;
    nof_experiment_per_snr_cnt = 0;

    randomgen.shuffle();

    // ##################################################
    // initialize generate_packet_params_t

    pp.psdef = {.u = worker_pool_config.radio_device_class.u_min,
                .b = worker_pool_config.radio_device_class.b_min,
                .PacketLengthType = 1,
                .PacketLength = 1,
                .tm_mode_index = 0,
                .mcs_index = 1,
                .Z = worker_pool_config.radio_device_class.Z_min};

    const auto packet_sizes_opt = sp3::get_packet_sizes(pp.psdef);

    dectnrp_assert(packet_sizes_opt.has_value(), "undefined packet sizes");

    pp.N_samples_in_packet_length =
        sp3::get_N_samples_in_packet_length(packet_sizes_opt.value(), buffer_rx.samp_rate);

    pp.PLCF_type = 1;
    pp.PLCF_type_header_format = 0;
    pp.identity = sp4::mac_architecture::identity_t(100, 10000000, 1000);
    pp.update_plcf_unpacked();

    pp.tx_time_multiple_64 = 1;
    pp.tx_time_64 = common::adt::UNDEFINED_EARLY_64;
    pp.amplitude_scale = 1.0f;
    pp.cfo_symmetric_range_subc_multiple = 1.75f;
}

phy::irregular_report_t tfw_loopback_t::work_start(const int64_t start_time_64) {
    // start some time in the near future
    state_time_reference_64 = start_time_64 + stt.x_to_A_64;

    A_reset_result_counter_for_next_snr();

    return phy::irregular_report_t(state_time_reference_64);
}

phy::machigh_phy_t tfw_loopback_t::work_regular(
    [[maybe_unused]] const phy::regular_report_t& regular_report) {
    return phy::machigh_phy_t();
}

phy::machigh_phy_t tfw_loopback_t::work_irregular(
    [[maybe_unused]] const phy::irregular_report_t& irregular_report) {
    // get current time
    const int64_t now_64 = buffer_rx.get_rx_time_passed();

    dectnrp_assert(state_time_reference_64 <= now_64, "time of irregular call has not passed");

    phy::machigh_phy_t machigh_phy;

    switch (state) {
        using enum STATE_t;
        case A_SET_CHANNEL_SNR:
            {
                hw_simulator->set_rx_snr_in_net_bandwidth_norm_dB(snr_vec.at(snr_cnt));

                A_reset_result_counter_for_next_snr();

                state = B_SET_CHANNEL_SMALL_SCALE_FADING;

                state_time_reference_64 = now_64 + stt.A_to_B_64;

                break;
            }

        case B_SET_CHANNEL_SMALL_SCALE_FADING:
            {
                hw_simulator->wchannel_randomize_small_scale();

                state = C_EXPERIMENT_GENERATE_PACKETS;

                state_time_reference_64 = now_64 + stt.B_to_C_64;

                break;
            }

        case C_EXPERIMENT_GENERATE_PACKETS:
            {
                C_generate_single_experiment_at_current_snr(now_64, machigh_phy);

                ++nof_experiment_per_snr_cnt;

                if (nof_experiment_per_snr_cnt < nof_experiment_per_snr) {
                    state = B_SET_CHANNEL_SMALL_SCALE_FADING;

                    // should be much longer than a single experiment
                    state_time_reference_64 = now_64 + stt.C_to_B_64;
                } else {
                    state = D_EXPERIMENT_SAVE_RESULTS;

                    // should be much longer than a single experiment
                    state_time_reference_64 = now_64 + stt.C_to_D_64;
                }

                break;
            }

        case D_EXPERIMENT_SAVE_RESULTS:
            {
                dectnrp_assert(nof_experiment_per_snr_cnt == nof_experiment_per_snr,
                               "incorrect number of experiments");

                D_save_result_of_current_snr();

                state = E_SET_PARAMETER;

                break;
            }

        case E_SET_PARAMETER:
            {
                ++snr_cnt;

                state = A_SET_CHANNEL_SNR;

                // abort condition for SNR
                if (snr_cnt == snr_vec.size()) {
                    snr_cnt = 0;

                    dectnrp_log_inf(" ");

                    // abort condition for outer parameter
                    if (E_set_next_parameter_or_go_to_dead_end()) {
                        state = DEAD_END;
                        dectnrp_log_inf("all measurements finished");
                    }
                }

                break;
            }

        case STATE_t::DEAD_END:
            {
                state_time_reference_64 = std::numeric_limits<int64_t>::max();
                break;
            }
    }

    // schedule next callback
    if (state != STATE_t::DEAD_END) {
        machigh_phy.irregular_report = phy::irregular_report_t(state_time_reference_64);
    }

    return machigh_phy;
}

phy::machigh_phy_t tfw_loopback_t::work_application(
    [[maybe_unused]] const application::application_report_t& application_report) {
    return phy::machigh_phy_t();
}

phy::machigh_phy_tx_t tfw_loopback_t::work_chscan_async(
    [[maybe_unused]] const phy::chscan_t& chscan) {
    return phy::machigh_phy_tx_t();
}

void tfw_loopback_t::work_stop() {
    if (state == STATE_t::DEAD_END) {
        save_all_results_to_file();
    }
}

void tfw_loopback_t::packet_params_t::update_plcf_unpacked() {
    plcf_10.HeaderFormat = 0;
    plcf_10.PacketLengthType = psdef.PacketLengthType;
    plcf_10.set_PacketLength_m1(psdef.PacketLength);
    plcf_10.ShortNetworkID = identity.ShortNetworkID;
    plcf_10.TransmitterIdentity = identity.ShortRadioDeviceID;
    plcf_10.set_TransmitPower(10);
    plcf_10.Reserved = 0;
    plcf_10.DFMCS = psdef.mcs_index;

    plcf_20.HeaderFormat = 0;
    plcf_20.PacketLengthType = plcf_10.PacketLengthType;
    plcf_20.PacketLength_m1 = plcf_10.PacketLength_m1;
    plcf_20.ShortNetworkID = plcf_10.ShortNetworkID;
    plcf_20.TransmitterIdentity = plcf_10.TransmitterIdentity;
    plcf_20.TransmitPower = plcf_10.TransmitPower;
    plcf_20.DFMCS = plcf_10.DFMCS;
    plcf_20.ReceiverIdentity = identity.ShortRadioDeviceID + 1;
    plcf_20.set_NumberOfSpatialStreams(sp3::tmmode::get_tm_mode(psdef.tm_mode_index).N_SS);
    plcf_20.DFRedundancyVersion = 0;
    plcf_20.DFNewDataIndication = 0;
    plcf_20.DFHARQProcessNumber = 0;
    plcf_20.FeedbackFormat = sp4::feedback_info_t::No_feedback;

    plcf_21.HeaderFormat = 1;
    plcf_21.PacketLengthType = plcf_10.PacketLengthType;
    plcf_21.PacketLength_m1 = plcf_10.PacketLength_m1;
    plcf_21.ShortNetworkID = plcf_10.ShortNetworkID;
    plcf_21.TransmitterIdentity = plcf_10.TransmitterIdentity;
    plcf_21.TransmitPower = plcf_10.TransmitPower;
    plcf_21.DFMCS = plcf_10.DFMCS;
    plcf_21.ReceiverIdentity = identity.ShortRadioDeviceID + 1;
    plcf_21.set_NumberOfSpatialStreams(sp3::tmmode::get_tm_mode(psdef.tm_mode_index).N_SS);
    plcf_21.Reserved = 0;
    plcf_21.FeedbackFormat = sp4::feedback_info_t::No_feedback;
}

void tfw_loopback_t::generate_packet(phy::machigh_phy_t& machigh_phy) {
    // request harq process
    auto* hp_tx = hpp.get_process_tx(pp.PLCF_type,
                                     pp.identity.NetworkID,
                                     pp.psdef,
                                     phy::harq::finalize_tx_t::reset_and_terminate);

    // every firmware has to decide how to deal with unavailable HARQ process
    if (hp_tx == nullptr) {
        dectnrp_log_wrn("HARQ process TX unavailable");
        return;
    }

    // this is now a well-defined packet size
    const sp3::packet_sizes_t& packet_sizes = hp_tx->get_packet_sizes();

    if (pp.PLCF_type == 1) {
        dectnrp_assert(hp_tx->get_rv() == pp.plcf_10.get_DFRedundancyVersion(), "incorrect rv");
        pp.plcf_10.pack(hp_tx->get_a_plcf());
    } else {
        if (pp.PLCF_type_header_format == 0) {
            dectnrp_assert(hp_tx->get_rv() == pp.plcf_20.get_DFRedundancyVersion(), "incorrect rv");
            pp.plcf_20.pack(hp_tx->get_a_plcf());
        } else {
            dectnrp_assert(hp_tx->get_rv() == pp.plcf_21.get_DFRedundancyVersion(), "incorrect rv");
            pp.plcf_21.pack(hp_tx->get_a_plcf());
        }
    }

    set_mac_pdu(hp_tx->get_a_tb(), packet_sizes.N_TB_byte);

    uint32_t codebook_index = 0;

    // determine a random CFO
    const float cfo_Hz = randomgen.rand_m1p1() * pp.cfo_symmetric_range_subc_multiple *
                         static_cast<float>(pp.psdef.u * constants::subcarrier_spacing_min_u_b);

    const phy::tx_meta_t tx_meta = {
        .optimal_scaling_DAC = false,
        .DAC_scale = phy::agc::agc_t::OFDM_AMPLITUDE_FACTOR_MINUS_00dB * pp.amplitude_scale,
        .iq_phase_rad = 0.0f,
        .iq_phase_increment_s2s_post_resampling_rad =
            common::adt::get_sample2sample_phase_inc(cfo_Hz, hw.get_samp_rate()),
        .GI_percentage = 5};

    const radio::buffer_tx_meta_t buffer_tx_meta = {.tx_order_id = tx_order_id,
                                                    .tx_time_64 = pp.tx_time_64};

    ++tx_order_id;

    machigh_phy.tx_descriptor_vec.push_back(
        phy::tx_descriptor_t(*hp_tx, codebook_index, tx_meta, buffer_tx_meta));
}

void tfw_loopback_t::set_mac_pdu(uint8_t* a_tb, const uint32_t N_TB_byte) {
    for (uint32_t i = 0; i < N_TB_byte; ++i) {
        a_tb[i] = std::rand() % 256;
    }
}

int64_t tfw_loopback_t::get_random_tx_time(const int64_t now_64) {
    // find next possible TX time
    int64_t tx_time_64 = now_64 + hw.get_tmin_samples(radio::hw_t::tmin_t::turnaround);

    // add a random jitter
    tx_time_64 += static_cast<int64_t>(randomgen.randi(
        0,
        static_cast<uint32_t>(
            duration_lut.get_N_samples_from_duration(sp3::duration_ec_t::subslot_u1_001))));

    // force TX time to some multiple
    tx_time_64 = common::adt::multiple_geq(tx_time_64, pp.tx_time_multiple_64);

    return tx_time_64;
}

}  // namespace dectnrp::upper::tfw::loopback
