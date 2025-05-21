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

#include "dectnrp/common/randomgen.hpp"
#include "dectnrp/radio/hw_simulator.hpp"
#include "dectnrp/sections_part4/mac_architecture/identity.hpp"
#include "dectnrp/upper/tpoint.hpp"

namespace dectnrp::upper::tfw::loopback {

class tfw_loopback_t : public tpoint_t {
    public:
        tfw_loopback_t(const tpoint_config_t& tpoint_config_, phy::mac_lower_t& mac_lower_);
        ~tfw_loopback_t() = default;

        tfw_loopback_t() = delete;
        tfw_loopback_t(const tfw_loopback_t&) = delete;
        tfw_loopback_t& operator=(const tfw_loopback_t&) = delete;
        tfw_loopback_t(tfw_loopback_t&&) = delete;
        tfw_loopback_t& operator=(tfw_loopback_t&&) = delete;

        void work_start_imminent(const int64_t start_time_64) override;
        phy::machigh_phy_t work_regular(const phy::phy_mac_reg_t& phy_mac_reg) override;
        // phy::maclow_phy_t work_pcc(const phy::phy_maclow_t& phy_maclow) override;
        // phy::machigh_phy_t work_pdc_async(const phy::phy_machigh_t& phy_machigh) override;
        phy::machigh_phy_t work_upper(const upper::upper_report_t& upper_report) override;
        phy::machigh_phy_tx_t work_chscan_async(const phy::chscan_t& chscan) override;

    protected:
        std::vector<std::string> start_threads() override final;
        std::vector<std::string> stop_threads() override final;

        radio::hw_simulator_t* hw_simulator;

        /// state machine for for experiment coordination
        enum class STATE_t {
            A_SET_CHANNEL_SNR,
            B_SET_CHANNEL_SMALL_SCALE_FADING,
            C_EXPERIMENT_GENERATE_PACKETS,
            D_EXPERIMENT_SAVE_RESULTS,
            E_SET_PARAMETER,
            DEAD_END
        } state;

        struct state_transition_time_t {
                int64_t x_to_A_64;
                int64_t A_to_B_64;
                int64_t B_to_C_64;
                int64_t C_to_B_64;
                int64_t C_to_D_64;
        } stt;

        /// timing between states
        int64_t state_time_reference_64;

        /// every deriving class uses a parameter vector
        uint32_t parameter_cnt;

        /// we measure PER over SNR regardless of the mode
        std::vector<float> snr_vec;
        uint32_t snr_cnt;

        /// at every SNR the same number of experiments is conducted
        uint32_t nof_experiment_per_snr;
        uint32_t nof_experiment_per_snr_cnt;

        /// random number generation
        common::randomgen_t randomgen;

        /// meta data used in generate_packet()
        struct packet_params_t {
                sp3::packet_sizes_def_t psdef;
                uint32_t N_samples_in_packet_length;

                /// PLCF to test
                uint32_t PLCF_type;
                uint32_t PLCF_type_header_format;

                sp4::mac_architecture::identity_t identity;
                sp4::plcf_10_t plcf_10;
                sp4::plcf_20_t plcf_20;
                sp4::plcf_21_t plcf_21;

                void update_plcf_unpacked();

                /// force transmission time to multiple of this value
                int64_t tx_time_multiple_64;

                // actual transmission time to use
                int64_t tx_time_64;

                /// amplitude scaling
                float amplitude_scale;

                /// fractional CFO
                float cfo_symmetric_range_subc_multiple;
        } pp;

        void generate_packet(phy::machigh_phy_t& machigh_phy);

        virtual void set_mac_pdu(uint8_t* a_tb, const uint32_t N_TB_byte);

        int64_t get_random_tx_time(const int64_t now_64);

        // ##################################################
        /* Virtual functions called in state machine. Prefixed letter is the state in which the
         * functions are called.
         */

        virtual void A_reset_result_counter_for_next_snr() = 0;

        virtual void C_generate_single_experiment_at_current_snr(
            const int64_t now_64, phy::machigh_phy_t& machigh_phy) = 0;

        virtual void D_save_result_of_current_snr() = 0;

        virtual bool E_set_next_parameter_or_go_to_dead_end() = 0;

        virtual void save_all_results_to_file() const = 0;
};

}  // namespace dectnrp::upper::tfw::loopback
