#if defined(__unix__) || defined(__APPLE__)
#ifdef __arm__
#warning "You are going to compile this code for ARM platform"
#endif

#include "coap_server.h"
#include "buffer.h"
#include "error.h"
#include "sensor.h"
#include "endpoint.h"
#ifdef __arm__
#include "DHT11.h"
#include "RGB_LED.h"
#else
#include "sensor_stubs.h"
#endif

#include <iostream>
#include <string>
#include <cstring>
#include <cstdint>
#include <memory>
#include <array>
#include <iterator>
#include <fstream>

#include <sys/select.h>
#include <unistd.h>
#include <netinet/in.h>

#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>

#include <getopt.h>
#include <signal.h>

using namespace std;
using namespace spdlog;
using namespace posix;
using namespace sensors;
using namespace coap;

struct CommandLineOptions
{
    bool useIPv4;
    int port;
};

typedef union 
{
    struct sockaddr_in addr;
    struct sockaddr_in6 addr6;
} SocketAddressType;

typedef struct
{
    bool useIPv4;
    SocketAddressType serv;
    SocketAddressType client;
    int socketFd;
} UdpConnectionType;

static bool g_terminate = false;

static const char * g_contentPath = "data/well-known_core.wlnk";

static UdpConnectionType g_connection;

static SensorSet g_KS0068;

#ifdef __arm__
static RgbLed g_RGB_LED(0, 1, 2); // RED: PIN 0, GREEN: PIN 1, BLUE: PIN2
static Dht11 g_DHT11(3); // DATA: PIN 3
#else
static RGB_LED_Simulator g_RGB_LED;
static DHT11_Simulator g_DHT11;
#endif

static const std::vector<EndpointType> g_endpoints = {
    { "sensors/DHT11/temp", DHT11, &g_KS0068 },
    { "sensors/DHT11/hum", DHT11, &g_KS0068 },
    { "sensors/RGB_LED/red", RGB_LED, &g_KS0068 },
    { "sensors/RGB_LED/green", RGB_LED, &g_KS0068 },
    { "sensors/RGB_LED/blue", RGB_LED, &g_KS0068 }
};

static const std::vector<SensorBindType> g_bindingInfo = {
    { &g_DHT11, &g_KS0068},
    { &g_RGB_LED, &g_KS0068}
};

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
    set_level(level::info);
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
                    error("Error: Unable to convert --port {} option value to port number", optarg);
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
        info("SIGINT cought");
        g_terminate = true;
    }
}

static EndpointPool *create_sensor_endpoints(const char *coreLinkContent, const vector<EndpointType> &endpoints, error_code &ec)
{
    coap::CoreLink parser;
    parser.parse_core_link(coreLinkContent, ec);
    if (ec.value()) // check if the content read is in Core-Link format
    {
        return nullptr;
    }

    static EndpointPool endpointPool(endpoints);

    endpointPool.compare_endpoints(parser, ec);// check if the parser contains the valid URIs
    if (ec.value())
    {
        return nullptr;
    }

    return &endpointPool;
}

static void initialize_sensors(const std::vector<SensorBindType> &bindingInfo, error_code &ec)
{
    for (auto record : bindingInfo)
    {
        record.sensor->bind(*record.set, ec);
        if (ec.value())
            return;
    }
}

static void initialize_connection(bool useIPv4, uint16_t portnum, UdpConnectionType &conn, error_code &ec)
{
    uint16_t port;
    int fd = socket(useIPv4 ? AF_INET : AF_INET6, SOCK_DGRAM, 0);
    if (fd == -1) {
        ec = make_error_code(CoapStatus::COAP_ERR_CREATE_SOCKET);
        return;
    }

    port = htons((uint16_t)portnum);
    conn.useIPv4 = useIPv4;
    if (conn.useIPv4)
    {
        // start inintialization by zero values
        bzero(&conn.serv.addr, sizeof(conn.serv.addr));
        // initialization of server address structure
        conn.serv.addr.sin_family = AF_INET;
        conn.serv.addr.sin_addr.s_addr = INADDR_ANY;
        conn.serv.addr.sin_port = port;
    }
    else
    {
        bzero(&conn.serv.addr6, sizeof(conn.serv.addr6));
        conn.serv.addr6.sin6_family = AF_INET6;
        conn.serv.addr6.sin6_addr = in6addr_any;
        conn.serv.addr6.sin6_port = port;
    }
    // bind an empty socket to serv.addr(6) structure
    if (bind(fd, useIPv4 ? (struct sockaddr *)&conn.serv.addr : (struct sockaddr *)&conn.serv.addr6,
                 useIPv4 ? sizeof(conn.serv.addr) : sizeof(conn.serv.addr6))==-1) {
        ec = make_error_code(CoapStatus::COAP_ERR_SOCKET_NOT_BOUND);
        close(fd);
        return;
    }
    conn.socketFd = fd;
}

int main(int argc, char **argv)
{
    set_level(level::info);

    if (argc == 1)
    {
        usage();
        exit(EXIT_FAILURE);
    }

    CommandLineOptions options;
    if (!parse_arguments(argc, argv, options))
    {
        exit(EXIT_FAILURE);
    }

    if (signal(SIGINT, signal_handler) == SIG_ERR)
    {
        error("Unable to set SIGINT handler");
        exit(EXIT_FAILURE);
    }

    string coreLinkContent;

    if (!read_core_link_content(g_contentPath, coreLinkContent))
    {
        error("read_core_link_content() failed");
        exit(EXIT_FAILURE);        
    }
    info("{} has been read", g_contentPath);

    error_code ec;

    EndpointPool *endpoints = create_sensor_endpoints(coreLinkContent.c_str(), g_endpoints, ec);
    if (ec.value())
    {
        error("{}", ec.message());
        exit(EXIT_FAILURE);
    }
    info("Endpoints initialized");

    initialize_sensors(g_bindingInfo, ec);
    if (ec.value())
    {
        error("{}", ec.message());
        exit(EXIT_FAILURE);
    }
    info("Sensors initialized");

    initialize_connection(options.useIPv4, options.port, g_connection, ec);
    if (ec.value())
    {
        error("{}", ec.message());
        exit(EXIT_FAILURE);
    }
    info("Connection initialized");

    struct sockaddr *client_addr;
    int addr_len;

    if (g_connection.useIPv4)
    {
        client_addr = (struct sockaddr *)&g_connection.client.addr;
        addr_len = sizeof(g_connection.client.addr);
    }
    else
    {
        client_addr = (struct sockaddr *)&g_connection.client.addr6;
        addr_len = sizeof(g_connection.client.addr6);
    }

    Buffer buf(BUFFER_SIZE);

    CoapServer server("CoAP Server",
                    coreLinkContent.c_str(),
                    endpoints
                );
    server.timeout(60);
    server.start();
    info("CoAP server has been started");

    int status;
    fd_set rd;
    int sock = g_connection.socketFd;
    struct timeval tv;

    while(!g_terminate && server.is_running())
    {
        FD_ZERO(&rd);
        FD_SET(sock, &rd);

        time_t timeout = server.timeout();
        tv.tv_sec = timeout;
        tv.tv_usec = 0;

        /* Thread is blocked until something is present on rd or timeout */
        status = select(FD_SETSIZE, &rd, NULL, NULL, &tv);
        if (status == -1)
        {
            error("select() error, errno = {0:d}", errno);
            break;
        }
        else if (status)
        {
            ssize_t received, sent;

            if (FD_ISSET(sock, &rd))
            {
                buf.clear();
                received = recvfrom(sock, buf.data(), buf.length(), MSG_DONTWAIT, client_addr, (socklen_t *)&addr_len);
                if (received > 0)
                {
                    info("<-- {0:d} bytes received", received);
                    buf.offset(received);
                    server.processing(buf);
                    if ((sent = sendto(sock, buf.data(), buf.offset(),  MSG_DONTWAIT, (const struct sockaddr *)client_addr, addr_len)) == -1)
                        error("sendto() returned -1");
                    else
                        info("--> {0:d} bytes sent", sent);
                }
            }
        }
        else
        {
            info("No data within {0:d} seconds.", (int)timeout);
        }
    }
    close(sock);
    info("CoAP server has terminated");
    exit(EXIT_SUCCESS);
}
#else
#error "Your target system is not POSIX"
#endif
