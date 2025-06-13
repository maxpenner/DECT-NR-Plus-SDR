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

#include "dectnrp/upper/p2p/data/rd.hpp"
#include "dectnrp/upper/tpoint.hpp"

namespace dectnrp::upper::tfw::p2p {

class tfw_p2p_rd_t {
    public:
        tfw_p2p_rd_t() = default;
        virtual ~tfw_p2p_rd_t() = default;

        tfw_p2p_rd_t(const tfw_p2p_rd_t&) = delete;
        tfw_p2p_rd_t& operator=(const tfw_p2p_rd_t&) = delete;
        tfw_p2p_rd_t(tfw_p2p_rd_t&&) = delete;
        tfw_p2p_rd_t& operator=(tfw_p2p_rd_t&&) = delete;

    protected:
        virtual void init_radio() = 0;
        virtual void init_simulation_if_detected() = 0;
        virtual void init_appiface() = 0;

        /// called by active state to request a state change
        virtual void state_transitions() = 0;

        /// data of radio device shared with all states
        rd_t rd;

        /// pointer to currently active state
        tpoint_t* state{nullptr};
};

}  // namespace dectnrp::upper::tfw::p2p
