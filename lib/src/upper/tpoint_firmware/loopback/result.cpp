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

#include "dectnrp/upper/tpoint_firmware/loopback/result.hpp"

#include <algorithm>
#include <cmath>

#include "dectnrp/common/prog/assert.hpp"

namespace dectnrp::upper::tfw::loopback {

result_t::result_t(const std::size_t N_parameter_values, const std::size_t N_snr_values) {
    for (std::size_t row = 0; row < N_parameter_values; ++row) {
        // add new row
        snr_max_vec.push_back(std::vector<float>());
        snr_min_vec.push_back(std::vector<float>());
        PER_pcc.push_back(std::vector<float>());
        PER_pcc_and_plcf.push_back(std::vector<float>());
        PER_pdc.push_back(std::vector<float>());

        // add columns in the same row
        snr_max_vec.at(row).resize(N_snr_values);
        snr_min_vec.at(row).resize(N_snr_values);
        PER_pcc.at(row).resize(N_snr_values);
        PER_pcc_and_plcf.at(row).resize(N_snr_values);
        PER_pdc.at(row).resize(N_snr_values);
    }

    dectnrp_assert(snr_max_vec.size() == N_parameter_values, "incorrect vector size");
    dectnrp_assert(snr_max_vec.at(0).size() == N_snr_values, "incorrect vector size");
    dectnrp_assert(snr_max_vec.size() == snr_min_vec.size(), "incorrect vector size");
    dectnrp_assert(snr_max_vec.size() == PER_pcc.size(), "incorrect vector size");
    dectnrp_assert(snr_max_vec.size() == PER_pcc_and_plcf.size(), "incorrect vector size");
    dectnrp_assert(snr_max_vec.size() == PER_pdc.size(), "incorrect vector size");

    for (std::size_t row = 0; row < snr_max_vec.size(); ++row) {
        std::fill(snr_max_vec.at(row).begin(), snr_max_vec.at(row).end(), -100.0e6);
        std::fill(snr_min_vec.at(row).begin(), snr_min_vec.at(row).end(), 100.0e6);

        std::fill(PER_pcc.at(row).begin(), PER_pcc.at(row).end(), -1.0f);
        std::fill(PER_pcc_and_plcf.at(row).begin(), PER_pcc_and_plcf.at(row).end(), -1.0f);
        std::fill(PER_pdc.at(row).begin(), PER_pdc.at(row).end(), -1.0f);
    }

    reset();
}

void result_t::overwrite_or_discard_snr_max(const uint32_t row,
                                            const uint32_t col,
                                            const float snr_max) {
    snr_max_vec.at(row).at(col) = std::max(snr_max_vec.at(row).at(col), snr_max);
}

void result_t::overwrite_or_discard_snr_min(const uint32_t row,
                                            const uint32_t col,
                                            const float snr_min) {
    snr_min_vec.at(row).at(col) = std::min(snr_min_vec.at(row).at(col), snr_min);
}

void result_t::set_PERs(const uint32_t row,
                        const uint32_t col,
                        const float nof_experiment_per_snr) {
    const float per_pcc = 1.0f - float(n_pcc) / float(nof_experiment_per_snr);
    const float per_pcc_and_plcf = 1.0f - float(n_pcc_and_plcf) / float(nof_experiment_per_snr);
    const float per_pdc = 1.0f - float(n_pdc) / float(nof_experiment_per_snr);

    PER_pcc.at(row).at(col) = per_pcc;
    PER_pcc_and_plcf.at(row).at(col) = per_pcc_and_plcf;
    PER_pdc.at(row).at(col) = per_pdc;
}

void result_t::reset() {
    n_pcc = 0;
    n_pcc_and_plcf = 0;
    n_pdc = 0;
}

}  // namespace dectnrp::upper::tfw::loopback
