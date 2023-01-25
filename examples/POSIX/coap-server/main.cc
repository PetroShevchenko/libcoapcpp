#include "coap_server.h"
#include "buffer.h"
#include "error.h"

#include <iostream>
#include <string>
#include <cstring>
#include <cstdint>
#include <memory>
#include <array>
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

static void initialize_connection(bool useIPv4, uint16_t portnum, UdpConnectionType &conn, error_code &ec)
{
	uint16_t port;
	int fd = socket(AF_INET, SOCK_DGRAM, 0);
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
    set_level(level::debug);

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
    debug("port: {0:d}", options.port);
    debug("use IPv4: {}", options.useIPv4);

    string coreLinkContent;

    if (!read_core_link_content(g_contentPath, coreLinkContent))
    {
        debug("read_core_link_content() failed");
        exit(EXIT_FAILURE);        
    }
    debug("core link content:\n{}", coreLinkContent.c_str());

    if (signal(SIGINT, signal_handler) == SIG_ERR)
    {
        debug("Unable to set SIGINT handler");
        exit(EXIT_FAILURE);
    }

    error_code ec;

	initialize_connection(options.useIPv4, options.port, g_connection, ec);
	if (ec.value())
	{
		debug("{}", ec.message());
		return EXIT_FAILURE;
	}

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

    CoapServer server("CoAP Server", coreLinkContent.c_str());
    server.timeout(60);
    server.start();
    debug("CoAP server has been started");

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
            debug("select() error, errno = {0:d}", errno);
            break;
        }
        else if (status)
        {
            ssize_t received;

            if (FD_ISSET(sock, &rd))
            {
                buf.clear();
                received = recvfrom(sock, buf.data(), buf.length(), MSG_DONTWAIT, client_addr, (socklen_t *)&addr_len);
                if (received > 0)
                {
                    buf.offset(received);
                    server.processing(buf);
                    if (sendto(sock, buf.data(), buf.offset(),  MSG_DONTWAIT, (const struct sockaddr *)client_addr, addr_len) == -1)
                        debug("sendto() returned -1");
                }
            }
        }
        else
        {
            debug("No data within {0:d} seconds.", (int)timeout);
        }
    }
    close(sock);
    debug("CoAP server has terminated");
    exit(EXIT_SUCCESS);
}
