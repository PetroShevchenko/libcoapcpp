# Unit tests

This directory holds the GoogleTest unit tests of libcoapcpp. All files are built into one
binary, `test_coapcpp`.

| Path | Contents |
|---|---|
| `test_<module>.cc` | Tests of one module, `TEST(test<Module>, camelCaseName)` |
| `test_common.h` | `TestCase` and the declarations of the `print_*` helpers |
| `test_common.cc` | The `print_*` helpers, built only with `PRINT_TESTED_VALUES` |
| `test_packet.cc` | Also holds `main()` |

This file describes how to write a **regression test**: a table-driven negative test that
proves a parser rejects a broken input with the right error code. The reference is
`TEST(testPacket, rejectTruncatedExtendedOption)` in `test_packet.cc`.

## When a regression test is needed

- Every fix in a parser or a serializer in `src/`.
- Every crash found by a fuzzer (see `../fuzz/README.md`, step 7). The minimized crash file
  becomes one case of the test.
- Every new `CoapStatus` that a parser returns.

## Running

```sh
./build.sh
./test_run.sh                                    # all unit tests

# a single test or a group
cd build/POSIX && LD_LIBRARY_PATH=$PWD/lib ./test_coapcpp --gtest_filter='testPacket.reject*'
```

The tests are built with `PRINT_TESTED_VALUES`, so every rejected case prints its name and its
error code through `print_error()`.

## `TestCase`

```cpp
struct TestCase {
    const char * name;              // what is wrong with the input, printed on failure
    std::vector<uint8_t> message;   // the input, exactly as many bytes as the case needs
    CoapStatus expected;            // the error the parser must report
};
```

One `TestCase` is one input. One `TEST` holds every input that breaks the same rule, so a group
of related bugs is checked by one loop and a new input is one more line of the table.

## Writing a regression test

### 1. Pick the name

- The file is the one of the tested module: a bug in `src/packet.cc` goes to `test_packet.cc`.
- The test is called `reject<What>`, where `<What>` names the broken rule, not the bug:
  `rejectTruncatedExtendedOption`, `rejectReservedBlockSize`, `rejectCorruptedPacket`.
- If a `reject*` test for the same rule already exists, add a case to its table instead of
  writing a new test.

### 2. Describe the bytes

Above the data, a `/* */` block says how many inputs there are, what they share and what every
byte of the broken part means:

```cpp
    /*
        There are 4 packets for testing

        Common header of every packet:
        0x40        Version 1, CON, token length 0
        0x01        GET
        0x00, 0x01  Message ID

        Truncated option that follows the header:
        0xD0        Delta 13 requires one more byte; length 0
        0xE0, 0x00  Delta 14 requires two more bytes, only one is present; length 0
        0x0D        Delta 0; length 13 requires one more byte
        0x0E, 0x00  Delta 0; length 14 requires two more bytes, only one is present
    */
```

- Bytes are written in hex, `0x` and upper case, in the order of the frame.
- Every line explains **why** the bytes are wrong: "requires two more bytes, only one is
  present", not just "delta 14".
- A field of the RFC is named as in the RFC: Delta, Length, TKL, SZX, NUM, M.

### 3. Fill the table

```cpp
    const vector<TestCase> cases = {
        {"delta 13, no extended byte",          {0x40, 0x01, 0x00, 0x01, 0xD0},       CoapStatus::COAP_ERR_OPTION_DELTA},
        {"delta 14, one extended byte of two",  {0x40, 0x01, 0x00, 0x01, 0xE0, 0x00}, CoapStatus::COAP_ERR_OPTION_DELTA},
        {"length 13, no extended byte",         {0x40, 0x01, 0x00, 0x01, 0x0D},       CoapStatus::COAP_ERR_OPTION_LENGTH},
        {"length 14, one extended byte of two", {0x40, 0x01, 0x00, 0x01, 0x0E, 0x00}, CoapStatus::COAP_ERR_OPTION_LENGTH},
    };
```

- The table is `const vector<TestCase>` and is called `cases`.
- One case per line, the three columns aligned with spaces.
- The name is a short lower-case phrase about the input, unique within the table.
- **The input ends where the broken field ends.** No padding after it: the parser must fail
  on the last byte, not on a byte that happens to follow it. A stack array or a bigger buffer
  would hide an out-of-bounds read; an exact `std::vector` lets ASan see it when the same
  input is replayed by the fuzzer.
- The error code is written qualified: `CoapStatus::COAP_ERR_OPTION_DELTA`.
- The order of the lines follows the order of the comment block.

An input too long to write by hand is built before the table, with the filler named in the
comment:

```cpp
    // 0x0E, 0xFF, 0xFF  Delta 0; length 65804 (wraps to 268), followed by 268 bytes
    vector<uint8_t> lengthOverflow = {0x40, 0x01, 0x00, 0x01, 0x0E, 0xFF, 0xFF};
    lengthOverflow.resize(lengthOverflow.size() + 268, 0x61);
```

and then used as the `message` of a case: `{"length above 65535", lengthOverflow, CoapStatus::COAP_ERR_OPTION_LENGTH},`.

### 4. Write the loop

```cpp
    for (const auto & tc : cases)
    {
        error_code ec;
        Packet packet;

        packet.parse(tc.message.data(), tc.message.size(), ec);

#ifdef PRINT_TESTED_VALUES
        print_error(tc.name, ec);
#endif

        const error_code expected = make_error_code(tc.expected);

        EXPECT_EQ(ec, expected)
            << "  TC name:  " << tc.name << "\n"
            << "  actual:   " << ec.message() << "\n"
            << "  expected: " << expected.message();
    }
```

- **The tested objects are created inside the loop**, so one case cannot affect the next.
- The size passed to the parser is `tc.message.size()`, never a constant.
- `print_error()` goes right after the tested call, inside `#ifdef PRINT_TESTED_VALUES`.
- **The check compares the whole `error_code`**, not `ec.value()` or `bool(ec)`: an input
  rejected for the wrong reason is a failure too.
- The failure message prints the case name, the actual and the expected message, so the
  failed case is found without a debugger.

### 5. Add the checks the bug needs

The error code is often not enough. The unit tests are built without sanitizers, so a test
checks what the bug made visible:

| Bug | Extra check | Example |
|---|---|---|
| The input is valid for an earlier step | `ASSERT_FALSE(ec)` after that step | `rejectReservedBlockSize`: `packet.parse()` must pass before `block1.get_header()` |
| A function also returns `bool` | `EXPECT_FALSE(status)` | `rejectReservedBlockSize` |
| A wrapped value was accepted | Check the parsed object | `rejectOptionNumberAndLengthOverflow`: no option became `URI_PATH` |
| A write outside of an output buffer | Guard bytes after the buffer, compared after the call | `rejectTooSmallBufferOnSerialize` |

- `ASSERT_*` is for preconditions of the case, `EXPECT_*` for the checks themselves.
- An inner loop (for example over buffer sizes) adds its variable to the failure message:
  `<< "  TC name:  " << tc.name << ", buffer size " << given << "\n"`.

### 6. See it fail, then fix

1. Add the test before the fix and run it. Every new case must fail; a case that passes
   before the fix does not test the bug.
2. Fix the code in `src/`.
3. Run the test group and then all tests:

   ```sh
   cd build/POSIX && LD_LIBRARY_PATH=$PWD/lib ./test_coapcpp --gtest_filter='testPacket.reject*'
   ./test_run.sh
   ```

4. If the module has a fuzz target, copy every new input to `fuzz/corpus/<target>/` under a
   `snake_case` name of what it contains (`truncated_delta_13`) and replay the corpus:

   ```sh
   echo "40 01 00 01 d0" | xxd -r -p > fuzz/corpus/packet/truncated_delta_13
   ./build/fuzz/fuzz_packet fuzz/corpus/packet/*
   ```

### 7. Commit

One logical change per commit:

| Commit | Contents |
|---|---|
| `[STATUS] Add ...` | A new error code, if the fix needs one: `api/error.h`, `src/error.cc` |
| `[<MODULE>] Fix ...` | The fix in `src/` |
| `[TEST] Add regression tests for ...` | The new test or the new cases |
| `[FUZZ] Add seed for ...` | The inputs copied to `fuzz/corpus/<target>/` |

## Template

```cpp
TEST(test<Module>, reject<What>)
{
    /*
        There are N packets for testing

        Common header of every packet:
        0x40        Version 1, CON, token length 0
        0x01        GET
        0x00, 0x01  Message ID

        <Part> that follows the header:
        0x..        <field>; <why it is wrong>
    */
    const vector<TestCase> cases = {
        {"<what is wrong>", {0x40, 0x01, 0x00, 0x01, 0x..}, CoapStatus::COAP_ERR_...},
    };

    for (const auto & tc : cases)
    {
        error_code ec;
        <Object> object;

        object.<parse>(tc.message.data(), tc.message.size(), ec);

#ifdef PRINT_TESTED_VALUES
        print_error(tc.name, ec);
#endif

        const error_code expected = make_error_code(tc.expected);

        EXPECT_EQ(ec, expected)
            << "  TC name:  " << tc.name << "\n"
            << "  actual:   " << ec.message() << "\n"
            << "  expected: " << expected.message();
    }
}
```

## Checklist

- [ ] The test is in `test_<module>.cc` and is called `reject<What>` after the broken rule.
- [ ] The `/* */` block gives the number of inputs and explains every byte of the broken part.
- [ ] Every input is a `std::vector` that ends at the broken field.
- [ ] Case names are unique and say what is wrong with the input.
- [ ] The objects under test are created inside the loop.
- [ ] The whole `error_code` is compared, and the failure message prints the case name.
- [ ] Every new case failed before the fix and passes after it.
- [ ] The test compiles without warnings with gcc and clang.
- [ ] The new inputs are in `fuzz/corpus/<target>/`, if the module has a fuzz target.
