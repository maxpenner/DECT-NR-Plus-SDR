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

# current directory
cwd=$(pwd)

echo "This script automates the installation of all SDR dependencies."
echo ""
echo "The script must be run with sudo or as root:"
echo "    sudo ./install_dependencies.sh"
echo "All dependencies build from source will be downloaded into the current directory:"
echo "    $cwd"
echo "" 

###############################################
# running with sudo or as root?
###############################################

error_handler () {
   echo "Error: $1 Exiting."
   exit -1
}

# https://serverfault.com/questions/37829
if [[ $(/usr/bin/id -u) -ne 0 ]]; then
    error_handler "Script must run with sudo or as root."
fi

###############################################
# indicate installation process has started
###############################################

echo "Starting!"

###############################################
# update the package lists
###############################################

apt-get update

###############################################
# install the Universal Hardware Driver (UHD)
###############################################

# https://files.ettus.com/manual/page_build_guide.html
# https://files.ettus.com/manual/page_dpdk.html

# dependency DPDK (only required for Ethernet based USRPs)
apt-get -y install dpdk dpdk-dev

# Option A: using package manager, https://files.ettus.com/manual/page_install.html
apt-get -y install libuhd-dev uhd-host

# Option B: building from source
: '
# dependencies
apt-get -y install autoconf automake build-essential ccache cmake cpufrequtils doxygen ethtool \
g++ git inetutils-tools libboost-all-dev libncurses5 libncurses5-dev libusb-1.0-0 libusb-1.0-0-dev \
libusb-dev python3-dev python3-mako python3-numpy python3-requests python3-scipy python3-setuptools \
python3-ruamel.yaml

# clone the source code
sudo -u "$SUDO_USER" git clone --depth 1 --branch UHD-4.6 https://github.com/EttusResearch/uhd.git

# build and install
cd uhd/host
sudo -u "$SUDO_USER" mkdir build
cd build
sudo -u "$SUDO_USER" cmake ../
sudo -u "$SUDO_USER" make -j
make install

ldconfig
'

###############################################
# install VOLK
###############################################

cd $cwd

# https://github.com/gnuradio/volk

apt-get -y install build-essential cmake

# clone the source code
sudo -u "$SUDO_USER" git clone --depth 1 --branch v3.1.0 --recurse-submodules -j8 https://github.com/gnuradio/volk.git

# build and install
cd volk
sudo -u "$SUDO_USER" mkdir build
cd build
sudo -u "$SUDO_USER" cmake ../
sudo -u "$SUDO_USER" make -j
make install

ldconfig

# optional
volk_profile

###############################################
# install srsRAN
###############################################

cd $cwd

# https://docs.srsran.com/projects/4g/en/latest/general/source/1_installation.html#installation-from-source

# dependencies
apt-get -y install build-essential cmake libfftw3-dev libmbedtls-dev libboost-program-options-dev libconfig++-dev libsctp-dev

# clone the source code
sudo -u "$SUDO_USER" git clone --depth 1 --branch release_23_11 https://github.com/srsRAN/srsRAN_4G.git

# build and install
cd srsRAN_4G
sudo -u "$SUDO_USER" mkdir build
cd build
# With gcc 12 or later, there are false alarms warnings of type Warray-bounds.
# To allow compilation with gcc 12 or later, Werror is disabled. 
sudo -u "$SUDO_USER" cmake -DENABLE_WERROR=OFF ../
sudo -u "$SUDO_USER" make -j
make install

ldconfig

###############################################
# install Eigen and fmt
###############################################

cd $cwd

# dependencies
apt-get -y install libeigen3-dev

###############################################
# indicate installation process has finished
###############################################

echo "Finished!"
