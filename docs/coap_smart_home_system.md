# CoAP Smart Home — System Architecture and Concept

> **Status:** concept / initial specification (v0.2)  
> **Date:** October 17, 2026  
> **Foundation:** `libcoapcpp`, OpenWrt, PlatformIO  
> **Purpose of the document:** to agree on the components, their scopes of responsibility, and the order in which an open IoT platform is built.  
> **Changes from v0.1:** incorporated the feedback from `coap_smart_home_review.md` — node self-registration instead of multicast discovery, a device profile based on standards (CoRE Link Format, SenML), stock firmware configured at run time, provisioning over Web Serial, pull-OTA, local builds on a PC instead of a Build Server, a defined concurrency model for the gateway, a refined security model and roadmap.

## 1. Overview

**CoAP Smart Home** is a modular, locally managed, open-source platform for monitoring sensors, controlling actuators, and managing the firmware life cycle. An OpenWrt router can serve as the main gateway, but the architecture stays portable to ordinary Linux (Ubuntu, Debian, Raspberry Pi OS).

Typical scenario: a user buys an ESP32 or a Raspberry Pi Pico W, connects a supported sensor, opens the "Add device" page in the browser, writes the stock firmware over USB straight from the browser, and enters the Wi-Fi settings and sensor pins. The device registers itself with the gateway and appears on the dashboard with its readings and controls. Nothing has to be compiled. Stock firmware updates are installed over OTA; developers build their own firmware locally on their PC.

### 1.1 Objectives

- Sensors and device control work **without the internet and without any cloud dependency**.
- Simple gateway installation on OpenWrt; Linux as an alternative.
- **No compilation for typical scenarios**: one stock firmware image per board, with sensors and pins configured at run time.
- Minimal programming for developers: the high-level **SmartNode SDK** layer on top of `libcoapcpp`.
- An open CoAP interface based on IETF standards; compatible devices can be built without the SmartNode SDK.
- Devices appear automatically through self-registration; CoAP Observe is supported.
- A single web interface for readings, control, configuration, initial flashing, and OTA.
- Custom firmware is compiled **locally on the user's PC** (PlatformIO or a Docker builder), with no toolchain on the router and no separate build server.

### 1.2 Out of Scope for the First Version

Not planned right away:

- replacing Home Assistant; implementing Zigbee, Matter, video surveillance, a mobile app, complex automation scenarios;
- a large historical database of readings on the gateway;
- a public or self-hosted Build Server;
- a LuCI application;
- full compatibility with OMA LwM2M (the object model and update logic are borrowed, but the protocol is not fully implemented);
- OTA for Pico W and support for STM32 — after ESP32 stabilizes;
- dozens of platforms and sensors.

### 1.3 Key Architectural Decisions

| Decision | Essence | Section |
|---|---|---|
| Node self-registration | The node registers itself with the gateway (CoRE Resource Directory); multicast is only a fallback | 5.2, 6 |
| Standard device profile | `rt`/`if`/`obs` in link-format + SenML instead of a custom format | 5.1 |
| Stock firmware per board | Drivers are built in, configuration happens at run time | 3.2 |
| Provisioning over Web Serial | Firmware, Wi-Fi, and the key are transferred from the browser over USB | 4.1 |
| Pull-OTA | The gateway notifies, the node downloads the image itself (Block2) | 6 |
| Local build | Docker builder on a PC + firmware repository on the gateway; no Build Server | 3.6 |
| One CoAP thread in the gateway | HTTP does not perform CoAP directly; command queue + state snapshot | 3.3 |
| Gateway without internet access | Images come from outside (browser or CLI) | 3.6 |

## 2. General Architecture

```text
 ┌──────────────────────────────┐         ┌──────────────────────────────────┐
 │ Web Browser                  │         │ Developer PC (optional)          │
 │ Dashboard / Devices /        │         │ smartnode build / sign / upload  │
 │ Add device / Firmware        │         │ (PlatformIO or Docker builder)   │
 └───────┬───────────────┬──────┘         └────────────────┬─────────────────┘
         │ HTTP(S)+SSE   │ Web Serial (USB)                │ HTTP POST image+manifest
         │               ▼                                 │
         │      board on USB: initial flashing,            │
         │      Wi-Fi, gateway address, key                │
         ▼                                                 ▼
 +-----------------------------------------------------------------------+
 |                   OpenWrt Router or Linux Host                        |
 |                                                                       |
 |  +------------------------------------------------------------------+ |
 |  | coap-manager (single process)                                    | |
 |  |  CivetWeb: REST API + SSE + static Web UI                        | |
 |  |      │ command queue           ▲ state snapshot                  | |
 |  |      ▼                         │                                  | |
 |  |  CoAP event loop (libcoapcpp, 1 thread)                          | |
 |  |   RD /rd | Registry | Observe | Control | OTA | Firmware Repo /fw| |
 |  |  State Store (RAM) + config/registry (Flash, rare writes)        | |
 |  +---------------------------------+--------------------------------+ |
 +------------------------------------|----------------------------------+
                                      | CoAP / UDP + DTLS-PSK
         registration, heartbeat ▲    |  ▼ GET/PUT, Observe, /system/update
         GET /fw (Block2)        |    |
                        +----------+--+---------+
                        |          |            |
                  +-----v----+ +---v------+ +---v--------+
                  | ESP32    | | Pico W   | | STM32*     |
                  | stock    | | stock    | | custom     |
                  | firmware | | firmware | | build      |
                  +----------+ +----------+ +------------+
                                              * after 1.0, needs a network module

 Image sources:  official releases (CI → GitHub/GitLab Releases) → browser ─┐
                 local build on a PC (CLI / Docker) ────────────────────────┴─► POST /api/v1/firmware
```

**Roles:**

- **Browser** — HTTP client of the gateway; also performs initial flashing and provisioning over Web Serial.
- **Gateway** — HTTP server (API, SSE, UI); CoAP server (Resource Directory `/rd`, image delivery `/fw`); CoAP client (requests to nodes, Observe).
- **Nodes** — CoAP servers for sensor and actuator resources, and CoAP clients for registration and firmware downloads.
- **Developer PC** — an optional environment for building custom firmware; has no direct connection to the nodes.

## 3. Components

### 3.1 libcoapcpp

A standalone C++ library implementing CoAP. It stays independent of Smart Home, OpenWrt, and HTTP. It is used both in the gateway and in device firmware.

Required capabilities:

| Capability | Gateway | Node |
|---|---|---|
| Non-blocking client, transactions, timeouts, retransmissions | yes | yes (registration, OTA) |
| Resource server, CoRE Link Format (RFC 6690) | yes (`/rd`, `/fw`) | yes |
| Observe (RFC 7641) | client | server |
| Block2 (RFC 7959) | server (image delivery) | client (image download) |
| DTLS 1.2 with PSK | yes | yes |
| OSCORE (RFC 8613) | after 1.0 | after 1.0 |

### 3.2 SmartNode SDK and Stock Firmware

A layer on top of `libcoapcpp`, integrated with PlatformIO. It hides tokens, CoAP codes, registration, link-format, SenML, OTA, and the networking logic. The SDK is also the basis of the **stock firmware**, so it is tested by a real product.

**What "universal firmware" means.** A single binary for all MCUs is impossible: ESP32 (Xtensa/RISC-V), RP2040, and STM32 (ARM Cortex-M) have different instruction sets, bootloaders, and Flash layouts. The goal is not to multiply builds by sensor combinations:

| Approach | Number of images |
|---|---|
| Build per configuration | boards × sensor combinations × pins — practically infinite |
| Configuration at run time | number of target boards — roughly 10–20 per release |

**Three layers of a node:**

1. **Platform layer (SDK HAL)** — I2C, SPI, GPIO, ADC, OneWire, UART, Wi-Fi, configuration storage, OTA backend. Compiled for each board.
2. **Sensor drivers** — written only against the HAL and independent of the MCU. The stock image contains the MVP drivers: BME280, DHT22, DS18B20, relay, button.
3. **Configuration** — stored in NVS/Flash, set during provisioning or through `PUT /system/config`; resources are created dynamically:

```json
{
  "buses": [{"id": "i2c0", "type": "i2c", "sda": 21, "scl": 22, "freq": 400000}],
  "devices": [
    {"driver": "bme280", "bus": "i2c0", "addr": "0x76", "name": "living_room"},
    {"driver": "relay",  "pin": 5, "active_low": true, "name": "heater"}
  ]
}
```

**The board descriptor** ships with the image: available pins, their functions (I2C/SPI/ADC/PWM), and limitations. The gateway and the "Add device" page validate the configuration before it is written to the device.

**API for custom firmware developers** (conceptual):

```cpp
#include <SmartNode.h>

SmartNode node;

void setup() {
  node.begin();   // configuration, Wi-Fi, keys, stock drivers, registration with the gateway
  node.addSensor("soil", "moisture", "%", [] { return read_soil(); });
  node.addSwitch("pump", [](bool on) { set_pump(on); });
}

void loop() { node.loop(); }
```

**When compilation is needed:** STM32 with small Flash (64–128 KB) and varied clocking/pinmux; the user's own logic; rare drivers that are not in the stock image. For these cases there is the local build (3.6).

### 3.3 CoAP Device Manager / Gateway

A single background C++ process, `coap-manager`, which:

- accepts node registrations (Resource Directory, RFC 9176) and keeps a registry by the stable identifier `ep`, not by IP;
- determines online/offline status from the registration lifetime;
- performs GET/PUT/POST/DELETE according to the capabilities of the nodes;
- maintains one Observe subscription per resource, a cache of the latest values, re-subscription after failures, and marking of stale values;
- broadcasts state changes to the Web UI over SSE;
- keeps a local firmware repository, verifies signatures and compatibility, and manages pull-OTA;
- keeps an event log and an audit trail of control operations;
- **does not need internet access**.

#### 3.3.1 Concurrency Model

```text
 CivetWeb HTTP threads ──(command in queue)──► CoAP event loop (1 thread)
        ▲                                         │
        │                                         ▼
        └──── snapshot read ◄──── State Store (mutex + copy / RCU)
```

- One thread owns `libcoapcpp`, the UDP sockets, timers, Observe subscriptions, and the registry.
- HTTP handlers **never perform CoAP themselves**: reads return a snapshot of the cache; control requests put a command in the queue and either wait for the result with a timeout or return `202 Accepted` with an operation ID.
- CoAP timeouts do not block HTTP.

#### 3.3.2 Storage

- Current state and a short ring buffer of readings — **in RAM only**.
- Flash holds only the gateway configuration, the device registry, and keys; writes are atomic and rare (on change, not on a timer).
- Firmware images — a limited number of recent versions per board; a USB drive if needed.
- Long-term history is outside the MVP; later through export (MQTT, InfluxDB, Prometheus).

### 3.4 Web UI

A separate responsive interface in HTML/CSS/JavaScript, available, for example, at `https://router.local:8443/` (the address and port are configurable).

| Page | Functionality |
|---|---|
| Dashboard | Room/sensor cards, readings, states, switches; updates over SSE |
| Devices | List of devices (`ep`, board, version, IP, online/offline), sensor configuration |
| Add device | Initial flashing over Web Serial, Wi-Fi and sensor setup, key issuance |
| Firmware | Image repository, importing official releases, uploading local builds, OTA and its progress |
| Resource Explorer | Arbitrary CoAP requests, only for the "developer" role (from stage 0.2) |
| Settings | Gateway configuration, users and roles, keys, security |

State updates arrive immediately through **Server-Sent Events** (`EventSource` in the browser), without polling. The Web UI never initiates a CoAP request on each page refresh.

### 3.5 HTTP Server

**Decision:** **CivetWeb** is embedded in `coap-manager` — portable between Linux and OpenWrt, supports long-lived connections for SSE and HTTPS.

Rejected alternatives:

- **uHTTPd + CGI** — a process per request does not suit SSE and shared state; uHTTPd may only serve static files, if packaging decides so.
- **LuCI** — is not the system's UI; an additional package is possible after 1.0.
- **simple-http-server** — is developed as a separate project and is not a platform dependency.

### 3.6 Firmware: Official Releases, Local Builds, the Gateway Repository

There is no separate Build Server. There are three sources of images and one way to deliver them.

```text
 User PC                                      Gateway
 ┌───────────────────────────────┐            ┌───────────────────────────┐
 │ smartnode build               │            │ Local firmware            │
 │  (PlatformIO or Docker image  │  HTTP POST │ repository:               │
 │   with a pinned toolchain)    ├───────────►│  image + manifest + sig.  │
 │ smartnode sign (key on PC)    │            │  verification → OTA       │
 └───────────────────────────────┘            └───────────────────────────┘
                                                   ▲
 Official releases (CI → GitHub/GitLab Releases) ──┘ (imported via browser)
```

| Scenario | Source |
|---|---|
| Mass-market boards (ESP32, Pico W) | Official stock images from CI; sensors are configured without compilation |
| STM32, custom code, non-standard drivers | Local build (`smartnode build` / Docker) → repository on the gateway |
| Build on another machine (home server, CI) | The same Docker builder; no separate Build Server API is needed |

**Principles:**

- **Reproducibility.** An official Docker builder image with pinned versions of PlatformIO and the toolchain. CI uses the same image, so local and official builds are identical.
- **Developer CLI:** `smartnode keygen`, `smartnode build`, `smartnode sign`, `smartnode upload --gateway router.local`.
- **Signing with two keys.** The node trusts the project key (official releases) and the user key (generated on the PC; its public part is written to the node during initial flashing). The user's private key **stays on the PC**, not on the gateway.
- **The gateway does not go to the internet.** Images come from outside: from the browser (importing a release) or from the CLI. The gateway only verifies the signature and compatibility.
- **PlatformIO and the toolchain are not installed on the router.**

## 4. Key Scenarios

### 4.1 Connecting a Device for the First Time

1. The user opens the "Add device" page and connects the board to the PC over USB.
2. Over **Web Serial**, the browser writes the stock firmware for the selected board (ESP32 — via `esptool-js`; Pico W — UF2 "drag-and-drop", then configuration over USB CDC).
3. The user selects sensors and pins; the page validates the configuration against the board descriptor.
4. The same channel carries: the Wi-Fi SSID/password, the gateway address, **the node key (PSK) generated by the gateway**, the public keys for firmware verification, and the sensor configuration.
5. The node connects to Wi-Fi and **registers itself** with the gateway with the identifier `ep` (from the MAC/chip ID).
6. The gateway already knows the key and the `ep` — the node is **trusted from the moment it appears** and is shown on the dashboard.

Limitations: Web Serial works in Chromium browsers and requires a secure context (HTTPS or `localhost`). The fallback is an access point (SoftAP) with a captive portal on the node itself; adding an address manually is always possible.

### 4.2 Reading a Sensor and Control

```text
Node    --POST /rd?ep=esp32-a1b2c3&lt=300 (link-format)-->  Gateway   registration
Gateway --GET /sensors/temperature, Observe: 0---------->  Node      subscription
Gateway <--2.05 Content (SenML), then notifications------  Node
Browser <--SSE: event: resource, data: {...}-------------  Gateway   push to UI

Browser --PUT /api/v1/devices/{id}/resources/heater----->  Gateway
Gateway --PUT /actuators/heater------------------------->  Node
Gateway <--2.04 Changed----------------------------------  Node
Browser <--HTTP 200 + JSON-------------------------------  Gateway
```

Ordinary HTTP GET requests return values from the gateway's cache; a CoAP request is not made for each HTTP request.

### 4.3 Firmware Update (Pull-OTA)

```text
Browser/CLI -> Gateway: POST /api/v1/firmware (image + manifest)
Gateway: checks the signature, board/bootloader, sequence > current
Web UI: "Update available"
User -> Gateway: approve OTA
Gateway --PUT /system/update {uri, size, sha256, version}-------> Node
Node    --GET coap://gateway/fw/esp32/1.2.0.bin (Block2)--------> Gateway  (repeated)
Node: checks SHA-256 and the signature, writes to the inactive slot, reboots
Node    --POST /rd/4521 (re-registration with the new version)--> Gateway  = confirmation
Gateway -> UI (SSE): success / error / rollback
```

The node controls the download pace and can continue from the needed block after a break. The node needs only a CoAP client with Block2.

Not every change needs new firmware: sensors, pins, the name, and intervals are changed through `PUT /system/config`.

### 4.4 Custom Developer Firmware

1. `smartnode keygen` — once; the public key is written to the nodes during provisioning.
2. `smartnode build --board pico_w` — a build in the Docker builder or in a local PlatformIO.
3. `smartnode sign` → `smartnode upload --gateway router.local`.
4. Then the standard pull-OTA (4.3) or initial flashing over Web Serial (4.1).

## 5. Protocols and APIs

### 5.1 CoAP Device Profile

The profile is made of standard parts; our own specification comes down to a vocabulary of `rt` values and the structure of the system resources.

| Need | Solution |
|---|---|
| Resource type, interface | `rt` and `if` attributes in CoRE Link Format (RFC 6690); `if="core.s"` — sensor, `if="core.a"` — actuator |
| Observe support | `obs` attribute |
| Reading format and units | **SenML** (RFC 8428): `n`, `v`, `vb`, `u`, `t`; registered units (`Cel`, `%RH`, `V`…) |
| JSON or CBOR | both through the SenML Content-Format; JSON by default, CBOR as an option |
| Type vocabulary | `rt` values modeled on IPSO Smart Objects (Temperature 3303, Humidity 3304, Light Control 3311…) |

Example of `/.well-known/core` (and of the registration payload):

```text
</sensors/temperature>;rt="temperature";if="core.s";obs;ct=110,
</sensors/humidity>;rt="humidity";if="core.s";obs;ct=110,
</actuators/relay>;rt="switch";if="core.a";ct=110,
</system/info>;rt="smarthome.info";ct=50,
</system/config>;rt="smarthome.config";ct=50,
</system/update>;rt="smarthome.update";ct=50
```

Resource value:

```json
[{"n":"temperature","u":"Cel","v":22.4}]
```

System resources:

```text
GET  /.well-known/core
GET  /system/info      identity, board, version, keys, capabilities
GET  /system/config    current sensor configuration
PUT  /system/config    new configuration (validated against the board descriptor)
PUT  /system/update    start pull-OTA
```

Example of `/system/info`:

```json
{
  "ep": "esp32-a1b2c3",
  "board": "esp32dev",
  "target": "esp32",
  "firmware_version": "1.2.0",
  "sequence": 12,
  "bootloader": "1.0",
  "ota_supported": true,
  "trusted_keys": ["project-2026", "user-7a3f"]
}
```

### 5.2 Node Registration (Resource Directory, RFC 9176)

```text
Node  --POST /rd?ep=esp32-a1b2c3&lt=300 (payload: link-format)-->  Gateway
Node  <--2.01 Created, Location: /rd/4521--------------------------  Gateway
Node  --POST /rd/4521 (every lt seconds, as a heartbeat)---------->  Gateway
```

- `ep` — a stable node identifier; the IP address is not an identifier.
- A registration without a valid DTLS-PSK is rejected.
- No update within `lt` → the node is offline.
- Re-registration after OTA confirms the new version.

### 5.3 Gateway HTTP API

**Product API** — logical resources with validation and ACLs:

```text
GET    /api/v1/devices
GET    /api/v1/devices/{id}
GET    /api/v1/devices/{id}/resources
GET    /api/v1/devices/{id}/resources/{name}
PUT    /api/v1/devices/{id}/resources/{name}
GET    /api/v1/devices/{id}/config
PUT    /api/v1/devices/{id}/config
POST   /api/v1/devices/provision          issue ep + PSK for a new node
DELETE /api/v1/devices/{id}
GET    /api/v1/events                     SSE stream of state changes
```

**Firmware repository and OTA:**

```text
POST   /api/v1/firmware                   image + manifest (multipart)
GET    /api/v1/firmware?board=...         list of images
GET    /api/v1/firmware/{id}              details / manifest
DELETE /api/v1/firmware/{id}
POST   /api/v1/devices/{id}/updates       start OTA
GET    /api/v1/devices/{id}/updates/{update_id}
```

**Developer API** ("developer" role) — an HTTP-to-CoAP proxy for the Resource Explorer, with mapping per RFC 8075:

```text
ANY    /api/v1/dev/coap/{id}/{coap-path}
```

Requirements: authentication, roles, payload size limits, auditing of control operations, asynchronous processing (3.3.1).

### 5.4 Firmware Manifest

A single format for any source (CI, local build, manual upload), modeled on IETF SUIT (RFC 9019, RFC 9124). For the MVP — JSON signed with Ed25519:

```json
{
  "target": "esp32",
  "board": "esp32dev",
  "version": "1.2.0",
  "sequence": 12,
  "size": 1048576,
  "sha256": "9f2c…",
  "min_bootloader": "1.0",
  "key_id": "user-7a3f",
  "signature": "base64…"
}
```

- `target`/`board` — compatibility check (the analogue of vendor/class ID in SUIT);
- `sequence` — a monotonic number that protects against downgrade;
- `key_id` — which of the trusted keys signed the image.

## 6. Registration, Observe, OTA

**Device discovery:** the main mechanism is self-registration (5.2); the gateway address is set during provisioning. CoAP multicast and mDNS are an optional fallback (they may not pass through VLANs, Wi-Fi isolation, and routed networks). Adding an address manually is always available.

**Resources:** the list arrives in the registration payload; `/.well-known/core` is used for re-synchronization.

**Observe:** one gateway subscription per resource, a state cache, re-subscription after failures or a new registration, marking of stale values. Observe does not guarantee delivery of every intermediate reading.

**OTA:** a pull model over Block2 (4.3), signed images, compatibility and integrity checks, A/B slots, confirmation through re-registration, rollback where the platform supports it. Initial programming is not part of OTA — Web Serial (4.1) is for that.

## 7. Security

| Stage | CoAP transport | Rationale |
|---|---|---|
| MVP | **DTLS 1.2 with PSK** per node; the key is issued during provisioning | Simple, supported by mbedTLS on ESP32/Pico; no certificate infrastructure |
| After 1.0 | **OSCORE** (RFC 8613) + **EDHOC** (RFC 9528) | Message-level protection, works through proxies, lighter for constrained nodes |

- **Trust model:** a node without a valid key cannot register — untrusted nodes are filtered out at the entrance and do not control actuator resources.
- **Firmware:** the Ed25519 signature is independent of the transport; two trusted keys (project and user); the user's private key is only on the PC; downgrade protection through `sequence`; an update log.
- **Web UI and HTTP API:** authentication, roles (administrator, developer, user), and ACLs; LAN-only access by default; no root access to LuCI is needed.
- **HTTPS:** a self-signed certificate scares users, but Web Serial needs a secure context. Options: HTTPS on the gateway with the ability to upload one's own certificate, or the initial flashing page on the project's HTTPS site. The final choice is among the open issues.
- **Gateway on OpenWrt:** minimal daemon privileges, a dedicated user, a secure firewall configuration, no outbound internet access.

## 8. Platforms

| Platform | Status | Comment |
|---|---|---|
| **ESP32** | MVP, full support | Native A/B OTA with rollback in ESP-IDF, Wi-Fi, mbedTLS, enough Flash for all stock drivers |
| **Pico W** | 1.0, no OTA | Stock firmware and Web Serial configuration; updates via UF2. OTA needs a custom bootloader with two slots — after 1.0 |
| **STM32** | after 1.0 | Most STM32 parts have no networking — they need Ethernet (built-in MAC, W5500) or a Wi-Fi co-processor; small Flash → a custom build instead of stock firmware |

## 9. Deployment and Packaging

### 9.1 OpenWrt

```text
libcoapcpp     — universal library
coap-manager   — C++ gateway / HTTP API / SSE / UI assets / firmware repository
```

`luci-app-coap-manager` — after 1.0. The gateway is started through `procd`, and its configuration is integrated with UCI. For publication: `openwrt/packages`. Packages are built with the cross-toolchain/SDK; the difference in package managers (APK / OPKG) between OpenWrt versions is taken into account.

**Non-functional requirement:** fix a memory budget (roughly package ≤ 1 MB, RSS ≤ 10 MB) and check it on target routers with 16 MB Flash / 128 MB RAM, taking `libstdc++`, CivetWeb, and mbedTLS into account.

### 9.2 Ordinary Linux

The same `coap-manager` and Web UI; a systemd service or a Docker container. This makes it possible to develop and test the MVP on Ubuntu without waiting for OpenWrt packaging.

### 9.3 Firmware Build Tools

- An official Docker builder image with pinned versions of PlatformIO/toolchain.
- The project's CI builds stock images for all supported boards with the same image and publishes them in Releases.
- The `smartnode` CLI for developers (keygen, build, sign, upload).

## 10. Implementation Plan

| Stage | Deliverable | Definition of done |
|---|---|---|
| **0.1 — PoC** | Gateway on Ubuntu: CoAP event loop + CivetWeb + SSE; ESP32 + BME280 + relay with hand-written resources | The value in the browser updates through Observe→SSE; the relay switches |
| **0.2 — Registration and identity** | RD (`/rd`), stable `ep`, online/offline by lifetime, SenML, `rt`/`if`; Resource Explorer | A new node appears in the UI by itself after power-on; an IP change breaks nothing |
| **0.3 — Stock firmware + SDK** | SmartNode SDK, stock ESP32 firmware with configurable drivers, board descriptor, Releases in CI | The user configures sensors without compilation |
| **0.4 — Provisioning and security** | Web Serial flashing, issuing Wi-Fi + PSK, DTLS-PSK, UI authentication and roles | From "board out of the box" to "sensor on the dashboard" without a command line |
| **0.5 — OTA (ESP32) + firmware repository** | Pull-OTA over Block2, signed manifest, A/B + rollback; `/api/v1/firmware` | Updates from Releases, confirmed through re-registration |
| **0.6 — Local build** | Docker builder, CLI `smartnode build / sign / upload`, user key | Custom firmware built on a PC, signed, and installed over OTA; the build is reproducible in CI |
| **0.7 — OpenWrt** | `libcoapcpp` and `coap-manager` packages, procd, UCI | Installation on a test router within the memory budget |
| **1.0 — Stable** | Documentation, security audit, tests, Pico W (no OTA), export to MQTT | Reproducible deployment and stable updates |

### MVP scope — what we build first

- A single `coap-manager` process on Ubuntu/Linux; the business logic is not tied to OpenWrt.
- CivetWeb, one CoAP thread, SSE.
- ESP32 + BME280 + relay; a simple web interface.
- CoAP GET/PUT, Observe, self-registration, SenML.
- Once the scheme is confirmed — stock firmware, provisioning, OTA, local build.

## 11. Open Decisions

**Closed in v0.2:**

| v0.1 issue | Decision |
|---|---|
| HTTP backend and concurrency | CivetWeb + one CoAP event loop + command queue + state snapshot (3.3.1) |
| Device profile specification | `rt`/`if`/`obs` + SenML + `rt` vocabulary (5.1) |
| JSON or CBOR | SenML in both; JSON by default |
| Wi-Fi and key provisioning | Web Serial during initial flashing; the key is generated by the gateway (4.1) |
| OTA scheme | Pull over Block2, A/B slots, manifest modeled on SUIT (4.3, 5.4) |
| First MCUs | ESP32 — fully; Pico W — without OTA; STM32 — after 1.0 (8) |
| Manifest signature | Ed25519, project and user keys, `sequence` against downgrade |
| History storage | RAM buffer; history through export (3.3.2) |
| Build Server and its funding | Not needed: stock firmware + local build (3.6) |

**Still open:**

1. The final project name; licenses (recommendation: Apache-2.0 or MIT for `libcoapcpp` and the SDK).
2. The full vocabulary of `rt` values and the JSON schema of the sensor configuration.
3. Where to host the initial flashing page: HTTPS on the gateway or the project's HTTPS site (the Web Serial secure-context requirement).
4. How many image versions to keep on the gateway and whether to support USB storage.
5. A two-slot bootloader for Pico W.
6. The network stack and the list of boards for STM32.
7. The migration path from DTLS-PSK to OSCORE/EDHOC.
8. Post-MVP integrations: an MQTT bridge with Home Assistant MQTT Discovery, charts, automation rules.

## 12. Links

- [libcoapcpp — the project library] — repository URL to be confirmed.
- [CivetWeb](https://github.com/civetweb/civetweb)
- [PlatformIO](https://platformio.org/)
- [OpenWrt packages](https://github.com/openwrt/packages)
- [esptool-js](https://github.com/espressif/esptool-js)
- [RFC 7252 — CoAP](https://www.rfc-editor.org/rfc/rfc7252)
- [RFC 6690 — CoRE Link Format](https://www.rfc-editor.org/rfc/rfc6690)
- [RFC 7641 — CoAP Observe](https://www.rfc-editor.org/rfc/rfc7641)
- [RFC 7959 — CoAP Block-Wise Transfers](https://www.rfc-editor.org/rfc/rfc7959)
- [RFC 8075 — HTTP-to-CoAP Mapping](https://www.rfc-editor.org/rfc/rfc8075)
- [RFC 8428 — SenML](https://www.rfc-editor.org/rfc/rfc8428)
- [RFC 8613 — OSCORE](https://www.rfc-editor.org/rfc/rfc8613)
- [RFC 9019 — SUIT Architecture](https://www.rfc-editor.org/rfc/rfc9019)
- [RFC 9124 — SUIT Information Model](https://www.rfc-editor.org/rfc/rfc9124)
- [RFC 9176 — CoRE Resource Directory](https://www.rfc-editor.org/rfc/rfc9176)
- [RFC 9528 — EDHOC](https://www.rfc-editor.org/rfc/rfc9528)

---

**Key principle:** local management, provisioning, and OTA work without any external server. A typical user never compiles firmware; a developer builds their own locally. The gateway and SmartNode stay open, portable, and based on IETF standards.
