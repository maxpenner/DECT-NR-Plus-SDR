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

#include "dectnrp/radio/buffer_tx_pool.hpp"

namespace dectnrp::phy {

class radio_ctrl_t {
    public:
        /**
         * \brief Interface for worker_pool_t on PHY to control a hw on the radio layer.
         *
         * Generally, each instance of worker_pool_t is associated with only one hw, but for
         * some uses cases it is helpful if every worker_pool_t can control any hw. However,
         * this control is limited. This class only grants access to components of a hw on
         * the radio layer that are actually required.
         *
         * \param buffer_tx_pool_ request buffer_tx for transmission
         */
        radio_ctrl_t(radio::buffer_tx_pool_t& buffer_tx_pool_)
            : buffer_tx_pool(buffer_tx_pool_) {}
        radio::buffer_tx_pool_t& buffer_tx_pool;
};

}  // namespace dectnrp::phy
