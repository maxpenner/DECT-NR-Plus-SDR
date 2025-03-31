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

#include <uhd/usrp/multi_usrp.hpp>

#include "dectnrp/radio/hw.hpp"

namespace dectnrp::radio {

class hw_usrp_t final : public hw_t {
    public:
        explicit hw_usrp_t(const hw_config_t& hw_config_);
        ~hw_usrp_t() = default;

        hw_usrp_t() = delete;
        hw_usrp_t(const hw_usrp_t&) = delete;
        hw_usrp_t& operator=(const hw_usrp_t&) = delete;
        hw_usrp_t(hw_usrp_t&&) = delete;
        hw_usrp_t& operator=(hw_usrp_t&&) = delete;

        static constexpr uint32_t BUFFER_TX_WAIT_NON_CRITICAL_TIMEOUT_MS = 100;

        /// what is the name of this hardware?
        static const std::string name;

        void set_samp_rate(const uint32_t samp_rate_in) override final;
        void initialize_device() override final;
        void initialize_buffer_tx_pool(
            const uint32_t ant_streams_length_samples_max) override final;
        void initialize_buffer_rx(const uint32_t ant_streams_length_samples) override final;

        void set_command_time(const int64_t set_time = -1) override final;
        double set_freq_tc(const double freq_Hz) override final;
        float set_tx_power_ant_0dBFS_tc(const float power_dBm) override final;
        float set_rx_power_ant_0dBFS_tc(const float power_dBm, const size_t idx) override final;

#ifdef RADIO_HW_IMPLEMENTS_GPIO_TOGGLE
        void toggle_gpio_tc() override final;
#endif

        void pps_wait_for_next() const override final;
        void pps_full_sec_at_next(const int64_t full_sec) const override final;

    private:
        std::vector<std::string> start_threads() override final;
        std::vector<std::string> stop_threads() override final;

        // ##################################################
        // hardware properties

        /// set in constructor
        uhd::device_addrs_t device_addrs;
        uint32_t master_clock_rate;
        std::vector<uint32_t> decimation_factors;

        /// set in initialize_device()
        uhd::usrp::multi_usrp::sptr m_usrp;
        uhd::tx_streamer::sptr tx_stream;
        uhd::rx_streamer::sptr rx_stream;

#ifdef RADIO_HW_IMPLEMENTS_GPIO_TOGGLE
        /**
         * \brief We use the first radio device, first radio slot, first bank, first GPIO and output
         * on the first front-side GPIO port.
         *
         * https://files.ettus.com/manual/page_usrp_n3xx.html#n3xx_fpgio
         * https://files.ettus.com/manual/page_x400_gpio_api.html
         * https://events.gnuradio.org/event/18/contributions/234/attachments/74/186/GPIOs%20on%20USRPs.pdf
         */
        struct gpio_state_t {
                std::string bank;
                uint32_t gpio_mask{1};
                bool high{false};
        } gpio_state;
#endif

        // ##################################################
        // threading

        /// two threads, derived classes can introduce more
        pthread_t thread_tx;
        pthread_t thread_rx;

        /// USRP specific helper to get error messages from USRP back to host
        pthread_t thread_tx_async_helper;
        static void* work_tx_async_helper(void* hw_usrp);

        /// function executed in threads
        static void* work_tx(void* hw_usrp);
        static void* work_rx(void* hw_usrp);

        // ##################################################
        // statistics

        struct tx_stats_t {
                int64_t buffer_tx_sent{0};
                int64_t buffer_tx_sent_consecutive{0};
        } tx_stats;
};

}  // namespace dectnrp::radio
