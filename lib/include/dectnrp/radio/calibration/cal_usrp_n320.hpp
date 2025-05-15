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

#include <vector>

#include "dectnrp/common/multidim.hpp"

namespace dectnrp::radio::calibration::n320 {

const static std::vector<float> freqs_Hz = {
    0.5e9, 1.0e9, 1.5e9, 2.0e9, 2.5e9, 3.0e9, 3.5e9, 4.0e9, 4.5e9, 5.0e9, 5.5e9, 6.0e9};

const static common::vec2d<float> gains_tx_dB = {
    {0.0, 5.0, 10.0, 15.0, 20.0, 25.0, 30.0, 35.0, 40.0, 45.0, 50.0, 55.0, 60.0},
    {0.0, 5.0, 10.0, 15.0, 20.0, 25.0, 30.0, 35.0, 40.0, 45.0, 50.0, 55.0, 60.0},
    {0.0, 5.0, 10.0, 15.0, 20.0, 25.0, 30.0, 35.0, 40.0, 45.0, 50.0, 55.0, 60.0},
    {0.0, 5.0, 10.0, 15.0, 20.0, 25.0, 30.0, 35.0, 40.0, 45.0, 50.0, 55.0, 60.0},
    {0.0, 5.0, 10.0, 15.0, 20.0, 25.0, 30.0, 35.0, 40.0, 45.0, 50.0, 55.0, 60.0},
    {0.0, 5.0, 10.0, 15.0, 20.0, 25.0, 30.0, 35.0, 40.0, 45.0, 50.0, 55.0, 60.0},
    {0.0, 5.0, 10.0, 15.0, 20.0, 25.0, 30.0, 35.0, 40.0, 45.0, 50.0, 55.0, 60.0},
    {0.0, 5.0, 10.0, 15.0, 20.0, 25.0, 30.0, 35.0, 40.0, 45.0, 50.0, 55.0, 60.0},
    {0.0, 5.0, 10.0, 15.0, 20.0, 25.0, 30.0, 35.0, 40.0, 45.0, 50.0, 55.0, 60.0},
    {0.0, 5.0, 10.0, 15.0, 20.0, 25.0, 30.0, 35.0, 40.0, 45.0, 50.0, 55.0, 60.0},
    {0.0, 5.0, 10.0, 15.0, 20.0, 25.0, 30.0, 35.0, 40.0, 45.0, 50.0, 55.0, 60.0},
    {0.0, 5.0, 10.0, 15.0, 20.0, 25.0, 30.0, 35.0, 40.0, 45.0, 50.0, 55.0, 60.0}};

const static common::vec2d<float> powers_tx_dBm = {
    {-35.0, -30.0, -25.0, -20.0, -15.0, -10.0, -5.0, 0.0, 5.0, 10.0, 15.0, 18.0, 20.0},
    {-36.0, -28.0, -22.0, -18.0, -12.0, -8.0, -2.0, 6.0, 11.0, 15.0, 13.0, 17.0, 19.0},
    {-37.0, -32.0, -27.0, -22.0, -17.0, -12.0, -7.0, -2.0, 3.0, 8.0, 13.0, 17.0, 19.0},
    {-35.0, -30.0, -25.0, -20.0, -15.0, -10.0, -5.0, 0.0, 5.0, 10.0, 14.0, 17.0, 19.0},
    {-36.0, -31.0, -26.0, -22.0, -16.0, -12.0, -7.0, 1.0, 5.0, 11.0, 13.0, 17.0, 19.0},
    {-36.0, -31.0, -26.0, -21.0, -16.0, -11.0, -6.0, -1.0, 4.0, 9.0, 14.0, 17.0, 19.0},
    {-37.0, -33.0, -28.0, -23.0, -17.0, -13.0, -8.0, -1.0, 3.0, 9.0, 13.0, 16.0, 18.0},
    {-40.0, -35.0, -30.0, -25.0, -20.0, -15.0, -10.0, -5.0, 0.0, 5.0, 10.0, 15.0, 17.0},
    {-37.0, -32.0, -27.0, -22.0, -17.0, -12.0, -7.0, -2.0, 3.0, 8.0, 13.0, 16.0, 18.0},
    {-40.0, -35.0, -29.0, -24.0, -19.0, -14.0, -9.0, -3.0, 2.0, 7.0, 12.0, 16.0, 18.0},
    {-44.0, -39.0, -34.0, -28.0, -23.0, -18.0, -13.0, -8.0, -2.0, 3.0, 8.0, 13.0, 15.0},
    {-47.0, -42.0, -37.0, -32.0, -26.0, -21.0, -15.0, -10.0, -5.0, 0.0, 5.0, 10.0, 15.0}};

const static float gains_tx_dB_step = 1.0;

const static common::vec2d<float> gains_rx_dB = {{60.0, 0.0},
                                                 {60.0, 0.0},
                                                 {60.0, 0.0},
                                                 {60.0, 0.0},
                                                 {60.0, 0.0},
                                                 {60.0, 0.0},
                                                 {60.0, 0.0},
                                                 {60.0, 0.0},
                                                 {60.0, 0.0},
                                                 {60.0, 0.0},
                                                 {60.0, 0.0},
                                                 {60.0, 0.0}};

/// measured at TX/RX
const static common::vec2d<float> powers_rx_dBm = {{-43.5, 16.5},
                                                   {-45.3, 14.7},
                                                   {-44.3, 15.7},
                                                   {-42.9, 17.1},
                                                   {-43.3, 16.7},
                                                   {-42.9, 17.1},
                                                   {-43.1, 16.9},
                                                   {-40.6, 19.4},
                                                   {-41.0, 19.0},
                                                   {-41.2, 18.8},
                                                   {-38.2, 21.8},
                                                   {-32.5, 27.5}};

const static float gains_rx_dB_step = 1.0;

}  // namespace dectnrp::radio::calibration::n320
