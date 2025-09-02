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

#include <stdint.h>

extern "C" {
#include "srsran/config.h"
#include "srsran/phy/modem/modem_table.h"
}

namespace dectnrp::sp3::fix {

int srsran_mod_modulate_bytes_FIX(const srsran_modem_table_t* q,
                                  const uint8_t* bits,
                                  cf_t* symbols,
                                  uint32_t nbits);

}  // namespace dectnrp::sp3::fix
