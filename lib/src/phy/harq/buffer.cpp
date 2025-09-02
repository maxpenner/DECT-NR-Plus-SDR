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

#include "dectnrp/phy/harq/buffer.hpp"

#include <cstring>

extern "C" {
#include "srsran/phy/utils/vector.h"
}

namespace dectnrp::phy::harq {

buffer_t::buffer_t(const uint32_t a_len_, const uint32_t d_len_)
    : a_len(a_len_),
      d_len(d_len_),
      a(srsran_vec_u8_malloc(a_len)),
      d(srsran_vec_u8_malloc(d_len)) {};

buffer_t::~buffer_t() {
    free(a);
    free(d);
}

}  // namespace dectnrp::phy::harq
