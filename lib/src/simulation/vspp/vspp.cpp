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

#include "dectnrp/simulation/vspp/vspp.hpp"

#include <cstring>

extern "C" {
#include "srsran/phy/utils/vector.h"
}

#include "dectnrp/common/prog/assert.hpp"

namespace dectnrp::simulation {

vspp_t::vspp_t(const uint32_t id_,
               const uint32_t nof_antennas_,
               const uint32_t samp_rate_,
               const uint32_t spp_size_)
    : id(id_),
      nof_antennas(nof_antennas_),
      samp_rate(samp_rate_),
      spp_size(spp_size_) {
    for (uint32_t i = 0; i < nof_antennas; ++i) {
        spp.push_back(srsran_vec_cf_malloc(spp_size));
    }
}

vspp_t::~vspp_t() {
    for (auto& elem : spp) {
        free(elem);
    }
}

void vspp_t::spp_zero(uint32_t offset, uint32_t nof_samples) {
    dectnrp_assert(check_args(spp.size(), offset, nof_samples), "input ill-configured");

    for (uint32_t ant_idx = 0; ant_idx < nof_antennas; ++ant_idx) {
        srsran_vec_cf_zero(&spp[ant_idx][offset], nof_samples);
    }
}

void vspp_t::spp_zero() { spp_zero(0, spp_size); }

bool vspp_t::check_args(const uint32_t inp_dim, uint32_t offset, uint32_t nof_samples) const {
    return inp_dim == nof_antennas && nof_samples != 0 && (offset + nof_samples) <= spp_size;
}

}  // namespace dectnrp::simulation
