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

#include "dectnrp/radio/complex.hpp"

#include <limits>

extern "C" {
#include "srsran/phy/utils/vector.h"
}

#include <dectnrp/common/prog/assert.hpp>

namespace dectnrp::radio {

cf32_t* cf32_malloc(const std::size_t size) {
    dectnrp_assert(size <= std::numeric_limits<uint32_t>::max(), "malloc limited");

    return srsran_vec_cf_malloc(size);
}

void cf32_zero(cf32_t* ptr, const std::size_t size) { srsran_vec_cf_zero(ptr, size); }

void cf32_copy(cf32_t* dst, const cf32_t* src, const std::size_t size) {
    srsran_vec_cf_copy(dst, src, size);
}

}  // namespace dectnrp::radio
