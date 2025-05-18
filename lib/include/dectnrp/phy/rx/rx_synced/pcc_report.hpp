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

#include "dectnrp/sections_part4/physical_header_field/plcf_decoder.hpp"

namespace dectnrp::phy {

class pcc_report_t {
    public:
        explicit pcc_report_t(const section4::plcf_decoder_t& plcf_decoder_,
                              const float snr_dB_ = 0.0f)
            : plcf_decoder(plcf_decoder_),
              snr_dB(snr_dB_){};

        /// contains all information about the PCC (or PLCF) available
        const section4::plcf_decoder_t& plcf_decoder;

        /**
         * \brief This SNR value is based on the STF and the first one or two OFDM symbols
         * containing DRS cells. The pdc_report contains another refined SNR value.
         */
        const float snr_dB;
};

}  // namespace dectnrp::phy
