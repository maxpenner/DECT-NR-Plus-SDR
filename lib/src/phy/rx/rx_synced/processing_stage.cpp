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

#include "dectnrp/phy/rx/rx_synced/processing_stage.hpp"

extern "C" {
#include "srsran/phy/utils/vector.h"
}

namespace dectnrp::phy {

template <>
processing_stage_t<cf_t>::processing_stage_t(const uint32_t N_f_domain_max_,
                                             const uint32_t N_t_domain_max_,
                                             const uint32_t N_stage_)
    : N_f_domain_max(N_f_domain_max_),
      N_t_domain_max(N_t_domain_max_),
      N_stage(N_stage_) {
    for (uint32_t i = 0; i < N_stage; ++i) {
        stage.push_back(srsran_vec_cf_malloc(N_f_domain_max * N_t_domain_max));
    }
}

template <>
processing_stage_t<cf_t>::~processing_stage_t() {
    for (auto& elem : stage) {
        free(elem);
    }
}

}  // namespace dectnrp::phy
