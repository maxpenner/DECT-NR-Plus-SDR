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

#include <cstdint>

namespace dectnrp::simulation::topology {

uint32_t complete_graph_nof_edges(const uint32_t nof_vertices);

/* We assume to have a complete graph with nof_vertices vertices. Each vertex gets an index from
 * 0,1,2,3,...,nof_vertices-1. There are complete_graph_nof_edges() edges connecting all vertices to
 * each other. The edges also get indices from 0,1,2,3...,complete_graph_nof_edges()-1.
 *
 * When given two vertex indices vidx_0 and vidx_1, which don't have to be sorted, we look up the
 * corresponding edge index. This is done under the assumption that the vertices are sorted as shown
 * in the examples below.
 *
 * Example 1 vertex (no edges):
 * edge_index  A  B
 *
 * Example 2 vertices:
 * edge_index  A  B
 * 0           0  1
 *
 * Example 3 vertices:
 * edge_index  A  B
 * 0           0  1
 * 1           0  2
 * 2           1  2
 *
 * Example 4 vertices:
 * edge_index  A  B
 * 0           0  1
 * 1           0  2
 * 2           0  3
 * 3           1  2
 * 4           1  3
 * 5           2  3
 *
 * Example 5 vertices:
 * edge_index  A  B
 * 0           0  1
 * 1           0  2
 * 2           0  3
 * 3           0  4
 * 4           1  2
 * 5           1  3
 * 6           1  4
 * 7           2  3
 * 8           2  4
 * 9           3  4
 *
 * Example 6 vertices:
 * edge_index  A  B
 *  0          0  1
 *  1          0  2
 *  2          0  3
 *  3          0  4
 *  4          0  5
 *  5          1  2
 *  6          1  3
 *  7          1  4
 *  8          1  5
 *  9          2  3
 * 10          2  4
 * 11          2  5
 * 12          3  4
 * 13          3  5
 * 14          4  5
 */
uint32_t complete_graph_sorted_edge_index(const uint32_t nof_vertices,
                                          const uint32_t vidx_0,
                                          const uint32_t vidx_1);

}  // namespace dectnrp::simulation::topology
