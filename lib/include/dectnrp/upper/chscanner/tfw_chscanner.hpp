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
#include <vector>

#include "dectnrp/common/ant.hpp"
#include "dectnrp/upper/tpoint.hpp"

namespace dectnrp::upper::tfw::chscanner {

class tfw_chscanner_t final : public tpoint_t {
    public:
        tfw_chscanner_t(const tpoint_config_t& tpoint_config_, phy::mac_lower_t& mac_lower_);
        ~tfw_chscanner_t() = default;

        tfw_chscanner_t() = delete;
        tfw_chscanner_t(const tfw_chscanner_t&) = delete;
        tfw_chscanner_t& operator=(const tfw_chscanner_t&) = delete;
        tfw_chscanner_t(tfw_chscanner_t&&) = delete;
        tfw_chscanner_t& operator=(tfw_chscanner_t&&) = delete;

        static const std::string firmware_name;

        phy::irregular_report_t work_start(const int64_t start_time_64) override final;
        phy::machigh_phy_t work_regular(const phy::regular_report_t& regular_report) override final;
        phy::machigh_phy_t work_irregular(
            const phy::irregular_report_t& irregular_report) override final;
        phy::maclow_phy_t work_pcc(const phy::phy_maclow_t& phy_maclow) override final;
        phy::machigh_phy_t work_pdc(const phy::phy_machigh_t& phy_machigh) override final;
        phy::machigh_phy_t work_pdc_error(const phy::phy_machigh_t& phy_machigh) override final;
        phy::machigh_phy_t work_application(
            const application::application_report_t& application_report) override final;
        phy::machigh_phy_tx_t work_chscan(const phy::chscan_t& chscan) override final;
        void work_stop() override final;

    private:
        /// timing between states
        int64_t next_measurement_time_64;

        /// bands to measure
        std::array<uint32_t, 3> bands{1, 2, 3};

        /// frequencies to measure
        std::vector<float> freqs{};

        /// index of next frequency to measure
        uint32_t freqs_idx{};

        /**
         * \brief Time a single frequency is measured. Only a approximation, actual duration will be
         * a multiple of measurement_period_ms.
         */
        const uint32_t measurement_duration_per_frequency_ms{500};
        /// time between two measurements
        const uint32_t measurement_period_ms{29};
        /// additional between before the next measurement immediately after switching frequency
        const uint32_t measurement_separation_between_frequencies_ms{100};

        /// number of measurements per frequency
        uint32_t N_measurement{0};
        uint32_t N_measurement_cnt{0};

        /// measured values during a single run
        float rms_min{1.0e9};
        float rms_max{-1.0e9};

        /// required to convert RMS to absolute power
        common::ant_t rx_power_ant_0dBFS{};
};

}  // namespace dectnrp::upper::tfw::chscanner
