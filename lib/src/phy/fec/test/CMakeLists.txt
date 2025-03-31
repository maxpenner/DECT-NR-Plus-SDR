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

add_executable(plcf2pcc plcf2pcc.cpp)
target_link_libraries(plcf2pcc dectnrp_phy)
add_test(plcf2pcc plcf2pcc)

add_executable(plcf2pcc_awgn plcf2pcc_awgn.cpp)
target_link_libraries(plcf2pcc_awgn dectnrp_phy)
add_test(plcf2pcc_awgn plcf2pcc_awgn)

add_executable(tb2pdc tb2pdc.cpp)
target_link_libraries(tb2pdc dectnrp_phy)
add_test(tb2pdc tb2pdc)

add_executable(tb2pdc_awgn tb2pdc_awgn.cpp)
target_link_libraries(tb2pdc_awgn dectnrp_phy)
add_test(tb2pdc_awgn tb2pdc_awgn)
