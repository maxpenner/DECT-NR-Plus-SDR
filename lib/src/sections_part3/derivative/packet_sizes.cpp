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

#include "dectnrp/sections_part3/derivative/packet_sizes.hpp"

#include <bit>
#include <cmath>
#include <limits>

#include "dectnrp/common/adt/miscellaneous.hpp"
#include "dectnrp/common/prog/assert.hpp"
#include "dectnrp/constants.hpp"
#include "dectnrp/sections_part3/derivative/duration.hpp"
#include "dectnrp/sections_part3/fix/cbsegm.hpp"
#include "dectnrp/sections_part3/pdc.hpp"
#include "dectnrp/sections_part3/radio_device_class.hpp"
#include "dectnrp/sections_part3/transmission_packet_structure.hpp"
#include "dectnrp/sections_part3/transport_block_size.hpp"

namespace dectnrp::sp3 {

static packet_sizes_t get_maximum_packet_sizes(const radio_device_class_t& radio_device_class) {
    // take maximum values from radio device class for maximum packet size
    packet_sizes_def_t psdef_maximum_sizes;
    psdef_maximum_sizes.u = radio_device_class.u_min;
    psdef_maximum_sizes.b = radio_device_class.b_min;
    psdef_maximum_sizes.PacketLengthType = 1;
    psdef_maximum_sizes.PacketLength = radio_device_class.PacketLength_min;
    psdef_maximum_sizes.tm_mode_index =
        tmmode::get_max_tm_mode_index_depending_on_N_TX(radio_device_class.N_TX_min);
    psdef_maximum_sizes.mcs_index = radio_device_class.mcs_index_min;
    psdef_maximum_sizes.Z = radio_device_class.Z_min;

    const auto q = get_packet_sizes(psdef_maximum_sizes);

    dectnrp_assert(q.has_value(), "unable to determine maximum packet sizes");

    for (uint32_t u_check = 1; u_check < radio_device_class.u_min; u_check *= 2) {
        // change value of u
        psdef_maximum_sizes.u = u_check;

        // try to generate new maximum packet size
        const auto mps_check = get_packet_sizes(psdef_maximum_sizes);

        dectnrp_assert(mps_check.has_value(), "unable to determine maximum packet sizes");

        /**
         * Compare two packet sizes:
         *
         * One would assume that a larger value of u yields larger maximum packet sizes. When we set
         * PacketLengthType=1, this is true for the number of OFDM symbols for a fixed value of
         * PacketLength. However, this is not necessarily true for the number bits in the transport
         * block, since a larger value of u also increases the number of zero symbols at the end of
         * a packet.
         *
         * For a given values of u_max, this function is used to iterate over each smaller value of
         * u_min to make sure the allocation for u_max covers all smaller configurations.
         */
        dectnrp_assert(q->N_PACKET_symb >= mps_check->N_PACKET_symb, "");
        dectnrp_assert(q->N_PDC_subc >= mps_check->N_PDC_subc, "");
        dectnrp_assert(q->G >= mps_check->G, "");
        dectnrp_assert(q->N_PDC_bits >= mps_check->N_PDC_bits, "");
        dectnrp_assert(q->N_TB_bits >= mps_check->N_TB_bits, "");
        dectnrp_assert(q->N_TB_byte >= mps_check->N_TB_byte, "");
        dectnrp_assert(q->C >= mps_check->C, "");
        dectnrp_assert(q->N_DF_symb >= mps_check->N_DF_symb, "");
        dectnrp_assert(q->N_DRS_subc >= mps_check->N_DRS_subc, "");
        dectnrp_assert(q->N_samples_OFDM_symbol >= mps_check->N_samples_OFDM_symbol, "");
        dectnrp_assert(q->N_samples_STF >= mps_check->N_samples_STF, "");
        dectnrp_assert(q->N_samples_STF_CP_only >= mps_check->N_samples_STF_CP_only, "");
        dectnrp_assert(q->N_samples_DF >= mps_check->N_samples_DF, "");
        dectnrp_assert(q->N_samples_GI >= mps_check->N_samples_GI, "");
        dectnrp_assert(q->N_samples_packet_no_GI >= mps_check->N_samples_packet_no_GI, "");
        dectnrp_assert(q->N_samples_packet >= mps_check->N_samples_packet, "");
    }

    return q.value();
}

packet_sizes_opt_t get_packet_sizes(const packet_sizes_def_t& qq) {
    // return value
    packet_sizes_t q;

    const uint32_t u = qq.u;
    const uint32_t b = qq.b;
    const uint32_t PacketLengthType = qq.PacketLengthType;
    const uint32_t PacketLength = qq.PacketLength;
    const uint32_t tm_mode_index = qq.tm_mode_index;
    const uint32_t mcs_index = qq.mcs_index;
    const uint32_t Z = qq.Z;

    dectnrp_assert(std::has_single_bit(u) && u <= 8, "u undefined");
    dectnrp_assert((std::has_single_bit(b) && b <= 16) || b == 12, "b undefined");
    dectnrp_assert(PacketLengthType <= 1, "PacketLengthType undefined");
    dectnrp_assert(PacketLength > 0 && PacketLength <= 16, "PacketLength undefined");
    dectnrp_assert(tm_mode_index <= 11, "tm_mode_index undefined");
    dectnrp_assert(mcs_index <= 11, "mcs_index undefined");
    dectnrp_assert(Z == 2048 || Z == 6144, "mcs_index undefined");

    // the following calculations depend only on the variables listed above

    const auto numerology = get_numerologies(u, b);

    const uint32_t N_PACKET_symb = transmission_packet_structure::get_N_PACKET_symb(
        PacketLengthType, PacketLength, numerology.N_SLOT_u_symb, numerology.N_SLOT_u_subslot);

    dectnrp_assert(N_PACKET_symb >= 5 && N_PACKET_symb <= 1280 && N_PACKET_symb % 5 == 0,
                   "N_PACKET_symb ill-defined");

    const auto tm_mode = tmmode::get_tm_mode(tm_mode_index);
    const uint32_t N_eff_TX = tm_mode.N_eff_TX;

    // see 5.1 Transmission packet structure
    if (N_eff_TX == 4 && N_PACKET_symb < 15) {
        return std::nullopt;
    }

    /* When u==8, we have three zero symbols for GI at the end of the packet. When N_PACKET_symb is
     * 15, this would imply that OFDM symbols with indices 12, 13 and 14 (+ N_step) are zero.
     * However, with 8 transmit streams, we also have DRS cells in OFDM symbol indices 11 and 12.
     * Thus, there is a collision at symbol 12 and this combination is not possible.
     *
     * As a solution, N_PACKET_symb must be at least 20 and a multiple of 10.
     */
    if (u == 8 && N_eff_TX == 8 && (N_PACKET_symb < 20 || N_PACKET_symb % 10 != 0)) {
        return std::nullopt;
    }

    /* When we have reached this point, we know that our packet can accommodate all DRS symbols.
     * However, depending on the combination of N_PACKET_symb, u, N_eff_TX and numerology.N_b_OCC,
     * the packet configuration can still fail because we can't accommodate all 98 PCC cells, or we
     * can but there is no space left for a single PDC cell.
     */
    const uint32_t N_PDC_subc =
        pdc_t::get_N_PDC_subc(N_PACKET_symb, u, N_eff_TX, numerology.N_b_OCC);

    if (N_PDC_subc == 0) {
        return std::nullopt;
    }

    const auto mcs = get_mcs(mcs_index);

    const uint32_t N_SS = tm_mode.N_SS;

    /* When we have reached this point, we know that our packet can accommodate all DRS symbols, all
     * PCC symbols and at least one PDC cell. How many bits can we then accommodate?
     */
    const uint32_t N_TB_bits = transport_block_size::get_N_TB_bits(
        N_SS, N_PDC_subc, mcs.N_bps, mcs.R_numerator, mcs.R_denominator, Z);

    if (N_TB_bits == 0) {
        return std::nullopt;
    }

    q.psdef = qq;

    q.numerology = numerology;
    q.mcs = mcs;
    q.tm_mode = tm_mode;

    q.N_PACKET_symb = N_PACKET_symb;
    q.N_PDC_subc = N_PDC_subc;
    q.G = transport_block_size::get_G(N_SS, N_PDC_subc, mcs.N_bps);
    q.N_PDC_bits = transport_block_size::get_N_PDC_bits(
        N_SS, N_PDC_subc, mcs.N_bps, mcs.R_numerator, mcs.R_denominator);
    q.N_TB_bits = N_TB_bits;
    q.N_TB_byte = common::adt::ceil_divide_integer(N_TB_bits, 8U);

    /* When we reached this point, we know that our packet can accommodate all DRS symbols, all PCC
     * symbols and at least one PDC cell. We also know the exact amount of bits in the transport
     * block.
     *
     * However, there is an error in the standard: 5.3: "With this definition of tbs the number of
     * filler bits in clause 6.1.3 is always 0." That is not correct as there are combinations where
     * N_TB_bits = 8. Then B is 8+L=8+24=32 and therefore smaller 40. Filler bits become necessary.
     * Though, no one is gonna use N_TB_bits = 8.
     */
    srsran_cbsegm_t cb_segm;
    if (fix::srsran_cbsegm_FIX(&cb_segm, q.N_TB_bits, Z) == SRSRAN_ERROR) {
        dectnrp_assert_failure("Error computing Codeword Segmentation for TBS");
        return std::nullopt;
    }

    if (cb_segm.F > 0) {
        // dectnrp_assert_failure("filler bits are not supported.");
        return std::nullopt;
    }

    q.C = cb_segm.C;

    q.N_DF_symb = pdc_t::get_N_DF_symb(u, N_PACKET_symb);

    q.N_DRS_subc = pdc_t::get_N_DRS_subc(u, N_PACKET_symb, tm_mode.N_eff_TX, numerology.N_b_OCC);

    q.N_samples_OFDM_symbol = transmission_packet_structure::get_N_samples_OFDM_symbol(b);
    q.N_samples_STF = transmission_packet_structure::get_N_samples_STF(u, b);
    q.N_samples_STF_CP_only = transmission_packet_structure::get_N_samples_STF_CP_only(u, b);
    q.N_samples_DF = transmission_packet_structure::get_N_samples_DF(u, b, N_PACKET_symb);
    q.N_samples_GI = transmission_packet_structure::get_N_samples_GI(u, b);
    q.N_samples_packet_no_GI = q.N_samples_STF + q.N_samples_DF;
    q.N_samples_packet = q.N_samples_packet_no_GI + q.N_samples_GI;

    dectnrp_assert(q.N_samples_packet ==
                       q.N_samples_STF_CP_only + 64 * q.psdef.b + q.N_samples_DF + q.N_samples_GI,
                   "Incorrect number of samples for STF, DF and GI.");
    dectnrp_assert(q.N_samples_packet == q.N_samples_OFDM_symbol * q.N_PACKET_symb,
                   "Incorrect number of samples for STF, DF and GI.");

    return q;
}

packet_sizes_t get_maximum_packet_sizes(const std::string& radio_device_class_string) {
    const auto radio_device_class = get_radio_device_class(radio_device_class_string);
    return get_maximum_packet_sizes(radio_device_class);
}

uint32_t get_N_samples_in_packet_length(const packet_sizes_t& packet_sizes,
                                        const uint32_t samp_rate) {
    const uint64_t A = static_cast<uint64_t>(packet_sizes.N_samples_packet);

    const uint64_t B = static_cast<uint64_t>(packet_sizes.psdef.u * packet_sizes.psdef.b *
                                             constants::samp_rate_min_u_b);

    const uint64_t C = static_cast<uint64_t>(samp_rate);

    const uint32_t ret =
        static_cast<uint32_t>(common::adt::ceil_divide_integer<uint64_t>(A * C, B));

    dectnrp_assert(packet_sizes.N_samples_packet <= ret, "packet too short");
    dectnrp_assert(ret <= std::numeric_limits<uint32_t>::max(), "packet too long");

    return ret;
}

uint32_t get_N_samples_in_packet_length_max(const packet_sizes_t& packet_sizes,
                                            const uint32_t samp_rate) {
    // Slots are longer than subslots, so when looking for the maximum duration of a packet in
    // seconds, we need the maximum number of slots.
    return packet_sizes.psdef.PacketLength *
           static_cast<uint32_t>(duration_t(samp_rate, duration_ec_t::slot001).get_N_samples());
}

packet_sizes_t get_random_packet_sizes_within_rdc(const std::string& radio_device_class_string,
                                                  common::randomgen_t& randomgen) {
    // get radio device parameters
    const auto radio_device_class = get_radio_device_class(radio_device_class_string);

    // this structure will describe a packet within the boundaries of the radio device class
    packet_sizes_def_t psdef;

    //                                      1  2     4           8          12          16
    static const uint32_t val2idx[17] = {0, 0, 1, 0, 2, 0, 0, 0, 3, 0, 0, 0, 4, 0, 0, 0, 5};
    static const uint32_t idx2val[6] = {1, 2, 4, 8, 12, 16};

    packet_sizes_opt_t q;

    // set random parameters and check if packet can be generated
    do {
        const uint32_t u_idx_max = val2idx[radio_device_class.u_min];
        const uint32_t u_random = idx2val[randomgen.randi(0, u_idx_max)];

        const uint32_t b_idx_max = val2idx[radio_device_class.b_min];
        const uint32_t b_random = idx2val[randomgen.randi(0, b_idx_max)];

        const uint32_t PacketLengthType_random = randomgen.randi(0, 1);

        const uint32_t PacketLength_random =
            randomgen.randi(1, radio_device_class.PacketLength_min);

        const uint32_t tm_mode_index_random = randomgen.randi(
            0, tmmode::get_max_tm_mode_index_depending_on_N_TX(radio_device_class.N_TX_min));

        const uint32_t mcs_index_random = randomgen.randi(0, radio_device_class.mcs_index_min);

        // OPTION A: Z_random is not random but fixed to hardware device capability.
        const uint32_t Z_random = radio_device_class.Z_min;

        // OPTION B: Z_random is in fact random and only limited by the device
        /*
        uint32_t Z_random = (randomgen.randi(0, 1) == 0) ? 2048 : 6144;
        if(radio_device_class.Z_min < Z_random){
            Z_random = radio_device_class.Z_min;
        }
         */

        // set struct
        psdef.u = u_random;
        psdef.b = b_random;
        psdef.PacketLengthType = PacketLengthType_random;
        psdef.PacketLength = PacketLength_random;
        psdef.tm_mode_index = tm_mode_index_random;
        psdef.mcs_index = mcs_index_random;
        psdef.Z = Z_random;

        q = get_packet_sizes(psdef);
    } while (!q.has_value());

    return q.value();
}

}  // namespace dectnrp::sp3
