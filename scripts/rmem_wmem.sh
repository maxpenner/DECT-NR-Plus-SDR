#!/bin/bash

#
# Copyright 2023-2025 Maxim Penner
#
# This file is part of DECTNRP.
#
# DECTNRP is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as
# published by the Free Software Foundation, either version 3 of
# the License, or (at your option) any later version.
#
# DECTNRP is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU Affero General Public License for more details.
#
# A copy of the GNU Affero General Public License can be found in
# the LICENSE file in the top-level directory of this distribution
# and at http://www.gnu.org/licenses/.
#

# https://kb.ettus.com/USRP_Host_Performance_Tuning_Tips_and_Tricks#Adjust_Network_Buffers

echo 'Setting net.core.rmem_max and net.core.wmem_max...'

sysctl -w net.core.wmem_max=33554432
sysctl -w net.core.rmem_max=33554432
sysctl -w net.core.wmem_default=33554432
sysctl -w net.core.rmem_default=33554432

ethtool -G enp104s0f0 tx 4096 rx 4096
ethtool -G enp104s0f1 tx 4096 rx 4096
