#include "packet.h"
#include "test_common.h"
#include <gtest/gtest.h>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>
#include <spdlog/fmt/bin_to_hex.h>
#include <cstdint>
#include <cstring>

using namespace std;
using namespace coap;
using namespace spdlog;

/*
    RFC7252 : CoAp frame format

    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |Ver| T |  TKL  |      Code     |          Message ID           |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |   Token (if any, TKL bytes) ...
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |   Options (if any) ...
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |1 1 1 1 1 1 1 1|    Payload (if any) ...
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*/

static const uint8_t testCoapPacket[] = {// in network order
    0x44, 0x02, 0x13, 0xe9, 0xe9, 0x13, 0xa3, 0x3f, 0xb2, 0x72, 0x64, 0x11, 0x28, 0x39, 0x6c, 0x77,
    0x6d, 0x32, 0x6d, 0x3d, 0x31, 0x2e, 0x31, 0x0d, 0x01, 0x65, 0x70, 0x3d, 0x74, 0x65, 0x73, 0x74,
    0x5f, 0x63, 0x6c, 0x69, 0x65, 0x6e, 0x74, 0x03, 0x62, 0x3d, 0x55, 0x06, 0x6c, 0x74, 0x3d, 0x33,
    0x36, 0x30, 0xff, 0x3c, 0x2f, 0x3e, 0x3b, 0x72, 0x74, 0x3d, 0x22, 0x6f, 0x6d, 0x61, 0x2e, 0x6c,
    0x77, 0x6d, 0x32, 0x6d, 0x22, 0x3b, 0x63, 0x74, 0x3d, 0x31, 0x31, 0x30, 0x2c, 0x3c, 0x2f, 0x31,
    0x2f, 0x30, 0x3e, 0x2c, 0x3c, 0x2f, 0x32, 0x2f, 0x30, 0x3e, 0x2c, 0x3c, 0x2f, 0x33, 0x2f, 0x30,
    0x3e, 0x2c, 0x3c, 0x2f, 0x34, 0x2f, 0x30, 0x3e, 0x2c, 0x3c, 0x2f, 0x35, 0x2f, 0x30, 0x3e, 0x2c,
    0x3c, 0x2f, 0x36, 0x2f, 0x30, 0x3e, 0x2c, 0x3c, 0x2f, 0x37, 0x2f, 0x30, 0x3e, 0x2c, 0x3c, 0x2f,
    0x33, 0x31, 0x30, 0x32, 0x34, 0x2f, 0x31, 0x30, 0x3e, 0x2c, 0x3c, 0x2f, 0x33, 0x31, 0x30, 0x32,
    0x34, 0x2f, 0x31, 0x31, 0x3e, 0x2c, 0x3c, 0x2f, 0x33, 0x31, 0x30, 0x32, 0x34, 0x2f, 0x31, 0x32,
    0x3e
};

static const OptionNumber testOptionSet[] = {
    IF_MATCH,
    URI_HOST,
    ETAG,
    IF_NONE_MATCH,
    URI_PORT,
    LOCATION_PATH,
    URI_PATH,
    CONTENT_FORMAT,
    MAX_AGE,
    URI_QUERY,
    ACCEPT,
    LOCATION_QUERY,
    BLOCK_2,
    BLOCK_1,
    SIZE_2,
    PROXY_URI,
    PROXY_SCHEME,
    SIZE_1
};

static uint8_t testOptionValue[32] = {0};

TEST(testPacket, parse)
{
    error_code ec;
    Packet packet;

    packet.parse(testCoapPacket, sizeof(testCoapPacket), ec);
    ASSERT_TRUE(!ec.value());

#ifdef PRINT_TESTED_VALUES
    print_packet(packet);
#endif

    EXPECT_EQ(packet.version(), COAP_VERSION);
    EXPECT_EQ(packet.type(), CONFIRMABLE);
    EXPECT_EQ(packet.token_length(), 4UL);
    EXPECT_EQ(packet.code_as_byte(), POST);
    EXPECT_EQ(packet.code_class(), 0);
    EXPECT_EQ(packet.code_detail(), 2);
    EXPECT_EQ(packet.identity(), 5097);

    int r = memcmp(&testCoapPacket[PACKET_HEADER_SIZE], packet.token().data(), packet.token_length());

    EXPECT_TRUE(r == 0);

    r = memcmp(&testCoapPacket[packet.payload_offset()], packet.payload().data(), packet.payload().size());

    EXPECT_TRUE(r == 0);
}

TEST(testPacket, parseNoPayload)
{
    error_code ec;
    Packet packet;
    const size_t noPayloadPacketSize = 50;
    vector<uint8_t> testCoapPacketWithoutPayload(testCoapPacket, testCoapPacket + noPayloadPacketSize);

    packet.parse(testCoapPacketWithoutPayload.data(), testCoapPacketWithoutPayload.size(), ec);
    ASSERT_TRUE(!ec.value());

#ifdef PRINT_TESTED_VALUES
    print_packet(packet);
#endif

    EXPECT_EQ(packet.version(), COAP_VERSION);
    EXPECT_EQ(packet.type(), CONFIRMABLE);
    EXPECT_EQ(packet.token_length(), 4UL);
    EXPECT_EQ(packet.code_as_byte(), POST);
    EXPECT_EQ(packet.code_class(), 0);
    EXPECT_EQ(packet.code_detail(), 2);
    EXPECT_EQ(packet.identity(), 5097);

    int r = memcmp(&testCoapPacketWithoutPayload[PACKET_HEADER_SIZE], packet.token().data(), packet.token_length());

    EXPECT_TRUE(r == 0);

    EXPECT_EQ(packet.options().size(), 6UL);
    EXPECT_TRUE(packet.payload().empty());
    EXPECT_EQ(packet.payload_offset(), 0);
}

TEST(testPacket, parseNoOptionsNoPayload)
{
    /*
        0x60        Version 1, ACK, token length 0
        0x00        Empty message (0.00)
        0x12, 0x34  Message ID
    */
    const vector<uint8_t> message = {0x60, 0x00, 0x12, 0x34};

    error_code ec;
    Packet packet;

    packet.parse(message.data(), message.size(), ec);

#ifdef PRINT_TESTED_VALUES
    print_error("without options and payload", ec);
#endif

    ASSERT_FALSE(ec) << "  actual: " << ec.message();

    EXPECT_EQ(packet.version(), 1);
    EXPECT_EQ(packet.token_length(), 0);
    EXPECT_EQ(packet.identity(), 0x1234);
    EXPECT_EQ(packet.type(), 0x2);
    EXPECT_EQ(packet.code_as_byte(), 0x00);
}

TEST(testPacket, rejectPayloadMarkerWithoutPayload)
{
    const vector<vector<uint8_t>> messages = {
        {0x40, 0x01, 0x00, 0x01, 0xFF},                        // marker right after the header
        vector<uint8_t>(testCoapPacket, testCoapPacket + 51),  // marker after the options
    };

    for (const auto & message : messages)
    {
        error_code ec;
        Packet packet;

        packet.parse(message.data(), message.size(), ec);

#ifdef PRINT_TESTED_VALUES
        print_error(ec);
#endif

        EXPECT_EQ(ec, make_error_code(CoapStatus::COAP_ERR_NO_PAYLOAD));
    }
}

/*
    RFC7252 : Option format

     0   1   2   3   4   5   6   7
   +---------------+---------------+
   |               |               |
   |  Option Delta | Option Length |   1 byte
   |               |               |
   +---------------+---------------+
   \                               \
   /         Option Delta          /   0-2 bytes
   \          (extended)           \
   +-------------------------------+
   \                               \
   /         Option Length         /   0-2 bytes
   \          (extended)           \
   +-------------------------------+
   \                               \
   /                               /
   \                               \
   /         Option Value          /   0 or more bytes
   \                               \
   /                               /
   \                               \
   +-------------------------------+
*/
TEST(testPacket, findOption)
{
    error_code ec;
    Packet packet;

    packet.parse(testCoapPacket, sizeof(testCoapPacket), ec);
    ASSERT_TRUE(!ec.value());

    vector<Option *> optList;
    size_t quantity;

    for(auto o : testOptionSet)
    {
        quantity = packet.find_option(o, optList);

#ifdef PRINT_TESTED_VALUES
        info("Searched option is {0:d}", o);
        if (!quantity)
            info("option is not presented");
        else
        {
            size_t index = 0;
            info("option number: {0:d}", optList[index]->number());
            info("option quantity : {0:d}", quantity);
            do
            {
                info("option sequens number : {0:d}", index);
                info("option value length : {0:d}",(optList[index])->length());
                info("option value delta : {0:d}",(optList[index])->delta());
                info("option value: ");
                fmt::print("{:02x}", fmt::join((optList[index])->value(), ", "));
                fmt::print("\n");
            } while (++index < quantity);
        }
        info("============================");
#endif

        switch(o)
        {
            case URI_PATH:
            {
                EXPECT_EQ(quantity, 1UL);
                EXPECT_EQ(quantity, optList.size());
                EXPECT_EQ(optList[0]->number(), o);
                break;
            }
            case CONTENT_FORMAT:
            {
                EXPECT_EQ(quantity, 1UL);
                EXPECT_EQ(quantity, optList.size());
                EXPECT_EQ(optList[0]->number(), o);
                break;
            }
            case URI_QUERY:
            {
                EXPECT_EQ(quantity, 4UL);
                EXPECT_EQ(quantity, optList.size());
                EXPECT_EQ(optList[0]->number(), o);
                EXPECT_EQ(optList[1]->number(), o);
                EXPECT_EQ(optList[2]->number(), o);
                EXPECT_EQ(optList[3]->number(), o);
                break;
            }
            default:
            {
                EXPECT_TRUE(quantity == 0);
                break;
            }
        }
    }
}

TEST(testPacket, rejectTruncatedExtendedOption)
{
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
    const vector<TestCase> cases = {
        {"delta 13, no extended byte",          {0x40, 0x01, 0x00, 0x01, 0xD0},       CoapStatus::COAP_ERR_OPTION_DELTA},
        {"delta 14, one extended byte of two",  {0x40, 0x01, 0x00, 0x01, 0xE0, 0x00}, CoapStatus::COAP_ERR_OPTION_DELTA},
        {"length 13, no extended byte",         {0x40, 0x01, 0x00, 0x01, 0x0D},       CoapStatus::COAP_ERR_OPTION_LENGTH},
        {"length 14, one extended byte of two", {0x40, 0x01, 0x00, 0x01, 0x0E, 0x00}, CoapStatus::COAP_ERR_OPTION_LENGTH},
    };

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
            << "  actual:   " << ec.message() << "\n"
            << "  expected: " << expected.message();
    }
}

TEST(testPacket, rejectOptionNumberAndLengthOverflow)
{
    /*
        There are 5 packets for testing

        Common header of every packet:
        0x40        Version 1, CON, token length 0
        0x01        GET
        0x00, 0x01  Message ID

        Option that follows the header:
        0xE1, 0xFE, 0xFE, 0x78  Delta 65547 (wraps to 11, Uri-Path); length 1, value 'x'
        0xE0, 0xFF, 0xFF        Delta 65804 (wraps to 268); length 0
        0xE0, 0xE9, 0x53        Delta 60000; length 0
        0xE0, 0x16, 0x63        Delta 6000, option number 66000 (wraps to 464); length 0
        0x0E, 0xFF, 0xFF        Delta 0; length 65804 (wraps to 268), followed by 268 bytes
    */
    vector<uint8_t> lengthOverflow = {0x40, 0x01, 0x00, 0x01, 0x0E, 0xFF, 0xFF};
    lengthOverflow.resize(lengthOverflow.size() + 268, 0x61);

    const vector<TestCase> cases = {
        {"delta wraps to Uri-Path",       {0x40, 0x01, 0x00, 0x01, 0xE1, 0xFE, 0xFE, 0x78},             CoapStatus::COAP_ERR_OPTION_DELTA},
        {"delta above 65535",             {0x40, 0x01, 0x00, 0x01, 0xE0, 0xFF, 0xFF},                   CoapStatus::COAP_ERR_OPTION_DELTA},
        {"accumulated number above 65535",{0x40, 0x01, 0x00, 0x01, 0xE0, 0xE9, 0x53, 0xE0, 0x16, 0x63}, CoapStatus::COAP_ERR_OPTION_DELTA},
        {"length above 65535",            lengthOverflow,                                               CoapStatus::COAP_ERR_OPTION_LENGTH},
    };

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
            << "  actual:   " << ec.message() << "\n"
            << "  expected: " << expected.message();

        for (const auto & opt : packet.options())
            EXPECT_NE(opt.number(), URI_PATH) << "a wrapped option number was accepted";
    }
}

TEST(testPacket, rejectCorruptedPacket)
{
    /*
        There are 4 packets for testing

        Common header of every packet:
        0x40        Version 1, CON, token length 0
        0x01        GET
        0x00, 0x01  Message ID

        Option that follows the header:
        05 61 - error: option length exceeds the remainder
        40 01 00 01 F0 - error: reserved nibble 15
        49 01 00 01 - error: token length 9
        40 01 00 - error: too short packet
    */
    const vector<TestCase> cases = {
        {"length exceeds the remainder",  {0x40, 0x01, 0x00, 0x01, 0x05, 0x61},     CoapStatus::COAP_ERR_OPTION_LENGTH},
        {"reserved nibble 15",            {0x40, 0x01, 0x00, 0x01, 0xF0},           CoapStatus::COAP_ERR_OPTION_DELTA},
        {"token length 9",                {0x49, 0x01, 0x00, 0x01},                 CoapStatus::COAP_ERR_TOKEN_LENGTH},
        {"too short packet",              {0x40, 0x01, 0x00},                       CoapStatus::COAP_ERR_PACKET_LENGTH},
    };

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
            << "  actual:   " << ec.message() << "\n"
            << "  expected: " << expected.message();
    }
}

TEST(testPacket, parseMaxOptionNumber)
{
    // 0xE0, 0xFE, 0xF2: delta 65535, the largest valid option number; length 0
    const vector<uint8_t> message = {0x40, 0x01, 0x00, 0x01, 0xE0, 0xFE, 0xF2};

    error_code ec;
    Packet packet;

    packet.parse(message.data(), message.size(), ec);

    ASSERT_TRUE(!ec.value());
    ASSERT_EQ(packet.options().size(), 1UL);
    EXPECT_EQ(packet.options()[0].number(), 65535);
}

static void set_testOptionValue(uint8_t offset)
{
    for(size_t i = 0; i < sizeof(testOptionValue); i++)
    {
        testOptionValue[i] = static_cast<uint8_t>(offset + i);
    }
}

static void create_testOptions(Packet & packet, error_code & ec)
{
    uint8_t offset = 0;

    for(auto o : testOptionSet)
    {
        set_testOptionValue(offset);
        offset += 0x10;
        packet.add_option(o, testOptionValue, sizeof(testOptionValue), ec);
        if (ec.value())
            return;
    }
}

TEST(testPacket, addOption)
{
    error_code ec;
    Packet packet;

    packet.parse(testCoapPacket, sizeof(testCoapPacket), ec);
    ASSERT_TRUE(!ec.value());

    packet.options().clear();

    create_testOptions(packet, ec);

    EXPECT_TRUE(ec.value() == 0);

#ifdef PRINT_TESTED_VALUES
    print_options(packet);
#endif
}

TEST(testPacket, isLittleEndianByteOrder)
{
#if (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__) || (__LITTLE_ENDIAN__ == 1)
    ASSERT_TRUE(is_little_endian_byte_order());
#elif (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__) || (__BIG_ENDIAN__ == 1)
    ASSERT_FALSE(is_little_endian_byte_order());
#else
    #error "The byte order is undefined or a compiler other than GCC is used"
#endif
}

TEST(testPacket, generateIdentity)
{
    uint16_t uniqueId1 = generate_identity();
    uint16_t uniqueId2 = generate_identity();

#ifdef PRINT_TESTED_VALUES
    info("unique indentity 1 : {0:x}", uniqueId1);
    info("unique indentity 2 : {0:x}", uniqueId2);
#endif
    ASSERT_NE(uniqueId1, uniqueId2);
}

TEST(testPacket, generateToken)
{
    Packet packet;
    bool r = packet.generate_token(TOKEN_MAX_LENGTH);
    ASSERT_TRUE(r);

    uint8_t token[TOKEN_MAX_LENGTH];
    memcpy(token, packet.token().data(), TOKEN_MAX_LENGTH);

    r = packet.generate_token(TOKEN_MAX_LENGTH);
    ASSERT_TRUE(r);

#ifdef PRINT_TESTED_VALUES
    info("generated unique token 1 :");
    fmt::print("{:02x}", fmt::join(token, ", "));
    fmt::print("\n");
    info("generated unique token 2 :");
    fmt::print("{:02x}", fmt::join(packet.token(), ", "));
    fmt::print("\n");
#endif

    int r2 = memcmp(token, packet.token().data(), TOKEN_MAX_LENGTH);

    ASSERT_TRUE(r2 != 0);
}

TEST(testPacket, makeRequest)
{
    error_code ec;

    Packet packet;

    create_testOptions(packet, ec);

    ASSERT_TRUE(!ec.value());

    uint16_t id = generate_identity();

    packet.make_request(ec, CONFIRMABLE, PUT, id, nullptr, 0);

    ASSERT_TRUE(!ec.value());

#ifdef PRINT_TESTED_VALUES
    info("Prepared request:");
    print_packet(packet);
#endif

    ASSERT_EQ(packet.token_length(), TOKEN_MAX_LENGTH);
    ASSERT_EQ(id, packet.identity());
    ASSERT_EQ(packet.type(), CONFIRMABLE);
    ASSERT_EQ(packet.code_as_byte(),PUT);
}

TEST(testPacket, prepareAnswer)
{
    error_code ec;

    Packet packet;

    create_testOptions(packet, ec);

    ASSERT_TRUE(!ec.value());

    uint16_t id = generate_identity();

    packet.make_request(ec, CONFIRMABLE, PUT, id, nullptr, 0);

    ASSERT_TRUE(!ec.value());

    ASSERT_EQ(packet.token_length(), TOKEN_MAX_LENGTH);
    ASSERT_EQ(id, packet.identity());
    ASSERT_EQ(packet.type(), CONFIRMABLE);
    ASSERT_EQ(packet.code_as_byte(),PUT);

    id = generate_identity();

    packet.prepare_answer(ec, ACKNOWLEDGEMENT, PUT, id, nullptr, 0);

    ASSERT_TRUE(!ec.value());

#ifdef PRINT_TESTED_VALUES
    info("Prepared answer:");
    print_packet(packet);
#endif

    ASSERT_EQ(packet.token_length(), TOKEN_MAX_LENGTH);
    ASSERT_EQ(id, packet.identity());
    ASSERT_EQ(packet.type(), ACKNOWLEDGEMENT);
    ASSERT_EQ(packet.code_as_byte(),PUT);
}

TEST(testPacket, serialize)
{
    error_code ec;

    Packet packet;

    ec.clear();

{
    uint8_t value[] = { 0x72, 0x64 };
    packet.add_option(URI_PATH, value, sizeof(value), ec);
    ASSERT_TRUE(!ec.value());
}

{
    uint8_t value[] = { 0x28 };
    packet.add_option(CONTENT_FORMAT, value, sizeof(value), ec);
    ASSERT_TRUE(!ec.value());
}

{
    uint8_t value[] = { 0x6c, 0x77, 0x6d, 0x32, 0x6d, 0x3d, 0x31, 0x2e, 0x31 };
    packet.add_option(URI_QUERY, value, sizeof(value), ec);
    ASSERT_TRUE(!ec.value());
}

{
    uint8_t value[] = { 0x65, 0x70, 0x3d, 0x74, 0x65, 0x73, 0x74, 0x5f, 0x63, 0x6c, 0x69, 0x65, 0x6e, 0x74 };
    packet.add_option(URI_QUERY, value, sizeof(value), ec);
    ASSERT_TRUE(!ec.value());
}

{
    uint8_t value[] = { 0x62, 0x3d, 0x55 };
    packet.add_option(URI_QUERY, value, sizeof(value), ec);
    ASSERT_TRUE(!ec.value());
}

{
    uint8_t value[] = { 0x6c, 0x74, 0x3d, 0x33, 0x36, 0x30 };
    packet.add_option(URI_QUERY, value, sizeof(value), ec);
    ASSERT_TRUE(!ec.value());
}

    uint16_t id = generate_identity();

    packet.make_request(ec, CONFIRMABLE, POST, id, &testCoapPacket[51], 110);

    ASSERT_TRUE(!ec.value());

    size_t size;

    packet.serialize(ec, nullptr, size, true);

    ASSERT_TRUE(!ec.value());

    uint8_t * buffer = new uint8_t [size];

    packet.serialize(ec, buffer, size);

    ASSERT_TRUE(!ec.value());

#ifdef PRINT_TESTED_VALUES
    print_serialized_packet(buffer, size);
#endif

    delete [] buffer;
}

TEST(testPacket, serializeIntoExactBuffer)
{
    /*
        There are 4 packets for testing, the first three end with an option that has no value

        Common header of the first three packets:
        0x40        Version 1, CON, token length 0
        0x01        GET
        0x00, 0x01  Message ID

        Option that follows the header:
        0x00              Delta 0 (If-Match); length 0
        0xD0, 0x01        Delta 14 (Max-Age), one extended byte; length 0
        0xE0, 0x00, 0x00  Delta 269, two extended bytes; length 0
    */
    const vector<vector<uint8_t>> messages = {
        {0x40, 0x01, 0x00, 0x01, 0x00},
        {0x40, 0x01, 0x00, 0x01, 0xD0, 0x01},
        {0x40, 0x01, 0x00, 0x01, 0xE0, 0x00, 0x00},
        vector<uint8_t>(testCoapPacket, testCoapPacket + sizeof(testCoapPacket)),  // token, options and payload
    };

    for (const auto & message : messages)
    {
        error_code ec;
        Packet packet;

        packet.parse(message.data(), message.size(), ec);
        ASSERT_FALSE(ec) << "  actual: " << ec.message();

        size_t size = 0;

        packet.serialize(ec, nullptr, size, true);
        ASSERT_FALSE(ec) << "  actual: " << ec.message();

        vector<uint8_t> buffer(size);

        packet.serialize(ec, buffer.data(), size);

#ifdef PRINT_TESTED_VALUES
        print_error(ec);
#endif

        EXPECT_FALSE(ec) << "  actual: " << ec.message();
        EXPECT_EQ(buffer, message);
    }
}

TEST(testPacket, rejectTooSmallBufferOnSerialize)
{
    /*
        There are 4 packets for testing

        Common header of every packet except the third one:
        0x40        Version 1, CON, token length 0
        0x01        GET
        0x00, 0x01  Message ID

        The rest of the packet:
        0x00                          Delta 0 (If-Match); length 0
        0xDD, 0x01, 0x00              Delta 14 and length 13, one extended byte each, followed by 13 bytes
        0xEE, 0x00, 0x00, 0x00, 0x00  Delta 269 and length 269, two extended bytes each, followed by 269 bytes

        The third packet:
        0x42, 0x01, 0x00, 0x01  The same header with token length 2
        0x11, 0x22              Token
        0xB1, 0x61              Delta 11 (Uri-Path); length 1, value 'a'
        0xFF, 0x62              Payload marker; payload 'b'
    */
    vector<uint8_t> extended13 = {0x40, 0x01, 0x00, 0x01, 0xDD, 0x01, 0x00};
    extended13.resize(extended13.size() + 13, 0x61);

    vector<uint8_t> extended14 = {0x40, 0x01, 0x00, 0x01, 0xEE, 0x00, 0x00, 0x00, 0x00};
    extended14.resize(extended14.size() + 269, 0x61);

    const vector<TestCase> cases = {
        {"option without value",       {0x40, 0x01, 0x00, 0x01, 0x00},                               CoapStatus::COAP_ERR_BUFFER_SIZE},
        {"one extended byte",          extended13,                                                   CoapStatus::COAP_ERR_BUFFER_SIZE},
        {"token, option and payload",  {0x42, 0x01, 0x00, 0x01, 0x11, 0x22, 0xB1, 0x61, 0xFF, 0x62}, CoapStatus::COAP_ERR_BUFFER_SIZE},
        {"two extended bytes",         extended14,                                                   CoapStatus::COAP_ERR_BUFFER_SIZE},
    };

    // The unit tests are built without sanitizers, so a write outside of the buffer is seen by a changed byte.
    // The value is not present in the packets above.
    const uint8_t untouched = 0xA5;

    for (const auto & tc : cases)
    {
        error_code ec;
        Packet packet;

        packet.parse(tc.message.data(), tc.message.size(), ec);
        ASSERT_FALSE(ec) << "  actual: " << ec.message();

        const error_code expected = make_error_code(tc.expected);

        // Every size that is smaller than the packet, so the buffer ends in every part of the frame
        for (size_t given = 0; given < tc.message.size(); ++given)
        {
            vector<uint8_t> buffer(tc.message.size(), untouched);
            size_t size = given;

            packet.serialize(ec, buffer.data(), size);

            EXPECT_EQ(ec, expected)
                << "  TC name:  " << tc.name << ", buffer size " << given << "\n"
                << "  actual:   " << ec.message() << "\n"
                << "  expected: " << expected.message();

            EXPECT_EQ(buffer[given], untouched)
                << "  TC name:  " << tc.name << ", buffer size " << given << "\n"
                << "  serialize() wrote outside of the buffer";
        }

#ifdef PRINT_TESTED_VALUES
        print_error(tc.name, ec);
#endif
    }
}

TEST(testPacket, DataType)
{
    const char * testString = "This is a test string";
    DataType data(testString);

#ifdef PRINT_TESTED_VALUES
    info("Data type:{0:d}", data.value.type);
    info("Data value: {}", to_hex(data.value.asString));
#endif  

    int r = memcmp(testString, data.value.asString.data(), data.value.asString.length());

    ASSERT_TRUE(r == 0);
    ASSERT_TRUE(data.value.type == DataType::TYPE_STRING);
}

TEST(testPacket, clear)
{
    error_code ec;

    Packet packet;

    create_testOptions(packet, ec);

    ASSERT_TRUE(!ec.value());

    uint16_t id = generate_identity();

    packet.make_request(ec, CONFIRMABLE, POST, id, &testCoapPacket[51], 110);

    ASSERT_TRUE(!ec.value());

#ifdef PRINT_TESTED_VALUES
    info("Before cleaning:");
    print_packet(packet);
#endif

    packet.clear();

#ifdef PRINT_TESTED_VALUES
    info("After cleaning:");
    print_packet(packet);
#endif

    ASSERT_EQ(packet.header_as_byte(), 0);
    ASSERT_EQ(packet.code_as_byte(), 0);
    ASSERT_EQ(packet.identity(), 0);
    ASSERT_EQ(packet.token_length(), 0);
    for (size_t i = 0; i < TOKEN_MAX_LENGTH; ++i)
    {
        ASSERT_EQ(packet.token()[i], 0);
    }
    ASSERT_EQ(packet.options().empty(), true);
    ASSERT_EQ(packet.payload().empty(), true);
}

int main(int argc, char ** argv)
{
    info("Running main() from test_packet.cc");

    testing::InitGoogleTest(&argc, argv);

    return RUN_ALL_TESTS();
}