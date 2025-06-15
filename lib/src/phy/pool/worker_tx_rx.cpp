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

#include "dectnrp/phy/pool/worker_tx_rx.hpp"

#include <variant>

#include "dectnrp/common/adt/cast.hpp"
#include "dectnrp/common/prog/assert.hpp"
#include "dectnrp/common/prog/log.hpp"
#include "dectnrp/common/thread/watch.hpp"
#include "dectnrp/constants.hpp"
#include "dectnrp/phy/interfaces/maclow_phy.hpp"
#include "dectnrp/phy/interfaces/phy_machigh.hpp"
#include "dectnrp/phy/interfaces/phy_maclow.hpp"
#include "dectnrp/phy/rx/sync/irregular_report.hpp"

#define TOKEN_LOCK_FIFO_OR_RETURN                               \
    while (!token->lock_fifo_to(token_call_id, job.fifo_cnt)) { \
        if (!keep_running.load(std::memory_order_acquire)) {    \
            return;                                             \
        }                                                       \
    }

namespace dectnrp::phy {

worker_tx_rx_t::worker_tx_rx_t(worker_config_t& worker_config,
                               irregular_t& irregular_,
                               phy_radio_t& phy_radio_,
                               common::json_export_t* json_export_)
    : worker_t(worker_config),
      irregular(irregular_),
      phy_radio(phy_radio_),
      json_export(json_export_) {
    tx = std::make_unique<tx_t>(worker_pool_config.maximum_packet_sizes,
                                worker_pool_config.os_min,
                                worker_pool_config.resampler_param);

    const uint32_t u8subslot_length_samples =
        worker_pool_config.resampler_param.hw_samp_rate / constants::u8_subslots_per_sec;

    rx_synced = std::make_unique<rx_synced_t>(
        buffer_rx,
        worker_pool_config,
        u8subslot_length_samples * worker_pool_config.rx_chunk_unit_length_u8subslot);

    chscanner = std::make_unique<chscanner_t>(buffer_rx);
}

void worker_tx_rx_t::work() {
    // to reduce calls of keep_running
    common::watch_t watch;

    // overwritten in job_queue
    job_t job;

    while (keep_running.load(std::memory_order_acquire)) {
        // reset protection duration
        watch.reset();

        while (!watch.is_elapsed<common::milli>(KEEP_RUNNING_POLL_PERIOD_MS)) {
            // wait for job or check exit condition upon timeout (to)
            if (!job_queue.wait_for_new_job_to(job)) {
                continue;
            }

            // different actions for different jobs
            if (std::holds_alternative<regular_report_t>(job.content)) {
                TOKEN_LOCK_FIFO_OR_RETURN
                auto machigh_phy = tpoint->work_regular(std::get<regular_report_t>(job.content));
                token->unlock_fifo();

                run_tx_chscan(machigh_phy.tx_descriptor_vec,
                              machigh_phy.irregular_report,
                              machigh_phy.chscan_opt);

                ++stats.tpoint_work_regular;

            } else if (std::holds_alternative<irregular_report_t>(job.content)) {
                TOKEN_LOCK_FIFO_OR_RETURN
                auto machigh_phy =
                    tpoint->work_irregular(std::get<irregular_report_t>(job.content));
                token->unlock_fifo();

                run_tx_chscan(machigh_phy.tx_descriptor_vec,
                              machigh_phy.irregular_report,
                              machigh_phy.chscan_opt);

                ++stats.tpoint_work_irregular;

            } else if (std::holds_alternative<sync_report_t>(job.content)) {
                /* Internally tries to determine correct PLCF type 1 or 2.
                 *
                 * 1) Demodulate and decode type 1 and 2, and check both CRCs.
                 * 2) For correct CRCs, interpret PLCF type 1 and/or type 2 fields.
                 * 3) Sanity check if values (Packetlength, MCS, N_SS etc.) are within the limits
                 * set by the radio device class.
                 *
                 * Checking only the CRC is not enough for two reasons:
                 *
                 * 1) We can get a false alarm. For instance, even under ideal conditions, we can
                 * receive PLCF type 1 and still get a correct CRC for type 2.
                 * 2) A packet MUST stay within the packet limits set by the radio device class.
                 * Otherwise, decoding might fail, potentially with an exception.
                 */
                const auto pcc_report =
                    rx_synced->demoddecod_rx_pcc(std::get<sync_report_t>(job.content));

                // any PLCF found?
                if (pcc_report.plcf_decoder.has_any_plcf() == 0) {
#ifdef UPPER_TPOINT_ENABLE_PCC_INCORRECT_CRC
                    // compile all reports
                    const phy_maclow_t phy_maclow{std::get<sync_report_t>(job.content), pcc_report};

                    TOKEN_LOCK_FIFO_OR_RETURN
                    auto machigh_phy = tpoint->work_pcc_error(phy_maclow);
                    token->unlock_fifo();

                    run_tx_chscan(machigh_phy.tx_descriptor_vec,
                                  machigh_phy.irregular_report,
                                  machigh_phy.chscan_opt);
#else
                    TOKEN_LOCK_FIFO_OR_RETURN
                    // nothing to do here, but we have to increment the token's internal fifo_cnt
                    // when unlocking
                    token->unlock_fifo();
#endif
                    // reset for next reception
                    rx_synced->reset_for_next_pcc();

                    ++stats.rx_pcc_fail;

                    continue;
                }

                ++stats.rx_pcc_success;

                /* At this point, we have one or two candidates for the correct PCC. Before
                 * continuing with the PDC, we first call the lower MAC of the tpoint to see if
                 * there is interest in either version of the PCC, i.e. whether we proceed with the
                 * PDC. It returns a structure of type maclow_phy_t. If the decision is made to
                 * proceed with the PDC, we need the PCC version to use, either type 1 or 2. If
                 * there is no interest, for instance due to self reception, we ignore the PDC.
                 */

                // compile all reports
                const phy_maclow_t phy_maclow{std::get<sync_report_t>(job.content), pcc_report};

                TOKEN_LOCK_FIFO_OR_RETURN
                const maclow_phy_t maclow_phy = tpoint->work_pcc(phy_maclow);
                token->unlock_fifo();

                // PDC can be of no interest, for instance, when a device is unknown
                if (!maclow_phy.continue_with_pdc) {
                    // reset for next reception
                    rx_synced->reset_for_next_pcc();

                    dectnrp_assert(maclow_phy.hp_rx == nullptr,
                                   "HARQ buffer defined although no interest in the PDC");

                    continue;
                }

                // demodulate and decode PDC for the PCC choice made by lower MAC
                const auto pdc_report = rx_synced->demoddecod_rx_pdc(maclow_phy);

                // compile all reports
                const phy_machigh_t phy_machigh{phy_maclow, maclow_phy, pdc_report};

                // allocate return value from higher MAC
                machigh_phy_t machigh_phy;

                // call MAC regardless of correct or incorrect CRC
                token->lock(token_call_id);
                if (pdc_report.crc_status) {
                    ++stats.rx_pdc_success;
                    machigh_phy = tpoint->work_pdc(phy_machigh);
                } else {
                    ++stats.rx_pdc_fail;
                    machigh_phy = tpoint->work_pdc_error(phy_machigh);
                }
                token->unlock();

                run_tx_chscan(machigh_phy.tx_descriptor_vec,
                              machigh_phy.irregular_report,
                              machigh_phy.chscan_opt);

                /* Resetting for next RX must be done AFTER transmitting and running channel
                 * measurement, as resetting buffers can be quite time consuming. For instance, a
                 * single HARQ buffer can have up tp 6144 bits (unpacked) for each of the C
                 * codeblocks.
                 */

                // reset for next reception
                rx_synced->reset_for_next_pcc();

#ifdef PHY_JSON_SWITCH_IMPLEMENT_ANY_JSON_FUNCTIONALITY
                /* Export data if requested. Must be exported before HARQ process is finalized,
                 * otherwise packet size is reset.
                 */
                if (json_export != nullptr) {
                    collect_and_write_json(
                        std::get<sync_report_t>(job.content), phy_maclow, maclow_phy);
                }
#endif

                // unlocking does not necessarily imply resetting as this depends on how the MAC
                // layer configured the HARQ process
                maclow_phy.hp_rx->finalize(pdc_report.crc_status);

            } else if (std::holds_alternative<application::application_report_t>(job.content)) {
                TOKEN_LOCK_FIFO_OR_RETURN
                auto machigh_phy = tpoint->work_application(
                    std::get<application::application_report_t>(job.content));
                token->unlock_fifo();

                run_tx_chscan(machigh_phy.tx_descriptor_vec,
                              machigh_phy.irregular_report,
                              machigh_phy.chscan_opt);

                ++stats.tpoint_work_upper;

            } else {
                dectnrp_assert_failure("undefined job content");
            }
        }
    }

    // keep running for some more time to dequeue all jobs
    watch.reset();
    while (!watch.is_elapsed<common::milli>(KEEP_RUNNING_POLL_PERIOD_MS * 2)) {
        job_queue.wait_for_new_job_to(job);
    }
}

std::vector<std::string> worker_tx_rx_t::report_start() const {
    std::vector<std::string> lines;

    std::string str("Worker TX/RX " + std::to_string(id));

    // components

    // neither tx nor rx_synced report

    lines.push_back(str);

    return lines;
}

std::vector<std::string> worker_tx_rx_t::report_stop() const {
    std::vector<std::string> lines;

    std::string str("Worker TX/RX " + std::to_string(id));
    str.append(" tx_desc " + std::to_string(stats.tx_desc));
    str.append(" tx_desc_other_hw " + std::to_string(stats.tx_desc_other_hw));
    str.append(" tx_fail_no_buffer " + std::to_string(stats.tx_fail_no_buffer));
    str.append(" tx_fail_no_buffer_other_hw " + std::to_string(stats.tx_fail_no_buffer_other_hw));
    str.append(" tx_sent " + std::to_string(stats.tx_sent));
    str.append(" rx_pcc_success " + std::to_string(stats.rx_pcc_success));
    str.append(" rx_pcc_fail " + std::to_string(stats.rx_pcc_fail));
    str.append(" rx_pdc_success " + std::to_string(stats.rx_pdc_success));
    str.append(" rx_pdc_fail " + std::to_string(stats.rx_pdc_fail));
    str.append(" tpoint_work_regular " + std::to_string(stats.tpoint_work_regular));
    str.append(" tpoint_work_irregular " + std::to_string(stats.tpoint_work_irregular));
    str.append(" tpoint_work_upper " + std::to_string(stats.tpoint_work_upper));

    // components

    // neither tx nor rx_synced report

    lines.push_back(str);

    return lines;
}

void worker_tx_rx_t::run_tx_chscan(const tx_descriptor_vec_t& tx_descriptor_vec,
                                   const irregular_report_t& irregular_report,
                                   chscan_opt_t& chscan_opt) {
    // generate packets and pass data to radio layer for transmission
    for (auto& tx_descriptor : tx_descriptor_vec) {
        radio::buffer_tx_t* buffer_tx = nullptr;

        if (tx_descriptor.hw_id <= tx_descriptor_t::hw_id_associated) {
            // try to get a buffer from associated hw
            buffer_tx = buffer_tx_pool.get_buffer_tx_to_fill();

            ++stats.tx_desc;

            if (buffer_tx == nullptr) {
                ++stats.tx_fail_no_buffer;
                dectnrp_assert_failure("worker_tx_rx {} got nullptr as buffer", id);
                continue;
            }

        } else {
            // try to get a buffer from another hw
            buffer_tx = phy_radio.radio_ctrl_vec.at(tx_descriptor.hw_id_associated)
                            .buffer_tx_pool.get_buffer_tx_to_fill();

            ++stats.tx_desc_other_hw;

            if (buffer_tx == nullptr) {
                ++stats.tx_fail_no_buffer_other_hw;
                dectnrp_assert_failure("worker_tx_rx {} got nullptr as buffer from other hw", id);
                continue;
            }
        }

        tx->generate_tx_packet(tx_descriptor, *buffer_tx);

        ++stats.tx_sent;

        tx_descriptor.hp_tx.finalize();
    }

    if (irregular_report.has_finite_time()) {
        irregular.push(std::move(irregular_report));
    }

    // run channel measurement
    if (chscan_opt.has_value()) {
        chscanner->scan(chscan_opt.value());

        token->lock(token_call_id);
        const auto machigh_phy = tpoint->work_chscan(chscan_opt.value());
        token->unlock();

        chscan_opt_t chscan_opt_empty = chscan_opt_t{std::nullopt};

        // recursive call
        run_tx_chscan(machigh_phy.tx_descriptor_vec, irregular_report_t(), chscan_opt_empty);
    }
}

#ifdef PHY_JSON_SWITCH_IMPLEMENT_ANY_JSON_FUNCTIONALITY
void worker_tx_rx_t::collect_and_write_json(const sync_report_t& sync_report,
                                            const phy_maclow_t& phy_maclow,
                                            const maclow_phy_t& maclow_phy) {
    nlohmann::ordered_json json;

    json["worker_id"] = id;
    json["elapsed_since_epoch"] = common::watch_t::get_elapsed_since_epoch();
    json["32fc_int32_scale"] = common::adt::cast::get_scale_int<int32_t>();

    // ####################################
    // RADIO

    json["RADIO"]["samp_rate"] = hw.buffer_rx->samp_rate;
    json["RADIO"]["N_TX_min"] = worker_pool_config.radio_device_class.N_TX_min;
    json["RADIO"]["tx_power_ant_0dBFS"] = maclow_phy.hw_status.tx_power_ant_0dBFS;
    json["RADIO"]["rx_power_ant_0dBFS"] = maclow_phy.hw_status.rx_power_ant_0dBFS.get_ary();

    // ####################################
    // PHY

    json["PHY"]["worker_pool_config"]["enforce_dectnrp_samp_rate_by_resampling"] =
        worker_pool_config.enforce_dectnrp_samp_rate_by_resampling;
    json["PHY"]["worker_pool_config"]["L"] = worker_pool_config.resampler_param.L;
    json["PHY"]["worker_pool_config"]["M"] = worker_pool_config.resampler_param.M;
    json["PHY"]["worker_pool_config"]["dect_samp_rate_max_oversampled"] =
        worker_pool_config.get_dect_samp_rate_max_oversampled();

    nlohmann::ordered_json json_sync_report;
    json_sync_report["detection_ant_idx"] = sync_report.detection_ant_idx;
    json_sync_report["detection_rms"] = sync_report.detection_rms;
    json_sync_report["detection_metric"] = sync_report.detection_metric;
    json_sync_report["u"] = sync_report.u;
    json_sync_report["coarse_peak_array"] = sync_report.coarse_peak_array.get_ary();
    json_sync_report["rms_array"] = sync_report.rms_array.get_ary();
    json_sync_report["cfo_f"] = sync_report.cfo_fractional_rad;
    json_sync_report["b"] = sync_report.b;
    json_sync_report["cfo_i"] = sync_report.cfo_integer_rad;
    json_sync_report["coarse_peak_time"] = sync_report.coarse_peak_time_64;
    json_sync_report["N_eff_TX"] = sync_report.N_eff_TX;
    json_sync_report["fine_peak_time"] = sync_report.fine_peak_time_64;
    json_sync_report["sto_fractional"] = sync_report.sto_fractional;
    json_sync_report["fine_peak_time_corrected_by_sto_fractional"] =
        sync_report.fine_peak_time_corrected_by_sto_fractional_64;

    json["PHY"]["sync_report"] = json_sync_report;
    json["PHY"]["rx_synced"] = rx_synced->get_json();

    // ####################################
    // MAC

    json["MAC"]["plcf"] =
        phy_maclow.pcc_report.plcf_decoder.get_json(maclow_phy.hp_rx->get_PLCF_type());

    // ####################################
    // save

    // add to exporter and perhaps write to disk
    json_export->append(std::move(json));
}
#endif

}  // namespace dectnrp::phy
