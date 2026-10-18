# Поетапна заміна компонентів STL, що використовують heap, з переходом на C++20

Цільові платформи: STM32F4/H7 (Cortex-M4F/M7), Raspberry Pi Pico / Pico 2 (RP2040/RP2350, Pico C/C++ SDK).
Тулчейн: `arm-none-eabi-gcc` (за потреби — Clang), стандарт C++20, `-fno-exceptions -fno-rtti`.

---

## Зміст

1. [Мета і принципи](#1-мета-і-принципи)
2. [Таблиця відповідностей](#2-таблиця-відповідностей)
3. [Етап 0. Інвентаризація алокацій](#етап-0-інвентаризація-алокацій)
4. [Етап 1. Перехід тулчейну на C++20](#етап-1-перехід-тулчейну-на-c20)
5. [Етап 2. Базова інфраструктура](#етап-2-базова-інфраструктура)
6. [Етап 3. Рядки](#етап-3-рядки)
7. [Етап 4. Послідовні контейнери](#етап-4-послідовні-контейнери)
8. [Етап 5. Асоціативні контейнери](#етап-5-асоціативні-контейнери)
9. [Етап 6. Володіння об'єктами і розумні вказівники](#етап-6-володіння-обєктами-і-розумні-вказівники)
10. [Етап 7. Колбеки](#етап-7-колбеки)
11. [Етап 8. Поліморфізм](#етап-8-поліморфізм)
12. [Етап 9. Відмова від винятків](#етап-9-відмова-від-винятків)
13. [Етап 10. Залишки динаміки: `std::pmr`](#етап-10-залишки-динаміки-stdpmr)
14. [Етап 11. Контроль у CI](#етап-11-контроль-у-ci)
15. [Примітки для STM32F4/H7 і Pico SDK](#примітки-для-stm32f4h7-і-pico-sdk)
16. [Підсумковий чекліст](#підсумковий-чекліст)

---

## 1. Мета і принципи

**Мета:** прошивка (або бібліотека) не викликає `malloc`, `free`, `operator new`, `operator delete` і не використовує винятки. Увесь обсяг RAM відомий після компонування з map-файлу.

**Принципи:**

- **Користувач володіє пам'яттю.** Буфери та ємності передаються ззовні або задаються параметрами шаблону.
- **Фіксована ємність замість росту.** Переповнення — це повернена помилка, а не алокація і не виняток.
- **Zero-copy.** Дані читаються через `std::span` / `std::string_view` над наявними буферами.
- **Помилки як значення.** `expected`-подібний тип замість `throw`.
- **Все, що можна, — на етапі компіляції.** `constexpr`-таблиці у Flash замість побудови структур у RAM.

**Правило міграції:** після кожного етапу проєкт збирається, тести проходять, а кількість алокацій (див. етап 0) не збільшується. Етапи можна виконувати по модулях: спершу ядро протоколу, потім решта.

---

## 2. Таблиця відповідностей

| Було (використовує heap) | Стало (без heap) | Етап |
|---|---|---|
| `std::string` (власник) | `etl::string<N>` | 3 |
| `const std::string&` (параметр) | `std::string_view` | 3 |
| `std::to_string`, `std::stoi`, `sprintf` | `std::to_chars`, `std::from_chars` | 3 |
| `std::vector<T>` (відомий максимум) | `std::array<T, N>`, `static_vector<T, N>` / `etl::vector<T, N>` | 4 |
| `const std::vector<T>&` (параметр) | `std::span<const T>` | 4 |
| `std::deque`, `std::queue` | кільцевий буфер `RingBuffer<T, N>` | 4 |
| `std::list` | інтрузивний список / `etl::list<T, N>` | 4 |
| `std::map`, `std::set` | відсортований `static_vector` + `lower_bound` / `etl::flat_map<K, V, N>` | 5 |
| `std::unordered_map` | `etl::unordered_map<K, V, N>` або `constexpr`-таблиця | 5 |
| `std::make_unique`, `new T` | статичний об'єкт або `ObjectPool<T, N>` | 6 |
| `std::shared_ptr` | однозначний власник; за потреби — пул з інтрузивним лічильником | 6 |
| `std::function` | шаблонний параметр, `function_ref`, `etl::delegate`, `inplace_function` | 7 |
| `std::unique_ptr<Interface>` + `make_unique<Impl>` | статичний `Impl` + посилання на інтерфейс або concepts | 8 |
| `throw` / `try` / `catch` | `Result<T>` (`etl::expected`, `tl::expected`, C++23 `std::expected`) | 9 |
| `optional::value()`, `std::get<T>(variant)`, `vector::at()` | `*opt`, `std::get_if`, перевірка індексу | 9 |
| Залишкові динамічні структури | `std::pmr::*` поверх статичної арени | 10 |

> Про майбутні стандарти: у C++26 з'являються `std::inplace_vector` і `std::function_ref`. Код, написаний з ETL або власними аналогами, потім легко перевести на них.

---

## Етап 0. Інвентаризація алокацій

Перш ніж щось міняти, треба знати, **де** і **скільки** алокацій відбувається.

### 0.1. Статичний пошук у коді

```bash
grep -rnE 'std::(vector|string|map|set|unordered_|list|deque|queue|function|shared_ptr|make_unique|make_shared|any)\b|\bnew\b|\bthrow\b|\btry\b' src/ include/
```

Результат — список файлів і рядків, який стає беклогом міграції.

### 0.2. Символи в зібраній прошивці

```bash
arm-none-eabi-nm -C build/firmware.elf | grep -E 'malloc|free|realloc|operator new|operator delete|__cxa_throw|__cxa_allocate_exception'
```

А в map-файлі (`-Wl,-Map=firmware.map`) видно, **який об'єктний файл** затягнув ці символи.

### 0.3. Лічильник алокацій у тестах на хості

Збірка тестів під Linux з перевизначеним `operator new`:

```cpp
// tests/alloc_counter.cpp
#include <cstdlib>
#include <cstddef>
#include <new>

std::size_t g_alloc_count = 0;

void* operator new(std::size_t n) {
    ++g_alloc_count;
    if (void* p = std::malloc(n)) return p;
    std::abort();
}
void* operator new[](std::size_t n) { return ::operator new(n); }
void operator delete(void* p) noexcept { std::free(p); }
void operator delete[](void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }
void operator delete[](void* p, std::size_t) noexcept { std::free(p); }
```

```cpp
// у тесті
extern std::size_t g_alloc_count;

TEST(Coap, ParseAndRespondWithoutHeap) {
    g_alloc_count = 0;
    run_request_response_scenario();
    EXPECT_EQ(g_alloc_count, 0u);
}
```

**Критерій завершення етапу:** є список місць алокацій і тест, що фіксує їх поточну кількість.

---

## Етап 1. Перехід тулчейну на C++20

На цьому етапі винятки ще **не вимикаємо** — спершу лише стандарт.

### 1.1. CMake

```cmake
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)        # -std=c++20, а не gnu++20

target_compile_options(firmware PRIVATE
    -ffunction-sections -fdata-sections
    -Os
    -Wall -Wextra
)
target_link_options(firmware PRIVATE
    -Wl,--gc-sections
    -Wl,-Map=${CMAKE_BINARY_DIR}/firmware.map
)
```

### 1.2. Перевірка версії компілятора

C++20-можливості, на які спирається цей документ (concepts, `std::span`, `<bit>`, `<ranges>`, класові типи як параметри шаблону), доступні в GCC 10–11 і новіших.

```bash
arm-none-eabi-gcc --version
```

### 1.3. Типові проблеми після перемикання

| Симптом | Причина | Рішення |
|---|---|---|
| Помилки з `register` у заголовках CMSIS | ключове слово видалено в C++17 | оновити CMSIS до актуальної версії |
| Багато попереджень `-Wvolatile` на `REG \|= x` | у C++20 складені операції над `volatile` були оголошені застарілими | новіші GCC не попереджають для `\|=`, `&=`, `^=`; на старших — `-Wno-volatile` |
| `char8_t` ламає `u8"..."` | у C++20 `u8`-літерали мають тип `const char8_t*` | прибрати `u8` або `-fno-char8_t` тимчасово |
| Неоднозначність `operator==` | переписані порівняння в C++20 | зробити оператори `const` і симетричними |

**Критерій завершення:** проєкт збирається з `-std=c++20`, поведінка не змінилась, тести зелені.

---

## Етап 2. Базова інфраструктура

Перш ніж міняти контейнери, потрібні типи, на які вони спиратимуться.

### 2.1. Бібліотека контейнерів

Рекомендовано підключити **ETL (Embedded Template Library)**: `etl::vector`, `etl::string`, `etl::flat_map`, `etl::delegate`, `etl::expected`, `etl::pool` тощо. Вона не використовує heap і розрахована на `-fno-exceptions`.

```cmake
include(FetchContent)
FetchContent_Declare(etl GIT_REPOSITORY https://github.com/ETLCPP/etl GIT_TAG <версія>)
FetchContent_MakeAvailable(etl)
target_link_libraries(firmware PRIVATE etl::etl)
```

Нижче наведено й мінімальні власні реалізації — на випадок, якщо зовнішня залежність небажана або для розуміння принципу.

### 2.2. Тип помилок

```cpp
// core/result.hpp
#pragma once
#include <cstdint>
#include <etl/expected.h>

enum class Error : std::uint8_t {
    BufferTooSmall,
    Malformed,
    NoSlot,        // вичерпано фіксовану ємність
    NotFound,
    Timeout,
    Io,
};

template <typename T>
using Result = etl::expected<T, Error>;

using Status = etl::expected<void, Error>;
```

### 2.3. Рядок як параметр шаблону (C++20)

```cpp
// core/fixed_string.hpp
#pragma once
#include <algorithm>
#include <cstddef>
#include <string_view>

template <std::size_t N>
struct fixed_string {
    char data[N]{};
    constexpr fixed_string(const char (&s)[N]) { std::copy_n(s, N, data); }
    constexpr std::string_view view() const { return {data, N - 1}; }
};
```

**Критерій завершення:** `Result`, ETL (або власні аналоги) доступні в проєкті й покриті тестами.

---

## Етап 3. Рядки

### 3.1. Параметри: `const std::string&` → `std::string_view`

```cpp
// було
bool match(const std::string& path);
// стало
bool match(std::string_view path);
```

> Обережно з часом життя: `string_view` нічим не володіє. Не зберігайте його довше, ніж живе буфер, на який він вказує.

### 3.2. Рядки-власники: `std::string` → `etl::string<N>`

```cpp
// було
std::string device_name;
// стало
etl::string<32> device_name;   // 32 символи, пам'ять усередині об'єкта
```

Переповнення не алокує, а обрізає рядок (поведінку можна перевірити через `is_truncated()`).

### 3.3. Форматування: `std::to_string` / `sprintf` → `std::to_chars`

```cpp
// було
std::string path = "/sensors/" + std::to_string(id);

// стало
#include <charconv>

Result<std::string_view> make_path(std::span<char> buf, unsigned id) {
    constexpr std::string_view prefix = "/sensors/";
    if (buf.size() < prefix.size()) return etl::unexpected(Error::BufferTooSmall);

    char* it = std::copy(prefix.begin(), prefix.end(), buf.data());
    auto [end, ec] = std::to_chars(it, buf.data() + buf.size(), id);
    if (ec != std::errc{}) return etl::unexpected(Error::BufferTooSmall);

    return std::string_view{buf.data(), static_cast<std::size_t>(end - buf.data())};
}
```

### 3.4. Розбір: `std::stoi` → `std::from_chars`

```cpp
Result<int> parse_int(std::string_view s) {
    int value{};
    auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), value);
    if (ec != std::errc{} || ptr != s.data() + s.size()) return etl::unexpected(Error::Malformed);
    return value;
}
```

`std::stoi` кидає винятки, `from_chars` — ні.

> Форматування `float` через `to_chars` або `printf` з плаваючою комою помітно збільшує розмір коду. Якщо можливо, передавайте значення як ціле з фіксованим масштабом (наприклад, сотні градуса).

**Критерій завершення:** у модулі немає `std::string`, `to_string`, `stoi`, `sprintf`.

---

## Етап 4. Послідовні контейнери

### 4.1. Параметри: `const std::vector<T>&` → `std::span<const T>`

```cpp
// було
void send(const std::vector<std::uint8_t>& data);
// стало
void send(std::span<const std::byte> data);
```

`span` приймає і `std::array`, і `static_vector`, і C-масив, і частину буфера.

### 4.2. Розмір відомий при компіляції: `std::array`

```cpp
// було
std::vector<std::byte> rx_buf(256);
// стало
std::array<std::byte, 256> rx_buf{};
```

### 4.3. Змінна кількість до максимуму: `static_vector<T, N>`

Використовуйте `etl::vector<T, N>`. Мінімальна власна версія для розуміння:

```cpp
// core/static_vector.hpp
#pragma once
#include <cstddef>
#include <new>
#include <utility>

template <typename T, std::size_t N>
class static_vector {
    static_assert(N > 0);
    alignas(T) std::byte storage_[N * sizeof(T)];
    std::size_t size_ = 0;

    T* slot(std::size_t i) noexcept {
        return std::launder(reinterpret_cast<T*>(storage_) + i);
    }

public:
    static_vector() = default;
    static_vector(const static_vector&) = delete;            // спрощення: див. ETL
    static_vector& operator=(const static_vector&) = delete;
    ~static_vector() { clear(); }

    template <typename... Args>
    T* emplace_back(Args&&... args) {
        if (size_ == N) return nullptr;                        // замість throw / алокації
        T* p = ::new (static_cast<void*>(slot(size_))) T(std::forward<Args>(args)...);
        ++size_;
        return p;
    }

    void pop_back() noexcept { slot(--size_)->~T(); }
    void clear() noexcept { while (size_) pop_back(); }

    T& operator[](std::size_t i) noexcept { return *slot(i); }
    T* begin() noexcept { return slot(0); }
    T* end() noexcept { return slot(size_); }

    std::size_t size() const noexcept { return size_; }
    bool full() const noexcept { return size_ == N; }
    static constexpr std::size_t capacity() noexcept { return N; }
};
```

```cpp
// було
std::vector<Observer> observers;
observers.push_back(obs);

// стало
static_vector<Observer, 4> observers;
if (!observers.emplace_back(obs)) {
    return etl::unexpected(Error::NoSlot);
}
```

### 4.4. Черги: `std::deque` / `std::queue` → кільцевий буфер

```cpp
// core/ring_buffer.hpp
#pragma once
#include <array>
#include <cstddef>
#include <optional>

template <typename T, std::size_t N>   // T має бути default-constructible
class RingBuffer {
    static_assert(N > 0);
    std::array<T, N> buf_{};
    std::size_t head_ = 0;
    std::size_t size_ = 0;

public:
    bool push(const T& v) noexcept {
        if (size_ == N) return false;
        buf_[(head_ + size_) % N] = v;
        ++size_;
        return true;
    }

    // варіант «перезаписати найстаріший», зручно для журналів подій
    void push_overwrite(const T& v) noexcept {
        if (size_ == N) { head_ = (head_ + 1) % N; --size_; }
        push(v);
    }

    std::optional<T> pop() noexcept {
        if (size_ == 0) return std::nullopt;
        T v = buf_[head_];
        head_ = (head_ + 1) % N;
        --size_;
        return v;
    }

    std::size_t size() const noexcept { return size_; }
    bool empty() const noexcept { return size_ == 0; }
};
```

> Якщо `N` — степінь двійки, `% N` компілятор замінить на побітове `&`. Для обміну між перериванням і основним циклом потрібна lock-free версія (одиночний виробник / одиночний споживач з `std::atomic`) або критична секція.

### 4.5. `std::list` → інтрузивний список

Вузол списку вбудовується в сам об'єкт, тому вставка не потребує пам'яті:

```cpp
struct PendingTx {
    PendingTx* next = nullptr;   // інтрузивне посилання
    std::uint16_t message_id;
    // ...
};
```

Готові варіанти: `etl::intrusive_list`, `etl::list<T, N>`.

**Критерій завершення:** у модулі немає `vector`, `deque`, `queue`, `list`; переповнення обробляється явно.

---

## Етап 5. Асоціативні контейнери

### 5.1. Таблиці, відомі при компіляції → `constexpr`-масив у Flash

Найкращий варіант для маршрутизації CoAP-ресурсів, кодів помилок, конфігурацій:

```cpp
struct Request; struct Response;
using Handler = void (*)(const Request&, Response&);

void on_cfg(const Request&, Response&);
void on_motion(const Request&, Response&);

struct Route {
    std::string_view path;
    Handler handler;
};

constexpr auto routes = std::to_array<Route>({
    {"/cfg/period", &on_cfg},
    {"/s/motion",   &on_motion},
});

// C++20: алгоритми ranges є constexpr — сортування перевіряється компілятором
static_assert(std::ranges::is_sorted(routes, {}, &Route::path),
              "routes must be sorted by path");

constexpr const Route* find_route(std::string_view path) {
    auto it = std::ranges::lower_bound(routes, path, {}, &Route::path);
    return (it != routes.end() && it->path == path) ? &*it : nullptr;
}
```

Таблиця лежить у `.rodata` (Flash), RAM не займає, пошук — бінарний.

### 5.2. Таблиці, що змінюються під час роботи → flat map

```cpp
// було
std::map<std::uint16_t, Transaction> pending;

// стало
etl::flat_map<std::uint16_t, Transaction, 8> pending;   // відсортований масив на 8 записів
if (pending.full()) return etl::unexpected(Error::NoSlot);
pending.insert({mid, tx});
```

Для малих `N` (до кількох десятків) відсортований масив або навіть лінійний пошук швидший за дерево і займає менше пам'яті.

### 5.3. `std::unordered_map`

- малий розмір → лінійний пошук у `static_vector` або `flat_map`;
- великий розмір → `etl::unordered_map<K, V, N>` (фіксована кількість вузлів і кошиків).

**Критерій завершення:** у модулі немає `map`, `set`, `unordered_*`.

---

## Етап 6. Володіння об'єктами і розумні вказівники

### 6.1. Об'єкти, що живуть увесь час роботи → статичні

```cpp
// було
auto radio = std::make_unique<LoraRadio>(cfg);

// стало
static LoraRadio radio{cfg};      // .bss / .data, розмір відомий при компонуванні
```

### 6.2. Об'єкти з довільним часом життя → пул

```cpp
// core/object_pool.hpp
#pragma once
#include <array>
#include <cstddef>
#include <new>
#include <utility>

template <typename T, std::size_t N>
class ObjectPool {
    union Slot {
        Slot* next;
        alignas(T) std::byte storage[sizeof(T)];
        Slot() : next(nullptr) {}
    };

    std::array<Slot, N> slots_;
    Slot* free_ = nullptr;

public:
    using value_type = T;

    ObjectPool() {
        for (auto& s : slots_) { s.next = free_; free_ = &s; }
    }
    ObjectPool(const ObjectPool&) = delete;
    ObjectPool& operator=(const ObjectPool&) = delete;

    template <typename... Args>
    T* create(Args&&... args) {
        if (!free_) return nullptr;                    // пул вичерпано
        Slot* s = free_;
        free_ = s->next;
        return ::new (static_cast<void*>(s->storage)) T(std::forward<Args>(args)...);
    }

    void destroy(T* p) noexcept {
        if (!p) return;
        p->~T();
        auto offset = reinterpret_cast<std::byte*>(p) -
                      reinterpret_cast<std::byte*>(slots_.data());
        Slot& s = slots_[static_cast<std::size_t>(offset) / sizeof(Slot)];
        s.next = free_;
        free_ = &s;
    }
};
```

Виділення і звільнення — O(1), без фрагментації. Пул не потокобезпечний: при використанні з переривань чи кількох задач RTOS обгорніть `create`/`destroy` критичною секцією.

### 6.3. `std::unique_ptr` з пулом

`unique_ptr` сам по собі не алокує — алокує `make_unique`. Його можна залишити для автоматичного повернення в пул:

```cpp
template <typename Pool>
struct PoolDeleter {
    Pool* pool;
    void operator()(typename Pool::value_type* p) const noexcept { pool->destroy(p); }
};

using TxPool = ObjectPool<Transaction, 4>;
using TxPtr  = std::unique_ptr<Transaction, PoolDeleter<TxPool>>;

static TxPool tx_pool;

Result<TxPtr> start_tx() {
    Transaction* t = tx_pool.create();
    if (!t) return etl::unexpected(Error::NoSlot);
    return TxPtr{t, PoolDeleter<TxPool>{&tx_pool}};
}
```

### 6.4. `std::shared_ptr`

На MCU спільне володіння майже завжди можна замінити однозначним власником (наприклад, endpoint володіє транзакціями, решта отримує посилання). Якщо спільне володіння справді потрібне — пул з інтрузивним лічильником посилань у самому об'єкті (`etl::reference_counted_object`).

**Критерій завершення:** немає `new`, `make_unique`, `make_shared`, `shared_ptr`.

---

## Етап 7. Колбеки

`std::function` виділяє пам'ять, якщо захоплений стан не вміщається у внутрішній буфер, і розмір цього буфера залежить від реалізації.

### 7.1. Шаблонний параметр (нуль накладних витрат)

```cpp
template <typename Handler>
void for_each_option(const MessageView& msg, Handler&& h) {
    for (auto opt : msg.options()) h(opt);
}
```

### 7.2. `function_ref` — невласний колбек

```cpp
// core/function_ref.hpp
#pragma once
#include <functional>   // std::invoke (сам std::function не використовується)
#include <memory>
#include <type_traits>
#include <utility>

template <typename Sig> class function_ref;

template <typename R, typename... Args>
class function_ref<R(Args...)> {
    void* obj_ = nullptr;
    R (*call_)(void*, Args...) = nullptr;

public:
    template <typename F>
        requires (!std::is_same_v<std::remove_cvref_t<F>, function_ref> &&
                  std::is_invocable_r_v<R, F&, Args...>)
    function_ref(F&& f) noexcept
        : obj_(const_cast<void*>(static_cast<const void*>(std::addressof(f)))),
          call_([](void* o, Args... a) -> R {
              return std::invoke(*static_cast<std::remove_reference_t<F>*>(o),
                                 std::forward<Args>(a)...);
          }) {}

    R operator()(Args... a) const { return call_(obj_, std::forward<Args>(a)...); }
};
```

```cpp
// було
void on_event(std::function<void(const Event&)> cb);
// стало
void on_event(function_ref<void(const Event&)> cb);   // лише на час виклику
```

> `function_ref` нічим не володіє: його **не можна зберігати**, якщо викликаний об'єкт (наприклад, лямбда) може зникнути раніше.

### 7.3. Колбек, який треба зберегти

- `etl::delegate<void(const Event&)>` — вказівник на функцію або метод конкретного об'єкта;
- `inplace_function<Sig, Bytes>` — як `std::function`, але з буфером фіксованого розміру; завеликий стан дає помилку компіляції;
- для C API — пара «вказівник на функцію + `void* ctx`».

**Критерій завершення:** немає `std::function`.

---

## Етап 8. Поліморфізм

Віртуальні функції самі по собі heap не потребують — проблемою є створення об'єктів через `new`.

### 8.1. Варіант А: статичний об'єкт + інтерфейс

```cpp
// було
std::unique_ptr<ITransport> transport = std::make_unique<UdpTransport>(cfg);

// стало
static UdpTransport udp{cfg};
ITransport& transport = udp;
```

**Важливо:** віртуальний деструктор змушує компілятор згенерувати «deleting destructor», який посилається на `operator delete`, навіть якщо об'єкт ніколи не створювався через `new`. Це ламає контроль з етапу 11. Рішення — захищений невіртуальний деструктор інтерфейсу:

```cpp
class ITransport {
public:
    virtual Status send(Endpoint ep, std::span<const std::byte> data) = 0;
    virtual Result<Received> receive(std::span<std::byte> buf) = 0;
    virtual std::size_t mtu() const = 0;
protected:
    ~ITransport() = default;      // видалення через інтерфейс заборонене
};
```

Також для чисто віртуальних функцій потрібно визначити `__cxa_pure_virtual` (див. етап 9.4).

### 8.2. Варіант Б: concepts (статичний поліморфізм, C++20)

```cpp
template <typename T>
concept Transport = requires(T t, Endpoint ep,
                             std::span<const std::byte> out,
                             std::span<std::byte> in) {
    { t.send(ep, out) } -> std::same_as<Status>;
    { t.receive(in) }   -> std::same_as<Result<Received>>;
    { t.mtu() }         -> std::convertible_to<std::size_t>;
};

template <Transport T, std::size_t MaxPending = 4>
class CoapEndpoint {
    T& transport_;
    etl::flat_map<std::uint16_t, Transaction, MaxPending> pending_;
public:
    explicit CoapEndpoint(T& t) : transport_(t) {}
    // ...
};

static LoraTransport radio;
static CoapEndpoint<LoraTransport, 2> coap{radio};
```

Без vtable, з інлайнингом і зрозумілими помилками компіляції. Ціна — окрема інстанціація коду для кожного типу транспорту.

**Критерій завершення:** жоден поліморфний об'єкт не створюється через `new`; інтерфейси мають захищений невіртуальний деструктор.

---

## Етап 9. Відмова від винятків

Виконується **після** етапів 3–8: більшість джерел `throw` (`at()`, `stoi`, алокації) на цей момент уже прибрано.

### 9.1. Власні `throw` → `Result`

```cpp
// було
MessageView parse(std::span<const std::byte> buf) {
    if (buf.size() < 4) throw std::runtime_error("too short");
    // ...
}

// стало
Result<MessageView> parse(std::span<const std::byte> buf) {
    if (buf.size() < 4) return etl::unexpected(Error::Malformed);
    // ...
}

// використання
auto msg = parse(rx);
if (!msg) { log_error(msg.error()); return; }
handle(*msg);
```

### 9.2. Методи STL, що кидають винятки

| Замість | Використовуйте |
|---|---|
| `opt.value()` | `if (opt) use(*opt);` |
| `std::get<T>(var)` | `if (auto* p = std::get_if<T>(&var)) use(*p);` |
| `container.at(i)` | перевірка `i < size()` + `operator[]` |
| `std::stoi`, `std::stof` | `std::from_chars` |

Під `-fno-exceptions` ці методи не кидають, а викликають `abort()`, тобто тихо перетворюються на аварійну зупинку.

### 9.3. Увімкнення прапорців

```cmake
target_compile_options(firmware PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions -fno-rtti -fno-threadsafe-statics>
)
```

- `-fno-rtti` — `dynamic_cast` і `typeid` стають недоступні;
- `-fno-threadsafe-statics` — прибирає guard-виклики для локальних `static`. Безпечно, якщо такі об'єкти не ініціалізуються одночасно з кількох задач або переривань.

### 9.4. Зменшення залишкового коду libstdc++

```cpp
// platform/cxx_support.cpp
#include <cstdlib>

extern "C" void __cxa_pure_virtual() { std::abort(); }

// GCC: стандартний обробник завершення тягне деманглер (десятки КБ)
namespace __gnu_cxx {
void __verbose_terminate_handler() { std::abort(); }
}
```

**Критерій завершення:** проєкт збирається з `-fno-exceptions -fno-rtti`; у ELF немає `__cxa_throw`, `__cxa_allocate_exception`.

---

## Етап 10. Залишки динаміки: `std::pmr`

Якщо в якомусь модулі (зазвичай не в ядрі протоколу, а в складнішій логіці або коді шлюзу) зручніше залишити звичайні контейнери, C++17 `std::pmr` дозволяє брати пам'ять зі статичного буфера:

```cpp
#include <memory_resource>
#include <vector>

alignas(std::max_align_t) static std::byte arena[2048];

void handle_request(const MessageView& msg) {
    std::pmr::monotonic_buffer_resource pool{
        arena, sizeof(arena),
        std::pmr::null_memory_resource()      // арена закінчилась → помилка, не heap
    };

    std::pmr::vector<OptionView> opts{&pool};
    // ... обробка ...
}   // пам'ять арени «звільняється» разом з pool
```

Особливості:

- `monotonic_buffer_resource` не звільняє окремі блоки — лише все разом. Ідеально для «арени на один запит»;
- `null_memory_resource()` як резервний ресурс гарантує, що heap не використовується;
- без винятків вичерпання арени призводить до `abort()`, тому розмір арени треба підбирати з запасом і перевіряти в тестах;
- `pmr` додає код і непряму диспетчеризацію, тому в ядрі краще обходитися етапами 3–8.

**Критерій завершення:** кожен залишковий `std::pmr`-контейнер має явну арену і `null_memory_resource()` як резерв.

---

## Етап 11. Контроль у CI

Домовленостей недостатньо: будь-яка нова залежність може повернути heap. Перевірка має бути автоматичною.

### 11.1. Перевірка символів у прошивці

```bash
#!/usr/bin/env bash
# ci/check_no_heap.sh
set -euo pipefail
ELF="$1"

FORBIDDEN='\b(malloc|calloc|realloc|free|_malloc_r|_free_r|operator new|operator delete|__cxa_throw|__cxa_allocate_exception)\b'

if arm-none-eabi-nm -C "$ELF" | grep -E "$FORBIDDEN"; then
    echo "ERROR: heap або винятки виявлено в $ELF" >&2
    exit 1
fi
echo "OK: $ELF не містить heap-символів"
```

### 11.2. Пастка на етапі компонування

`-Wl,--wrap=malloc` перенаправляє всі виклики `malloc` на `__wrap_malloc`. Якщо цю функцію не визначати, будь-яке звернення до `malloc` дасть помилку компонування:

```cmake
target_link_options(firmware PRIVATE -Wl,--wrap=malloc -Wl,--wrap=free -Wl,--wrap=calloc -Wl,--wrap=realloc)
```

> Pico SDK сам обгортає `malloc` таким самим способом (модуль `pico_malloc`), тому там цей прийом конфліктує. Для Pico використовуйте перевірку символів (11.1).
> Деякі функції newlib (`printf` з певними форматами, `strdup` тощо) всередині викликають `malloc` — пастка це покаже, і їх треба замінити.

### 11.3. Тест на хості

Тест з етапу 0.3 переводиться з «кількість не зростає» на «кількість дорівнює нулю» для всіх сценаріїв бібліотеки.

### 11.4. Звіт про пам'ять

Для кожної цільової плати CI зберігає розміри секцій:

```bash
arm-none-eabi-size -A build/firmware.elf
```

Різке зростання `.text` або `.bss` між комітами — сигнал перевірити, що саме додалося.

**Критерій завершення:** CI падає при появі будь-якого heap-символу або винятку.

---

## Примітки для STM32F4/H7 і Pico SDK

### STM32F4 / STM32H7

- **Тулчейн:** STM32CubeIDE використовує `arm-none-eabi-gcc`; стандарт C++20 вибирається в налаштуваннях компілятора G++ проєкту (або через CMake-проєкт).
- **Згенерований код C:** CubeMX генерує `main.c` і перезаписує його при регенерації. Зручний шаблон — викликати з `main.c` одну C++-функцію:

  ```cpp
  // app/app_main.cpp
  extern "C" void app_main() {
      static App app;      // увесь застосунок — статичний об'єкт
      app.run();
  }
  ```

  ```c
  /* main.c, у секції USER CODE */
  extern void app_main(void);
  /* ... після ініціалізації HAL ... */
  app_main();
  ```

- **newlib-nano** (`--specs=nano.specs`) зменшує розмір libc; разом з відсутністю heap можна обнулити `_Min_Heap_Size` у лінкер-скрипті, щоб пам'ять не резервувалась даремно.
- **STM32H7 і zero-copy:** у Cortex-M7 є D-кеш, а DMA його не бачить. Буфери, які заповнює DMA (Ethernet, SPI-радіо), мають бути або в некешованій області (налаштування MPU), або супроводжуватись інвалідацією кешу перед читанням через `span`. Вирівнювання — `alignas(32)` (розмір рядка кешу). Пам'ять DTCM недоступна для частини DMA-контролерів, тому розміщення буферів має контролювати користувач бібліотеки:

  ```cpp
  __attribute__((section(".dma_buffer"), aligned(32)))
  static std::array<std::byte, 512> eth_rx;
  ```

### Raspberry Pi Pico / Pico 2 (Pico C/C++ SDK)

- **Стандарт** задається в CMake до `pico_sdk_init()`:

  ```cmake
  set(CMAKE_CXX_STANDARD 20)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  pico_sdk_init()
  ```

- **Винятки і RTTI** у Pico SDK вимкнені за замовчуванням (`PICO_CXX_ENABLE_EXCEPTIONS=0`, `PICO_CXX_ENABLE_RTTI=0`) — це якраз потрібна конфігурація.
- **`pico_malloc`** уже обгортає `malloc`, тому для контролю використовуйте перевірку символів (етап 11.1), а не `--wrap`.
- **Мережевий стек Pico W** (lwIP + драйвер CYW43) має власні пули пам'яті. Вони не використовують heap бібліотеки C, але їх розміри (`MEM_SIZE`, `PBUF_POOL_SIZE` у `lwipopts.h`) треба враховувати в бюджеті RAM.
- **RP2040 (Cortex-M0+)** має обмежений кеш XIP-Flash: великі шаблонні інстанціації і `constexpr`-таблиці виконуються/читаються з Flash повільніше. Гарячі функції можна розмістити в RAM (`__not_in_flash_func`).
- **RP2350 в режимі RISC-V** потребує окремого RISC-V GCC — додайте цю конфігурацію в матрицю CI, щоб переконатися, що код не прив'язаний до ARM.

---

## Підсумковий чекліст

**Етап 0 — інвентаризація**
- [ ] Складено список місць алокацій і `throw`
- [ ] Є тест-лічильник алокацій на хості
- [ ] Map-файл генерується при кожній збірці

**Етап 1 — C++20**
- [ ] `-std=c++20` на всіх цілях
- [ ] Виправлено попередження CMSIS/HAL (`register`, `-Wvolatile`)

**Етапи 2–8 — заміна компонентів**
- [ ] `Result`, ETL або власні аналоги доступні
- [ ] Немає `std::string`, `to_string`, `stoi`, `sprintf`
- [ ] Немає `vector`, `deque`, `queue`, `list`
- [ ] Немає `map`, `set`, `unordered_*`; статичні таблиці — `constexpr` у Flash
- [ ] Немає `new`, `make_unique`, `make_shared`, `shared_ptr`
- [ ] Немає `std::function`
- [ ] Інтерфейси мають захищений невіртуальний деструктор

**Етап 9 — винятки**
- [ ] Увімкнено `-fno-exceptions -fno-rtti -fno-threadsafe-statics`
- [ ] Визначено `__cxa_pure_virtual` і спрощений обробник завершення

**Етап 10 — `pmr`**
- [ ] Кожен `pmr`-контейнер має арену і `null_memory_resource()`

**Етап 11 — контроль**
- [ ] CI перевіряє ELF на heap-символи для кожної плати
- [ ] Тест на хості вимагає нуль алокацій
- [ ] Розміри секцій відстежуються між комітами
