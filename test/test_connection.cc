#include "unix_connection.h"
#include "unix_socket.h"
#include "error.h"
#include <gtest/gtest.h>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>
#include <cstdint>
#include <cstring>

using namespace std;
using namespace spdlog;

static const char * testUri = "coap://localhost:5683";

TEST(testConnection, connect)
{
    std::error_code ec;

    UnixConnection conn(testUri, ec);

    ASSERT_TRUE(!ec.value());

    conn.connect(ec);

    ASSERT_TRUE(!ec.value());
}