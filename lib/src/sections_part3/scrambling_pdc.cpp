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

#include "dectnrp/sections_part3/scrambling_pdc.hpp"

#include "dectnrp/common/adt/bitbyte.hpp"
#include "dectnrp/common/prog/assert.hpp"
#include "dectnrp/limits.hpp"

namespace dectnrp::section3 {

scrambling_pdc_t::~scrambling_pdc_t() {
    for (auto& [k, v] : sequences) {
        srsran_sequence_free(&v.first);
        srsran_sequence_free(&v.second);
    }
}

void scrambling_pdc_t::add(const uint32_t network_id) {
    dectnrp_assert(G > 0, "G not set");
    dectnrp_assert(!sequences.contains(network_id), "network ID already known");
    dectnrp_assert(sequences.size() < limits::max_nof_network_id_for_scrambling,
                   "maximum number of network IDs reached");

    // add new sequences
    const uint32_t g_init_type1 = network_id & common::adt::bitmask_lsb<8>();
    const uint32_t g_init_type2 = network_id >> 8;

    srsran_sequence_t seq_type1 = {};
    srsran_sequence_t seq_type2 = {};

    if (srsran_sequence_LTE_pr(&seq_type1, G, g_init_type1)) {
        dectnrp_assert_failure("Error initializing Scrambling Sequence of Type 1 for network ID");
    }
    if (srsran_sequence_LTE_pr(&seq_type2, G, g_init_type2)) {
        dectnrp_assert_failure("Error initializing Scrambling Sequence of Type 2 for network ID");
    }

    sequences[network_id] = std::make_pair(seq_type1, seq_type2);
}

srsran_sequence_t* scrambling_pdc_t::get(const uint32_t network_id, const uint32_t plcf_type) {
    dectnrp_assert(sequences.contains(network_id), "network ID unknown");
    dectnrp_assert(plcf_type == 1 || plcf_type == 2, "PLCF type unknown");

    auto& pair = sequences.at(network_id);
    return plcf_type == 1 ? &pair.first : &pair.second;
}

}  // namespace dectnrp::section3
