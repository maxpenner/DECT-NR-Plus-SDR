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

#include "dectnrp/phy/rx/rx_synced/mimo/mimo_csi.hpp"

namespace dectnrp::phy {

void mimo_csi_t::update(const uint32_t feedback_format,
                        const section4::feedback_info_pool_t& feedback_info_pool,
                        const sync_report_t& sync_report) {
    // extract at which time this update was received
    const int64_t time_64 = sync_report.fine_peak_time_64;

    switch (feedback_format) {
        case section4::feedback_info_t::No_feedback:
        case 1:
            break;

        case 2:
            break;

        case 3:
            break;

        case 4:
            update_mcs(feedback_info_pool.feedback_info_f4.MCS, time_64);
            break;

        case 5:
            update_ci(feedback_info_pool.feedback_info_f5.Codebook_index, time_64);
            break;

        case 6:
            break;

        default:
            dectnrp_assert_failure("undefined feedback format");
            break;
    }
}

void mimo_csi_t::update(const mimo_report_t& mimo_report, const sync_report_t& sync_report) {
    codebook_index = common::adt::expiring_t(mimo_report.tm_3_7_beamforming_reciprocal_idx,
                                             sync_report.fine_peak_time_64);
}

void mimo_csi_t::update_mcs(const uint32_t MCS_, const int64_t time_64) {
    MCS = common::adt::expiring_t(MCS_, time_64);
}

void mimo_csi_t::update_ci(const uint32_t codebook_index_, const int64_t time_64) {
    codebook_index = common::adt::expiring_t(codebook_index_, time_64);
}

}  // namespace dectnrp::phy
