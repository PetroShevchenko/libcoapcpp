#include "error.h"
#include "unix_udp_server.h"
#include "unix_endpoint.h"

#include <iostream>
#include <string>
#include <cstring>
#include <memory>
#include <array>
#include <fstream>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>

#include <getopt.h>
#include <signal.h>

using namespace std;
using namespace spdlog;
using namespace Unix;
using namespace coap;

struct CommandLineOptions
{
    bool useIPv4;
    int port;
};

static bool g_terminate = false;

static const char * g_contentPath = "data/well-known_core.wlnk";

static void usage()
{
    std::cerr << "Usage: coap-server [OPTIONS]\n";
    std::cerr << "Launch coap-server\n";
    std::cerr << "Example: ./coap-server -p 5693 -4\n";
    std::cerr << "Options:\n";
    std::cerr << "-h,--help\tshow this message and exit\n";
    std::cerr << "-p,--port\t<PORT_NUMBER>\tset the port number to listen to incomming connections. Default: 5683\n";
    std::cerr << "-4,--ipv4\tListen to only IPv4 addresses\n";
    std::cerr << "-6,--ipv6\tListen to only IPv6 addresses. Default\n";
}

static bool parse_arguments(int argc, char ** argv, CommandLineOptions &options)
{
    int opt;
    char * endptr;

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

static bool read_core_link_content(const char * filePath, string &content)
{
    ifstream ifs(filePath);

    if (ifs.is_open())
    {
        ifs.seekg(0, std::ios::end);
        size_t size = ifs.tellg();
        content.reserve(size);
        ifs.seekg(0);
        ifs.read(&content[0], size);
        ifs.close();
        return true;
    }
    return false;
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

    string coreLinkContent;

    if (!read_core_link_content(g_contentPath, coreLinkContent))
    {
        debug("read_core_link_content() failed");
        return EXIT_FAILURE;        
    }
    debug("core link content:\n{}", coreLinkContent.c_str());

    if (signal(SIGINT, signal_handler) == SIG_ERR)
    {
        debug("Unable to set SIGINT handler");
        return EXIT_FAILURE;
    }

    error_code ec;
    int status;
    fd_set rd;
    struct timeval tv;
    const UnixSocket *sock;
    shared_ptr<Buffer> bufferPtr;

    bufferPtr = make_shared<Buffer>(BUFFER_SIZE);

    debug("creating a new connection...");

    UdpServerConnection connection(options.port, options.useIPv4, move(bufferPtr), ec);
    if (ec.value())
    {
        debug("FAILED\nerror occured : {}", ec.message());
        return EXIT_FAILURE;
    }
    debug("OK");

    debug("binding socket with IP and port...");
    connection.bind(ec);
    if (ec.value())
    {
        debug("FAILED\nerror occured : {}", ec.message());
        return EXIT_FAILURE;
    }
    debug("OK");

    sock = static_cast<const UnixSocket *>(connection.socket());

    debug("creating a new endpoint...");

    ServerEndpoint server("CoAP Server", coreLinkContent.c_str(), &connection, ec);
    if (ec.value())
    {
        debug("FAILED\nerror occured : {}", ec.message());
        return EXIT_FAILURE;        
    }
    debug("OK");

    debug("server is launching...");
    server.start();

    while(!g_terminate)
    {
        FD_ZERO (&rd);
        FD_SET (sock->descriptor(), &rd);

        tv.tv_sec = server.timeout();
        tv.tv_usec = 0;

        status = select(FD_SETSIZE, &rd, NULL, NULL, &tv);

        if (status == 0)
        {
            debug("select() timeout {0:d} sec expired", server.timeout());
        }
        else if (status < 0)
        {
            debug("select() error, errno = {0:d}", errno);
        }

        if (FD_ISSET(sock->descriptor(), &rd))
        {
            debug("something received");
            server.received(true); 
        }

        debug("transaction_step <-- {0:d}", server.received());

        server.transaction_step(ec);
        if (ec.value())
        {
            debug("transaction_step failed: {}", ec.message());
        }
    }

    server.stop();

    debug("{} has been finished", argv[0]);

    return EXIT_SUCCESS;
}
