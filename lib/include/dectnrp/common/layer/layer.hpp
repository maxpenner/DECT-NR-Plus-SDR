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

#include <concepts>
#include <memory>
#include <string>
#include <vector>

#include "dectnrp/common/layer/layer_unit.hpp"

namespace dectnrp::common {

/**
 * \brief Layer refers to OSI model layer. It is a base class for three children:
 *
 *  1) radio_t          manages hw_t
 *  2) phy_t            manages worker_pool_t
 *  3) upper_t          manages tpoint_t
 *
 * The class upper_t comprises MAC and everything above. All managed types are themselves children
 * of layer_unit_t.
 *
 *  1) hw_t             is a children of layer_unit_t
 *  2) worker_pool_t    is a children of layer_unit_t
 *  3) tpoint_t         is a children of layer_unit_t
 *
 * \tparam T
 */
template <std::derived_from<layer_unit_t> T>
class layer_t {
    public:
        /**
         * \brief Base class for radio, PHY and upper layer. Each layer contains layer units.
         *
         * \param type_ name of layer
         */
        explicit layer_t(const std::string type_)
            : type(type_) {};
        virtual ~layer_t() = default;

        layer_t() = delete;
        layer_t(const layer_t&) = delete;
        layer_t& operator=(const layer_t&) = delete;
        layer_t(layer_t&&) = delete;
        layer_t& operator=(layer_t&&) = delete;

        /**
         * \brief Number of units (hw, worker pool, tpoint) on layer
         *
         * \return number of units
         */
        [[nodiscard]] size_t get_nof_layer_unit() const { return layer_unit_vec.size(); };

        /// reference to derivative of layer unit
        [[nodiscard]] T& get_layer_unit(const size_t idx) const { return *layer_unit_vec[idx]; };

        void shutdown_all_layer_units() {
            for (size_t idx = 0; idx < get_nof_layer_unit(); ++idx) {
                get_layer_unit_base(idx).shutdown();
            }
        };

    private:
        /**
         * \brief Base reference to layer unit, i.e. hw_t, worker_pool_t, tpoint_t.
         *
         * \param idx index of layer unit
         * \return reference to specific layer unit
         */
        [[nodiscard]] layer_unit_t& get_layer_unit_base(const size_t idx) const {
            return *layer_unit_vec[idx];
        };

    protected:
        const std::string type;
        std::vector<std::unique_ptr<T>> layer_unit_vec;
};

}  // namespace dectnrp::common
