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
#include "srsran/phy/common/sequence.h"
#include "srsran/phy/fec/cbsegm.h"
}

#include "dectnrp/phy/fec/pcc_enc.hpp"
#include "dectnrp/phy/fec/pdc_enc.hpp"
#include "dectnrp/phy/harq/buffer_rx.hpp"
#include "dectnrp/phy/harq/buffer_rx_plcf.hpp"
#include "dectnrp/phy/harq/buffer_tx.hpp"
#include "dectnrp/sections_part3/derivative/fec_cfg.hpp"
#include "dectnrp/sections_part3/derivative/packet_sizes.hpp"
#include "dectnrp/sections_part3/scrambling_pdc.hpp"

namespace dectnrp::phy {

class fec_t {
    public:
        /**
         * \brief This class acts as a state machine facilitating the encoding and decoding of
         * DECTNRP packets with the channel coding functionality provided by srsran. It comprises
         * segmentation, scrambling and the turbo encoding and decoding of transport blocks for
         * either PCC or PDC. For turbo encoding and decoding, fec_t internally uses functionality
         * from pcc_enc.h and pcd_enc.h, which are variations of srsran functions from
         * https://github.com/srsran/srsRAN_4G/blob/master/lib/src/phy/phch/sch.c.
         *
         * The major changes compared to the srsran functions are codeblock-wise encoding/decoding,
         * and DECTNRP specific additions such as scrambling (based on network ID), CRC masking and
         * the number of soft bits at the receiver.
         *
         * \param packet_sizes_maximum maximum packet sizes across the radio device class
         */
        explicit fec_t(const section3::packet_sizes_t& packet_sizes_maximum);
        ~fec_t();

        fec_t() = delete;
        fec_t(const fec_t&) = delete;
        fec_t& operator=(const fec_t&) = delete;
        fec_t(fec_t&&) = delete;
        fec_t& operator=(fec_t&&) = delete;

        /**
         * \brief network IDs must be precalculated, otherwise timing can hiccup
         *
         * \param network_id 32-bit version
         */
        void add_new_network_id(const uint32_t network_id);

        // ##################################################
        // PLCF

        /**
         * \brief Encode in a single step. The structure tx_cfg contains the type and the required
         * configuration for masking the CRC, while hb contains the actual bits (a bits) to encode,
         * the intermediate softbuffer and the target buffer (d bits).
         *
         * \param tx_cfg FEC configuration
         * \param hb HARQ buffer
         */
        void encode_plcf(const section3::fec_cfg_t& tx_cfg, harq::buffer_tx_t& hb);

        /**
         * \brief Quote form 7.5.1 in part 3: "The receiver shall blind decode both transport block
         * sizes and select the one with a CRC match." Here, transport block sizes refers to either
         * PLCF type 1 or type 2.
         *
         * Unfortunately, making a decision depending solely on the CRC is ambiguous. There are rare
         * cases where we receive a PLCF type 1, but trying to decode it as a type 2 still returns a
         * correct CRC. Same can happen when we receive a PLCF type 2, but decode it as a type 1.
         * For this reason, it is essential to always test both types. We can then have no, one or
         * even two correct CRCs. When we have two correct CRCs, the receiver has to make a decision
         * depending on the actual content of either type 1 or type 2, i.e. which values make more
         * sense and stay within the limits defined by the radio device class.
         *
         * To reflect this fact, the name of the function contains the word "test" - we test
         * PLCF_type_test=1 or PLCF_type_test=2, the function decodes the corresponding number of
         * bits and returns whether the CRC is correct or not.
         *
         * \param rx_cfg FEC configuration
         * \param hb HARQ buffer
         * \param PLCF_type_test either 1 or 2
         * \return
         */
        bool decode_plcf_test(section3::fec_cfg_t& rx_cfg,
                              harq::buffer_rx_plcf_t& hb,
                              const uint32_t PLCF_type_test);

        // ##################################################
        // Transport Block

        /**
         * \brief must be called each time before encoding or decoding a new packet
         *
         * \param tx_cfg
         */
        void segmentate_and_pick_scrambling_sequence(const section3::fec_cfg_t& tx_cfg);

        /**
         * \brief encode TB to PDC in a single step
         *
         * \param tx_cfg
         * \param hb
         * \return
         */
        void encode_tb(const section3::fec_cfg_t& tx_cfg, harq::buffer_tx_t& hb);

        /**
         * \brief Encode TB to PDC in multiple steps. Encode the minimum number of codeblocks
         * required until at least nof_bits_minimum d bits of the PDC are produced. Can be called
         * multiple times for a single transport block. Helps reducing the initial TX delay in case
         * a transport block is segmented into more than one codeblock.
         *
         * \param tx_cfg
         * \param hb
         * \param nof_bits_minimum
         * \return
         */
        void encode_tb(const section3::fec_cfg_t& tx_cfg,
                       harq::buffer_tx_t& hb,
                       const uint32_t nof_bits_minimum);

        /**
         * \brief decode PDC to TB in a single step
         *
         * \param rx_cfg
         * \param hb
         */
        void decode_tb(const section3::fec_cfg_t& rx_cfg, harq::buffer_rx_t& hb);

        /**
         * \brief Decode PDC to TB in multiple steps. Decode the maximum number of codeblocks
         * possible with nof_bits_minimum d bits of the PDC being available. Can be called multiple
         * times for a single transport block. Helps reducing the RX delay in case a transport block
         * is segmented into more than one codeblock.
         *
         * \param rx_cfg
         * \param hb
         * \param nof_bits_maximum
         */
        void decode_tb(const section3::fec_cfg_t& rx_cfg,
                       harq::buffer_rx_t& hb,
                       const uint32_t nof_bits_maximum);

        /// poll latest status of TB decoding
        bool get_decode_tb_status_latest() const { return decode_tb_status_latest; };

        /**
         * \brief get write pointer to check if final pointer value is correct
         *
         * \return wp
         */
        uint32_t get_wp() const { return wp; };

    private:
        pcc_enc_t pcc_enc;
        pdc_enc_t pdc_enc;

        /// container for known scrambling sequences, depend on network IDs
        section3::scrambling_pdc_t scrambling_pdc;

        // state machine variables

        /// valid for current packet, set in segmentate_and_pick_scrambling_sequence()
        srsran_cbsegm_t srsran_cbsegm;
        srsran_sequence_t* srsran_sequence;

        /**
         * \brief Variables for encoding and decoding across multiple codeblocks, get reset in
         * segmentate_and_pick_scrambling_sequence().
         *
         * cb_idx:  code block index
         * rp:      read pointer
         * wp:      write pointer
         */
        uint32_t cb_idx, rp, wp;

        /// last decoding status after having processed entire transport block
        bool decode_tb_status_latest;
};

}  // namespace dectnrp::phy
