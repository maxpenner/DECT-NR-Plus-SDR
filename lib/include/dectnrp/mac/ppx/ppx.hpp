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
#include <optional>
#include <utility>

#include "dectnrp/common/adt/miscellaneous.hpp"
#include "dectnrp/mac/ppx/pll.hpp"
#include "dectnrp/radio/pulse_config.hpp"
#include "dectnrp/sections_part3/derivative/duration.hpp"
#include "dectnrp/sections_part3/derivative/duration_lut.hpp"

namespace dectnrp::mac::ppx {

class ppx_t {
    public:
        ppx_t() = default;
        ppx_t(const section3::duration_t ppx_period_,
              const section3::duration_t ppx_length_,
              const section3::duration_t ppx_time_advance_,
              const section3::duration_t beacon_period_,
              const section3::duration_t time_deviation_max_);

        bool has_ppx_rising_edge() const { return 0 <= ppx_rising_edge_estimation_64; };

        void set_ppx_rising_edge(const int64_t ppx_rising_edge_64);

        void extrapolate_next_rising_edge(const int64_t now_64);

        void provide_reference_in_beacon_raster(const int64_t time_assumed_in_raster_64);

        void provide_reference_in_custom_raster(const int64_t time_assumed_in_raster_64,
                                                const int64_t custom_raster_64);

        radio::pulse_config_t get_ppx_imminent(const int64_t now_64);

        int64_t get_ppx_period_samples() const { return ppx_period.get_N_samples_64(); };
        int64_t get_ppx_length_samples() const { return ppx_length.get_N_samples_64(); };
        int64_t get_ppx_time_advance_samples() const {
            return ppx_time_advance.get_N_samples_64();
        };

    private:
        section3::duration_t ppx_period;
        section3::duration_t ppx_length;
        section3::duration_t ppx_time_advance;

        /// time updates are given in a specific raster, for instance every 10ms
        section3::duration_t beacon_period;

        /// how much time deviation is acceptable before assuming synchronization is lost?
        section3::duration_t time_deviation_max;

        /// for estimation of ppx_period_warped_64
        pll_t pll;

        /// observed long term ppx period
        int64_t ppx_period_warped_64{common::adt::UNDEFINED_EARLY_64};

        /// current best estimation for when the next ppx is due
        int64_t ppx_rising_edge_estimation_64{common::adt::UNDEFINED_EARLY_64};

        /**
         * \brief Given a reference time and a raster, by how much does the time to test deviate
         * from its closest pointer in the raster?
         *
         * \param ref_64
         * \param raster_64
         * \param now_64
         * \return
         */
        static int64_t determine_offset(const int64_t ref_64,
                                        const int64_t raster_64,
                                        const int64_t time_to_test_64);
};

}  // namespace dectnrp::mac::ppx
