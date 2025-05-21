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

#include "dectnrp/phy/rx/chscan/chscanner.hpp"

#include <volk/volk.h>

#include <algorithm>
#include <cmath>

#include "dectnrp/common/prog/assert.hpp"

namespace dectnrp::phy {

chscanner_t::chscanner_t(const radio::buffer_rx_t& buffer_rx_)
    : buffer_rx(buffer_rx_),
      ant_streams(buffer_rx.get_ant_streams()),
      ant_streams_length_samples(buffer_rx.ant_streams_length_samples),
      duration_lut(sp3::duration_lut_t(buffer_rx.samp_rate)) {}

void chscanner_t::scan(chscan_t& ch_scan) {
    dectnrp_assert(ch_scan.end_64 <= buffer_rx.get_rx_time_passed(), "end time not reached yet");
    dectnrp_assert(0 < ch_scan.N_partial, "must be at least one scan");
    dectnrp_assert(0 < ch_scan.N_ant, "must be at least one antenna");

    // if not set explicitly, the value of ch_scan.N_ant is 2^32-1
    N_ant = std::min(ch_scan.N_ant, static_cast<uint32_t>(ant_streams.size()));

    // how many samples per partial scan?
    const int64_t length_64 = duration_lut.get_N_samples_from_duration(ch_scan.duration_ec);

    dectnrp_assert(0 < length_64, "cannot measure zero length");

    // determine start time of first partial scan
    int64_t start_64 = ch_scan.end_64 - (static_cast<int64_t>(ch_scan.N_partial) * length_64 - 1L);

    dectnrp_assert(start_64 <= ch_scan.end_64, "start time larger end time");
    dectnrp_assert((ch_scan.end_64 - start_64) < static_cast<int64_t>(ant_streams_length_samples),
                   "start time and end time too far apart");
    dectnrp_assert((buffer_rx.get_rx_time_passed() -
                    static_cast<int64_t>(ant_streams_length_samples)) <= start_64,
                   "start time too far in past");

    // init RMS container
    ch_scan.rms_vec.resize(ch_scan.N_partial);
    ch_scan.rms_avg = 0.0f;

    for (uint32_t n = 0; n < ch_scan.N_partial; ++n) {
        // run partial scan
        ch_scan.rms_vec.at(n) = scan_partial(start_64, length_64);

        // add to average RMS
        ch_scan.rms_avg += ch_scan.rms_vec.at(n);

        // prepare next scan
        start_64 += length_64;
    }
}

float chscanner_t::scan_partial(const int64_t start_64, const int64_t length_64) {
    // index of final sample to scan at
    const int64_t end_local_64 = start_64 + length_64 - 1;

    // convert global times to local indices relative to ant_streams
    const uint32_t A =
        static_cast<uint32_t>(start_64 % static_cast<int64_t>(ant_streams_length_samples));
    const uint32_t B =
        static_cast<uint32_t>(end_local_64 % static_cast<int64_t>(ant_streams_length_samples));

    /* Depending on A and B, we might have to wrap and the end of ant_streams.
     *
     *
     *      0 1 2 3 4 5 6 7 8 9
     *              A     B
     *      _|_|_|_|_|_|_|_|_|_|
     *              x x x x
     *
     *              A          |  B unwrapped
     *        B wrapped        |
     *      _|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|
     *      x x     x x x x x x
     */
    const bool wrap_not_required = A < B;

    // sum of energy across all antennas
    float energy_across_all_antennas{0.0f};

    // temporary volk target
    lv_32fc_t result;

    if (wrap_not_required) {
        // go over each antenna
        for (size_t ant_idx = 0; ant_idx < N_ant; ++ant_idx) {
            volk_32fc_x2_conjugate_dot_prod_32fc(&result,
                                                 (const lv_32fc_t*)&ant_streams[ant_idx][A],
                                                 (const lv_32fc_t*)&ant_streams[ant_idx][A],
                                                 B - A + 1u);

            energy_across_all_antennas += result.real();
        }
    } else {
        // go over each antenna
        for (size_t ant_idx = 0; ant_idx < N_ant; ++ant_idx) {
            volk_32fc_x2_conjugate_dot_prod_32fc(&result,
                                                 (const lv_32fc_t*)&ant_streams[ant_idx][A],
                                                 (const lv_32fc_t*)&ant_streams[ant_idx][A],
                                                 ant_streams_length_samples - A);

            energy_across_all_antennas += result.real();

            volk_32fc_x2_conjugate_dot_prod_32fc(&result,
                                                 (const lv_32fc_t*)&ant_streams[ant_idx][0],
                                                 (const lv_32fc_t*)&ant_streams[ant_idx][0],
                                                 B + 1u);

            energy_across_all_antennas += result.real();
        }
    }

    // normalization factor to convert energy to rms
    const float norm = static_cast<float>(length_64) * static_cast<float>(N_ant);

    return std::sqrt(energy_across_all_antennas / norm);
}

}  // namespace dectnrp::phy
