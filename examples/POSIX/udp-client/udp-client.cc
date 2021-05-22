#include <iostream>
#include <string>
#include <cstring>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>
#include "error.h"
#include "unix_connection.h"

using namespace std;
using namespace spdlog;

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    std::error_code ec;
    set_level(level::debug);

    ClientConnection *
    client = create_client_connection(
                            UDP,
                            "localhost",
                            5683,
                            ec
                        );
    if (ec.value())
    {
        debug("create_client_connection() error: {}", ec.message());
        return 1;
    }

    debug("connect()");
    client->connect(ec);
    if (ec.value())
    {
        debug("connect() error: {}", ec.message());
        return 1;
    }

    debug("OK");
    const char *message = "This is a Ping-Pong message";
    size_t mesLen = strlen(message);

    debug("send()");
    client->send(message, mesLen, ec);
    if (ec.value())
    {
        debug("send() error: {}", ec.message());
        return 1;
    }
    debug("OK");

    char receiveBuffer[256];
    size_t len = sizeof(receiveBuffer);

    debug("receive()");
    client->receive(receiveBuffer, len, ec, 60);
    if (ec.value())
    {
        debug("receive() error: {}", ec.message());
        return 1;
    }
    debug("OK");

    debug("There was received a message from the server: {}", receiveBuffer);

    delete client;

    return 0;
}