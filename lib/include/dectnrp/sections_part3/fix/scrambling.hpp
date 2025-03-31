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

extern "C" {
#include "srsran/phy/common/sequence.h"
}

namespace dectnrp::section3::fix {

/**
 * \brief By default, srsRAN only supports scrambling starting at bit index 0 and ending at an
 * arbitrary bit index, i.e. not a multiple of 8. This function also supports starting at arbitrary
 * bit offsets. Both len and offset are given as a number of bits, and neither has to be a multiple
 * of 8.
 *
 * \param s
 * \param data
 * \param len
 * \param offset
 */
void srsran_scrambling_bytes_with_sequence_offset_FIX(srsran_sequence_t* s,
                                                      uint8_t* data,
                                                      int len,
                                                      int offset);

}  // namespace dectnrp::section3::fix
