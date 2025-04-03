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

#include "dectnrp/upper/tpoint_firmware/loopback/tfw_loopback.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>

#include "dectnrp/common/adt/freq_shift.hpp"
#include "dectnrp/common/prog/assert.hpp"
#include "dectnrp/common/prog/log.hpp"
#include "dectnrp/constants.hpp"
#include "dectnrp/external/nlohmann/json.hpp"
#include "dectnrp/phy/rx/sync/sync_param.hpp"

namespace dectnrp::upper::tfw::loopback {

const std::string tfw_loopback_t::firmware_name("loopback");

tfw_loopback_t::tfw_loopback_t(const tpoint_config_t& tpoint_config_, phy::mac_lower_t& mac_lower_)
    : tpoint_t(tpoint_config_, mac_lower_) {
    dectnrp_assert(100.f <= RX_SYNC_PARAM_AUTOCORRELATOR_DETECTION_RMS_THRESHOLD_MAX_SP,
                   "loopback requires large RMS limit");

    // set frequency, TX and RX power
    hw.set_command_time();
    hw.set_freq_tc(0.0);
    hw.set_tx_power_ant_0dBFS_tc(0.0f);
    hw.set_rx_power_ant_0dBFS_uniform_tc(0.0f);

    // init HARQ process pool
    hpp =
        std::make_unique<phy::harq::process_pool_t>(worker_pool_config.maximum_packet_sizes, 4, 4);

    // ##################################################
    // Radio Layer + PHY

    // loopback requires the hardware to be a simulator
    hw_simulator = dynamic_cast<radio::hw_simulator_t*>(&hw);

    dectnrp_assert(hw_simulator != nullptr, "hw not simulator");

    // called from tpoint firmware, thread-safe
    // hw_simulator->set_position(const simulation::topology::position_t position);
    hw_simulator->set_net_bandwidth_norm(1.0f / static_cast<float>(worker_pool_config.os_min));
    hw_simulator->set_tx_into_rx_leakage_dB(0.0f);
    hw_simulator->set_rx_noise_figure_dB(0.0f);
    hw_simulator->set_rx_snr_in_net_bandwidth_norm_dB(0.0f);

    // ##################################################
    // MAC Layer

    identity = section4::mac_architecture::identity_t(100, 10000000, 1000);
    ShortRadioDeviceID_rx = identity.ShortRadioDeviceID + 1;
    PLCF_type = 2;
    PLCF_type_header_format = 0;

    // ##################################################
    // measurement logic

    state = STATE_t::SET_SNR;
    state_time_reference_64 = 0;

    // ##################################################
    // What do we measure?

    snr_start = -2;
    snr_step = 1;
    snr_stop = 10;
    snr = snr_start;

    mcs_index_start = 1;
    mcs_index_end = 2;
    mcs_index = mcs_index_start;
    mcs_cnt = 0;

    nof_packets = 50;

    reset_for_new_run();

    for (uint32_t i = mcs_index_start; i <= mcs_index_end; ++i) {
        TB_bits.push_back(0);
        PER_pcc_crc.push_back(std::vector<float>());
        PER_pcc_crc_and_plcf.push_back(std::vector<float>());
        PER_pdc_crc.push_back(std::vector<float>());
    }

    // ##################################################
    // additional debugging

    packet_tx_time_multiple = 1;
    cfo_symmetric_range_subc_multiple = 1.75f;

    // ##################################################
    // utilities

    randomgen.shuffle();
}

void tfw_loopback_t::work_start_imminent(const int64_t start_time_64) {
    // start some time in the near future
    state_time_reference_64 = start_time_64 + duration_lut.get_N_samples_from_duration(
                                                  section3::duration_ec_t::ms001, 127);
}

phy::machigh_phy_t tfw_loopback_t::work_regular(const phy::phy_mac_reg_t& phy_mac_reg) {
    // get current time
    const int64_t now_64 = buffer_rx.get_rx_time_passed();

    // should we process the state machine?
    if (now_64 <= state_time_reference_64) {
        return phy::machigh_phy_t();
    }

    phy::machigh_phy_t machigh_phy;

    switch (state) {
        using enum STATE_t;
        case SET_SNR:
            {
                hw_simulator->set_rx_snr_in_net_bandwidth_norm_dB(snr);

                reset_for_new_run();

                state = SET_SMALL_SCALE_FADING;

                state_time_reference_64 = now_64 + duration_lut.get_N_samples_from_duration(
                                                       section3::duration_ec_t::ms001, 5);

                break;
            }

        case SET_SMALL_SCALE_FADING:
            {
                // re-randomize channel
                hw_simulator->wchannel_randomize_small_scale();

                state = GENERATE_PACKET;

                state_time_reference_64 = now_64 + duration_lut.get_N_samples_from_duration(
                                                       section3::duration_ec_t::ms001, 5);

                break;
            }

        case GENERATE_PACKET:
            {
                generate_one_new_packet(now_64, machigh_phy);

                ++nof_packets_cnt;

                if (nof_packets_cnt < nof_packets) {
                    state = SET_SMALL_SCALE_FADING;
                } else {
                    state = SAVE_RESULTS;
                }

                // must include the entire packet
                state_time_reference_64 = now_64 + duration_lut.get_N_samples_from_duration(
                                                       section3::duration_ec_t::ms001, 13);

                break;
            }

        case SAVE_RESULTS:
            {
                dectnrp_assert(nof_packets_cnt == nof_packets, "incorrect number of packets");

                // clang-format off
                const float per_pcc_crc = 1.0f - float(n_pcc_crc) / float(nof_packets);
                const float per_pcc_crc_and_plcf =1.0f - float(n_pcc_crc_and_plcf) / float(nof_packets);
                const float per_pdc_crc = 1.0f - float(n_pdc_crc) / float(nof_packets);
                // clang-format on

                // put result in global container
                // TB_bits[mcs_cnt] is directly written in function generate_one_new_packet()
                PER_pcc_crc[mcs_cnt].push_back(per_pcc_crc);
                PER_pcc_crc_and_plcf[mcs_cnt].push_back(per_pcc_crc_and_plcf);
                PER_pdc_crc[mcs_cnt].push_back(per_pdc_crc);

                dectnrp_log_inf(
                    "MCS={} SNR={} nof_packets={} | per_pcc_crc={} per_pcc_crc_and_plcf={} per_pdc_crc={} | snr_max={} snr_min={}",
                    mcs_index,
                    snr,
                    nof_packets,
                    per_pcc_crc,
                    per_pcc_crc_and_plcf,
                    per_pdc_crc,
                    snr_max,
                    snr_min);

                state = SETUP_NEXT_MEASUREMENT;

                break;
            }

        case SETUP_NEXT_MEASUREMENT:
            {
                snr += snr_step;

                state = SET_SNR;

                // abort condition for SNR
                if (snr > snr_stop) {
                    snr = snr_start;
                    ++mcs_index;
                    ++mcs_cnt;

                    dectnrp_log_inf(" ");

                    // global abort condition for MCS
                    if (mcs_index > mcs_index_end) {
                        state = DEAD_END;
                        dectnrp_log_inf("All measurements finished.");
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

    return machigh_phy;
}

phy::maclow_phy_t tfw_loopback_t::work_pcc(const phy::phy_maclow_t& phy_maclow) {
    ++n_pcc_crc;

    // base pointer to extract PLCF_type
    const auto* plcf_base = phy_maclow.pcc_report.plcf_decoder.get_plcf_base(PLCF_type);

    // is this the correct PLCF type?
    if (plcf_base == nullptr) {
        return phy::maclow_phy_t();
    }

    // is this the correct header type?
    if (plcf_base->get_HeaderFormat() != PLCF_type_header_format) {
        return phy::maclow_phy_t();
    }

    ++n_pcc_crc_and_plcf;

    return worksub_pcc2pdc(phy_maclow,
                           PLCF_type,
                           identity.NetworkID,
                           0,
                           phy::harq::finalize_rx_t::reset_and_terminate,
                           phy::maclow_phy_handle_t());
}

phy::machigh_phy_t tfw_loopback_t::work_pdc_async(const phy::phy_machigh_t& phy_machigh) {
    if (phy_machigh.pdc_report.crc_status) {
        ++n_pdc_crc;

        snr_max = std::max(snr_max, phy_machigh.pdc_report.snr_dB);
        snr_min = std::min(snr_min, phy_machigh.pdc_report.snr_dB);
    }

    return phy::machigh_phy_t();
}

phy::machigh_phy_t tfw_loopback_t::work_upper(const upper::upper_report_t& upper_report) {
    return phy::machigh_phy_t();
}

phy::machigh_phy_tx_t tfw_loopback_t::work_chscan_async(const phy::chscan_t& chscan) {
    return phy::machigh_phy_tx_t();
}

std::vector<std::string> tfw_loopback_t::start_threads() {
    return std::vector<std::string>{{"tpoint " + firmware_name + " " + std::to_string(id)}};
}

std::vector<std::string> tfw_loopback_t::stop_threads() {
    if (state == STATE_t::DEAD_END) {
        save_to_files();
    }

    return std::vector<std::string>{{"tpoint " + firmware_name + " " + std::to_string(id)}};
}

void tfw_loopback_t::reset_for_new_run() {
    nof_packets_cnt = 0;

    n_pcc_crc = 0;
    n_pcc_crc_and_plcf = 0;
    n_pdc_crc = 0;

    snr_max = -100.0e6;
    snr_min = -snr_max;
}

void tfw_loopback_t::generate_one_new_packet(const int64_t now_64,
                                             phy::machigh_phy_t& machigh_phy) {
    // define required input to calculate packet dimensions
    const section3::packet_sizes_def_t psdef = {.u = worker_pool_config.radio_device_class.u_min,
                                                .b = worker_pool_config.radio_device_class.b_min,
                                                .PacketLengthType = 1,
                                                .PacketLength = 1,
                                                .tm_mode_index = 0,
                                                .mcs_index = mcs_index,
                                                .Z = worker_pool_config.radio_device_class.Z_min};

    // request harq process
    auto* hp_tx = hpp->get_process_tx(
        PLCF_type, identity.NetworkID, psdef, phy::harq::finalize_tx_t::reset_and_terminate);

    // every firmware has to decide how to deal with unavailable HARQ process
    if (hp_tx == nullptr) {
        dectnrp_log_wrn("HARQ process TX unavailable. Bug in firmware.");
        return;
    }

    // this is now a well-defined packet size
    const section3::packet_sizes_t& packet_sizes = hp_tx->get_packet_sizes();

    // save number of bits in transport block directly into results container
    TB_bits[mcs_cnt] = packet_sizes.N_TB_bits;

    // pick beamforming codebook index, for beacon usually 0 to be used for channel sounding
    uint32_t codebook_index = 0;

    // random number to have some variation of the CFO
    randomgen.shuffle();

    // find a random CFO
    const float cfo_Hz = randomgen.rand_m1p1() * cfo_symmetric_range_subc_multiple *
                         static_cast<float>(psdef.u * constants::subcarrier_spacing_min_u_b);

    // define non-dect metadata of transmission
    section3::tx_meta_t tx_meta = {
        .optimal_scaling_DAC = false,
        .DAC_scale = phy::agc::agc_t::OFDM_AMPLITUDE_FACTOR_MINUS_00dB,
        .iq_phase_rad = 0.0f,
        .iq_phase_increment_s2s_post_resampling_rad =
            common::adt::get_sample2sample_phase_inc(cfo_Hz, hw.get_samp_rate()),
        .GI_percentage = 25};

    // jitter of TX time
    const int64_t random_jitter_64 = static_cast<int64_t>(randomgen.randi(
        0,
        static_cast<uint32_t>(
            duration_lut.get_N_samples_from_duration(section3::duration_ec_t::us100))));

    // required meta data on radio layer
    radio::buffer_tx_meta_t buffer_tx_meta = {
        .tx_order_id = tx_order_id,
        .tx_time_64 =
            now_64 +
            duration_lut.get_N_samples_from_duration(section3::duration_ec_t::turn_around_time_us) +
            random_jitter_64};

    // force transmission time to multiple of packet_tx_time_multiple
    const int64_t res = buffer_tx_meta.tx_time_64 % packet_tx_time_multiple;
    if (res != 0) {
        buffer_tx_meta.tx_time_64 += packet_tx_time_multiple - res;
    }

    // define PLCF type 1, header format 0
    section4::plcf_10_t plcf_10;
    plcf_10.HeaderFormat = 0;
    plcf_10.PacketLengthType = psdef.PacketLengthType;
    plcf_10.set_PacketLength_m1(psdef.PacketLength);
    plcf_10.ShortNetworkID = identity.ShortNetworkID;
    plcf_10.TransmitterIdentity = identity.ShortRadioDeviceID;
    plcf_10.set_TransmitPower(10);
    plcf_10.Reserved = 0;
    plcf_10.DFMCS = psdef.mcs_index;

    // choose PLCF type 1 or 2
    if (PLCF_type == 1) {
        plcf_10.pack(hp_tx->get_a_plcf());
    } else {
        if (PLCF_type_header_format == 0) {
            section4::plcf_20_t plcf_20;
            plcf_20.HeaderFormat = 0;
            plcf_20.PacketLengthType = plcf_10.PacketLengthType;
            plcf_20.PacketLength_m1 = plcf_10.PacketLength_m1;
            plcf_20.ShortNetworkID = plcf_10.ShortNetworkID;
            plcf_20.TransmitterIdentity = plcf_10.TransmitterIdentity;
            plcf_20.TransmitPower = plcf_10.TransmitPower;
            plcf_20.DFMCS = plcf_10.DFMCS;
            plcf_20.ReceiverIdentity = ShortRadioDeviceID_rx;
            plcf_20.set_NumberOfSpatialStreams(packet_sizes.tm_mode.N_SS);
            plcf_20.DFRedundancyVersion = hp_tx->get_rv();
            plcf_20.DFNewDataIndication = 0;
            plcf_20.DFHARQProcessNumber = 0;
            plcf_20.FeedbackFormat = 0;

            plcf_20.pack(hp_tx->get_a_plcf());

        } else {
            section4::plcf_21_t plcf_21;
            plcf_21.HeaderFormat = 1;
            plcf_21.PacketLengthType = plcf_10.PacketLengthType;
            plcf_21.PacketLength_m1 = plcf_10.PacketLength_m1;
            plcf_21.ShortNetworkID = plcf_10.ShortNetworkID;
            plcf_21.TransmitterIdentity = plcf_10.TransmitterIdentity;
            plcf_21.TransmitPower = plcf_10.TransmitPower;
            plcf_21.DFMCS = plcf_10.DFMCS;
            plcf_21.ReceiverIdentity = ShortRadioDeviceID_rx;
            plcf_21.set_NumberOfSpatialStreams(packet_sizes.tm_mode.N_SS);
            plcf_21.Reserved = 0;
            plcf_21.FeedbackFormat = 0;

            plcf_21.pack(hp_tx->get_a_plcf());
        }
    }

    // increase tx order id
    ++tx_order_id;

    // fill transport block with random data
    uint8_t* a_tb = hp_tx->get_a_tb();
    for (uint32_t i = 0; i < packet_sizes.N_TB_byte; ++i) {
        a_tb[i] = std::rand() % 256;
    }

    // add to tx_descriptor_vec
    machigh_phy.tx_descriptor_vec.push_back(
        phy::tx_descriptor_t(*hp_tx, codebook_index, tx_meta, buffer_tx_meta));
}

void tfw_loopback_t::save_to_files() const {
    // create SNR vector
    std::vector<float> snr_vec;
    for (float snr_ = snr_start; snr_ <= snr_stop; snr_ += snr_step) {
        snr_vec.push_back(snr_);
    }

    // save one file for every MCS
    uint32_t mcs_cnt_local = 0;
    for (uint32_t i = mcs_index_start; i <= mcs_index_end; ++i) {
        // save all data to json file
        std::ostringstream filename;
        filename << "rx_loopback_MCS_" << std::setw(4) << std::setfill('0') << i;

        nlohmann::ordered_json j_packet_data;

        j_packet_data["nof_packets"] = nof_packets;
        j_packet_data["MCS_index"] = i;
        j_packet_data["TB_bits"] = TB_bits[mcs_cnt_local];

        j_packet_data["data"]["snr_vec"] = snr_vec;
        j_packet_data["data"]["PER_pcc_crc"] = PER_pcc_crc[mcs_cnt_local];
        j_packet_data["data"]["PER_pcc_crc_and_plcf"] = PER_pcc_crc_and_plcf[mcs_cnt_local];
        j_packet_data["data"]["PER_pdc_crc"] = PER_pdc_crc[mcs_cnt_local];

        std::ofstream out_file(filename.str());
        out_file << std::setw(4) << j_packet_data << std::endl;
        out_file.close();

        ++mcs_cnt_local;
    }
}

}  // namespace dectnrp::upper::tfw::loopback
