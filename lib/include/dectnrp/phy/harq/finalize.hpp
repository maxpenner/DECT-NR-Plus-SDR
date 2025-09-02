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

namespace dectnrp::phy::harq {

enum class finalize_tx_t {
    reset_and_terminate,
    increase_rv_and_keep_running,
    keep_rv_and_keep_running
};

enum class finalize_rx_t {
    reset_and_terminate,
    keep_running,
    keep_running_or_reset_and_terminate_if_crc_correct
};

}  // namespace dectnrp::phy::harq
