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

#include "dectnrp/radio/hw_simulator.hpp"

#include <utility>

#include "dectnrp/common/prog/assert.hpp"
#include "dectnrp/common/thread/watch.hpp"
#include "dectnrp/radio/calibration/cal_simulator.hpp"
#include "dectnrp/radio/complex.hpp"
#include "dectnrp/simulation/hardware/clip.hpp"
#include "dectnrp/simulation/hardware/quantize.hpp"

// #define SET_BREAKPOINT_AT_START_OF_EVERY_PACKET
// #define SET_BREAKPOINT_AT_END_OF_EVERY_PACKET

#if defined(SET_BREAKPOINT_AT_START_OF_EVERY_PACKET) || \
    defined(SET_BREAKPOINT_AT_END_OF_EVERY_PACKET)
#include "dectnrp/common/prog/debugbreakpoint.hpp"
#endif

namespace dectnrp::radio {

const std::string hw_simulator_t::name = "simulator";

hw_simulator_t::hw_simulator_t(const hw_config_t& hw_config_, simulation::vspace_t& vspace_)
    : hw_t(hw_config_),
      vspace(vspace_) {
    dectnrp_assert(hw_config.rx_prestream_ms == 0,
                   "simulator has to stay aligned with vspace and hence does not prestream");

    nof_antennas_max = 8;
    ADC_bits = 12;
    DAC_bits = 12;
    tmin_us.at(std::to_underlying(tmin_t::freq)) = 250;
    tmin_us.at(std::to_underlying(tmin_t::gain)) = 50;
    tmin_us.at(std::to_underlying(tmin_t::turnaround)) = hw_config.turnaround_time_us;

    ppm = 0.0f;
    time_advance_fpga2ant_samples = 0;

    gain_lut.freqs_Hz = calibration::simulator::freqs_Hz;

    gain_lut.gains_tx_dB = &calibration::simulator::gains_tx_dB;
    gain_lut.powers_tx_dBm = &calibration::simulator::powers_tx_dBm;
    gain_lut.gains_tx_dB_step = calibration::simulator::gains_tx_dB_step;

    gain_lut.gains_rx_dB = &calibration::simulator::gains_rx_dB;
    gain_lut.powers_rx_dBm = &calibration::simulator::powers_rx_dBm;
    gain_lut.gains_rx_dB_step = calibration::simulator::gains_rx_dB_step;
}

void hw_simulator_t::set_samp_rate(const uint32_t samp_rate_in) {
    dectnrp_assert(0 < samp_rate_in, "sample rate is 0");

    if (!hw_config_t::sim_samp_rate_lte) {
        // assume we can generate any DECT NR+ sample rate
        samp_rate = samp_rate_in;
    } else {
        // multiple of the LTE base frequency 30.72MS/s*16 = 245760000MS/s*2 = 491520000MS/s.
        const uint32_t master_clock_rate = 491520000;
        const std::vector<uint32_t> decimation_factors = {1, 2, 4, 6, 8, 16, 32, 64, 128, 256};

        // find the smallest sample rate larger than samp_rate_in
        samp_rate = master_clock_rate;
        for (uint32_t i = 0; i < decimation_factors.size(); ++i) {
            const uint32_t lower_suitable_usrp_samp_rate =
                master_clock_rate / decimation_factors[i];

            if (lower_suitable_usrp_samp_rate < samp_rate_in) {
                break;
            } else {
                samp_rate = lower_suitable_usrp_samp_rate;
            }
        }
    }

    dectnrp_assert(samp_rate_in <= samp_rate, "sample rate smaller than input sample rate");

    // convert from us to samples
    for (std::size_t i = 0; i < std::to_underlying(tmin_t::CARDINALITY); ++i) {
        tmin_samples.at(i) = get_samples_in_us(tmin_us.at(i));
    }
}

void hw_simulator_t::initialize_buffer_tx_pool(const uint32_t ant_streams_length_samples_max) {
    dectnrp_assert(tx_gap_samples > 0, "allowed gap size should be larger than 0");

    buffer_tx_pool = std::make_unique<buffer_tx_pool_t>(
        id, nof_antennas, hw_config.nof_buffer_tx, ant_streams_length_samples_max + tx_gap_samples);
}

void hw_simulator_t::initialize_buffer_rx(const uint32_t ant_streams_length_samples) {
    buffer_rx = std::make_unique<buffer_rx_t>(id,
                                              nof_antennas,
                                              ant_streams_length_samples,
                                              samp_rate,
                                              vspptx->spp_size,
                                              hw_config.rx_prestream_ms,
                                              hw_config.rx_notification_period_us);
}

void hw_simulator_t::initialize_device() {
    dectnrp_assert(50 <= hw_config.sim_spp_us, "spp too small");
    dectnrp_assert(hw_config.sim_spp_us <= 500, "spp too large");

    // maximum number of samples per spp
    const double spp_size_d = double(samp_rate) * double(hw_config.sim_spp_us) / double(1e6);
    const uint32_t spp_size = static_cast<uint32_t>(spp_size_d);

    dectnrp_assert(spp_size >= 15, "minimum spp size");

    // init spp
    vspptx = std::make_unique<simulation::vspptx_t>(id, nof_antennas, samp_rate, spp_size);
    vspprx = std::make_unique<simulation::vspprx_t>(id, nof_antennas, samp_rate, spp_size);

    set_command_time();
    set_freq_tc(HW_DEFAULT_FREQ_HZ);

    set_tx_power_ant_0dBFS_tc(-1000.0f);         // minimum TX power
    set_rx_power_ant_0dBFS_uniform_tc(1000.0f);  // minimum RX sensitivity
}

void hw_simulator_t::start_threads_and_iq_streaming() {
    dectnrp_assert(!keep_running.load(std::memory_order_acquire), "keep_running already true");

    dectnrp_assert(0 < nof_antennas && nof_antennas <= nof_antennas_max,
                   "number of antennas not set correctly");
    dectnrp_assert(samp_rate > 0, "sample rate not set correctly");
    dectnrp_assert(tx_gap_samples > 0, "minimum size of gap not set correctly");

    // set before starting threads
    keep_running.store(true, std::memory_order_release);

    // start tx thread
    if (!common::threads_new_rt_mask_custom(
            &thread_tx, &work_tx, this, hw_config.tx_thread_config)) {
        dectnrp_assert_failure("simulator unable to start TX thread");
    }

    log_line(std::string("Thread TX " +
                         common::get_thread_properties(thread_tx, hw_config.tx_thread_config)));

#ifdef RADIO_HW_SLEEP_BEFORE_STARTING_RX_THREAD_MS
    common::watch_t::sleep<common::milli>(RADIO_HW_SLEEP_BEFORE_STARTING_RX_THREAD_MS);
#endif

    // start rx thread
    if (!common::threads_new_rt_mask_custom(
            &thread_rx, &work_rx, this, hw_config.rx_thread_config)) {
        dectnrp_assert_failure("simulator unable to start RX thread");
    }

    log_line(std::string("Thread RX " +
                         common::get_thread_properties(thread_rx, hw_config.rx_thread_config)));

    std::string str("Simulator");
    str.append(" nof_antennas " + std::to_string(nof_antennas));
    str.append(" samp_rate " + std::to_string(samp_rate));
    str.append(" spp_size " + std::to_string(buffer_rx->nof_new_samples_max));
    str.append(" buffer_tx_pool.size() " + std::to_string(buffer_tx_pool->nof_buffer_tx));
    str.append(" buffer_rx->ant_streams_length_samples " +
               std::to_string(buffer_rx->ant_streams_length_samples));
    log_line(str);
}

void hw_simulator_t::set_command_time([[maybe_unused]] const int64_t set_time) {
    // timed commands are not implemented for the simulator
}

double hw_simulator_t::set_freq_tc(const double freq_Hz) {
    // NOTE: set_xx_power_ant_0dBFS locks the same mutex!
    std::unique_lock<std::mutex> lock(hw_mtx);

    // equivalent to making a hardware change
    vspptx->meta.freq_Hz = freq_Hz;

    return vspptx->meta.freq_Hz;
}

float hw_simulator_t::set_tx_power_ant_0dBFS_tc(const float power_dBm) {
    std::unique_lock<std::mutex> lock(hw_mtx);

    const auto achievable_power_gain =
        gain_lut.get_achievable_power_gain_tx(power_dBm, vspptx->meta.freq_Hz);

    // equivalent to making a hardware change
    vspptx->meta.tx_power_ant_0dBFS = achievable_power_gain.power_dBm;

    tx_power_ant_0dBFS = achievable_power_gain.power_dBm;

    return tx_power_ant_0dBFS;
}

float hw_simulator_t::set_rx_power_ant_0dBFS_tc(const float power_dBm, const size_t idx) {
    std::unique_lock<std::mutex> lock(hw_mtx);

    const auto achievable_power_gain =
        gain_lut.get_achievable_power_gain_rx(power_dBm, vspptx->meta.freq_Hz);

    // equivalent to making a hardware change
    vspprx->meta.rx_power_ant_0dBFS.at(idx) = achievable_power_gain.power_dBm;

    rx_power_ant_0dBFS.at(idx) = achievable_power_gain.power_dBm;

    return rx_power_ant_0dBFS.at(idx);
}

#ifdef RADIO_HW_IMPLEMENTS_GPIO_TOGGLE
void hw_simulator_t::toggle_gpio_tc() {
    // nothing to do here
}
#endif

void hw_simulator_t::pps_wait_for_next() const {
    const auto A = common::watch_t::get_elapsed_since_epoch<int64_t, common::micro>();

    // at what absolute time does the PPS occur in the current second?
    const auto C = A - (A % 1000000) + hw_config.full_second_to_pps_us;

    // what is the next witnessable PPS time?
    const auto target_time_64 = A < C ? C : C + 1000000;

    do {
        common::watch_t::sleep<common::milli>(1);
    } while (common::watch_t::get_elapsed_since_epoch<int64_t, common::micro>() <= target_time_64);
}

void hw_simulator_t::pps_set_full_sec_at_next_pps_and_wait_until_it_passed() {
    dectnrp_assert(buffer_rx.get() != nullptr, "buffer not initialized");

    pps_wait_for_next();

    const int64_t full_sec = pps_time_base_sec_in_one_second();

    buffer_rx->time_as_sample_cnt_64 = full_sec * static_cast<int64_t>(samp_rate);
    buffer_rx->rx_time_passed_64.store(buffer_rx->time_as_sample_cnt_64, std::memory_order_release);
    vspptx->meta.now_64 = buffer_rx->time_as_sample_cnt_64;
    vspprx->meta.now_64 = buffer_rx->time_as_sample_cnt_64;

    pps_wait_for_next();

    /* The PPS just happened. Immediately measure the operating system time and save the offset
     * between the full second of the operating system and the PPS.
     */
    const int64_t A =
        common::watch_t::get_elapsed_since_epoch<int64_t, common::nano>() % 1000000000;
    full_second_to_pps_measured_samples = A * static_cast<int64_t>(samp_rate) / 1000000000;

    dectnrp_assert(full_second_to_pps_measured_samples < samp_rate, "ill-defined");
}

void hw_simulator_t::set_trajectory(const simulation::topology::trajectory_t trajectory) {
    std::unique_lock<std::mutex> lock(hw_mtx);

    vspptx->meta.trajectory = trajectory;
}

void hw_simulator_t::set_net_bandwidth_norm(const float net_bandwidth_norm) {
    std::unique_lock<std::mutex> lock(hw_mtx);

    vspptx->meta.net_bandwidth_norm = net_bandwidth_norm;
}

void hw_simulator_t::set_tx_into_rx_leakage_dB(const float tx_into_rx_leakage_dB) {
    std::unique_lock<std::mutex> lock(hw_mtx);

    vspptx->meta.tx_into_rx_leakage_dB = tx_into_rx_leakage_dB;
}

void hw_simulator_t::set_rx_noise_figure_dB(const float rx_noise_figure_dB) {
    std::unique_lock<std::mutex> lock(hw_mtx);

    vspprx->meta.rx_noise_figure_dB = rx_noise_figure_dB;
}

void hw_simulator_t::set_rx_snr_in_net_bandwidth_norm_dB(
    const float rx_snr_in_net_bandwidth_norm_dB) {
    std::unique_lock<std::mutex> lock(hw_mtx);

    vspprx->meta.rx_snr_in_net_bandwidth_norm_dB = rx_snr_in_net_bandwidth_norm_dB;
}

void hw_simulator_t::set_all_buffers_as_transmitted() {
    for (auto& elem : buffer_tx_pool->buffer_tx_vec) {
        if (elem->is_inner_locked()) {
            elem->set_transmitted_or_abort();
        }
    }
}

void hw_simulator_t::work_stop() {
    dectnrp_assert(keep_running.load(std::memory_order_acquire), "keep_running already false");

    keep_running.store(false, std::memory_order_release);

    pthread_join(thread_rx, NULL);
    pthread_join(thread_tx, NULL);

    std::string str("Simulator");
    str.append(" Sample Rate Target " + std::to_string(static_cast<double>(samp_rate)));
    str.append(" TX Samples sent " + std::to_string(tx_stats.samples_sent));
    str.append(" TX Sample Rate Is " + std::to_string(tx_stats.samp_rate_is));
    str.append(" RX Samples received " + std::to_string(rx_stats.samples_received));
    str.append(" RX Sample Rate Is " + std::to_string(rx_stats.samp_rate_is));
    log_line(str);

    for (auto& elem : buffer_tx_pool->buffer_tx_vec) {
        log_lines(elem->report_stop());
    }
}

void* hw_simulator_t::work_tx(void* hw_simulator) {
    hw_simulator_t* calling_instance = reinterpret_cast<hw_simulator_t*>(hw_simulator);

    // for readability
    auto& vspace = calling_instance->vspace;
    auto& hw_mtx = calling_instance->hw_mtx;
    auto& vspptx = *calling_instance->vspptx.get();
    auto& buffer_tx_pool = *calling_instance->buffer_tx_pool.get();
    const auto spp_size = calling_instance->buffer_rx->nof_new_samples_max;
    auto& keep_running = calling_instance->keep_running;
    auto buffer_tx_vec = buffer_tx_pool.get_buffer_tx_vec();

    // device registration
    vspace.hw_register_tx(vspptx);
    vspace.wait_for_all_rx_registered_and_inits_done_nto();

    // reset TX counter
    uint64_t tx_order_id_expected = 0;

    // working copy
    std::vector<void*> ant_streams(calling_instance->nof_antennas);

    // get latest time
    const int64_t now_start_64 = calling_instance->buffer_rx->get_rx_time_passed();
    int64_t now_64 = now_start_64;

    dectnrp_assert(
        !(calling_instance->hw_config.pps_time_base == hw_config_t::pps_time_base_t::zero &&
          now_64 != 0),
        "buffer RX time not at zero");

    // measure wall clock execution time
    common::watch_t watch;

    while (keep_running.load(std::memory_order_acquire)) {
        // check if the currently expected TX buffer is ready
        int32_t buffer_tx_idx =
            buffer_tx_pool.get_specific_tx_order_id_if_available(tx_order_id_expected);

        if (buffer_tx_idx >= 0) {
            /* At this point, we know that a packet can be transmitted. We now keep transmitting
             * zeros until the packets starts, and then we transmit the actual samples. This process
             * cannot be interrupted and we always wait for the virtual space with no timeout (nto).
             */

            // transmission length
            uint32_t tx_length_samples = buffer_tx_vec[buffer_tx_idx]->tx_length_samples;

            dectnrp_assert(tx_length_samples > spp_size,
                           "transmission length must be larger than one spp");

            // transmission time
            int64_t tx_time_64 = buffer_tx_vec[buffer_tx_idx]->buffer_tx_meta.tx_time_64;

            dectnrp_assert(tx_time_64 >= now_64,
                           "transmission time of expected packet earlier than current time");

            // how many zero samples must be transmitted before the first sample of the packet?
            const int64_t time2tx = tx_time_64 - now_64;

            // how many full zero spp are that?
            int64_t nof_spp_zero = time2tx / static_cast<int64_t>(spp_size);

            vspptx.spp_zero();
            vspptx.tx_set_no_non_zero_samples();

            while (nof_spp_zero > 0) {
                // wait for vspace to become ready with no timeout
                vspace.wait_writable_nto(calling_instance->id);

                // send zeros into virtual space
                {
                    std::unique_lock<std::mutex> lock(hw_mtx);
                    vspptx.meta.now_64 = now_64;
                    vspace.write(vspptx);
                }

                // increase local simulation time
                now_64 += static_cast<int64_t>(spp_size);

                --nof_spp_zero;
            }

            // how many zero samples are left before the first sample of the packet?
            uint32_t spp_offset = static_cast<uint32_t>(tx_time_64 - now_64);

            dectnrp_assert(spp_offset < spp_size, "nof leading zeros larger than one spp");

            // number of transmitted samples
            uint32_t tx_length_samples_cnt = 0;

#ifdef SET_BREAKPOINT_AT_START_OF_EVERY_PACKET
            dectnrp_debugbreak("Start of packet. id={} tx_time_64={} time={:.3f}ms",
                               calling_instance->id,
                               tx_time_64,
                               static_cast<double>(tx_time_64) * 1000.0 /
                                   static_cast<double>(calling_instance->get_samp_rate()));
#endif

            vspptx.spp_zero();

            // start transmitting until the entire packet is gone
            while (tx_length_samples_cnt < tx_length_samples) {
                // how many non-zero samples can we put into the spp?
                uint32_t tx_length_samples_this_spp =
                    std::min(tx_length_samples - tx_length_samples_cnt, spp_size - spp_offset);

                // wait until enough samples are available
                buffer_tx_vec[buffer_tx_idx]->wait_for_samples_busy_nto(tx_length_samples_cnt +
                                                                        tx_length_samples_this_spp);

                // get pointer to TX buffer with correct offset
                buffer_tx_vec[buffer_tx_idx]->get_ant_streams_offset(ant_streams,
                                                                     tx_length_samples_cnt);

                // copy to vspptx for later deep copy
                vspptx.spp_write_v(ant_streams, spp_offset, tx_length_samples_this_spp);

                // set meta data of transmission
                vspptx.tx_idx = static_cast<int32_t>(spp_offset);
                vspptx.tx_length = static_cast<int32_t>(tx_length_samples_this_spp);

                // add processed samples
                tx_length_samples_cnt += tx_length_samples_this_spp;

                dectnrp_assert(tx_length_samples_cnt <= tx_length_samples,
                               "more samples counted than packet has in total");

                // were these the final samples of the current packet?
                if (tx_length_samples_cnt == tx_length_samples) {
                    // set buffer as transmitted
                    buffer_tx_vec[buffer_tx_idx]->set_transmitted_or_abort();

#ifdef SET_BREAKPOINT_AT_END_OF_EVERY_PACKET
                    dectnrp_debugbreak(
                        "End of packet.   id={} tx_time_64={}", calling_instance->id, tx_time_64);
#endif

                    // this buffer is as good as sent ...
                    if (buffer_tx_vec[buffer_tx_idx]->buffer_tx_meta.tx_order_id_expect_next < 0) {
                        // ... so either increase ID counter for next packet by one ...
                        ++tx_order_id_expected;
                    } else {
                        // ... or overwrite if firmware wants to change the order
                        tx_order_id_expected =
                            buffer_tx_vec[buffer_tx_idx]->buffer_tx_meta.tx_order_id_expect_next;
                    }

                    ++calling_instance->tx_stats.buffer_tx_sent;

                    /* Check whether packet ends somewhere in the center of the spp. If so, another
                     * transmission could be started in the current spp.
                     */
                    if (spp_offset + tx_length_samples_this_spp < spp_size) {
                        // check if the currently expected TX buffer is ready
                        buffer_tx_idx = buffer_tx_pool.get_specific_tx_order_id_if_available(
                            tx_order_id_expected);

                        if (buffer_tx_idx >= 0) {
                            // transmission length
                            tx_length_samples = buffer_tx_vec[buffer_tx_idx]->tx_length_samples;

                            dectnrp_assert(tx_length_samples > spp_size,
                                           "transmission length must be larger than one spp");

                            // transmission time
                            tx_time_64 = buffer_tx_vec[buffer_tx_idx]->buffer_tx_meta.tx_time_64;

                            dectnrp_assert(
                                tx_time_64 >= now_64 + static_cast<int64_t>(
                                                           spp_offset + tx_length_samples_this_spp),
                                "transmission time of expected packet earlier than current time");

                            // does the transmission start in the current spp?
                            if (tx_time_64 < now_64 + static_cast<int64_t>(spp_size)) {
                                // reset counter
                                tx_length_samples_cnt = 0;

                                // set offset to start of current packet
                                spp_offset = static_cast<int64_t>(tx_time_64 - now_64);

                                dectnrp_assert(spp_offset < spp_size,
                                               "nof leading samples larger than one spp");

                                // how many non-zero samples can we put into the spp?
                                tx_length_samples_this_spp = spp_size - spp_offset;

                                // wait until enough samples are available
                                buffer_tx_vec[buffer_tx_idx]->wait_for_samples_busy_nto(
                                    tx_length_samples_this_spp);

                                // get pointer to TX buffer with correct offset
                                buffer_tx_vec[buffer_tx_idx]->get_ant_streams_offset(ant_streams,
                                                                                     0);

                                // copy to vspptx for later deep copy
                                vspptx.spp_write_v(
                                    ant_streams, spp_offset, tx_length_samples_this_spp);

                                // apply hardware effects directly to spp
                                /// \todo

                                // set meta data of transmission
                                vspptx.tx_idx = static_cast<int32_t>(0);
                                vspptx.tx_length = static_cast<int32_t>(spp_size);

                                // add processed samples
                                tx_length_samples_cnt += tx_length_samples_this_spp;
                            }
                        }
                    }
                }

                // apply hardware effects directly to spp
                if (calling_instance->hw_config.simulator_clip_and_quantize) {
                    clip_and_quantize(vspptx.spp,
                                      vspptx.spp,
                                      vspptx.spp_size,
                                      symmetric_clipping_amplitude,
                                      calling_instance->get_DAC_bits());
                }

                // wait for vspace to become ready with no timeout
                vspace.wait_writable_nto(calling_instance->id);

                // send samples to virtual space
                {
                    std::unique_lock<std::mutex> lock(hw_mtx);
                    vspptx.meta.now_64 = now_64;
                    vspace.write(vspptx);
                }

                // increase local simulation time
                now_64 += static_cast<int64_t>(spp_size);

                // starting with the second TX spp, offset is always zero
                spp_offset = 0;

                vspptx.spp_zero();
            }
        }
        // expected TX buffer not available, so send zeros
        else {
            vspptx.spp_zero();
            vspptx.tx_set_no_non_zero_samples();

            /* We wait for the virtual space to become ready, indicated by a return value true. Only
             * then do we copy samples to the virtual space. However, if the function returns false
             * due to a timeout, we don't copy samples to the virtual space and instead check the
             * external exit condition keep_running.
             */
            if (vspace.wait_writable_to(calling_instance->id)) {
                // send zeros to virtual space
                {
                    std::unique_lock<std::mutex> lock(hw_mtx);
                    vspptx.meta.now_64 = now_64;
                    vspace.write(vspptx);
                }

                // increase local simulation time
                now_64 += static_cast<int64_t>(spp_size);
            }
        }
    }

    // set stats
    calling_instance->tx_stats.samples_sent = now_64 - now_start_64;
    calling_instance->tx_stats.samp_rate_is =
        static_cast<double>(calling_instance->tx_stats.samples_sent) /
        (static_cast<double>(watch.get_elapsed<uint64_t, common::milli>()) / 1000.0);

    return nullptr;
}

void* hw_simulator_t::work_rx(void* hw_simulator) {
    hw_simulator_t* calling_instance = reinterpret_cast<hw_simulator_t*>(hw_simulator);

    // for readability
    auto& vspace = calling_instance->vspace;
    auto& hw_mtx = calling_instance->hw_mtx;
    auto& vspprx = *calling_instance->vspprx.get();
    auto& buffer_rx = *calling_instance->buffer_rx;
    const auto spp_size = calling_instance->buffer_rx->nof_new_samples_max;
    auto& keep_running = calling_instance->keep_running;

    // device registration
    vspace.wait_for_all_tx_registered_and_inits_done_nto();

    // TX are all registered and waiting for RX, so now we can adjust the time
    calling_instance->pps_set_full_sec_at_next_pps_and_wait_until_it_passed();

    vspace.hw_register_rx(vspprx);

    std::vector<void*> ant_streams(calling_instance->nof_antennas);

    dectnrp_assert(ant_streams.size() == buffer_rx.nof_antennas, "incorrect number of antennas");

    // init pointer into streaming buffer
    buffer_rx.get_ant_streams_next(ant_streams, 0, 0);

    // zero RX buffer
    buffer_rx.set_zero();

    // get latest time
    const int64_t now_start_64 = calling_instance->buffer_rx->get_rx_time_passed();
    int64_t now_64 = now_start_64;

    dectnrp_assert(
        !(calling_instance->hw_config.pps_time_base == hw_config_t::pps_time_base_t::zero &&
          now_64 != 0),
        "buffer RX time not at zero");

    // wait until all simulators are registered
    vspace.wait_for_all_rx_registered_and_inits_done_nto();

    // measure wall clock execution time
    common::watch_t watch;

    while (keep_running.load(std::memory_order_acquire)) {
        /* We wait for the virtual space to become ready, indicated by a return value true. Only
         * then do we copy samples from the virtual space. However, if the function returns false
         * due to a timeout, we don't copy samples from the virtual space and instead check the
         * external exit condition keep_running.
         */
        if (!vspace.wait_readable_to(calling_instance->id)) {
            continue;
        }

        // copy from vspace
        {
            std::unique_lock<std::mutex> lock(hw_mtx);
            vspprx.meta.now_64 = now_64;
            vspace.read(vspprx);
        }

        // copy from virtual interface to actual buffer_rx
        vspprx.spp_read_v(ant_streams, spp_size);

        // apply hardware effects directly to spp
        if (calling_instance->hw_config.simulator_clip_and_quantize) {
            clip_and_quantize(vspprx.spp,
                              vspprx.spp,
                              vspprx.spp_size,
                              symmetric_clipping_amplitude,
                              calling_instance->get_ADC_bits());
        }

        // advance internal time
        buffer_rx.get_ant_streams_next(ant_streams, now_64, spp_size);

        // update local time
        now_64 = buffer_rx.get_rx_time_passed();
    }

    // set stats
    calling_instance->rx_stats.samples_received = now_64 - now_start_64;
    calling_instance->rx_stats.samp_rate_is =
        static_cast<double>(calling_instance->rx_stats.samples_received) /
        (static_cast<double>(watch.get_elapsed<uint64_t, common::milli>()) / 1000.0);

    return nullptr;
}

void hw_simulator_t::clip_and_quantize(const std::vector<cf32_t*>& inp,
                                       std::vector<cf32_t*>& out,
                                       const uint32_t nof_samples,
                                       const float clip_limit,
                                       const uint32_t n_bits) {
    // clipping?
    if (clip_limit > 0.0f) {
        simulation::clip_re_im(inp, out, nof_samples, clip_limit);

        // quantization?
        if (n_bits > 0) {
            simulation::quantize_re_im(
                out,
                out,
                nof_samples,
                2.0f * clip_limit / std::pow(2.0f, static_cast<float>(n_bits)));
        }
    } else {
        // simply copy
        for (uint32_t ant_idx = 0; ant_idx < inp.size(); ++ant_idx) {
            cf32_copy(out[ant_idx], inp[ant_idx], nof_samples);
        }
    }
}

}  // namespace dectnrp::radio
