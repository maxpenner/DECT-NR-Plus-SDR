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

extern "C" {
#include "srsran/config.h"
}

#include "dectnrp/phy/rx/rx_synced/estimator/estimator.hpp"

namespace dectnrp::phy {

class estimator_snr_t final : public estimator_t {
    public:
        explicit estimator_snr_t(const uint32_t b_max);
        ~estimator_snr_t();

        estimator_snr_t() = delete;
        estimator_snr_t(const estimator_snr_t&) = delete;
        estimator_snr_t& operator=(const estimator_snr_t&) = delete;
        estimator_snr_t(estimator_snr_t&&) = delete;
        estimator_snr_t& operator=(estimator_snr_t&&) = delete;

        virtual void process_stf(const channel_antennas_t& channel_antennas,
                                 const process_stf_meta_t& process_stf_meta) override final;

        virtual void process_drs(const channel_antennas_t& channel_antennas,
                                 const process_drs_meta_t& process_drs_meta) override final;

        /// valid only if process_drs() was called before
        float get_current_snr_dB_estimation() const;

    private:
        virtual void reset_internal() override final;

        struct snr_acc_t {
                /// equation (13) from "SNR Estimation Algorithm Based on the Preamble for OFDM
                /// Systems in Frequency Selective Channels"
                float S_plus_N_sum{};
                uint32_t S_plus_N_cnt{};

                /// equation (9) from "SNR Estimation Algorithm Based on the Preamble for OFDM
                /// Systems in Frequency Selective Channels"
                float N_sum{};
                uint32_t N_cnt{};

                void operator+=(const snr_acc_t& rhs) {
                    S_plus_N_sum += rhs.S_plus_N_sum;
                    S_plus_N_cnt += rhs.S_plus_N_cnt;
                    N_sum += rhs.N_sum;
                    N_cnt += rhs.N_cnt;
                }
        } snr_acc;

        cf_t* subtraction_stage;

        [[nodiscard]] snr_acc_t process_stf_or_drs_packed(const cf_t* chestim_drs_zf,
                                                          const uint32_t nof_pilots);
};

}  // namespace dectnrp::phy
