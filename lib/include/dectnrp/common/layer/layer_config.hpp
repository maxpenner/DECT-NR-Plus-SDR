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

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "dectnrp/common/json_parse.hpp"
#include "dectnrp/common/prog/assert.hpp"
#include "dectnrp/external/nlohmann/json.hpp"

namespace dectnrp::common {

template <typename T>
class layer_config_t {
    public:
        layer_config_t() = default;

        /**
         * \brief Base class for derived configurations on radio, PHY and upper layer.
         *
         * \param directory path for folder containing all configurations files
         * \param filename file name in folder path
         */
        explicit layer_config_t(const std::string directory, const std::string filename)
            : filepath(directory + filename),
              json_parsed(jsonparse::parse(filepath)) {}

        std::size_t get_nof_layer_unit_config() const { return layer_unit_config_vec.size(); };

        const T& get_layer_unit_config(const std::size_t id) const {
            return layer_unit_config_vec[id];
        };

    protected:
        std::string filepath;
        nlohmann::ordered_json json_parsed;

        std::vector<T> layer_unit_config_vec;
};

}  // namespace dectnrp::common
