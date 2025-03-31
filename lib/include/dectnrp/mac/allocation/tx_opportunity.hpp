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

namespace dectnrp::mac::allocation {

class tx_opportunity_t {
    public:
        tx_opportunity_t() = default;

        explicit tx_opportunity_t(const int64_t tx_time_64_, const int64_t N_samples_64_)
            : tx_time_64(tx_time_64_),
              N_samples_64(N_samples_64_) {}

        int64_t get_end() const { return tx_time_64 + N_samples_64; };

        const int64_t tx_time_64{-1};
        const int64_t N_samples_64{-1};
};

}  // namespace dectnrp::mac::allocation
