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
        typedef std::function<void(void)> leave_callback_t;

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

        /// called by meta firmware when state is entered
        virtual void entry() = 0;

    protected:
        /// called to notify meta firmware of state having finished
        leave_callback_t leave_callback;
};

}  // namespace dectnrp::upper
