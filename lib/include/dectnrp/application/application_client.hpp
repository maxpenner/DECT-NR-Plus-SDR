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

#include "dectnrp/application/application.hpp"

#define APPLICATION_APP_CLIENT_CONDITION_VARIABLE_OR_BUSY_WAITING
#ifdef APPLICATION_APP_CLIENT_CONDITION_VARIABLE_OR_BUSY_WAITING
#include <condition_variable>
#include <mutex>
#else
#endif

namespace dectnrp::application {

class application_client_t : public application_t {
    public:
        explicit application_client_t(const uint32_t id_,
                                      const common::threads_core_prio_config_t thread_config_,
                                      phy::job_queue_t& job_queue_,
                                      const uint32_t N_queue,
                                      const queue_size_t queue_size);
        virtual ~application_client_t() = default;

        application_client_t() = delete;
        application_client_t(const application_client_t&) = delete;
        application_client_t& operator=(const application_client_t&) = delete;
        application_client_t(application_client_t&&) = delete;
        application_client_t& operator=(application_client_t&&) = delete;

        [[nodiscard]] virtual uint32_t get_n_connections() const override = 0;

        [[nodiscard]] virtual uint32_t write_immediate(const uint32_t conn_idx,
                                                       const uint8_t* inp,
                                                       const uint32_t n) = 0;
        [[nodiscard]] virtual uint32_t write_nto(const uint32_t conn_idx,
                                                 const uint8_t* inp,
                                                 const uint32_t n) = 0;
        [[nodiscard]] virtual uint32_t write_try(const uint32_t conn_idx,
                                                 const uint8_t* inp,
                                                 const uint32_t n) = 0;

        void trigger_forward_nto(const uint32_t datagram_cnt);

    protected:
        void work_sc() override final;

#ifdef APPLICATION_APP_CLIENT_CONDITION_VARIABLE_OR_BUSY_WAITING
        std::mutex lockv;
        std::condition_variable cv;
        int32_t indicator_cnt;
#else
        std::atomic<int32_t> indicator_cnt;
#endif

        /**
         * \brief Every deriving class must filter egress datagrams.
         *
         * \param conn_idx
         * \return true to forward datagram
         * \return false to discard datagram
         */
        [[nodiscard]] virtual bool filter_egress_datagram(const uint32_t conn_idx) = 0;

        void inc_indicator_cnt_under_lock(const uint32_t datagram_cnt);
        void dec_indicator_cnt_under_lock();
        [[nodiscard]] uint32_t get_indicator_cnt_under_lock();

        void forward_under_lock();

        [[nodiscard]] virtual uint32_t copy_from_queue_to_localbuffer(const uint32_t conn_idx) = 0;
};

}  // namespace dectnrp::application
