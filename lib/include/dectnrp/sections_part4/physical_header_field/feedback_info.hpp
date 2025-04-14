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

#include <array>
#include <cstdint>
#include <limits>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>

#include "dectnrp/common/adt/miscellaneous.hpp"
#include "dectnrp/common/serdes/packing.hpp"

namespace dectnrp::section4 {

class feedback_info_t : public common::serdes::packing_t {
    public:
        virtual ~feedback_info_t() = default;

        static constexpr uint32_t No_feedback{0};

        enum class transmission_feedback_t : uint32_t {
            NACK = 0,
            ACK = 1,
            not_defined = common::adt::UNDEFINED_NUMERIC_32
        };

        enum class mimo_feedback_t : uint32_t {
            single_layer = 0,
            dual_layer = 1,
            four_layer = 2,
            reserved = 3,
            not_defined = common::adt::UNDEFINED_NUMERIC_32
        };

        static constexpr uint32_t MCS_out_of_range{common::adt::UNDEFINED_NUMERIC_32};

        static uint32_t buffer_size_2_buffer_status(const uint32_t buffer_size);

        static constexpr std::array<uint32_t, 16> buffer_status_2_buffer_size_lower = {
            0, 0, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768, 65536, 131072};

        static constexpr std::array<uint32_t, 16> buffer_status_2_buffer_size_upper = {
            0,
            16,
            32,
            64,
            128,
            256,
            512,
            1024,
            2048,
            4096,
            8192,
            16384,
            32768,
            65536,
            131072,
            std::numeric_limits<uint32_t>::max()};

    protected:
        virtual void zero() override = 0;
        virtual bool is_valid() const override = 0;
        virtual uint32_t get_packed_size() const override final;
        virtual void pack(uint8_t* a_ptr) const override = 0;
        virtual bool unpack(const uint8_t* a_ptr) override = 0;

        static uint32_t mcs_2_cqi(const int32_t mcs);
        static uint32_t cqi_2_mcs(const uint32_t cqi);
};

class feedback_info_f1_t final : public feedback_info_t {
    public:
        uint32_t HARQ_Process_number;
        transmission_feedback_t Transmission_feedback;
        uint32_t Buffer_Size;  // Buffer_Status;
        uint32_t MCS;          // CQI;

        friend class feedback_info_pool_t;

    private:
        void zero() override final;
        bool is_valid() const override final;
        void pack(uint8_t* a_ptr) const override final;
        bool unpack(const uint8_t* a_ptr) override final;
};

class feedback_info_f2_t final : public feedback_info_t {
    public:
        uint32_t Codebook_index;
        mimo_feedback_t MIMO_feedback;
        uint32_t Buffer_Size;  // Buffer_Status;
        uint32_t MCS;          // CQI;

        friend class feedback_info_pool_t;

    private:
        void zero() override final;
        bool is_valid() const override final;
        void pack(uint8_t* a_ptr) const override final;
        bool unpack(const uint8_t* a_ptr) override final;
};

class feedback_info_f3_t final : public feedback_info_t {
    public:
        uint32_t HARQ_Process_number_0;
        transmission_feedback_t Transmission_feedback_0;
        uint32_t HARQ_Process_number_1;
        transmission_feedback_t Transmission_feedback_1;
        uint32_t MCS;  // CQI;

        friend class feedback_info_pool_t;

    private:
        void zero() override final;
        bool is_valid() const override final;
        void pack(uint8_t* a_ptr) const override final;
        bool unpack(const uint8_t* a_ptr) override final;
};

class feedback_info_f4_t final : public feedback_info_t {
    public:
        uint32_t HARQ_feedback_bitmap;
        uint32_t MCS;  // CQI;

        friend class feedback_info_pool_t;

    private:
        void zero() override final;
        bool is_valid() const override final;
        void pack(uint8_t* a_ptr) const override final;
        bool unpack(const uint8_t* a_ptr) override final;
};

class feedback_info_f5_t final : public feedback_info_t {
    public:
        uint32_t HARQ_Process_number;
        transmission_feedback_t Transmission_feedback;
        mimo_feedback_t MIMO_feedback;
        uint32_t Codebook_index;

        friend class feedback_info_pool_t;

    private:
        void zero() override final;
        bool is_valid() const override final;
        void pack(uint8_t* a_ptr) const override final;
        bool unpack(const uint8_t* a_ptr) override final;
};

class feedback_info_f6_t final : public feedback_info_t {
    public:
        uint32_t HARQ_Process_number;
        uint32_t Reserved;
        uint32_t Buffer_Size;  // Buffer_Status;
        uint32_t MCS;          // CQI;

        friend class feedback_info_pool_t;

    private:
        void zero() override final;
        bool is_valid() const override final;
        void pack(uint8_t* a_ptr) const override final;
        bool unpack(const uint8_t* a_ptr) override final;
};

class feedback_info_pool_t {
    public:
        void pack(const uint32_t feedback_format, uint8_t* a_ptr) const;
        bool unpack(const uint32_t feedback_format, const uint8_t* a_ptr);

        feedback_info_f1_t feedback_info_f1;
        feedback_info_f2_t feedback_info_f2;
        feedback_info_f3_t feedback_info_f3;
        feedback_info_f4_t feedback_info_f4;
        feedback_info_f5_t feedback_info_f5;
        feedback_info_f6_t feedback_info_f6;
};

}  // namespace dectnrp::section4
