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

#include <functional>

#include "dectnrp/upper/tpoint.hpp"

namespace dectnrp::upper {

class tpoint_state_t : public tpoint_t {
    public:
        typedef std::function<phy::irregular_report_t(void)> leave_callback_t;

        explicit tpoint_state_t(const tpoint_config_t& tpoint_config_,
                                phy::mac_lower_t& mac_lower_,
                                leave_callback_t leave_callback_)
            : tpoint_t(tpoint_config_, mac_lower_),
              leave_callback(leave_callback_) {}
        virtual ~tpoint_state_t() = default;

        tpoint_state_t() = delete;
        tpoint_state_t(const tpoint_state_t&) = delete;
        tpoint_state_t& operator=(const tpoint_state_t&) = delete;
        tpoint_state_t(tpoint_state_t&&) = delete;
        tpoint_state_t& operator=(tpoint_state_t&&) = delete;

        [[nodiscard]] virtual phy::irregular_report_t work_start(
            const int64_t start_time_64) override = 0;
        [[nodiscard]] virtual phy::machigh_phy_t work_regular(
            const phy::regular_report_t& regular_report) override = 0;
        [[nodiscard]] virtual phy::machigh_phy_t work_irregular(
            const phy::irregular_report_t& irregular_report) override = 0;
        [[nodiscard]] virtual phy::maclow_phy_t work_pcc(
            const phy::phy_maclow_t& phy_maclow) override = 0;
        [[nodiscard]] virtual phy::machigh_phy_t work_pdc_async(
            const phy::phy_machigh_t& phy_machigh) override = 0;
        [[nodiscard]] virtual phy::machigh_phy_t work_application(
            const application::application_report_t& application_report) override = 0;
        [[nodiscard]] virtual phy::machigh_phy_tx_t work_chscan_async(
            const phy::chscan_t& chscan) override = 0;
        virtual void work_stop() override = 0;

        /// called by meta firmware when state is entered
        [[nodiscard]] virtual phy::irregular_report_t entry() = 0;

    protected:
        /// called to notify meta firmware of state having finished
        leave_callback_t leave_callback;
};

}  // namespace dectnrp::upper
