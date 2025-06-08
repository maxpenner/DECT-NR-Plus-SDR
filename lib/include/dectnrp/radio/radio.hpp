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

#include "dectnrp/common/layer/layer.hpp"
#include "dectnrp/radio/hw.hpp"
#include "dectnrp/radio/radio_config.hpp"
#include "dectnrp/simulation/vspace.hpp"

namespace dectnrp::radio {

class radio_t final : public common::layer_t<hw_t> {
    public:
        explicit radio_t(const radio_config_t& radio_config_);
        ~radio_t() = default;

        radio_t() = delete;
        radio_t(const radio_t&) = delete;
        radio_t& operator=(const radio_t&) = delete;
        radio_t(radio_t&&) = delete;
        radio_t& operator=(radio_t&&) = delete;

        /// higher layers need to know whether devices are real or part of a simulation
        [[nodiscard]] bool is_simulation() const { return vspace.get() != nullptr; };

        void start_threads_of_all_layer_units();

        const radio_config_t& radio_config;

    private:
        /**
         * \brief The radio layer can contain either only real hardware devices (USRP, BladeRF
         * etc.), or only virtual hardware devices (simulator). These types cannot be mixed. When
         * all hw is virtual, we run a simulation in a virtual space which connects all virtual
         * devices with each other. Only in this case is the vspace initialized.
         */
        void is_vspace_valid_if_so_init();
        std::unique_ptr<simulation::vspace_t> vspace;
};

}  // namespace dectnrp::radio
