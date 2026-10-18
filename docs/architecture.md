# libcoapcpp Architecture

Status as of 2026-10-18.

This document describes the target structure of the project: what it should become after the rework, not what it is today. The sequence of steps leading to it is given in the [roadmap](roadmap.md).

## Goal

libcoapcpp is being developed as a C++ CoAP library for microcontrollers and POSIX systems. The core performs no I/O (the sans-IO approach), and everything platform-specific is plugged in through thin adapters.

Target platforms:

- POSIX: Linux on x86, Raspberry Pi 4/5, STM32MP1 (Cortex-A, OpenSTLinux), OpenWrt routers (musl, MIPS and ARM);
- ESP32, ESP32-C3, ESP32-S3 (ESP-IDF: FreeRTOS, lwIP, mbedTLS);
- STM32F4 and STM32H7;
- RP2040 and RP2350 (Raspberry Pi Pico, Pico W, Pico 2, Pico 2 W).

The main consumer of the library is the CoAP Smart Home system ([description](coap_smart_home_system.md)). Its gateway `coap-manager` runs on Linux and OpenWrt, and its nodes (stock firmware and the SmartNode SDK) run on ESP32 and Pico W. The requirements the system puts on the library are collected in [CoAP Smart Home requirements](#coap-smart-home-requirements).

## Principles

1. **A core without I/O.** `src/` includes no system headers and contains no platform `#ifdef`.
2. **Everything platform-specific is hidden behind narrow interfaces.** There are five: transport, clock, random numbers, log, and an optional name resolver. They are declared in `api/coap/port/` and implemented in `port/`.
3. **Ports are split by software API, not by board.** There are three axes: network stack (`port/net`), operating system (`port/os`), and chip (`port/hal`). Each platform is a combination of directories from these axes.
4. **Board code lives only in `boards/`.** That means `main`, pins, clocking, `lwipopts.h`, `FreeRTOSConfig.h`, the linker script, and sensor drivers. Example logic lives in `examples/` and contains no platform-specific lines.
5. **The build system selects the platform**, through the list of CMake targets, not through the preprocessor in source files.

## Dependency direction

Dependencies point one way:

```
boards/*  ──►  port/*  ──►  api/coap/port/*.h  ◄──  src/*
   │                                                  ▲
   └──►  examples/*  ─────────────────────────────────┘
```

- `src/` includes only the C++ standard library, `api/coap/` and, for the formats, cJSON.
- `port/X/Y` includes `api/coap/port/` and the headers of its own platform. Ports do not include each other. The exception is `port/net/dtls`, which wraps any transport through the interface.
- `examples/` includes only `api/coap/`: no `port/` and no system headers.
- `boards/Y` puts the required ports together, adds the board code, and calls `app_main` from `examples/`.

The rule is checked with a single command that must return an empty result. It is worth running in CI:

```sh
grep -rE '#\s*if.*(__linux__|__unix__|__arm__|STM32|PICO_|LWIP|ESP_PLATFORM|CONFIG_IDF)' api src examples
```

## Directory structure

```
libcoapcpp/
├── api/coap/                    public headers; included as <coap/packet.h>
│   ├── consts.h  error.h  buffer.h  packet.h  uri.h  blockwise.h  utils.h
│   ├── core_link.h  senml_json.h  data_type.h        formats
│   ├── server.h  client.h  endpoint.h                engine: one endpoint = server + client
│   ├── messaging.h  observe.h  block_transfer.h      message layer, Observe, streaming block-wise
│   ├── config.h                                      sizes and limits, overridable
│   └── port/                    <coap/port/transport.h>
│       └── transport.h  clock.h  random.h  log.h  resolver.h  psk_store.h     pure interfaces
│
├── src/                         no system headers, no platform #ifdef
│   ├── packet.cc  uri.cc  blockwise.cc  error.cc  utils.cc
│   ├── core_link.cc  senml_json.cc  base64.cc  base64.h
│   ├── server.cc  client.cc  endpoint.cc  packet_helper.cc
│   └── messaging.cc  observe.cc  block_transfer.cc
│
├── port/                        interface implementations; each directory is a separate CMake target
│   ├── net/                     Transport, Resolver
│   │   ├── posix/     udp_transport.cc  resolver.cc
│   │   ├── lwip/      udp_transport_sockets.cc  udp_transport_raw.cc  resolver.cc
│   │   └── dtls/
│   │       ├── wolfssl/   dtls_transport.cc  wolfssl_error.cc  wolfssl_error.h
│   │       └── mbedtls/   dtls_transport.cc     (client and server, PSK)
│   ├── os/                      Clock, Log, heap
│   │   ├── posix/     clock.cc  random.cc  spdlog_log.cc
│   │   └── freertos/  clock.cc  heap.cc
│   └── hal/                     Random, Clock without an OS
│       ├── stm32/     random.cc  clock.cc
│       ├── pico/      random.cc  clock.cc
│       └── esp32/     random.cc  clock.cc
│
├── examples/                    example logic; no main() and no platform code
│   ├── coap-server/   app_main.cc  sensor.*  endpoint.*  sensor_stubs.*
│   └── coap-client/   app_main.cc
│
├── boards/                      main(), BSP, port creation, board resources
│   ├── posix/                main_server.cc  main_client.cc  (getopt, signals)
│   │   ├── gpio/             dht11.cc  rgb_led.cc  (libgpiod)
│   │   └── docs/             instructions for Raspberry Pi and STM32MP157A-DK1
│   ├── esp32-devkitc/        main.cc  sdkconfig.defaults    (ESP-IDF project; ESP32, ESP32-C3, ESP32-S3)
│   ├── nucleo-f429zi/        main.cc  bsp/  lwipopts.h  FreeRTOSConfig.h
│   ├── nucleo-h743zi/        main.cc  bsp/  lwipopts.h  FreeRTOSConfig.h
│   ├── pico-w/               main.cc  lwipopts.h  FreeRTOSConfig.h    (Pico W and Pico 2 W)
│   ├── pico-eth/             main.cc  rmii.pio  pio_netif.cc  lwipopts.h  (RP2350, Ethernet MAC on PIO)
│   └── pico-spi/             CoAP over SPI for a Pico without networking
│       ├── common/       spi_framing.*            frame codec, platform-independent
│       ├── firmware/     main.cc  spi_transport.cc     (RP2040 / RP2350)
│       └── gateway/      main.cc  spidev.cc            UDP ↔ SPI bridge on Linux
│
├── cmake/                       Pico SDK and FreeRTOS import scripts
│   └── toolchains/              arm-none-eabi-cortex-m4f.cmake  …-m7.cmake  aarch64-linux-gnu.cmake
├── test/                        host only
│   ├── mocks/     fake_clock.h  fake_random.h  loopback_transport.h
│   └── test_*.cc
├── packaging/
│   └── openwrt/                 Makefile of the libcoapcpp package
├── docs/
├── library.json                 PlatformIO, with srcFilter on src/ and the required port/
├── idf_component.yml            ESP-IDF component
└── third-party/
```

## Core

The core consists of `api/coap/` and `src/`.

- **Codec and formats.** Packets (RFC 7252), block-wise transfers (RFC 7959), URI, SenML-JSON (RFC 8428), CoRE Link Format (RFC 6690).
- **Engine.** `server` and `client` work through `processing(Buffer &)`: bytes in, bytes out. The engine knows nothing about the transport. Both are combined in `Endpoint`: one transport, one message layer, the server and client roles at once (R2).
- **Processing loop.** `coap::run(endpoint, transport, clock)` reads datagrams from the transport and passes them to `processing`. Anyone with their own event loop calls `processing` directly and drives the timers through `next_timeout_ms()` and `tick(now)` (R3).
- **Threads.** The core is single-threaded: one instance serves one thread or one task. There are no mutexes among the ports.
- **Byte order.** Multi-byte fields are written with shifts in the core itself; this is not a port, and `htons` is not needed.
- **Dependencies.** The C++ standard library and cJSON. cJSON is needed only by the formats and is enabled with the `COAPCPP_WITH_FORMATS` option.
- **Configuration.** Buffer sizes and limits are collected in `api/coap/config.h` and can be overridden by the user's project.

Limitation: the core uses dynamic memory (`std::string`, `std::vector`, `new[]`). With `-fno-exceptions`, a failed allocation terminates the program. On FreeRTOS, the port redirects `operator new` and `malloc` to the RTOS heap, and cJSON is configured through `cJSON_InitHooks`. Removing the heap is separate work and is not covered by this architecture. For the CoAP Smart Home targets (Linux, OpenWrt, ESP32, Pico W) a heap is acceptable; the question matters only for small STM32 parts.

## CoAP Smart Home requirements

The [CoAP Smart Home](coap_smart_home_system.md) system uses the library in two roles:

| Role | Platform | What it does over CoAP |
|---|---|---|
| Gateway `coap-manager` | Linux, OpenWrt | Server: Resource Directory `/rd`, firmware images `/fw` over Block2. Client: requests to nodes, one Observe subscription per resource, arbitrary requests from the Resource Explorer. Tens of nodes on one socket. |
| Node (stock firmware, SmartNode SDK) | ESP32, Pico W; STM32 after system 1.0 | Server: sensor and actuator resources, `/.well-known/core`, `/system/*`. Client: registration and its refresh every `lt` seconds, image download over Block2. |

**Boundary.** libcoapcpp stays independent of the system. It implements generic IETF mechanisms and knows nothing about the semantics of `/rd`, the `rt` vocabulary, `/system/*`, HTTP, SSE, the firmware manifest, or image signatures. The RD server, the device registry, the HTTP-to-CoAP mapping (RFC 8075), and the OTA logic live in `coap-manager`. The device profile, driver configuration, and the OTA backend (flash slots) live in the SmartNode SDK. A device built on libcoap or aiocoap without the SDK must work with the gateway unchanged.

The requirements are numbered so that the [roadmap](roadmap.md) can refer to them.

### Engine

| # | Requirement | Gateway | Node |
|---|---|---|---|
| R1 | Message layer: CON/NON/ACK/RST, retransmissions with exponential backoff, deduplication by (peer, Message ID) within `EXCHANGE_LIFETIME`, piggybacked and separate responses, `NSTART` per peer. The RFC 7252 §4.8 parameters are in `config.h`. | yes | yes |
| R2 | One `Endpoint` on one transport plays the server and client roles at once. The gateway accepts `/rd` and sends GET/PUT/Observe from the same port; the node registers and serves resources from the same socket, so the gateway reaches it at the source address of the registration. | yes | yes |
| R3 | No call blocks. Results arrive through callbacks, and a transaction can be cancelled. The engine reports `next_timeout_ms()` and is advanced by `tick(now)`, so it embeds in the gateway's own event loop next to the command queue from the HTTP threads (system §3.3.1). The engine stays single-threaded. | yes | yes |
| R4 | Many concurrent transactions to different peers, matched by token and peer address; tokens of at least 4 random bytes. | yes | — |
| R5 | Server API: routing by Uri-Path including a variable segment (`/rd/{id}`, `/fw/{board}/{file}`); adding and removing resources at run time (the stock firmware creates them from its configuration); access to Uri-Query (`ep`, `lt`); setting Location-Path (`2.01 Created`, `Location: /rd/4521`); a deferred response for slow handlers; automatic 4.04, 4.05, 4.06, and 4.15. | yes | yes |
| R6 | Automatic `/.well-known/core` from resource attributes (`rt`, `if`, `obs`, `ct`, `sz`) with filtering by query (`?rt=temperature`, RFC 6690 §4.1). The same serializer produces the registration payload. | — | yes |
| R7 | Client API with an arbitrary method, options, Content-Format, Accept, and payload: the Resource Explorer sends arbitrary requests. | yes | yes |

### Observe (RFC 7641)

| # | Requirement | Gateway | Node |
|---|---|---|---|
| R8 | Server: registration and deregistration (Observe 0 and 1, RST, failed CON notification); a 24-bit sequence number; a notification on change; a CON notification at least once every 24 hours; Max-Age; a limit on the number of observers. | — | yes |
| R9 | Client: one subscription per (peer, resource); dropping stale notifications by sequence number (§3.4); re-registration after Max-Age expires or after the node registers again; explicit cancellation; a callback both for each notification and for the loss of the subscription, so the gateway can mark the value stale. | yes | — |

### Block-wise (RFC 7959)

| # | Requirement | Gateway | Node |
|---|---|---|---|
| R10 | Block2 server without loading the resource into memory: data comes from an `(offset, size)` callback; Size2 and ETag; any block on request, so the node resumes the download after a break; a smaller block size requested by the client is honoured. A 1 MB image on the router does not go into RAM. | yes | — |
| R11 | Block2 client with a callback per block (writing into a flash slot), starting from an arbitrary block number, aborting on an ETag change, and Size2 known before the first write. Transparent reassembly is allowed only for small payloads, with a limit. | — | yes |
| R12 | Block1 on both sides: the node sends a registration payload longer than one block, and the gateway reassembles it with a limit, answering 2.31 Continue between blocks and 4.13 when the limit is exceeded. `PUT /system/config` uses the same mechanism. | yes | yes |

### Formats and options

| # | Requirement | Gateway | Node |
|---|---|---|---|
| R13 | CoRE Link Format per RFC 6690: attributes without a value (`obs`), values as a ptoken and as a quoted-string, several values in one attribute (`rt="a b"`); parsing and serializing are symmetrical. | yes | yes |
| R14 | SenML-JSON (Content-Format 110): `bn`, `bt`, `bu`, `n`, `u`, `v`, `vb`, `vs`, `vd`, `t`. SenML-CBOR (112) is an option and needs a CBOR codec of its own, because cJSON does not handle CBOR. | yes | yes |
| R15 | Options Observe (6), Uri-Query, Location-Path, Accept, Max-Age, ETag, Size1, Size2, with helpers for uint values. Content-Formats 40, 42, 50, 110, 112. | yes | yes |

### Security

| # | Requirement | Gateway | Node |
|---|---|---|---|
| R16 | DTLS 1.2 with PSK and `TLS_PSK_WITH_AES_128_CCM_8` (RFC 7252 §9.1.3.1). The mbedTLS backend comes first, because ESP-IDF and OpenWrt ship it. The node is a DTLS client, the gateway a DTLS server. | yes | yes |
| R17 | DTLS server: many simultaneous sessions keyed by peer address, cookies (HelloVerifyRequest), an idle-session timeout, a session limit. The key is looked up by identity through the `PskStore` callback; the keys are issued by the gateway during provisioning. | yes | — |
| R18 | The authenticated PSK identity reaches the request handler, so the gateway can check that `ep` in `/rd` matches the key and reject requests that arrived without DTLS. | yes | — |
| R19 | Requests in both directions over one DTLS association: the node opens the session, and the gateway sends GET/PUT/Observe to it through the same session. The node needs neither a DTLS server role nor a second session. | yes | yes |
| R20 | After system 1.0: OSCORE (RFC 8613) and EDHOC (RFC 9528). The message layer must leave room for a layer that protects options and payload between the engine and the transport. | later | later |

### Platforms, build, and quality

| # | Requirement | Gateway | Node |
|---|---|---|---|
| R21 | Platforms: the gateway on Linux x86/ARM and OpenWrt (musl, MIPS/ARM); the node on ESP32 (ESP-IDF; PlatformIO `espressif32` with the `espidf` and `arduino` frameworks) and Pico W (Pico SDK); STM32 after system 1.0. | yes | yes |
| R22 | Packaging: CMake `install` with package export and pkg-config, a static or shared library, the system mbedTLS through `find_package`, no mandatory submodule; an OpenWrt package; an ESP-IDF component; `library.json` for PlatformIO. | yes | yes |
| R23 | Memory budget: the library's share of the `coap-manager` package (≤ 1 MB together with CivetWeb and mbedTLS, RSS ≤ 10 MB on a router with 16 MB Flash / 128 MB RAM) is fixed and measured in CI. | yes | — |
| R24 | Limits in `config.h`: peers, transactions, observers, subscriptions, DTLS sessions, reassembly size. When a limit is exhausted, the answer is 5.03 with Max-Age or an error code, never a crash. All parsers are covered by fuzzing. | yes | yes |
| R25 | Interoperability with libcoap, aiocoap, and Californium is checked in CI, including DTLS-PSK: the system promises compatible devices built without the SDK. | yes | yes |
| R26 | Optional: multicast (RFC 7252 §8) for fallback discovery: joining `ff02::fd` and `224.0.1.187` in the port, NON requests, several responses to one token, no error responses to a multicast request. | later | later |

## Port interfaces

Four mandatory interfaces and two optional, all compatible with C++11. The core receives them through its constructor, so fake implementations can be substituted in tests.

```cpp
namespace coap { namespace port {

// api/coap/port/clock.h
class Clock {
public:
    virtual ~Clock() = default;
    virtual std::uint64_t now_ms() = 0;          // monotonic, not calendar time
};

// api/coap/port/random.h
class Random {
public:
    virtual ~Random() = default;
    virtual void fill(std::uint8_t *out, std::size_t size) = 0;
};

// api/coap/port/transport.h
struct Address {                                 // no sockaddr and no ip_addr_t
    std::uint8_t  bytes[16];
    std::uint8_t  length;                        // 4 or 16
    std::uint16_t port;                          // in host byte order
};

class Transport {
public:
    virtual ~Transport() = default;
    virtual void send(const Address &to, const std::uint8_t *data,
                      std::size_t size, std::error_code &ec) = 0;
    // returns 0 and COAP_ERR_TIMEOUT if nothing arrived within timeoutMs
    virtual std::size_t receive(Address &from, std::uint8_t *data, std::size_t capacity,
                                std::uint32_t timeoutMs, std::error_code &ec) = 0;
    // authenticated identity of the peer (the PSK identity for DTLS); false for plain UDP
    virtual bool peer_identity(const Address &peer, std::string &out) const
    { (void)peer; (void)out; return false; }
};

// api/coap/port/log.h
enum class LogLevel { Debug, Info, Warning, Error };
class Log {
public:
    virtual ~Log() = default;
    virtual void write(LogLevel level, const char *message) = 0;
};

// api/coap/port/resolver.h (optional, needed only by the client)
class Resolver {
public:
    virtual ~Resolver() = default;
    virtual void resolve(const char *hostname, std::uint16_t port,
                         Address &out, std::error_code &ec) = 0;
};

// api/coap/port/psk_store.h (optional, needed only by the DTLS server)
class PskStore {
public:
    virtual ~PskStore() = default;
    // false rejects the handshake
    virtual bool find(const std::uint8_t *identity, std::size_t identityLength,
                      std::uint8_t *key, std::size_t &keyLength) = 0;
};

}}
```

Purpose of the interfaces:

| Interface | Why the core needs it |
|---|---|
| `Transport` | Needed only by the `coap::run` loop; the engine works with buffers. `peer_identity` passes the authenticated identity of the peer to the request handlers (R18). |
| `Clock` | Client and server timeouts, and later CON retransmissions. |
| `Random` | Tokens and Message IDs. Per RFC 7252 §5.3.1, a token must be unpredictable. |
| `Log` | Diagnostics without a dependency on spdlog. |
| `Resolver` | Turning a host name into an `Address` in the client. |
| `PskStore` | Looking up the key by PSK identity in the DTLS server (R17). The application implements it: on the gateway, keys come from the device registry. |

## Ports

A port is an implementation of the interfaces for a particular software API, not for a piece of hardware. Raspberry Pi 4, Pi 5, STM32MP1, and x86 are identical for the library: the same sockets, `clock_gettime`, `getrandom`. STM32F4, STM32H7, and RP2040 share a network stack (lwIP) and differ only in their entropy source and clock.

Axes:

- `port/net`: network stack;
- `port/os`: operating system;
- `port/hal`: chip. This axis is needed because the random number generator and the clock without an OS belong neither to the network nor to the OS, and in `boards/` they would be duplicated between boards of the same family.

A board takes one directory from each axis it needs.

| Directory | What it implements | Notes |
|---|---|---|
| `port/net/posix` | `Transport`: `socket`, `sendto`, `recvfrom`, `poll`. `Resolver`: `getaddrinfo`. | Shared by all Linux targets, OpenWrt included. `native_handle()` gives the socket for the application's own `poll` (R3). Joining a multicast group is optional (R26). |
| `port/net/lwip` | `Transport` in two variants: on lwIP sockets (`udp_transport_sockets.cc`, requires an OS) and on the raw `udp_*` API (`udp_transport_raw.cc`). `Resolver`: `lwip_getaddrinfo`. | The board picks one of the two files; the core is unaware of it. The raw variant is needed only for builds without an OS (`NO_SYS=1`). |
| `port/net/dtls/wolfssl`, `port/net/dtls/mbedtls` | `DtlsTransport`: a wrapper around any `Transport` that itself implements `Transport`. | A layer on top of `posix` or `lwip`, not an alternative to them. Built only with `COAPCPP_WITH_DTLS`. mbedTLS is the primary backend, since ESP-IDF and OpenWrt ship it. Client and server roles, PSK, sessions keyed by address (R16, R17, R19). |
| `port/os/posix` | `Clock`: `clock_gettime(CLOCK_MONOTONIC)`. `Random`: `getrandom`. `Log`: spdlog. | On Linux the OS provides entropy, so `Random` is here rather than in `hal`. |
| `port/os/freertos` | `Clock`: `xTaskGetTickCount`. `heap.cc`: `operator new`/`delete` and cJSON hooks on `pvPortMalloc`. | Shared by STM32 and RP2040/RP2350. ESP-IDF uses only `clock.cc`: its `malloc` is already thread-safe. |
| `port/hal/stm32` | `Random`: `HAL_RNG_GenerateRandomNumber`. `Clock`: `HAL_GetTick` for builds without an OS. | One file for F4 and H7, since the HAL API is the same; the board includes the family header. |
| `port/hal/pico` | `Random`: `get_rand_32` from `pico_rand`. `Clock`: `time_us_64`. | One file for RP2040 and RP2350. |
| `port/hal/esp32` | `Random`: `esp_fill_random`. `Clock`: `esp_timer_get_time`. | One file for ESP32, ESP32-C3, and ESP32-S3. `Random` is a true RNG only while Wi-Fi or Bluetooth is on, which holds on a node. |

lwIP, FreeRTOS, and the HAL themselves are not in the repository: they come from the vendor SDK (STM32Cube, Pico SDK, ESP-IDF) or from PlatformIO. The ports are written against their public APIs.

## Boards

| Board (`boards/`) | `port/net` | `port/os` | `port/hal` | Toolchain |
|---|---|---|---|---|
| `posix` (Linux x86) | `posix` | `posix` | none | host |
| `posix`, preset `raspberry-pi` (Pi 4/5) | `posix` | `posix` | none | host or `aarch64-linux-gnu` |
| `posix`, preset `stm32mp157a-dk1` | `posix` | `posix` | none | OpenSTLinux SDK |
| `posix`, preset `openwrt` | `posix` | `posix` | none | OpenWrt SDK (musl) |
| `esp32-devkitc` (ESP32, ESP32-C3, ESP32-S3) | `lwip` (sockets) | `freertos` (ESP-IDF) | `esp32` | ESP-IDF or PlatformIO `espressif32` |
| `nucleo-f429zi` | `lwip` | `freertos` | `stm32` | `arm-none-eabi`, cortex-m4f |
| `nucleo-h743zi` | `lwip` | `freertos` | `stm32` | `arm-none-eabi`, cortex-m7 |
| `pico-w` (RP2040 / RP2350) | `lwip` | `freertos` or none | `pico` | Pico SDK |
| `pico-eth` (RP2350, Ethernet over RMII on PIO) | `lwip` | `freertos` or none | `pico` | Pico SDK |
| `pico-spi/firmware` (RP2040 / RP2350 without networking) | custom `Transport` over SPI | `freertos` | `pico` | Pico SDK |

`port/net/dtls/wolfssl` or `port/net/dtls/mbedtls` can be added to any row when needed.

Rules for the `boards/` directory:

- **`main()` is always in the board directory.** On a microcontroller there is no other way: `main` initializes the HAL and starts the scheduler. It creates the ports and calls `app_main` from `examples/`.
- **One board corresponds to one directory until the code diverges.** Pi 4 and Pi 5 differ in the GPIO chip number, which is a parameter. Pico W and Pico 2 W differ in the `PICO_BOARD` variable, which is a build preset.
- **`boards/posix` is the single board for all of Linux.** x86, Raspberry Pi 4/5, and STM32MP157A-DK1 are built from one directory: the same `main` with argument parsing and signals, the same ports. They differ in the toolchain and in the `SENSORS=stub|gpio` option.
- **Sensor drivers live in `boards/`**, because they are tied to the GPIO API. The simulators (`sensor_stubs.*`) need no platform and live in `examples/coap-server/`, so that any board without sensors can use them.
- **BSP-level differences do not reach `port/`.** The ETH driver, cache and MPU, and the placement of ETH descriptors in memory for F4 and H7 live only in `boards/`.
- **Files shared by the Pico boards.** `pico_sdk_import.cmake` and `FreeRTOS_Kernel_import.cmake` live in `cmake/` instead of being copied into every board. `FreeRTOSConfig.h` and `lwipopts.h` stay in the boards, because they are exactly what the boards differ in.

### Notes on individual boards

- **`posix`, sensors.** GPIO goes through libgpiod, so the code is the same for Raspberry Pi 4/5 and STM32MP1. Line numbers and the GPIO chip are passed as command-line arguments. DHT11 is better read through the kernel driver (`dtoverlay=dht11`, values from `/sys/bus/iio/`), because the kernel then meets the timing requirements of the protocol.
- **`pico-spi`.** A Pico without a network stack serves CoAP over SPI, while a Linux board acts as a UDP ↔ SPI gateway. A frame carries up to 251 bytes, so block-wise blocks cannot exceed 128 bytes. The frame has no address, so the gateway serves one client at a time. The gateway does not need the library.
- **`pico-eth`.** A MAC on PIO is a network interface driver (`netif`) for lwIP, the same level as `ethernetif.c` on NUCLEO-F429ZI. The board adds no new ports. If something in `port/` or `src/` has to change for it, that is a sign of a mistake in the split. RMII requires a 50 MHz reference, and the system clock will have to be chosen to match it.
- **`posix`, preset `openwrt`.** The same `boards/posix` built with the OpenWrt SDK against musl. mbedTLS comes from the system package through `find_package`, not from `third-party/`. This preset is the gateway's build check; the `libcoapcpp` package itself is built by `packaging/openwrt/Makefile`.
- **`esp32-devkitc`.** An ESP-IDF project; the library is connected as a component. lwIP and FreeRTOS come from ESP-IDF, so the board adds only `port/hal/esp32`. `sdkconfig.defaults` enables DTLS and PSK in mbedTLS (`CONFIG_MBEDTLS_SSL_PROTO_DTLS`, `CONFIG_MBEDTLS_PSK_MODES`). NVS, OTA slots, and Wi-Fi provisioning are the SmartNode SDK's business, not the library's.
- **The Cortex-M4 core in STM32MP1** is not considered. If it is ever needed, it is one more board with `port/hal/stm32` and a transport over OpenAMP.

## Examples

Each example is an `app_main` function that receives ready-made ports and the board resources:

```cpp
// examples/coap-server/app_main.h
struct AppPorts {
    coap::port::Transport &transport;
    coap::port::Clock     &clock;
    coap::port::Random    &random;
    coap::port::Log       &log;
};
void app_main(AppPorts &ports, sensors::EndpointPool &boardEndpoints);
```

Thanks to this, `examples/` builds unchanged for every board. A board registers its own resources (sensors, RTC, accelerometer), so the list of sensor types in the example cannot be a closed one.

## Build

Each layer is a separate CMake target. The platform is selected by the list of targets, which the board defines, so the user specifies only the board.

```cmake
cmake_minimum_required(VERSION 3.13)
project(coapcpp CXX C)

option(COAPCPP_WITH_FORMATS "SenML-JSON, CoRE Link"  ON)
option(COAPCPP_WITH_DTLS    "DTLS transport"          OFF)
option(COAPCPP_BUILD_TESTS  "Unit tests (host only)"  OFF)
set(COAPCPP_BOARD "" CACHE STRING "directory from boards/, for example nucleo-f429zi")

add_library(coapcpp STATIC src/packet.cc src/uri.cc ...)
target_include_directories(coapcpp PUBLIC api)
target_compile_features(coapcpp PUBLIC cxx_std_11)

if(COAPCPP_BOARD)
    add_subdirectory(boards/${COAPCPP_BOARD})   # pulls in the required port/* and examples/*
endif()
```

A board file, for example `boards/nucleo-f429zi/CMakeLists.txt`:

```cmake
add_subdirectory(${PROJECT_SOURCE_DIR}/port/net/lwip     port/net/lwip)
add_subdirectory(${PROJECT_SOURCE_DIR}/port/os/freertos  port/os/freertos)
add_subdirectory(${PROJECT_SOURCE_DIR}/port/hal/stm32    port/hal/stm32)
add_subdirectory(${PROJECT_SOURCE_DIR}/examples/coap-server examples/coap-server)

add_executable(coap-server main.cc bsp/...)
target_link_libraries(coap-server
    coap_server_app coapcpp
    coapcpp_net_lwip coapcpp_os_freertos coapcpp_hal_stm32)
```

Rules:

- The library does not set optimization flags, `-frtti`, or `-L`, and does not touch `NDEBUG`. That is the job of the toolchain file or the user's project.
- `third-party/` is pulled in only when needed: cJSON with `COAPCPP_WITH_FORMATS`, wolfSSL or mbedTLS with `COAPCPP_WITH_DTLS`, spdlog with `port/os/posix`, GoogleTest with `COAPCPP_BUILD_TESTS`.
- `port/net/lwip`, `port/os/freertos`, and `port/hal/*` are declared as `INTERFACE` libraries with sources. They need `lwipopts.h`, `FreeRTOSConfig.h`, and the HAL headers from the board directory, so they must be compiled in its context.
- The path to the vendor SDK is set with a CMake variable; the SDK itself lives outside the repository.
- Build presets are described in `CMakePresets.json`, and `build.sh` is reduced to `cmake --preset <name>`.
- For consumers (R22): `install` with CMake package export and a `.pc` file for pkg-config; `BUILD_SHARED_LIBS` is respected; mbedTLS, cJSON, and spdlog are found with `find_package` first and taken from `third-party/` only as a fallback, so the OpenWrt build needs no submodule.
- Under ESP-IDF (the CMake variable `ESP_PLATFORM`, not the preprocessor in sources) the root `CMakeLists.txt` registers the library through `idf_component_register`; the manifest is `idf_component.yml`.

| Preset | Board directory | What differs |
|---|---|---|
| `posix` | `boards/posix` | host toolchain, `SENSORS=stub` |
| `raspberry-pi` | `boards/posix` | `SENSORS=gpio`, optionally the `aarch64-linux-gnu` toolchain |
| `stm32mp157a-dk1` | `boards/posix` | OpenSTLinux SDK toolchain, `SENSORS=gpio` |
| `nucleo-f429zi` | `boards/nucleo-f429zi` | cortex-m4f |
| `nucleo-h743zi` | `boards/nucleo-h743zi` | cortex-m7 |
| `pico-w`, `pico2-w` | `boards/pico-w` | `PICO_BOARD` |
| `pico-eth` | `boards/pico-eth` | RP2350 |
| `pico-spi` | `boards/pico-spi` | firmware and gateway |
| `esp32` | `boards/esp32-devkitc` | ESP-IDF, `IDF_TARGET=esp32\|esp32c3\|esp32s3` |
| `openwrt` | `boards/posix` | OpenWrt SDK toolchain, musl, system mbedTLS |

**PlatformIO.** `library.json` lives in the root of `main` and uses `srcFilter` to take `src/`, `port/net/lwip`, `port/os/freertos`, and the `port/hal/*` of the platform; the platform is chosen by `extraScript`. Supported platforms are `espressif32` (the `espidf` and `arduino` frameworks), `raspberrypi`, and `ststm32`. The separate `platformio` branch becomes unnecessary.

## Testing

- Unit tests in `test/` are built only on the host and only with `COAPCPP_BUILD_TESTS`.
- The core is tested with fake ports from `test/mocks/`: `fake_clock.h`, `fake_random.h`, `loopback_transport.h`.
- Each port has its own test next to the code. For example, `port/net/posix/test/` sends a datagram through `Transport` to `::1` and receives it.
- Cross-compilation of the core for `arm-none-eabi` is checked in CI without any port; `esp32-devkitc` and the `openwrt` preset are built in CI as well.
- `loopback_transport.h` can drop, duplicate, and reorder datagrams. CoAP Smart Home scenarios run on it with a fake clock: registration in `/rd`, Observe with notification loss, control with a separate response, a Block2 download with resume after a break.
- Interoperability with libcoap, aiocoap, and Californium, over plain UDP and DTLS-PSK, is checked in CI (R25).
- The size of the stripped library and the RSS of the example are measured in CI and compared with the budget (R23).

The sign that the architecture works: `examples/coap-server/` builds unchanged for all boards, and each board directory contains only hardware initialization, port creation, and registration of its own resources.

## Decisions made

- **Virtual interfaces instead of templates or link-time selection.** The cost is one indirect call per packet, which is negligible for CoAP. In return, the core is built once and tested on the host with substitute ports.
- **The engine does not know the transport.** This allows embedding it in a custom event loop and testing it without a network.
- **`port/` is separate from `src/`.** The `src/` directory can be added as a whole to any build system (STM32CubeIDE, PlatformIO, Pico SDK) without filters.
- **The old network layer is not restored.** The `Socket`/`Connection` abstraction removed in commit `60f749e` mirrored BSD sockets rather than the needs of CoAP, and it cannot be implemented for an SPI link or for an RP2040 without an OS. Its contents come back, distributed across `port/` behind the `Transport` interface.
- **New port error codes** are declared in the ports themselves through their own `std::error_category` instead of being added to `CoapStatus`.
- **The library does not know about CoAP Smart Home.** It takes from the system only generic IETF mechanisms (R1–R26). The RD server, the device profile, and the OTA logic stay in `coap-manager` and the SmartNode SDK, so the library remains useful outside the system and devices without the SDK stay compatible.
- **Streaming block-wise instead of assembling in memory.** A firmware image is about 1 MB: the gateway reads it from a file block by block, and the node writes each block into a flash slot. Assembly in a buffer remains only for small payloads, with a limit (R10–R12).
- **One DTLS association for both directions.** The node opens the session as a client, and the gateway sends its requests through the same session. A DTLS server on the node would cost RAM and a second key exchange (R19).
