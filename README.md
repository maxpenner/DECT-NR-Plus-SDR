<p align="center">
  <img src="docs/logo.png" style="width: 100%; background-color: transparent;" alt="logo.png"/>
</p>

This repository contains a work-in-progress SDR implementation of DECT NR+ ([ETSI TS 103 636, Part 1 to 5](https://www.etsi.org/committee/1394-dect)). DECT NR+ is a non-cellular radio standard and part of [5G as defined by ITU-R](https://www.etsi.org/newsroom/press-releases/1988-2021-10-world-s-first-non-cellular-5g-technology-etsi-dect-2020-gets-itu-r-approval-setting-example-of-new-era-connectivity). Introductions are available at [ETSI](https://www.etsi.org/technologies/dect), [DECT Forum](https://www.dect.org/nrplus) and [Wikipedia](https://en.wikipedia.org/wiki/DECT-2020).

While commonly referred to as DECT NR+, the standard's official designation is DECT-2020 New Radio (NR).

## Table of Contents

1. [Core Idea](#core-idea)
2. [Directories](#directories)
3. [Installation](#installation)
4. [Starting](#starting)
5. [Contributing](#contributing)
6. [Citation](#citation)
7. [Known Issues](#known-issues)
8. [To Do](#to-do)

Advanced Topics

1. [Architecture](#architecture)
2. [AGC](#agc)
3. [Resampling](#resampling)
4. [Synchronization](#synchronization)
5. [Compatibility](#compatibility)
6. [JSON Export](#json-export)
7. [PPS Export and PTP](#pps-export-and-ptp)
8. [Host and SDR Tuning](#host-and-sdr-tuning)
9. [Firmware P2P](#firmware-p2p)

## Core Idea

The core idea of the SDR is to provide a basis to write custom DECT NR+ firmware. In terms of the OSI model, the firmware is located on the MAC layer and the layers above as shown in [Architecture](#architecture). The SDR provides:

- Radio Layer (typically part of PHY, here separate layer)
- PHY Layer
- PHY to MAC layer interface
- Application layer interface (e.g. UDP sockets, virtual NIC).

Custom DECT NR+ firmware is implemented by deriving from the class [tpoint_t](lib/include/dectnrp/upper/tpoint.hpp) and implementing its virtual functions. The abbreviation tpoint stands for termination point which is DECT terminology and simply refers to a DECT NR+ node. There are multiple firmware examples in [lib/include/dectnrp/upper/tpoint_firmware/](lib/include/dectnrp/upper/tpoint_firmware/). For instance, the termination point firmware (tfw) [tfw_basic_t](lib/include/dectnrp/upper/tpoint_firmware/basic/tfw_basic.hpp) provides the most basic firmware possible. It derives from [tpoint_t](lib/include/dectnrp/upper/tpoint.hpp) and leaves all virtual functions mostly empty. The full list of virtual functions is:

|   | **Virtual Function**     | **Properties**                                                            |
|---|--------------------------|---------------------------------------------------------------------------|
| 1 | work_start_imminent()    | called once immediately before IQ sample processing begins                |
| 2 | work_regular()           | called regularly (polling)                                                |
| 3 | work_pcc()               | called upon PCC reception with correct CRC (event-driven)                 |
| 4 | work_pcc_incorrect_crc() | called upon PCC reception with incorrect CRC (event-driven, optional)     |
| 5 | work_pdc_async()         | called upon PDC reception (event-driven)                                  |
| 6 | work_upper()             | called upon availability of new data on application layer (event-driven)  |
| 7 | work_chscan_async()      | called upon finished channel measurement (event-driven)                   |
| 8 | start_threads()          | called once during SDR startup to start application layer threads         |
| 9 | stop_threads()           | called once during SDR shutdown to stop application layer threads         |

## Directories

    ├─ .vscode/                 VS Code settings
    ├─ apps/                    apps sources
    ├─ bin/                     apps post-compilation binaries
    ├─ cmake/                   CMake modules
    ├─ configurations/          configuration files required to start the SDR
    ├─ docs/                    documentation (doxygen, graphics etc.)
    ├─ gnuradio/                flow graphs (SDR oscilloscope, USRP calibration etc.)
    ├─ json/                    submodule to analyze exported JSON files in Matlab
    ├─ lib/                     library code used by applications
    │  ├─ include/
    │  |  ├─ application/       application layer interfaces
    │  |  ├─ apps/              utilities for apps in directory apps/
    │  |  ├─ common/            common functionality across all layers/directories
    │  |  ├─ dlccl/             DLC and convergence layer
    │  |  ├─ external/          external libraries
    │  |  ├─ mac/               MAC layer
    │  |  ├─ phy/               physical layer
    │  |  ├─ radio/             radio layer
    │  |  ├─ sections_part2/    sections of ETSI TS 103 636-2
    │  |  ├─ sections_part3/    sections of ETSI TS 103 636-3
    │  |  ├─ sections_part4/    sections of ETSI TS 103 636-4
    │  |  ├─ sections_part5/    sections of ETSI TS 103 636-5
    │  |  ├─ simulation/        wireless simulation
    │  |  ├─ upper/             upper layers, i.e. layers between PHY and application layer
    │  ├─ src/                  source code (same directories as in include/)
    └─ scripts/                 shell scripts

## Installation

The SDR has been tested on Ubuntu 22.04 and 24.04. It has five dependencies:
- [UHD](https://github.com/EttusResearch/uhd): Radio Devices
- [srsRAN 4G](https://github.com/srsran/srsRAN_4G): Turbo coding, PHY processing, SIMD
- [VOLK](https://github.com/gnuradio/volk): SIMD
- [Eigen](https://eigen.tuxfamily.org/index.php?title=Main_Page): Matrix Inversion
- [fmt](https://github.com/fmtlib/fmt): Logging

Installation instructions for these dependencies can be found in [scripts/install_dependencies.sh](scripts/install_dependencies.sh). After installing the dependencies, the SDR can be either downloaded and compiled with [scripts/install_sdr.sh](scripts/install_sdr.sh), or manually with: 

```console
git clone --recurse-submodules https://github.com/maxpenner/DECT-NR-Plus-SDR
cd DECT-NR-Plus-SDR
mkdir build
cd build
cmake ..
make
```
The compiled code is kept local in [bin/](bin/) and not copied to any operating system directories such as `usr/local/`.

## Starting

To start the SDR, the main executable `dectnrp` must be invoked. It is located in [bin/](bin/) after compilation.

```console
cd bin/
sudo ./dectnrp "../configurations/basic_simulator/"
```

The SDR is stopped by pressing control+C. The executable `dectnrp` requires exactly one argument, which is a path to a directory containing three configuration files `radio.json`, `phy.json` and `upper.json`. Each configuration file configures its respective layer(s). In the case of `upper.json`, this also implies the name of the firmware to load.

The configuration files may also contain multiple instances of the SDR. In fact, each configuration file configures its layer(s) by listing one or multiple layer units:

- `radio.json`: list of hardwares [hw_t](lib/include/dectnrp/radio/hw.hpp) (e.g. USRP or simulator), details in [hw_config_t](lib/include/dectnrp/radio/hw_config.hpp)
- `phy.json`: list of worker pools [worker_pool_t](lib/include/dectnrp/phy/worker_pool.hpp), details in [worker_pool_config_t](lib/include/dectnrp/phy/worker_pool_config.hpp)
- `upper.json`: list of termination points [tpoint_t](lib/include/dectnrp/upper/tpoint.hpp), details in [tpoint_config_t](lib/include/dectnrp/upper/tpoint_config.hpp)

<p align="center">
  <img src="docs/layer_and_layer_units.drawio.png" style="width: 75%; background-color: transparent;" alt="layer_and_layer_units.drawio.png"/>
</p>

If `radio.json` and `phy.json` contain multiple layer instances, each trio of [tpoint_t](lib/include/dectnrp/upper/tpoint.hpp), [worker_pool_t](lib/include/dectnrp/phy/worker_pool.hpp) and [hw_t](lib/include/dectnrp/radio/hw.hpp) is either started independently, or a single [tpoint_t](lib/include/dectnrp/upper/tpoint.hpp) controls multiple duos of [worker_pool_t](lib/include/dectnrp/phy/worker_pool.hpp) and [hw_t](lib/include/dectnrp/radio/hw.hpp).

Furthermore, a set of configuration files is not bound to a specific firmware. By replacing the firmware name in `upper.json`, the configuration files can be combined with a different firmware. It is up to the firmware to verify at runtime whether the applied configuration is compatible.

## Contributing

Submit a pull request and keep the same licence.

## Citation

If you use this repository for any publication, please cite the repository according to [CITATION.cff](CITATION.cff).
    
## Known Issues

1. The channel coding requires verification. It is based on [srsRAN 4G](https://github.com/srsran/srsRAN_4G) with multiple changes, for instance, an additional maximum code block size Z=2048. Furthermore, channel coding with a limited number of soft bits is not implemented yet.
2. [MAC messages and information elements (MMIEs)](lib/include/dectnrp/sections_part4/mac_messages_and_ie) in the standard are subject to frequent changes. Previously completed MMIEs are currently being revised and will be updated soon.
3. If asserts are enabled, the program may stop abruptly if IQ samples are not processed fast enough. This is triggered by a backlog of unprocessed IQ samples within synchronization.
4. For some combinations of operating system, CPU and DPDK, pressing control+C does not stop the SDR. The SDR process must then be stopped manually.
5. With gcc 12 and above, a [warning is issued in relation to fmt](https://github.com/fmtlib/fmt/issues/3354) which becomes an error due to the compiler flag *Werror* being used by default. It can be disabled in [CMakeLists.txt](CMakeLists.txt) by turning off the option *ENABLE_WERROR*.
6. In an earlier version of the standard, the number of transmit streams was signaled by a cyclical rotation of the STF in frequency domain. This functionality will be kept for the time being. In the current version of the standard, the number of transmit streams in a packet must be tested blindly.

## To Do

### Radio Layer

- [ ] GPIO support for B210, currently only X410 supported

### Physical Layer

- [ ] **$\mu$** detection
- [ ] integer CFO
- [ ] residual STO based on DRS
- [ ] residual CFO based on STF
- [ ] residual CFO based on DRS
- [ ] MIMO modes with two or more spatial streams
- [ ] 1024-QAM

### Upper layers

- [ ] integration of retransmissions with HARQ into [Firmware P2P](#firmware-p2p) to finalize interfaces
- [ ] reusable firmware procedures (association etc.)
- [ ] DLC and Convergence layers
- [ ] enhanced application layer interfaces to DECT NR+ stack (blocking and multiplexing of addresses, streams, control information etc.)

### Application Layer

- [ ] lock around individual items instead of entire queues

## Architecture

The figure below illustrates the architecture of the SDR with blocks representing C++ classes, objects and threads. All types (postfix *_t*) have identical names in the source code. The original image is [docs/sdr_architecture.drawio](docs/sdr_architecture.drawio).

<p align="center">
  <img src="docs/sdr_architecture.drawio.png" style="width: 75%; background-color: transparent;" alt="sdr_architecture.drawio.png"/>
</p>

The key takeaways are:

1. The radio layer uses a single RX ring buffer of type [buffer_rx_t](lib/include/dectnrp/radio/buffer_rx.hpp) to distribute IQ samples to all workers. For TX, it uses multiple independent [buffer_tx_t](lib/include/dectnrp/radio/buffer_tx.hpp) instances from a [buffer_tx_pool_t](lib/include/dectnrp/radio/buffer_tx_pool.hpp). The size and number of buffers is defined in `radio.json`.
2. The PHY has workers for synchronization ([worker_sync_t](lib/include/dectnrp/phy/pool/worker_sync.hpp)) and workers for packet encoding/decoding and modulation/demodulation ([worker_tx_rx_t](lib/include/dectnrp/phy/pool/worker_tx_rx.hpp)). The number of workers, their CPU affinity and priority are set in `phy.json`. Both worker types communicate through a MPMC [job_queue_t](lib/include/dectnrp/phy/pool/job_queue.hpp).
3. When synchronization detects a DECT NR+ packet, it creates a job with a [sync_report_t](lib/include/dectnrp/phy/rx/sync/sync_report.hpp), which is then processed by instances of [worker_tx_rx_t](lib/include/dectnrp/phy/pool/worker_tx_rx.hpp). During packet processing, these workers call the firmware through the virtual work_*() functions mentioned in [Core Idea](#core-idea). Access to the firmware is thread-safe as each worker has to acquire a [token_t](lib/include/dectnrp/phy/pool/token.hpp).
4. Synchronization also creates regular jobs with a [time_report_t](lib/include/dectnrp/phy/rx/sync/time_report.hpp). Each of these jobs contains a time update for the firmware, and the starting time of the last known packet. The rate of regular jobs depends on how processing of [buffer_rx_t](lib/include/dectnrp/radio/buffer_rx.hpp) is split up between instances of [worker_sync_t](lib/include/dectnrp/phy/pool/worker_sync.hpp) in `phy.json`. A typical rate is one job for each 2 slots, equivalent to 1200 jobs per second. 
5. The firmware of the SDR is not executed in an independent thread. Instead, the firmware is equivalent to a thread-safe state machine whose state changes are triggered by calls of the work_*() functions. The type of firmware executed is defined in `upper.json`.
6. Each firmware starts and controls its own application layer interface ([app_t](lib/include/dectnrp/application/app.hpp)). To allow immediate action for new application layer data, [app_t](lib/include/dectnrp/application/app.hpp) is given access to [job_queue_t](lib/include/dectnrp/phy/pool/job_queue.hpp). The job type created by [app_t](lib/include/dectnrp/application/app.hpp) contains an [upper_report_t](lib/include/dectnrp/upper/upper_report.hpp) with the number and size of data items available on the application layer.
7. The application layer interface is either a [set of UDP ports](lib/include/dectnrp/application/socket/) or a [virtual NIC](lib/include/dectnrp/application/vnic/).

## AGC

An ideal AGC receives a packet and adjusts its sensitivity within a fraction of the STF (e.g. the first two or three patterns). However, as the SDR performs all processing exclusively on the host computer, only a slow software AGC is feasible, which, for example, adjusts the sensitivity 50 times per second.

One drawback of a software AGC is that packets can be masked. This happens when a packet with very high input power is received, followed immediately by a packet with very low input power. Both packets are separated by a guard interval (GI). Since synchronization is based on correlations of the length of the STF, and the STF for $\mu$ < 8 is longer than the GI, correlation is partially performed across both packets. This can lead to the second packet not being detected.

| **$\mu$** | **GI length ($\mu s$)** | **STF length ($\mu s$)**  |
|:---------:|:-----------------------:|:-------------------------:|
|     1     |          18.52          |           64.81           |
|     2     |          20.83          |           41.67           |
|     4     |          10.42          |           20.83           |
|     8     |          10.42          |           10.42           |

It is typically best for the FT to keep both transmit power and sensitivity constant, and only for the PT to adjust its own transmit power and sensitivity. The objective of the PT in the uplink is to achieve a specific receive power at the FT, such that all PTs in the uplink are received with similar power levels.

## Resampling

Most SDRs support a limited set of sample rates, which typically do not match the DECT NR+ base rate of 1.728MS/s or multiples thereof. A sample rate supported by most SDRs is 30.72MS/s. Therefore, the SDR uses fractional resampling with a polyphase filter to output IQ samples at the new base rate of 30.72MS/s / 16 = 1.92MS/s or multiples thereof.

### Examples

- 1.728MS/s without oversampling is resampled to 30.72MS/s / 16 = 1.92MS/s
- 1.728MS/s with oversampling by 2 is resampled to 30.72MS/s / 8 = 3.84MS/s
- 27.648MS/s with oversampling by 2 is resampled to 30.72MS/s * 2 = 61.44MS/s

### Tuning

Fractional resampling is computationally expensive. To reduce the computational load and enable larger bandwidths, the implicit FIR low-pass filter of the resampler can be made shorter. However, this also leads to more aliasing and thus to a larger EVM at the receiver. The resampler parameters are defined in [lib/include/dectnrp/phy/resample/resampler_param.hpp](lib/include/dectnrp/phy/resample/resampler_param.hpp).

Another option is to disable resampling entirely and generate DECT NR+ packets directly at 1.92MS/s or multiples thereof. The internal time of the SDR then runs at the same rate and it is still possible, for example, to send out packets exactly every 10ms. However, the packets have a wider bandwidth and are shorter in time domain. Fractional resampling to a DECT NR+ sample rate is disabled by setting `"enforce_dectnrp_samp_rate_by_resampling": false` in `phy.json`.

## Synchronization

Synchronization of packets based on the STF is described in [DECT-NR-Plus-Simulation](https://github.com/maxpenner/DECT-NR-Plus-Simulation?tab=readme-ov-file#synchronization). According to the DECT NR+ standard, the STF cover sequence can be disabled for testing purposes. In the SDR, this is done by changing the following line in [lib/include/dectnrp/sections_part3/stf_param.hpp](lib/include/dectnrp/sections_part3/stf_param.hpp) and recompiling:

``` C++
// #define SECTIONS_PART_3_STF_COVER_SEQUENCE_ACTIVE
```

With the SDR, synchronization is more reliable if no cover sequence is applied to the STF. This is due to the [coarse metric without a cover sequence being wider](https://github.com/maxpenner/DECT-NR-Plus-Simulation?tab=readme-ov-file#synchronization), and thus harder to miss.

## Compatibility

Packets generated with the SDR are compatible with the MATLAB code in [DECT-NR-Plus-Simulation](https://github.com/maxpenner/DECT-NR-Plus-Simulation). The SDR also has been tested with commercially available DECT NR+ solutions.

## JSON Export

Each worker pool can export JSON files with information about received DECT NR+ packets. The export is activated by changing the value of `"json_export_length"` in `phy.json` to 100 or higher, i.e. the worker pool collects information of at least 100 packets before exporting all in a single JSON file. To enable the export without jeopardizing the real-time operation of the SDR, the worker pool must have at least two instances of [worker_tx_rx_t](lib/include/dectnrp/phy/pool/worker_tx_rx.hpp). This way, at least one worker can continue processing IQ samples while another worker saves a JSON file.

Exported files can be analyzed with [DECT-NR-Plus-SDR-json](https://github.com/maxpenner/DECT-NR-Plus-SDR-json.git).

## PPS Export and PTP

With the [GPIOs of an USRP](https://files.ettus.com/manual/page_gpio_api.html), the SDR can create pulses that are synchronized to DECT NR+ events, for instance the beginning of a wirelessly received beacon. The generation of pulses is controlled by each firmware individually. The example [Firmware P2P](#firmware-p2p) contains logic to export one pulse per second (PPS).

A PPS signal itself is a frequently used clock for other systems. For example, a Raspberry PI can be disciplined with a PPS and at the same time act as a PTP source which itself operates synchronously with the PPS ([pi5-pps-ptp](https://github.com/maxpenner/pi5-pps-ptp)). Such a PTP source can then be used to discipline further systems:

- The SDR's [host computer can synchronize to PTP](https://github.com/maxpenner/pi5-pps-ptp/blob/main/docs/client.md) such that the operating system time and the USRP sample count run synchronously.
- The PPS is stable enough to allow deriving higher clock speeds, for instance 48kHz for distributed wireless audio applications.
- The SDR can also be synchronized to an existing PTP network by converting [PTP to a 10MHz and 1PPS signal](https://www.meinberg.de/german/products/ntp-ptp-signalkonverter.htm), and feeding these to the USRP.

<p align="center">
  <img src="docs/synchronization.drawio.png" style="width: 75%; background-color: transparent;" alt="docs/synchronization.drawio.png"/>
</p>

## Host and SDR Tuning

The following tuning tips have been tested with Ubuntu and help achieving low-latency real-time performance:

1. Elevated thread priority through [threads_core_prio_config_t](lib/include/dectnrp/common/thread/threads.hpp)
2. SDR threads on [isolated CPU cores with disabled interrupts](https://kb.ettus.com/Getting_Started_with_DPDK_and_UHD#Isolate_Cores.2FCPUs) through [threads_core_prio_config_t](lib/include/dectnrp/common/thread/threads.hpp)
2. [Disabled CPU sleep states and CPU frequency scaling](https://gitlab.eurecom.fr/oai/openairinterface5g/-/wikis/OpenAirKernelMainSetup#power-management)
5. Interface to SDR
    1. [DPDK with UHD](https://kb.ettus.com/Getting_Started_with_DPDK_and_UHD)
    2. [Adjusted send_frame_size and recv_frame_size](https://files.ettus.com/manual/page_transport.html#transport_param_overrides) in `radio.json`
    3. [Increased buffer sizes](https://kb.ettus.com/USRP_Host_Performance_Tuning_Tips_and_Tricks#Adjust_Network_Buffers)
    4. In each `radio.json`, the value `“turnaround_time_us”` defines how soon the SDR can schedule a packet transmission relative to the last known SDR timestamp. For UHD, the SDR time is a [64-bit counter in the FPGA](https://kb.ettus.com/Synchronizing_USRP_Events_Using_Timed_Commands_in_UHD#Radio_Core_Block_Timing). The smaller the turn around time, the lower the latency. Under optimal conditions, between 80 and 150 microseconds are possible.
6. Low-latency kernel

## Firmware P2P

The P2P (point-to-point) firmware is started on two separate host computers, each connected to an USRP (in this example an X410). One combination of host and USRP acts as a fixed termination point (FT), while the other is the portable termination point (PT). The FT is connected to the internet and once both FT and PT are started, the PT can access the internet through the wireless DECT NR+ connection acting as pipe for IP packets.

### Common settings on FT and PT

If a different USRP type is used, the value of `“uspr_args”` in `radio.json` must be modified accordingly. Furthermore, FT and PT must be tuned to a common center frequency. This is done by opening the following two files

- [lib/src/upper/tpoint_firmware/p2p/tfw_p2p_ft_once.cpp](lib/src/upper/tpoint_firmware/p2p/tfw_p2p_ft_once.cpp)
- [lib/src/upper/tpoint_firmware/p2p/tfw_p2p_pt_once.cpp](lib/src/upper/tpoint_firmware/p2p/tfw_p2p_pt_once.cpp)

and changing the following line in both files to the desired center frequency in Hz:

``` C++
hw.set_freq_tc(3890.0e6);
```

The CPU cores used may also require modification as both FT and PT use specific cores for their threads. For instance, in `radio.json` the thread handling TX is specified as:

```JSON
"rx_thread_config": [0, 1]
```

The first number 0 is the priority offset (or niceness), so here the thread is started with the highest priority 99 - 0 = 99. The second number 1 specifies the CPU core. Both numbers can also be negative, in which case the scheduler selects the priority and the CPU core. Further thread specifications can be found in all `.json` configuration files. 

### On the FT:

```console
cd bin/
sudo ./dectnrp "../configurations/p2p_usrpX410/"
```
Once the SDR is running, a TUN interface is instantiated which can be verified with `ifconfig`. To enable internet sharing at the FT, the interface to the internet is masqueraded:

```console
cd scripts/
sudo ./masquerade.sh <Interface Name>
```
### On the PT

In the file [configurations/p2p_usrpX410/upper.json](configurations/p2p_usrpX410/upper.json), the firmware is changed to:

```JSON
"firmware_name": "p2p_pt"
```

Then the SDR is started.

```console
cd bin/
sudo ./dectnrp "../configurations/p2p_usrpX410/"
```
Once the SDR is running, a TUN interface is instantiated which can be verified with `ifconfig`. To access the internet through that TUN interface, is it made the default gateway:

```console
sudo ./defaultgateway_dns.sh -a 172.23.180.101 -i tuntap_pt
```

On the PT, internet access should now happen through the DECT NR+ connection. This can be verified by running a speed test and observing the spectrum with a spectrum analyzer, or by checking the packet count in the log file in [bin/](bin/).
