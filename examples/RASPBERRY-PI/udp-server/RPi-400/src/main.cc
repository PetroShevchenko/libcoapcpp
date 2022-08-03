#include "udp_server.h"
#include "command.h"
#include "error.h"
#include "log.h"

#include <iostream>
#include <string>
#include <cstring>
#include <memory>
#include <array>
#include <fstream>

#include <getopt.h>
#include <signal.h>

using namespace std;
using namespace spdlog;
using namespace Unix;

struct CommandLineOptions
{
    bool useIPv4;
    int port;
};

static bool g_terminate = false;

static void usage()
{
    std::cerr << "Usage: udp-server [OPTIONS]\n";
    std::cerr << "Launch udp-server\n";
    std::cerr << "Example: ./udp-server -p 5555 -4\n";
    std::cerr << "Options:\n";
    std::cerr << "-h,--help\tshow this message and exit\n";
    std::cerr << "-p,--port\t<PORT_NUMBER>\tset the port number to listen to incomming connections. Default: 5555\n";
    std::cerr << "-4,--ipv4\tListen to only IPv4 addresses\n";
    std::cerr << "-6,--ipv6\tListen to only IPv6 addresses. Default\n";
}

static bool parse_arguments(int argc, char ** argv, CommandLineOptions &options)
{
    int opt;
    char * endptr;

    options.port = 5555;
    options.useIPv4 = false;
    set_level(level::debug);
    while(true)
    {
        int option_index = 0;// getopt_long stores the option index here
        static struct option long_options[] = {
                {"help", no_argument, 0, 'h'},
                {"port", required_argument, 0, 'p'},
                {"ipv4", no_argument, 0, '4'},
                {"ipv6", no_argument, 0, '6'},
                {0, 0, 0, 0}
        };
        opt = getopt_long (argc, argv, "hp:46", long_options, &option_index);
        if (opt == -1) break;

        switch (opt)
        {
            case '?':
            case 'h':
                usage();
                return false;
            case 'p':
                options.port = (int)strtol(optarg, &endptr, 10);
                if (options.port <= 0) {
                    debug("Error: Unable to convert --port {} option value to port number", optarg);
                    return false;
                }
                break;
            case '4':
                options.useIPv4 = true;
                break;
            case '6':
                options.useIPv4 = false;
                break;
            default:
                return false;
        }
    }
    return true;
}

static void signal_handler(int signo)
{
    if (signo == SIGINT)
    {
        debug("SIGINT cought");
        g_terminate = true;
    }
}

int main(int argc, char **argv)
{
    set_level(level::debug);
    debug("{} has been started", argv[0]);

    CommandLineOptions options;
    if (!parse_arguments(argc, argv, options))
    {
        debug("parse_arguments() failed");
        return EXIT_FAILURE;
    }
    debug("port: {0:d}", options.port);
    debug("use IPv4: {}", options.useIPv4);

    if (signal(SIGINT, signal_handler) == SIG_ERR)
    {
        debug("Unable to set SIGINT handler");
        return EXIT_FAILURE;
    }

    error_code ec;

    UdpServer server(options.port, options.useIPv4, ec);

    if (ec.value())
    {
        debug("Server creation error: {}", ec.message());
        return EXIT_FAILURE; 
    }

    debug("Server initialization...");
    server.init(ec);
    if (ec.value())
    {
        debug("Failed: {}", ec.message());
        return EXIT_FAILURE; 
    }
    debug("OK");

    server.set_received_packed_handler_callback(static_cast<ReceivedPacketHandlerCallback>(command_handler));

    debug("Server is launching...");
    server.start();
    debug("OK");

    server.process();

    debug("{} has been finished", argv[0]);

    return EXIT_SUCCESS;
}
