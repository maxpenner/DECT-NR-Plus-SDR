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

# check for sudo rights
sudo -v || exit 

while getopts a:i: flag
do
    case "${flag}" in
        a) ipaddress=${OPTARG};;
        i) interface=${OPTARG};;
    esac
done

if [[ -z $ipaddress  ||  -z $interface ]]
  then
    echo "Arguments -a for IP-address and -i for interface are mandatory.\n";
    exit 1;
fi

echo "Changing default gateway" $ipaddress $interface

sudo route add default gw $ipaddress $interface

# temporarily set name server to Google Public DNS
# https://askubuntu.com/questions/845430
echo "nameserver 8.8.8.8" | sudo tee /etc/resolv.conf

