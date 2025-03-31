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

#include "dectnrp/common/prog/simd.hpp"

#include <volk/volk.h>

extern "C" {
#include "srsran/phy/utils/vector.h"
}

#include "dectnrp/common/prog/assert.hpp"
#include "dectnrp/common/prog/log.hpp"

namespace dectnrp::common::simd {

void assert_simd_libs_use_same_alignment() {
    cf_t* ptr = srsran_vec_cf_malloc(1);
    dectnrp_assert(reinterpret_cast<uintptr_t>(ptr) % volk_get_alignment() == 0,
                   "SIMD alignment not the same");
    free(ptr);

    dectnrp_log_inf("SIMD alignment {} bytes", volk_get_alignment());
}

void print_info_for_this_kernel() {
    volk_list_machines();

    volk_func_desc_t info = PROG_SIMD_PRINT_INFO_FOR_THIS_KERNEL;
    for (uint32_t i = 0; i < info.n_impls; ++i) {
        dectnrp_log_inf("Volk: i={} {}", i, info.impl_names[i]);
    }
}

}  // namespace dectnrp::common::simd
