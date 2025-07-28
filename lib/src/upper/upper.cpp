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

#include "dectnrp/upper/upper.hpp"

#include "dectnrp/common/prog/assert.hpp"
#include "dectnrp/limits.hpp"
#include "dectnrp/phy/interfaces/layers_downwards/mac_lower.hpp"
#include "dectnrp/phy/pool/token.hpp"
#include "dectnrp/upper/basic/tfw_basic.hpp"
#include "dectnrp/upper/chscanner/tfw_chscanner.hpp"
#include "dectnrp/upper/loopback/tfw_loopback_mmie.hpp"
#include "dectnrp/upper/loopback/tfw_loopback_ratio.hpp"
#include "dectnrp/upper/loopback/tfw_loopback_snr.hpp"
#include "dectnrp/upper/p2p/tfw_p2p_ft.hpp"
#include "dectnrp/upper/p2p/tfw_p2p_pt.hpp"
#include "dectnrp/upper/rtt/tfw_rtt.hpp"
#include "dectnrp/upper/txrxagc/tfw_txrxagc.hpp"
#include "dectnrp/upper/txrxdelay/tfw_txrxdelay.hpp"

// more readable
#define TFW_NAME_STARTS_WITH(x) tpoint_config.firmware_name.starts_with(x)
#define TFW_ARGS tpoint_config, mac_lower

namespace dectnrp::upper {

upper_t::upper_t(const upper_config_t& upper_config_,
                 const radio::radio_t& radio_,
                 const phy::phy_t& phy_)
    : layer_t("UPPER"),
      upper_config(upper_config_) {
    dectnrp_assert(upper_config.get_nof_layer_unit_config() >= 1,
                   "at least one layer_unit required");

    /* Each hw on the radio layer is always paired with exactly one worker_pool on the PHY which
     * processes streamed samples. For the tpoint on the upper layers, however, we have two options:
     *
     * Option A) For all pairs of hw + worker_pool, there is exactly one tpoint.
     *
     *      N hw + worker_pool pairs        N tpoint
     *              2                           1
     *              3                           1
     *              4                           1
     */
    // option A
    if ((phy_.get_nof_layer_unit() > 1) && (upper_config.get_nof_layer_unit_config() == 1)) {
        init_one_to_many(radio_, phy_);
    }
    /*
     * Option B) For each pair of hw + worker_pool, there is exactly one tpoint.
     *
     *      N hw + worker_pool pairs        N tpoint
     *              1                           1
     *              2                           2
     *              3                           3
     *              4                           4
     */
    else {
        init_one_to_one(radio_, phy_);
    }
}

void upper_t::add_tpoint(const tpoint_config_t& tpoint_config, phy::mac_lower_t& mac_lower) {
    /* New firmware must be added here by creating a new "else if" case. Every firmware must
     * have a static std::string member called firmware_name. The value of firmware_name must be
     * unique.
     */
    if (TFW_NAME_STARTS_WITH(tfw::basic::tfw_basic_t::firmware_name)) {
        layer_unit_vec.push_back(std::make_unique<tfw::basic::tfw_basic_t>(TFW_ARGS));

    } else if (TFW_NAME_STARTS_WITH(tfw::chscanner::tfw_chscanner_t::firmware_name)) {
        layer_unit_vec.push_back(std::make_unique<tfw::chscanner::tfw_chscanner_t>(TFW_ARGS));

    } else if (TFW_NAME_STARTS_WITH(tfw::loopback::tfw_loopback_mmie_t::firmware_name)) {
        layer_unit_vec.push_back(std::make_unique<tfw::loopback::tfw_loopback_mmie_t>(TFW_ARGS));

    } else if (TFW_NAME_STARTS_WITH(tfw::loopback::tfw_loopback_ratio_t::firmware_name)) {
        layer_unit_vec.push_back(std::make_unique<tfw::loopback::tfw_loopback_ratio_t>(TFW_ARGS));

    } else if (TFW_NAME_STARTS_WITH(tfw::loopback::tfw_loopback_snr_t::firmware_name)) {
        layer_unit_vec.push_back(std::make_unique<tfw::loopback::tfw_loopback_snr_t>(TFW_ARGS));

    } else if (TFW_NAME_STARTS_WITH(tfw::p2p::tfw_p2p_ft_t::firmware_name)) {
        layer_unit_vec.push_back(std::make_unique<tfw::p2p::tfw_p2p_ft_t>(TFW_ARGS));

    } else if (TFW_NAME_STARTS_WITH(tfw::p2p::tfw_p2p_pt_t::firmware_name)) {
        layer_unit_vec.push_back(std::make_unique<tfw::p2p::tfw_p2p_pt_t>(TFW_ARGS));

    } else if (TFW_NAME_STARTS_WITH(tfw::rtt::tfw_rtt_t::firmware_name)) {
        layer_unit_vec.push_back(std::make_unique<tfw::rtt::tfw_rtt_t>(TFW_ARGS));

    } else if (TFW_NAME_STARTS_WITH(tfw::txrxagc::tfw_txrxagc_t::firmware_name)) {
        layer_unit_vec.push_back(std::make_unique<tfw::txrxagc::tfw_txrxagc_t>(TFW_ARGS));

    } else if (TFW_NAME_STARTS_WITH(tfw::txrxdelay::tfw_txrxdelay_t::firmware_name)) {
        layer_unit_vec.push_back(std::make_unique<tfw::txrxdelay::tfw_txrxdelay_t>(TFW_ARGS));

    } else {
        dectnrp_assert_failure("unknown tpoint firmware name {}", tpoint_config.firmware_name);
    }
}

void upper_t::init_one_to_many(const radio::radio_t& radio_, const phy::phy_t& phy_) {
    dectnrp_assert(!radio_.is_simulation(), "currently not possible in simulation");
    dectnrp_assert(phy_.get_nof_layer_unit() <= limits::max_nof_radio_phy_pairs_one_tpoint,
                   "too many pairs for a single tpoint");

    // all hw + worker_pool pairs use the same token
    auto token = phy::token_t::create(phy_.get_nof_layer_unit());

    mac_lower_vec.push_back(phy::mac_lower_t(token));

    // tpoint controls alls hw + worker_pool pairs through this interface
    phy::mac_lower_t& mac_lower = mac_lower_vec.at(0);

    // vector with all hw + worker_pool pairs
    for (uint32_t id = 0; id < phy_.get_nof_layer_unit(); ++id) {
        mac_lower.lower_ctrl_vec.emplace_back(radio_.get_layer_unit(id),
                                              phy_.phy_config.get_layer_unit_config(id),
                                              phy_.get_layer_unit(id).get_job_queue());
    }

    // load configuration of this tpoint
    const auto& tpoint_config = upper_config.get_layer_unit_config(0);

    dectnrp_assert(tpoint_config.id == 0, "incorrect ID");

    add_tpoint(tpoint_config, mac_lower);

    // give necessary data to worker_pool_t instances
    for (uint32_t id = 0; id < phy_.get_nof_layer_unit(); ++id) {
        /* We give every worker pool a pointer to the same single tpoint we have. They also all get
         * a shared pointer to the same token. However, they have to call the token with different
         * IDs.
         */
        auto& worker_pool = phy_.get_layer_unit(id);
        worker_pool.configure_tpoint_calls(layer_unit_vec.at(0).get(), mac_lower.token, id);

        // add network IDs to associated worker pool
        for (auto elem : tpoint_config.network_ids) {
            worker_pool.add_network_id(elem);
        }
    }

    dectnrp_assert(
        layer_unit_vec.size() == 1, "must be exactly one tpoint {}", layer_unit_vec.size());
}

void upper_t::init_one_to_one(const radio::radio_t& radio_, const phy::phy_t& phy_) {
    dectnrp_assert(phy_.get_nof_layer_unit() == upper_config.get_nof_layer_unit_config(),
                   "number of hw, worker_pool and tpoint ill-configured");
    /* Init mac_lower_vec, we need reference to its elements. If we push back and then get the
     * reference, they may be invalidated when pushing back again.
     */
    for (uint32_t t_id = 0; t_id < upper_config.get_nof_layer_unit_config(); ++t_id) {
        // all hw + worker_pool pairs use a different token
        auto token = phy::token_t::create(phy_.get_nof_layer_unit());

        mac_lower_vec.push_back(phy::mac_lower_t(token));
    }

    // all hw + worker_pool pairs get a pointer to a unique tpoint
    for (uint32_t t_id = 0; t_id < upper_config.get_nof_layer_unit_config(); ++t_id) {
        // tpoint controls this hw + worker_pool pair through this interface
        phy::mac_lower_t& mac_lower = mac_lower_vec.at(t_id);

        // vector with a single hw + worker_pool pair given to tpoint
        mac_lower.lower_ctrl_vec.emplace_back(radio_.get_layer_unit(t_id),
                                              phy_.phy_config.get_layer_unit_config(t_id),
                                              phy_.get_layer_unit(t_id).get_job_queue());

        // load configuration of this tpoint
        const auto& tpoint_config = upper_config.get_layer_unit_config(t_id);

        dectnrp_assert(tpoint_config.id == t_id, "incorrect ID");

        add_tpoint(tpoint_config, mac_lower);

        /* We give every worker pool a pointer to a unique tpoint. They also all get a new shared
         * pointer to a new token. However, they have to call the token with the same ID within
         * their respective pool.
         */
        auto& worker_pool = phy_.get_layer_unit(t_id);
        worker_pool.configure_tpoint_calls(layer_unit_vec.at(t_id).get(), mac_lower.token, 0);

        // add network IDs to associated worker pool
        for (auto elem : tpoint_config.network_ids) {
            worker_pool.add_network_id(elem);
        }
    }

    dectnrp_assert(layer_unit_vec.size() == phy_.get_nof_layer_unit(),
                   "incorrect number of tpoint instances");
}

}  // namespace dectnrp::upper
