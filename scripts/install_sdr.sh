#!/bin/bash

#
# Copyright 2025-2025 Maxim Penner
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

echo "Starting!"

# get the source code
sudo -u "$SUDO_USER" git clone --recurse-submodules https://github.com/maxpenner/DECT-NR-Plus-SDR

# build
cd DECT-NR-Plus-SDR
sudo -u "$SUDO_USER" mkdir build
cd build
sudo -u "$SUDO_USER" cmake ..
sudo -u "$SUDO_USER" make -j

echo "Finished!"
