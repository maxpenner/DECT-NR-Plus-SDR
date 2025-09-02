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

#include <memory>
#include <vector>

#include "dectnrp/phy/interfaces/layers_downwards/lower_ctrl.hpp"
#include "dectnrp/phy/pool/token.hpp"

namespace dectnrp::phy {

class mac_lower_t {
    public:
        mac_lower_t(const std::shared_ptr<token_t> token_)
            : token(token_) {}

        /// coordinates access to tpoint from worker_pool
        const std::shared_ptr<token_t> token;

        /// one tpoint can control multiple pairs
        std::vector<lower_ctrl_t> lower_ctrl_vec;
};

}  // namespace dectnrp::phy
