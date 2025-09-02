/*
 * Copyright 2023-present Maxim Penner
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

#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "dectnrp/common/randomgen.hpp"
#include "dectnrp/common/thread/watch.hpp"
#include "dectnrp/simulation/hardware/noise.hpp"
#include "dectnrp/simulation/vspp/vspprx.hpp"
#include "dectnrp/simulation/vspp/vspptx.hpp"
#include "dectnrp/simulation/wireless/channel.hpp"

namespace dectnrp::simulation {

class vspace_t {
    public:
        explicit vspace_t(const uint32_t nof_hw_simulator_,
                          const uint32_t samp_rate_speed_,
                          const std::string sim_channel_name_inter_,
                          const std::string sim_channel_name_intra_,
                          const std::string sim_noise_type_);
        ~vspace_t() = default;

        vspace_t() = delete;
        vspace_t(const vspace_t&) = delete;
        vspace_t& operator=(const vspace_t&) = delete;
        vspace_t(vspace_t&&) = delete;
        vspace_t& operator=(vspace_t&&) = delete;

        static constexpr uint32_t TXRX_WAIT_TIMEOUT_MS = 100;

        /**
         * \brief When the radio layer is instantiated, the SDR only checks whether the hardware
         * devices listed in the configuration file are detectable. It does, however, not fully
         * configure the devices, because many hardware properties such as sample rate depend on the
         * radio device class, which is read on the PHY after the radio layer.
         *
         * Thus, the radio devices are configured during runtime by the PHY, and only once they are
         * fully configured, they can register in the virtual space.
         *
         * \param vspptx
         */
        void hw_register_tx(const vspptx_t& vspptx);
        void hw_register_rx(const vspprx_t& vspprx);

        /// all hw_simulator threads wait for the registration to complete with no timeout (nto)
        void wait_for_all_tx_registered_and_inits_done_nto();
        void wait_for_all_rx_registered_and_inits_done_nto();

        /// to = timeout, nto = no timeout
        bool wait_writable_to(const uint32_t id);   // hw_simulator TX threads
        void wait_writable_nto(const uint32_t id);  // hw_simulator TX threads
        bool wait_readable_to(const uint32_t id);   // hw_simulator RX threads

        /// signal exchange to and from virtual space
        void write(vspptx_t& vspptx);  // hw_simulator TX threads
        void read(vspprx_t& vspprx);   // hw_simulator RX threads

        /// randomize all wireless channels
        void wchannel_randomize_small_scale();

        const uint32_t nof_hw_simulator;
        const uint32_t samp_rate_speed;
        const std::string sim_channel_name_inter;
        const std::string sim_channel_name_intra;
        const std::string sim_noise_type;

    private:
        std::mutex mtx;
        std::condition_variable cv_all_tx_registered_and_inits_done;
        std::condition_variable cv_all_rx_registered_and_inits_done;
        std::condition_variable cv_tx;
        std::condition_variable cv_rx;

        /// false while registration is ongoing
        bool all_tx_registered_and_inits_done;
        bool all_rx_registered_and_inits_done;

        /// for every TX and RX thread, we log whether it has written/read current spp
        std::vector<bool> status_tx_written;
        std::vector<bool> status_rx_read;

        /// vspptx_vec contains the latest spp from all TX threads
        std::vector<std::unique_ptr<vspptx_t>> vspptx_vec;

        /// once the last hw has registered, we make sure all devices have the same configuration
        uint32_t samp_rate_common;
        uint32_t spp_size_common;

        common::watch_t watch;  // real time
        int64_t now_start_64;   // simulation start time
        int64_t now_64;         // simulation time

        /// called by last RX thread
        static void realign_realtime_with_simulation_time(const common::watch_t& watch,
                                                          const int32_t samp_rate_speed,
                                                          const int64_t samp_rate_64,
                                                          const int64_t simulation_now_start_64,
                                                          const int64_t simulation_now_64);

        bool is_first_to_register_tx() const;
        bool is_last_to_register_tx() const;
        bool is_first_to_register_rx() const;
        bool is_last_to_register_rx() const;
        bool is_last_to_write() const;
        bool is_last_to_read() const;
        void set_status_tx_written(const bool val);
        void set_status_rx_read(const bool val);

        // ##################################################
        // wireless channel functionality of vspace

        /* Within the virtual space, all simulators send TX signals and receive superpositions of
         * those TX signals. Every simulator also experiences leakage from TX to RX.
         */

        enum class NOISE_TYPE_t {
            relative,
            thermal
        } noise_type;

        common::randomgen_t wchannel_randomgen;
        std::unique_ptr<noise_t> wchannel_noise_source;

        /// inter-simulator wireless channels
        std::vector<std::unique_ptr<channel_t>> wchannel_inter_vec;

        /// intra-simulator TX/RX leakage
        std::vector<std::unique_ptr<channel_t>> wchannel_intra_vec;

        /// once the last hw has registered, we can initialize all wireless channels
        void wchannel_generate_graph();

        /// execute wireless environment
        void wchannel_execute(vspprx_t& vspprx) const;
};

}  // namespace dectnrp::simulation
