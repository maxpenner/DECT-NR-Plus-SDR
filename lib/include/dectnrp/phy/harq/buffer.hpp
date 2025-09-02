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

#include <cstdint>

namespace dectnrp::phy::harq {

class buffer_t {
    public:
        explicit buffer_t(const uint32_t a_len_, const uint32_t d_len_);
        virtual ~buffer_t();

        buffer_t() = delete;
        buffer_t(const buffer_t&) = delete;
        buffer_t& operator=(const buffer_t&) = delete;
        buffer_t(buffer_t&&) = delete;
        buffer_t& operator=(buffer_t&&) = delete;

        /**
         * \brief It remains a mystery to me where the number 18600 in srsran is coming from. It
         * would make more sense to use 3*6144=18432 or even better 3*6144+12=18444. But we keep it
         * as the consequences of changing it are unclear. It is valid for Z=6144.
         *
         * 18600-18432=168
         * 18600-18444=156
         * 18600/3=6200=6144+56=6144+32+24
         *
         * References:
         * https://github.com/srsran/srsRAN/blob/master/lib/include/srsran/phy/fec/softbuffer.
         * https://github.com/srsran/srsRAN_4G/blob/master/lib/src/phy/fec/turbo/turbocoder.c
         * https://github.com/srsran/srsRAN_4G/blob/master/lib/src/phy/fec/turbo/rm_turbo.c
         *
         */
        static constexpr uint32_t HARQ_SOFTBUFFER_SIZE_Z_6144_PDC = 18600;

        /// reduced memory requirement for Z=2048, 3*2048+12=6156, we make it 6400
        static constexpr uint32_t HARQ_SOFTBUFFER_SIZE_Z_2048_PDC = 6400;

        /// PLCF has at most 80+16 input bits, thus 3*96+12=300, we make it 500
        static constexpr uint32_t HARQ_SOFTBUFFER_SIZE_PCC = 500;

        /// access to buffers
        uint8_t* get_a() const { return a; }
        uint8_t* get_a(const uint32_t byte_offset) const { return &a[byte_offset]; }
        uint8_t* get_d() const { return d; }
        uint8_t* get_d(const uint32_t byte_offset) const { return &d[byte_offset]; }

        uint32_t get_a_cnt() const { return a_cnt; };
        uint32_t add_a_cnt(const uint32_t add) {
            a_cnt += add;
            return a_cnt;
        };

        virtual void reset_a_cnt_and_softbuffer() = 0;
        virtual void reset_a_cnt_and_softbuffer(const uint32_t nof_cb) = 0;

        const uint32_t a_len;
        const uint32_t d_len;

    protected:
        uint8_t* a;
        uint8_t* d;

        /// counter of bytes written/read to transport block (not PLCF in case of TX)
        uint32_t a_cnt;
};

}  // namespace dectnrp::phy::harq
