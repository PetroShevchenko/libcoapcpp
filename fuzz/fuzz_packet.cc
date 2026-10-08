#include "packet.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>

using namespace std;
using namespace coap;

/*
    Checked properties:
    1. parse() never reads outside of the input, whatever the input is.
    2. A parsed packet is serialized into a buffer of exactly the calculated size.
    3. The serialized packet is byte-for-byte equal to the input.
    4. serialize() into a smaller buffer reports an error and writes nothing outside of it.

    libFuzzer passes the input in a heap block of exactly `size` bytes and every output buffer
    below is allocated with its exact size, so ASan sees any access outside of them.
*/

static void fail(const char * what, const error_code &ec = error_code())
{
    if (ec)
        fprintf(stderr, "fuzz_packet: %s: %s\n", what, ec.message().c_str());
    else
        fprintf(stderr, "fuzz_packet: %s\n", what);
    abort();
}

// A buffer size in [0, limit) that depends only on the input, so every run is reproducible
static size_t pick_size(const uint8_t * data, size_t size, size_t limit)
{
    uint32_t hash = 2166136261u;
    for (size_t i = 0; i < size; i++)
        hash = (hash ^ data[i]) * 16777619u;
    return hash % limit;
}

static void serialize_into_small_buffer(Packet &packet, size_t given)
{
    error_code ec;
    unique_ptr<uint8_t[]> buffer(new uint8_t[given]);
    size_t size = given;

    packet.serialize(ec, buffer.get(), size);
    if (!ec)
        fail("serialize() succeeded with a too small buffer");
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t * data, size_t size)
{
    error_code ec;
    Packet packet;

    packet.parse(data, size, ec);
    if (ec)
        return 0;

    size_t need = 0;
    packet.serialize(ec, nullptr, need, true);
    if (ec)
        fail("size calculation failed for a parsed packet", ec);
    if (need != size)
        fail("calculated size differs from the size of the parsed packet");

    unique_ptr<uint8_t[]> buffer(new uint8_t[need]);
    size_t written = need;

    packet.serialize(ec, buffer.get(), written);
    if (ec)
        fail("serialize() failed with a buffer of the calculated size", ec);
    if (written != size || memcmp(buffer.get(), data, size) != 0)
        fail("serialized packet differs from the parsed one");

    serialize_into_small_buffer(packet, need - 1);
    serialize_into_small_buffer(packet, pick_size(data, size, need));

    return 0;
}
