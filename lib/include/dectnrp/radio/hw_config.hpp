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

#include <cstdint>
#include <string>

#include "dectnrp/common/thread/threads.hpp"

namespace dectnrp::radio {

struct hw_config_t {
        /// identifiers used in JSON and log file
        static const std::string json_log_key;
        static const std::string json_log_key_simulation;

        /// every hardware has a unique ID starting at 0
        uint32_t id{};

        /// simulator, USRP
        std::string hw_name{};

        /// TX buffers that can be assigned to PHY threads, typical value is 16
        uint32_t nof_buffer_tx{};

        /// hardware + driver properties, B210 2ms, N- and X-series as low as 100us
        uint32_t turnaround_time_us{};

        /**
         * \brief The radio layer can send zeros before transmitting a packet to mitigate effects of
         * transient responses at burst starts. If set to 0, no zeros are prepended. Given in
         * microseconds. A typical value is 5.
         */
        uint32_t tx_burst_leading_zero_us{};

        /**
         * \brief The radio layer can send packets slightly earlier to compensate delays in the
         * radio hardware. The exact amount of time advance samples depends on the radio hardware
         * and its settings such as sample rate, filter stages etc.
         */
        uint32_t tx_time_advance_samples{};

        /**
         * \brief Immediately after the SDR has started, the radio layer can prestream for some
         * time instead of passing samples directly to PHY. This can help to avoid initial underruns
         * or overflows. Given in milliseconds, typical values are 1000ms to 2000ms.
         */
        uint32_t rx_prestream_ms{};

        /**
         * \brief The RX thread is provided IQ samples by the radio hardware in very small chunks.
         * Instead of notifying the PHY every single time, the minimum notification time can be
         * limited. This avoids waking up threads on PHY for a small number of new IQ samples. Given
         * in microseconds. If set to 0, the PHY threads are notified at the maximum rate possible
         * which minimizes latency at the cost of higher CPU usage.
         */
        uint32_t rx_notification_period_us{};

        /// cpu core and priority for TX/RX threads
        common::threads_core_prio_config_t tx_thread_config{};
        common::threads_core_prio_config_t rx_thread_config{};

        /**
         * \brief Before streaming samples, the radio device's internal clock can be synchronized to
         * a time base. Time is aligned to the start of a full second at the next PPS.
         */
        enum class pps_time_base_t {
            zero,
            TAI,
        } pps_time_base{};

        // ##################################################
        // simulator specifics

        /// how many microseconds after the OS's start of a full second does the internal PPS rise?
        uint32_t full_second_to_pps_us{};

        /// clip TX and RX signals and quantize with bit width of hw_simulator_t
        bool simulator_clip_and_quantize{};

        // ##################################################
        // USRP specifics

        /// USRP arguments must be specific enough to identify exactly one USRP
        std::string usrp_args{};
        common::threads_core_prio_config_t usrp_tx_async_helper_thread_config{};

        // ##################################################
        // simulation specifics

        /// simulation variables are the same for every tpoint
        static bool sim_samp_rate_lte;

        /// number of samples hw_simulator exchanges with the virtual space
        static uint32_t sim_spp_us;

        /// >1 for speedup, <-1 for slowdown
        static int32_t sim_samp_rate_speed;

        /// channel between tpoints
        static std::string sim_channel_name_inter;

        /// TX/RX leakage channel within tpoint
        static std::string sim_channel_name_intra;

        /// relative to 0dBFS or thermal noise
        static std::string sim_noise_type;
};

}  // namespace dectnrp::radio
