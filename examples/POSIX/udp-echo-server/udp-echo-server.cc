#include "common/udp_server.h"
#include "error.h"

#include <iostream>
#include <string>
#include <cstring>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>

#include <getopt.h>
#include <signal.h>

using namespace std;
using namespace spdlog;

static void udp_echo_server_packet_handler(Incomming* incomming, error_code &ec)
{
    Payload *payload = udpserver::receive(incomming, ec);
    if (ec.value())
    {
        debug("receive() error: {}", ec.message());
        return;
    }
    udpserver::send(incomming, payload, ec);
    if (ec.value())
    {
        debug("send() error: {}", ec.message());
    }
}

static void usage()
{
    std::cerr << "Usage: udp-echo-client [OPTIONS]" << std::endl;
    std::cerr << "Launch a UDP echo server" << std::endl;
    std::cerr << "Options:" << std::endl;
    std::cerr << "-h,--help\tshow this message and exit" << std::endl;
    std::cerr << "-p,--port\t<PORT_NUMBER>\tset the port number of the server. Default: 5683" << std::endl;
    std::cerr << "-4,--ipv4\tListen to only IPv4 addresses" << std::endl;
    std::cerr << "-6,--ipv6\tListen to only IPv6 addresses. Default" << std::endl;
}

struct CommandLineOptions
{
    bool useIPv4;
    int port;
};

static bool parse_arguments(int argc, char ** argv, CommandLineOptions &options)
{
    int opt;
    char * endptr;
    if (argc == 1) {
        usage();
        return false;
    }
    options.port = 5683;
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

static UdpServer * server;

static void signal_handler(int signo)
{
    if (signo == SIGINT)
    {
        if (server)
        { server->stop(); debug("server stopped");}
    }
}

int main(int argc, char **argv)
{
    set_level(level::debug);
    debug("udp-echo-server");

    CommandLineOptions options;
    if (!parse_arguments(argc, argv, options))
    {
        debug("parse_arguments() error");
        return 1;
    }
    debug("port: {0:d}", options.port);
    debug("use IPv4: {}", options.useIPv4);

    error_code ec;

    server = new UdpServer(options.port, ec, options.useIPv4);
    if (ec.value())
    {
        debug("UdpServer constructor error: {}", ec.message());
        return 1;
    }

    if (signal(SIGINT, signal_handler) == SIG_ERR)
    {
        debug("Unable to set SIGINT handler");
        return 1;
    }

    server->set_received_packed_handler_callback(udp_echo_server_packet_handler);

    server->start();

    server->listening(ec);
    if (ec.value())
    {
        debug("listening error: {}", ec.message());
    }

    server->shutdown(ec);
    if (ec.value())
    {
        debug("shutdown error: {}", ec.message());
    }

    delete server;

    return 0;
}
