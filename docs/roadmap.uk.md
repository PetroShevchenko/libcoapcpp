# План робіт libcoapcpp

Стан на 2026-10-18.

Цей документ описує поточний стан проєкту і послідовність кроків до цільової структури з [опису архітектури](architecture.uk.md).

Перший споживач бібліотеки — система [CoAP Smart Home](coap_smart_home_system.uk.md). Її вимоги до бібліотеки пронумеровано R1–R26 в [описі архітектури](architecture.uk.md#вимоги-coap-smart-home); етапи нижче посилаються на них, а розділ [Відповідність етапам CoAP Smart Home](#відповідність-етапам-coap-smart-home) показує, який етап бібліотеки потрібен кожному етапу системи.

Висновки про код зроблено з читання вихідних файлів та історії репозиторію. Код при цьому не збирався й не запускався, тому все, що стосується поведінки парсера, треба підтвердити тестами.

## Поточний стан

- **Активність.** 72 коміти в `main` з квітня 2021 до січня 2023. Гілка `platformio` (жовтень 2023) не змерджена. CI немає.
- **Що реалізовано в бібліотеці.** Кодек пакетів (RFC 7252), block-wise (RFC 7959), SenML-JSON (RFC 8428), CoRE Link Format (RFC 6690), URI, юніт-тести на GoogleTest. Observe (RFC 7641) запланований.
- **Рушій сервера і клієнта.** Лежить не в бібліотеці, а в `examples/POSIX/common/`. Він працює через `processing(Buffer &)` і сокетів не торкається, тобто фактично вже є sans-IO, але через розташування недоступний для мікроконтролерів.
- **Приклади в `main`.** `examples/POSIX` (сервер і клієнт) та `examples/RASPBERRY-PI-4`, який є копією POSIX-версії з іншими номерами пінів.
- **Видалене в коміті `60f749e` (15.01.2023).** 149 файлів: мережевий шар бібліотеки (сокети для POSIX і lwIP, DNS, UDP клієнт і сервер, DTLS-клієнт на wolfSSL) та приклади для NUCLEO-F429ZI, Raspberry Pi Pico і STM32MP157A-DK1. Усе доступне в історії за адресою `60f749e^`.
- **Гілка `platformio`.** Розійшлася з `main` структурно (`api/` перейменовано на `include/`, wolfSSL прибрано), тому злити її неможливо. Має теги `v1.0.0`–`v1.0.5`. Приклад `coap-server` у ній є заготовкою, яка лише блимає світлодіодом.
- **Відкриті issues.** #1 і #2, обидва про читання за межами буфера в `src/packet.cc`.
- **Прогалини відносно CoAP Smart Home.** В `OptionNumber` у `consts.h` немає Observe (6). Парсер CoRE Link приймає лише атрибути з `=` і значення без лапок, що є числами, тому запис з `;obs` чи `if=core.s` відкидається (R13). Немає допоміжних функцій для Uri-Query і Location-Path (R5, R15). Немає підтримки ESP32. DTLS був лише як клієнт на wolfSSL і його видалено; ролі сервера з PSK немає (R16–R19). SenML-CBOR відсутній (R14).

## Головні проблеми

1. **Це набір будівельних блоків, а не бібліотека.** Логіка сервера лежить у прикладі, клієнт названий шаблоном. Користувач не може написати «зареєструй ресурс `/temp` з обробником GET».
2. **Немає рівня повідомлень.** Не видно окремого модуля для повторних передач CON, дедуплікації та зіставлення запитів і відповідей за токеном. Без цього CoAP поверх UDP ненадійний.
3. **Парсер небезпечний на недовіреному вході.** Для мережевої бібліотеки це блокер.
4. **Збірка відлякує.** wolfSSL і mbedTLS обидва обов'язкові, `-O0 -ggdb` зашиті глобально, немає `install` і `find_package`, тести збираються завжди (тому крос-компіляція неможлива), платформа в `build.sh` обирається редагуванням скрипта, у репозиторії лежать `firmware.bin` і картинки.
5. **У ядрі лишилися платформні залежності.** `htons` у `blockwise.cc`, `rand`/`time` для токенів, глобально ввімкнений spdlog, заголовок wolfSSL у цілі бібліотеки, cJSON у публічному заголовку, зайвий `<iostream>`.
6. **Немає ніші.** Для Linux є libcoap, і конкурувати з ним «ще однією реалізацією» безглуздо.

## Напрям

Позиціонувати libcoapcpp як сучасну C++-бібліотеку CoAP для мікроконтролерів: ядро без вводу-виводу з тонкими адаптерами транспорту. У цій ніші libcoap важкий, а C++-альтернативи або покинуті, або розраховані на Arduino.

Пріоритети задає CoAP Smart Home: до цільових платформ додаються ESP32 і OpenWrt, а DTLS-PSK в обох ролях, Observe з обох боків і потоковий block-wise стають обов'язковими, а не «колись».

## Етапи

| Етап | Результат | Реліз |
|---|---|---|
| 1. Гігієна | Безпечний парсер, fuzzing, CI | `v1.0.6` |
| 2. Ядро без платформних залежностей | `src/` збирається під `arm-none-eabi` без жодного порту | `v1.0.x` |
| 3. Нова структура каталогів | Рушій у бібліотеці, приклади відділені від плат | `v1.1.0` або `v2.0.0` |
| 4. Мікроконтролери | Один `coap-server` під усі плати, `platformio` у `main` | за платами |
| 5. Повноцінний CoAP | Рівень повідомлень, API ресурсів, Observe | перший публічний реліз |
| 6. Помітність і пакування | Реєстр PlatformIO, пакет OpenWrt, тести сумісності | далі |

Етапи 4 і 5 не залежать один від одного, але обидва спираються на етап 3. PoC CoAP Smart Home потребує частини кожного: плати ESP32 з етапу 4 і рівня повідомлень з Observe з етапу 5.

## Відповідність етапам CoAP Smart Home

| Етап системи | Що має дати бібліотека | Вимоги | Етап бібліотеки |
|---|---|---|---|
| 0.1 PoC | `Endpoint` з рівнем повідомлень, серверний і клієнтський API, Observe з обох боків, SenML-JSON; вбудовування в цикл подій gateway на Linux; плата ESP32 | R1–R5, R7–R9, R14, R15, R21 | 1–3, 5; ESP32 з 4 |
| 0.2 Реєстрація й ідентичність | Uri-Query і Location-Path, Block1, CoRE Link з `obs` і значеннями-ptoken, `/.well-known/core` з фільтрацією | R5, R6, R12, R13 | 5 |
| 0.3 Стокова прошивка й SDK | Ресурси, що створюються під час роботи, PlatformIO і компонент ESP-IDF, Pico W | R5, R21, R22 | 4 |
| 0.4 Provisioning і безпека | DTLS-PSK на mbedTLS, сервер із сесіями і `PskStore`, identity в обробниках, обидва напрямки через одне з'єднання | R16–R19 | 4 (DTLS) |
| 0.5 OTA | Потоковий Block2 з обох боків із продовженням і ETag | R10, R11 | 5 |
| 0.7 OpenWrt | Пакет, збірка під musl, бюджет пам'яті | R22, R23 | 6 |
| 1.0 Stable | Сумісність у CI, ліміти, fuzzing усіх парсерів | R24, R25 | 1, 6 |
| після 1.0 | OSCORE і EDHOC, multicast, SenML-CBOR, STM32 | R14, R20, R26 | 6 |

PoC системи не потребує DTLS: він працює через звичайний UDP у межах LAN. Усе до 0.4 можна розробляти на `boards/posix` і ESP32 без шару безпеки.

### Етап 1. Гігієна

Критерій готовності: CI зелений разом із санітайзерами, випущено bugfix-реліз.

#### Регресійні тести (`test/test_packet.cc`)

Буфер класти в `std::vector` точного розміру, інакше ASan не побачить вихід за межі на стековому масиві.

| Вхід (hex) | Очікування |
|---|---|
| `60 00 12 34` | успіх, без опцій і payload (#1) |
| `40 01 00 01 D0` | помилка, обрізане розширення delta (#2) |
| `40 01 00 01 E0 00` | помилка, один байт розширення з двох (#2) |
| `40 01 00 01 05 61` | помилка, довжина опції більша за залишок |
| `40 01 00 01 F0` | помилка, зарезервований nibble 15 |
| `40 01 00 01 FF` | помилка, маркер без payload |
| `40 01 00` | помилка без аварійного завершення |
| `49 01 00 01 …` | помилка, довжина токена 9 |

- [ ] Додати тести з таблиці.

#### Fuzzing

Братися лише після прибирання `assert`, інакше фазер зупиниться на них за першу секунду.

- [ ] Ціль `fuzz/fuzz_packet.cc` з `LLVMFuzzerTestOneInput`: викликає `Packet::parse`, а при успіху `serialize` і повторний `parse` з порівнянням результатів.
- [ ] Окрема опція CMake (наприклад, `COAPCPP_FUZZ`), лише clang, прапорці `-fsanitize=fuzzer,address,undefined`.
- [ ] Початковий корпус у `fuzz/corpus/packet/`: пакети з таблиці вище і `testCoapPacket` з тестів.
- [ ] Кожен знайдений crash-файл перетворити на регресійний тест.
- [ ] Тим самим шаблоном покрити `core_link.cc`, `senml_json.cc`, `uri.cc`, `blockwise.cc`, `base64.cc`: усі вони розбирають недовірений вхід.

#### GitHub Actions

Що треба обійти: `build.sh` має зашитий `TARGET`, тому в CI викликати `cmake` напряму; тести потребують змінної `GTEST_DIR`; немає `enable_testing()` і `add_test`; `testDnsResolver.*` звертається до мережі, тож у CI його слід виключити; wolfSSL і mbedTLS збираються щоразу, тому потрібен `ccache`.

- [ ] **build-test:** матриця gcc і clang на `ubuntu-latest`, `actions/checkout` з `submodules: recursive`.
- [ ] **sanitizers:** clang з `-fsanitize=address,undefined -fno-sanitize-recover=all`.
- [ ] **fuzz-smoke:** кожна fuzz-ціль по 60 секунд на PR; довгий прогін за розкладом раз на тиждень.
- [ ] Після першого зеленого прогону додати бейдж у README.

#### Репозиторій і реліз

- [ ] Додати опис і topics репозиторію (`coap`, `cpp`, `iot`, `embedded`, `lwip`, `dtls`, `stm32`, `raspberry-pi-pico`, `rfc7252`).
- [ ] Перевірити `git branch --contains v1.0.5`. Якщо старі теги досяжні лише з `platformio`, згадати це в описі релізу.
- [ ] Тег на `main` після зеленого CI, з GitHub Release і коротким changelog.

### Етап 2. Ядро без платформних залежностей

Критерій готовності: `src/` збирається компілятором `arm-none-eabi-g++` без жодного порту, а перевірка `grep` з опису архітектури проходить у CI. Публічний API не змінюється.

#### Очистити ядро на місці

- [ ] `blockwise.cc`: записувати порт двома зсувами замість `htons`; прибрати `<arpa/inet.h>` і `lwip/def.h`.
- [ ] `core_link.cc`, `senml_json.cc`: прибрати `<iostream>`, який не використовується, але на мікроконтролері коштує сотні кілобайт flash.
- [ ] `wolfssl_error.*`: винести з цілі бібліотеки; у ядрі він ніде не використовується.
- [ ] `senml_json.h`: замінити `#include "cJSON.h"` на попереднє оголошення `struct cJSON;`.
- [ ] `packet.h`: перенести `DataType` в окремий заголовок, щоб кодек пакетів не тягнув за собою cJSON.
- [ ] `packet.h`: замінити бітові поля під `#pragma pack` масками і зсувами; після цього `is_little_endian_byte_order()` стає непотрібним.
- [ ] `consts.h`: додати `OBSERVE = 6` в `OptionNumber` і допоміжні функції для uint-значень опцій (R15). Це додавання, API лишається сумісним.

#### Інтерфейси портів і порти для POSIX

- [ ] Додати `api/coap/port/` з інтерфейсами `Transport`, `Clock`, `Random`, `Log`.
- [ ] `packet.cc`: токен і Message ID брати з `coap::port::Random` замість `srand(time(nullptr))` і `rand()`. На мікроконтролері без RTC стара схема після кожного перезапуску дає однакову послідовність.
- [ ] Журнал вести через `coap::port::Log`; прибрати глобальний `USE_SPDLOG` і макроси `debug(...)`, `set_level(...)`.
- [ ] Додати `port/net/posix` і `port/os/posix`. Код транспорту взяти зі старого `src/unix/unix_socket.cc`.
- [ ] `Transport::peer_identity` з типовою реалізацією, що повертає `false` (R18); `native_handle()` у `port/net/posix` для власного `poll` застосунку (R3).
- [ ] Додати фальшиві реалізації в `test/mocks/`.
- [ ] Повернути `udp-echo` як димовий тест порту і найкоротший зразок реалізації `Transport`.

#### Переписати CMake

- [ ] Розділити на цілі: ядро `coapcpp` без залежностей і окремі цілі портів.
- [ ] Опції `COAPCPP_WITH_FORMATS`, `COAPCPP_WITH_DTLS`, `COAPCPP_BUILD_TESTS`; TLS-бекенд обирається опцією, spdlog і cJSON стають необов'язковими.
- [ ] Прибрати глобальні `CMAKE_CXX_FLAGS` з `-O0 -ggdb -frtti`, глобальні `include_directories` і `remove_definitions(-DNDEBUG)`.
- [ ] Сторонні бібліотеки підключати через `find_package` або `FetchContent` і лише за потребою.
- [ ] Додати `install` і експорт CMake-пакета, `enable_testing()` і `add_test`.
- [ ] Файл `.pc` для pkg-config, `BUILD_SHARED_LIBS`, системний mbedTLS через `find_package` з `third-party/` як запасним варіантом (R22).
- [ ] Додати тулчейн-файли в `cmake/toolchains/` і перевірку крос-компіляції ядра в CI.
- [ ] Винести `firmware.bin` і картинки з репозиторію; додати `.pio/` у `.gitignore`.

### Етап 3. Нова структура каталогів

Критерій готовності: `examples/coap-server` збирається для `boards/posix` наборами `posix`, `raspberry-pi` і `stm32mp157a-dk1`. Цей етап змінює шляхи заголовків і простори імен.

- [ ] Перенести заголовки в `api/coap/`; старі шляхи на кшталт `#include "packet.h"` тимчасово підтримати заголовками-перенаправленнями.
- [ ] Перенести `Buffer`, `ConnectionType` і `CoapStatus` у `namespace coap`: імена `TCP` і `UDP` у глобальному просторі ризикують зіткнутися з макросами HAL чи SDK.
- [ ] Перенести рушій з `examples/POSIX/common` у `src/`; цикли `select` замінити на `coap::run` з `port/net/posix`.
- [ ] Розділити приклади і плати: `examples/coap-server` і `examples/coap-client` з `app_main`, `boards/posix`, `boards/posix/gpio`.
- [ ] Замінити вибір датчиків через `#ifdef __arm__` на опцію `SENSORS=stub|gpio`, а wiringPi на libgpiod.
- [ ] Описати набори збирання в `CMakePresets.json`; `build.sh` звести до `cmake --preset <назва>`.
- [ ] Перенести README для STM32MP157A-DK1 у `boards/posix/docs/stm32mp157a-dk1.md`.
- [ ] Видалити `examples/POSIX` і `examples/RASPBERRY-PI-4`.

Куди переїжджають наявні файли:

| Зараз | Стане | Зміни по дорозі |
|---|---|---|
| `api/*.h` | `api/coap/*.h` | `namespace coap` |
| `src/wolfssl_error.*` | `port/net/dtls/wolfssl/` | немає |
| `examples/POSIX/common/coap_server.*`, `coap_client.*`, `packet_helper.*` | `src/server.cc`, `client.cc`, `packet_helper.cc`; `api/coap/server.h`, `client.h` | `namespace posix` → `coap`; роздача файлів через зворотний виклик замість `<fstream>`; `htons`/`htonl` → зсуви |
| `examples/POSIX/common/sensor.*`, `endpoint.*`, `sensor_stubs.*` | `examples/coap-server/` | час через `coap::port::Clock` |
| `examples/POSIX/common/trace.h` | `boards/posix/` | залежить від `<iostream>` |
| `examples/POSIX/coap-*/main.cc` | `boards/posix/` | цикл `select` → `coap::run`; логіка прикладу → `app_main` |
| `examples/RASPBERRY-PI-4/coap-server/main.cc` | видалити | дублікат; різниця лише в пінах |
| `examples/RASPBERRY-PI-4/common/DHT11.*`, `RGB_LED.*` | `boards/posix/gpio/` | wiringPi → libgpiod |

### Етап 4. Мікроконтролери

Критерій готовності: `examples/coap-server/` збирається без змін під усі плати, а каталог кожної плати містить лише ініціалізацію заліза, створення портів і реєстрацію своїх ресурсів.

Першим іде ESP32: це MVP-платформа CoAP Smart Home. Далі Pico W, потрібний для 1.0 системи. Решта — за наявним залізом:

| Порядок | Плата | Що для цього потрібно |
|---|---|---|
| 1 | `esp32-devkitc` | `port/net/lwip` (сокети), `port/os/freertos` (лише `clock.cc`), `port/hal/esp32`; компонент ESP-IDF |
| 2 | `pico-w` | `port/hal/pico`; ADXL345 як ресурс |
| 3 | `nucleo-f429zi` | `port/net/lwip` (сокети), `port/os/freertos`, `port/hal/stm32`; BSP із `60f749e^`; `/rtc` як ресурс |
| 4 | `pico-spi` (прошивка і шлюз) | спільний кодек кадру в `boards/pico-spi/common` |
| 5 | `nucleo-h743zi` | новий BSP, решта без змін |
| 6 | `pico-eth` | драйвер RMII на PIO як `netif` lwIP; портів не додає |
| окремо | `dtls-echo-server` | `port/net/dtls/mbedtls`, далі `wolfssl`; потрібен для 0.4 системи |

#### ESP32

- [ ] `port/hal/esp32`: `Random` на `esp_fill_random`, `Clock` на `esp_timer_get_time`.
- [ ] `boards/esp32-devkitc`: проєкт ESP-IDF із тим самим `examples/coap-server`. Сокети lwIP з ESP-IDF працюють через `udp_transport_sockets.cc`; `heap.cc` не потрібен.
- [ ] Компонент ESP-IDF: `idf_component.yml` і гілка `idf_component_register` у кореневому `CMakeLists.txt` (R22). Перенесено сюди з етапу 6, бо він потрібен для PoC системи.
- [ ] Збірка під PlatformIO `espressif32` з фреймворками `espidf` і `arduino`: SmartNode SDK ще не обрав фреймворк.
- [ ] Збірка в CI під ESP32 (Xtensa) і ESP32-C3 (RISC-V).
- [ ] Звірити `BUFFER_SIZE` і стек задачі з RAM ESP32-C3 і MTU Wi-Fi.

#### NUCLEO-F429ZI

Найцінніша частина видаленого коду: робочий BSP, якого в `main` немає.

- [ ] Відновити без змін у `boards/nucleo-f429zi/bsp/`: `Core/Src`, `LwIP/App`, `LwIP/Target/ethernetif.c`, `lwipopts.h`, `FreeRTOSConfig.h`, `stm32f4xx_hal_conf.h`, `flash.sh`, `monitor.sh`, README.
- [ ] `main.cc`: ініціалізацію заліза і задачу DHCP лишити; задачу UDP-сервера замінити на таку, що створює порти і викликає `coap::run`.
- [ ] Команди `get rtc` і `set rtc` зі старого `command.cc` перетворити на `GET`/`PUT /rtc` із SenML-JSON.
- [ ] Ініціалізувати RNG у BSP, щоб працював `port/hal/stm32`.
- [ ] Звірити розмір `Buffer` (1600 байт) із купою lwIP і стеком задачі; старий сервер мав 256 байт на прийом і 512 на передачу.
- [ ] Сім `.make`-скриптів замінити одним `CMakeLists.txt` плати і тулчейн-файлом.
- [ ] Не повертати в git `NUCLEO-F429ZI.jpeg` і `firmware.bin`.

#### Pico W і Pico 2 W

- [ ] Wi-Fi через `cyw43` і lwIP із Pico SDK, той самий `examples/coap-server`.
- [ ] Зі старого коду взяти `FreeRTOSConfig.h`, скрипти імпорту SDK, `flash.sh`, `monitor.sh` і драйвер `ADXL345.*`, який стає ресурсом `/sensors/ADXL345`.
- [ ] Під FreeRTOS використати `udp_transport_sockets.cc`, без ОС `udp_transport_raw.cc`.

#### Pico через SPI

- [ ] Кодек кадру, продубльований зараз у двох копіях `pico_protocol.h`, звести в `boards/pico-spi/common/spi_framing.*`.
- [ ] Прошивка: `SpiFramedTransport` поверх `hardware_spi`.
- [ ] Шлюз: міст UDP ↔ SPI на Linux через spidev.

#### NUCLEO-H743ZI і Pico з Ethernet на PIO

- [ ] `nucleo-h743zi`: копія `nucleo-f429zi` із BSP, згенерованим для H7. Якщо `main.cc`, ресурси і порти лишаються тими самими, поділ на порти працює.
- [ ] `pico-eth`: програма PIO, драйвер `netif`, робота з PHY через MDIO. Сторонній драйвер, якщо він використовується, класти в `third-party/`.

#### DTLS

mbedTLS іде першим: саме цей варіант потрібен на ESP32, OpenWrt, STM32 і Pico (R16).

- [ ] `port/net/dtls/mbedtls`: за зразком `tls-client2.cc`, але DTLS 1.2, PSK і `TLS_PSK_WITH_AES_128_CCM_8`.
- [ ] Роль сервера: сесії за адресою співрозмовника, cookies, тайм-аут неактивної сесії, обмеження кількості сесій, ключ через `PskStore` (R17).
- [ ] `peer_identity` для обробників запитів (R18); запити в обох напрямках через одне з'єднання (R19).
- [ ] Сумісність із `coap-client -k` і `coap-server -k` з libcoap.
- [ ] `port/net/dtls/wolfssl`: рукостискання взяти зі старих `unix_dtls_client.cc`, `dtls_server.cc`, `tls-client1.cc`.
- [ ] Замінити вбудований кореневий сертифікат у `my_certificates.cc`: схоже, це DST Root CA X3, строк дії якого сплив у вересні 2021 року.
- [ ] Повернути `dtls-echo-server` у `boards/posix/`.

#### PlatformIO

- [ ] Додати `library.json` у корінь `main` із `srcFilter` на `src/` і потрібні `port/`; платформи `espressif32`, `raspberrypi`, `ststm32`.
- [ ] Перенести приклад із гілки `platformio` і довести його до справжнього CoAP-сервера.
- [ ] Закрити гілку `platformio`.

#### Як діставати файли з історії

Файли беруться з батьківського коміту, без скасування самого `60f749e`:

```sh
# подивитися
git show 60f749e^:examples/NUCLEO-F429ZI/udp-server/Core/Src/main.cc

# відновити каталог у робоче дерево й одразу перенести
git checkout 60f749e^ -- examples/NUCLEO-F429ZI
git mv examples/NUCLEO-F429ZI/udp-server/Core boards/nucleo-f429zi/bsp/Core
```

`git revert 60f749e` не підходить: він поверне і старий мережевий шар, а `CMakeLists.txt`, `build.sh` і `src/utils.h` відтоді змінилися.

Куди лягає старий код:

| Видалене | Нове місце |
|---|---|
| `src/unix/unix_socket.cc`, `unix_udp_client.cc`, `unix_udp_server.cc` | `port/net/posix/udp_transport.cc` |
| `src/lwip/lwip_socket.cc` | `port/net/lwip/udp_transport_sockets.cc` |
| `src/unix/unix_dns_resolver.cc`, `src/lwip/lwip_dns_resolver.cc` | `port/net/posix/resolver.cc`, `port/net/lwip/resolver.cc` |
| `src/unix/unix_dtls_client.cc`, `examples/POSIX/common/dtls_server.cc` | `port/net/dtls/wolfssl/dtls_transport.cc` |
| `examples/POSIX/tls-client2/tls-client2.cc`, `mbedtls_error.*` | `port/net/dtls/mbedtls/` |
| `test/test_socket.cc`, `test_dns_resolver.cc` | `port/net/posix/test/` |
| `api/socket.h`, `connection.h`, `dns_resolver.h`, `endpoint.h`, `src/unix/unix_endpoint.*` | не відновлювати |
| `examples/POSIX/tcp-client`, `tls-client1`, `tls-client2` | не відновлювати як приклади: це HTTP-клієнти |

### Етап 5. Повноцінний CoAP

Критерій готовності: сервер з одним ресурсом і клієнт до нього вміщаються в приклад на 30 рядків, а сценарії CoAP Smart Home з таблиці нижче проходять на `loopback_transport` з фальшивим годинником.

#### Рівень повідомлень і кінцева точка

- [ ] CON/NON/ACK/RST, повторні передачі з backoff, дедуплікація, piggybacked і відкладені відповіді, `NSTART` (R1).
- [ ] `Endpoint`: сервер і клієнт на одному транспорті; багато транзакцій, зіставлених за токеном і адресою (R2, R4). `server` і `client` з етапу 3 лишаються тонкими обгортками.
- [ ] `next_timeout_ms()` і `tick(now)`; `coap::run` будується на них (R3).
- [ ] Ліміти в `config.h` і поведінка при їх вичерпанні (R24).

#### Серверний і клієнтський API

- [ ] Маршрутизація за Uri-Path зі змінним сегментом, обробники методів, ресурси, що додаються під час роботи, Uri-Query, Location-Path, відкладена відповідь (R5).
- [ ] Автоматичний `/.well-known/core` з фільтрацією `?rt=` (R6).
- [ ] Клієнтський API: `get`, `post`, `put`, `delete` і довільний запит, зі зворотним викликом і скасуванням (R7).

#### Observe (RFC 7641)

- [ ] Серверний бік: реєстрація, скасування, порядковий номер, CON-нотифікація раз на 24 години (R8).
- [ ] Клієнтський бік: одна підписка на ресурс, відкидання застарілих нотифікацій, повторна підписка, зворотний виклик на втрату підписки (R9).

#### Потоковий block-wise

- [ ] Сервер Block2 зі зворотного виклику `(offset, size)`, Size2, ETag, будь-який блок на запит (R10).
- [ ] Клієнт Block2 зі зворотним викликом на кожен блок, початком із довільного блока, перериванням при зміні ETag (R11).
- [ ] Block1 з обох боків з обмеженням розміру збирання (R12).

#### Формати

- [ ] CoRE Link: атрибути без значення, значення-ptoken, кілька значень в одному атрибуті (R13). Додати seed-и для цих випадків у `fuzz/corpus/core_link/` і регресійні тести.
- [ ] SenML-JSON: перевірити тестами кожне поле з R14 на відповідність RFC 8428.

#### Сценарії CoAP Smart Home

| Сценарій | Що перевіряється |
|---|---|
| Реєстрація | `POST /rd?ep=…&lt=…` з payload у link-format, довшим за один блок → `2.01 Created` з Location-Path; оновлення `POST /rd/{id}` |
| Observe | підписка, нотифікації, старіша нотифікація після новішої відкидається, повторна підписка після втрати |
| Керування | `PUT /actuators/relay` з відкладеною відповіддю |
| OTA | завантаження Block2 на 1 МБ з втратою 10% датаграм; завантаження продовжується з потрібного блока; зміна ETag його перериває |

### Етап 6. Помітність і пакування

- [ ] Опублікувати бібліотеку в реєстрі PlatformIO і в ESP-IDF Component Registry.
- [ ] Пакет OpenWrt для 0.7 системи: `packaging/openwrt/Makefile`, далі подання в `openwrt/packages`. Врахувати різницю APK/OPKG між версіями OpenWrt (R22).
- [ ] Збірка в CI через OpenWrt SDK під musl для однієї цілі MIPS і однієї ARM (R21).
- [ ] Вимірювати в CI розмір бібліотеки без символів і RSS прикладу та порівнювати з бюджетом (R23).
- [ ] Перевіряти сумісність із libcoap, aiocoap і Californium у CI через звичайний UDP і DTLS-PSK (R25).
- [ ] CBOR і SenML-CBOR (R14).
- [ ] Multicast для резервного пошуку (R26).
- [ ] OSCORE (RFC 8613) і EDHOC (RFC 9528): CoAP Smart Home потребує їх після свого 1.0 (R20). CoAP over TCP (RFC 8323) лишається необов'язковим.

### Стратегічна опція: клієнт LwM2M

SenML, CoRE Link і роздача прошивки через block-wise уже є. Легкий C++-клієнт LwM2M для мікроконтролерів дав би проєкту практичну цінність, але це великий обсяг, і братися варто лише після етапу 5. CoAP Smart Home запозичує модель об'єктів LwM2M, але протоколу не реалізує, тож LwM2M лишається опцією, а не вимогою.

## Мінімум для першого публічного релізу

Етапи 1–3 і 5, а також приклад на 30 рядків у README: сервер з одним ресурсом і клієнт до нього.

## Мінімум для PoC CoAP Smart Home

Етапи 1–3; з етапу 4 — плата `esp32-devkitc`; з етапу 5 — рівень повідомлень, `Endpoint`, серверний і клієнтський API, Observe з обох боків і SenML-JSON. DTLS і потоковий block-wise поки не потрібні.

## Версії

- `v1.0.6`: етап 1, виправлення парсера без зміни API.
- `v1.0.x`: етап 2, публічний API не змінюється.
- `v1.1.0` або `v2.0.0`: етап 3, змінюються шляхи заголовків і простори імен.

## Відкриті питання

1. **Номер наступного тега.** План передбачає `v1.0.6` на `main`. Якщо цей номер уже зайнятий релізом на гілці `platformio`, bugfix-реліз отримує наступний вільний.
2. **Стандарт C++.** Інтерфейси портів сумісні з C++11, але серед побажань до збирання є перехід на C++17 заради `string_view` і `optional`. Треба вирішити, чи лишається C++11 мінімальним для ядра.
3. **Динамічна пам'ять.** Ніша передбачає ядро без купи, але зараз воно використовує `std::string`, `std::vector` і `new[]`, і жоден етап цього не змінює. Потрібен окремий етап або чесне формулювання ніші. Для CoAP Smart Home (Linux, OpenWrt, ESP32, Pico W) купа прийнятна, тож питання може почекати до STM32.
4. **Порядок етапів 4 і 5.** Плати дають видимий результат, рівень повідомлень дає справжній CoAP. Для CoAP Smart Home порядок визначено: плата ESP32 ведеться паралельно з етапом 5, решта плат — після нього.
5. **Драйвер RMII на PIO.** Придатність сторонніх драйверів для RP2350 не перевірялася; на RP2040 запасу частоти для 100 Мбіт/с може не вистачити.
6. **Клієнт RD у бібліотеці.** Реєстрація з оновленням кожні `lt` секунд (RFC 9176) є загальним механізмом. Вона може бути необов'язковим модулем бібліотеки або частиною SmartNode SDK. У RD-сервері бібліотека участі не бере.
7. **CBOR-кодек.** cJSON із CBOR не працює. Для SenML-CBOR потрібен або власний мінімальний кодек, або стороння бібліотека (tinycbor, QCBOR).
8. **Фреймворк SmartNode SDK.** API SDK в описі системи схожий на Arduino (`setup`/`loop`), а OTA з rollback є рідним для ESP-IDF. Поки вибір не зроблено, бібліотека збирається під обидва фреймворки PlatformIO.
9. **DTLS-сесія і перезапуск gateway.** З одним з'єднанням для обох напрямків (R19) після перезапуску gateway вузол має помітити втрату сесії і повторити рукостискання. Треба вирішити, чи це робота бібліотеки (зворотний виклик на DTLS alert або тайм-аут), чи клієнта RD.
