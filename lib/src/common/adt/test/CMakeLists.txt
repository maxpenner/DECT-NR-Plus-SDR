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

add_executable(bimap bimap.cpp)
target_link_libraries(bimap dectnrp_common)
add_test(bimap bimap)

add_executable(callbacks callbacks.cpp)
target_link_libraries(callbacks dectnrp_common)
add_test(callbacks callbacks)

add_executable(enumeration enumeration.cpp)
target_link_libraries(enumeration dectnrp_common)
add_test(enumeration enumeration)

add_executable(miscellaneous miscellaneous.cpp)
target_link_libraries(miscellaneous dectnrp_common)
add_test(miscellaneous miscellaneous)
