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

#include "dectnrp/sections_part3/stf_param.hpp"

namespace dectnrp::phy {

// ####################################################
// General Configuration of Synchronization
// ####################################################

/**
 * \brief Every instance of worker_sync_t makes a suggestion where synchronization should start, and
 * the latest time wins. This directive defines how far into the future the suggestion should lie.
 * This time advance has become unnecessary with the warmup function, but the functionality is kept.
 */
#define RX_SYNC_PARAM_SYNCHRONIZATION_START_TIME_ADVANCE_MS 0

/**
 * \brief To fully utilize multi-threading for synchronization, individual threads must be able to
 * process their current chunk without holding the baton yet. While processing the current chunk,
 * multiple packets can be found which are then buffered if the baton has not been passed on yet.
 * The minimum number of bufferable synchronizations is 1, in which case each thread must acquire
 * the baton after finding its first packet, or reaching the end of its respective chunk.
 */
#define RX_SYNC_PARAM_MAX_NOF_BUFFERABLE_SYNC_BEFORE_ACQUIRING_BATON 10

/**
 * \brief When using multiple antennas, we can reduce the computational load by only using the first
 * few antennas. Must be at least 1, by setting to 8 the SDR configuration determines the number of
 * antennas.
 */
#define RX_SYNC_PARAM_AUTOCORRELATOR_ANTENNA_LIMIT 8U

// ####################################################
// Unique Packet Sync Time Limit for Double Detection Avoidance
// ####################################################

/**
 * \brief Due to the multi-threaded structure of the synchronization and the split into chunks on
 * the time axis, it can happen that two threads synchronize the same packet in the common overlap
 * area of two neighbouring chunks - a double detection. To avoid those, a packet's synchronization
 * time must be at least this amount of time after the packet before it. Otherwise, the
 * worker_sync_t instance discards the packet.
 */
#define RX_SYNC_PARAM_SYNC_TIME_UNIQUE_LIMIT_IN_STF_PATTERNS_DP 1.0

// ####################################################
// Autocorrelation Detection
// ####################################################

/**
 * \brief We can increase the overlap area between neighbouring chunks of threads, which makes
 * misses less likely at the cost of more double detections and higher computational load. As a rule
 * thumb, the length of the overlap area should at the 4 times the length of the longest conceivable
 * STF.
 */
#define RX_SYNC_PARAM_AUTOCORRELATOR_DETECTION_OVERLAP_LENGTH_IN_STFS_DP 4.0

/**
 * \brief The maximum step width for STF detection is one STF pattern, i.e. 16*b*oversampling
 * (16*7=112, 16*9=144). This step width is active when the step divider is set to 1. This should be
 * sufficient for most SNRs. When set to 2, the step width is halved to 8*b*oversampling, which is
 * enough even for very low SNRs. Note that the step width also influences other aspects of
 * synchronization.
 */
#ifdef SECTIONS_PART_3_STF_COVER_SEQUENCE_ACTIVE
#define RX_SYNC_PARAM_AUTOCORRELATOR_DETECTION_STEP_DIVIDER 4
#else
#define RX_SYNC_PARAM_AUTOCORRELATOR_DETECTION_STEP_DIVIDER 2
#endif

/**
 * \brief For synchronization, we use accumulators. From time to time, these have to be re-summed to
 * avoid numerical imprecision. This is especially important when packets with highly uneven power
 * levels are received with a small time gap in between them, for instance self-reception followed
 * by a packet with small power.
 */
#define RX_SYNC_PARAM_AUTOCORRELATOR_DETECTION_RESUM_PERIODICITY_IN_STEPS 16

/**
 * \brief Incoming samples of a potential STF must stay within these RMS thresholds to be considered
 * for correlation.
 *
 * Lower RMS bound: The goal is to make it as small as possible, but not too small to avoid
 * numerical imprecision and to stay above the noise floor of the ADC. The latter is typically so
 * high that numerical imprecision with 32-bit floats is rather unlikely. The optimal lower bound
 * also depends on the system bandwidth, as the maximum Vpp of an ADC always remains the same, but
 * the thermal noise increases. We use 30.72MHz as a reference value, measured with an 50 Ohm
 * resistor.
 *
 * At 30.72MHz: 0.005   equivalent to 20*log10(0.005)   = -46.0  dBFS
 * At  1.92MHz: 0.00125 equivalent to 20*log10(0.00125) = -58.0  dBFS
 *
 * Upper boundary: RX signals can become so large that the ADC operates at its upper resolution
 * limit. When clipping, it generates long sequences of +1 and -1. Worst case scenario is, that due
 * to the small set of distinct numerical values the correlation becomes large and causes false
 * alarms. By defining a maximum amount of input power, we can avoid that.
 */
#define RX_SYNC_PARAM_AUTOCORRELATOR_DETECTION_RMS_THRESHOLD_MIN_REFERENCE_SAMPLE_RATE_DP 30.72e6
#define RX_SYNC_PARAM_AUTOCORRELATOR_DETECTION_RMS_THRESHOLD_MIN_SP 0.005f
#define RX_SYNC_PARAM_AUTOCORRELATOR_DETECTION_RMS_THRESHOLD_MAX_SP 2.0f

/**
 * \brief We can receive two packets right after each other as shown here:
 *
 *       Packet      STF
 *      XXXXXXXXX__1234567
 *          _______
 *           _______
 *            _______
 *      back-> _______ <-front
 *              _______
 *               _______
 *                _______
 *
 * They are separated only by two patterns. This is very challenging if the first packet has much
 * more power than the STF. As a countermeasure, we expect more power at the front of the
 * correlation window than at the back. This way we make sure the first packet is no longer within
 * the correlation window.
 */
#define RX_SYNC_PARAM_AUTOCORRELATOR_DETECTION_RMS_N_STEPS_FRONT 2
#define RX_SYNC_PARAM_AUTOCORRELATOR_DETECTION_RMS_N_STEPS_BACK 2
#define RX_SYNC_PARAM_AUTOCORRELATOR_DETECTION_RMS_STEPS_RATIO_FRONT_TO_BACK 1.0f

/**
 * \brief Normalized coarse metric thresholds for detection.
 *
 * Lower boundary: Even pure white noise can temporarily show increased correlation values during
 * synchronization, which could indicate a false packet reception. By defining a minimum limit for
 * the normalized coarse metric, we can avoid this type of false alarm. However, this limit should
 * not be too large as we could miss actual packets at low SNRs.
 *
 * Upper boundary: The normalized coarse metric ideally shows values between 0.0 and 1.0. However,
 * when the ADC operates at it lower resolution limit, we can see numerical instability and
 * correlations values above 1.0. We set an upper limit beyond which we ignore the metric. Also, the
 * lower RMS limit should be set to a value large enough to avoid this scenario.
 */
#ifdef SECTIONS_PART_3_STF_COVER_SEQUENCE_ACTIVE
#define RX_SYNC_PARAM_AUTOCORRELATOR_DETECTION_METRIC_THRESHOLD_MIN_SP 0.18f
#else
#define RX_SYNC_PARAM_AUTOCORRELATOR_DETECTION_METRIC_THRESHOLD_MIN_SP 0.30f
#endif
#define RX_SYNC_PARAM_AUTOCORRELATOR_DETECTION_METRIC_THRESHOLD_MAX_SP 1.50f

/**
 * \brief During detection, the metric must increase multiple times in a row to make sure we
 * detected a rising edge of the coarse metric.
 */
#define RX_SYNC_PARAM_AUTOCORRELATOR_DETECTION_METRIC_STREAK_RELATIVE_GAIN 1.005
#ifdef SECTIONS_PART_3_STF_COVER_SEQUENCE_ACTIVE
#define RX_SYNC_PARAM_AUTOCORRELATOR_DETECTION_METRIC_STREAK 1
#else
#define RX_SYNC_PARAM_AUTOCORRELATOR_DETECTION_METRIC_STREAK 2
#endif

/**
 * \brief When a packet was detected, we can jump backwards in time before starting the search for
 * the coarse peak. If the SNR is small, or the coarse metric very narrow (for instance due to the
 * cover sequence), we can potentially hit the falling edge of the coarse metric during detection.
 */
#define RX_SYNC_PARAM_AUTOCORRELATOR_DETECTION_JUMP_BACK_IN_PATTERNS 1

/**
 * \brief When a packet was detected and we found a rising edge with a valid coarse peak afterwards,
 * we skip a longer sequence of samples to avoid re-detecting the same STF again. We skip from the
 * beginning of an STF, thus we should skip a range close to the full metric, i.e. two STFs.
 */
#define RX_SYNC_PARAM_AUTOCORRELATOR_DETECTION_SKIP_AFTER_PEAK_IN_STFS_DP 2.0

// ####################################################
// Autocorrelation Coarse Peak Search for RISING EDGE
// ####################################################

/// coarse peak search requires only one new samples, but for efficiency we process larger amounts
#define RX_SYNC_PARAM_AUTOCORRELATOR_PEAK_SAMPLES_REQUEST_IN_PATTERNS 1

/// see RX_SYNC_PARAM_AUTOCORRELATOR_DETECTION_RESUM_PERIODICITY_IN_STEPS
#define RX_SYNC_PARAM_AUTOCORRELATOR_PEAK_RESUM_PERIODICITY_IN_STEPS 64

/// search length after detection point, should take into consideration the jump back width
#define RX_SYNC_PARAM_AUTOCORRELATOR_PEAK_MAX_SEARCH_LENGTH_IN_STFS_DP 1.0

/// to smoothen the coarse peak, we apply some averaging
#ifdef SECTIONS_PART_3_STF_COVER_SEQUENCE_ACTIVE
#define RX_SYNC_PARAM_AUTOCORRELATOR_PEAK_MOVMEAN_SMOOTH_LEFT 1
#define RX_SYNC_PARAM_AUTOCORRELATOR_PEAK_MOVMEAN_SMOOTH_RIGHT 1
#else
#define RX_SYNC_PARAM_AUTOCORRELATOR_PEAK_MOVMEAN_SMOOTH_LEFT 7
#define RX_SYNC_PARAM_AUTOCORRELATOR_PEAK_MOVMEAN_SMOOTH_RIGHT 1
#endif

/// the coarse peak metric must be larger by this amount than metric at the detection point
#define RX_SYNC_PARAM_AUTOCORRELATOR_PEAK_METRIC_ABOVE_DETECTION_THRESHOLD_SP 0.05f

/// minimum distance between the detection point and the coarse peak
#define RX_SYNC_PARAM_AUTOCORRELATOR_PEAK_DETEC2PEAK_THRESHOLD_IN_STFS_DP 0.1

/**
 * \brief If a radio device class defines a maximum of b=8, the synchronization is triggered by
 * packets for b=8,4,2,1. First option is to find the best fitting b in frequency domain by
 * measuring power with the given threshold, second option is to always assume b=8, i.e. the maximum
 * value of the radio device class.
 */
// #define RX_SYNC_PARAM_AUTOCORRELATOR_PEAK_FIND_BETA_THRESHOLD_DB_OR_ASSUME_MAX_OF_RDC -10.0f

/**
 * \brief The DECTNRP standard allows up to 30ppm at TX and RX. With 27kHz subcarrier spacing, this
 * can lead to a significant amount of integer CFO. Luckily, coarse peak search is not impaired by
 * an integer CFO. Fine peak search, however, is very susceptible to integer CFOs. First option is
 * to find the best fitting integer CFO within the search range, other option is to assume zero.
 */
// #define RX_SYNC_PARAM_AUTOCORRELATOR_PEAK_FIND_INTEGER_CFO_SEARCH_RANGE_OR_ASSUME_ZERO 4

// ####################################################
// Crosscorrelation Fine Peak Search
// ####################################################

// correct CFO estimated at coarse peak before crosscorrelation
#define RX_SYNC_PARAM_CROSSCORRELATOR_CFO_PRECORRECTION

/**
 * \brief We can reduce the STF length used for crosscorrelation. This way the number of
 * multiplications becomes smaller. Must be smaller or equal 1.0.
 */
#define RX_SYNC_PARAM_CROSSCORRELATOR_STF_LENGTH_EFFECTIVE_DP 1.0

/**
 * \brief We search for the fine peak to the left and right of the coarse peak. These values refer
 * to the search length in samples when a pattern has 16 samples, the actual search range is then
 * multiplied by b*oversampling. The maximum values allowed is two STF patterns both to the left and
 * right, i.e. a total search range of 4 pattern.
 */
#define RX_SYNC_PARAM_CROSSCORRELATOR_SEARCH_LEFT_SAMPLES 16   // maximum allowed is 2*16=32
#define RX_SYNC_PARAM_CROSSCORRELATOR_SEARCH_RIGHT_SAMPLES 16  // maximum allowed is 2*16=32

/**
 * \brief Perform crosscorrelation across all antennas or only the strongest antenna as provided by
 * coarse peak search. Crosscorrelation requires a very large number of multiplications (frequency
 * domain methods for fast convolution/crosscorrelation are not applicable).
 */
// #define RX_SYNC_PARAM_CROSSCORRELATOR_ALL_ANTENNAS_OR_ONLY_STRONGEST

// ####################################################
// DEBUGGING: Packet Fine Sync Point Multiple
// ####################################################

/**
 * \brief Packet detection through autocorrelation is a quite robust algorithm, so we assume we can
 * detect every packet of interest to us. Under the assumption that a packet is detected, this flag
 * can be set to force a coarse synchronization point to a specific sample multiple. If TX uses the
 * same multiple for transmission, this is as good as perfect STO synchronization. The multiple
 * should be a relatively large (prime) number to avoid perfect alignment when resampling.
 *
 * To also force the fine synchronization point to this sample multiple, overwrite these values with
 * zero:
 *
 *      #define RX_SYNC_PARAM_CROSSCORRELATOR_SEARCH_LEFT_SAMPLES 20
 *      #define RX_SYNC_PARAM_CROSSCORRELATOR_SEARCH_RIGHT_SAMPLES 20
 */
// #define RX_SYNC_PARAM_DBG_COARSE_SYNC_PEAK_FORCED_TO_TIME_MULTIPLE 1007

}  // namespace dectnrp::phy
