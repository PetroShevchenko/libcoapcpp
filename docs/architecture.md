# libcoapcpp Architecture

Status as of 2026-10-06.

This document describes the target structure of the project: what it should become after the rework, not what it is today. The sequence of steps leading to it is given in the [roadmap](Roadmap.md).

## Goal

libcoapcpp is being developed as a C++ CoAP library for microcontrollers and POSIX systems. The core performs no I/O (the sans-IO approach), and everything platform-specific is plugged in through thin adapters.

Target platforms:

- POSIX: Linux on x86, Raspberry Pi 4/5, STM32MP1 (Cortex-A, OpenSTLinux);
- STM32F4 and STM32H7;
- RP2040 and RP2350 (Raspberry Pi Pico, Pico W, Pico 2, Pico 2 W).

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
grep -rE '#\s*if.*(__linux__|__unix__|__arm__|STM32|PICO_|LWIP)' api src examples
```

## Directory structure

```
libcoapcpp/
├── api/coap/                    public headers; included as <coap/packet.h>
│   ├── consts.h  error.h  buffer.h  packet.h  uri.h  blockwise.h  utils.h
│   ├── core_link.h  senml_json.h  data_type.h        formats
│   ├── server.h  client.h                            engine
│   ├── config.h                                      sizes and limits, overridable
│   └── port/                    <coap/port/transport.h>
│       └── transport.h  clock.h  random.h  log.h  resolver.h     pure interfaces
│
├── src/                         no system headers, no platform #ifdef
│   ├── packet.cc  uri.cc  blockwise.cc  error.cc  utils.cc
│   ├── core_link.cc  senml_json.cc  base64.cc  base64.h
│   └── server.cc  client.cc  packet_helper.cc
│
├── port/                        interface implementations; each directory is a separate CMake target
│   ├── net/                     Transport, Resolver
│   │   ├── posix/     udp_transport.cc  resolver.cc
│   │   ├── lwip/      udp_transport_sockets.cc  udp_transport_raw.cc  resolver.cc
│   │   └── dtls/
│   │       ├── wolfssl/   dtls_transport.cc  wolfssl_error.cc  wolfssl_error.h
│   │       └── mbedtls/   dtls_transport.cc
│   ├── os/                      Clock, Log, heap
│   │   ├── posix/     clock.cc  random.cc  spdlog_log.cc
│   │   └── freertos/  clock.cc  heap.cc
│   └── hal/                     Random, Clock without an OS
│       ├── stm32/     random.cc  clock.cc
│       └── pico/      random.cc  clock.cc
│
├── examples/                    example logic; no main() and no platform code
│   ├── coap-server/   app_main.cc  sensor.*  endpoint.*  sensor_stubs.*
│   └── coap-client/   app_main.cc
│
├── boards/                      main(), BSP, port creation, board resources
│   ├── posix/                main_server.cc  main_client.cc  (getopt, signals)
│   │   ├── gpio/             dht11.cc  rgb_led.cc  (libgpiod)
│   │   └── docs/             instructions for Raspberry Pi and STM32MP157A-DK1
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
├── docs/
├── library.json                 PlatformIO, with srcFilter on src/ and the required port/
└── third-party/
```

## Core

The core consists of `api/coap/` and `src/`.

- **Codec and formats.** Packets (RFC 7252), block-wise transfers (RFC 7959), URI, SenML-JSON (RFC 8428), CoRE Link Format (RFC 6690).
- **Engine.** `server` and `client` work through `processing(Buffer &)`: bytes in, bytes out. The engine knows nothing about the transport.
- **Processing loop.** `coap::run(server, transport, clock)` reads datagrams from the transport and passes them to `processing`. Anyone with their own event loop calls `processing` directly.
- **Threads.** The core is single-threaded: one instance serves one thread or one task. There are no mutexes among the ports.
- **Byte order.** Multi-byte fields are written with shifts in the core itself; this is not a port, and `htons` is not needed.
- **Dependencies.** The C++ standard library and cJSON. cJSON is needed only by the formats and is enabled with the `COAPCPP_WITH_FORMATS` option.
- **Configuration.** Buffer sizes and limits are collected in `api/coap/config.h` and can be overridden by the user's project.

Limitation: the core uses dynamic memory (`std::string`, `std::vector`, `new[]`). With `-fno-exceptions`, a failed allocation terminates the program. On FreeRTOS, the port redirects `operator new` and `malloc` to the RTOS heap, and cJSON is configured through `cJSON_InitHooks`. Removing the heap is separate work and is not covered by this architecture.

## Port interfaces

Four mandatory interfaces and one optional, all compatible with C++11. The core receives them through its constructor, so fake implementations can be substituted in tests.

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

}}
```

Purpose of the interfaces:

| Interface | Why the core needs it |
|---|---|
| `Transport` | Needed only by the `coap::run` loop; the engine works with buffers. |
| `Clock` | Client and server timeouts, and later CON retransmissions. |
| `Random` | Tokens and Message IDs. Per RFC 7252 §5.3.1, a token must be unpredictable. |
| `Log` | Diagnostics without a dependency on spdlog. |
| `Resolver` | Turning a host name into an `Address` in the client. |

## Ports

A port is an implementation of the interfaces for a particular software API, not for a piece of hardware. Raspberry Pi 4, Pi 5, STM32MP1, and x86 are identical for the library: the same sockets, `clock_gettime`, `getrandom`. STM32F4, STM32H7, and RP2040 share a network stack (lwIP) and differ only in their entropy source and clock.

Axes:

- `port/net`: network stack;
- `port/os`: operating system;
- `port/hal`: chip. This axis is needed because the random number generator and the clock without an OS belong neither to the network nor to the OS, and in `boards/` they would be duplicated between boards of the same family.

A board takes one directory from each axis it needs.

| Directory | What it implements | Notes |
|---|---|---|
| `port/net/posix` | `Transport`: `socket`, `sendto`, `recvfrom`, `poll`. `Resolver`: `getaddrinfo`. | Shared by all Linux targets. |
| `port/net/lwip` | `Transport` in two variants: on lwIP sockets (`udp_transport_sockets.cc`, requires an OS) and on the raw `udp_*` API (`udp_transport_raw.cc`). `Resolver`: `lwip_getaddrinfo`. | The board picks one of the two files; the core is unaware of it. The raw variant is needed only for builds without an OS (`NO_SYS=1`). |
| `port/net/dtls/wolfssl`, `port/net/dtls/mbedtls` | `DtlsTransport`: a wrapper around any `Transport` that itself implements `Transport`. | A layer on top of `posix` or `lwip`, not an alternative to them. Built only with `COAPCPP_WITH_DTLS`. |
| `port/os/posix` | `Clock`: `clock_gettime(CLOCK_MONOTONIC)`. `Random`: `getrandom`. `Log`: spdlog. | On Linux the OS provides entropy, so `Random` is here rather than in `hal`. |
| `port/os/freertos` | `Clock`: `xTaskGetTickCount`. `heap.cc`: `operator new`/`delete` and cJSON hooks on `pvPortMalloc`. | Shared by STM32 and RP2040/RP2350. |
| `port/hal/stm32` | `Random`: `HAL_RNG_GenerateRandomNumber`. `Clock`: `HAL_GetTick` for builds without an OS. | One file for F4 and H7, since the HAL API is the same; the board includes the family header. |
| `port/hal/pico` | `Random`: `get_rand_32` from `pico_rand`. `Clock`: `time_us_64`. | One file for RP2040 and RP2350. |

lwIP, FreeRTOS, and the HAL themselves are not in the repository: they come from the vendor SDK (STM32Cube, Pico SDK) or from PlatformIO. The ports are written against their public APIs.

## Boards

| Board (`boards/`) | `port/net` | `port/os` | `port/hal` | Toolchain |
|---|---|---|---|---|
| `posix` (Linux x86) | `posix` | `posix` | none | host |
| `posix`, preset `raspberry-pi` (Pi 4/5) | `posix` | `posix` | none | host or `aarch64-linux-gnu` |
| `posix`, preset `stm32mp157a-dk1` | `posix` | `posix` | none | OpenSTLinux SDK |
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

**PlatformIO.** `library.json` lives in the root of `main` and uses `srcFilter` to take `src/`, `port/net/lwip`, `port/os/freertos`, and `port/hal/stm32`. The separate `platformio` branch becomes unnecessary.

## Testing

- Unit tests in `test/` are built only on the host and only with `COAPCPP_BUILD_TESTS`.
- The core is tested with fake ports from `test/mocks/`: `fake_clock.h`, `fake_random.h`, `loopback_transport.h`.
- Each port has its own test next to the code. For example, `port/net/posix/test/` sends a datagram through `Transport` to `::1` and receives it.
- Cross-compilation of the core for `arm-none-eabi` is checked in CI without any port.

The sign that the architecture works: `examples/coap-server/` builds unchanged for all boards, and each board directory contains only hardware initialization, port creation, and registration of its own resources.

## Decisions made

- **Virtual interfaces instead of templates or link-time selection.** The cost is one indirect call per packet, which is negligible for CoAP. In return, the core is built once and tested on the host with substitute ports.
- **The engine does not know the transport.** This allows embedding it in a custom event loop and testing it without a network.
- **`port/` is separate from `src/`.** The `src/` directory can be added as a whole to any build system (STM32CubeIDE, PlatformIO, Pico SDK) without filters.
- **The old network layer is not restored.** The `Socket`/`Connection` abstraction removed in commit `60f749e` mirrored BSD sockets rather than the needs of CoAP, and it cannot be implemented for an SPI link or for an RP2040 without an OS. Its contents come back, distributed across `port/` behind the `Transport` interface.
- **New port error codes** are declared in the ports themselves through their own `std::error_category` instead of being added to `CoapStatus`.
