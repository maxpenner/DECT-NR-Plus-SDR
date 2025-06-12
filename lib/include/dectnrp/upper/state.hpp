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

namespace dectnrp::upper {

class state_t {
    public:
        typedef std::function<void(void)> leave_callback_t;

        explicit state_t(leave_callback_t leave_callback_)
            : leave_callback(leave_callback_) {}
        virtual ~state_t() = default;

        state_t() = delete;
        state_t(const state_t&) = delete;
        state_t& operator=(const state_t&) = delete;
        state_t(state_t&&) = delete;
        state_t& operator=(state_t&&) = delete;

        /// called by meta firmware when state is entered
        virtual void entry() = 0;

    private:
        /// called to notify meta firmware of state having finished
        leave_callback_t leave_callback;
};

}  // namespace dectnrp::upper
