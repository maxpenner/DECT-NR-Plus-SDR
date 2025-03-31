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

#include <memory>
#include <vector>

#include "dectnrp/common/layer/layer.hpp"
#include "dectnrp/phy/interfaces/layers_downwards/phy_radio.hpp"
#include "dectnrp/phy/worker_pool.hpp"
#include "dectnrp/phy/worker_pool_config.hpp"
#include "dectnrp/radio/radio.hpp"

namespace dectnrp::phy {

class phy_t final : public common::layer_t<worker_pool_t> {
    public:
        explicit phy_t(const phy_config_t& phy_config_, const radio::radio_t& radio_);
        ~phy_t() = default;

        phy_t() = delete;
        phy_t(const phy_t&) = delete;
        phy_t& operator=(const phy_t&) = delete;
        phy_t(phy_t&&) = delete;
        phy_t& operator=(phy_t&&) = delete;

        const phy_config_t& phy_config;

    private:
        // structure given to each worker pool for cross controlling transmission
        phy_radio_t phy_radio;
};

}  // namespace dectnrp::phy
