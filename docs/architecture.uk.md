# Архітектура libcoapcpp

Стан на 2026-10-18.

Цей документ описує цільову структуру проєкту: якою вона має стати після переробки, а не якою є зараз. Послідовність кроків до неї наведено в [плані робіт](roadmap.uk.md).

## Мета

libcoapcpp розвивається як C++-бібліотека CoAP для мікроконтролерів і POSIX-систем. Ядро не виконує вводу-виводу (підхід sans-IO), а все платформне підключається тонкими адаптерами.

Цільові платформи:

- POSIX: Linux на x86, Raspberry Pi 4/5, STM32MP1 (Cortex-A, OpenSTLinux), маршрутизатори з OpenWrt (musl, MIPS і ARM);
- ESP32, ESP32-C3, ESP32-S3 (ESP-IDF: FreeRTOS, lwIP, mbedTLS);
- STM32F4 і STM32H7;
- RP2040 і RP2350 (Raspberry Pi Pico, Pico W, Pico 2, Pico 2 W).

Головний споживач бібліотеки — система CoAP Smart Home ([опис](coap_smart_home_system.uk.md)). Її gateway `coap-manager` працює на Linux і OpenWrt, а вузли (стокова прошивка і SmartNode SDK) — на ESP32 і Pico W. Вимоги системи до бібліотеки зібрано в розділі [Вимоги CoAP Smart Home](#вимоги-coap-smart-home).

## Принципи

1. **Ядро без вводу-виводу.** `src/` не включає жодного системного заголовка і не містить жодного `#ifdef` за платформою.
2. **Усе платформне сховане за вузькими інтерфейсами.** Їх п'ять: транспорт, годинник, випадкові числа, журнал і необов'язковий резолвер імен. Вони оголошені в `api/coap/port/`, а реалізовані в `port/`.
3. **Порти діляться за програмним API, а не за платою.** Осей три: мережевий стек (`port/net`), операційна система (`port/os`) і мікросхема (`port/hal`). Кожна платформа є комбінацією каталогів із цих осей.
4. **Код плати лежить лише в `boards/`.** Це `main`, піни, тактування, `lwipopts.h`, `FreeRTOSConfig.h`, лінкер-скрипт і драйвери датчиків. Логіка прикладів лежить в `examples/` і не містить жодного платформного рядка.
5. **Платформу обирає система збирання**, тобто перелік цілей CMake, а не препроцесор у вихідних файлах.

## Напрям залежностей

Залежності спрямовані в один бік:

```
boards/*  ──►  port/*  ──►  api/coap/port/*.h  ◄──  src/*
   │                                                  ▲
   └──►  examples/*  ─────────────────────────────────┘
```

- `src/` включає лише стандартну бібліотеку C++, `api/coap/` і, для форматів, cJSON.
- `port/X/Y` включає `api/coap/port/` і заголовки своєї платформи. Порти не включають один одного. Виняток: `port/net/dtls`, який обгортає будь-який транспорт через інтерфейс.
- `examples/` включає лише `api/coap/`: жодного `port/` і жодного системного заголовка.
- `boards/Y` збирає потрібні порти докупи, додає код плати і викликає `app_main` з `examples/`.

Правило перевіряється однією командою, яка має повертати порожній результат. Її варто запускати в CI:

```sh
grep -rE '#\s*if.*(__linux__|__unix__|__arm__|STM32|PICO_|LWIP|ESP_PLATFORM|CONFIG_IDF)' api src examples
```

## Структура каталогів

```
libcoapcpp/
├── api/coap/                    публічні заголовки; підключаються як <coap/packet.h>
│   ├── consts.h  error.h  buffer.h  packet.h  uri.h  blockwise.h  utils.h
│   ├── core_link.h  senml_json.h  data_type.h        формати
│   ├── server.h  client.h  endpoint.h                рушій: одна кінцева точка = сервер + клієнт
│   ├── messaging.h  observe.h  block_transfer.h      рівень повідомлень, Observe, потоковий block-wise
│   ├── config.h                                      розміри й ліміти, перевизначаються
│   └── port/                    <coap/port/transport.h>
│       └── transport.h  clock.h  random.h  log.h  resolver.h  psk_store.h     чисті інтерфейси
│
├── src/                         жодного системного заголовка, жодного #ifdef платформи
│   ├── packet.cc  uri.cc  blockwise.cc  error.cc  utils.cc
│   ├── core_link.cc  senml_json.cc  base64.cc  base64.h
│   ├── server.cc  client.cc  endpoint.cc  packet_helper.cc
│   └── messaging.cc  observe.cc  block_transfer.cc
│
├── port/                        реалізації інтерфейсів; кожен каталог є окремою ціллю CMake
│   ├── net/                     Transport, Resolver
│   │   ├── posix/     udp_transport.cc  resolver.cc
│   │   ├── lwip/      udp_transport_sockets.cc  udp_transport_raw.cc  resolver.cc
│   │   └── dtls/
│   │       ├── wolfssl/   dtls_transport.cc  wolfssl_error.cc  wolfssl_error.h
│   │       └── mbedtls/   dtls_transport.cc     (клієнт і сервер, PSK)
│   ├── os/                      Clock, Log, купа
│   │   ├── posix/     clock.cc  random.cc  spdlog_log.cc
│   │   └── freertos/  clock.cc  heap.cc
│   └── hal/                     Random, Clock без ОС
│       ├── stm32/     random.cc  clock.cc
│       ├── pico/      random.cc  clock.cc
│       └── esp32/     random.cc  clock.cc
│
├── examples/                    логіка прикладів; без main() і без платформного коду
│   ├── coap-server/   app_main.cc  sensor.*  endpoint.*  sensor_stubs.*
│   └── coap-client/   app_main.cc
│
├── boards/                      main(), BSP, створення портів, ресурси плати
│   ├── posix/                main_server.cc  main_client.cc  (getopt, сигнали)
│   │   ├── gpio/             dht11.cc  rgb_led.cc  (libgpiod)
│   │   └── docs/             інструкції для Raspberry Pi та STM32MP157A-DK1
│   ├── esp32-devkitc/        main.cc  sdkconfig.defaults    (проєкт ESP-IDF; ESP32, ESP32-C3, ESP32-S3)
│   ├── nucleo-f429zi/        main.cc  bsp/  lwipopts.h  FreeRTOSConfig.h
│   ├── nucleo-h743zi/        main.cc  bsp/  lwipopts.h  FreeRTOSConfig.h
│   ├── pico-w/               main.cc  lwipopts.h  FreeRTOSConfig.h    (Pico W і Pico 2 W)
│   ├── pico-eth/             main.cc  rmii.pio  pio_netif.cc  lwipopts.h  (RP2350, Ethernet MAC на PIO)
│   └── pico-spi/             CoAP через SPI для Pico без мережі
│       ├── common/       spi_framing.*            кодек кадру, без платформи
│       ├── firmware/     main.cc  spi_transport.cc     (RP2040 / RP2350)
│       └── gateway/      main.cc  spidev.cc            міст UDP ↔ SPI на Linux
│
├── cmake/                       скрипти імпорту Pico SDK і FreeRTOS
│   └── toolchains/              arm-none-eabi-cortex-m4f.cmake  …-m7.cmake  aarch64-linux-gnu.cmake
├── test/                        лише хост
│   ├── mocks/     fake_clock.h  fake_random.h  loopback_transport.h
│   └── test_*.cc
├── packaging/
│   └── openwrt/                 Makefile пакета libcoapcpp
├── docs/
├── library.json                 PlatformIO, з srcFilter на src/ і потрібні port/
├── idf_component.yml            компонент ESP-IDF
└── third-party/
```

## Ядро

Ядро складається з `api/coap/` і `src/`.

- **Кодек і формати.** Пакети (RFC 7252), block-wise (RFC 7959), URI, SenML-JSON (RFC 8428), CoRE Link Format (RFC 6690).
- **Рушій.** `server` і `client` працюють через `processing(Buffer &)`: байти на вхід, байти на вихід. Про транспорт рушій не знає. Обидва об'єднано в `Endpoint`: один транспорт, один рівень повідомлень, ролі сервера і клієнта одночасно (R2).
- **Цикл обробки.** `coap::run(endpoint, transport, clock)` читає датаграми з транспорту і передає їх у `processing`. Хто має власний цикл подій, викликає `processing` напряму, а таймери веде через `next_timeout_ms()` і `tick(now)` (R3).
- **Потоки.** Ядро однопотокове: один екземпляр обслуговує один потік або одну задачу. М'ютексів серед портів немає.
- **Порядок байтів.** Багатобайтові поля записуються зсувами в самому ядрі; це не порт, і `htons` не потрібен.
- **Залежності.** Стандартна бібліотека C++ і cJSON. cJSON потрібен лише форматам і підключається опцією `COAPCPP_WITH_FORMATS`.
- **Налаштування.** Розміри буферів і ліміти зібрані в `api/coap/config.h` і перевизначаються проєктом користувача.

Обмеження: ядро використовує динамічну пам'ять (`std::string`, `std::vector`, `new[]`). З `-fno-exceptions` невдале виділення завершує програму. На FreeRTOS порт спрямовує `operator new` і `malloc` у купу RTOS, а cJSON налаштовується через `cJSON_InitHooks`. Відмова від купи є окремою роботою і цією архітектурою не охоплена. Для цілей CoAP Smart Home (Linux, OpenWrt, ESP32, Pico W) купа прийнятна; питання стосується лише малих STM32.

## Вимоги CoAP Smart Home

Система [CoAP Smart Home](coap_smart_home_system.uk.md) використовує бібліотеку у двох ролях:

| Роль | Платформа | Що робить через CoAP |
|---|---|---|
| Gateway `coap-manager` | Linux, OpenWrt | Сервер: Resource Directory `/rd`, образи прошивок `/fw` через Block2. Клієнт: запити до вузлів, одна Observe-підписка на ресурс, довільні запити з Resource Explorer. Десятки вузлів на одному сокеті. |
| Вузол (стокова прошивка, SmartNode SDK) | ESP32, Pico W; STM32 після 1.0 системи | Сервер: ресурси датчиків і виконавчих пристроїв, `/.well-known/core`, `/system/*`. Клієнт: реєстрація та її оновлення кожні `lt` секунд, завантаження образу через Block2. |

**Межа.** libcoapcpp лишається незалежною від системи. Вона реалізує загальні механізми IETF і нічого не знає про семантику `/rd`, словник `rt`, `/system/*`, HTTP, SSE, маніфест прошивки чи підписи образів. RD-сервер, реєстр пристроїв, відображення HTTP→CoAP (RFC 8075) і логіка OTA живуть у `coap-manager`. Профіль пристрою, конфігурація драйверів і OTA-backend (слоти Flash) живуть у SmartNode SDK. Пристрій, зібраний на libcoap чи aiocoap без SDK, має працювати з gateway без змін.

Вимоги пронумеровано, щоб на них міг посилатися [план робіт](roadmap.uk.md).

### Рушій

| № | Вимога | Gateway | Вузол |
|---|---|---|---|
| R1 | Рівень повідомлень: CON/NON/ACK/RST, повторні передачі з експоненційним backoff, дедуплікація за (вузол, Message ID) протягом `EXCHANGE_LIFETIME`, piggybacked і відкладені (separate) відповіді, `NSTART` на кожного співрозмовника. Параметри з RFC 7252 §4.8 — у `config.h`. | так | так |
| R2 | Один `Endpoint` на одному транспорті одночасно виконує ролі сервера і клієнта. Gateway приймає `/rd` і надсилає GET/PUT/Observe з того самого порту; вузол реєструється і обслуговує ресурси з того самого сокета, тож gateway досягає його за адресою джерела реєстрації. | так | так |
| R3 | Жоден виклик не блокує. Результати надходять через зворотні виклики, транзакцію можна скасувати. Рушій повідомляє `next_timeout_ms()` і просувається викликом `tick(now)`, тож вбудовується у власний цикл подій gateway поруч із чергою команд від HTTP-потоків (розділ 3.3.1 системи). Рушій лишається однопотоковим. | так | так |
| R4 | Багато одночасних транзакцій до різних співрозмовників, зіставлення за токеном і адресою; токени щонайменше з 4 випадкових байтів. | так | — |
| R5 | Серверний API: маршрутизація за Uri-Path зі змінним сегментом (`/rd/{id}`, `/fw/{board}/{file}`); додавання й видалення ресурсів під час роботи (стокова прошивка створює їх із конфігурації); доступ до Uri-Query (`ep`, `lt`); встановлення Location-Path (`2.01 Created`, `Location: /rd/4521`); відкладена відповідь для повільних обробників; автоматичні 4.04, 4.05, 4.06 і 4.15. | так | так |
| R6 | Автоматичний `/.well-known/core` з атрибутів ресурсів (`rt`, `if`, `obs`, `ct`, `sz`) з фільтрацією за запитом (`?rt=temperature`, RFC 6690 §4.1). Той самий серіалізатор формує payload реєстрації. | — | так |
| R7 | Клієнтський API з довільним методом, опціями, Content-Format, Accept і payload: Resource Explorer надсилає довільні запити. | так | так |

### Observe (RFC 7641)

| № | Вимога | Gateway | Вузол |
|---|---|---|---|
| R8 | Сервер: реєстрація і скасування (Observe 0 і 1, RST, невдала CON-нотифікація); 24-бітний порядковий номер; нотифікація за зміною; CON-нотифікація щонайменше раз на 24 години; Max-Age; обмеження кількості спостерігачів. | — | так |
| R9 | Клієнт: одна підписка на (вузол, ресурс); відкидання застарілих нотифікацій за порядковим номером (§3.4); повторна підписка після спливання Max-Age або після нової реєстрації вузла; явне скасування; зворотний виклик і на кожну нотифікацію, і на втрату підписки, щоб gateway міг позначити значення застарілим. | так | — |

### Block-wise (RFC 7959)

| № | Вимога | Gateway | Вузол |
|---|---|---|---|
| R10 | Сервер Block2 без завантаження ресурсу в пам'ять: дані надходять зі зворотного виклику `(offset, size)`; Size2 і ETag; будь-який блок на запит, тож вузол продовжує завантаження після обриву; менший розмір блока, запитаний клієнтом, враховується. Образ на 1 МБ на роутері не потрапляє в RAM. | так | — |
| R11 | Клієнт Block2 зі зворотним викликом на кожен блок (запис у слот Flash), початок із довільного номера блока, переривання при зміні ETag, Size2 відомий до першого запису. Прозоре збирання дозволене лише для малих payload і з обмеженням. | — | так |
| R12 | Block1 з обох боків: вузол надсилає payload реєстрації, довший за один блок, а gateway збирає його з обмеженням, відповідаючи 2.31 Continue між блоками і 4.13 при перевищенні. Той самий механізм використовує `PUT /system/config`. | так | так |

### Формати й опції

| № | Вимога | Gateway | Вузол |
|---|---|---|---|
| R13 | CoRE Link Format за RFC 6690: атрибути без значення (`obs`), значення як ptoken і як quoted-string, кілька значень в одному атрибуті (`rt="a b"`); розбір і серіалізація симетричні. | так | так |
| R14 | SenML-JSON (Content-Format 110): `bn`, `bt`, `bu`, `n`, `u`, `v`, `vb`, `vs`, `vd`, `t`. SenML-CBOR (112) — опція, і їй потрібен власний CBOR-кодек, бо cJSON із CBOR не працює. | так | так |
| R15 | Опції Observe (6), Uri-Query, Location-Path, Accept, Max-Age, ETag, Size1, Size2 з допоміжними функціями для uint-значень. Content-Format 40, 42, 50, 110, 112. | так | так |

### Безпека

| № | Вимога | Gateway | Вузол |
|---|---|---|---|
| R16 | DTLS 1.2 з PSK і `TLS_PSK_WITH_AES_128_CCM_8` (RFC 7252 §9.1.3.1). Бекенд mbedTLS — першим, бо його постачають ESP-IDF і OpenWrt. Вузол є DTLS-клієнтом, gateway — DTLS-сервером. | так | так |
| R17 | DTLS-сервер: багато одночасних сесій за адресою співрозмовника, cookies (HelloVerifyRequest), тайм-аут неактивної сесії, обмеження кількості сесій. Ключ шукається за identity через зворотний виклик `PskStore`; ключі видає gateway під час provisioning. | так | — |
| R18 | Автентифікована PSK identity доходить до обробника запиту, тож gateway може перевірити, що `ep` у `/rd` відповідає ключу, і відхилити запити, що прийшли без DTLS. | так | — |
| R19 | Запити в обох напрямках через одне DTLS-з'єднання: сесію відкриває вузол, а gateway надсилає йому GET/PUT/Observe тією самою сесією. Вузлу не потрібні ні роль DTLS-сервера, ні друга сесія. | так | так |
| R20 | Після 1.0 системи: OSCORE (RFC 8613) і EDHOC (RFC 9528). Рівень повідомлень має лишати місце для шару, що захищає опції й payload між рушієм і транспортом. | пізніше | пізніше |

### Платформи, збірка, якість

| № | Вимога | Gateway | Вузол |
|---|---|---|---|
| R21 | Платформи: gateway на Linux x86/ARM і OpenWrt (musl, MIPS/ARM); вузол на ESP32 (ESP-IDF; PlatformIO `espressif32` з фреймворками `espidf` і `arduino`) і Pico W (Pico SDK); STM32 після 1.0 системи. | так | так |
| R22 | Пакування: CMake `install` з експортом пакета і pkg-config, статична або спільна бібліотека, системний mbedTLS через `find_package`, жодного обов'язкового субмодуля; пакет OpenWrt; компонент ESP-IDF; `library.json` для PlatformIO. | так | так |
| R23 | Бюджет пам'яті: частка бібліотеки в пакеті `coap-manager` (≤ 1 МБ разом із CivetWeb і mbedTLS, RSS ≤ 10 МБ на роутері з 16 МБ Flash / 128 МБ RAM) фіксується і вимірюється в CI. | так | — |
| R24 | Ліміти в `config.h`: співрозмовники, транзакції, спостерігачі, підписки, DTLS-сесії, розмір збирання. При вичерпанні ліміту — відповідь 5.03 з Max-Age або код помилки, але ніколи не аварійне завершення. Усі парсери покриті fuzzing. | так | так |
| R25 | Сумісність із libcoap, aiocoap і Californium перевіряється в CI, зокрема з DTLS-PSK: система обіцяє сумісні пристрої без SDK. | так | так |
| R26 | Необов'язково: multicast (RFC 7252 §8) як резервний пошук: приєднання до `ff02::fd` і `224.0.1.187` у порті, NON-запити, кілька відповідей на один токен, жодних відповідей-помилок на multicast-запит. | пізніше | пізніше |

## Інтерфейси портів

Чотири обов'язкові інтерфейси і два необов'язкові, усі сумісні з C++11. Ядро отримує їх через конструктор, тому в тестах підставляються фальшиві реалізації.

```cpp
namespace coap { namespace port {

// api/coap/port/clock.h
class Clock {
public:
    virtual ~Clock() = default;
    virtual std::uint64_t now_ms() = 0;          // монотонний, не календарний
};

// api/coap/port/random.h
class Random {
public:
    virtual ~Random() = default;
    virtual void fill(std::uint8_t *out, std::size_t size) = 0;
};

// api/coap/port/transport.h
struct Address {                                 // без sockaddr і ip_addr_t
    std::uint8_t  bytes[16];
    std::uint8_t  length;                        // 4 або 16
    std::uint16_t port;                          // у порядку хоста
};

class Transport {
public:
    virtual ~Transport() = default;
    virtual void send(const Address &to, const std::uint8_t *data,
                      std::size_t size, std::error_code &ec) = 0;
    // повертає 0 і COAP_ERR_TIMEOUT, якщо за timeoutMs нічого не надійшло
    virtual std::size_t receive(Address &from, std::uint8_t *data, std::size_t capacity,
                                std::uint32_t timeoutMs, std::error_code &ec) = 0;
    // автентифікована identity співрозмовника (PSK identity для DTLS); false для звичайного UDP
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

// api/coap/port/resolver.h (необов'язковий, потрібен лише клієнту)
class Resolver {
public:
    virtual ~Resolver() = default;
    virtual void resolve(const char *hostname, std::uint16_t port,
                         Address &out, std::error_code &ec) = 0;
};

// api/coap/port/psk_store.h (необов'язковий, потрібен лише DTLS-серверу)
class PskStore {
public:
    virtual ~PskStore() = default;
    // false відхиляє рукостискання
    virtual bool find(const std::uint8_t *identity, std::size_t identityLength,
                      std::uint8_t *key, std::size_t &keyLength) = 0;
};

}}
```

Призначення інтерфейсів:

| Інтерфейс | Навіщо він ядру |
|---|---|
| `Transport` | Потрібен лише циклу `coap::run`; рушій працює з буферами. `peer_identity` передає обробникам запитів автентифіковану identity співрозмовника (R18). |
| `Clock` | Тайм-аути клієнта і сервера, а надалі повторні передачі CON. |
| `Random` | Токени та Message ID. Токен за RFC 7252 §5.3.1 має бути непередбачуваним. |
| `Log` | Діагностика без прив'язки до spdlog. |
| `Resolver` | Перетворення імені хоста на `Address` у клієнті. |
| `PskStore` | Пошук ключа за PSK identity у DTLS-сервері (R17). Реалізує застосунок: на gateway ключі беруться з реєстру пристроїв. |

## Порти

Порт є реалізацією інтерфейсів для певного програмного API, а не для заліза. Raspberry Pi 4, Pi 5, STM32MP1 і x86 для бібліотеки однакові: ті самі сокети, `clock_gettime`, `getrandom`. STM32F4, STM32H7 і RP2040 мають спільний мережевий стек (lwIP) і різняться лише джерелом ентропії та годинником.

Осі поділу:

- `port/net`: мережевий стек;
- `port/os`: операційна система;
- `port/hal`: мікросхема. Ця вісь потрібна, бо генератор випадкових чисел і годинник без ОС не належать ні до мережі, ні до ОС, а в `boards/` вони дублювалися б між платами однієї родини.

Плата бере по одному каталогу з кожної потрібної осі.

| Каталог | Що реалізує | Примітки |
|---|---|---|
| `port/net/posix` | `Transport`: `socket`, `sendto`, `recvfrom`, `poll`. `Resolver`: `getaddrinfo`. | Спільний для всього Linux, зокрема OpenWrt. `native_handle()` віддає сокет для власного `poll` застосунку (R3). Приєднання до multicast-групи — необов'язкове (R26). |
| `port/net/lwip` | `Transport` у двох варіантах: на сокетах lwIP (`udp_transport_sockets.cc`, потребує ОС) і на сирому API `udp_*` (`udp_transport_raw.cc`). `Resolver`: `lwip_getaddrinfo`. | Плата обирає один із двох файлів; ядро про це не знає. Сирий варіант потрібен лише для збірки без ОС (`NO_SYS=1`). |
| `port/net/dtls/wolfssl`, `port/net/dtls/mbedtls` | `DtlsTransport`: обгортка над будь-яким `Transport`, яка сама реалізує `Transport`. | Це шар над `posix` чи `lwip`, а не альтернатива їм. Збирається лише з `COAPCPP_WITH_DTLS`. Основний бекенд — mbedTLS, бо його постачають ESP-IDF і OpenWrt. Ролі клієнта і сервера, PSK, сесії за адресою (R16, R17, R19). |
| `port/os/posix` | `Clock`: `clock_gettime(CLOCK_MONOTONIC)`. `Random`: `getrandom`. `Log`: spdlog. | На Linux ентропію дає ОС, тому `Random` тут, а не в `hal`. |
| `port/os/freertos` | `Clock`: `xTaskGetTickCount`. `heap.cc`: `operator new`/`delete` і хуки cJSON на `pvPortMalloc`. | Спільний для STM32 і RP2040/RP2350. ESP-IDF використовує лише `clock.cc`: його `malloc` уже потокобезпечний. |
| `port/hal/stm32` | `Random`: `HAL_RNG_GenerateRandomNumber`. `Clock`: `HAL_GetTick` для збірки без ОС. | Один файл для F4 і H7, бо API HAL однаковий; заголовок родини підключає плата. |
| `port/hal/pico` | `Random`: `get_rand_32` з `pico_rand`. `Clock`: `time_us_64`. | Один файл для RP2040 і RP2350. |
| `port/hal/esp32` | `Random`: `esp_fill_random`. `Clock`: `esp_timer_get_time`. | Один файл для ESP32, ESP32-C3 і ESP32-S3. `Random` дає справжні випадкові числа лише при ввімкненому Wi-Fi чи Bluetooth, що на вузлі виконується. |

Самих lwIP, FreeRTOS і HAL у репозиторії немає: їх приносить SDK вендора (STM32Cube, Pico SDK, ESP-IDF) або PlatformIO. Порти написані проти їхнього публічного API.

## Плати

| Плата (`boards/`) | `port/net` | `port/os` | `port/hal` | Тулчейн |
|---|---|---|---|---|
| `posix` (Linux x86) | `posix` | `posix` | немає | хостовий |
| `posix`, набір `raspberry-pi` (Pi 4/5) | `posix` | `posix` | немає | хостовий або `aarch64-linux-gnu` |
| `posix`, набір `stm32mp157a-dk1` | `posix` | `posix` | немає | SDK OpenSTLinux |
| `posix`, набір `openwrt` | `posix` | `posix` | немає | OpenWrt SDK (musl) |
| `esp32-devkitc` (ESP32, ESP32-C3, ESP32-S3) | `lwip` (сокети) | `freertos` (ESP-IDF) | `esp32` | ESP-IDF або PlatformIO `espressif32` |
| `nucleo-f429zi` | `lwip` | `freertos` | `stm32` | `arm-none-eabi`, cortex-m4f |
| `nucleo-h743zi` | `lwip` | `freertos` | `stm32` | `arm-none-eabi`, cortex-m7 |
| `pico-w` (RP2040 / RP2350) | `lwip` | `freertos` або жодного | `pico` | Pico SDK |
| `pico-eth` (RP2350, Ethernet через RMII на PIO) | `lwip` | `freertos` або жодного | `pico` | Pico SDK |
| `pico-spi/firmware` (RP2040 / RP2350 без мережі) | власний `Transport` через SPI | `freertos` | `pico` | Pico SDK |

До будь-якого рядка за потреби додається `port/net/dtls/wolfssl` або `port/net/dtls/mbedtls`.

Правила для каталогу `boards/`:

- **`main()` завжди в каталозі плати.** На мікроконтролері інакше неможливо: `main` ініціалізує HAL і запускає планувальник. Він створює порти і викликає `app_main` з `examples/`.
- **Одна плата відповідає одному каталогу, поки не розійшовся код.** Pi 4 і Pi 5 різняться номером GPIO-чипа, тобто параметром. Pico W і Pico 2 W різняться змінною `PICO_BOARD`, тобто набором збирання.
- **`boards/posix` є єдиною платою для всього Linux.** x86, Raspberry Pi 4/5 і STM32MP157A-DK1 збираються з одного каталогу: той самий `main` із розбором аргументів і сигналами, ті самі порти. Різняться вони тулчейном і опцією `SENSORS=stub|gpio`.
- **Драйвери датчиків лежать у `boards/`**, бо прив'язані до API GPIO. Симулятори (`sensor_stubs.*`) платформи не потребують і лежать в `examples/coap-server/`, щоб їх могла взяти будь-яка плата без датчиків.
- **Відмінності рівня BSP не потрапляють у `port/`.** Драйвер ETH, кеш і MPU, розміщення дескрипторів ETH у пам'яті для F4 і H7 живуть лише в `boards/`.
- **Спільні файли плат Pico.** `pico_sdk_import.cmake` і `FreeRTOS_Kernel_import.cmake` лежать у `cmake/`, а не копіюються в кожну плату. `FreeRTOSConfig.h` і `lwipopts.h` лишаються в платах, бо саме ними плати й різняться.

### Нотатки щодо окремих плат

- **`posix`, датчики.** GPIO працює через libgpiod, тож код один для Raspberry Pi 4/5 і STM32MP1. Номери ліній і GPIO-чип передаються аргументами командного рядка. DHT11 краще читати через драйвер ядра (`dtoverlay=dht11`, значення з `/sys/bus/iio/`), бо часові вимоги протоколу тоді виконує ядро.
- **`pico-spi`.** Pico без мережевого стека обслуговує CoAP через SPI, а Linux-плата працює шлюзом UDP ↔ SPI. Кадр несе до 251 байта, тому блоки block-wise не можуть перевищувати 128 байт. У кадрі немає адреси, тож шлюз обслуговує одного клієнта за раз. Шлюзу бібліотека не потрібна.
- **`pico-eth`.** MAC на PIO є драйвером мережевого інтерфейсу (`netif`) для lwIP, тобто тим самим рівнем, що `ethernetif.c` у NUCLEO-F429ZI. Нових портів плата не додає. Якщо для неї доведеться щось змінити в `port/` чи `src/`, це ознака помилки в поділі. RMII потребує опорних 50 МГц, і системну частоту доведеться підбирати під неї.
- **`posix`, набір `openwrt`.** Той самий `boards/posix`, зібраний OpenWrt SDK проти musl. mbedTLS береться із системного пакета через `find_package`, а не з `third-party/`. Цей набір перевіряє збірку для gateway; сам пакет `libcoapcpp` збирає `packaging/openwrt/Makefile`.
- **`esp32-devkitc`.** Проєкт ESP-IDF, бібліотека підключається як компонент. lwIP і FreeRTOS приносить ESP-IDF, тож плата додає лише `port/hal/esp32`. `sdkconfig.defaults` вмикає DTLS і PSK у mbedTLS (`CONFIG_MBEDTLS_SSL_PROTO_DTLS`, `CONFIG_MBEDTLS_PSK_MODES`). NVS, OTA-слоти й provisioning Wi-Fi — справа SmartNode SDK, а не бібліотеки.
- **Ядро Cortex-M4 у STM32MP1** не розглядається. Якщо воно знадобиться, це ще одна плата з `port/hal/stm32` і транспортом через OpenAMP.

## Приклади

Кожен приклад є функцією `app_main`, яка отримує готові порти і ресурси плати:

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

Завдяки цьому `examples/` збирається без змін під кожну плату. Плата реєструє власні ресурси (датчики, RTC, акселерометр), тому перелік типів датчиків у прикладі не може бути закритим.

## Збірка

Кожен шар є окремою ціллю CMake. Платформа обирається переліком цілей, який задає плата, тож користувач вказує лише її.

```cmake
cmake_minimum_required(VERSION 3.13)
project(coapcpp CXX C)

option(COAPCPP_WITH_FORMATS "SenML-JSON, CoRE Link"  ON)
option(COAPCPP_WITH_DTLS    "DTLS transport"          OFF)
option(COAPCPP_BUILD_TESTS  "Unit tests (host only)"  OFF)
set(COAPCPP_BOARD "" CACHE STRING "каталог із boards/, наприклад nucleo-f429zi")

add_library(coapcpp STATIC src/packet.cc src/uri.cc ...)
target_include_directories(coapcpp PUBLIC api)
target_compile_features(coapcpp PUBLIC cxx_std_11)

if(COAPCPP_BOARD)
    add_subdirectory(boards/${COAPCPP_BOARD})   # підключає потрібні port/* і examples/*
endif()
```

Файл плати, наприклад `boards/nucleo-f429zi/CMakeLists.txt`:

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

Правила:

- Бібліотека не задає прапорців оптимізації, `-frtti`, `-L` і не чіпає `NDEBUG`. Це справа тулчейн-файлу або проєкту користувача.
- `third-party/` підключається лише за потребою: cJSON з `COAPCPP_WITH_FORMATS`, wolfSSL або mbedTLS з `COAPCPP_WITH_DTLS`, spdlog з `port/os/posix`, GoogleTest з `COAPCPP_BUILD_TESTS`.
- `port/net/lwip`, `port/os/freertos` і `port/hal/*` оголошуються як `INTERFACE`-бібліотеки з джерелами. Їм потрібні `lwipopts.h`, `FreeRTOSConfig.h` і заголовки HAL із каталогу плати, тому компілюватися вони мають у його контексті.
- Шлях до SDK вендора задається змінною CMake; сам SDK лежить поза репозиторієм.
- Набори збирання описані в `CMakePresets.json`, а `build.sh` зводиться до `cmake --preset <назва>`.
- Для споживачів (R22): `install` з експортом CMake-пакета і файлом `.pc` для pkg-config; `BUILD_SHARED_LIBS` враховується; mbedTLS, cJSON і spdlog спершу шукаються через `find_package` і лише як запасний варіант беруться з `third-party/`, тож збірці для OpenWrt субмодулі не потрібні.
- Під ESP-IDF (змінна CMake `ESP_PLATFORM`, а не препроцесор у вихідних файлах) кореневий `CMakeLists.txt` реєструє бібліотеку через `idf_component_register`; маніфест — `idf_component.yml`.

| Набір | Каталог плати | Чим відрізняється |
|---|---|---|
| `posix` | `boards/posix` | хостовий тулчейн, `SENSORS=stub` |
| `raspberry-pi` | `boards/posix` | `SENSORS=gpio`, за потреби тулчейн `aarch64-linux-gnu` |
| `stm32mp157a-dk1` | `boards/posix` | тулчейн OpenSTLinux SDK, `SENSORS=gpio` |
| `nucleo-f429zi` | `boards/nucleo-f429zi` | cortex-m4f |
| `nucleo-h743zi` | `boards/nucleo-h743zi` | cortex-m7 |
| `pico-w`, `pico2-w` | `boards/pico-w` | `PICO_BOARD` |
| `pico-eth` | `boards/pico-eth` | RP2350 |
| `pico-spi` | `boards/pico-spi` | прошивка і шлюз |
| `esp32` | `boards/esp32-devkitc` | ESP-IDF, `IDF_TARGET=esp32\|esp32c3\|esp32s3` |
| `openwrt` | `boards/posix` | тулчейн OpenWrt SDK, musl, системний mbedTLS |

**PlatformIO.** `library.json` лежить у корені `main` і через `srcFilter` бере `src/`, `port/net/lwip`, `port/os/freertos` і `port/hal/*` своєї платформи; платформу обирає `extraScript`. Підтримувані платформи: `espressif32` (фреймворки `espidf` і `arduino`), `raspberrypi`, `ststm32`. Окрема гілка `platformio` стає непотрібною.

## Тестування

- Юніт-тести в `test/` збираються лише на хості і лише з `COAPCPP_BUILD_TESTS`.
- Ядро тестується з фальшивими портами з `test/mocks/`: `fake_clock.h`, `fake_random.h`, `loopback_transport.h`.
- Кожен порт має власний тест поруч із кодом. Наприклад, `port/net/posix/test/` надсилає датаграму через `Transport` на `::1` і приймає її.
- Крос-компіляція ядра під `arm-none-eabi` перевіряється в CI без жодного порту; `esp32-devkitc` і набір `openwrt` теж збираються в CI.
- `loopback_transport.h` уміє губити, дублювати й переставляти датаграми. На ньому з фальшивим годинником проганяються сценарії CoAP Smart Home: реєстрація в `/rd`, Observe із втратою нотифікацій, керування з відкладеною відповіддю, завантаження Block2 з продовженням після обриву.
- Сумісність із libcoap, aiocoap і Californium через звичайний UDP і DTLS-PSK перевіряється в CI (R25).
- Розмір бібліотеки без символів і RSS прикладу вимірюються в CI і порівнюються з бюджетом (R23).

Ознака того, що архітектура працює: `examples/coap-server/` збирається без змін під усі плати, а каталог кожної плати містить лише ініціалізацію заліза, створення портів і реєстрацію своїх ресурсів.

## Прийняті рішення

- **Віртуальні інтерфейси замість шаблонів чи вибору на етапі лінкування.** Ціна становить один непрямий виклик на пакет, що для CoAP непомітно. Натомість ядро збирається один раз і тестується на хості з підставними портами.
- **Рушій не знає транспорту.** Це дозволяє вбудувати його у власний цикл подій і тестувати без мережі.
- **`port/` лежить окремо від `src/`.** Каталог `src/` можна цілком додати в будь-яку систему збирання (STM32CubeIDE, PlatformIO, Pico SDK) без фільтрів.
- **Старий мережевий шар не відновлюється.** Абстракція `Socket`/`Connection`, видалена в коміті `60f749e`, повторювала BSD-сокети, а не потреби CoAP, і її неможливо реалізувати для каналу SPI чи для RP2040 без ОС. Повертається її вміст, розкладений по `port/` за інтерфейсом `Transport`.
- **Нові коди помилок портів** оголошуються в самих портах через власну `std::error_category`, а не додаються в `CoapStatus`.
- **Бібліотека не знає про CoAP Smart Home.** Від системи вона бере лише загальні механізми IETF (R1–R26). RD-сервер, профіль пристрою і логіка OTA лишаються в `coap-manager` і SmartNode SDK, тож бібліотека корисна й поза системою, а пристрої без SDK лишаються сумісними.
- **Потоковий block-wise замість збирання в пам'яті.** Образ прошивки має близько 1 МБ: gateway читає його з файла поблоково, а вузол пише кожен блок у слот Flash. Збирання в буфері лишається тільки для малих payload і з обмеженням (R10–R12).
- **Одне DTLS-з'єднання для обох напрямків.** Вузол відкриває сесію як клієнт, а gateway надсилає свої запити тією самою сесією. DTLS-сервер на вузлі коштував би RAM і другого обміну ключами (R19).
