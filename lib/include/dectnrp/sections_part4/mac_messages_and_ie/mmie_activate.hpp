/*
 * Copyright 2023-2024 Maxim Penner
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

// ##################################################
// Table 6.3.4-2

#define ACTIVATE_Padding_IE
// #define ACTIVATE_Higher_Layer_Signalling
#define ACTIVATE_User_Plane_Data
// #define ACTIVATE_Network_Beacon_Message
#define ACTIVATE_Cluster_Beacon_Message
// #define ACTIVATE_Association_Request_Message
// #define ACTIVATE_Association_Response_Message
// #define ACTIVATE_Association_Release_Message
// #define ACTIVATE_Reconfiguration_Request_Message
// #define ACTIVATE_Reconfiguration_Response_Message
// #define ACTIVATE_Additional_Mac_Messages
// #define ACTIVATE_MAC_Security_Info_IE
// #define ACTIVATE_Route_Info_IE
// #define ACTIVATE_Resource_Allocation_IE
// #define ACTIVATE_Random_Access_Resource_IE
// #define ACTIVATE_RD_Capability_IE
// #define ACTIVATE_Neighbouring_IE
// #define ACTIVATE_Broadcast_Indication_IE
// #define ACTIVATE_Group_Assignment_IE
// #define ACTIVATE_Load_Info_IE
// #define ACTIVATE_Measurement_Report_IE
// #define ACTIVATE_Source_Routing_IE           // added in 2024-10
// #define ACTIVATE_Joining_Beacon_Message      // added in 2024-10
// #define ACTIVATE_Joining_Information_IE      // added in 2024-10
// #define ACTIVATE_Escape
// #define ACTIVATE_IE_Type_Extension

// ##################################################
// Table 6.3.4-3

// #define ACTIVATE_Padding_IE                  // activated above
// #define ACTIVATE_Configuration_Request_IE
// #define ACTIVATE_Keep_Alive_IE               // added in 2024-10
// #define ACTIVATE_MAC_Security_Info_IE

// ##################################################
// Table 6.3.4-4

// #define ACTIVATE_Padding_IE                  // activated above
// #define ACTIVATE_Radio_Device_Status_IE
// #define ACTIVATE_RD_Capability_Short_IE      // added in 2024-10
// #define ACTIVATE_Association_Control_IE      // added in 2024-10

// ##################################################
// separated as not standard-compliant

#define ACTIVATE_Power_Target_IE
#define ACTIVATE_Time_Announce_IE
