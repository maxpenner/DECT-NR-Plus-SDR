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

using hw_config_t = struct hw_config_t {
        /// identifiers used in JSON and log file
        static const std::string json_log_key;
        static const std::string json_log_key_simulation;

        /// every hardware has a unique ID starting at 0
        uint32_t id;

        /// simulator, USRP
        std::string hw_name;

        /// TX buffers that can be assigned to PHY threads
        uint32_t nof_buffer_tx;

        /// hardware + driver properties
        uint32_t turn_around_time_us;  // B210 2ms, N- and X-series as low as 100us

        /// mitigate effects of transient responses at burst starts
        uint32_t tx_burst_leading_zero_us;

        /// time advance for each packet in samples
        uint32_t tx_time_advance_smpl;

        /// RX thread streams for a time before passing samples to PHY
        uint32_t rx_prestream_ms;

        /// RX thread notifies threads waiting for new IQ samples
        uint32_t rx_notification_period_us;

        /// cpu core and priority for TX/RX threads
        common::threads_core_prio_config_t tx_thread_config;
        common::threads_core_prio_config_t rx_thread_config;

        /// simulator specifics
        /// nothing so far

        /// USRP specifics
        std::string uspr_args;
        common::threads_core_prio_config_t usrp_tx_async_helper_thread_config;

        /// simulation variables are the same for every tpoint
        static bool sim_samp_rate_lte;
        /// >1 for speedup, <-1 for slowdown
        static int32_t sim_samp_rate_speedup;
        /// channel between tpoints
        static std::string sim_channel_name_inter;
        /// TX/RX leakage channel within tpoint
        static std::string sim_channel_name_intra;
        /// relative to 0dBFS or thermal noise
        static std::string sim_noise_type;
};

}  // namespace dectnrp::radio
