/*
 * Copyright 2023-present Maxim Penner
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

extern "C" {
#include "srsran/phy/fec/softbuffer.h"
}

#include "dectnrp/phy/harq/buffer.hpp"

namespace dectnrp::phy::harq {

class buffer_tx_t final : public buffer_t {
    public:
        /// HARQ buffers for PLCF and TB have different maximum sizes
        enum class COMPONENT_T {
            PLCF,
            TRANSPORT_BLOCK
        };

        /// PLCF
        explicit buffer_tx_t(const COMPONENT_T component);

        /**
         * \brief Theory of operation:
         *
         * The MAC layer is given MSDUs from the higher layers, adds its own headers and by that
         * generates MPDUs. These MPDUs are consecutively written into the buffer represented by a*.
         * The size of the buffer a* is pointing to is limited by the radio device class.
         *
         * Before channel coding the sequence of MPDUs, the softbuffer must be reset with the
         * corresponding functions.
         *
         * Channel coding then must be started with rv=0.
         *
         * When rv=0:
         *
         * 1. Classes pcc_enc and pdc_enc of FEC calculate and store systematic and parity bits of
         * a* in softbuffer_d.
         *
         * 2. Systematic and parity bits are then given to the rate matching which calculates the
         * circular w buffer.
         *
         * 3. Rate matching also calculates the output bits d (srsran calls them e_bits) by bit
         * selection and pruning.
         *
         * 4. The result of channel coding with rv=0 is then written to d*.
         *
         * When rv>0:
         *
         * 1. Recalculating systematic and parity bits is skipped, the internal memory of pcc_enc
         * and pdc_enc is irrelevant and only softbuffer_d is reused.
         *
         * 2. Recalculating the w buffer is also skipped.
         *
         * 3. Only the rate matching is applied. It takes the w buffer and directly calculates new
         * bits d.
         *
         * 4. The result of channel coding with rv>0 is then written to d*.
         *
         * \param component
         * \param N_TB_byte_max
         * \param G_max
         * \param C_max
         * \param Z
         */
        explicit buffer_tx_t(const COMPONENT_T component,
                             const uint32_t N_TB_byte_max,
                             const uint32_t G_max,
                             const uint32_t C_max,
                             const uint32_t Z);
        ~buffer_tx_t();

        buffer_tx_t() = delete;
        buffer_tx_t(const buffer_tx_t&) = delete;
        buffer_tx_t& operator=(const buffer_tx_t&) = delete;
        buffer_tx_t(buffer_tx_t&&) = delete;
        buffer_tx_t& operator=(buffer_tx_t&&) = delete;

        /// call to apply channel coding to a bits and yield d bits
        srsran_softbuffer_tx_t* get_softbuffer_d() { return &softbuffer_d; }

        /// call to clear the softbuffer for new encoding process
        void reset_a_cnt_and_softbuffer() override final;
        void reset_a_cnt_and_softbuffer(const uint32_t nof_cb) override final;

    private:
        srsran_softbuffer_tx_t softbuffer_d;
};

}  // namespace dectnrp::phy::harq
