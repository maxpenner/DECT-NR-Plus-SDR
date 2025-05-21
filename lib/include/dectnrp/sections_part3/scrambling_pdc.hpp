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

#include <cstdint>
#include <unordered_map>

extern "C" {
#include "srsran/phy/common/sequence.h"
}

namespace dectnrp::sp3 {

class scrambling_pdc_t {
    public:
        scrambling_pdc_t() = default;
        ~scrambling_pdc_t();

        scrambling_pdc_t(const scrambling_pdc_t&) = delete;
        scrambling_pdc_t& operator=(const scrambling_pdc_t&) = delete;
        scrambling_pdc_t(scrambling_pdc_t&&) = delete;
        scrambling_pdc_t& operator=(scrambling_pdc_t&&) = delete;

        /// preallocation length G used for all scrambling sequences
        void set_maximum_sequence_length_G(const uint32_t G_) { G = G_; }

        /// add new network ID and its respective scrambling sequences
        void add(const uint32_t network_id);

        /// https://github.com/srsran/srsRAN/blob/master/lib/include/srsran/phy/common/sequence.h
        srsran_sequence_t* get(const uint32_t network_id, const uint32_t plcf_type);

    private:
        /// see ETSI part 3
        uint32_t G{0};

        std::unordered_map<uint32_t, std::pair<srsran_sequence_t, srsran_sequence_t>> sequences;
};

}  // namespace dectnrp::sp3
