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

#include <vector>

#include "dectnrp/common/multidim.hpp"

namespace dectnrp::radio::calibration::x410 {

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
    {-42.5, -40.5, -37.5, -32.5, -27.5, -23.0, -17.0, -13.0, -8.0, -3.0, 1.8, 6.8, 11.8},
    {-40.0, -35.0, -31.5, -26.5, -21.5, -16.5, -11.5, -6.8, -1.8, 3.1, 8.0, 12.8, 17.5},
    {-40.0, -37.5, -32.5, -27.5, -23.0, -18.0, -13.0, -8.0, -3.0, 1.7, 6.6, 11.5, 16.2},
    {-40.0, -35.0, -31.0, -26.5, -21.5, -16.5, -11.5, -6.5, -1.5, 3.4, 8.3, 13.2, 18.0},
    {-40.0, -37.0, -32.5, -27.0, -23.0, -18.0, -13.0, -7.5, -2.8, 2.1, 7.2, 12.1, 16.8},
    {-42.0, -41.0, -40.0, -38.0, -33.0, -28.0, -23.0, -18.0, -13.0, -8.0, -2.8, 2.0, 7.2},
    {-40.0, -38.0, -34.0, -28.0, -23.0, -18.5, -13.5, -8.0, -3.2, 1.8, 6.6, 9.2, 14},
    {-41.0, -39.0, -36.0, -32.0, -27.0, -22.0, -17.0, -11.8, -6.5, -1.5, 3.3, 8.3, 13.2},
    {-41.0, -39.0, -35.0, -31.0, -27.0, -21.0, -16.5, -11.0, -6.1, -0.5, 4.8, 10.0, 15.0},
    {-42.0, -41.0, -39.0, -35.0, -30.0, -25.0, -20.0, -15.0, -10.0, -5.0, 0.0, 5.0, 10.0},
    {-42.0, -42.0, -40.0, -37.0, -33.0, -28.0, -23.0, -17.5, -12.0, -6.7, -1.6, 3.6, 8.8},
    {-42.0, -41.0, -39.0, -37.0, -32.0, -27.0, -22.0, -17.0, -11.0, -6.0, -0.5, 4.6, 9.6}};

const static float gains_tx_dB_step = 1.0;

const static common::vec2d<float> gains_rx_dB = {{38.0, 0.0},
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
const static common::vec2d<float> powers_rx_dBm = {{-35.0, 3.0},
                                                   {-51.6, 8.4},
                                                   {-51.4, 8.6},
                                                   {-54.4, 5.6},
                                                   {-54.6, 5.4},
                                                   {-36.0, 24.0},
                                                   {-49.8, 11.2},
                                                   {-48.2, 11.8},
                                                   {-48.6, 11.4},
                                                   {-47.6, 12.4},
                                                   {-46.8, 13.2},
                                                   {-45.2, 14.8}};

const static float gains_rx_dB_step = 1.0;

}  // namespace dectnrp::radio::calibration::x410
