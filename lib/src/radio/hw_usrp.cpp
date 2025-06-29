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

#include "dectnrp/radio/hw_usrp.hpp"

#include <cmath>
#include <string>
#include <uhd/convert.hpp>
#include <utility>
#include <vector>

#include "dectnrp/common/prog/assert.hpp"
#include "dectnrp/common/thread/threads.hpp"
#include "dectnrp/common/thread/watch.hpp"
#include "dectnrp/radio/calibration/cal_usrp_b210.hpp"
#include "dectnrp/radio/calibration/cal_usrp_n310.hpp"
#include "dectnrp/radio/calibration/cal_usrp_n320.hpp"
#include "dectnrp/radio/calibration/cal_usrp_x410.hpp"

#define USRP_DIGITAL_LO_TUNING_OFFSET_FREQUENCY_HZ (master_clock_rate / 4)

namespace dectnrp::radio {

const std::string hw_usrp_t::name = "usrp";

hw_usrp_t::hw_usrp_t(const hw_config_t& hw_config_)
    : hw_t(hw_config_) {
    // hw_config.usrp_args must be specific enough to find only one USRP
    device_addrs = uhd::device::find(hw_config.usrp_args, uhd::device::USRP);

    dectnrp_assert(
        !device_addrs.empty(),
        "Requested USRP not found. Check usrp_args, they must match your device. Also check if use_dpdk=1 is set.");
    dectnrp_assert(device_addrs.size() == 1,
                   "More than one USRP found. Check if usrp_args are specific enough.");

    // Next check that the device is actually a supported USRP and set parameters.
    // source for master clock:
    // https://kb.ettus.com/USRP_N300/N310/N320/N321_Getting_Started_Guide#Verifying_Device_Operation
    // Decimation factors must be even, full list can be found here:
    // https://kb.ettus.com/USRP_N300/N310/N320/N321_Getting_Started_Guide#Verifying_Device_Operation
    if (device_addrs.at(0).get("product", "") == "n310") {
        master_clock_rate = 122880000;
        decimation_factors = {1, 2, 4, 8, 16, 32, 64};

        nof_antennas_max = 4;
        ADC_bits = 16;
        DAC_bits = 14;
        tmin_us.at(std::to_underlying(tmin_t::freq)) = 250;
        tmin_us.at(std::to_underlying(tmin_t::gain)) = 200;
        tmin_us.at(std::to_underlying(tmin_t::turnaround)) = hw_config.turnaround_time_us;
        ppm = 1.0f;
        time_advance_fpga2ant_samples = 0;

        // https://www.ettus.com/wp-content/uploads/2019/01/USRP_N310_Datasheet_v3.pdf

        gain_lut.freqs_Hz = calibration::n310::freqs_Hz;

        gain_lut.gains_tx_dB = &calibration::n310::gains_tx_dB;
        gain_lut.powers_tx_dBm = &calibration::n310::powers_tx_dBm;
        gain_lut.gains_tx_dB_step = calibration::n310::gains_tx_dB_step;

        gain_lut.gains_rx_dB = &calibration::n310::gains_rx_dB;
        gain_lut.powers_rx_dBm = &calibration::n310::powers_rx_dBm;
        gain_lut.gains_rx_dB_step = calibration::n310::gains_rx_dB_step;

    } else if (device_addrs.at(0).get("product", "") == "n320") {
        master_clock_rate = 245760000;
        decimation_factors = {1, 2, 4, 8, 16, 32, 64, 128};

        nof_antennas_max = 2;
        ADC_bits = 14;
        DAC_bits = 16;
        tmin_us.at(std::to_underlying(tmin_t::freq)) = 250;
        tmin_us.at(std::to_underlying(tmin_t::gain)) = 200;
        tmin_us.at(std::to_underlying(tmin_t::turnaround)) = hw_config.turnaround_time_us;
        ppm = 1.0f;
        time_advance_fpga2ant_samples = 0;

        // https://www.ettus.com/wp-content/uploads/2019/04/USRP-N320-Datasheet-2.pdf

        gain_lut.freqs_Hz = calibration::n320::freqs_Hz;

        gain_lut.gains_tx_dB = &calibration::n320::gains_tx_dB;
        gain_lut.powers_tx_dBm = &calibration::n320::powers_tx_dBm;
        gain_lut.gains_tx_dB_step = calibration::n320::gains_tx_dB_step;

        gain_lut.gains_rx_dB = &calibration::n320::gains_rx_dB;
        gain_lut.powers_rx_dBm = &calibration::n320::powers_rx_dBm;
        gain_lut.gains_rx_dB_step = calibration::n320::gains_rx_dB_step;

    } else if (device_addrs.at(0).get("product", "") == "B210") {
        // B210 supports arbitrary master clocks, so we neither need a fixed master clock nor
        // decimation factors
        master_clock_rate = 55296000;

        nof_antennas_max = 2;
        ADC_bits = 12;
        DAC_bits = 12;
        tmin_us.at(std::to_underlying(tmin_t::freq)) = 250;
        tmin_us.at(std::to_underlying(tmin_t::gain)) = 200;
        tmin_us.at(std::to_underlying(tmin_t::turnaround)) = hw_config.turnaround_time_us;
        ppm = 2.0f;
        time_advance_fpga2ant_samples = 0;

        // https://kb.ettus.com/images/c/cb/B200_RF_Performance.pdf

        gain_lut.freqs_Hz = calibration::b210::freqs_Hz;

        gain_lut.gains_tx_dB = &calibration::b210::gains_tx_dB;
        gain_lut.powers_tx_dBm = &calibration::b210::powers_tx_dBm;
        gain_lut.gains_tx_dB_step = calibration::b210::gains_tx_dB_step;

        gain_lut.gains_rx_dB = &calibration::b210::gains_rx_dB;
        gain_lut.powers_rx_dBm = &calibration::b210::powers_rx_dBm;
        gain_lut.gains_rx_dB_step = calibration::b210::gains_rx_dB_step;

    } else if (device_addrs.at(0).get("product", "") == "x410") {
        master_clock_rate = 245760000;
        decimation_factors = {1, 2, 4, 8, 16, 32, 64, 128};

        nof_antennas_max = 4;
        ADC_bits = 12;
        DAC_bits = 14;
        tmin_us.at(std::to_underlying(tmin_t::freq)) = 250;
        tmin_us.at(std::to_underlying(tmin_t::gain)) = 200;
        tmin_us.at(std::to_underlying(tmin_t::turnaround)) = hw_config.turnaround_time_us;
        ppm = 2.0f;
        time_advance_fpga2ant_samples = 0;

        // https://www.ni.com/docs/de-DE/bundle/ettus-usrp-x410-specs/page/specs.html

        gain_lut.freqs_Hz = calibration::x410::freqs_Hz;

        gain_lut.gains_tx_dB = &calibration::x410::gains_tx_dB;
        gain_lut.powers_tx_dBm = &calibration::x410::powers_tx_dBm;
        gain_lut.gains_tx_dB_step = calibration::x410::gains_tx_dB_step;

        gain_lut.gains_rx_dB = &calibration::x410::gains_rx_dB;
        gain_lut.powers_rx_dBm = &calibration::x410::powers_rx_dBm;
        gain_lut.gains_rx_dB_step = calibration::x410::gains_rx_dB_step;

    } else {
        dectnrp_assert_failure("USRP device {} not supported",
                               device_addrs.at(0).get("product", ""));
    }
}

void hw_usrp_t::set_samp_rate(const uint32_t samp_rate_in) {
    // B-series supports arbitrary sample rates
    if (device_addrs.at(0).get("product", "") == "B210") {
        samp_rate = samp_rate_in;
    }
    /* The following code is only relevant for N- and X-series, which have only one or two fixed
     * master clock rates.
     */
    else {
        dectnrp_assert(samp_rate_in <= master_clock_rate, "master clock smaller input frequency");
        dectnrp_assert(!decimation_factors.empty(), "mo decimation factors available");

        // find the smallest sample rate larger than samp_rate_in
        samp_rate = master_clock_rate;
        for (uint32_t i = 0; i < decimation_factors.size(); ++i) {
            const uint32_t lower_suitable_usrp_samp_rate =
                master_clock_rate / decimation_factors[i];

            if (lower_suitable_usrp_samp_rate < samp_rate_in) {
                break;
            } else {
                samp_rate = lower_suitable_usrp_samp_rate;
            }
        }
    }

    dectnrp_assert(samp_rate_in <= samp_rate, "sample rate smaller than input sample rate");

    dectnrp_assert(
        (samp_rate / 2 + USRP_DIGITAL_LO_TUNING_OFFSET_FREQUENCY_HZ) <= master_clock_rate / 2,
        "sample rate + digital tuning offset too large");

    // convert from us to samples
    for (std::size_t i = 0; i < std::to_underlying(tmin_t::CARDINALITY); ++i) {
        tmin_samples.at(i) = get_samples_in_us(tmin_us.at(i));
    }
}

void hw_usrp_t::initialize_buffer_tx_pool(const uint32_t ant_streams_length_samples_max) {
    dectnrp_assert(tx_gap_samples > 0, "allowed gap size should be larger than 0");

    buffer_tx_pool = std::make_unique<buffer_tx_pool_t>(
        id, nof_antennas, hw_config.nof_buffer_tx, ant_streams_length_samples_max + tx_gap_samples);
}

void hw_usrp_t::initialize_buffer_rx(const uint32_t ant_streams_length_samples) {
    buffer_rx = std::make_unique<buffer_rx_t>(id,
                                              nof_antennas,
                                              ant_streams_length_samples,
                                              samp_rate,
                                              rx_stream->get_max_num_samps(),
                                              hw_config.rx_prestream_ms,
                                              hw_config.rx_notification_period_us);
}

void hw_usrp_t::initialize_device() {
    // now create the USRP
    m_usrp = uhd::usrp::multi_usrp::make(hw_config.usrp_args);

    /* Specify the subdev:
     * https://kb.ettus.com/USRP_N300/N310/N320/N321_Getting_Started_Guide#Subdevice_Specification_Mapping
     *
     * The value of master_clock_rate set in args, or arbitrary for B-series.
     */
    if (device_addrs.at(0).get("product", "") == "n310") {
        dectnrp_assert(nof_antennas == 1 || nof_antennas == 2 || nof_antennas == 4,
                       "Incorrect number of antennas.");

        if (nof_antennas == 1) {
            m_usrp->set_tx_subdev_spec(std::string("A:0"));
            m_usrp->set_rx_subdev_spec(std::string("A:0"));
        } else if (nof_antennas == 2) {
            m_usrp->set_tx_subdev_spec(std::string("A:0 A:1"));
            m_usrp->set_rx_subdev_spec(std::string("A:0 A:1"));
        } else {
            m_usrp->set_tx_subdev_spec(std::string("A:0 A:1 B:0 B:1"));
            m_usrp->set_rx_subdev_spec(std::string("A:0 A:1 B:0 B:1"));
        }

#ifdef RADIO_HW_IMPLEMENTS_GPIO_TOGGLE
        // https://files.ettus.com/manual/page_usrp_n3xx.html#n3xx_fpgio
        dectnrp_assert(
            false, "GPIO toggle not implmented for {}", device_addrs.at(0).get("product", ""));
#endif

    } else if (device_addrs.at(0).get("product", "") == "n320") {
        dectnrp_assert(nof_antennas == 1 || nof_antennas == 2, "Incorrect number of antennas.");

        if (nof_antennas == 1) {
            m_usrp->set_tx_subdev_spec(std::string("A:0"));
            m_usrp->set_rx_subdev_spec(std::string("A:0"));
        } else {
            m_usrp->set_tx_subdev_spec(std::string("A:0 B:0"));
            m_usrp->set_rx_subdev_spec(std::string("A:0 B:0"));
        }

#ifdef RADIO_HW_IMPLEMENTS_GPIO_TOGGLE
        // https://files.ettus.com/manual/page_usrp_n3xx.html#n3xx_fpgio
        dectnrp_assert(
            false, "GPIO toggle not implmented for {}", device_addrs.at(0).get("product", ""));
#endif

    } else if (device_addrs.at(0).get("product", "") == "B210") {
        dectnrp_assert(nof_antennas == 1 || nof_antennas == 2, "Incorrect number of antennas.");

        if (nof_antennas == 1) {
            m_usrp->set_tx_subdev_spec(std::string("A:A"));
            m_usrp->set_rx_subdev_spec(std::string("A:A"));
        } else {
            m_usrp->set_tx_subdev_spec(std::string("A:A A:B"));
            m_usrp->set_rx_subdev_spec(std::string("A:A A:B"));
        }

#ifdef RADIO_HW_IMPLEMENTS_GPIO_TOGGLE
        gpio_state.bank = "FP0";

        // set as not controlled by ATR
        m_usrp->set_gpio_attr(gpio_state.bank, "CTRL", 0x00, gpio_state.gpio_mask);

        // set as output
        m_usrp->set_gpio_attr(gpio_state.bank, "DDR", gpio_state.gpio_mask, gpio_state.gpio_mask);

        // set to low by default
        m_usrp->set_gpio_attr(gpio_state.bank, "OUT", 0x00, gpio_state.gpio_mask);
#endif

    } else if (device_addrs.at(0).get("product", "") == "x410") {
        dectnrp_assert(nof_antennas == 1 || nof_antennas == 2 || nof_antennas == 4,
                       "Incorrect number of antennas.");

        if (nof_antennas == 1) {
            m_usrp->set_tx_subdev_spec(std::string("A:0"));
            m_usrp->set_rx_subdev_spec(std::string("A:0"));
        } else if (nof_antennas == 2) {
            m_usrp->set_tx_subdev_spec(std::string("A:0 A:1"));
            m_usrp->set_rx_subdev_spec(std::string("A:0 A:1"));
        } else {
            m_usrp->set_tx_subdev_spec(std::string("A:0 A:1 B:0 B:1"));
            m_usrp->set_rx_subdev_spec(std::string("A:0 A:1 B:0 B:1"));
        }

#ifdef RADIO_HW_IMPLEMENTS_GPIO_TOGGLE
        // returns GPIO0 and GPIO1, these are the front-panel connectors (only X410 has two)
        const auto banks = m_usrp->get_gpio_src_banks();

        // sources of the GPIOs of specific front-panel connector
        auto gpio_src = m_usrp->get_gpio_src(banks.at(0));

        // overwrite source for first GPIO
        gpio_src.at(0) = "DB0_RF0";

        // update sources for all GPIOs, effectively only first GPIO
        m_usrp->set_gpio_src(banks.at(0), gpio_src);

        // set_gpio_attr set all 24 points of GPIO0 and GPIO1, thus the name "GPIOA" and not
        // "GPIO0A" or "GPIO1A"
        gpio_state.bank = "GPIOA";

        // set as not controlled by ATR
        m_usrp->set_gpio_attr(gpio_state.bank, "CTRL", 0x00, gpio_state.gpio_mask);

        // set as output
        m_usrp->set_gpio_attr(gpio_state.bank, "DDR", gpio_state.gpio_mask, gpio_state.gpio_mask);

        // set to low by default
        m_usrp->set_gpio_attr(gpio_state.bank, "OUT", 0x00, gpio_state.gpio_mask);
#endif

    } else {
        dectnrp_assert_failure("USRP device not supported");
    }

    dectnrp_assert(m_usrp->get_tx_num_channels() == nof_antennas,
                   "Number of TX channels is not the same as the number of antennas requested.");
    dectnrp_assert(m_usrp->get_rx_num_channels() == nof_antennas,
                   "Number of RX channels is not the same as the number of antennas requested.");
    dectnrp_assert(!(m_usrp->get_master_clock_rate() != static_cast<double>(master_clock_rate) &&
                     device_addrs.at(0).get("product", "") != "B210"),
                   "Master Clock set incorrectly");

    set_command_time();

    // m_usrp->set_time_source("internal");
    // m_usrp->set_clock_source("internal");

    double samp_rate_d = static_cast<double>(samp_rate);

    m_usrp->set_tx_rate(samp_rate_d);
    m_usrp->set_rx_rate(samp_rate_d);

    // assert
    if (m_usrp->get_tx_rate() != samp_rate_d) {
        // small mismatch for B210
        if (device_addrs.at(0).get("product", "") == "B210") {
            dectnrp_assert(std::round(m_usrp->get_tx_rate()) == samp_rate_d,
                           "TX sample rate not set correctly");
        } else {
            dectnrp_assert_failure("TX sample rate not set correctly");
        }
    }
    if (m_usrp->get_rx_rate() != samp_rate_d) {
        // small mismatch for B210
        if (device_addrs.at(0).get("product", "") == "B210") {
            dectnrp_assert(std::round(m_usrp->get_rx_rate()) == samp_rate_d,
                           "RX sample rate not set correctly");
        } else {
            dectnrp_assert_failure("RX sample rate not set correctly");
        }
    }

    // usrp->set_tx_spp();
    // usrp->set_rx_spp();

    set_freq_tc(HW_DEFAULT_FREQ_HZ);

    // per channel
    std::vector<size_t> channel_nums;
    for (uint32_t i = 0; i < nof_antennas; ++i) {
        channel_nums.push_back(i);

        m_usrp->set_tx_antenna("TX/RX", i);

        m_usrp->set_rx_antenna("TX/RX", i);
        // m_usrp->set_rx_antenna("RX2", i);

        // not all settings are supported by all devices
        try {
            m_usrp->set_rx_dc_offset(true, i);
        } catch (std::exception& e) {
            log_line("Exception during USRP configuration: {}" + std::string(e.what()));
        }

        // not all settings are supported by all devices
        try {
            m_usrp->set_rx_iq_balance(true, i);
        } catch (std::exception& e) {
            log_line("Exception during USRP configuration: {}" + std::string(e.what()));
        }
    }

    set_tx_power_ant_0dBFS_uniform_tc(-1000.0f);  // minimum TX power
    set_rx_power_ant_0dBFS_uniform_tc(1000.0f);   // minimum RX sensitivity

    // https://files.ettus.com/manual/page_general.html#general_tuning_rfsettling
    while (!m_usrp->get_rx_sensor("lo_locked").to_bool()) {
        common::watch_t::sleep<common::milli>(100);
    }

    m_usrp->clear_command_time();

    // create a transmit streamer
    // https://files.ettus.com/manual/structuhd_1_1stream__args__t.html
    uhd::stream_args_t stream_args_tx;
    stream_args_tx.cpu_format = "fc32";
    stream_args_tx.otw_format = "sc16";
    stream_args_tx.channels = channel_nums;
    // stream_args_tx.args["fullscale"]
    // stream_args_tx.args["peak"]
    // stream_args_tx.args["underflow_policy"]   = std::string("next_burst");
    // stream_args_tx.args["spp"]                = std::to_string(996);          // "When not
    // specified, the packets are always maximum frame size." stream_args_tx.args["noclear"]

    tx_stream = m_usrp->get_tx_stream(stream_args_tx);

    // create a receive streamer
    // https://files.ettus.com/manual/structuhd_1_1stream__args__t.html
    uhd::stream_args_t stream_args_rx;
    stream_args_rx.cpu_format = "fc32";
    stream_args_rx.otw_format = "sc16";
    stream_args_rx.channels = channel_nums;
    // stream_args_rx.args["fullscale"]
    // stream_args_rx.args["peak"]
    // stream_args_rx.args["underflow_policy"]   = std::string("next_burst");
    // stream_args_rx.args["spp"]                = std::to_string(996);          // "When not
    // specified, the packets are always maximum frame size." stream_args_rx.args["noclear"]

    rx_stream = m_usrp->get_rx_stream(stream_args_rx);
}

void hw_usrp_t::start_threads_and_iq_streaming() {
    dectnrp_assert(!keep_running.load(std::memory_order_acquire), "keep_running already true");

    dectnrp_assert(0 < nof_antennas && nof_antennas <= nof_antennas_max,
                   "Number of antennas not set correctly.");
    dectnrp_assert(samp_rate > 0, "Sample rate not set correctly.");
    dectnrp_assert(tx_gap_samples > 0, "Minimum size of gap not set correctly.");

    // give the threads the okay to execute
    keep_running.store(true, std::memory_order_release);

    /* Correct order of thread startup:
     * 1) First TX async helper to detect potential errors.
     *
     * 2) TX thread. However, transmission timing should depend on RX sense of time which remains at
     * 0 as long as the RX thread is not running.
     *
     * 3) RX thread, which increases system time and only by that enables upper layers to start
     * transmission.
     */

    // start tx thread async helper thread
    if (!common::threads_new_rt_mask_custom(&thread_tx_async_helper,
                                            &work_tx_async_helper,
                                            this,
                                            hw_config.usrp_tx_async_helper_thread_config)) {
        dectnrp_assert_failure("USRP unable to start TX async helper thread.");
    }

    log_line(
        std::string("Thread Async Helper " +
                    common::get_thread_properties(thread_tx_async_helper,
                                                  hw_config.usrp_tx_async_helper_thread_config)));

    // start tx thread first
    if (!common::threads_new_rt_mask_custom(
            &thread_tx, &work_tx, this, hw_config.tx_thread_config)) {
        dectnrp_assert_failure("USRP unable to start TX work thread.");
    }

    log_line(std::string("Thread TX " +
                         common::get_thread_properties(thread_tx, hw_config.tx_thread_config)));

#ifdef RADIO_HW_SLEEP_BEFORE_STARTING_RX_THREAD_MS
    common::watch_t::sleep<common::milli>(RADIO_HW_SLEEP_BEFORE_STARTING_RX_THREAD_MS);
#endif

    // start rx thread second
    if (!common::threads_new_rt_mask_custom(
            &thread_rx, &work_rx, this, hw_config.rx_thread_config)) {
        dectnrp_assert_failure("USRP unable to start RX work thread.");
    }

    log_line(std::string("Thread RX " +
                         common::get_thread_properties(thread_rx, hw_config.rx_thread_config)));

    std::string str("USRP");
    str.append(" nof_antennas " + std::to_string(nof_antennas));
    str.append(" samp_rate " + std::to_string(samp_rate));
    str.append(" buffer_tx_pool.nof_buffer_tx " + std::to_string(buffer_tx_pool->nof_buffer_tx));
    str.append(" buffer_rx->ant_streams_length_samples " +
               std::to_string(buffer_rx->ant_streams_length_samples));
    log_line(str);

    str.clear();
    str.append("product " + device_addrs.at(0).get("product", ""));
    log_line(str);
}

void hw_usrp_t::set_command_time(const int64_t set_time) {
    if (set_time >= 0) {
        m_usrp->set_command_time(
            uhd::time_spec_t::from_ticks(set_time, static_cast<double>(samp_rate)));
    } else {
        m_usrp->clear_command_time();
    }
}

double hw_usrp_t::set_freq_tc(const double freq_Hz) {
    // https://dsp.stackexchange.com/questions/30562/large-spike-at-the-center-frequency-when-using-ettus-x310
    const uhd::tune_request_t tune_tx(
        freq_Hz, static_cast<double>(USRP_DIGITAL_LO_TUNING_OFFSET_FREQUENCY_HZ));
    const uhd::tune_request_t tune_rx(
        freq_Hz, static_cast<double>(USRP_DIGITAL_LO_TUNING_OFFSET_FREQUENCY_HZ));
    uhd::tune_result_t tune_result_tx, tune_result_rx;

    // TX and RX frequencies are the same, we return tune_result_tx, must be initialized
    tune_result_tx.target_rf_freq = HW_DEFAULT_FREQ_HZ;

    for (uint32_t i = 0; i < nof_antennas; ++i) {
        tune_result_tx = m_usrp->set_tx_freq(tune_tx, i);
        tune_result_rx = m_usrp->set_rx_freq(tune_rx, i);
    }

    return tune_result_tx.target_rf_freq;
}

float hw_usrp_t::set_tx_power_ant_0dBFS_tc(const float power_dBm, const size_t idx) {
    const auto achievable_power_gain =
        gain_lut.get_achievable_power_gain_tx(power_dBm, m_usrp->get_tx_freq());

    // do not call hardware functions if there isn't a real change
    if (std::abs(achievable_power_gain.power_dBm - tx_power_ant_0dBFS.at(idx)) <
        gain_lut.gains_tx_dB_step) {
        return tx_power_ant_0dBFS.at(idx);
    }

    m_usrp->set_tx_gain(static_cast<double>(achievable_power_gain.gain_dB), idx);

    tx_power_ant_0dBFS.at(idx) = achievable_power_gain.power_dBm;

    return tx_power_ant_0dBFS.at(idx);
}

float hw_usrp_t::set_rx_power_ant_0dBFS_tc(const float power_dBm, const size_t idx) {
    const auto achievable_power_gain =
        gain_lut.get_achievable_power_gain_rx(power_dBm, m_usrp->get_tx_freq());

    // do not call hardware functions if there isn't a real change
    if (std::abs(achievable_power_gain.power_dBm - rx_power_ant_0dBFS.at(idx)) <
        gain_lut.gains_rx_dB_step) {
        return rx_power_ant_0dBFS.at(idx);
    }

    m_usrp->set_rx_gain(static_cast<double>(achievable_power_gain.gain_dB), idx);

    rx_power_ant_0dBFS.at(idx) = achievable_power_gain.power_dBm;

    return rx_power_ant_0dBFS.at(idx);
}

#ifdef RADIO_HW_IMPLEMENTS_GPIO_TOGGLE
void hw_usrp_t::toggle_gpio_tc() {
    // in case an USRP does not support toggling
    if (!gpio_state.bank.empty()) {
        if (gpio_state.high) {
            m_usrp->set_gpio_attr(gpio_state.bank, "OUT", 0x00, gpio_state.gpio_mask);
        } else {
            m_usrp->set_gpio_attr(
                gpio_state.bank, "OUT", gpio_state.gpio_mask, gpio_state.gpio_mask);
        }

        gpio_state.high = !gpio_state.high;
    }
}
#endif

void hw_usrp_t::pps_wait_for_next() const {
    const auto last_pps = m_usrp->get_time_last_pps();
    while (last_pps == m_usrp->get_time_last_pps()) {
        common::watch_t::sleep<common::milli>(1);
    }
}

void hw_usrp_t::pps_set_full_sec_at_next_pps_and_wait_until_it_passed() {
    pps_wait_for_next();

    const int64_t full_sec = pps_time_base_sec_in_one_second();

    m_usrp->set_time_next_pps(uhd::time_spec_t(full_sec));

    // time is now undefined, sleep until next pps when time will be set
    common::watch_t::sleep<common::milli>(1200);

    pps_wait_for_next();

    /* The PPS just happened. Immediately measure the operating system time and save the offset
     * between the full second of the operating system and the PPS.
     */
    const int64_t A =
        common::watch_t::get_elapsed_since_epoch<int64_t, common::nano>() % 1000000000;
    full_second_to_pps_measured_samples = A * static_cast<int64_t>(samp_rate) / 1000000000;

    dectnrp_assert(full_second_to_pps_measured_samples < samp_rate, "ill-defined");
}

void hw_usrp_t::work_stop() {
    dectnrp_assert(keep_running.load(std::memory_order_acquire), "keep_running already false");

    // this variable will be read in all threads and leads to them leaving their loops
    keep_running.store(false, std::memory_order_release);

    pthread_join(thread_rx, NULL);
    pthread_join(thread_tx, NULL);
    pthread_join(thread_tx_async_helper, NULL);

    std::string str("USRP");
    str.append(" TX Sent " + std::to_string(tx_stats.buffer_tx_sent));
    str.append(" TX Sent Consecutive " + std::to_string(tx_stats.buffer_tx_sent_consecutive));
    str.append(" RX Samples " + std::to_string(buffer_rx->time_as_sample_cnt_64));
    str.append(
        " RX System Runtime " +
        std::to_string(buffer_rx->time_as_sample_cnt_64 / static_cast<int64_t>(samp_rate) / 60ULL) +
        " min " +
        std::to_string((buffer_rx->time_as_sample_cnt_64 / static_cast<int64_t>(samp_rate)) %
                       60ULL) +
        " sec");
    log_line(str);

    // each TX Buffer
    for (auto& elem : buffer_tx_pool->buffer_tx_vec) {
        log_lines(elem->report_stop());
    }
}

void* hw_usrp_t::work_tx_async_helper(void* hw_usrp) {
    // pointer to our hw_usrp instance that was passed to the thread executing this function as
    // argument this
    hw_usrp_t* calling_instance = reinterpret_cast<hw_usrp_t*>(hw_usrp);

    // for readability
    auto& keep_running = calling_instance->keep_running;
    auto& tx_stream = calling_instance->tx_stream;

    // collect error messages here
    uhd::async_metadata_t async_md;

    // async message receive is blocking
    const double timeout = 0.1;

    // stats (unused if logging is turned off)
    [[maybe_unused]] uint64_t ACK = 0;
    [[maybe_unused]] uint64_t UF = 0;
    [[maybe_unused]] uint64_t SE = 0;
    [[maybe_unused]] uint64_t TE = 0;
    [[maybe_unused]] uint64_t UFIP = 0;
    [[maybe_unused]] uint64_t SEIB = 0;
    [[maybe_unused]] uint64_t UP = 0;
    [[maybe_unused]] uint64_t OTHER = 0;

    // print from time to time
    common::watch_t watch;

    while (keep_running.load(std::memory_order_acquire)) {
        if (tx_stream->recv_async_msg(async_md, timeout)) {
            // handle the error codes
            switch (async_md.event_code) {
                case uhd::async_metadata_t::EVENT_CODE_BURST_ACK:
                    ++ACK;
                    break;
                case uhd::async_metadata_t::EVENT_CODE_UNDERFLOW:
                    ++UF;
                    break;
                case uhd::async_metadata_t::EVENT_CODE_SEQ_ERROR:
                    ++SE;
                    break;
                case uhd::async_metadata_t::EVENT_CODE_TIME_ERROR:
                    ++TE;
                    break;
                case uhd::async_metadata_t::EVENT_CODE_UNDERFLOW_IN_PACKET:
                    ++UFIP;
                    break;
                case uhd::async_metadata_t::EVENT_CODE_SEQ_ERROR_IN_BURST:
                    ++SEIB;
                    break;
                case uhd::async_metadata_t::EVENT_CODE_USER_PAYLOAD:
                    ++UP;
                    break;
                default:
                    ++OTHER;
                    break;
            }
        }

        if (watch.is_elapsed<common::seconds>(5)) {
            calling_instance->log_line(
                "USRP ACK: " + std::to_string(ACK) + "  UF: " + std::to_string(UF) +
                "  SE: " + std::to_string(SE) + "  TE: " + std::to_string(TE) +
                "  UFIP: " + std::to_string(UFIP) + "  SEIB: " + std::to_string(SEIB) +
                "  UP: " + std::to_string(UP) + "  OTHER: " + std::to_string(OTHER));
            watch.reset();
        }
    }

    return NULL;
}

void* hw_usrp_t::work_tx(void* hw_usrp) {
    // pointer to our hw_usrp instance that was passed to the thread executing this function as
    // argument this
    hw_usrp_t* calling_instance = reinterpret_cast<hw_usrp_t*>(hw_usrp);

    // for readability
    auto& buffer_tx_pool = *calling_instance->buffer_tx_pool.get();
    auto& keep_running = calling_instance->keep_running;
    const auto tx_gap_samples = calling_instance->tx_gap_samples;
    auto buffer_tx_vec = buffer_tx_pool.get_buffer_tx_vec();

    // hw usrp parameters for readability
    auto& usrp = calling_instance->m_usrp;
    auto& tx_stream = calling_instance->tx_stream;
    auto& tx_stats = calling_instance->tx_stats;

    // reset statistics
    tx_stats.buffer_tx_sent = 0;
    tx_stats.buffer_tx_sent_consecutive = 0;

#ifdef RADIO_HW_IMPLEMENTS_TX_BURST_LEADING_ZERO
    // how many zeros do we have to prefix?
    const int64_t N_leading_zero_64 =
        static_cast<int64_t>(calling_instance->samp_rate) *
        static_cast<int64_t>(calling_instance->hw_config.tx_burst_leading_zero_us) /
        static_cast<int64_t>(1000000);

    // similiar to https://github.com/EttusResearch/uhd/blob/master/host/examples/benchmark_rate.cpp
    const std::vector<cf32_t> leading_zero(N_leading_zero_64, cf32_t{0.0f, 0.0f});
    const std::vector<const cf32_t*> leading_zero_vec(buffer_tx_vec[0]->nof_antennas,
                                                      &leading_zero[0]);
#endif

#ifdef RADIO_HW_IMPLEMENTS_TX_TIME_ADVANCE
    const int64_t tx_time_advance_samples =
        static_cast<int64_t>(calling_instance->hw_config.tx_time_advance_samples);
#endif

    // working copy
    std::vector<void*> ant_streams(calling_instance->nof_antennas);

    // sample rate needed for to-tick conversion
    const double rate = usrp->get_tx_rate();

    // maximum number of samples per call of send()
    const uint32_t spp_max = tx_stream->get_max_num_samps();

    // timeout when calling send(), should ideally never timeout
    const double send_timeout = 0.01;

    // clang-format off
    calling_instance->log_line("USRP Work TX Thread: rate:         " + std::to_string(rate / 1e6) + " MHz");
    calling_instance->log_line("USRP Work TX Thread: nof channels: " + std::to_string(tx_stream->get_num_channels()));
    calling_instance->log_line("USRP Work TX Thread: spp samples:  " + std::to_string(tx_stream->get_max_num_samps()));
    calling_instance->log_line("USRP Work TX Thread: spp length:   " + std::to_string(static_cast<double>(tx_stream->get_max_num_samps()) / rate * 1e6) + " us");
    calling_instance->log_line("USRP Work TX Thread: send_timeout: " + std::to_string(rate / 1e6) + " us");
    // clang-format on

    // each packet has a 64 bit number, we transmit them in exact order
    int64_t tx_order_id_expected = 0;

    // we check keep_running approximately every 100ms when there are no packets
    const uint32_t nof_rounds = 10;

    // execute until external flag is set
    while (keep_running.load(std::memory_order_acquire)) {
        for (uint32_t round = 0; round < nof_rounds; ++round) {
            // wait for specific packet to become available
            int32_t next_buffer_tx = buffer_tx_pool.wait_for_specific_tx_order_id_to(
                tx_order_id_expected, BUFFER_TX_WAIT_NON_CRITICAL_TIMEOUT_MS);

            // since above function can timeout, we have to check its return value
            if (next_buffer_tx < 0) {
                continue;
            }

#ifdef RADIO_HW_IMPLEMENTS_TX_TIME_ADVANCE
            // artificially move tx time forward
            buffer_tx_vec[next_buffer_tx]->buffer_tx_meta.tx_time_64 -= tx_time_advance_samples;
#endif

            // when should the packet be transmitted?
#ifdef RADIO_HW_IMPLEMENTS_TX_BURST_LEADING_ZERO
            const int64_t tx_burst_start_time =
                buffer_tx_vec[next_buffer_tx]->buffer_tx_meta.tx_time_64 - leading_zero.size();
#else
            const int64_t tx_burst_start_time =
                buffer_tx_vec[next_buffer_tx]->buffer_tx_meta.tx_time_64;
#endif
            // construct the stream command for a burst transmission
            uhd::tx_metadata_t tx_md;
            tx_md.start_of_burst = true;
            tx_md.end_of_burst = false;
            tx_md.has_time_spec = true;
            tx_md.time_spec = uhd::time_spec_t::from_ticks(tx_burst_start_time, rate);

#ifdef RADIO_HW_IMPLEMENTS_TX_BURST_LEADING_ZERO
            if (N_leading_zero_64 > 0) {
                // send zeros
                tx_stream->send(leading_zero_vec, N_leading_zero_64, tx_md, send_timeout);

                // next call requires these settings
                tx_md.start_of_burst = false;
                tx_md.has_time_spec = false;
            }
#endif
            /* At this point we know that we have at least one packet to transmit. However, there
             * might be multiple packets with small or no gaps between them. These packets get
             * transmitted consecutively in a single UHD burst. Between the packets, we send zeros.
             * This way we avoid unnecessary TX/RX switches.
             */
            bool consecutive_transmission_on = true;
            while (consecutive_transmission_on) {
                // make next packet current packet, logic is needed for consecutive transmissions
                const uint32_t current_buffer_tx = next_buffer_tx;

                // this buffer is as good as sent ...
                if (buffer_tx_vec[current_buffer_tx]->buffer_tx_meta.tx_order_id_expect_next < 0) {
                    // ... so either increase ID counter for next packet by one ...
                    ++tx_order_id_expected;
                } else {
                    // ... or overwrite if firmware wants to change the order
                    tx_order_id_expected =
                        buffer_tx_vec[current_buffer_tx]->buffer_tx_meta.tx_order_id_expect_next;
                }

                ++tx_stats.buffer_tx_sent;

                // how long will the current packet ultimately be?
                uint32_t tx_length_samples = buffer_tx_vec[current_buffer_tx]->tx_length_samples;

                dectnrp_assert(spp_max < tx_length_samples,
                               "tx_length_samples too small, reduce spp");

                // when does the current packet end on the global time axis?
                const int64_t tx_time_in_samples_end =
                    buffer_tx_vec[current_buffer_tx]->buffer_tx_meta.tx_time_64 +
                    static_cast<int64_t>(tx_length_samples);

                // counter of samples sent
                uint32_t tx_length_samples_cnt = 0;

                // backpressure
                buffer_tx_vec[current_buffer_tx]->wait_for_samples_busy_nto(spp_max);

                // update pointers
                buffer_tx_vec[current_buffer_tx]->get_ant_streams_offset(ant_streams, 0);

                // send the one maximum sized chunk that we have at least
                tx_length_samples_cnt += tx_stream->send(ant_streams, spp_max, tx_md, send_timeout);

                // next call requires these settings
                tx_md.start_of_burst = false;
                tx_md.has_time_spec = false;

                // number of samples left to transmit
                uint32_t tx_length_samples_residual = tx_length_samples - tx_length_samples_cnt;

                // transmit until we have no more maximum sized chunks left
                while (tx_length_samples_residual > spp_max) {
                    // backpressure
                    buffer_tx_vec[current_buffer_tx]->wait_for_samples_busy_nto(
                        tx_length_samples_cnt + spp_max);

                    // update pointers
                    buffer_tx_vec[current_buffer_tx]->get_ant_streams_offset(ant_streams,
                                                                             tx_length_samples_cnt);

                    // send
                    tx_length_samples_cnt +=
                        tx_stream->send(ant_streams, spp_max, tx_md, send_timeout);

                    // update
                    tx_length_samples_residual = tx_length_samples - tx_length_samples_cnt;
                }

                dectnrp_assert(tx_length_samples - tx_length_samples_cnt > 0 &&
                                   (tx_length_samples - tx_length_samples_cnt) <= spp_max,
                               "Incorrect number of samples in final chunk");

                /* At this point, we know that we still have some samples in the current buffer left
                 * for transmission. However, we can't transmit them just yet, because the final
                 * UHD-packet of a UHD-burst has to be marked with tx_md.end_of_burst = true. We
                 * must check if another packet starts right after the current one to remain within
                 * the same burst.
                 */

                // if not last packet we wait for some time
                if (buffer_tx_vec[current_buffer_tx]->buffer_tx_meta.busy_wait_us > 0) {
                    // busy wait some time for next buffer
                    next_buffer_tx = buffer_tx_pool.wait_for_specific_tx_order_id_busy_to(
                        tx_order_id_expected,
                        buffer_tx_vec[current_buffer_tx]->buffer_tx_meta.busy_wait_us);
                } else {
                    // check if packet with next tx_order_id is already available
                    next_buffer_tx =
                        buffer_tx_pool.get_specific_tx_order_id_if_available(tx_order_id_expected);
                }

                // assume there are no gap samples
                uint32_t tx_gap_samples_this = 0;

                // packet available?
                if (next_buffer_tx >= 0) {
#ifdef RADIO_HW_IMPLEMENTS_TX_TIME_ADVANCE
                    // artificially move tx time forward
                    buffer_tx_vec[next_buffer_tx]->buffer_tx_meta.tx_time_64 -=
                        tx_time_advance_samples;
#endif
                    // transmission time of potential consecutive transmission
                    const int64_t next_buffer_tx_time =
                        buffer_tx_vec[next_buffer_tx]->buffer_tx_meta.tx_time_64;

                    dectnrp_assert(tx_time_in_samples_end <= next_buffer_tx_time,
                                   "packet TX times are out of order {} {} {} {}",
                                   current_buffer_tx,
                                   tx_time_in_samples_end,
                                   next_buffer_tx,
                                   next_buffer_tx_time);

                    // gap between this buffer's final sample and the next buffers first sample
                    tx_gap_samples_this =
                        static_cast<uint32_t>(next_buffer_tx_time - tx_time_in_samples_end);

                    // gap too large? burst ends here
                    if (tx_gap_samples_this > tx_gap_samples) {
                        tx_gap_samples_this = 0;
                        tx_md.end_of_burst = true;
                        consecutive_transmission_on = false;
                    }
                    // gap small enough? we transmit consecutively
                    else {
                        ++tx_stats.buffer_tx_sent_consecutive;

                        // zero gap samples at the end of the buffer so we don't send garbage
                        buffer_tx_vec[current_buffer_tx]->set_zero(tx_length_samples,
                                                                   tx_gap_samples_this);
                    }
                }
                // no packet found, burst ends here
                else {
                    tx_md.end_of_burst = true;
                    consecutive_transmission_on = false;
                }

                // backpressure until PHY has created all samples
                buffer_tx_vec[current_buffer_tx]->wait_for_samples_busy_nto(tx_length_samples);

                // artificially increase size of packet by the number of gap samples
                tx_length_samples += tx_gap_samples_this;

                // transmit until we have no more samples left
                while (tx_length_samples_cnt < tx_length_samples) {
                    tx_length_samples_residual = tx_length_samples - tx_length_samples_cnt;

                    const uint32_t n_samples_send_this =
                        std::min(spp_max, tx_length_samples_residual);

                    buffer_tx_vec[current_buffer_tx]->get_ant_streams_offset(ant_streams,
                                                                             tx_length_samples_cnt);

                    tx_length_samples_cnt +=
                        tx_stream->send(ant_streams, n_samples_send_this, tx_md, send_timeout);
                }

#ifdef RADIO_HW_AGC_IMMEDIATE_OR_AT_PACKET_END
                const int64_t agc_time_64 = -1;
#else
                // when does the current packet end on the global time axis?
                const int64_t agc_time_64 = tx_time_in_samples_end;
#endif

                // TX AGC
                if (buffer_tx_vec[current_buffer_tx]->buffer_tx_meta.tx_power_adj_dB.has_value()) {
                    calling_instance->set_command_time(agc_time_64);
                    calling_instance->adjust_tx_power_ant_0dBFS_tc(
                        buffer_tx_vec[current_buffer_tx]->buffer_tx_meta.tx_power_adj_dB.value());
                }

                // RX AGC
                if (buffer_tx_vec[current_buffer_tx]->buffer_tx_meta.rx_power_adj_dB.has_value()) {
                    calling_instance->set_command_time(agc_time_64);
                    calling_instance->adjust_rx_power_ant_0dBFS_tc(
                        buffer_tx_vec[current_buffer_tx]->buffer_tx_meta.rx_power_adj_dB.value());
                }

                // release TX buffer
                buffer_tx_vec[current_buffer_tx]->set_transmitted_or_abort();

            }  // consecutive_transmission_on
        }  // nof_rounds
    }  // burst time

    return nullptr;
}

void* hw_usrp_t::work_rx(void* hw_usrp) {
    // pointer to our hw_usrp instance that was passed to the thread executing this function as
    // argument this
    hw_usrp_t* calling_instance = reinterpret_cast<hw_usrp_t*>(hw_usrp);

    // synchronize PPS across all devices
    calling_instance->pps_set_full_sec_at_next_pps_and_wait_until_it_passed();

    // hw parameters for readability
    auto& buffer_rx = *calling_instance->buffer_rx;
    auto& keep_running = calling_instance->keep_running;

    // hw usrp parameters for readability
    auto& usrp = calling_instance->m_usrp;
    auto& rx_stream = calling_instance->rx_stream;

    // sample rate needed for to-tick conversion
    const double rate = usrp->get_rx_rate();

    // maximum number of samples per call of recv()
    const uint32_t spp_max = rx_stream->get_max_num_samps();

    // timeout when calling recv(), should ideally never timeout
    const double recv_timeout = 0.01;

    // initial delay until streaming begins, irrelevant for later latency
    const double rx_delay = 5000.0 / 1.0e6;

    // clang-format off
    calling_instance->log_line("USRP Work RX Thread: rate:         " + std::to_string(rate / 1e6) + " MHz");
    calling_instance->log_line("USRP Work RX Thread: nof channels: " + std::to_string(rx_stream->get_num_channels()));
    calling_instance->log_line("USRP Work RX Thread: spp samples:  " + std::to_string(static_cast<long unsigned int>(spp_max)));
    calling_instance->log_line("USRP Work RX Thread: spp length:   " + std::to_string(static_cast<double>(spp_max) / rate * 1e6) + " us");
    calling_instance->log_line("USRP Work RX Thread: rx_delay:     " + std::to_string(rx_delay * 1e6) + " us");
    calling_instance->log_line("USRP Work RX Thread: recv_timeout: " + std::to_string(recv_timeout * 1e6) + " us");
    // clang-format on

    // updated for every new call of rx_streamer->recv()
    int64_t time_of_first_sample;
    uint32_t nof_new_samples;

    std::vector<void*> ant_streams(calling_instance->nof_antennas);

    dectnrp_assert(ant_streams.size() == buffer_rx.nof_antennas, "incorrect number of antennas");

    // init pointer into streaming buffer
    buffer_rx.get_ant_streams_next(ant_streams, 0, 0);

    // meta data for every call of recv()
    uhd::rx_metadata_t md;

    // construct stream command
    uhd::stream_cmd_t cmd(uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS);
    cmd.num_samps = spp_max;  // should be irrelevant as we stream continuously
    cmd.time_spec = usrp->get_time_now() + uhd::time_spec_t(rx_delay);
    cmd.stream_now = false;  // if set to true, UHD will be probably hiccup at startup

    // we check keep_running approximately every 100ms
    const uint32_t nof_rounds = static_cast<uint32_t>(rate) / spp_max / 10;

    // start streaming, recv() must now be called ASAP
    rx_stream->issue_stream_cmd(cmd);

    while (keep_running.load(std::memory_order_acquire)) {
        // to reduce calls of keep_running
        for (uint32_t round = 0; round < nof_rounds; ++round) {
            try {
                nof_new_samples = rx_stream->recv(ant_streams, spp_max, md, recv_timeout);
            } catch (uhd::io_error& e) {
                std::cerr << "[] Caught an IO exception. " << std::endl;
                std::cerr << e.what() << std::endl;
                keep_running.store(false, std::memory_order_release);
                return NULL;
            }

            // handle the error codes
            switch (md.error_code) {
                case uhd::rx_metadata_t::ERROR_CODE_NONE:
                    {
                        // We cannot assume md.time_spec to be continuous as recv() might fail
                        // (overflow, sequence error etc.). Therefore, we have to recalculate the
                        // time in clock counts each time.
                        time_of_first_sample = static_cast<int64_t>(md.time_spec.to_ticks(rate));

                        // advance pointer for next call of rx_stream->recv()
                        buffer_rx.get_ant_streams_next(
                            ant_streams, time_of_first_sample, nof_new_samples);

                        break;
                    }

                // ERROR_CODE_OVERFLOW can indicate overflow or sequence error
                case uhd::rx_metadata_t::ERROR_CODE_OVERFLOW:
                    if (md.out_of_sequence) {
                        std::cerr << "HW/USRP: Detected RX sequence error." << std::endl;
                    } else {
                        std::cerr << "HW/USRP: Detected RX overflow." << std::endl;
                    }
                    break;

                case uhd::rx_metadata_t::ERROR_CODE_LATE_COMMAND:
                    std::cerr << "HW/USRP: RX error: " << md.strerror() << ", restart streaming..."
                              << std::endl;
                    cmd.time_spec = usrp->get_time_now() + uhd::time_spec_t(rx_delay);
                    cmd.stream_now = false;
                    rx_stream->issue_stream_cmd(cmd);
                    break;

                case uhd::rx_metadata_t::ERROR_CODE_TIMEOUT:
                    std::cerr << "HW/USRP: RX error: " << md.strerror() << ", continuing..."
                              << std::endl;
                    break;

                // Otherwise, it's an error
                default:
                    std::cerr << "[] RX error: " << md.strerror() << std::endl;
                    std::cerr << "[] Unexpected error on recv, continuing..." << std::endl;
                    break;
            }
        }
    }

    // we want to shut down gracefully, so issue a stop command ...
    rx_stream->issue_stream_cmd(uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS);

    // ... and keep receiving until EOB flag is set. We don't advance the pointers during this
    // period.
    while (!md.end_of_burst) {
        try {
            // just read samples to empty buffer, don't advance pointers
            rx_stream->recv(ant_streams, spp_max, md, recv_timeout);
        } catch (uhd::io_error& e) {
            std::cerr << "[] Caught an IO exception. " << std::endl;
            std::cerr << e.what() << std::endl;
            keep_running.store(false, std::memory_order_release);
            return NULL;
        }
    }

    return nullptr;
}

}  // namespace dectnrp::radio
