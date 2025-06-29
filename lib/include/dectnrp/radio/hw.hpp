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

#include <pthread.h>

#include <array>
#include <atomic>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>

#include "dectnrp/common/ant.hpp"
#include "dectnrp/common/layer/layer_unit.hpp"
#include "dectnrp/radio/antenna_array.hpp"
#include "dectnrp/radio/buffer_rx.hpp"
#include "dectnrp/radio/buffer_tx.hpp"
#include "dectnrp/radio/buffer_tx_pool.hpp"
#include "dectnrp/radio/gain_lut.hpp"
#include "dectnrp/radio/hw_config.hpp"

#define RADIO_HW_SLEEP_BEFORE_STARTING_RX_THREAD_MS 100

// only refers to real hardware, simulator does not prefix zeros
#define RADIO_HW_IMPLEMENTS_TX_BURST_LEADING_ZERO

// only refers to real hardware, simulator does not send earlier
#define RADIO_HW_IMPLEMENTS_TX_TIME_ADVANCE

// only refers to real hardware, simulator has no GPIOs
// #define RADIO_HW_IMPLEMENTS_GPIO_TOGGLE

#ifdef RADIO_HW_IMPLEMENTS_GPIO_TOGGLE
#include "dectnrp/radio/pulse_config.hpp"
#endif

#define RADIO_HW_AGC_IMMEDIATE_OR_AT_PACKET_END

namespace dectnrp::radio {

class hw_t : public common::layer_unit_t {
    public:
        explicit hw_t(const hw_config_t& hw_config_)
            : common::layer_unit_t(hw_config_.json_log_key, hw_config_.id),
              hw_config(hw_config_) {
            keep_running.store(false, std::memory_order_release);
        };
        virtual ~hw_t() = default;

        hw_t() = delete;
        hw_t(const hw_t&) = delete;
        hw_t& operator=(const hw_t&) = delete;
        hw_t(hw_t&&) = delete;
        hw_t& operator=(hw_t&&) = delete;

        static constexpr double HW_DEFAULT_FREQ_HZ = 100.0e6;
        static constexpr uint32_t HW_TX_GAP_SAMPLES_MAX = 100;

        uint32_t get_nof_antennas_max() const { return nof_antennas_max; };
        uint32_t get_nof_antennas() const { return nof_antennas; };
        uint32_t get_samp_rate() const { return samp_rate; };

        /// must satisfy 0 < nof_antennas_ <= nof_antennas_max
        void set_nof_antennas(const uint32_t nof_antennas_);

        /**
         * \brief This function does NOT set samp_rate = samp_rate_in, as most SDRs have a limited
         * amount of possible sample rates. Instead, the hw picks the smallest possible sample rate
         * larger/equal samp_rate_in. The internal value of samp_rate can then be requested with
         * get_samp_rate(). It is up to the caller to verify whether the sample rate is acceptable.
         *
         * \param samp_rate_in requested sample rate in Hz
         */
        virtual void set_samp_rate(const uint32_t samp_rate_in) = 0;

        /**
         * \brief If hw detects a gap smaller or equal to tx_gap_samples_ between consecutive TX
         * buffer transmissions, if will fill the gap with zeros. In this period of time, the RX
         * path remains detached from the antenna. This approach helps stabilizing TX buffer
         * transmission.
         *
         * \param tx_gap_samples_ maximum width of gap in samples
         */
        void set_tx_gap_samples(const uint32_t tx_gap_samples_);

        /**
         * \brief Call after nof antennas and sample rate have been negotiated. Creates internal
         * buffers for streaming. Actual buffer size can depend on device type, therefore virtual.
         *
         * \param ant_streams_length_samples_max maximum transmission lengths
         */
        virtual void initialize_buffer_tx_pool(const uint32_t ant_streams_length_samples_max) = 0;

        /**
         * \brief Call after nof antennas and sample rate have been negotiated. Creates internal
         * buffers for streaming. Actual buffer size can depend on device type, therefore virtual.
         *
         * \param ant_streams_length_samples RX ring buffer length
         */
        virtual void initialize_buffer_rx(const uint32_t ant_streams_length_samples) = 0;

        /**
         * \brief Call after nof antennas and sample rate have been negotiated. Puts device into a
         * state so that TX/RX threads can be started.
         */
        virtual void initialize_device() = 0;

        /**
         * \brief Called after initialize_device(). Starts threads to exchange information with
         * radio device.
         */
        virtual void start_threads_and_iq_streaming() = 0;

        /**
         * \brief Command time of timed commands, all functions ending with tc.
         *
         * \param set_time is < 0, command will be executed asap
         */
        virtual void set_command_time(const int64_t set_time = -1) = 0;

        /**
         * \brief Call after initialize_device(). Sets and returns the closest center frequency
         * possible, same for TX and RX. tc = timed command
         *
         * \param freq_Hz frequency in Hz
         * \return actual frequency hw has set in Hz
         */
        virtual double set_freq_tc(const double freq_Hz) = 0;

        /// get current TX power at 0dBFS per antenna (value is the same for all antennas)
        float get_tx_power_ant_0dBFS() const { return tx_power_ant_0dBFS; };

        /**
         * \brief Set TX power at 0dBFS. Call after initialize_device(). Internally sets the PA
         * gain. As input from PHY, we assume a complex exponential function with 0dBFS full scale
         * (i.e. DAC dynamic range for both real and imag fully utilized, V_RMS for both real and
         * imag is 1/sqrt(2), V_peakpeak for both real and imag is 1-(-1)=2, V_RMS of the complex
         * signal is 1). It returns the achieved passband transmit power in dBm per antenna. The
         * power depends on the current frequency setting, and is capped in both directions. tc =
         * timed command
         *
         * \param power_dBm power at 0dBFS (usually between -40 and 20 dBm)
         * \return actual TX power at 0dBFS
         */
        virtual float set_tx_power_ant_0dBFS_tc(const float power_dBm) = 0;

        /**
         * \brief For a software AGC, it is easier to have relative gain changes in dB. When
         * requesting a change of 0, returns the current value of tx_power_ant_0dBFS. tc = timed
         * command
         *
         * \param adj_dB adjustment in dBm
         * \return actual TX power at 0dBFS
         */
        float adjust_tx_power_ant_0dBFS_tc(const float adj_dB);

        /// get current RX power at 0dBFS per antenna (value is not the same for all antennas)
        const common::ant_t& get_rx_power_ant_0dBFS() const { return rx_power_ant_0dBFS; };

        /**
         * \brief Set RX power at 0dBFS. Call after initialize_device(). Internally sets the LNA
         * gain. As passband input at the antenna, we assume a single sinusoid slightly offset from
         * the center frequency, leading to a complex exponential function with 0dBFS full scale on
         * the PHY (i.e. DAC dynamic range for both real and imag fully utilized, V_RMS for both
         * real and imag is 1/sqrt(2), V_peakpeak for both real and imag is 1-(-1)=2, V_RMS of the
         * complex signal is 1). The power depends on the current frequency setting, and is capped
         * in both directions. tc = timed command
         *
         * \param power_dBm power at 0dBFS (usually between -75 and 0 dBm)
         * \return actual RX power at 0dBFS
         */
        virtual float set_rx_power_ant_0dBFS_tc(const float power_dBm, const size_t idx) = 0;

        /// set same value for all antennas
        const common::ant_t& set_rx_power_ant_0dBFS_uniform_tc(const float power_dBm);

        /**
         * \brief For a software AGC, it is easier to have relative gain changes in dB. When
         * requesting a change of 0, returns the current value of rx_power_ant_0dBFS. tc = timed
         * command
         *
         * \param adj_dB adjustment in dBm
         * \return actual RX power at 0dBFS
         */
        const common::ant_t& adjust_rx_power_ant_0dBFS_tc(const common::ant_t& adj_dB);

#ifdef RADIO_HW_IMPLEMENTS_GPIO_TOGGLE
        /**
         * \brief USRP can toggle GPIOs at specific times. This can be used to export the start of
         * beacons (and thus the network synchronization) on a hardware level. tc = timed command
         */
        virtual void toggle_gpio_tc() = 0;

        /// pulse with one rising and one falling edge
        void schedule_pulse_tc(const pulse_config_t& pulse_config);
#endif

        /**
         * \brief Wait for next PPS and once it occurred return as soon as possible. This way we
         * have almost a full second until the next PPS occurs.
         */
        virtual void pps_wait_for_next() const = 0;

        /**
         * \brief Set internal time counter at next PPS. Internally waits to the PPS, sets the time
         * and then waits for another PPS to avoid the undefined state of time between the two PPS.
         */
        virtual void pps_set_full_sec_at_next_pps_and_wait_until_it_passed() = 0;

        /**
         * \brief The buffers for TX and RX use two (real, imag) 32-bit floats per sample per
         * antenna. There must be an amplitude value for float at which the dynamic range of the
         * ADC/DAC is fully utilized. We assume +/-1.0f, which is the default value used by UHD.
         * This range of +/-1.0f is quantized with a hardware dependent number of bits. This number
         * must be known to PHY to set correct threshold for synchronization etc.
         *
         * \return number of ADC bits, taken from HW data sheet
         */
        uint32_t get_ADC_bits() const { return ADC_bits; };
        uint32_t get_DAC_bits() const { return DAC_bits; };

        /**
         * \brief Changing hardware properties does not happen instantaneously. Depending on what is
         * changed, different minimum settling times must be used on the MAC layer.
         */
        enum class tmin_t : uint32_t {
            freq = 0,
            gain,
            turnaround,
            CARDINALITY
        };

        /// generic settling time getter
        uint32_t get_tmin_samples(const tmin_t tmin) const;

        /// can be used to limit the CFO search range
        float get_ppm() const { return ppm; };

        int64_t get_pps_to_full_second_measured_samples() const {
            return samp_rate - full_second_to_pps_measured_samples;
        };

        /// buffer's public interfaces used by hardware and PHY
        std::unique_ptr<buffer_tx_pool_t> buffer_tx_pool;
        std::unique_ptr<buffer_rx_t> buffer_rx;

        const hw_config_t hw_config;

    protected:
        virtual void work_stop() override = 0;

        // ##################################################
        // hardware properties

        /// device dependent
        uint32_t nof_antennas_max{};
        uint32_t ADC_bits{};
        uint32_t DAC_bits{};
        std::array<uint32_t, std::to_underlying(tmin_t::CARDINALITY)> tmin_us{}, tmin_samples{};
        float ppm{};

        /**
         * \brief In an USRP, once the IQ signal has arrived in FPGA it is shortly buffered
         * until the target time is reached. At that moment, transmission is triggered in the
         * FPGA. However, it still may take a few nanoseconds until the signal actually arrives
         * at the antenna. We save that value in nanoseconds and let the TX thread calculate the
         * respective number of samples at the given sample rate and correct it.
         */
        int32_t time_advance_fpga2ant_samples{};

        /// must be negotiated with PHY
        uint32_t nof_antennas{};
        uint32_t samp_rate{};
        uint32_t tx_gap_samples{};

        /// look up table for gain at specific frequency and power
        gain_lut_t gain_lut{};

        /// current power settings
        float tx_power_ant_0dBFS{};
        common::ant_t rx_power_ant_0dBFS{};

        antenna_array_t antenna_array{};

        /// convert microseconds to a number of samples at the sample rate
        uint32_t get_samples_in_us(const uint32_t us) const;

        /// get closest full second for the time base in hw_config
        int64_t pps_time_base_sec_in_one_second() const;

        /// measured offset between the start of a full second and the internal PPS
        int64_t full_second_to_pps_measured_samples{};

        // ##################################################
        // threading

        /** \brief
         *
         * if keep_running is true, then keep threads running once their started
         * if keep_running is false, then stop the threads
         */
        std::atomic<bool> keep_running{};
};

}  // namespace dectnrp::radio
