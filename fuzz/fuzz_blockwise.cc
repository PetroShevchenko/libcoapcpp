#include "blockwise.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <vector>

using namespace std;
using namespace coap;

/*
    The input is a CoAP packet. It is parsed, then its Block1, Block2, Size1 and Size2 options
    are decoded.

    Checked properties:
    1. Decoding never reads outside of the option value, whatever the option is.
    2. A decoded block has a number below 2^20 and an offset equal to number * size.
    3. A decoded block option can be encoded again, and decoding of the result gives
       the same number, size and more bit.
    4. A decoded Size1 or Size2 is equal to the unsigned integer in the option value.

    An option value is kept in a std::vector. _GLIBCXX_SANITIZE_VECTOR in CMakeLists.txt makes
    ASan see an access between the size and the capacity of the vector.
*/

static const uint32_t BLOCK_NUMBER_LIMIT = 1048576; // 2^20, the number takes 20 bits at most

static void fail(const char * what, const error_code &ec = error_code())
{
    if (ec)
        fprintf(stderr, "fuzz_blockwise: %s: %s\n", what, ec.message().c_str());
    else
        fprintf(stderr, "fuzz_blockwise: %s\n", what);
    abort();
}

// Block is Block1 or Block2
template <typename Block>
static void check_block_option(Packet &packet)
{
    error_code ec;
    Block decoded;

    if (!decoded.get_header(packet, &ec))
        return;

    if (decoded.number() >= BLOCK_NUMBER_LIMIT)
        fail("decoded block number does not fit into 20 bits");
    if (decoded.offset() != static_cast<uint64_t>(decoded.number()) * decoded.size())
        fail("decoded offset differs from number * size");

    Option opt;

    if (!decoded.encode_block_option(opt))
        fail("a decoded block option cannot be encoded again");

    Packet encoded;

    encoded.add_option(static_cast<OptionNumber>(opt.number()), opt.value().data(), opt.value().size(), ec);
    if (ec)
        fail("an encoded block option cannot be added to a packet", ec);

    Block again;

    ec.clear();
    if (!again.get_header(encoded, &ec))
        fail("an encoded block option cannot be decoded", ec);
    if (again.number() != decoded.number())
        fail("block number differs after encoding and decoding");
    if (again.size() != decoded.size())
        fail("block size differs after encoding and decoding");
    if (again.more() != decoded.more())
        fail("more bit differs after encoding and decoding");
}

static void check_size_option(Packet &packet, OptionNumber number)
{
    vector<Option *> options;

    packet.find_option(number, options);

    for (const Option * opt : options)
    {
        Block1 block;

        if (!block.decode_size_option(*opt))
            continue;

        uint64_t expected = 0;
        for (uint8_t byte : opt->value())
            expected = (expected << 8) | byte;

        if (block.total() != expected)
            fail("decoded total size differs from the value of the option");
    }
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t * data, size_t size)
{
    error_code ec;
    Packet packet;

    packet.parse(data, size, ec);
    if (ec)
        return 0;

    check_block_option<Block1>(packet);
    check_block_option<Block2>(packet);
    check_size_option(packet, SIZE_1);
    check_size_option(packet, SIZE_2);

    return 0;
}
