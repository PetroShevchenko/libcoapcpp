#include "unix_socket.h"
#include "error.h"
#include <gtest/gtest.h>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>
#include <cstdint>
#include <cstring>

using namespace std;
using namespace spdlog;

TEST(testSocket, socket)
{
    error_code ec;
    Socket * sock = new UnixSocket(AF_INET, SOCK_DGRAM, 0, ec);
    delete sock;
    ASSERT_TRUE(!ec.value());
}