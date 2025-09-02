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

#include <vector>

#include "dectnrp/common/layer/layer.hpp"
#include "dectnrp/phy/phy.hpp"
#include "dectnrp/radio/radio.hpp"
#include "dectnrp/upper/tpoint.hpp"
#include "dectnrp/upper/upper_config.hpp"

namespace dectnrp::upper {

class upper_t final : public common::layer_t<tpoint_t> {
    public:
        explicit upper_t(const upper_config_t& upper_config_,
                         const radio::radio_t& radio_,
                         const phy::phy_t& phy_);
        ~upper_t() = default;

        upper_t() = delete;
        upper_t(const upper_t&) = delete;
        upper_t& operator=(const upper_t&) = delete;
        upper_t(upper_t&&) = delete;
        upper_t& operator=(upper_t&&) = delete;

        const upper_config_t& upper_config;

    private:
        /// one instance for each tpoint
        std::vector<phy::mac_lower_t> mac_lower_vec;

        void add_tpoint(const tpoint_config_t& tpoint_config, phy::mac_lower_t& mac_lower);

        void init_one_to_many(const radio::radio_t& radio_, const phy::phy_t& phy_);

        void init_one_to_one(const radio::radio_t& radio_, const phy::phy_t& phy_);
};

}  // namespace dectnrp::upper
