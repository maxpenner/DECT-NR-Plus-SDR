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

#include "dectnrp/simulation/topology/graph.hpp"

#include "dectnrp/common/prog/assert.hpp"

namespace dectnrp::simulation::topology {

uint32_t complete_graph_nof_edges(const uint32_t nof_vertices) {
    return nof_vertices * (nof_vertices - 1) / 2;
}

uint32_t complete_graph_sorted_edge_index(const uint32_t nof_vertices,
                                          const uint32_t vidx_0,
                                          const uint32_t vidx_1) {
    uint32_t A = 0;
    uint32_t B = 0;

    // it must be A < B
    if (vidx_0 < vidx_1) {
        A = vidx_0;
        B = vidx_1;
    } else if (vidx_1 < vidx_0) {
        A = vidx_1;
        B = vidx_0;
    } else {
        dectnrp_assert_failure("Equal vertex indices.");
    }

    /* Edge index = [sum from x=0 to (A-1) of (nof_vertices - 1 - x)] + B - A - 1
     *            = (2 * nof_vertices - A - A * A) / 2                + B - A - 1
     */
    return (2 * nof_vertices * A - A - A * A) / 2 + B - A - 1;
}

}  // namespace dectnrp::simulation::topology
