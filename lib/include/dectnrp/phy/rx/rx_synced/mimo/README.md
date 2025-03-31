# MIMO modes in DECT NR+

According to Table 7.2-1 in ETSI TS 103 636-3, there are twelve (index 0 to 11) transmission modes for 1, 2, 4 and 8 antennas.

## MIMO Terminology in DECT NR+ and LTE

| DECT NR+                      | LTE                               |
| ----------------------------- | --------------------------------- |
| Transmission mode             | Transmission Mode (TM)            |
| Channel Quality Indication    | Channel Quality Indication (CQI)  |
| MIMO Feedback                 | Rank Indicator (RI)               |
| Codebook Index                | Pre-coding Matrix Indicator (PMI) |

## TMs with one Antenna
0) Single antenna
    - equivalent to LTE TM1
    - use cases:
        - only one antenna is available for transmission
        - even with a single antennas, a device can decode packet transmitted with transmit diversity

## TMs with two Antennas
1) Transmit diversity, Space Frequency Block Coding (SFBC)
    - equivalent to LTE TM2
    - use cases:
        - RX has only one antenna (must support transmit diversity)
        - fallback if other MIMO modes are undesirable (rank-one transmission)
        - increase reliability
        - beacon with PMI 0, allows RX to conduct a channel measurement across all transmit streams (in that case equivalent to antennas streams)
        - broadcast with PMI 0 to multiple users
    - PMI may be based on:
        - feedback PMI from RX
        - own channel measurement
        - heuristic (e.g. alternating PMI)
    - beamforming possible and likely beneficial for a unicast as Table 6.3.3.2-1 has only one element
2) MIMO open loop
    - equivalent to LTE TM3 (without CDD)
    - use cases:
        - high speed with short coherence time
        - single stream if SNR and/or scattering is low
        - multi stream if SNR is high and scattering rich
    - requires RI and CQI, but no PMI (thus less feedback than for closed loop)
    - instead relying on PMI from RX:
        - PMI is fixed
        - PMI is switched cyclically
        - PMI can be based on own channel measurement, but then the accuracy depends on antenna and RF frontend calibration
    - DECT NR+ always reports RI and PMI together
3) MIMO closed loop
    - equivalent to LTE TM6 (special type of LTE TM4)
    - use cases:
        - low speed with long coherence time
        - low SNR and/or low scattering
        - rank 1
    - requires RI(=1), CQI and PMI
4) MIMO closed loop
    - equivalent to LTE TM4
    - use cases
        - low speed with long coherence time
        - high SNR and rich scattering
    - requires RI, CQI and PMI
    
## TMs with four Antennas
5) Transmit diversity
    - equivalent to LTE TM2
    - beamforming possible, but Table 6.3.3.2-2 has 6 elements, thus many paths
6) MIMO open loop
    - equivalent to LTE TM3 (without CDD)
7) MIMO closed loop
    - equivalent to LTE TM6 (special type of LTE TM4)
8) MIMO closed loop
    - equivalent to LTE TM4
9) MIMO closed loop
    - equivalent to LTE TM4

## TMs with eight Antennas
10) Transmit diversity
    - equivalent to LTE TM2
    - beamforming possible, but Table 6.3.3.2-3 has 12 elements, thus many paths
11) MIMO open loop
    - equivalent to LTE TM4

## Procedure
1) FT transmit beacon with TX diversity across all N_TX antennas with PMI 0
2) PT receives beacon
    - detects that transmit diversity was used (cyclic rotation of STF, or blind test)
    - detects that no beamforming was used (masked PLCF CRC)
    - detects that a beacon was received (PLCF)
    - measures channel estimate H for each transmit stream (for PMI 0 equivalent to antennas streams)
    - TM search range depending on number of antennas
        - 1 antenna: TM0
        - 2 antennas: TM1 to TM4
        - 4 antennas: TM5 to TM9
        - 8 antennas: TM10 or TM11
        - Option A: SVD based methods
            - based on H, measure SINR
            - based on H, perform SVD and determine CN (condition number)
            - based on SINR and CN, determine achievable CQI
            - based on SVD, determine RI (how to avoid being overly optimistic?)
            - based on SVD and RI, determine PMI maximizing the sum of diagonal elements
        - Option B: Maximum likelihood exhaustive search
            - iterate over each RI and PMI
                - determine SINR for each layer
                - determine capacity for each layer
            - use combination that maximizes overall capacity
        - sends back feedback_info with next packet

## Sources
- https://scdn.rohde-schwarz.com/ur/pws/dl_downloads/dl_application/application_notes/1ma187/1MA187_1E_LTE_Beamforming_Mesurments.pdf
- https://scdn.rohde-schwarz.com/ur/pws/dl_downloads/dl_application/application_notes/1ma186/1MA186_2e_LTE_TMs_and_beamforming.pdf
- https://cdn.rohde-schwarz.com/pws/dl_downloads/dl_application/application_notes/1sp18/1SP18_10e.pdf
- https://de.mathworks.com/help/lte/ug/pdsch-throughput-conformance-test-for-single-antenna-tm1-transmit-diversity-tm2-open-loop-tm3-and-closed-loop-tm4-6-spatial-multiplexing.html
- https://de.mathworks.com/help/lte/ug/pdsch-throughput-for-non-codebook-based-precoding-schemes-port-5-tm7-port-7-or-8-or-port-7-8-tm8-port-7-14-tm9-and-tm10.html
- https://de.mathworks.com/help/lte/ref/ltepmiselect.html
- https://www.sharetechnote.com/html/Handbook_LTE_ClosedLoop.html
- https://www.sharetechnote.com/html/BasicProcedure_LTE_MIMO.html
- https://www.sharetechnote.com/html/Communication_ChannelModel_SVD.html
- https://blog.3g4g.co.uk/2013/07/decision-tree-of-transmission-modes-tm.html
- https://blog.3g4g.co.uk/2011/03/quick-recap-of-mimo-in-lte-and-lte.html
- https://math.stackexchange.com/questions/3967922/singular-values-and-matrix-rank
- https://ma-mimo.ellintech.se/2017/10/03/what-is-the-difference-between-beamforming-and-precoding/#:~:text=A%20third%20answer%20is%20that,multiplexing%20of%20several%20data%20streams
- https://eescholars.iitm.ac.in/sites/default/files/eethesis/ee15b074.pdf