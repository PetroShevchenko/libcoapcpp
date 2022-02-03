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
    info("version : {0:d}", packet.version());
    info("type : {0:d}", packet.type());
    info("token length : {0:d}", packet.token_length());
    info("code : {0:d}",packet.code_as_byte());
    info("identity: {0:d}", packet.identity());
    info("token :");
    fmt::print("{:02x}", fmt::join(packet.token(), ", "));
    fmt::print("\n");
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
    info("version : {0:d}", packet.version());
    info("type : {0:d}", packet.type());
    info("token length : {0:d}", packet.token_length());
    info("code : {0:d}",packet.code_as_byte());
    info("identity: {0:d}", packet.identity());
    info("token :");
    fmt::print("{:02x}", fmt::join(packet.token(), ", "));
    fmt::print("\n");
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

int main(int argc, char ** argv)
{
    info("Running main() from test_packet.cc");

    testing::InitGoogleTest(&argc, argv);

    return RUN_ALL_TESTS();
}