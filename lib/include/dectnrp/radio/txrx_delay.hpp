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

#include <cstdint>

namespace dectnrp::radio {

struct txrx_delay_t {
        txrx_delay_t() = default;
        txrx_delay_t(const int64_t clamp_lim_);

        int64_t get_delay() const;

        void set_delay(const int64_t delay_);

    private:
        static constexpr int64_t delay_max{150};
        static constexpr int64_t clamp_lim_default{100};

        int64_t delay{};
        int64_t clamp_lim{clamp_lim_default};
};

}  // namespace dectnrp::radio
