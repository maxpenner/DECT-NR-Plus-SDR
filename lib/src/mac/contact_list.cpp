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

#include "dectnrp/mac/contact_list.hpp"

#include "dectnrp/common/prog/assert.hpp"

#define ASSERT_LRDID_IS_KNOWN dectnrp_assert(is_lrdid_known(key_lrdid), "unknown key")

namespace dectnrp::mac {

bool contact_list_t::have_heard_from_at_or_after(const key_lrdid_t key_lrdid,
                                                 const int64_t time_64) const {
    ASSERT_LRDID_IS_KNOWN;

    return time_64 <= sync_report_last_known.at(key_lrdid).fine_peak_time_64;
}

bool contact_list_t::is_lrdid_known(const key_lrdid_t key_lrdid) const {
    return lrdid2srdid.is_k_known(key_lrdid);
}

bool contact_list_t::is_srdid_known(const uint32_t srdid) const {
    return lrdid2srdid.is_v_known(srdid);
}

}  // namespace dectnrp::mac
