#include "base64.h"
#include "test_common.h"
#include <gtest/gtest.h>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>
#include <spdlog/fmt/bin_to_hex.h>
#include <cstdint>
#include <cstring>
#include <ctime>

using namespace std;
using namespace coap;
using namespace spdlog;

TEST(testBase64, encode)
{
    error_code ec;
    Base64Encoder encoder;
    const char test1[] = "This is a test string to be encoded to Base64";
   	encoder.encode((const uint8_t *)test1, strlen(test1), ec);
   	EXPECT_EQ(ec.value(), 0);

#ifdef PRINT_TESTED_VALUES
   	info("Encoded size: {0:d}", encoder.encoded_size());
	info("Encoded data: {}", encoder.encoded_data());
#endif
}

TEST(testBase64, decode)
{
    error_code ec;
    Base64Encoder encoder;
    const char test1[] = "VGhpcyBpcyBhIHRlc3Qgc3RyaW5nIHRvIGJlIGVuY29kZWQgdG8gQmFzZTY0";
   	encoder.decode(test1, strlen(test1), ec);
   	EXPECT_EQ(ec.value(), 0);

#ifdef PRINT_TESTED_VALUES
   	info("Decoded size: {0:d}", encoder.decoded_size());
	info("Decoded data: {}", to_hex(encoder.decoded_data(), encoder.decoded_data() + encoder.decoded_size()));
#endif
}
