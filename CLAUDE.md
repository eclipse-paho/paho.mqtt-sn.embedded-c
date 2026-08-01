# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Repository overview

Eclipse Paho MQTT-SN C/C++ client for embedded platforms. Dual-licensed EPL 2.0 / EDL 1.0. Four independent sub-projects, each buildable on its own:

1. **MQTTSNPacket** — de/serialization of MQTT-SN 1.2 packets (lowest-level C library, minimal dependencies).
2. **MQTTSN2Packet** — de/serialization of MQTT-SN 2.0 packets, including a `MQTTSNProtection` (packet signing/encryption) module and a CPython extension module (`mqttsn2`) wrapping the same C sources.
3. **MQTTSNGateway** — MQTT-SN transparent/aggregating gateway (C++) that bridges MQTT-SN clients to an MQTT broker over TCP/IP.
4. **MQTTSNClient** — higher-level C++ client mirroring the Paho embedded MQTT client API (incomplete/in progress).

Each sub-project has its own `CMakeLists.txt` and can be built standalone, but the top-level `CMakeLists.txt` builds MQTTSNPacket, MQTTSN2Packet, and MQTTSNGateway together (MQTTSNClient is not wired into the top-level build).

## Build & test commands

### Whole repo (Packet libraries + Gateway)
```
./cmake-build.sh
```
This does a clean `build.paho/` CMake build with `-DSENSORNET=loralink` and runs `ctest -VV --timeout 600`. Edit the script (or pass `-DSENSORNET=<udp|udp6|xbee|rfcomm|dtls>` manually) to build the gateway against a different sensor network transport — only one sensor network can be compiled in at a time.

### MQTTSNPacket / MQTTSN2Packet only
Each has its own `cmake-build.sh` doing a clean CMake build + `ctest`:
```
cd MQTTSNPacket && ./cmake-build.sh
cd MQTTSN2Packet && ./cmake-build.sh
```
To run/debug a single test binary directly:
```
cd MQTTSNPacket/build.paho && ctest -R test1 -VV
# or run the binary directly, e.g. ./MQTTSNPacket/test/test1
```
MQTTSN2Packet's `test22`/`test23` targets link against a hardcoded Homebrew OpenSSL path (`/opt/homebrew/opt/openssl@3/...`) in `MQTTSN2Packet/test/CMakeLists.txt` — expect build failures for these two targets on machines without OpenSSL at that path (other tests are unaffected). `test3`/`test23` exercise `MQTTSNProtection`; `test23` needs a running MQTT-SN 2.0 UDP broker (see below).

### MQTTSN2Packet Python extension
`MQTTSN2Packet/src/setup.py` builds a CPython extension (`mqttsn2`) from `mqttsn2module.c` plus the same C sources used by the CMake library (compiled independently, not via the shared lib):
```
cd MQTTSN2Packet/src && python setup.py build_ext --inplace
```

### MQTTSNGateway only
```
cd MQTTSNGateway && ./build.sh [udp|udp6|xbee|loralink|rfcomm|dtls|dtls6]
```
A sensor-network argument is required — this determines which `src/linux/<sensornet>/SensorNetwork.cpp` is compiled in. Builds into `MQTTSNGateway/build.gateway/` and copies executables + `.conf` files to `MQTTSNGateway/bin/`. Append `-DDEBUG_NW` and/or `-DDEBUG_MQTTSN` for verbose sensor-network / MQTT-SN message logs, e.g. `./build.sh udp -DDEBUG_NW`. `./build.sh clean` removes the build dir and `bin/`.

Gateway self-tests (`testPFW`, covering the ring buffer, task queue, 2-3 tree, topic tables) run via `ADD_TEST` in the top-level/`cmake-build.sh` flow, or directly: `./bin/testPFW -f ./gateway.conf` from `MQTTSNGateway/`.

Some MQTTSN2Packet tests and gateway integration testing require a live MQTT-SN 2.0 UDP broker from the `paho.mqtt.testing` project (run with the `udp.conf` config) — see that project for setup.

## Architecture

### Packet libraries (MQTTSNPacket, MQTTSN2Packet)
Both follow the same layout and pattern, MQTTSN2Packet being the newer/actively-developed protocol version:
- `src/MQTTSNPacket.h` — central header: message type enum (`MQTTSN_msgTypes`), `MQTTSN_topicid` (tagged union for normal/predefined/short topics), flags bitfield (`MQTTSNFlags`, with a `REVERSED` compile switch for bitfield-order-sensitive platforms), and shared read/write primitives (`readMQTTSNString`, `writeMQTTSNString`, etc). It pulls in the per-message-type headers (`MQTTSNConnect.h`, `MQTTSNPublish.h`, `MQTTSNSubscribe.h`, `MQTTSNUnsubscribe.h`, `MQTTSNSearch.h` — v1.2 only).
- Each message type has separate `*Client.c` / `*Server.c` source files (e.g. `MQTTSNConnectClient.c` serializes a CONNECT / deserializes a CONNACK, `MQTTSNConnectServer.c` is the reverse) — serialize/deserialize functions are one-way and side-specific, not symmetric pairs in the same file.
- `MQTTSNPacket_read`/`MQTTSNPacket_read_nb` handle the common length-prefixed framing (including the extended 3-byte length form via `MQTTSNPacket_decode`/`_encode`); actual field parsing is delegated to the per-message deserialize functions.
- MQTTSN2Packet adds `MQTTSNProtection.c/h` for packet signing/encryption (tested via `test_protection.c`/`test3.c`) and `mqttsn2module.c` (CPython bindings, built via `setup.py`, independent of the CMake `MQTTSN2Packet` shared library target).
- `samples/` in each show wire-level usage over UDP via a shared `transport.c/h`; `paho_sn_pub`/`paho_sn_sub` in MQTTSN2Packet are mosquitto_pub/sub-style CLI tools.
- Deserialization functions perform buffer bounds checking against `buflen`/`enddata` — preserve this when adding new message types.

### MQTTSNGateway
A multi-threaded C++ process bridging MQTT-SN (UDP/XBee/Bluetooth/LoRa/DTLS clients) to MQTT (TCP/TLS broker), configured via `gateway.conf`. Two gateway modes, selected by `AggregatingGateway` in `gateway.conf`:
- **Transparent**: one MQTT connection per MQTT-SN client (`MQTTSNGWConnectionHandler`/`MQTTGWConnectionHandler`).
- **Aggregating**: many MQTT-SN clients share one MQTT connection (`MQTTSNAggregateConnectionHandler`, `MQTTSNGWAggregater`, `MQTTSNGWAggregateTopicTable`), plus optional `MQTTSNGWForwarder` and `MQTTSNGWQoSm1Proxy` (QoS -1 support) and predefined-topic / client-authentication modes.
- Sensor-network transport is abstracted behind `src/linux/<sensornet>/SensorNetwork.{h,cpp}` (one implementation per transport: `udp`, `udp6`, `xbee`, `loralink`, `rfcomm`, `dtls`); everything above that layer (`MQTTSNGWClientRecvTask`/`SendTask`, `MQTTSNGWPacketHandleTask`, topic/client tables) is transport-agnostic and only one transport is compiled in per build (`-DSENSORNET=...`).
- `src/linux/{Network,Timer,Threading}.cpp` are the OS-abstraction layer (an `OS` variable in CMake selects the `linux` implementation; only Linux/macOS-via-Linux-paths is supported today).
- `MQTTSNGWLogmonitor`/`mainLogmonitor.cpp` is a separate executable (`MQTT-SNLogmonitor`) that reads gateway logs from shared memory (enabled via `ShearedMemory=YES` in `gateway.conf`) for remote monitoring — it shares most of the gateway's object code (`mqtt-sngateway_common`) but is a distinct process from `MQTT-SNGateway`.
- `GatewayTester/` contains standalone sample MQTT-SN clients (`ClientPub`, `ClientPubQoS-1`, `ClientSub`) for exercising a running gateway.
- `src/tests/` (built as `testPFW`) unit-tests internal data structures (ring buffer/queue, 2-3 tree, topic table, task scheduling) independent of any live broker/network.

### MQTTSNClient
Intended to mirror the Paho embedded MQTT C++ client API; `MQTTSNClient.h`/`FP.h`/`MQTTLogging.h` plus a `linux/linux.cpp` platform layer. Not wired into the top-level CMake build and marked incomplete in the README — treat as WIP, not a stable API.
