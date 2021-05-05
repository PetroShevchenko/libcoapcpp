#include "unix_connection.h"
#include "unix_socket.h"
#include "error.h"
#include <gtest/gtest.h>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>
#include <cstdint>
#include <cstring>
#include <thread>
#include <chrono>
#include <mutex>
#include <condition_variable>

using namespace std;
using namespace spdlog;

static const char * testUri = "coap://localhost:5683";
static const char * testMessage = "Hello, it is a UDP message";

TEST(testConnection, connect)
{
    std::error_code ec;

    Connection * conn = create_client_connection(testUri, ec);

    ASSERT_TRUE(!ec.value());

    ASSERT_EQ(conn->type(), UDP);

    conn->connect(ec);

    ASSERT_TRUE(!ec.value());

    conn->disconnect(ec);

    ASSERT_TRUE(!ec.value());

    delete conn;
}

TEST(testConnection, send)
{
    std::error_code ec;

    Connection * conn = create_client_connection(testUri, ec);

    ASSERT_TRUE(!ec.value());

    conn->connect(ec);

    ASSERT_TRUE(!ec.value());

    conn->send(testMessage, strlen(testMessage), ec);

    ASSERT_TRUE(!ec.value());

    conn->disconnect(ec);

    ASSERT_TRUE(!ec.value());

    delete conn;
}
