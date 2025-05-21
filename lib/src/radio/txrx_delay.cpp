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

#include "dectnrp/radio/txrx_delay.hpp"

#include <algorithm>
#include <cmath>

#include "dectnrp/common/prog/assert.hpp"

namespace dectnrp::radio {

txrx_delay_t::txrx_delay_t(const int64_t clamp_lim_)
    : clamp_lim(clamp_lim_) {
    dectnrp_assert(0 <= clamp_lim, "limit may not be negative");
};

int64_t txrx_delay_t::get_delay() const { return std::clamp(delay, -clamp_lim, clamp_lim); };

void txrx_delay_t::set_delay(const int64_t delay_) {
    dectnrp_assert(std::abs(delay_) <= delay_max, "delay too large");

    delay = delay_;
};

}  // namespace dectnrp::radio
