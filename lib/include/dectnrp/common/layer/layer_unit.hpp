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

#include <string>
#include <vector>

namespace dectnrp::common {

class layer_unit_t {
    public:
        /**
         * \brief Base class for units on each layer. For instance, on radio, the layer unit is a
         * hardware.
         *
         * \param unit_type_ common name for all units
         * \param id_ unique numeric id for specific unit
         */
        explicit layer_unit_t(const std::string unit_type_, const size_t id_)
            : id(id_),
              identifier(unit_type_ + " " + std::to_string(id)) {};
        virtual ~layer_unit_t() = default;

        layer_unit_t() = delete;
        layer_unit_t(const layer_unit_t&) = delete;
        layer_unit_t& operator=(const layer_unit_t&) = delete;
        layer_unit_t(layer_unit_t&&) = delete;
        layer_unit_t& operator=(layer_unit_t&&) = delete;

        const size_t id;
        const std::string identifier;

        void log_line(const std::string line) const;
        void log_lines(const std::vector<std::string> lines) const;

        /**
         * \brief This function is called by the main thread to signal that the SDR must stop
         * execution. Any running threads must be stopped. Deriving classes may also block this
         * function, and hence the main thread, for a finite duration to finish task that take
         * longer, for instance, shutting down a DECT NR+ connection gracefully.
         */
        virtual void work_stop() = 0;
};

}  // namespace dectnrp::common
