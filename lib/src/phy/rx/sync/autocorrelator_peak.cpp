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

#include "dectnrp/phy/rx/sync/autocorrelator_peak.hpp"

#include <volk/volk.h>

#include <cmath>
#include <complex>

extern "C" {
#include "srsran/phy/utils/vector.h"
}

#include "dectnrp/common/prog/assert.hpp"
#include "dectnrp/phy/rx/sync/sync_param.hpp"
#include "dectnrp/sections_part3/stf.hpp"

#ifdef PHY_RX_AUTOCORRELATOR_PEAK_JSON_EXPORT
#include <atomic>

#include "dectnrp/common/json/json_export.hpp"

// must be static in case multiple threads can save files
static std::atomic<uint32_t> json_file_cnt{0};
#endif

namespace dectnrp::phy {

autocorrelator_peak_t::autocorrelator_peak_t(const std::vector<cf_t*> localbuffer_,
                                             const uint32_t u_max_,
                                             const uint32_t b_max_,
                                             const uint32_t nof_antennas_limited_,
                                             const uint32_t bos_fac_,
                                             const uint32_t stf_bos_length_samples_,
                                             const uint32_t stf_bos_pattern_length_samples_,
                                             const uint32_t search_length_samples_)
    : correlator_t(localbuffer_),

      nof_antennas_limited(nof_antennas_limited_),
      stf_bos_length_samples(stf_bos_length_samples_),
      stf_bos_pattern_length_samples(stf_bos_pattern_length_samples_),
      stf_nof_pattern(stf_bos_length_samples / stf_bos_pattern_length_samples),

      search_length_samples(search_length_samples_),
      detection2peak_samples_64(
          static_cast<int64_t>(RX_SYNC_PARAM_AUTOCORRELATOR_PEAK_DETECTION2PEAK_IN_STFS_DP *
                               static_cast<double>(stf_bos_length_samples))),
      prefactor(static_cast<float>(stf_nof_pattern) / static_cast<float>(stf_nof_pattern - 1)),

      metric_smoother_bos_offset_to_center_samples(
          RX_SYNC_PARAM_AUTOCORRELATOR_PEAK_MOVMEAN_SMOOTH_RIGHT * bos_fac_)

{
    multiplication_stage_correlation = srsran_vec_cf_malloc(search_length_samples);

    // we make it slightly longer for faster power calculation
    multiplication_stage_power =
        srsran_vec_cf_malloc(std::max(stf_bos_length_samples, search_length_samples));

    movsums_correlation.resize(nof_antennas_limited);
    movsums_power.resize(nof_antennas_limited);

    metric_smoother.resize(nof_antennas_limited);

    for (uint32_t ant_idx = 0; ant_idx < nof_antennas_limited; ++ant_idx) {
        movsums_correlation[ant_idx] =
            movsum_uw_t(sp3::stf_t::get_cover_sequence_pairwise_product(
                            sp3::stf_t::get_equivalent_u(stf_nof_pattern)),
                        stf_bos_pattern_length_samples);

        movsums_power[ant_idx] = movsum_t<float>(stf_bos_length_samples);

        metric_smoother[ant_idx] =
            movsum_t<float>(RX_SYNC_PARAM_AUTOCORRELATOR_PEAK_MOVMEAN_SMOOTH_LEFT * bos_fac_ + 1 +
                            metric_smoother_bos_offset_to_center_samples);
    }

    peak_vec.resize(nof_antennas_limited);

    coarse_peak_f_domain = std::make_unique<coarse_peak_f_domain_t>(
        localbuffer, u_max_, b_max_, nof_antennas_limited, bos_fac_);
}

autocorrelator_peak_t::~autocorrelator_peak_t() {
    free(multiplication_stage_correlation);
    free(multiplication_stage_power);
}

void autocorrelator_peak_t::set_initial_state(const uint32_t detection_time_with_jump_back_local_) {
    dectnrp_assert(stf_bos_length_samples <= detection_time_with_jump_back_local_,
                   "detection with jump back too early");

    localbuffer_cnt_r = detection_time_with_jump_back_local_;
    localbuffer_cnt_r_max = detection_time_with_jump_back_local_ + search_length_samples;

    set_initial_movsums(localbuffer_cnt_r - stf_bos_length_samples);

#ifdef RX_SYNC_PARAM_AUTOCORRELATOR_PEAK_RESUM_PERIODICITY_IN_STEPS
    resum_cnt = 0;
#endif

    for (uint32_t ant_idx = 0; ant_idx < nof_antennas_limited; ++ant_idx) {
        peak_vec[ant_idx].reset();
    }

#ifdef PHY_RX_AUTOCORRELATOR_PEAK_JSON_EXPORT
    waveform_power.clear();
    waveform_power.resize(nof_antennas_limited);
    waveform_rms.clear();
    waveform_rms.resize(nof_antennas_limited);
    waveform_metric.clear();
    waveform_metric.resize(nof_antennas_limited);
    waveform_metric_smooth.clear();
    waveform_metric_smooth.resize(nof_antennas_limited);
    waveform_metric_max_idx.clear();
    waveform_metric_max_idx.resize(nof_antennas_limited);
#endif
}

uint32_t autocorrelator_peak_t::get_nof_samples_required() const {
    return std::min(
        localbuffer_cnt_r + stf_bos_pattern_length_samples *
                                RX_SYNC_PARAM_AUTOCORRELATOR_PEAK_SAMPLES_REQUEST_IN_PATTERNS,
        localbuffer_cnt_r_max);
}

bool autocorrelator_peak_t::search_by_correlation(const uint32_t localbuffer_cnt_w,
                                                  sync_report_t& sync_report) {
    const uint32_t localbuffer_cnt_r_limit = std::min(localbuffer_cnt_r_max, localbuffer_cnt_w);

    const uint32_t consumable = localbuffer_cnt_r_limit - localbuffer_cnt_r;

    dectnrp_assert(consumable <= search_length_samples, "too many samples");

    for (uint32_t ant_idx = 0; ant_idx < nof_antennas_limited; ++ant_idx) {
        // readability pointers
        const cf_t* A = &localbuffer[ant_idx][localbuffer_cnt_r - stf_bos_pattern_length_samples];
        const cf_t* B = &localbuffer[ant_idx][localbuffer_cnt_r];

        volk_32fc_x2_multiply_conjugate_32fc((lv_32fc_t*)multiplication_stage_correlation,
                                             (const lv_32fc_t*)A,
                                             (const lv_32fc_t*)B,
                                             consumable);

        volk_32fc_x2_multiply_conjugate_32fc((lv_32fc_t*)multiplication_stage_power,
                                             (const lv_32fc_t*)B,
                                             (const lv_32fc_t*)B,
                                             consumable);

        uint32_t consumed = 0;
        while (consumed < consumable) {
            movsums_correlation[ant_idx].pop_push(multiplication_stage_correlation[consumed]);
            movsums_power[ant_idx].pop_push(__real__(multiplication_stage_power[consumed]));

            // "A Robust Timing and Frequency Synchronization for OFDM Systems", Hlaing Minn,
            // Equation (14)
            const float power = movsums_power[ant_idx].get_sum();
            const std::complex<float> correlation(movsums_correlation[ant_idx].get_sum());
            const float metric = powf(prefactor * std::abs(correlation) / power, 2.0f);

            // push into smoother
            metric_smoother[ant_idx].pop_push(metric);

            // metric larger?
            peak_vec[ant_idx].update_if_metric_is_larger(
                metric_smoother[ant_idx].get_mean(),
                localbuffer_cnt_r + consumed - metric_smoother_bos_offset_to_center_samples);

#ifdef PHY_RX_AUTOCORRELATOR_PEAK_JSON_EXPORT
            waveform_power[ant_idx].push_back(power);
            waveform_rms[ant_idx].push_back(
                std::sqrt(power / static_cast<float>(stf_bos_length_samples)));
            waveform_metric[ant_idx].push_back(metric);
            waveform_metric_smooth[ant_idx].push_back(metric_smoother[ant_idx].get_mean());
#endif

            ++consumed;

#ifdef RX_SYNC_PARAM_AUTOCORRELATOR_PEAK_RESUM_PERIODICITY_IN_STEPS
            if (resum_cnt++ == RX_SYNC_PARAM_AUTOCORRELATOR_PEAK_RESUM_PERIODICITY_IN_STEPS) {
                resum_for_numerical_stability();
                resum_cnt = 0;
            }
#endif
        }
    }

    localbuffer_cnt_r = localbuffer_cnt_r_limit;

    // have we covered the complete search range?
    if (localbuffer_cnt_r == localbuffer_cnt_r_max) {
        // run final processing after covering full search range
        if (post_processing_validity(sync_report)) {
            post_processing_at_coarse_peak(sync_report);
        }

        // done peak search, whether we found a packet or not is written into sync_report
        return true;
    }

    return false;
}

#ifdef RX_SYNC_PARAM_AUTOCORRELATOR_PEAK_RESUM_PERIODICITY_IN_STEPS
void autocorrelator_peak_t::resum_for_numerical_stability() {
    for (auto& elem : movsums_correlation) {
        elem.resum();
    }

    for (auto& elem : movsums_power) {
        elem.resum();
    }

    for (auto& elem : metric_smoother) {
        elem.resum();
    }
}
#endif

void autocorrelator_peak_t::set_initial_movsums(const uint32_t start_time_local) {
    const uint32_t A_idx = start_time_local;
    const uint32_t B_idx = A_idx + stf_bos_pattern_length_samples;

    for (uint32_t ant_idx = 0; ant_idx < nof_antennas_limited; ++ant_idx) {
        // readability pointers
        const cf_t* A = &localbuffer[ant_idx][A_idx];
        const cf_t* B = &localbuffer[ant_idx][B_idx];

        // correlation

        // directly copy into moving sums
        volk_32fc_x2_multiply_conjugate_32fc((lv_32fc_t*)movsums_correlation[ant_idx].get_front(),
                                             (const lv_32fc_t*)A,
                                             (const lv_32fc_t*)B,
                                             movsums_correlation[ant_idx].get_length());

        // the pointer must be reset before resum()
        movsums_correlation[ant_idx].reset_ptr();
        movsums_correlation[ant_idx].resum();

        metric_smoother[ant_idx].reset();

        // power

        // first multiply onto multiplication_stage_power and ...
        volk_32fc_x2_multiply_conjugate_32fc((lv_32fc_t*)multiplication_stage_power,
                                             (const lv_32fc_t*)A,
                                             (const lv_32fc_t*)A,
                                             stf_bos_length_samples);

        // ... then write real part into moving sums
        volk_32fc_deinterleave_real_32f(movsums_power[ant_idx].get_front(),
                                        (const lv_32fc_t*)multiplication_stage_power,
                                        stf_bos_length_samples);

        movsums_power[ant_idx].reset_ptr();
        movsums_power[ant_idx].resum();
    }
}

bool autocorrelator_peak_t::post_processing_validity(sync_report_t& sync_report) {
    /* At this point, we still don't know whether there is a packet with a valid coarse peak metric
     * or whether this was a false alarm. We check every individual antenna for a valid coarse peak
     * metric.
     */

    float metric_weighted_index = 0.0f;

    for (uint32_t ant_idx = 0; ant_idx < nof_antennas_limited; ++ant_idx) {
        // this antenna's metric
        const float metric = peak_vec[ant_idx].metric;

        // peak validity requirement 0: metric must be larger than at detection point
        if (sync_report.detection_metric +
                RX_SYNC_PARAM_AUTOCORRELATOR_PEAK_METRIC_ABOVE_DETECTION_THRESHOLD_SP >=
            metric) {
            continue;
        }

        // peak validity requirement 1: minimum distance between detection point and coarse peak
        if (static_cast<int64_t>(sync_report.detection_time_local) + detection2peak_samples_64 >=
            static_cast<int64_t>(peak_vec[ant_idx].index)) {
            continue;
        }

        dectnrp_assert(sync_report.coarse_peak_array.at(ant_idx) <= 0.0f, "antenna already valid");

        // mark antenna as valid with a positive value
        sync_report.coarse_peak_array.at(ant_idx) = metric;

        metric_weighted_index += metric * static_cast<float>(peak_vec[ant_idx].index);

#ifdef PHY_RX_AUTOCORRELATOR_PEAK_JSON_EXPORT
        waveform_metric_max_idx[ant_idx] =
            peak_vec[ant_idx].index - sync_report.detection_time_with_jump_back_local;
#endif
    }

    // number of antennas with a valid coarse peak
    const uint32_t valid_cnt = sync_report.coarse_peak_array.get_nof_larger(0.0f);

    // despite a detection, we may have found no valid peaks afterwards (probably false alarm)
    if (valid_cnt == 0) {
        return false;
    }

    // normalized weighted peak index/time
    sync_report.coarse_peak_time_local = static_cast<uint32_t>(
        std::round(metric_weighted_index / sync_report.coarse_peak_array.get_sum()));

    /* Set peak index to actual beginning of STF. The weighted index of the coarse peak currently
     * points to the end of the correlation window. By subtracting (stf_bos_length_samples - 1) it
     * points to the front.
     *
     * Example: When the length of the STF is 4, and we found the peak at index 16, the STF
     * starts at index 16 -= 4-1, i.e. 16-(4-1)=13.
     *
     *  x x x x x x x x x x x x x S S S S x x x x x x x x
     * |0                              |16
     *                           |13
     */
    dectnrp_assert(sync_report.coarse_peak_time_local >= (stf_bos_length_samples - 1),
                   "local coarse peak index negative");

    sync_report.coarse_peak_time_local -= stf_bos_length_samples - 1;

    return true;
}

void autocorrelator_peak_t::post_processing_at_coarse_peak(sync_report_t& sync_report) {
    // recalculate power and correlations
    set_initial_movsums(sync_report.coarse_peak_time_local);

    float metric_sum = 0.0f;
    float metric_weighted_cfo_frac = 0.0f;

    for (uint32_t ant_idx = 0; ant_idx < nof_antennas_limited; ++ant_idx) {
        // skip antennas without a valid coarse peak
        if (sync_report.coarse_peak_array.at(ant_idx) <= 0.0f) {
            continue;
        }

        const float metric = peak_vec[ant_idx].metric;

        metric_sum += metric;

        const float power = movsums_power[ant_idx].get_sum();
        const float rms = std::sqrt(power / static_cast<float>(stf_bos_length_samples));
        sync_report.rms_array.at(ant_idx) = rms;

        const std::complex<float> correlation{movsums_correlation[ant_idx].get_sum()};

        metric_weighted_cfo_frac +=
            metric * std::arg(correlation) / static_cast<float>(stf_bos_pattern_length_samples);
    }

    sync_report.cfo_fractional_rad = metric_weighted_cfo_frac / metric_sum;

    // determine the occupied bandwidth, i.e. beta, and the integer CFO in frequency domain
    std::tie(sync_report.b, sync_report.cfo_integer_rad) =
        coarse_peak_f_domain->process(sync_report.coarse_peak_time_local);

#ifdef PHY_RX_AUTOCORRELATOR_PEAK_JSON_EXPORT
    write_all_data_to_json(sync_report);
#endif
}

#ifdef PHY_RX_AUTOCORRELATOR_PEAK_JSON_EXPORT
void autocorrelator_peak_t::write_all_data_to_json(const sync_report_t& sync_report) const {
    // create filename with unique file count
    std::ostringstream filename;
    filename << "metric_packet_" << std::setw(10) << std::setfill('0') << json_file_cnt.load();

    // increase static file count
    json_file_cnt.fetch_add(1);

    nlohmann::ordered_json j_packet_data;

    j_packet_data["nof_antennas_limited"] = nof_antennas_limited;

    j_packet_data["sync_report"]["cfo_fractional_rad"] = sync_report.cfo_fractional_rad;
    j_packet_data["sync_report"]["rms_array"] = sync_report.rms_array.get_ary();

    auto concatenate = [](const common::vec2d<float>& vec) {
        // concatenate metric vec of each antenna
        std::vector<float> vec_concatenated;
        for (uint32_t ant_idx = 0; ant_idx < vec.size(); ++ant_idx) {
            for (const auto elem : vec[ant_idx]) {
                vec_concatenated.push_back(elem);
            }
        }

        return vec_concatenated;
    };

    j_packet_data["waveform_power"] = concatenate(waveform_power);
    j_packet_data["waveform_rms"] = concatenate(waveform_rms);
    j_packet_data["waveform_metric"] = concatenate(waveform_metric);
    j_packet_data["waveform_metric_smooth"] = concatenate(waveform_metric_smooth);
    j_packet_data["waveform_metric_max_idx"] = waveform_metric_max_idx;

    j_packet_data["metric_smoother_length"] = metric_smoother[0].get_length();
    j_packet_data["metric_smoother_bos_offset_to_center_samples"] =
        metric_smoother_bos_offset_to_center_samples;

    common::json_export_t::write_to_disk(j_packet_data, filename.str());
}
#endif

}  // namespace dectnrp::phy
