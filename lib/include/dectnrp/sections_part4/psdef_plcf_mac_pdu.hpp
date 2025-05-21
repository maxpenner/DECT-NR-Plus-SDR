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

#include "dectnrp/sections_part3/derivative/packet_sizes_def.hpp"
#include "dectnrp/sections_part4/mac_pdu/mac_common_header.hpp"
#include "dectnrp/sections_part4/mac_pdu/mac_header_type.hpp"
#include "dectnrp/sections_part4/physical_header_field/plcf_10.hpp"
#include "dectnrp/sections_part4/physical_header_field/plcf_20.hpp"
#include "dectnrp/sections_part4/physical_header_field/plcf_21.hpp"

namespace dectnrp::sp4 {

class psdef_plcf_mac_pdu_t {
    public:
        /// defines all sizes of the transmission (bits, bytes and IQ samples)
        sp3::packet_sizes_def_t psdef;

        /// returns byte offset of MAC header type and MAC common header
        [[nodiscard]] uint32_t pack_first_3_header(uint8_t* a_plcf, uint8_t* a_mac_pdu) const;

        /// deriving classes add specific PLCFs, this pointers picks one
        sp4::plcf_base_t* plcf_base_effective{nullptr};

        /// every packet type uses the same MAC header type
        sp4::mac_header_type_t mac_header_type;

        /// deriving classes add specific MAC common headers, this pointers picks one
        sp4::mac_common_header_t* mch_base_effective{nullptr};

        uint32_t get_packed_size_mht_mch() const {
            return mac_header_type.get_packed_size() + mch_base_effective->get_packed_size();
        }
};

/// ppmp = psdef_plcf_mac_pdu
class ppmp_data_t final : public psdef_plcf_mac_pdu_t {
    public:
        sp4::plcf_20_t plcf_20;
        sp4::plcf_21_t plcf_21;

        /// base class contains MAC header type

        sp4::data_mac_pdu_header_t data_mac_pdu_header;

        // add restrictions as to which MMIEs can be attached
        // ToDo
};

/// ppmp = psdef_plcf_mac_pdu
class ppmp_beacon_t final : public psdef_plcf_mac_pdu_t {
    public:
        sp4::plcf_10_t plcf_10;

        /// base class contains MAC header type

        sp4::beacon_header_t beacon_header;

        // add restrictions as to which MMIEs can be attached
        // ToDo
};

/// ppmp = psdef_plcf_mac_pdu
class ppmp_unicast_t final : public psdef_plcf_mac_pdu_t {
    public:
        sp4::plcf_20_t plcf_20;
        sp4::plcf_21_t plcf_21;

        /// base class contains MAC header type

        sp4::mch_empty_t mch_empty;
        sp4::unicast_header_t unicast_header;

        // add restrictions as to which MMIEs can be attached
        // ToDo
};

/// ppmp = psdef_plcf_mac_pdu
class ppmp_rd_broadcast_t final : public psdef_plcf_mac_pdu_t {
    public:
        sp4::plcf_10_t plcf_10;

        /// base class contains MAC header type

        sp4::rd_broadcasting_header_t rd_broadcasting_header;

        // add restrictions as to which MMIEs can be attached
        // ToDo
};

}  // namespace dectnrp::sp4
