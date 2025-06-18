# MAC message and information element (MMIE)

> [!WARNING]  
> MMIEs in the standard are subject to frequent changes. Previously completed MMIEs are currently being revised. Moreover, most MMIEs have not yet been tested in dedicated tests or integrated into DECT NR+ procedures. MMIEs are activated in [mmie_activate.hpp](mmie_activate.hpp) after their correctness has been confirmed.

## Table of Contents

1. [Transmission](#transmission)
2. [Reception](#reception)
3. [MMIE Types](#mmie-types)

## Transmission

Every MMIE must be added to [mmie_pool_tx_t](mmie_pool_tx.hpp) in the constructor. The pool must contain each MMIE exactly once. To attach an MMIE to a MAC PDU, the MMIE must be requested from the pool, filled with proper values and then packed by calling the method
```C++
void pack_mmh_sdu(uint8_t* mac_pdu_offset);
```
Calling this method also must take care of adding a [mac_multiplexing_header_t](../mac_pdu/mac_multiplexing_header.hpp).

## Reception

The decoder at the receiver side is [mac_pdu_decoder_t](../mac_pdu/mac_pdu_decoder.hpp). It decodes speculatively, i.e. it tries to decode MMIEs from available data before the final CRC of the MAC PDU has been confirmed. The class [mac_pdu_decoder_t](../mac_pdu/mac_pdu_decoder.hpp) derives from [mmie_pool_tx_t](mmie_pool_tx.hpp) and thus contains the same MMIEs. However, the decoder may contain each MMIE as often as it can occur in a MAC PDU. For example, it is possible that there are multiple instances of [user_plane_data_t](user_plane_data.hpp) or [higher_layer_signalling_t](higher_layer_signalling.hpp) in a single MAC PDU.

## MMIE Types

The file [mmie.hpp](mmie.hpp) contains abstract classes that each MMIE must derive from. The correct classes to derive from depend on the properties of the MMIE.

### mmie_packing_t

An MMIE deriving from this abstract parent class has the following properties:

- It contains multiple fields that must be packed, i.e. copied to some byte at some bit offset.
- It has a fixed size.

MMIEs of this type are:

1. association_control_ie_t (missing)
2. association_release_message_t
3. mac_security_info_ie_t
4. radio_device_status_ie_t
5. rd_capability_short_ie_t (missing)
6. route_info_ie_t
7. source_routing_ie_t (missing)

[Additional, non-standard compliant MMIEs](extensions/) of this type are:

8. power_target_ie_t
9. time_announce_ie_t

### mmie_packing_peeking_t

An MMIE deriving from this abstract parent class has the following properties:

- It contains multiple fields that must be packed, i.e. copied to some byte at some bit offset.
- It has optional fields and thus a variable size. The decoder [mac_pdu_decoder_t](../mac_pdu/mac_pdu_decoder.hpp) must first decode some of the first bytes to determine which optional fields are set and what the total size is. Its size is *peeked*, hence the name.

MMIEs of this type are:

1. association_request_message_t 
2. association_response_message_t
3. broadcast_indication_ie_t
4. cluster_beacon_message_t
5. group_assignment_ie_t
6. joining_beacon_message_t (missing)
7. joining_information_ie_t (missing)
8. load_info_ie_t
9. measurement_report_ie_t
10. neighbouring_ie_t
11. network_beacon_message_t
12. random_access_resource_ie_t
13. rd_capability_ie_t
14. reconfiguration_request_message_t
15. reconfiguration_response_message_t
16. resource_allocation_ie_t

### mmie_flowing_t

An MMIE deriving from this abstract parent class has the following properties:

- The MMIE does not contain any fields that are packed. Instead, it contains binary data that is not formatted on MAC layer.
- It it associated with a flow.

MMIEs of this type are:

1. higher_layer_signalling_t
2. user_plane_data_t

### mu_depending_t

The size of some MMIEs depends on the value of the subcarrier spacing mu. If an MMIE derives from [mu_depending_t](mmie.hpp), it is guaranteed to be given the value of mu during the decoding process by calling the method
```C++
void set_mu(const uint32_t mu_);
```

### Special Cases

#### Configuration Request IE

The MMIE of type configuration_request_ie_t has zero length. It directly derives from [mmie_t](mmie.hpp).

#### Keep Alive IE (missing)

The MMIE of type keep_alive_ie_t has zero length. It directly derives from [mmie_t](mmie.hpp).

#### Padding IE

The MMIE of type padding_ie_t does not fall in any of the above categories. Instead, it directly derives from [mmie_t](mmie.hpp). A MAC PDU can be finalized with padding IEs by calling the [mmie_pool_tx_t](mmie_pool_tx.hpp) method
 ```C++
 void fill_with_padding_ies(uint8_t* mac_pdu_offset, const uint32_t N_bytes_to_fill);
```
