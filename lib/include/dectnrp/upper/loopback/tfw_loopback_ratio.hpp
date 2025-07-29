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

#include <vector>

#include "dectnrp/upper/loopback/result.hpp"
#include "dectnrp/upper/loopback/tfw_loopback.hpp"

namespace dectnrp::upper::tfw::loopback {

class tfw_loopback_ratio_t final : public tfw_loopback_t {
    public:
        /**
         * \brief Transmits two packets with different power levels (thus ratio) and measures the
         * achievable PER for the second packet.
         *
         * \param tpoint_config_
         * \param mac_lower_
         */
        explicit tfw_loopback_ratio_t(const tpoint_config_t& tpoint_config_,
                                      phy::mac_lower_t& mac_lower_);
        ~tfw_loopback_ratio_t() = default;

        tfw_loopback_ratio_t() = delete;
        tfw_loopback_ratio_t(const tfw_loopback_ratio_t&) = delete;
        tfw_loopback_ratio_t& operator=(const tfw_loopback_ratio_t&) = delete;
        tfw_loopback_ratio_t(tfw_loopback_ratio_t&&) = delete;
        tfw_loopback_ratio_t& operator=(tfw_loopback_ratio_t&&) = delete;

        static const std::string firmware_name;

        phy::maclow_phy_t work_pcc(const phy::phy_maclow_t& phy_maclow) override;
        phy::machigh_phy_t work_pdc(const phy::phy_machigh_t& phy_machigh) override;
        phy::machigh_phy_t work_pdc_error(const phy::phy_machigh_t& phy_machigh) override;

    private:
        /// power ratio range to measure
        std::vector<int32_t> ratio_vec;

        /// identity for first and second packet
        sp4::mac_architecture::identity_t identity_A, identity_B;

        /// measured values
        result_t result;

        void A_reset_result_counter_for_next_snr() override final;

        void C_generate_single_experiment_at_current_snr(
            const int64_t now_64, phy::machigh_phy_t& machigh_phy) override final;

        void D_save_result_of_current_snr() override final;

        bool E_set_next_parameter_or_go_to_dead_end() override final;

        void save_all_results_to_file() const override final;
};

}  // namespace dectnrp::upper::tfw::loopback
