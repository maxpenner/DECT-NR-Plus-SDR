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

#include <memory>

#include "dectnrp/common/json_export.hpp"
#include "dectnrp/common/json_switch.hpp"
#include "dectnrp/phy/interfaces/layers_downwards/phy_radio.hpp"
#include "dectnrp/phy/interfaces/machigh_phy.hpp"
#include "dectnrp/phy/pool/token.hpp"
#include "dectnrp/phy/pool/worker.hpp"
#include "dectnrp/phy/rx/chscan/chscanner.hpp"
#include "dectnrp/phy/rx/rx_synced/rx_synced.hpp"
#include "dectnrp/phy/tx/tx.hpp"
#include "dectnrp/upper/tpoint.hpp"

namespace dectnrp::phy {

class worker_tx_rx_t final : public worker_t {
    public:
        explicit worker_tx_rx_t(worker_config_t& worker_config,
                                phy_radio_t& phy_radio_,
                                common::json_export_t* json_export_);
        ~worker_tx_rx_t() = default;

        worker_tx_rx_t() = delete;
        worker_tx_rx_t(const worker_tx_rx_t&) = delete;
        worker_tx_rx_t& operator=(const worker_tx_rx_t&) = delete;
        worker_tx_rx_t(worker_tx_rx_t&&) = delete;
        worker_tx_rx_t& operator=(worker_tx_rx_t&&) = delete;

        friend class worker_pool_t;

    private:
        void work() override final;
        std::vector<std::string> report_start() const override final;
        std::vector<std::string> report_stop() const override final;

        std::unique_ptr<tx_t> tx;
        std::unique_ptr<rx_synced_t> rx_synced;
        std::unique_ptr<chscanner_t> chscanner;

        upper::tpoint_t* tpoint{nullptr};
        std::shared_ptr<token_t> token;
        uint32_t token_call_id;

        using tx_descriptor_vec_t = machigh_phy_tx_t::tx_descriptor_vec_t;
        using chscan_opt_t = machigh_phy_t::chscan_opt_t;

        void run_tx_chscan(const tx_descriptor_vec_t& tx_descriptor_vec, chscan_opt_t& chscan_opt);

        phy_radio_t& phy_radio;

        // JSON export of information available to worker_tx_rx
        common::json_export_t* json_export;

#ifdef PHY_JSON_SWITCH_IMPLEMENT_ANY_JSON_FUNCTIONALITY
        void collect_and_write_json(const sync_report_t& sync_report,
                                    const phy_maclow_t& phy_maclow,
                                    const maclow_phy_t& maclow_phy);
#endif

        struct stats_t {
                int64_t tx_desc{};
                int64_t tx_desc_other_hw{};

                int64_t tx_fail_no_buffer{};
                int64_t tx_fail_no_buffer_other_hw{};

                int64_t tx_sent{};

                int64_t rx_pcc_success{};
                int64_t rx_pcc_fail{};

                int64_t rx_pdc_success{};
                int64_t rx_pdc_fail{};

                int64_t tpoint_work_regular{};
                int64_t tpoint_work_upper{};
        } stats;
};

}  // namespace dectnrp::phy
