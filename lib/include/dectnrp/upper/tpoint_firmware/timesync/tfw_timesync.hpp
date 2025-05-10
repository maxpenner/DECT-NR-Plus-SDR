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

#include <array>

#include "dectnrp/common/adt/miscellaneous.hpp"
#include "dectnrp/common/thread/watch.hpp"
#include "dectnrp/upper/tpoint.hpp"

// #define TFW_TIMESYNC_EXPORT_1PPS
#ifndef RADIO_HW_IMPLEMENTS_GPIO_TOGGLE
#undef TFW_TIMESYNC_EXPORT_1PPS
#endif

namespace dectnrp::upper::tfw::timesync {

class tfw_timesync_t final : public tpoint_t {
    public:
        /**
         * \brief Firmware begins exporting a PPS signal. This signal then must be used externally
         * to synchronize the host computer, for instance by converting the PPS to PTP with a
         * Raspberry Pi, and feeding the PTP back to the host computer.
         *
         * After some time (measurement_start_delay_s), this firmware begins comparing the SDR
         * sample count with the operating system time. Except for a static offset, the time axes
         * should run synchronously.
         *
         * \param tpoint_config_
         * \param mac_lower_
         */
        tfw_timesync_t(const tpoint_config_t& tpoint_config_, phy::mac_lower_t& mac_lower_);
        ~tfw_timesync_t() = default;

        tfw_timesync_t() = delete;
        tfw_timesync_t(const tfw_timesync_t&) = delete;
        tfw_timesync_t& operator=(const tfw_timesync_t&) = delete;
        tfw_timesync_t(tfw_timesync_t&&) = delete;
        tfw_timesync_t& operator=(tfw_timesync_t&&) = delete;

        static const std::string firmware_name;

        void work_start_imminent(const int64_t start_time_64) override;
        phy::machigh_phy_t work_regular(const phy::phy_mac_reg_t& phy_mac_reg) override;
        phy::maclow_phy_t work_pcc(const phy::phy_maclow_t& phy_maclow) override;
        phy::machigh_phy_t work_pdc_async(const phy::phy_machigh_t& phy_machigh) override;
        phy::machigh_phy_t work_upper(const upper::upper_report_t& upper_report) override;
        phy::machigh_phy_tx_t work_chscan_async(const phy::chscan_t& chscan) override;

    private:
        std::vector<std::string> start_threads() override final;
        std::vector<std::string> stop_threads() override final;

        // ##################################################
        // PPS generation

        // time when we generate another PPS pulse
        int64_t sample_count_at_next_pps_change_64{};

#ifdef TFW_TIMESYNC_EXPORT_1PPS
        /// export 1 PPS, can be used to
        void worksub_pps(const int64_t now_64, const size_t idx);
#endif

        // ##################################################
        // measurement of time offset between operating system and hardware

        /// required to measure operation system time
        common::watch_t watch;

        /// after exporting PPS, we wait some time to operating system synchronize to PPS
        static constexpr uint32_t measurement_start_delay_s{180};

        /// time between two measurements in milliseconds?
        static constexpr uint32_t measurement_interval_ms{10};

        /// how many measurements in total?
        static constexpr uint32_t N_measurements{1000 / measurement_interval_ms * 600};
        uint32_t N_measurements_cnt{0};

        /// start and end time between first and last measurement
        int64_t start_ns_64{common::adt::UNDEFINED_EARLY_64};
        int64_t end_ns_64{common::adt::UNDEFINED_EARLY_64};

        /// measured values as time offset between os and hw in nanoseconds
        std::array<int64_t, N_measurements> measurements_ns_vec;

        /// time of next measurement
        int64_t sample_count_at_next_measurement_64{};

        /// log every few measurements to show progress
        static constexpr uint32_t N_measurements_log{1000};
};

}  // namespace dectnrp::upper::tfw::timesync
