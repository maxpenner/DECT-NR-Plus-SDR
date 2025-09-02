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

#include "dectnrp/common/adt/result.hpp"
#include "dectnrp/common/layer/layer_unit.hpp"
#include "dectnrp/cvg/cvg_config.hpp"
#include "dectnrp/cvg/io/close_res.hpp"
#include "dectnrp/cvg/io/handle.hpp"
#include "dectnrp/cvg/io/inp.hpp"
#include "dectnrp/cvg/io/inp_res.hpp"
#include "dectnrp/cvg/io/out.hpp"
#include "dectnrp/cvg/io/out_res.hpp"
#include "dectnrp/cvg/request.hpp"
#include "dectnrp/cvg/request_error.hpp"

namespace dectnrp::cvg {

class cvg_t final : public common::layer_unit_t {
    public:
        cvg_t(const size_t id_, const cvg_config_t cvg_config_);

        [[nodiscard]] common::adt::expected_t<handle_t, request_error_t> request_handle(
            const request_t request);

        [[nodiscard]] inp_res_t write(const handle_t& handle, const inp_t& inp);

        [[nodiscard]] out_res_t read(const handle_t& handle, out_t& out);

        [[nodiscard]] close_res_t close(const handle_t& handle);

    private:
        virtual void work_stop() override final;

        [[maybe_unused]] cvg_config_t cvg_config;
};

}  // namespace dectnrp::cvg
