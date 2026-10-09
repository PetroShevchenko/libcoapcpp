# Fuzzing

This directory holds the fuzz targets of libcoapcpp. A fuzz target feeds one module with
millions of generated inputs and stops at the first input that breaks it.

| Path | Contents |
|---|---|
| `fuzz_<target>.cc` | One fuzz target per module |
| `corpus/<target>/` | Seed inputs of that target, kept in git |
| `CMakeLists.txt` | Standalone project that builds the targets |
| `../fuzzer_run.sh` | Builds one target and runs it |
| `../build/fuzz/` | Build output, grown corpus and crash files; not in git |

Existing targets:

| Target | Module | Entry points |
|---|---|---|
| `packet` | `src/packet.cc` | `Packet::parse`, `Packet::serialize` |
| `blockwise` | `src/blockwise.cc` | `Block1::get_header`, `Block2::get_header`, `encode_block_option`, `decode_size_option` |

## How it works

**libFuzzer** is a part of clang. It is linked into the target, generates the inputs and calls
one function for each of them:

```cpp
extern "C" int LLVMFuzzerTestOneInput(const uint8_t * data, size_t size);
```

The compiler instruments the library code, so libFuzzer sees which branches an input reached.
An input that reaches new code is kept and mutated further; this is how the fuzzer gets through
the checks of a parser instead of guessing blindly.

A run stops with a crash file in three cases:

| Detector | What it catches | First line of the report |
|---|---|---|
| AddressSanitizer | Read or write outside of a buffer, use after free, leak | `ERROR: AddressSanitizer: ...` |
| UndefinedBehaviorSanitizer | Null pointer argument, integer overflow, bad shift | `runtime error: ...` |
| The target itself | A broken property of the module, reported by `fail()` | `fuzz_<target>: ...` |

The sanitizers only see what the target lets them see, and the properties exist only if the
target checks them. Both are the job of `fuzz_<target>.cc`.

The **seed corpus** in `corpus/<target>/` gives the fuzzer a starting point: a few valid inputs
and the known rejected ones. Without seeds it has to discover the format from nothing.

## Running

Requirements: clang with libFuzzer (`clang` and, on Debian or Ubuntu, `libclang-rt-dev`), CMake.

```sh
./fuzzer_run.sh                                         # list of the available targets
./fuzzer_run.sh packet                                  # 60 seconds
./fuzzer_run.sh packet 600                              # 10 minutes
./fuzzer_run.sh packet 600 -fork=4 -ignore_crashes=1    # 4 processes, collect every crash
```

Everything after the duration is passed to libFuzzer as is.

During a run libFuzzer prints lines like this:

```
#52113  NEW    cov: 214 ft: 608 corp: 71/2103b exec/s: 26056
```

`cov` is the number of code points reached and `corp` the number of inputs kept. While `cov`
grows the fuzzer is still finding new code. A run that ends with `Done ... runs` and no report
found nothing.

## CI

The `Fuzz` workflow (`.github/workflows/fuzz.yml`) runs every target of its matrix:

| Trigger | Duration of every target |
|---|---|
| Push to `private/development`, pull request | 60 seconds |
| Every Monday, 03:00 UTC | 1 hour |
| Manual start (`workflow_dispatch`) | Set in the `seconds` input, 600 by default |

A failed run prints the crash inputs as hex in the log and uploads them as the artifact
`fuzz-crashes-<target>`. Download it and continue from step 6 below.

## Adding a fuzz target

The steps below use `<target>` for the name. It is the module name: `src/core_link.cc` gives
`core_link`, and the same word is used for the file, the executable, the corpus directory and
the argument of `fuzzer_run.sh`.

### 1. Decide what to check

Write the list of properties before the code. Take every item that holds for the module:

1. **No access outside of the input**, whatever the input is. Always applies.
2. **Round trip**: what was parsed can be written back, and the result equals the input.
   Use it only if the module guarantees it; a parser that normalizes its input does not.
3. **Size calculation is exact**: the size reported by a dry run is the size actually written.
4. **A too small output buffer is rejected** with an error and nothing is written outside of it.
5. **Consistency**: accepted input leaves the object in a state its own accessors agree on.

An input that the module rejects with an error code is a normal outcome, not a finding.

### 2. Create `fuzz/fuzz_<target>.cc`

Start from this skeleton and keep the order of the parts:

```cpp
#include "<target>.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>

using namespace std;
using namespace coap;

/*
    Checked properties:
    1. ...
    2. ...
*/

static void fail(const char * what, const error_code &ec = error_code())
{
    if (ec)
        fprintf(stderr, "fuzz_<target>: %s: %s\n", what, ec.message().c_str());
    else
        fprintf(stderr, "fuzz_<target>: %s\n", what);
    abort();
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t * data, size_t size)
{
    error_code ec;

    // 1. Feed the input to the module
    // 2. if (ec) return 0;
    // 3. Check the properties, call fail() for a broken one

    return 0;
}
```

Rules for the body:

- **Return 0 for a rejected input.** `fail()` is only for a broken property.
- **Report through `fail()`**, with a sentence that names the property. The text is the first
  thing read in a crash report.
- **Allocate every buffer on the heap with its exact size** (`new uint8_t[size]`). ASan does
  not see an overflow into the unused tail of a larger buffer, and it is less precise on the
  stack.
- **Data kept in a `std::vector` is covered too.** `FUZZ_FLAGS` defines
  `_GLIBCXX_SANITIZE_VECTOR`, so ASan reports an access between the size and the capacity of
  a vector, for example a read past the end of an option value.
- **Be deterministic.** No `rand()`, time or state kept between calls. If a value has to vary,
  derive it from the input, as `pick_size()` in `fuzz_packet.cc` does. A crash file must fail
  the same way on every run.
- **Create the tested objects inside the function**, so one input cannot affect the next.
- **Print nothing on success.**
- The code follows the style of the repository, see `CLAUDE.md`.

A module whose API takes a C string needs a terminated copy, because the input has no
terminating zero:

```cpp
static unique_ptr<char[]> to_c_string(const uint8_t * data, size_t size)
{
    unique_ptr<char[]> text(new char[size + 1]);
    memcpy(text.get(), data, size);
    text[size] = '\0';
    return text;
}
```

### 3. Add the target to `fuzz/CMakeLists.txt`

Append a block of the same shape as `fuzz_packet`. List the fuzz target, the module and only
the sources the module needs to link:

```cmake
#############################################################
# fuzz_<target>
#############################################################

add_executable(
    fuzz_<target>
        ${CMAKE_CURRENT_LIST_DIR}/fuzz_<target>.cc
        ${SRC_DIR}/<target>.cc
        ${SRC_DIR}/error.cc
)

target_include_directories(
    fuzz_<target> PRIVATE
        ${INC_DIR}
        ${SRC_DIR}
        ${CJSON_PATH}
)

target_compile_options(
    fuzz_<target> PRIVATE
        -Wall -Wextra -Wpedantic -Wshadow
        ${FUZZ_FLAGS}
)

target_link_libraries(
    fuzz_<target> PRIVATE
        ${FUZZ_FLAGS}
)
```

- `${SRC_DIR}` in the include directories is needed only for a private header such as
  `base64.h`.
- A module that calls cJSON (`senml_json.cc`) also needs `${CJSON_PATH}/cJSON.c` in the source
  list and the C language in the project line: `project("fuzzer" CXX C)`.
- Do not link the `coapcpp` library of the root project: it is built without instrumentation.

### 4. Add the seed corpus

Create `fuzz/corpus/<target>/` with one file per input:

- one or two valid inputs, taken from the unit tests of the module;
- every input the unit tests expect to be rejected;
- for a text format, at least one seed with every keyword the parser knows (for SenML-JSON:
  `"n"`, `"u"`, `"v"`, `"vs"`, `"vb"`, `"vd"`, ...). The fuzzer rarely guesses a keyword, and
  the code behind a missing one stays untested;
- nothing generated by the fuzzer itself, except the minimized crash files from step 7.

A file is named after what it contains, in `snake_case`: `empty_ack`, `truncated_delta_13`.
There is no extension. Binary seeds are written from hex:

```sh
echo "40 01 00 01 d0" | xxd -r -p > fuzz/corpus/<target>/truncated_delta_13
```

### 5. Run

```sh
./fuzzer_run.sh <target> 60
```

The script needs no change: it finds the target by the name of `fuzz/fuzz_<target>.cc`.
The first run of a new target should be short. If it stops within a second, check the target
before the library: the usual cause is a property from step 1 that the module never promised.

### 6. Analyze the crashes

Crash files are written to `build/fuzz/crashes/<target>/`. Each is an input that can be
replayed without fuzzing:

```sh
./build/fuzz/fuzz_<target> build/fuzz/crashes/<target>/crash-<hash>
xxd build/fuzz/crashes/<target>/crash-<hash>
```

1. **Read the report.** The first line tells the detector, see the table in "How it works".
   In the stack, the first frame inside `src/` is the place of the bug; the `fuzz_<target>.cc`
   frame below it tells which call of the target led there.
2. **Group the files.** Many crash files usually share one cause. Group them by the kind of
   error and the line in `src/`, then fix one cause at a time.
3. **Minimize** one file of the group:

   ```sh
   ./build/fuzz/fuzz_<target> -minimize_crash=1 -runs=100000 \
       -exact_artifact_path=build/fuzz/min build/fuzz/crashes/<target>/crash-<hash>
   ```

### 7. Fix and keep the case

For every cause:

1. Add a regression test to `test/test_<target>.cc` with the minimized input and see it fail.
   The unit tests are built without sanitizers, so the test has to check something visible:
   the error code, or guard bytes placed after the output buffer.
2. Fix the code in `src/`.
3. Replay the crash file; it must finish silently.
4. Copy the minimized file to `fuzz/corpus/<target>/` under a descriptive name.
5. Run the fuzzer again, longer. A fix often uncovers the next bug that the first one hid.

### 8. Add the target to CI

Add the name to the matrix of the `fuzz-smoke` job in `.github/workflows/fuzz.yml`:

```yaml
        target: [packet, <target>]
```

Do it after step 7, when a local run of several minutes finds nothing: a target that still
crashes keeps the workflow red.

### 9. Commit

One logical change per commit:

| Commit | Contents |
|---|---|
| `[FUZZ] Add libFuzzer wrapper for <what is fuzzed>` | `fuzz_<target>.cc`, the CMake block, the seeds, a new row in the table at the top of this file |
| `[<MODULE>] Fix ...` | One commit per fixed cause |
| `[TEST] Add regression test for ...` | The test of that cause |
| `[FUZZ] Add seed for ...` | The minimized crash file |
| `[CI] Add fuzz target <target>` | The new matrix entry |

## Checklist

- [ ] The name `<target>` is the same in the file, the executable, the corpus directory.
- [ ] The comment block at the top lists the checked properties.
- [ ] Rejected input returns 0; `fail()` is called only for a broken property.
- [ ] Every buffer is allocated on the heap with its exact size.
- [ ] No randomness, time or state between calls.
- [ ] `fuzz_<target>.cc` compiles without warnings.
- [ ] Every seed runs without a crash: `./build/fuzz/fuzz_<target> fuzz/corpus/<target>/*`.
- [ ] The target is listed in the table at the top of this file.
- [ ] The target is in the matrix of `.github/workflows/fuzz.yml`.
