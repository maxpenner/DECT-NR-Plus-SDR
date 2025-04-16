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

#pragma once

#include <mutex>

#include "dectnrp/radio/complex.hpp"
#include "dectnrp/radio/hw.hpp"
#include "dectnrp/simulation/vspace.hpp"
#include "dectnrp/simulation/vspp/vspprx.hpp"
#include "dectnrp/simulation/vspp/vspptx.hpp"

namespace dectnrp::radio {

class hw_simulator_t final : public hw_t {
    public:
        /**
         * \brief The UHD exchanges complex samples with an USRP in packets of a predefined maximum
         * size, defined either by stream argument "spp: (samples per packet)", or by
         * recv_frame_size and send_frame_size as part or the device args. For the simulation, this
         * behavior is reproduced.
         *
         * https://files.ettus.com/manual/structuhd_1_1stream__args__t.html#a4463f2eec2cc7ee70f84baacbb26e1ef
         * https://files.ettus.com/manual/page_transport.html
         *
         * \param id_ unique ID in SDR
         * \param hw_config_ configuration taken from user configuration file
         * \param vspace_ reference to virtual space, where hw_simulator_t has to register
         */
        explicit hw_simulator_t(const hw_config_t& hw_config_, simulation::vspace_t& vspace_);
        ~hw_simulator_t() = default;

        hw_simulator_t() = delete;
        hw_simulator_t(const hw_simulator_t&) = delete;
        hw_simulator_t& operator=(const hw_simulator_t&) = delete;
        hw_simulator_t(hw_simulator_t&&) = delete;
        hw_simulator_t& operator=(hw_simulator_t&&) = delete;

        /// what is the name of this hardware?
        static const std::string name;

        /// called from the main thread before starting any threads, not thread-safe
        void set_samp_rate(const uint32_t samp_rate_in) override final;
        void initialize_device() override final;
        void initialize_buffer_tx_pool(
            const uint32_t ant_streams_length_samples_max) override final;
        void initialize_buffer_rx(const uint32_t ant_streams_length_samples) override final;

        /// called from tpoint firmware, thread-safe
        void set_command_time(const int64_t set_time = -1) override final;
        double set_freq_tc(const double freq_Hz) override final;
        float set_tx_power_ant_0dBFS_tc(const float power_dBm) override final;
        float set_rx_power_ant_0dBFS_tc(const float power_dBm, const size_t idx) override final;

#ifdef RADIO_HW_IMPLEMENTS_GPIO_TOGGLE
        void toggle_gpio_tc() override final;
#endif

        void pps_wait_for_next() const override final;
        void pps_full_sec_at_next(const int64_t full_sec) const override final;

        /// called from tpoint firmware, thread-safe
        void set_trajectory(const simulation::topology::trajectory_t trajectory);
        void set_net_bandwidth_norm(const float net_bandwidth_norm);
        void set_tx_into_rx_leakage_dB(const float tx_into_rx_leakage_dB);
        void set_rx_noise_figure_dB(const float rx_noise_figure_dB);
        void set_rx_snr_in_net_bandwidth_norm_dB(const float rx_snr_in_net_bandwidth_norm_dB);

        /// called from tpoint firmware to randomize all channel in virtual space
        void wchannel_randomize_small_scale() { vspace.wchannel_randomize_small_scale(); }

        /// used when hw_simulator_t is only required for access to a buffer_tx_pool without TX/RX
        /// threads, not thread-safe
        void set_all_buffers_as_transmitted();

    private:
        std::vector<std::string> start_threads() override final;
        std::vector<std::string> stop_threads() override final;

        /// ##################################################
        /// hardware properties and simulation properties

        /// virtual space hardware is part of
        simulation::vspace_t& vspace;

        /**
         * \brief This class has a bunch of hardware properties, some of which are inherited and
         * some of which are added here. These properties can be read and written from within a
         * tpoint firmware by whatever worker_tx_rx thread holding the lock at that point in time.
         * These properties are also read by the TX and RX thread of this class when sending to or
         * receiving from the virtual space. The access is made exclusive by this mutex.
         */
        std::mutex hw_mtx;

        /// spp = samples_per_packet, adopted from UHD
        std::unique_ptr<simulation::vspptx_t> vspptx;
        std::unique_ptr<simulation::vspprx_t> vspprx;

        /// simulated hardware effects
        static void clip_and_quantize(const std::vector<cf32_t*>& inp,
                                      std::vector<cf32_t*>& out,
                                      const uint32_t nof_samples,
                                      const float clip_limit,
                                      const uint32_t n_bits);

        /// ##################################################
        /// threading

        /// two threads as for USRP
        pthread_t thread_tx;
        pthread_t thread_rx;

        /// function executed in threads
        static void* work_tx(void* hw_simulator);
        static void* work_rx(void* hw_simulator);

        /// ##################################################
        /// statistics

        /// is reset with every call of work_tx()
        struct tx_stats_t {
                uint64_t samples_sent{};
                double samp_rate_is{};
                int64_t buffer_tx_sent{};
        } tx_stats;

        /// is reset with every call of work_rx()
        struct rx_stats_t {
                uint64_t samples_received{};
                double samp_rate_is{};
        } rx_stats;
};

}  // namespace dectnrp::radio
