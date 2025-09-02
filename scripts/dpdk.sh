#!/bin/bash

#
# Copyright 2023-present Maxim Penner
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

# dpdk-devbind.py --status

# Ettus source 0: https://kb.ettus.com/Getting_Started_with_DPDK_and_UHD
# Ettus source 1: https://files.ettus.com/manual/page_dpdk.html
#
# Running this script is only necessary for Intel X520 NICs, not Mellanox.
# See here: https://files.ettus.com/manual/page_dpdk.html#dpdk_system_configuration
#
#   Quote:
#   "Mellanox ConnectX 4 and later NICs do not use the vfio-pci driver, so this step is not necessary for them. They will use the standard driver in conjunction with libibverbs."
#   "For NICs that require vfio-pci (like Intel's X520), you'll want to use the dpdk-devbind.py script to the vfio-pci driver."

echo 'Disabling interfaces, setting up dpdk...'

ifconfig enp104s0f0 down
ifconfig enp104s0f1 down

modprobe vfio-pci

dpdk-devbind.py --bind=vfio-pci 0000:68:00.0
dpdk-devbind.py --bind=vfio-pci 0000:68:00.1
