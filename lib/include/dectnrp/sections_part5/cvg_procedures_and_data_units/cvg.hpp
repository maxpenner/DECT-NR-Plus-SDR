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

#include "dectnrp/common/adt/result.hpp"
#include "dectnrp/sections_part5/cvg_procedures_and_data_units/config.hpp"
#include "dectnrp/sections_part5/cvg_procedures_and_data_units/handle.hpp"
#include "dectnrp/sections_part5/cvg_procedures_and_data_units/request.hpp"
#include "dectnrp/sections_part5/cvg_procedures_and_data_units/request_error.hpp"

namespace dectnrp::section5::cvg {

class cvg_t {
    public:
        cvg_t() = default;
        cvg_t(const config_t config_);

        common::adt::expected_t<handle_t, request_error_t> get_handle(const request_t request);

    private:
        [[maybe_unused]] config_t config;
};

}  // namespace dectnrp::section5::cvg
