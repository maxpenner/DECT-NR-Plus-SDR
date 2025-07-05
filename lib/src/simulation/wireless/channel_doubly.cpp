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

#include "dectnrp/simulation/wireless/channel_doubly.hpp"

extern "C" {
#include "srsran/phy/utils/vector.h"
}

#include "dectnrp/common/prog/assert.hpp"

namespace dectnrp::simulation {

const std::string channel_doubly_t::name = "doubly";
const std::string channel_doubly_t::delimiter = "_";

channel_doubly_t::channel_doubly_t(const uint32_t id_0_,
                                   const uint32_t id_1_,
                                   const uint32_t samp_rate_,
                                   const uint32_t spp_size_,
                                   const uint32_t nof_antennas_0_,
                                   const uint32_t nof_antennas_1_,
                                   const std::string sim_channel_name_intxx_)
    : channel_t(id_0_, id_1_, samp_rate_, spp_size_),
      nof_antennas_0(nof_antennas_0_),
      nof_antennas_1(nof_antennas_1_) {
    // init all links ...
    parse_and_init_links(sim_channel_name_intxx_);

    // ... and initialize with random values
    randomize_small_scale();

    // make all links print
    const std::string prefix =
        "id_0=" + std::to_string(id_0) + " and id_1=" + std::to_string(id_1) + ": ";
    for (auto& elem : link_vec) {
        elem->print_pdp(prefix);
    }
}

void channel_doubly_t::superimpose(const vspptx_t& vspptx,
                                   vspprx_t& vspprx,
                                   const vspptx_t& vspptx_other) const {
    dectnrp_assert(are_args_valid(vspptx_other, vspprx), "Incorrect two devices");

    // a single link has two directions, is this the primary direction?
    const bool primary_direction = vspptx_other.id < vspprx.id;

    // superimpose every TX antenna ...
    for (uint32_t tx_idx = 0; tx_idx < vspptx_other.nof_antennas; ++tx_idx) {
        // ... onto every RX antenna
        for (uint32_t rx_idx = 0; rx_idx < vspprx.nof_antennas; ++rx_idx) {
            // large scale also includes RX sensitivity
            const float large_scale =
                get_large_scale_via_pathloss(vspptx, vspprx, vspptx_other, tx_idx, rx_idx);

            // apply large scale fading
            srsran_vec_sc_prod_cfc(
                vspptx_other.spp[tx_idx], large_scale, large_scale_stage, vspprx.spp_size);

            // apply small scale fading
            link_vec[tx_idx * vspprx.nof_antennas + rx_idx]->pass_through_link(
                large_scale_stage, small_scale_stage, primary_direction, vspptx.meta.now_64);

            // apply superposition
            srsran_vec_sum_ccc(
                small_scale_stage, vspprx.spp[rx_idx], vspprx.spp[rx_idx], vspprx.spp_size);
        }
    }
}

void channel_doubly_t::randomize_small_scale() {
    // (re-)randomize every single link
    for (auto& elem : link_vec) {
        elem->randomize();
    }
}

void channel_doubly_t::parse_and_init_links(std::string arg) {
    // elements to be extracted
    uint32_t pdp_idx = 0;
    float tau_rms_ns = -1.0f;
    float fD_Hz = -1.0f;

    // split at delimiter
    size_t pos_element = 0;
    size_t pos_string = 0;
    while ((pos_string = arg.find(delimiter)) != std::string::npos) {
        // read element
        const std::string elem = arg.substr(0, pos_string);

        // remove element and delimiter
        arg.erase(0, pos_string + delimiter.length());

        // interpret element
        switch (pos_element) {
            case 1:
                {
                    pdp_idx = stoi(elem);
                    break;
                }
            case 2:
                {
                    tau_rms_ns = stof(elem);
                    fD_Hz = stof(arg);
                    break;
                }
        }

        ++pos_element;
    }

    // initialize all links with the same channel properties
    for (uint32_t i = 0; i < nof_antennas_0 * nof_antennas_1; ++i) {
        link_vec.push_back(
            std::make_unique<link_t>(samp_rate, spp_size, pdp_idx, tau_rms_ns, fD_Hz));
    }
}

}  // namespace dectnrp::simulation
