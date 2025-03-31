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

#define TFW_RTT_UDP_PORT_DATA 8000
#define TFW_RTT_UDP_PORT_PRINT 8001

#define TFW_RTT_TX_LENGTH_MINIMUM_BYTE 16
#define TFW_RTT_TX_LENGTH_MAXIMUM_BYTE 2000

/// how many bytes of each TX packet are used to verify the origin of an acknowledgment?
#define TFW_RTT_TX_VS_RX_VERIFICATION_LENGTH_BYTE 8