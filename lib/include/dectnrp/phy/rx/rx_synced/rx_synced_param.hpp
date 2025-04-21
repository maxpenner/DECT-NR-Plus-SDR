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

namespace dectnrp::phy {

// ########################################################################################################
// What is provided to rx_synced_t by synchronization for optimal SINR?
// ########################################################################################################

/* Synchronization provides all of its information to rx_synced_t via sync_report_t. Each
 * sync_report_t contains the results of the individual synchronization steps (detection, coarse
 * peak, fine peak). All of these information and steps are solely performed on the STF. Some of
 * that information is critical for rx_synced_t to achieve demodulation and decoding with the
 * highest SINR possible:
 *
 * 0) mu                gathered at detection
 * 1) RMS               gathered at coarse peak (not every antennas necessarily has a coarse peak)
 * 2) Fractional CFO    gathered at coarse peak
 * 3) Integer CFO       gathered at coarse peak in frequency domain
 * 4) beta              gathered at coarse peak in frequency domain
 * 5) Integer STO       gathered at fine peak, variable name is fine_peak_time_64
 *
 * Two of the above values can be overwritten by rx_synced:
 *
 *  A) RMS              sync only provides the RMS for antennas with a valid coarse peak, rx_synced
 *                      can fill the gaps and optionally overwrite existing values
 *  B) Fractional CFO   synchronization may start resampling unaligned to the STF, rx_synced starts
 *                      resampling exactly at the fine peak and estimates the CFO
 *
 * Furthermore, rx_synced_t has to estimate additional values to allow optimal demodulation and
 * decoding:
 *
 * 6) Fractional STO    gathered at fine peak in frequency domain, solely performed on STF,
 *                      phase error gradient (PEG)
 * 7) Residual STO      performed on DRS in frequency domain, improves fractional STO, some of the
 *                      fractional STO is due to a symbol clock offset (SCO) between TX and RX,
 *                      phase error gradient (PEG)
 * 8) Residual CFO      performed on STF and/or DRS, frequency domain, improves fractional CFO
 *                      common phase error (CPE)
 * 9) Channel estimate  performed on STF and/or DRS, frequency domain
 * 10) SNR              performed on STF and/or DRS, frequency domain
 *
 * Note that transceivers typically use the same clock source for mixing and sampling. As a
 * consequence, the CFO and SCO have the same source and are tied together.
 */

// ####################################################
// Integer STO adjustment
// ####################################################

/**
 * \brief If set to 0, the fine sync point as provided by synchronization is used. It set to
 * positive value the sync point is moved into the cyclic prefix (CP). Given as percentage of the
 * total STF length. Typical values are 0 to 15.
 */
#define RX_SYNCED_PARAM_STO_INTEGER_MOVE_INTO_CP_IN_PERCENTAGE_OF_STF 0

// ####################################################
// RMS
// ####################################################

/**
 * \brief Synchronization always provides at least one RMS value for one of the physical RX
 * antennas, and at most one RMS value for each physical RX antenna. If activated, the RMS values of
 * synchronization are updated, so that each physical RX antenna has an RMS value. In that case the
 * AGC on the MAC layer can tune each antenna individually.
 */
#define RX_SYNCED_PARAM_RMS_FILL_COMPLETELY_OR_KEEP_WHAT_SYNCHRONIZATION_PROVIDED

/// if the RMS value is estimated for each antenna, we can limit the total length of the estimation
#define RX_SYNCED_PARAM_RMS_PERCENTAGE_OF_STF_USED_FOR_RMS_ESTIMATION 100

/// if activated, any RMS values provided by sync are kept and only the gaps are filled
#define RX_SYNCED_PARAM_RMS_KEEP_VALUES_PROVIDED_BY_SYNC

// ####################################################
// Fractional + Integer CFO correction
// ####################################################

/**
 * \brief Correction of the CFO begins at the fine synchronization point and is applied after
 * resampling. Can be deactivated for debugging purposes. At the beginning, only the fractional plus
 * integer CFO are corrected, over time the fractional CFO is added.
 */
#define RX_SYNCED_PARAM_CFO_CORRECTION

// ####################################################
// Fractional CFO adjustment
// ####################################################

/**
 * \brief Synchronization delivers us a precise estimation of both the integer and fractional CFO.
 * In rx_synced_t, however, we can adjust the fractional CFO after resampling starting at the fine
 * synchronization point provided by synchronization. The value in the sync_report_t provided by
 * synchronization will be adjusted as well.
 */
#define RX_SYNCED_PARAM_CFO_FRACTIONAL_ADJUST

// ########################################################################################################
// The following actions are all performed after the FFT in frequency domain.
// ########################################################################################################

// ####################################################
// Amplitude Scaling to maintain an average power of 1
// ####################################################

/**
 * \brief FFTW by default does not apply a scaling factor. Furthermore, scaling the amplitude is not
 * required as it is done implicitly in the channel estimation anyway. However, it is useful when
 * debugging in an AWGN channel and does not deteriorate performance.
 */
#define RX_SYNCED_PARAM_AMPLITUDE_SCALING

// ####################################################
// Fractional and Residual STO (as a result of an SCO)
// ####################################################

/**
 * \brief The synchronization delivers a fine synchronization point with a single sample resolution.
 * However, there might be a fractional STO of less than one sample. This fractional time delay in
 * time domain leads to a frequency dependent phase rotation in frequency domain. We exploit that
 * fact and measure the phase rotation between two neighbouring subcarriers of the STF. Based on
 * that, we get an initial estimation of the phase rotation and correct every OFDM symbol.
 *
 * Additionally, the perceived time delay may change over time if there is a significant SCO due to
 * the transmitted signal and the received signal being sampled with different clocks. Over time,
 * the alignment at the receiver deteriorates. This residual phase rotation can be estimated based
 * on the OFDM symbols with DRS cells.
 *
 * https://de.mathworks.com/help/wlan/ug/joint-sampling-rate-and-carrier-frequency-offset-tracking.html
 */

/// fractional STO based on STF
#define RX_SYNCED_PARAM_STO_FRACTIONAL_BASED_ON_STF

/// residual STO estimation based on DRS (not functional yet)
// #define RX_SYNCED_PARAM_STO_RESIDUAL_BASED_ON_DRS

/// if residual STO based on DRS is active, we can limit the number of transmit streams to use
#define RX_SYNCED_PARAM_STO_RESIDUAL_BASED_ON_DRS_N_TS_MAX 8U

// ####################################################
// Residual CFO
// ####################################################

/**
 * \brief The residual CFO estimation is based on two OFDM symbols with STO and/or DRS cells. In
 * case of two OFDM with DRS cells are compared, their separation in time domain is N_step. The
 * phase rotation due to the residual CFO is the same for each subcarrier of an OFDM symbol, but
 * increases over time.
 *
 * https://de.mathworks.com/help/wlan/ug/joint-sampling-rate-and-carrier-frequency-offset-tracking.html
 */

/// residual CFO estimation based on STF (not functional yet)
// #define RX_SYNCED_PARAM_CFO_RESIDUAL_BASED_ON_STF

/// residual CFO estimation based on DRS (not functional yet)
// #define RX_SYNCED_PARAM_CFO_RESIDUAL_BASED_ON_DRS

/// if residual CFO based on DRS is active, we can limit the number of transmit streams to use
#define RX_SYNCED_PARAM_CFO_RESIDUAL_BASED_ON_DRS_N_TS_MAX 8U

// ####################################################
// Channel estimation (Amplitude and Phase)
// ####################################################

/// type of the LUT indices
#define RX_SYNCED_PARAM_LUT_IDX_TYPE uint32_t

/**
 * \brief The channel estimation can use real or complex weights when interpolating at/between the
 * zero-forced channel estimates from the pilot positions. This also determines whether the
 * Wiener-Hopf equation uses real of complex numbers, and real or complex correlation values.
 *
 * \todo: Currently only the real version is functional, matrix inversion in wiener.cpp for complex
 * type leads to ill-formed interpolation coefficients.
 */
#define RX_SYNCED_PARAM_WEIGHTS_TYPE_REAL 0
#define RX_SYNCED_PARAM_WEIGHTS_TYPE_COMP 1
#define RX_SYNCED_PARAM_WEIGHTS_TYPE_CHOICE RX_SYNCED_PARAM_WEIGHTS_TYPE_REAL

/// define actual type used for calculations
#if RX_SYNCED_PARAM_WEIGHTS_TYPE_CHOICE == RX_SYNCED_PARAM_WEIGHTS_TYPE_REAL
#define RX_SYNCED_PARAM_WEIGHTS_TYPE float
#define RX_SYNCED_PARAM_WEIGHTS_TYPE_INTERNAL double
#elif RX_SYNCED_PARAM_WEIGHTS_TYPE_CHOICE == RX_SYNCED_PARAM_WEIGHTS_TYPE_COMP
extern "C" {
#include "srsran/config.h"
}
#include <complex>
#define RX_SYNCED_PARAM_WEIGHTS_TYPE cf_t
#define RX_SYNCED_PARAM_WEIGHTS_TYPE_INTERNAL std::complex<double>
#endif

// clang-format off
/// default values for Wiener filter statistics
#define RX_SYNCED_PARAM_NU_MAX_HZ_VEC {100.0, 100.0, 500.0f}
#define RX_SYNCED_PARAM_TAU_RMS_SEC_VEC {0.1e-6, 0.1e-6, 1.0e-6}

/**
 * \brief For channel estimation based on a Wiener filter, the SNR determines the amount of smoothing
 * that is applied between individual DRS symbols. At low SNR, smoothing is increased. For optimal
 * performance, it is best to precalculate multiple Wiener filters for different SNRs, estimate the
 * instantaneous SNR during decoding and the pick the best fitting Wiener filter.
 */
#define RX_SYNCED_PARAM_SNR_DB_VEC {-5.0, 15.0, 35.0}

/**
 * \brief Number of consecutive DRS pilots used for interpolation, with pilots being interlaced for mode
 * lr. Determines the computational complexity of interpolation.
 */
#define RX_SYNCED_PARAM_NOF_DRS_INTERP_LR_VEC {14, 8, 3}
#define RX_SYNCED_PARAM_NOF_DRS_INTERP_L_VEC {7, 4, 2}
// clang-format on

/// optimization flags to speed up LUT generation
#define RX_SYNCED_PARAM_CHANNEL_LUT_OPT_INDEX_PREVIOUS
#define RX_SYNCED_PARAM_CHANNEL_LUT_SEARCH_ABORT_THRESHOLD 1.1

/**
 * \brief Wiener filter coefficients are chosen based on the SNR estimation. The SNR estimation is
 * improved after every new OFDM symbol with DRS cells. We can make the coefficients lookup either
 * once for the first symbol with DRS cells, or after every such OFDM symbol.
 */
#define RX_SYNCED_PARAM_CHANNEL_LUT_LOOKUP_AFTER_EVERY_DRS_SYMBOL_OR_ONCE

// ####################################################
// Signal to Noise Ratio (SNR)
// ####################################################

#define RX_SYNCED_PARAM_SNR_BASED_ON_STF
#define RX_SYNCED_PARAM_SNR_BASED_ON_DRS
#define RX_SYNCED_PARAM_SNR_BASED_ON_DRS_N_TS_MAX 8U

// ####################################################
// Multiple Input Multiple Output (MIMO)
// ####################################################

/**
 * \brief The MIMO report must estimate the channel across all transmit streams. This is not
 * possible based on the STF if more than one transmit stream is used, since the STF is always
 * allocated in transmit stream 0. Thus, we use only the DRS cells by default. Furthermore, we
 * always use the final DRS cells at the very end of the packet, as these represent the latest state
 * of the channel.
 */
#define RX_SYNCED_PARAM_MIMO_BASED_ON_STF_AND_DRS_AT_PACKET_END

/**
 * \brief Running all MIMO algorithms across the full bandwidth is not necessary. Instead, a few
 * cells across the spectrum are extracted and all algorithms are based on this selection.
 */
#define RX_SYNCED_PARAM_MIMO_N_WIDEBAND_CELLS 4

/**
 * \brief We can maximize power by emitting only non-zero signals from all antennas, and ignoring
 * the beamforming matrix scaling factor.
 */
// #define RX_SYNCED_PARAM_MIMO_USE_ALL_W_MATRICES_OR_ONLY_NON_ZERO

/// option A: highest minimum power across all antennas
#define RX_SYNCED_PARAM_MODE_3_7_METRIC_HIGHEST_MIN_RX_POWER 0
/// option B: maximum power across all antennas
#define RX_SYNCED_PARAM_MODE_3_7_METRIC_MAX_RX_POWER 1
// choice
#define RX_SYNCED_PARAM_MODE_3_7_METRIC RX_SYNCED_PARAM_MODE_3_7_METRIC_HIGHEST_MIN_RX_POWER

// ########################################################################################################
// Temporary System Restrictions
// ########################################################################################################

// ####################################################
// Block any MIMO mode with N_eff_TX > 1
// ####################################################

// #define RX_SYNCED_PARAM_BLOCK_N_EFF_TX_LARGER_1_AT_PCC
// #define RX_SYNCED_PARAM_BLOCK_N_EFF_TX_LARGER_1_AT_PDC

#define RX_SYNCED_PARAM_BLOCK_N_SS_TX_LARGER_1_AT_PCC

}  // namespace dectnrp::phy
