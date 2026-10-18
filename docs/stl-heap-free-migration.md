# Step-by-Step Replacement of Heap-Based STL Components with a Move to C++20

Target platforms: STM32F4/H7 (Cortex-M4F/M7), Raspberry Pi Pico / Pico 2 (RP2040/RP2350, Pico C/C++ SDK).
Toolchain: `arm-none-eabi-gcc` (Clang if needed), C++20 standard, `-fno-exceptions -fno-rtti`.

---

## Contents

1. [Goals and principles](#1-goals-and-principles)
2. [Mapping table](#2-mapping-table)
3. [Stage 0. Allocation inventory](#stage-0-allocation-inventory)
4. [Stage 1. Switching the toolchain to C++20](#stage-1-switching-the-toolchain-to-c20)
5. [Stage 2. Base infrastructure](#stage-2-base-infrastructure)
6. [Stage 3. Strings](#stage-3-strings)
7. [Stage 4. Sequence containers](#stage-4-sequence-containers)
8. [Stage 5. Associative containers](#stage-5-associative-containers)
9. [Stage 6. Object ownership and smart pointers](#stage-6-object-ownership-and-smart-pointers)
10. [Stage 7. Callbacks](#stage-7-callbacks)
11. [Stage 8. Polymorphism](#stage-8-polymorphism)
12. [Stage 9. Removing exceptions](#stage-9-removing-exceptions)
13. [Stage 10. Remaining dynamic code: `std::pmr`](#stage-10-remaining-dynamic-code-stdpmr)
14. [Stage 11. CI enforcement](#stage-11-ci-enforcement)
15. [Notes for STM32F4/H7 and Pico SDK](#notes-for-stm32f4h7-and-pico-sdk)
16. [Final checklist](#final-checklist)

---

## 1. Goals and principles

**Goal:** the firmware (or library) never calls `malloc`, `free`, `operator new`, `operator delete`, and does not use exceptions. The entire RAM footprint is known after linking, from the map file.

**Principles:**

- **The user owns the memory.** Buffers and capacities are passed in from outside or set as template parameters.
- **Fixed capacity instead of growth.** Overflow is a returned error, not an allocation and not an exception.
- **Zero-copy.** Data is read through `std::span` / `std::string_view` over existing buffers.
- **Errors as values.** An `expected`-like type instead of `throw`.
- **Do everything possible at compile time.** `constexpr` tables in Flash instead of building structures in RAM.

**Migration rule:** after each stage the project builds, tests pass, and the number of allocations (see Stage 0) does not increase. Stages can be applied module by module: first the protocol core, then the rest.

---

## 2. Mapping table

| Before (uses heap) | After (heap-free) | Stage |
|---|---|---|
| `std::string` (owning) | `etl::string<N>` | 3 |
| `const std::string&` (parameter) | `std::string_view` | 3 |
| `std::to_string`, `std::stoi`, `sprintf` | `std::to_chars`, `std::from_chars` | 3 |
| `std::vector<T>` (known maximum) | `std::array<T, N>`, `static_vector<T, N>` / `etl::vector<T, N>` | 4 |
| `const std::vector<T>&` (parameter) | `std::span<const T>` | 4 |
| `std::deque`, `std::queue` | ring buffer `RingBuffer<T, N>` | 4 |
| `std::list` | intrusive list / `etl::list<T, N>` | 4 |
| `std::map`, `std::set` | sorted `static_vector` + `lower_bound` / `etl::flat_map<K, V, N>` | 5 |
| `std::unordered_map` | `etl::unordered_map<K, V, N>` or a `constexpr` table | 5 |
| `std::make_unique`, `new T` | static object or `ObjectPool<T, N>` | 6 |
| `std::shared_ptr` | single owner; if needed, a pool with an intrusive reference count | 6 |
| `std::function` | template parameter, `function_ref`, `etl::delegate`, `inplace_function` | 7 |
| `std::unique_ptr<Interface>` + `make_unique<Impl>` | static `Impl` + reference to the interface, or concepts | 8 |
| `throw` / `try` / `catch` | `Result<T>` (`etl::expected`, `tl::expected`, C++23 `std::expected`) | 9 |
| `optional::value()`, `std::get<T>(variant)`, `vector::at()` | `*opt`, `std::get_if`, explicit index check | 9 |
| Remaining dynamic structures | `std::pmr::*` on top of a static arena | 10 |

> About future standards: C++26 adds `std::inplace_vector` and `std::function_ref`. Code written with ETL or custom equivalents can later be moved to them easily.

---

## Stage 0. Allocation inventory

Before changing anything, you need to know **where** and **how many** allocations happen.

### 0.1. Static code search

```bash
grep -rnE 'std::(vector|string|map|set|unordered_|list|deque|queue|function|shared_ptr|make_unique|make_shared|any)\b|\bnew\b|\bthrow\b|\btry\b' src/ include/
```

The result is a list of files and lines that becomes the migration backlog.

### 0.2. Symbols in the built firmware

```bash
arm-none-eabi-nm -C build/firmware.elf | grep -E 'malloc|free|realloc|operator new|operator delete|__cxa_throw|__cxa_allocate_exception'
```

The map file (`-Wl,-Map=firmware.map`) shows **which object file** pulled in these symbols.

### 0.3. Allocation counter in host tests

Build the tests for Linux with an overridden `operator new`:

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
// in a test
extern std::size_t g_alloc_count;

TEST(Coap, ParseAndRespondWithoutHeap) {
    g_alloc_count = 0;
    run_request_response_scenario();
    EXPECT_EQ(g_alloc_count, 0u);
}
```

**Done when:** there is a list of allocation sites and a test that records their current count.

---

## Stage 1. Switching the toolchain to C++20

At this stage exceptions are **not yet disabled** — only the standard changes.

### 1.1. CMake

```cmake
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)        # -std=c++20 rather than gnu++20

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

### 1.2. Checking the compiler version

The C++20 features this document relies on (concepts, `std::span`, `<bit>`, `<ranges>`, class types as template parameters) are available in GCC 10–11 and newer.

```bash
arm-none-eabi-gcc --version
```

### 1.3. Common issues after the switch

| Symptom | Cause | Fix |
|---|---|---|
| Errors about `register` in CMSIS headers | the keyword was removed in C++17 | update CMSIS to a current version |
| Lots of `-Wvolatile` warnings on `REG \|= x` | C++20 deprecated compound operations on `volatile` | newer GCC no longer warns for `\|=`, `&=`, `^=`; on older versions use `-Wno-volatile` |
| `char8_t` breaks `u8"..."` | in C++20, `u8` literals have type `const char8_t*` | drop the `u8` prefix or use `-fno-char8_t` temporarily |
| Ambiguous `operator==` | rewritten comparisons in C++20 | make operators `const` and symmetric |

**Done when:** the project builds with `-std=c++20`, behaviour is unchanged, tests are green.

---

## Stage 2. Base infrastructure

Before replacing containers, you need the types they will rely on.

### 2.1. Container library

It is recommended to add **ETL (Embedded Template Library)**: `etl::vector`, `etl::string`, `etl::flat_map`, `etl::delegate`, `etl::expected`, `etl::pool`, etc. It does not use the heap and is designed for `-fno-exceptions`.

```cmake
include(FetchContent)
FetchContent_Declare(etl GIT_REPOSITORY https://github.com/ETLCPP/etl GIT_TAG <version>)
FetchContent_MakeAvailable(etl)
target_link_libraries(firmware PRIVATE etl::etl)
```

Minimal custom implementations are also shown below — for cases where an external dependency is undesirable, or to illustrate the principle.

### 2.2. Error type

```cpp
// core/result.hpp
#pragma once
#include <cstdint>
#include <etl/expected.h>

enum class Error : std::uint8_t {
    BufferTooSmall,
    Malformed,
    NoSlot,        // fixed capacity exhausted
    NotFound,
    Timeout,
    Io,
};

template <typename T>
using Result = etl::expected<T, Error>;

using Status = etl::expected<void, Error>;
```

### 2.3. String as a template parameter (C++20)

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

**Done when:** `Result` and ETL (or custom equivalents) are available in the project and covered by tests.

---

## Stage 3. Strings

### 3.1. Parameters: `const std::string&` → `std::string_view`

```cpp
// before
bool match(const std::string& path);
// after
bool match(std::string_view path);
```

> Watch lifetimes: `string_view` owns nothing. Do not keep it longer than the buffer it points to lives.

### 3.2. Owning strings: `std::string` → `etl::string<N>`

```cpp
// before
std::string device_name;
// after
etl::string<32> device_name;   // 32 characters, storage inside the object
```

Overflow does not allocate; the string is truncated (this can be checked with `is_truncated()`).

### 3.3. Formatting: `std::to_string` / `sprintf` → `std::to_chars`

```cpp
// before
std::string path = "/sensors/" + std::to_string(id);

// after
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

### 3.4. Parsing: `std::stoi` → `std::from_chars`

```cpp
Result<int> parse_int(std::string_view s) {
    int value{};
    auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), value);
    if (ec != std::errc{} || ptr != s.data() + s.size()) return etl::unexpected(Error::Malformed);
    return value;
}
```

`std::stoi` throws exceptions; `from_chars` does not.

> Formatting `float` via `to_chars` or floating-point `printf` noticeably increases code size. Where possible, transmit values as fixed-point integers (for example, hundredths of a degree).

**Done when:** the module contains no `std::string`, `to_string`, `stoi`, `sprintf`.

---

## Stage 4. Sequence containers

### 4.1. Parameters: `const std::vector<T>&` → `std::span<const T>`

```cpp
// before
void send(const std::vector<std::uint8_t>& data);
// after
void send(std::span<const std::byte> data);
```

`span` accepts `std::array`, `static_vector`, C arrays, and slices of a buffer.

### 4.2. Size known at compile time: `std::array`

```cpp
// before
std::vector<std::byte> rx_buf(256);
// after
std::array<std::byte, 256> rx_buf{};
```

### 4.3. Variable count up to a maximum: `static_vector<T, N>`

Use `etl::vector<T, N>`. A minimal custom version for illustration:

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
    static_vector(const static_vector&) = delete;            // simplification: see ETL
    static_vector& operator=(const static_vector&) = delete;
    ~static_vector() { clear(); }

    template <typename... Args>
    T* emplace_back(Args&&... args) {
        if (size_ == N) return nullptr;                        // instead of throw / allocation
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
// before
std::vector<Observer> observers;
observers.push_back(obs);

// after
static_vector<Observer, 4> observers;
if (!observers.emplace_back(obs)) {
    return etl::unexpected(Error::NoSlot);
}
```

### 4.4. Queues: `std::deque` / `std::queue` → ring buffer

```cpp
// core/ring_buffer.hpp
#pragma once
#include <array>
#include <cstddef>
#include <optional>

template <typename T, std::size_t N>   // T must be default-constructible
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

    // "overwrite the oldest" variant, handy for event logs
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

> If `N` is a power of two, the compiler replaces `% N` with a bitwise `&`. Exchanging data between an interrupt and the main loop requires a lock-free version (single producer / single consumer with `std::atomic`) or a critical section.

### 4.5. `std::list` → intrusive list

The list node is embedded in the object itself, so insertion needs no memory:

```cpp
struct PendingTx {
    PendingTx* next = nullptr;   // intrusive link
    std::uint16_t message_id;
    // ...
};
```

Ready-made options: `etl::intrusive_list`, `etl::list<T, N>`.

**Done when:** the module contains no `vector`, `deque`, `queue`, `list`; overflow is handled explicitly.

---

## Stage 5. Associative containers

### 5.1. Tables known at compile time → `constexpr` array in Flash

The best option for CoAP resource routing, error codes, configuration:

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

// C++20: ranges algorithms are constexpr — the compiler verifies the ordering
static_assert(std::ranges::is_sorted(routes, {}, &Route::path),
              "routes must be sorted by path");

constexpr const Route* find_route(std::string_view path) {
    auto it = std::ranges::lower_bound(routes, path, {}, &Route::path);
    return (it != routes.end() && it->path == path) ? &*it : nullptr;
}
```

The table lives in `.rodata` (Flash), uses no RAM, and lookup is a binary search.

### 5.2. Tables that change at run time → flat map

```cpp
// before
std::map<std::uint16_t, Transaction> pending;

// after
etl::flat_map<std::uint16_t, Transaction, 8> pending;   // sorted array of 8 entries
if (pending.full()) return etl::unexpected(Error::NoSlot);
pending.insert({mid, tx});
```

For small `N` (up to a few dozen), a sorted array or even a linear search is faster than a tree and uses less memory.

### 5.3. `std::unordered_map`

- small size → linear search in a `static_vector` or `flat_map`;
- large size → `etl::unordered_map<K, V, N>` (fixed number of nodes and buckets).

**Done when:** the module contains no `map`, `set`, `unordered_*`.

---

## Stage 6. Object ownership and smart pointers

### 6.1. Objects that live for the whole run time → static

```cpp
// before
auto radio = std::make_unique<LoraRadio>(cfg);

// after
static LoraRadio radio{cfg};      // .bss / .data, size known at link time
```

### 6.2. Objects with arbitrary lifetime → pool

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
        if (!free_) return nullptr;                    // pool exhausted
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

Allocation and release are O(1) with no fragmentation. The pool is not thread-safe: when used from interrupts or multiple RTOS tasks, wrap `create`/`destroy` in a critical section.

### 6.3. `std::unique_ptr` with a pool

`unique_ptr` itself does not allocate — `make_unique` does. It can be kept for automatic return to the pool:

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

On an MCU, shared ownership can almost always be replaced by a single owner (for example, the endpoint owns transactions and everything else gets references). If shared ownership is really needed, use a pool with an intrusive reference count inside the object itself (`etl::reference_counted_object`).

**Done when:** there is no `new`, `make_unique`, `make_shared`, `shared_ptr`.

---

## Stage 7. Callbacks

`std::function` allocates memory if the captured state does not fit its internal buffer, and the size of that buffer is implementation-defined.

### 7.1. Template parameter (zero overhead)

```cpp
template <typename Handler>
void for_each_option(const MessageView& msg, Handler&& h) {
    for (auto opt : msg.options()) h(opt);
}
```

### 7.2. `function_ref` — non-owning callback

```cpp
// core/function_ref.hpp
#pragma once
#include <functional>   // std::invoke (std::function itself is not used)
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
// before
void on_event(std::function<void(const Event&)> cb);
// after
void on_event(function_ref<void(const Event&)> cb);   // only for the duration of the call
```

> `function_ref` owns nothing: it **must not be stored** if the callable (for example, a lambda) may go away first.

### 7.3. Callbacks that must be stored

- `etl::delegate<void(const Event&)>` — a pointer to a function or to a member function of a specific object;
- `inplace_function<Sig, Bytes>` — like `std::function` but with a fixed-size buffer; oversized state is a compile error;
- for a C API — a "function pointer + `void* ctx`" pair.

**Done when:** there is no `std::function`.

---

## Stage 8. Polymorphism

Virtual functions themselves do not need the heap — the problem is creating objects with `new`.

### 8.1. Option A: static object + interface

```cpp
// before
std::unique_ptr<ITransport> transport = std::make_unique<UdpTransport>(cfg);

// after
static UdpTransport udp{cfg};
ITransport& transport = udp;
```

**Important:** a virtual destructor makes the compiler emit a "deleting destructor" that references `operator delete`, even if the object is never created with `new`. This breaks the checks from Stage 11. The fix is a protected non-virtual destructor in the interface:

```cpp
class ITransport {
public:
    virtual Status send(Endpoint ep, std::span<const std::byte> data) = 0;
    virtual Result<Received> receive(std::span<std::byte> buf) = 0;
    virtual std::size_t mtu() const = 0;
protected:
    ~ITransport() = default;      // deletion through the interface is forbidden
};
```

Pure virtual functions also require `__cxa_pure_virtual` to be defined (see Stage 9.4).

### 8.2. Option B: concepts (static polymorphism, C++20)

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

No vtable, inlining, and clear compile errors. The cost is a separate code instantiation for each transport type.

**Done when:** no polymorphic object is created with `new`; interfaces have a protected non-virtual destructor.

---

## Stage 9. Removing exceptions

Done **after** Stages 3–8: by then most sources of `throw` (`at()`, `stoi`, allocations) are already gone.

### 9.1. Own `throw` → `Result`

```cpp
// before
MessageView parse(std::span<const std::byte> buf) {
    if (buf.size() < 4) throw std::runtime_error("too short");
    // ...
}

// after
Result<MessageView> parse(std::span<const std::byte> buf) {
    if (buf.size() < 4) return etl::unexpected(Error::Malformed);
    // ...
}

// usage
auto msg = parse(rx);
if (!msg) { log_error(msg.error()); return; }
handle(*msg);
```

### 9.2. STL methods that throw

| Instead of | Use |
|---|---|
| `opt.value()` | `if (opt) use(*opt);` |
| `std::get<T>(var)` | `if (auto* p = std::get_if<T>(&var)) use(*p);` |
| `container.at(i)` | check `i < size()` + `operator[]` |
| `std::stoi`, `std::stof` | `std::from_chars` |

Under `-fno-exceptions` these methods do not throw — they call `abort()`, silently turning into a hard stop.

### 9.3. Enabling the flags

```cmake
target_compile_options(firmware PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions -fno-rtti -fno-threadsafe-statics>
)
```

- `-fno-rtti` — `dynamic_cast` and `typeid` become unavailable;
- `-fno-threadsafe-statics` — removes guard calls for local `static` objects. Safe as long as such objects are not initialised concurrently from several tasks or interrupts.

### 9.4. Reducing leftover libstdc++ code

```cpp
// platform/cxx_support.cpp
#include <cstdlib>

extern "C" void __cxa_pure_virtual() { std::abort(); }

// GCC: the default terminate handler pulls in the demangler (tens of KB)
namespace __gnu_cxx {
void __verbose_terminate_handler() { std::abort(); }
}
```

**Done when:** the project builds with `-fno-exceptions -fno-rtti`; the ELF contains no `__cxa_throw`, `__cxa_allocate_exception`.

---

## Stage 10. Remaining dynamic code: `std::pmr`

If some module (usually not the protocol core, but more complex logic or gateway code) is easier to write with ordinary containers, C++17 `std::pmr` lets them take memory from a static buffer:

```cpp
#include <memory_resource>
#include <vector>

alignas(std::max_align_t) static std::byte arena[2048];

void handle_request(const MessageView& msg) {
    std::pmr::monotonic_buffer_resource pool{
        arena, sizeof(arena),
        std::pmr::null_memory_resource()      // arena exhausted → error, not heap
    };

    std::pmr::vector<OptionView> opts{&pool};
    // ... processing ...
}   // arena memory is "released" together with pool
```

Notes:

- `monotonic_buffer_resource` does not free individual blocks — only everything at once. Ideal for a "per-request arena";
- `null_memory_resource()` as the upstream resource guarantees the heap is never used;
- without exceptions, exhausting the arena leads to `abort()`, so size the arena with headroom and verify it in tests;
- `pmr` adds code and indirect dispatch, so in the core it is better to rely on Stages 3–8.

**Done when:** every remaining `std::pmr` container has an explicit arena and `null_memory_resource()` as upstream.

---

## Stage 11. CI enforcement

Agreements are not enough: any new dependency can bring the heap back. The check must be automatic.

### 11.1. Checking symbols in the firmware

```bash
#!/usr/bin/env bash
# ci/check_no_heap.sh
set -euo pipefail
ELF="$1"

FORBIDDEN='\b(malloc|calloc|realloc|free|_malloc_r|_free_r|operator new|operator delete|__cxa_throw|__cxa_allocate_exception)\b'

if arm-none-eabi-nm -C "$ELF" | grep -E "$FORBIDDEN"; then
    echo "ERROR: heap or exceptions found in $ELF" >&2
    exit 1
fi
echo "OK: $ELF contains no heap symbols"
```

### 11.2. Link-time trap

`-Wl,--wrap=malloc` redirects all `malloc` calls to `__wrap_malloc`. If that function is left undefined, any reference to `malloc` results in a link error:

```cmake
target_link_options(firmware PRIVATE -Wl,--wrap=malloc -Wl,--wrap=free -Wl,--wrap=calloc -Wl,--wrap=realloc)
```

> The Pico SDK wraps `malloc` the same way itself (the `pico_malloc` module), so this technique conflicts there. For Pico, use the symbol check (11.1).
> Some newlib functions (`printf` with certain formats, `strdup`, etc.) call `malloc` internally — the trap will reveal them so they can be replaced.

### 11.3. Host test

The test from Stage 0.3 changes from "the count does not grow" to "the count is zero" for all library scenarios.

### 11.4. Memory report

For each target board, CI records section sizes:

```bash
arm-none-eabi-size -A build/firmware.elf
```

A sharp increase in `.text` or `.bss` between commits is a signal to check what was added.

**Done when:** CI fails on the appearance of any heap symbol or exception.

---

## Notes for STM32F4/H7 and Pico SDK

### STM32F4 / STM32H7

- **Toolchain:** STM32CubeIDE uses `arm-none-eabi-gcc`; the C++20 standard is selected in the project's G++ compiler settings (or via a CMake project).
- **Generated C code:** CubeMX generates `main.c` and overwrites it on regeneration. A convenient pattern is to call a single C++ function from `main.c`:

  ```cpp
  // app/app_main.cpp
  extern "C" void app_main() {
      static App app;      // the whole application is a static object
      app.run();
  }
  ```

  ```c
  /* main.c, inside a USER CODE section */
  extern void app_main(void);
  /* ... after HAL initialisation ... */
  app_main();
  ```

- **newlib-nano** (`--specs=nano.specs`) reduces libc size; together with no heap you can set `_Min_Heap_Size` in the linker script to zero so memory is not reserved needlessly.
- **STM32H7 and zero-copy:** the Cortex-M7 has a D-cache that DMA does not see. Buffers filled by DMA (Ethernet, SPI radio) must either live in a non-cacheable region (MPU configuration) or be accompanied by cache invalidation before being read through a `span`. Alignment: `alignas(32)` (cache line size). DTCM memory is not accessible to some DMA controllers, so buffer placement must be controlled by the library user:

  ```cpp
  __attribute__((section(".dma_buffer"), aligned(32)))
  static std::array<std::byte, 512> eth_rx;
  ```

### Raspberry Pi Pico / Pico 2 (Pico C/C++ SDK)

- **The standard** is set in CMake before `pico_sdk_init()`:

  ```cmake
  set(CMAKE_CXX_STANDARD 20)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  pico_sdk_init()
  ```

- **Exceptions and RTTI** are disabled by default in the Pico SDK (`PICO_CXX_ENABLE_EXCEPTIONS=0`, `PICO_CXX_ENABLE_RTTI=0`) — exactly the configuration needed.
- **`pico_malloc`** already wraps `malloc`, so use the symbol check (Stage 11.1) rather than `--wrap`.
- **The Pico W network stack** (lwIP + CYW43 driver) has its own memory pools. They do not use the C library heap, but their sizes (`MEM_SIZE`, `PBUF_POOL_SIZE` in `lwipopts.h`) must be included in the RAM budget.
- **RP2040 (Cortex-M0+)** has a limited XIP Flash cache: large template instantiations and `constexpr` tables execute/read more slowly from Flash. Hot functions can be placed in RAM (`__not_in_flash_func`).
- **RP2350 in RISC-V mode** requires a separate RISC-V GCC — add this configuration to the CI matrix to make sure the code is not tied to ARM.

---

## Final checklist

**Stage 0 — inventory**
- [ ] A list of allocation and `throw` sites has been compiled
- [ ] There is a host-side allocation counter test
- [ ] A map file is generated on every build

**Stage 1 — C++20**
- [ ] `-std=c++20` on all targets
- [ ] CMSIS/HAL warnings fixed (`register`, `-Wvolatile`)

**Stages 2–8 — component replacement**
- [ ] `Result`, ETL or custom equivalents are available
- [ ] No `std::string`, `to_string`, `stoi`, `sprintf`
- [ ] No `vector`, `deque`, `queue`, `list`
- [ ] No `map`, `set`, `unordered_*`; static tables are `constexpr` in Flash
- [ ] No `new`, `make_unique`, `make_shared`, `shared_ptr`
- [ ] No `std::function`
- [ ] Interfaces have a protected non-virtual destructor

**Stage 9 — exceptions**
- [ ] `-fno-exceptions -fno-rtti -fno-threadsafe-statics` enabled
- [ ] `__cxa_pure_virtual` and a simplified terminate handler are defined

**Stage 10 — `pmr`**
- [ ] Every `pmr` container has an arena and `null_memory_resource()`

**Stage 11 — enforcement**
- [ ] CI checks the ELF for heap symbols for every board
- [ ] The host test requires zero allocations
- [ ] Section sizes are tracked between commits
