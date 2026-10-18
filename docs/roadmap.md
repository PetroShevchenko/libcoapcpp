# libcoapcpp Roadmap

Status as of 2026-10-18.

This document describes the current state of the project and the sequence of steps toward the target structure from the [architecture description](architecture.md).

The first consumer of the library is the [CoAP Smart Home](coap_smart_home_system.md) system. The requirements it puts on the library are numbered R1–R26 in the [architecture description](architecture.md#coap-smart-home-requirements); the stages below refer to them, and [Mapping to CoAP Smart Home stages](#mapping-to-coap-smart-home-stages) shows which library stage each system stage needs.

The conclusions about the code come from reading the source files and the repository history. The code was not built or run in the process, so everything concerning parser behavior must be confirmed with tests.

## Current state

- **Activity.** 72 commits in `main` from April 2021 to January 2023. The `platformio` branch (October 2023) is not merged. There is no CI.
- **What the library implements.** Packet codec (RFC 7252), block-wise transfers (RFC 7959), SenML-JSON (RFC 8428), CoRE Link Format (RFC 6690), URI, unit tests on GoogleTest. Observe (RFC 7641) is planned.
- **Server and client engine.** It lives not in the library but in `examples/POSIX/common/`. It works through `processing(Buffer &)` and does not touch sockets, so it is effectively sans-IO already, but its location makes it unavailable to microcontrollers.
- **Examples in `main`.** `examples/POSIX` (server and client) and `examples/RASPBERRY-PI-4`, which is a copy of the POSIX version with different pin numbers.
- **Removed in commit `60f749e` (2023-01-15).** 149 files: the library's network layer (sockets for POSIX and lwIP, DNS, UDP client and server, a DTLS client on wolfSSL) and the examples for NUCLEO-F429ZI, Raspberry Pi Pico, and STM32MP157A-DK1. All of it is available in the history at `60f749e^`.
- **The `platformio` branch.** It has diverged from `main` structurally (`api/` renamed to `include/`, wolfSSL removed), so it cannot be merged. It carries the tags `v1.0.0`–`v1.0.5`. Its `coap-server` example is a stub that only blinks an LED.
- **Open issues.** #1 and #2, both about out-of-bounds reads in `src/packet.cc`.
- **Gaps relative to CoAP Smart Home.** `OptionNumber` in `consts.h` has no Observe (6). The CoRE Link parser accepts only attributes with `=` and unquoted values that are numbers, so a record with `;obs` or `if=core.s` is dropped (R13). There are no helpers for Uri-Query and Location-Path (R5, R15). There is no support for ESP32. DTLS existed only as a wolfSSL client and was removed; there is no server role with PSK (R16–R19). SenML-CBOR is absent (R14).

## Main problems

1. **It is a set of building blocks, not a library.** The server logic lives in an example, and the client is called a template. A user cannot write "register the `/temp` resource with a GET handler".
2. **There is no message layer.** No separate module is visible for CON retransmissions, deduplication, or matching requests and responses by token. Without it, CoAP over UDP is unreliable.
3. **The parser is unsafe on untrusted input.** For a network library this is a blocker.
4. **The build is off-putting.** wolfSSL and mbedTLS are both mandatory, `-O0 -ggdb` are hard-coded globally, there is no `install` or `find_package`, tests are always built (so cross-compilation is impossible), the platform in `build.sh` is selected by editing the script, and the repository contains `firmware.bin` and images.
5. **Platform dependencies remain in the core.** `htons` in `blockwise.cc`, `rand`/`time` for tokens, globally enabled spdlog, a wolfSSL header in the library target, cJSON in a public header, an unused `<iostream>`.
6. **There is no niche.** Linux has libcoap, and competing with it as "one more implementation" is pointless.

## Direction

Position libcoapcpp as a modern C++ CoAP library for microcontrollers: a core without I/O and with thin transport adapters. In this niche libcoap is heavy, and the C++ alternatives are either abandoned or aimed at Arduino.

CoAP Smart Home sets the priorities: ESP32 and OpenWrt join the target platforms, and DTLS-PSK in both roles, Observe on both sides, and streaming block-wise become mandatory rather than "later".

## Stages

| Stage | Result | Release |
|---|---|---|
| 1. Hygiene | Safe parser, fuzzing, CI | `v1.0.6` |
| 2. Core without platform dependencies | `src/` builds for `arm-none-eabi` without any port | `v1.0.x` |
| 3. New directory structure | Engine in the library, examples separated from boards | `v1.1.0` or `v2.0.0` |
| 4. Microcontrollers | One `coap-server` for all boards, `platformio` in `main` | per board |
| 5. Full CoAP | Message layer, resource API, Observe | first public release |
| 6. Visibility and packaging | PlatformIO Registry, OpenWrt package, interoperability tests | later |

Stages 4 and 5 do not depend on each other, but both build on stage 3. The PoC of CoAP Smart Home needs part of each: the ESP32 board from stage 4 and the message layer with Observe from stage 5.

## Mapping to CoAP Smart Home stages

| System stage | What the library must provide | Requirements | Library stage |
|---|---|---|---|
| 0.1 PoC | `Endpoint` with the message layer, server and client API, Observe on both sides, SenML-JSON; embedding in the gateway's event loop on Linux; the ESP32 board | R1–R5, R7–R9, R14, R15, R21 | 1–3, 5; ESP32 from 4 |
| 0.2 Registration and identity | Uri-Query and Location-Path, Block1, CoRE Link with `obs` and ptoken values, `/.well-known/core` with filtering | R5, R6, R12, R13 | 5 |
| 0.3 Stock firmware and SDK | Resources created at run time, PlatformIO and the ESP-IDF component, Pico W | R5, R21, R22 | 4 |
| 0.4 Provisioning and security | DTLS-PSK on mbedTLS, a server with sessions and `PskStore`, identity in handlers, both directions over one association | R16–R19 | 4 (DTLS) |
| 0.5 OTA | Streaming Block2 on both sides with resume and ETag | R10, R11 | 5 |
| 0.7 OpenWrt | Package, musl build, memory budget | R22, R23 | 6 |
| 1.0 Stable | Interoperability in CI, limits, fuzzing of all parsers | R24, R25 | 1, 6 |
| after 1.0 | OSCORE and EDHOC, multicast, SenML-CBOR, STM32 | R14, R20, R26 | 6 |

The system's PoC does not need DTLS: it runs over plain UDP inside the LAN. Everything up to 0.4 can be developed on `boards/posix` and ESP32 without the security layer.

### Stage 1. Hygiene

Done when: CI is green including the sanitizers, and a bugfix release is out.

#### Regression tests (`test/test_packet.cc`)

Put the buffer into a `std::vector` of the exact size, otherwise ASan will not see an out-of-bounds access on a stack array.

| Input (hex) | Expectation |
|---|---|
| `60 00 12 34` | success, no options and no payload (#1) |
| `40 01 00 01 D0` | error, truncated delta extension (#2) |
| `40 01 00 01 E0 00` | error, one extension byte out of two (#2) |
| `40 01 00 01 05 61` | error, option length exceeds the remainder |
| `40 01 00 01 F0` | error, reserved nibble 15 |
| `40 01 00 01 FF` | error, marker without a payload |
| `40 01 00` | error without a crash |
| `49 01 00 01 …` | error, token length 9 |

- [ ] Add the tests from the table.

#### Fuzzing

Start only after the `assert`s are removed, otherwise the fuzzer will stop on them within the first second.

- [ ] A `fuzz/fuzz_packet.cc` target with `LLVMFuzzerTestOneInput`: it calls `Packet::parse` and, on success, `serialize` and a second `parse`, comparing the results.
- [ ] A separate CMake option (for example, `COAPCPP_FUZZ`), clang only, flags `-fsanitize=fuzzer,address,undefined`.
- [ ] An initial corpus in `fuzz/corpus/packet/`: the packets from the table above and `testCoapPacket` from the tests.
- [ ] Turn every crash file found into a regression test.
- [ ] Cover `core_link.cc`, `senml_json.cc`, `uri.cc`, `blockwise.cc`, and `base64.cc` with the same template: all of them parse untrusted input.

#### GitHub Actions

What has to be worked around: `build.sh` has a hard-coded `TARGET`, so call `cmake` directly in CI; the tests need the `GTEST_DIR` variable; there is no `enable_testing()` or `add_test`; `testDnsResolver.*` accesses the network, so it should be excluded in CI; wolfSSL and mbedTLS are built every time, so `ccache` is needed.

- [ ] **build-test:** a gcc and clang matrix on `ubuntu-latest`, `actions/checkout` with `submodules: recursive`.
- [ ] **sanitizers:** clang with `-fsanitize=address,undefined -fno-sanitize-recover=all`.
- [ ] **fuzz-smoke:** each fuzz target for 60 seconds per PR; a long run on a weekly schedule.
- [ ] Add a badge to the README after the first green run.

#### Repository and release

- [ ] Add a repository description and topics (`coap`, `cpp`, `iot`, `embedded`, `lwip`, `dtls`, `stm32`, `raspberry-pi-pico`, `rfc7252`).
- [ ] Check `git branch --contains v1.0.5`. If the old tags are reachable only from `platformio`, mention this in the release notes.
- [ ] A tag on `main` after CI is green, with a GitHub Release and a short changelog.

### Stage 2. Core without platform dependencies

Done when: `src/` builds with `arm-none-eabi-g++` without any port, and the `grep` check from the architecture description passes in CI. The public API does not change.

#### Clean up the core in place

- [ ] `blockwise.cc`: write the port with two shifts instead of `htons`; remove `<arpa/inet.h>` and `lwip/def.h`.
- [ ] `core_link.cc`, `senml_json.cc`: remove `<iostream>`, which is unused but costs hundreds of kilobytes of flash on a microcontroller.
- [ ] `wolfssl_error.*`: move out of the library target; nothing in the core uses it.
- [ ] `senml_json.h`: replace `#include "cJSON.h"` with a forward declaration `struct cJSON;`.
- [ ] `packet.h`: move `DataType` into a separate header so that the packet codec does not drag cJSON along.
- [ ] `packet.h`: replace the bit fields under `#pragma pack` with masks and shifts; after that `is_little_endian_byte_order()` becomes unnecessary.
- [ ] `consts.h`: add `OBSERVE = 6` to `OptionNumber` and helpers for uint option values (R15). This is an addition; the API stays compatible.

#### Port interfaces and POSIX ports

- [ ] Add `api/coap/port/` with the `Transport`, `Clock`, `Random`, and `Log` interfaces.
- [ ] `packet.cc`: take the token and Message ID from `coap::port::Random` instead of `srand(time(nullptr))` and `rand()`. On a microcontroller without an RTC, the old scheme yields the same sequence after every restart.
- [ ] Log through `coap::port::Log`; remove the global `USE_SPDLOG` and the `debug(...)`, `set_level(...)` macros.
- [ ] Add `port/net/posix` and `port/os/posix`. Take the transport code from the old `src/unix/unix_socket.cc`.
- [ ] `Transport::peer_identity` with a default implementation that returns `false` (R18); `native_handle()` in `port/net/posix` for the application's own `poll` (R3).
- [ ] Add fake implementations in `test/mocks/`.
- [ ] Bring back `udp-echo` as a smoke test of the port and the shortest sample of a `Transport` implementation.

#### Rewrite CMake

- [ ] Split into targets: the `coapcpp` core without dependencies and separate port targets.
- [ ] Options `COAPCPP_WITH_FORMATS`, `COAPCPP_WITH_DTLS`, `COAPCPP_BUILD_TESTS`; the TLS backend is chosen with an option, spdlog and cJSON become optional.
- [ ] Remove the global `CMAKE_CXX_FLAGS` with `-O0 -ggdb -frtti`, the global `include_directories`, and `remove_definitions(-DNDEBUG)`.
- [ ] Pull in third-party libraries through `find_package` or `FetchContent`, and only when needed.
- [ ] Add `install` and CMake package export, `enable_testing()` and `add_test`.
- [ ] A `.pc` file for pkg-config, `BUILD_SHARED_LIBS`, the system mbedTLS through `find_package` with `third-party/` as a fallback (R22).
- [ ] Add toolchain files in `cmake/toolchains/` and a cross-compilation check of the core in CI.
- [ ] Move `firmware.bin` and the images out of the repository; add `.pio/` to `.gitignore`.

### Stage 3. New directory structure

Done when: `examples/coap-server` builds for `boards/posix` with the `posix`, `raspberry-pi`, and `stm32mp157a-dk1` presets. This stage changes header paths and namespaces.

- [ ] Move the headers to `api/coap/`; temporarily support old paths such as `#include "packet.h"` with forwarding headers.
- [ ] Move `Buffer`, `ConnectionType`, and `CoapStatus` into `namespace coap`: the names `TCP` and `UDP` in the global namespace risk colliding with HAL or SDK macros.
- [ ] Move the engine from `examples/POSIX/common` to `src/`; replace the `select` loops with `coap::run` from `port/net/posix`.
- [ ] Separate examples and boards: `examples/coap-server` and `examples/coap-client` with `app_main`, `boards/posix`, `boards/posix/gpio`.
- [ ] Replace sensor selection through `#ifdef __arm__` with the `SENSORS=stub|gpio` option, and wiringPi with libgpiod.
- [ ] Describe the build presets in `CMakePresets.json`; reduce `build.sh` to `cmake --preset <name>`.
- [ ] Move the STM32MP157A-DK1 README to `boards/posix/docs/stm32mp157a-dk1.md`.
- [ ] Delete `examples/POSIX` and `examples/RASPBERRY-PI-4`.

Where the existing files move:

| Now | Becomes | Changes along the way |
|---|---|---|
| `api/*.h` | `api/coap/*.h` | `namespace coap` |
| `src/wolfssl_error.*` | `port/net/dtls/wolfssl/` | none |
| `examples/POSIX/common/coap_server.*`, `coap_client.*`, `packet_helper.*` | `src/server.cc`, `client.cc`, `packet_helper.cc`; `api/coap/server.h`, `client.h` | `namespace posix` → `coap`; file serving through a callback instead of `<fstream>`; `htons`/`htonl` → shifts |
| `examples/POSIX/common/sensor.*`, `endpoint.*`, `sensor_stubs.*` | `examples/coap-server/` | time through `coap::port::Clock` |
| `examples/POSIX/common/trace.h` | `boards/posix/` | depends on `<iostream>` |
| `examples/POSIX/coap-*/main.cc` | `boards/posix/` | `select` loop → `coap::run`; example logic → `app_main` |
| `examples/RASPBERRY-PI-4/coap-server/main.cc` | delete | duplicate; differs only in pins |
| `examples/RASPBERRY-PI-4/common/DHT11.*`, `RGB_LED.*` | `boards/posix/gpio/` | wiringPi → libgpiod |

### Stage 4. Microcontrollers

Done when: `examples/coap-server/` builds unchanged for all boards, and each board directory contains only hardware initialization, port creation, and registration of its own resources.

ESP32 comes first: it is the MVP platform of CoAP Smart Home. Pico W follows, needed by system 1.0. The rest follow the hardware at hand:

| Order | Board | What it requires |
|---|---|---|
| 1 | `esp32-devkitc` | `port/net/lwip` (sockets), `port/os/freertos` (`clock.cc` only), `port/hal/esp32`; the ESP-IDF component |
| 2 | `pico-w` | `port/hal/pico`; ADXL345 as a resource |
| 3 | `nucleo-f429zi` | `port/net/lwip` (sockets), `port/os/freertos`, `port/hal/stm32`; the BSP from `60f749e^`; `/rtc` as a resource |
| 4 | `pico-spi` (firmware and gateway) | a shared frame codec in `boards/pico-spi/common` |
| 5 | `nucleo-h743zi` | a new BSP, everything else unchanged |
| 6 | `pico-eth` | an RMII-on-PIO driver as an lwIP `netif`; adds no ports |
| separately | `dtls-echo-server` | `port/net/dtls/mbedtls`, then `wolfssl`; needed by system 0.4 |

#### ESP32

- [ ] `port/hal/esp32`: `Random` on `esp_fill_random`, `Clock` on `esp_timer_get_time`.
- [ ] `boards/esp32-devkitc`: an ESP-IDF project with the same `examples/coap-server`. lwIP sockets from ESP-IDF go through `udp_transport_sockets.cc`; `heap.cc` is not needed.
- [ ] ESP-IDF component: `idf_component.yml` and an `idf_component_register` branch in the root `CMakeLists.txt` (R22). Moved here from stage 6, since the system's PoC needs it.
- [ ] Build under PlatformIO `espressif32` with the `espidf` and `arduino` frameworks: the SmartNode SDK has not chosen its framework yet.
- [ ] CI build for ESP32 (Xtensa) and ESP32-C3 (RISC-V).
- [ ] Check `BUFFER_SIZE` and the task stack against the RAM of ESP32-C3 and the Wi-Fi MTU.

#### NUCLEO-F429ZI

The most valuable part of the removed code: a working BSP that `main` does not have.

- [ ] Restore unchanged into `boards/nucleo-f429zi/bsp/`: `Core/Src`, `LwIP/App`, `LwIP/Target/ethernetif.c`, `lwipopts.h`, `FreeRTOSConfig.h`, `stm32f4xx_hal_conf.h`, `flash.sh`, `monitor.sh`, README.
- [ ] `main.cc`: keep the hardware initialization and the DHCP task; replace the UDP server task with one that creates the ports and calls `coap::run`.
- [ ] Turn the `get rtc` and `set rtc` commands from the old `command.cc` into `GET`/`PUT /rtc` with SenML-JSON.
- [ ] Initialize the RNG in the BSP so that `port/hal/stm32` works.
- [ ] Check the `Buffer` size (1600 bytes) against the lwIP heap and the task stack; the old server had 256 bytes for receive and 512 for transmit.
- [ ] Replace the seven `.make` scripts with a single board `CMakeLists.txt` and a toolchain file.
- [ ] Do not bring `NUCLEO-F429ZI.jpeg` and `firmware.bin` back into git.

#### Pico W and Pico 2 W

- [ ] Wi-Fi through `cyw43` and lwIP from the Pico SDK, the same `examples/coap-server`.
- [ ] From the old code, take `FreeRTOSConfig.h`, the SDK import scripts, `flash.sh`, `monitor.sh`, and the `ADXL345.*` driver, which becomes the `/sensors/ADXL345` resource.
- [ ] Use `udp_transport_sockets.cc` under FreeRTOS and `udp_transport_raw.cc` without an OS.

#### Pico over SPI

- [ ] Merge the frame codec, currently duplicated in two copies of `pico_protocol.h`, into `boards/pico-spi/common/spi_framing.*`.
- [ ] Firmware: `SpiFramedTransport` on top of `hardware_spi`.
- [ ] Gateway: a UDP ↔ SPI bridge on Linux through spidev.

#### NUCLEO-H743ZI and Pico with Ethernet on PIO

- [ ] `nucleo-h743zi`: a copy of `nucleo-f429zi` with a BSP generated for H7. If `main.cc`, the resources, and the ports stay the same, the port split works.
- [ ] `pico-eth`: a PIO program, a `netif` driver, PHY access over MDIO. A third-party driver, if one is used, goes into `third-party/`.

#### DTLS

mbedTLS comes first: it is the variant needed on ESP32, OpenWrt, STM32, and Pico (R16).

- [ ] `port/net/dtls/mbedtls`: modeled on `tls-client2.cc`, but DTLS 1.2, PSK, and `TLS_PSK_WITH_AES_128_CCM_8`.
- [ ] Server role: sessions keyed by peer address, cookies, idle-session timeout, a session limit, the key through `PskStore` (R17).
- [ ] `peer_identity` for request handlers (R18); requests in both directions over one association (R19).
- [ ] Interoperability with `coap-client -k` and `coap-server -k` from libcoap.
- [ ] `port/net/dtls/wolfssl`: take the handshake from the old `unix_dtls_client.cc`, `dtls_server.cc`, `tls-client1.cc`.
- [ ] Replace the embedded root certificate in `my_certificates.cc`: it appears to be DST Root CA X3, which expired in September 2021.
- [ ] Bring `dtls-echo-server` back into `boards/posix/`.

#### PlatformIO

- [ ] Add `library.json` to the root of `main` with `srcFilter` on `src/` and the required `port/`; platforms `espressif32`, `raspberrypi`, `ststm32`.
- [ ] Move the example from the `platformio` branch and turn it into a real CoAP server.
- [ ] Close the `platformio` branch.

#### How to get files from the history

Files are taken from the parent commit, without reverting `60f749e` itself:

```sh
# look at a file
git show 60f749e^:examples/NUCLEO-F429ZI/udp-server/Core/Src/main.cc

# restore a directory into the working tree and move it right away
git checkout 60f749e^ -- examples/NUCLEO-F429ZI
git mv examples/NUCLEO-F429ZI/udp-server/Core boards/nucleo-f429zi/bsp/Core
```

`git revert 60f749e` is not suitable: it would also bring back the old network layer, and `CMakeLists.txt`, `build.sh`, and `src/utils.h` have changed since then.

Where the old code goes:

| Removed | New location |
|---|---|
| `src/unix/unix_socket.cc`, `unix_udp_client.cc`, `unix_udp_server.cc` | `port/net/posix/udp_transport.cc` |
| `src/lwip/lwip_socket.cc` | `port/net/lwip/udp_transport_sockets.cc` |
| `src/unix/unix_dns_resolver.cc`, `src/lwip/lwip_dns_resolver.cc` | `port/net/posix/resolver.cc`, `port/net/lwip/resolver.cc` |
| `src/unix/unix_dtls_client.cc`, `examples/POSIX/common/dtls_server.cc` | `port/net/dtls/wolfssl/dtls_transport.cc` |
| `examples/POSIX/tls-client2/tls-client2.cc`, `mbedtls_error.*` | `port/net/dtls/mbedtls/` |
| `test/test_socket.cc`, `test_dns_resolver.cc` | `port/net/posix/test/` |
| `api/socket.h`, `connection.h`, `dns_resolver.h`, `endpoint.h`, `src/unix/unix_endpoint.*` | do not restore |
| `examples/POSIX/tcp-client`, `tls-client1`, `tls-client2` | do not restore as examples: they are HTTP clients |

### Stage 5. Full CoAP

Done when: a server with one resource and a client for it fit into a 30-line example, and the CoAP Smart Home scenarios from the table below pass on `loopback_transport` with a fake clock.

#### Message layer and endpoint

- [ ] CON/NON/ACK/RST, retransmissions with backoff, deduplication, piggybacked and separate responses, `NSTART` (R1).
- [ ] `Endpoint`: server and client on one transport; many transactions matched by token and address (R2, R4). The `server` and `client` from stage 3 remain thin wrappers.
- [ ] `next_timeout_ms()` and `tick(now)`; `coap::run` is built on them (R3).
- [ ] Limits in `config.h` and the behaviour when they are exhausted (R24).

#### Server and client API

- [ ] Routing by Uri-Path with a variable segment, method handlers, resources added at run time, Uri-Query, Location-Path, deferred response (R5).
- [ ] Automatic `/.well-known/core` with `?rt=` filtering (R6).
- [ ] A client API: `get`, `post`, `put`, `delete`, and an arbitrary request, with a callback and cancellation (R7).

#### Observe (RFC 7641)

- [ ] Server side: registration, deregistration, sequence number, a CON notification once every 24 hours (R8).
- [ ] Client side: one subscription per resource, dropping stale notifications, re-registration, a callback on subscription loss (R9).

#### Streaming block-wise

- [ ] Block2 server from an `(offset, size)` callback, Size2, ETag, any block on request (R10).
- [ ] Block2 client with a callback per block, a start from an arbitrary block, abort on an ETag change (R11).
- [ ] Block1 on both sides with a reassembly limit (R12).

#### Formats

- [ ] CoRE Link: attributes without a value, ptoken values, several values in one attribute (R13). Add seeds for these cases to `fuzz/corpus/core_link/` and regression tests.
- [ ] SenML-JSON: check every field from R14 against RFC 8428 with tests.

#### CoAP Smart Home scenarios

| Scenario | What is checked |
|---|---|
| Registration | `POST /rd?ep=…&lt=…` with a link-format payload longer than one block → `2.01 Created` with Location-Path; refresh `POST /rd/{id}` |
| Observe | subscription, notifications, an older notification after a newer one is dropped, re-registration after loss |
| Control | `PUT /actuators/relay` with a separate response |
| OTA | a Block2 download of 1 MB with 10% datagram loss; the download resumes from the needed block; an ETag change aborts it |

### Stage 6. Visibility and packaging

- [ ] Publish the library in the PlatformIO Registry and the ESP-IDF Component Registry.
- [ ] OpenWrt package for system 0.7: `packaging/openwrt/Makefile`, then a submission to `openwrt/packages`. Account for the APK/OPKG difference between OpenWrt versions (R22).
- [ ] CI build with the OpenWrt SDK against musl for one MIPS and one ARM target (R21).
- [ ] Measure the size of the stripped library and the RSS of the example in CI and compare them with the budget (R23).
- [ ] Check interoperability with libcoap, aiocoap, and Californium in CI, over plain UDP and DTLS-PSK (R25).
- [ ] CBOR and SenML-CBOR (R14).
- [ ] Multicast for fallback discovery (R26).
- [ ] OSCORE (RFC 8613) and EDHOC (RFC 9528): CoAP Smart Home needs them after its 1.0 (R20). CoAP over TCP (RFC 8323) stays optional.

### Strategic option: an LwM2M client

SenML, CoRE Link, and firmware delivery over block-wise already exist. A lightweight C++ LwM2M client for microcontrollers would give the project practical value, but it is a large amount of work and should be taken on only after stage 5. CoAP Smart Home borrows the LwM2M object model but does not implement the protocol, so LwM2M stays an option rather than a requirement.

## Minimum for the first public release

Stages 1–3 and 5, plus a 30-line example in the README: a server with one resource and a client for it.

## Minimum for the CoAP Smart Home PoC

Stages 1–3; from stage 4, the `esp32-devkitc` board; from stage 5, the message layer, `Endpoint`, the server and client API, Observe on both sides, and SenML-JSON. DTLS and streaming block-wise are not needed yet.

## Versions

- `v1.0.6`: stage 1, parser fixes without API changes.
- `v1.0.x`: stage 2, the public API does not change.
- `v1.1.0` or `v2.0.0`: stage 3, header paths and namespaces change.

## Open questions

1. **The next tag number.** The plan assumes `v1.0.6` on `main`. If that number is already taken by a release on the `platformio` branch, the bugfix release gets the next free one.
2. **C++ standard.** The port interfaces are compatible with C++11, but the build wishlist includes moving to C++17 for `string_view` and `optional`. It has to be decided whether C++11 remains the minimum for the core.
3. **Dynamic memory.** The niche implies a core without a heap, but today it uses `std::string`, `std::vector`, and `new[]`, and no stage changes that. Either a separate stage or an honest wording of the niche is needed. For CoAP Smart Home (Linux, OpenWrt, ESP32, Pico W) the heap is acceptable, so the question can wait until STM32.
4. **Order of stages 4 and 5.** Boards give a visible result; the message layer gives real CoAP. For CoAP Smart Home the order is set: the ESP32 board runs in parallel with stage 5, and the other boards come after it.
5. **RMII-on-PIO driver.** The suitability of third-party drivers for RP2350 has not been checked; on RP2040 there may not be enough clock headroom for 100 Mbit/s.
6. **RD client in the library.** Registration with a refresh every `lt` seconds (RFC 9176) is a generic mechanism. It can be an optional library module or part of the SmartNode SDK. The library already takes no part in the RD server.
7. **CBOR codec.** cJSON does not handle CBOR. SenML-CBOR needs either a minimal codec of our own or a third-party library (tinycbor, QCBOR).
8. **SmartNode SDK framework.** The SDK API in the system description looks like Arduino (`setup`/`loop`), while OTA with rollback is native to ESP-IDF. Until the choice is made, the library is built under both PlatformIO frameworks.
9. **DTLS session and gateway restarts.** With one association for both directions (R19), after a gateway restart the node must notice the session is lost and repeat the handshake. It remains to be decided whether that is the library's job (a callback on a DTLS alert or timeout) or the RD client's job.
