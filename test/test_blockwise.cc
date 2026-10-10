#include "packet.h"
#include "blockwise.h"
#include "test_common.h"
#include <gtest/gtest.h>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>
#include <cstdint>
#include <cstring>

using namespace std;
using namespace coap;
using namespace spdlog;

static uint8_t payload[2000];

TEST(testBlockwise, block1SetHeader)
{
    error_code ec;
    Block1 block1;
    Packet packet;

    UriPath uri("data/firmware.bin", ec);
    ASSERT_TRUE(!ec.value());

    memset(payload, 0xAA, sizeof(payload));
    block1.total(sizeof(payload));
    block1.more(true);
    block1.offset(0);
    block1.size(1024);

    bool status = block1.set_header (5683, uri, packet);
    ASSERT_EQ(status, true);

    packet.make_request(ec, CONFIRMABLE, PUT, generate_identity(), payload + block1.offset(), block1.size());
    ASSERT_TRUE(!ec.value());

    size_t size;

    packet.serialize(ec, nullptr, size, true);
    ASSERT_TRUE(!ec.value());

    uint8_t * buffer = new uint8_t [size];

    packet.serialize(ec, buffer, size);
    ASSERT_TRUE(!ec.value());

#ifdef PRINT_TESTED_VALUES
    print_packet(packet);
    print_serialized_packet(buffer, size);
#endif

    delete [] buffer;

    block1.offset(block1.size());
    block1.size(block1.total() - block1.offset());
    block1.more(false);

    status = block1.set_header (5683, uri, packet);
    ASSERT_EQ(status, true);

    packet.make_request(ec, CONFIRMABLE, PUT, generate_identity(), payload + block1.offset(), block1.size());
    ASSERT_TRUE(!ec.value());

    packet.serialize(ec, nullptr, size, true);
    ASSERT_TRUE(!ec.value());

    buffer = new uint8_t [size];

    packet.serialize(ec, buffer, size);
    ASSERT_TRUE(!ec.value());

#ifdef PRINT_TESTED_VALUES
    print_packet(packet);
    print_serialized_packet(buffer, size);
#endif

    delete [] buffer;

}

TEST(testBlockwise, block2SetHeader)
{
    error_code ec;
    Block2 block2;
    Packet packet;

    UriPath uri("data/firmware.bin", ec);
    ASSERT_TRUE(!ec.value());

    size_t size = 700;

    block2.total(size);
    block2.size(1024);
    block2.offset(0);

    bool status = block2.set_header (5683, uri, packet);
    ASSERT_EQ(status, true);

    packet.make_request(ec, CONFIRMABLE, GET, generate_identity(), nullptr, 0);
    ASSERT_TRUE(!ec.value());

    block2.more(false);

    packet.serialize(ec, nullptr, size, true);
    ASSERT_TRUE(!ec.value());

    uint8_t * buffer = new uint8_t [size];

    packet.serialize(ec, buffer, size);
    ASSERT_TRUE(!ec.value());

#ifdef PRINT_TESTED_VALUES
    print_packet(packet);
    print_serialized_packet(buffer, size);
#endif

    delete [] buffer;
}

TEST(testBlockwise, block1GetHeader)
{
    error_code ec;
    Block1 block1;
    Packet packet;

    UriPath uri("data/firmware.bin", ec);
    ASSERT_TRUE(!ec.value());


    block1.total(sizeof(payload));
    block1.more(true);
    block1.size(1024);

    bool status = block1.set_header (5683, uri, packet);
    ASSERT_EQ(status, true);

    memset(payload, 0xAA, sizeof(payload));

    packet.make_request(ec, CONFIRMABLE, PUT, generate_identity(), payload + block1.offset(), block1.size());
    ASSERT_TRUE(!ec.value());

    status =  block1.get_header(packet);
    ASSERT_EQ(status, true);
    ASSERT_EQ(block1.size(), 1024);
    ASSERT_EQ(block1.number(), 0);
    ASSERT_EQ(block1.more(), true);

#ifdef PRINT_TESTED_VALUES
    info("Decoded BLOCK1 option:");
    info("Block size : {0:d}", block1.size());
    info("Block number : {0:d}", block1.number());
    info("More bit : {}", block1.more());
#endif

}

TEST(testBlockwise, decodeBlockOption)
{
    Option opt;
    opt.number(SIZE_1);
    opt.value().push_back(50);    
    Block1 block1;

    bool status = block1.decode_size_option(opt);
    ASSERT_EQ(status, true);

#ifdef PRINT_TESTED_VALUES
    info("Decoded SIZE_1 option:");
    info("total size: {0:0}", block1.total());
#endif
    ASSERT_EQ(block1.total(), 50);

}

TEST(testBlockwise, rejectReservedBlockSize)
{
    /*
        There are 3 packets for testing

        Common header of every packet:
        0x40        Version 1, CON, token length 0
        0x03        PUT
        0x00, 0x01  Message ID

        Block1 option that follows the header, SZX 7 is reserved by RFC7959:
        0xD1, 0x0E, 0x07              Delta 27 (Block1); length 1; NUM 0, M 0, SZX 7
        0xD2, 0x0E, 0x01, 0x2F        Delta 27 (Block1); length 2; NUM 18, M 1, SZX 7
        0xD3, 0x0E, 0x12, 0x34, 0x57  Delta 27 (Block1); length 3; NUM 74565, M 0, SZX 7
    */
    const vector<TestCase> cases = {
        {"one byte option",    {0x40, 0x03, 0x00, 0x01, 0xD1, 0x0E, 0x07},             CoapStatus::COAP_ERR_DECODE_BLOCK_OPTION},
        {"two bytes option",   {0x40, 0x03, 0x00, 0x01, 0xD2, 0x0E, 0x01, 0x2F},       CoapStatus::COAP_ERR_DECODE_BLOCK_OPTION},
        {"three bytes option", {0x40, 0x03, 0x00, 0x01, 0xD3, 0x0E, 0x12, 0x34, 0x57}, CoapStatus::COAP_ERR_DECODE_BLOCK_OPTION},
    };

    for (const auto & tc : cases)
    {
        error_code ec;
        Packet packet;
        Block1 block1;

        packet.parse(tc.message.data(), tc.message.size(), ec);
        ASSERT_FALSE(ec) << "  actual: " << ec.message();

        bool status = block1.get_header(packet, &ec);

#ifdef PRINT_TESTED_VALUES
        print_error(tc.name, ec);
#endif

        const error_code expected = make_error_code(tc.expected);

        EXPECT_FALSE(status);
        EXPECT_EQ(ec, expected)
            << "  actual:   " << ec.message() << "\n"
            << "  expected: " << expected.message();
    }
}
