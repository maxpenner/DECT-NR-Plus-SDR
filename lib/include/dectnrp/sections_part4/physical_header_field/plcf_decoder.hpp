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

#include <array>
#include <cstdint>

#include "dectnrp/common/json_switch.hpp"
#include "dectnrp/sections_part3/derivative/fec_cfg.hpp"
#include "dectnrp/sections_part4/physical_header_field/plcf_10.hpp"
#include "dectnrp/sections_part4/physical_header_field/plcf_20.hpp"
#include "dectnrp/sections_part4/physical_header_field/plcf_21.hpp"

#ifdef PHY_JSON_SWITCH_IMPLEMENT_ANY_JSON_FUNCTIONALITY
#include "dectnrp/external/nlohmann/json.hpp"
#endif

namespace dectnrp::section4 {

class plcf_decoder_t {
    public:
        /**
         * \brief The PLCF pool takes packed bits from PHY for either PLCF type 1 or 2 after the CRC
         * has been confirmed, and extracts the content of the PLCF depending on the header format.
         * Also checks whether the extracted values stay within the limits set by the radio device
         * class, which is a prerequisite for proceeding with the PDC.
         *
         * \param PacketLength_max_ maximum packet length supported by radio device class
         * \param mcs_index_max_ maximum MCS index supported by radio device class
         * \param N_SS_max_ maximum number of spatial streams supported by radio device class
         */
        explicit plcf_decoder_t(uint32_t PacketLength_max_,
                                uint32_t mcs_index_max_,
                                uint32_t N_SS_max_);
        ~plcf_decoder_t() = default;

        plcf_decoder_t() = delete;
        plcf_decoder_t(const plcf_decoder_t&) = delete;
        plcf_decoder_t& operator=(const plcf_decoder_t&) = delete;
        plcf_decoder_t(plcf_decoder_t&&) = delete;
        plcf_decoder_t& operator=(plcf_decoder_t&&) = delete;

        // ##################################################
        // PLCF decoding

        /**
         * \brief Must be called for every packet before decoding, internally should set both PLCF
         * types to an undefined state.
         */
        void set_configuration();

        /**
         * \brief Called once with PLCF_type=1 and once with PLCF_type=2 for every packet. Every
         * PLCF's CRC can be masked to indicate "closed loop" and/or "beamforming", which we extract
         * into instances of fec_cfg_t. To provide it alongside with the PLCF's binary content, we
         * keep the structures in this decoder.
         *
         * \param PLCF_type either 1 or 2
         * \return separate fec_cfg_t for either type 1 or type 2
         */
        [[nodiscard]] section3::fec_cfg_t& get_fec_cfg(const uint32_t PLCF_type);

        /**
         * \brief Called once with PLCF_type=1 and once with PLCF_type=2 for every packet.
         * Internally, reads the PLCF, unpacks and checks for radio device class limits. Calls for
         * type 1 and type 2 are separated because PHY reuses the same HARQ buffer for type 1 and
         * type 2.
         *
         * \param PLCF_type either 1 or 2
         * \param plcf_front pointer to packed bits of PLCF after CRC confirmation
         */
        void decode_and_rdc_check(const uint32_t PLCF_type, const uint8_t* plcf_front);

        // ##################################################
        // PLCF retrieval after decoding

        /**
         * \brief Convenience function to poll which PLCFs were decoded into valid states.
         *
         * \return 0 if no valid PLCF found, 1 for type 1, 2 for type 2, 3 for type 1 and type 2
         */
        [[nodiscard]] uint32_t has_any_plcf() const;

        /**
         * \brief Get a base pointer to a PLCF type 1 or 2. The base pointer points to the correct
         * header format and can be casted.
         *
         * \param PLCF_type either 1 or 2
         * \return base pointer to PLCF with correct header format, otherwise nullptr
         */
        [[nodiscard]] const plcf_base_t* get_plcf_base(const uint32_t PLCF_type) const;

#ifdef PHY_JSON_SWITCH_IMPLEMENT_ANY_JSON_FUNCTIONALITY
        nlohmann::ordered_json get_json(const uint32_t PLCF_type) const;
#endif

    private:
        const uint32_t PacketLength_max;
        const uint32_t mcs_index_max;
        const uint32_t N_SS_max;

        // type 1
        section3::fec_cfg_t fec_cfg_type1;
        uint32_t HeaderFormat_type1;
        plcf_10_t plcf_10;

        // type 2
        section3::fec_cfg_t fec_cfg_type2;
        uint32_t HeaderFormat_type2;
        plcf_20_t plcf_20;
        plcf_21_t plcf_21;

        /// container with base pointers, must be ordered by header format
        const std::array<plcf_base_t*, 1> array_plcf_type1{&plcf_10};

        /// container with base pointers, must be ordered by header format
        const std::array<plcf_base_t*, 2> array_plcf_type2{&plcf_20, &plcf_21};
};

}  // namespace dectnrp::section4
