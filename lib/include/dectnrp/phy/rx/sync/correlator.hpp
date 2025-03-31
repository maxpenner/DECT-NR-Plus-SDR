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
#include <vector>

extern "C" {
#include "srsran/config.h"
}

#include "dectnrp/phy/rx/sync/sync_report.hpp"

namespace dectnrp::phy {

class correlator_t {
    public:
        explicit correlator_t(const std::vector<cf_t*> localbuffer_)
            : localbuffer(localbuffer_),
              localbuffer_cnt_r(0) {};
        virtual ~correlator_t() = default;

        correlator_t() = delete;
        correlator_t(const correlator_t&) = delete;
        correlator_t& operator=(const correlator_t&) = delete;
        correlator_t(correlator_t&&) = delete;
        correlator_t& operator=(correlator_t&&) = delete;

        /// number of samples required before next call of search_by_correlation()
        virtual uint32_t get_nof_samples_required() const = 0;

        /// internal processing as far as localbuffer_cnt_w allows
        virtual bool search_by_correlation(const uint32_t localbuffer_cnt_w,
                                           sync_report_t& sync_report) = 0;

        uint32_t get_localbuffer_cnt_r() const { return localbuffer_cnt_r; };

    protected:
        /// resampler output read-only
        const std::vector<cf_t*> localbuffer;

        /// refers to an index in localbuffer
        uint32_t localbuffer_cnt_r;
};

}  // namespace dectnrp::phy
