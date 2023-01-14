#include "error.h"
#include "consts.h"
#include "unix_udp_client.h"
#include "unix_endpoint.h"

#include <iostream>
#include <string>
#include <cstring>
#include <memory>
#include <array>
#include <fstream>
#include <cstdio>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>

#include <getopt.h>
#include <signal.h>

using namespace std;
using namespace spdlog;
using namespace Unix;
using namespace coap;

#define STDIN_MAX_BUF_SIZE 64U

struct CommandLineOptions
{
    bool interactiveMode;
    bool blockwise;
    int port;
    string uri;
    string request;
    string filename;
};

struct Request
{
    MethodCode code;
    string url;
    string value;
};

static bool g_terminate = false;

static const char * g_defaultUri = "coap://[::1]:5683";
static const int g_defaultPort = 56083;

static void usage()
{
    std::cerr << "Usage: coap-client [OPTIONS]\n";
    std::cerr << "Launch coap-client\n";
    std::cerr << "Example 1: ./coap-client -u \"coap://192.168.0.104:5683\" -r \"GET /.well-known/core\"\n";
    std::cerr << "Example 2: ./coap-client -u \"coap://[2001:db8:0:85a3:0:8a2e:370:7334]:5683\" -i\n";
    std::cerr << "Options:\n";
    std::cerr << "-h,--help\tshow this message and exit\n";
    std::cerr << "-p,--port\t<PORT_NUMBER>\tset the client port number. Default: 56083\n";
    std::cerr << "-i,--interactive\t\trequests to the server are read from STDIN\n";
    std::cerr << "-u,--uri\t\tset a server URI. Default: coap://[::1]:5683\n";
    std::cerr << "-r,--request\t\trequest to the server:\n";
    std::cerr << "\tGET <path> -- GET request. Ex: GET /.well-known/core\n";
    std::cerr << "\tPUT <path> <value> -- PUT request. Ex: PUT /sensors/light 100\n";
    std::cerr << "\tPOST <path> <value> -- POST request. Ex: POST /sensors/light 100\n";
    std::cerr << "-b,--block-wise\t\tuse block-wise transfer\n";
    std::cerr << "-f,--file\t<FILE_NAME>\tstore requeted payload into the following file\n"; 
}

static bool parse_arguments(int argc, char ** argv, CommandLineOptions &options)
{
    int opt;
    char * endptr;

    options.interactiveMode = false;
    options.port = g_defaultPort;
    options.uri.reserve(strlen(g_defaultUri));
    options.uri = g_defaultUri;

    set_level(level::debug);
    while(true)
    {
        int option_index = 0;// getopt_long stores the option index here
        static struct option long_options[] = {
                {"help", no_argument, 0, 'h'},
                {"port", required_argument, 0, 'p'},
                {"interactive", no_argument, 0, 'i'},
                {"uri", required_argument, 0, 'u'},
                {"request", required_argument, 0, 'r'},
                {"block-wise", no_argument, 0, 'b'},
                {"file", required_argument, 0, 'f'},
                {0, 0, 0, 0}
        };
        opt = getopt_long (argc, argv, "hp:iu:r:", long_options, &option_index);
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
            case 'i':
                options.interactiveMode = true;
                break;
            case 'u':
                options.uri.reserve(strlen(optarg));
                options.uri = optarg;
                break;
            case 'r':
                options.request.reserve(strlen(optarg));
                options.request = optarg;
                break;
            case 'b':
                options.blockwise = true;
                break;
            case 'f':
                options.filename.reserve(strlen(optarg));
                options.filename = optarg;
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

static bool parse_request(const string &req, struct Request &out)
{
    size_t offset;
    if ((offset = req.find("GET")) != string::npos) {
        out.code = METHOD_GET;
    }
    else if ((offset = req.find("PUT")) != string::npos) {
        out.code = METHOD_PUT;    
    }
    else if ((offset = req.find("POST")) != string::npos) {
        out.code = METHOD_POST;    
    }
    else {
        debug("Method has not been recognized");
        return false;
    }
    if (offset != 0) {
        debug("Wrong parameter sequence");
        return false;
    }
    offset = req.find(" ");
    if (offset == string::npos)
    {
        debug("Wrong format");
        return false;
    }
    size_t end = req.find(" ", offset + 1);
    if (end == string::npos)
    {
        end = req.length();
        if (out.code == METHOD_PUT || out.code == METHOD_POST)
        {
            debug("Value is not presented");
            return false;    
        }
    }
    out.url.append(req.substr(offset + strlen(" "), end - offset));
    if (end != req.length())
    {
        out.value.append(req.substr(end + strlen(" "), req.length() - end));
    }
    return true;
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

    debug("interactiveMode: {}", options.interactiveMode);
    debug("port: {0:d}", options.port);
    debug("uri: {}", options.uri.c_str());
    debug("request: {}", options.request.c_str());

    if (!options.interactiveMode && options.request.empty())
    {
        usage();
        return EXIT_SUCCESS;
    }

    struct Request req;
    bool listenToStdin = false;

    if (!options.interactiveMode)
    {
        if (!parse_request(options.request, req)) {
            debug("parse_request failed");
            return EXIT_FAILURE;        
        }
        debug("method code: {0:d}", req.code);
        debug("url: {}", req.url.c_str());
        debug("value: {}", req.value.c_str());
    }
    else {
        listenToStdin = true;
    }

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

    debug("creating a new connection...");

    UdpClientConnection connection(options.uri.c_str(), ec);
    if (ec.value())
    {
        debug("FAILED\nerror occured : {}", ec.message());
        return EXIT_FAILURE;
    }
    debug("OK");

    debug("connecting to the server...");
    connection.connect(ec);
    if (ec.value())
    {
        debug("FAILED\nerror occured : {}", ec.message());
        return EXIT_FAILURE;
    }
    debug("OK");

    sock = static_cast<const UnixSocket *>(connection.socket());

    debug("creating a new endpoint...");

    ClientEndpoint client("CoAP Client", &connection);
    debug("OK");

    if (!options.interactiveMode)
    {
        client.start();
    }

    debug("client is launching...");

    while(!g_terminate)
    {
        FD_ZERO (&rd);
        FD_SET (sock->descriptor(), &rd);
        if (listenToStdin)
        {
            FD_SET (STDIN_FILENO, &rd);
        }

        tv.tv_sec = client.timeout();
        tv.tv_usec = 0;

        status = select(FD_SETSIZE, &rd, NULL, NULL, &tv);

        if (status == 0)
        {
            debug("select() timeout {0:d} sec expired", client.timeout());
        }
        else if (status < 0)
        {
            debug("select() error, errno = {0:d}", errno);
            return EXIT_FAILURE;
        }

        if (FD_ISSET(sock->descriptor(), &rd))
        {
            debug("something received");
            client.received(true); 
        }
        else if (FD_ISSET(STDIN_FILENO, &rd))
        {
            char buffer[STDIN_MAX_BUF_SIZE];

            ssize_t received = read(STDIN_FILENO, buffer, STDIN_MAX_BUF_SIZE);
            if (received > 0)
            {
                options.request = buffer;
                if (!parse_request(options.request, req)) {
                    debug("parse_request failed");
                    continue;        
                }
                debug("method code: {0:d}", req.code);
                debug("url: {}", req.url.c_str());
                debug("value: {}", req.value.c_str());
                listenToStdin = false;
                client.start();
            }
        }

        debug("transaction_step <-- {0:d}", client.received());

        client.transaction_step(ec);
        if (ec.value())
        {
            debug("transaction_step failed: {}", ec.message());
        }
        if (client.currentState() == ClientEndpoint::COMPLETE
            || client.currentState() == ClientEndpoint::ERROR)
        {
            listenToStdin = true;
        }
    }

    client.stop();

    debug("{} has been finished", argv[0]);

    return EXIT_SUCCESS;
}
