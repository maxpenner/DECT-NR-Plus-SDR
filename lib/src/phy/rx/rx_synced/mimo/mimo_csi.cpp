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

#include "dectnrp/common/prog/assert.hpp"

namespace dectnrp::phy {

void mimo_csi_t::update_from_feedback(const uint32_t feedback_format,
                                      const sp4::feedback_info_pool_t& feedback_info_pool,
                                      const sync_report_t& sync_report) {
    // extract at which time this update was received
    const int64_t time_64 = sync_report.fine_peak_time_64;

    switch (feedback_format) {
        case sp4::feedback_info_t::No_feedback:
            break;
        case 1:
            break;
        case 2:
            break;
        case 3:
            break;
        case 4:
            feedback_MCS =
                common::adt::expiring_t(feedback_info_pool.feedback_info_f4.MCS, time_64);

            // update feedback_tm_mode
            // ToDo

            break;
        case 5:
            feedback_codebook_index = common::adt::expiring_t(
                feedback_info_pool.feedback_info_f5.Codebook_index, time_64);

            // update feedback_tm_mode
            // ToDo

            // use the codebook index suggested by the feedback in the PLCF
            codebook_index = feedback_codebook_index;

            break;
        case 6:
            break;
        default:
            dectnrp_assert_failure("undefined feedback format");
            break;
    }
}

void mimo_csi_t::update_from_phy(const mimo_report_t& mimo_report,
                                 const sync_report_t& sync_report) {
    phy_codebook_index =
        common::adt::expiring_t(mimo_report.tm_3_7_beamforming_idx, sync_report.fine_peak_time_64);

    phy_codebook_index_reciprocal = common::adt::expiring_t(
        mimo_report.tm_3_7_beamforming_reciprocal_idx, sync_report.fine_peak_time_64);

    // update phy_tm_mode_reciprocal
    // ToDo

    // use the codebook index suggested by PHY
    codebook_index = phy_codebook_index_reciprocal;
}

void mimo_csi_t::update_from_phy(const uint32_t MCS, const sync_report_t& sync_report) {
    phy_MCS = common::adt::expiring_t(MCS, sync_report.fine_peak_time_64);
}

}  // namespace dectnrp::phy
