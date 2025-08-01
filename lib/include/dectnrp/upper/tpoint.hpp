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

#include <cstdint>
#include <utility>

#include "dectnrp/application/application_report.hpp"
#include "dectnrp/common/ant.hpp"
#include "dectnrp/common/layer/layer_unit.hpp"
#include "dectnrp/phy/agc/agc_rx.hpp"
#include "dectnrp/phy/agc/agc_tx.hpp"
#include "dectnrp/phy/harq/process_pool.hpp"
#include "dectnrp/phy/interfaces/layers_downwards/mac_lower.hpp"
#include "dectnrp/phy/interfaces/machigh_phy.hpp"
#include "dectnrp/phy/interfaces/maclow_phy.hpp"
#include "dectnrp/phy/interfaces/phy_machigh.hpp"
#include "dectnrp/phy/interfaces/phy_maclow.hpp"
#include "dectnrp/phy/rx/chscan/chscan.hpp"
#include "dectnrp/phy/rx/sync/irregular_report.hpp"
#include "dectnrp/sections_part3/derivative/packet_sizes_def.hpp"
#include "dectnrp/upper/tpoint_config.hpp"
#include "dectnrp/upper/tpoint_stats.hpp"

namespace dectnrp::upper {

class tpoint_t : public common::layer_unit_t {
    public:
        /**
         * \brief This class tpoint_t (tpoint = termination point) is a base class every firmware
         * has to derive from. It declares the absolute minimum interface of functions and members
         * that every firmware must implement. Additional methods and members should not be included
         * here, but in deriving classes/firmware.
         *
         * To load a new firmware at SDR startup, the class deriving from tpoint_t must have a
         * unique static member "firmware_name" of type std::string, and it must be added to the
         * function upper_t::add_tpoint() in upper.cpp. The value of "firmware_name" can then be
         * used in any configuration file "upper.json".
         *
         * \param tpoint_config_ configuration of this termination point
         * \param mac_lower_ access to phy + radio layer
         */
        explicit tpoint_t(const tpoint_config_t& tpoint_config_, phy::mac_lower_t& mac_lower_);
        virtual ~tpoint_t() = default;

        tpoint_t() = delete;
        tpoint_t(const tpoint_t&) = delete;
        tpoint_t& operator=(const tpoint_t&) = delete;
        tpoint_t(tpoint_t&&) = delete;
        tpoint_t& operator=(tpoint_t&&) = delete;

        /**
         * \brief Function is called shortly before PHY and radio layer become operational and start
         * processing IQ samples. Once operational, the other work-functions will start being
         * called. This function should be used to make time-critical settings, e.g. setting the
         * time of the first beacon. It must return ASAP.
         *
         * 1. Called exactly once.
         * 2. Called before any other work-function.
         * 3. Called shortly before PHY starts packet synchronization at start_time_64 >=0.
         *
         * \param start_time_64 time at which PHY starts synchronization, given as sample
         * count at the hardware sample rate
         * \return first opportunity to schedule an irregular callback
         */
        [[nodiscard]] virtual phy::irregular_report_t work_start(const int64_t start_time_64) = 0;

        /**
         * \brief Function being called regularly. It contains two important pieces of information.
         * Firstly, a time stamp before which no more packets can be detected, called the barrier
         * time. Secondly, the time of the last STF found. While the barrier time is constantly
         * growing, the last known STF can be arbitrarily far in the past or even have a negative
         * time stamp if no STF has been found yet.
         *
         * 1. Call rate depends on the synchronization's chunk size, e.g. every two slots.
         * 2. Call rate can be set to zero in phy.json.
         * 3. Contains information mentioned above.
         * 4. Called in the same FIFO order as put into the job_queue.
         *
         * \param regular_report contains current time stamp
         * \return
         */
        [[nodiscard]] virtual phy::machigh_phy_t work_regular(
            const phy::regular_report_t& regular_report) = 0;

        /**
         * \brief Function being called irregularly. A firmware can request an irregular callback to
         * be called as soon as the specified time has passed for synchronization, i.e. once no more
         * packets can be found before the specified time. The callback than happens through this
         * function.
         *
         * 1. Only called if firmware requested it beforehand.
         * 2. Called in the same FIFO order as put into the job_queue.
         *
         * \param irregular_report contains a handle to identify the request
         * \return
         */
        [[nodiscard]] virtual phy::machigh_phy_t work_irregular(
            const phy::irregular_report_t& irregular_report) = 0;

        /**
         * \brief Function called after decoding a PCC with correct CRC.
         *
         * 1. Called only after successful PCC decoding, i.e. correct CRC.
         * 2. Called in the same FIFO order as put into the job_queue (sync_report_t).
         *
         * \param phy_maclow information provided by PHY about synchronization and PCC
         * \return
         */
        [[nodiscard]] virtual phy::maclow_phy_t work_pcc(const phy::phy_maclow_t& phy_maclow) = 0;

#ifdef UPPER_TPOINT_ENABLE_PCC_INCORRECT_CRC
        /**
         * \brief Function called after decoding a PCC with incorrect CRC. This function is virtual
         * while all other functions are pure virtual. Furthermore, it must be explicitly enabled by
         * defining the enclosing preprocessor directive. Notifying the MAC layer of a PCC with
         * incorrect CRC can be useful if the MAC layer was expecting a packet at that time.
         * Downside is that the MAC layer is called and blocked for every false alarm produced by
         * synchronization.
         *
         * 1. Called only after unsuccessful PCC decoding, i.e. incorrect CRC.
         * 2. Called in the same FIFO order as put into the job_queue (sync_report_t).
         *
         * \param phy_maclow information provided by PHY about synchronization and PCC
         * \return
         */
        [[nodiscard]] virtual phy::machigh_phy_t work_pcc_error(
            const phy::phy_maclow_t& phy_maclow);
#endif

        /**
         * \brief Function called after decoding a PDC with correct CRC.
         *
         * 1. PHY processes PDC only if maclow_phy_t::continue_with_pdc=true for the respective PCC.
         * 2. Called after successful and unsuccessful PDC decoding, i.e. correct and incorrect CRC.
         * 3. Called ASAP, but not in any specific order relative to other work-functions.
         *
         * \param phy_machigh information provided by PHY about synchronization, PCC and PDC
         * \return
         */
        [[nodiscard]] virtual phy::machigh_phy_t work_pdc(
            const phy::phy_machigh_t& phy_machigh) = 0;

        /**
         * \brief Function called after decoding a PDC with incorrect CRC. Same properties as
         * work_pdc().
         *
         * \param phy_machigh information provided by PHY about synchronization, PCC and PDC
         * \return
         */
        [[nodiscard]] virtual phy::machigh_phy_t work_pdc_error(
            const phy::phy_machigh_t& phy_machigh) = 0;

        /**
         * \brief Function called to notify lower layers of new data being available on the
         * application layer.
         *
         * 1. Call rate depends on settings on application layer.
         * 2. Called in the same FIFO order as put into the job_queue (application_report_).
         *
         * \param application_report information about available data on upper layers
         * \return
         */
        [[nodiscard]] virtual phy::machigh_phy_t work_application(
            const application::application_report_t& application_report) = 0;

        /**
         * \brief Function called when a channel measurement has finished.
         *
         * 1. PHY conducts ch measurements only if machigh_phy_t::chscan_opt contains a value.
         * 2. Called ASAP, but not in any specific order relative to other work-functions.
         *
         * \param chscan result of channel measurement instruction
         * \return
         */
        [[nodiscard]] virtual phy::machigh_phy_tx_t work_channel(const phy::chscan_t& chscan) = 0;

        /**
         * \brief Function will be called by the main thread when the SDR is supposed to shut down
         * because the user pressed ctrl+c. A firmware may block this function until all DECT NR+
         * connections have been shut down gracefully. After that, all job queues should be made
         * impermeable so that the work-function will no longer be called after processing the
         * remaining jobs. Lastly, all threads must be shut down.
         */
        virtual void work_stop() override = 0;

    protected:
        /// configuration received during construction
        const tpoint_config_t& tpoint_config;

        // ##################################################
        // Radio Layer + PHY

        /**
         * \brief This member gives the MAC layer control over the lower part of all stacks. It can
         * contain multiple lower stacks in mac_lower.lower_ctrl_vec, each representing one
         * combination of PHY plus radio layer. Thus, a single firmware can control multiple
         * hardware radios.
         */
        phy::mac_lower_t& mac_lower;

        /* Most firmware will use only one lower stack (PHY plus radio layer), and accessing this
         * single lower stack through mac_lower is cumbersome. For this reason, we save references
         * to the elements of the first object in mac_lower.lower_ctrl_vec, which is the primary
         * lower stack.
         */
        radio::hw_t& hw;
        const radio::buffer_rx_t& buffer_rx;
        const phy::worker_pool_config_t& worker_pool_config;
        phy::job_queue_t& job_queue;
        const sp3::duration_lut_t& duration_lut;
        phy::agc::agc_tx_t& agc_tx;
        phy::agc::agc_rx_t& agc_rx;
        int64_t& tx_order_id;
        int64_t& tx_earliest_64;
        phy::harq::process_pool_t& hpp;

        // ##################################################
        // MAC Layer

        /**
         * \brief This convenience function can be used by any firmware to apply AGC gain changes
         * for both TX and RX. AGC gain changes are based on the RMS of a received packet read from
         * sync_report, and the transmit power of the same packet extracted from its PLCF.
         *
         * AGC gain changes can be applied ASAP (t_agc_xx_change_64 < 0), or at a fixed point in
         * time in the future (t_agc_xx_change_64 >= 0). Typically, the AGC settings should be
         * applied when it is guaranteed that no packet will be transmitted or received while the
         * changes are made, for instance, in a GI at the end of a packet.
         *
         * \param sync_report contains received RMS
         * \param plcf_base contains transmit power
         * \param t_agc_tx_change_64 if negative, settings are applied ASAP
         * \param t_agc_rx_change_64 if negative, settings are applied ASAP
         * \param hw_idx index of hardware in mac_lower
         */
        void worksub_agc(const phy::sync_report_t& sync_report,
                         const sp4::plcf_base_t& plcf_base,
                         const int64_t t_agc_tx_change_64,
                         const int64_t t_agc_rx_change_64,
                         const std::size_t hw_idx = 0);

        /**
         * \brief Same as above, but only returns optimal power adjustments without applying them.
         * This is useful if AGC changes are made by the radio layer during packet transmission.
         *
         * \param sync_report see above
         * \param plcf_base see above
         * \param hw_idx see above
         * \return optimal TX and RX AGC adjustments
         */
        [[nodiscard]] std::pair<common::ant_t, common::ant_t> worksub_agc_adj(
            const phy::sync_report_t& sync_report,
            const sp4::plcf_base_t& plcf_base,
            const std::size_t hw_idx = 0);

        /**
         * \brief When a firmware is called with a successfully received PCC and the choice is
         * made to proceed with the PDC, this convenience can be used to create the respective
         * instruction of type maclow_phy_t for the PHY. For the PDC, a new HARQ buffer is
         * requested.
         *
         * \param phy_maclow provided by PHY, contains sync_report and PLCF
         * \param PLCF_type PLCF type to decode
         * \param network_id network ID for scrambling on PHY
         * \param rv redundancy version for decoding on PHY
         * \param frx final action applied to HARQ buffer after PHY used it
         * \param mph handle to identify PCC when PHY calls with decoded PDC
         * \param process_id id of HARQ process created, must not be nullptr if kept running
         * \return
         */
        [[nodiscard]] phy::maclow_phy_t worksub_pcc2pdc(const phy::phy_maclow_t& phy_maclow,
                                                        const uint32_t PLCF_type,
                                                        const uint32_t network_id,
                                                        const uint32_t rv,
                                                        const phy::harq::finalize_rx_t frx,
                                                        const phy::maclow_phy_handle_t mph,
                                                        uint32_t* process_id = nullptr);

        /**
         * \brief same as worksub_pcc2pdc(), but for an already running HARQ process
         *
         * \param rv redundancy version
         * \param frx final action applied to HARQ buffer after PHY used it
         * \param mph handle to identify PCC when PHY calls with decoded PDC
         * \param process_id HARQ process id
         * \return
         */
        [[nodiscard]] phy::maclow_phy_t worksub_pcc2pdc_running(const uint32_t rv,
                                                                const phy::harq::finalize_rx_t frx,
                                                                const phy::maclow_phy_handle_t mph,
                                                                const uint32_t process_id);

        // ##################################################
        // DLC and Convergence Layer
        // -

        // ##################################################
        // Application Layer
        // -

        // ##################################################
        // statistics

        tpoint_stats_t stats;

    private:
        /**
         * \brief Convenience function to convert a structure phy_maclow_t to an unequivocal packet
         * size based on the PCC choice made by the calling firmware.
         *
         * \param phy_maclow provided by PHY, contains sync_report and PLCF
         * \param PLCF_type PLCF type to decode
         * \return
         */
        [[nodiscard]] sp3::packet_sizes_def_t worksub_psdef(const phy::phy_maclow_t& phy_maclow,
                                                            const uint32_t PLCF_type) const;
};

}  // namespace dectnrp::upper
