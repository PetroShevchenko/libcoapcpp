#include "coap_client.h"
#include "error.h"
#include "consts.h"
#include "senml_json.h"
#include "utils.h"

#include <iostream>
#include <string>
#include <cstring>
#include <memory>
#include <array>
#include <fstream>
#include <cstdio>
#include <ctime>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>

#include <getopt.h>
#include <signal.h>

using namespace std;
using namespace spdlog;
using namespace coap;
using namespace posix;

struct CommandLineOptions
{
    bool blockwise;
    size_t blockSize;
    string uri;
    string request;
    string filename;
};

struct Request
{
    MethodCode code;
    string uriPath;
    vector<string> records;
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
    int socketFd;
    uint16_t port;
} UdpConnectionType;

static bool g_terminate = false;

static const char * g_defaultUri = "coap://[::1]:5683";

static UdpConnectionType g_connection;

static void usage()
{
    std::cerr << "Usage: coap-client [OPTIONS]\n";
    std::cerr << "Run coap-client\n";
    std::cerr << "Options:\n";
    std::cerr << "-h,--help\t\t-- show this message and exit\n";
    std::cerr << "-u,--uri <URI>\t\t-- set a server URI. Default: coap://[::1]:5683\n";
    std::cerr << "-r,--request <REQUEST>\t-- request to the server:\n";
    std::cerr << "\tGET <PATH>\t\t\t-- GET request. Ex: get /.well-known/core\n";
    std::cerr << "\tPUT <PATH> <RECORD1> <RECORDN>\t-- PUT request. <RECORD>: <NAME=VALUE,UNIT>. Ex: \"PUT /sensors/RGB_LED/red light=100,light-%\"\n";
    std::cerr << "\tPOST <PATH> <RECORD1> <RECORDN>\t-- POST request. <RECORD>: <NAME=VALUE,UNIT>. Ex: \"POST /sensors/RGB_LED/green light=10,light-%\"\n";
    std::cerr << "\tDELETE <PATH>\t\t\t-- DELETE request. Ex: DELETE /firmware/pico/firmware.bin\n";
    std::cerr << "-b,--block-wise <BLOCK-SIZE>\t-- use block-wise transfer. <BLOCK-SIZE>: 16, 32, 64, 128, 256, 512, 1024. Default: 64\n";
    std::cerr << "-f,--file <FILE_NAME>\t\t-- set a file name for the following purposes:\n";
    std::cerr << "\tGET\t -- filename is used to store received payload to the file\n";
    std::cerr << "\tPUT/POST -- filename is used to read the request payload from the file\n";
    std::cerr << "\tDELETE\t -- not used, ignored\n";
    std::cerr << "Examples:\n";
    std::cerr << "\t./coap-client -u \"coap://[2001:db8:0:85a3:0:8a2e:370:7334]:5683\" -r \"GET /.well-known/core\"\n";
    std::cerr << "\t./coap-client -u \"coap://192.168.0.104:5683\" -r \"PUT /sensors/RGB_LED/red light=100,light-%\"\n";
    std::cerr << "\t./coap-client -u \"coap://192.168.0.104:5683\" -f \"data/put_request_template.json\" -r \"PUT /sensors/RGB_LED/red\"\n";
    std::cerr << "\t./coap-client -u \"coap://192.168.0.104:5683\" -f \"data/firmware.bin\" -r \"GET /firmware/pico/firmware.bin\"\n";
}

static bool parse_arguments(int argc, char ** argv, CommandLineOptions &options)
{
    int opt;

    options.blockwise = false;
    options.blockSize = 0;
    options.uri.reserve(strlen(g_defaultUri));
    options.uri = g_defaultUri;

    set_level(level::debug);
    while(true)
    {
        int option_index = 0;// getopt_long stores the option index here
        static struct option long_options[] = {
                {"help", no_argument, 0, 'h'},
                {"uri", required_argument, 0, 'u'},
                {"request", required_argument, 0, 'r'},
                {"block-wise", required_argument, 0, 'b'},
                {"file", required_argument, 0, 'f'},
                {0, 0, 0, 0}
        };
        opt = getopt_long (argc, argv, "hu:r:b:f:", long_options, &option_index);
        if (opt == -1) break;

        switch (opt)
        {
            case '?':
            case 'h':
                usage();
                return false;
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
                options.blockSize = atol(optarg);
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

static bool parse_request(const string &req, bool noFile, struct Request &out)
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
    else if ((offset = req.find("DELETE")) != string::npos) {
        out.code = METHOD_DELETE;    
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
        if (noFile && (out.code == METHOD_PUT
            || out.code == METHOD_POST))
        {
            debug("Records are not presented");
            return false;    
        }
    }
    out.uriPath.append(req.substr(offset + strlen(" "), end - offset - strlen(" ")));
    while (end != req.length())
    {
        offset = req.find(" ", end);
        if (offset == string::npos)
        {
            debug("Records are not presented");
            return false;    
        }
        end = req.find(" ", offset + 1);
        if (end == string::npos)
        {
            debug("The last record");
            end = req.length();
        }
        out.records.push_back(req.substr(offset + strlen(" "), end - offset - strlen(" ")));
    }
    return true;
}

static double get_timestamp()
{
    time_t currentTime = time(nullptr);
    return static_cast<double>(mktime(localtime(&currentTime)));
}

static void records_to_payload(
                    const vector<string> &records,
                    vector<uint8_t> &payload,
                    error_code &ec,
                    MediaType payloadType = SENML_JSON
                )
{
    if (payloadType != SENML_JSON)
    {
        ec = make_error_code(CoapStatus::COAP_ERR_NOT_IMPLEMENTED);
        return;
    }
    ec = make_error_code(CoapStatus::COAP_ERR_RECORD_FORMAT);
    SenmlJson parser;
    for (auto record : records)
    {
        string name, value, unit;// name=value,unit
        SenmlJsonType json;
        size_t end;
        size_t offset = record.find("=");
        if (offset == string::npos)
            return;
        name = record.substr(0, offset);
        end = record.find(",", offset + strlen("="));
        if (end == string::npos)
            return;
        value = record.substr(offset + strlen("="), end - offset -  strlen(","));
        unit = record.substr(end + strlen(","), record.size() - end - strlen(","));

        json.name = name;
        json.unit = unit;
        json.time = get_timestamp();

        long number = atol(value.c_str());
        debug("value = {}", value);
        if (number == 0 && value != "0" && value != "0.0")
        {
            if (value == "true" || value == "false")
            {
                json.value.type = SenmlJsonType::BOOLEAN;
                json.value.asBoolean = (value == "true") ? true : false;
            }
            else
            {
                json.value.type = SenmlJsonType::STRING;
                json.value.asString = value;
            }
        }
        else
        {
            json.value.type = SenmlJsonType::NUMBER;
            json.value.asNumber = number;            
        }
        parser.add_record(json);
    }
    ec.clear();
    parser.create_json(ec);
    if (ec.value())
        return;
    string jsonStr = parser.json();
    payload = vector<uint8_t>(jsonStr.begin(), jsonStr.end());
}

static void initialize_connection(const char * uri, UdpConnectionType &conn, error_code &ec)
{
    std::string hostname;
    int portnum;
    ec = make_error_code(CoapStatus::COAP_ERR_RESOLVE_ADDRESS);

    if (!uri2hostname(uri, hostname, portnum, true)
        && !uri2hostname(uri, hostname, portnum, false)) {
        return;
    }
    if (inet_pton(AF_INET, hostname.c_str(), &conn.serv.addr)) {// IPv4
        conn.useIPv4 = true;
        debug("conn.useIPv4: {}", conn.useIPv4);
    }
    else if (inet_pton(AF_INET6, hostname.c_str(), &conn.serv.addr6)) {// IPv6
        conn.useIPv4 = false;
        debug("conn.useIPv4: {}", conn.useIPv4);
    }
    else {// resolve server name
        struct hostent *hp;
        if ((hp = gethostbyname(hostname.c_str())) == NULL) {
            return;
        }
        uint16_t port = htons((uint16_t)portnum);// convert port number to the network order
        if (hp->h_addrtype == AF_INET) {
            bzero(&conn.serv.addr, sizeof(conn.serv.addr));
            bcopy(hp->h_addr, &conn.serv.addr.sin_addr, hp->h_length);
            conn.serv.addr.sin_family = hp->h_addrtype;
            conn.serv.addr.sin_port = port;
            conn.useIPv4 = true;
        }
        else if (hp->h_addrtype == AF_INET6) {
            bzero(&conn.serv.addr6, sizeof(conn.serv.addr6));
            bcopy(hp->h_addr, &conn.serv.addr6.sin6_addr, hp->h_length);
            conn.serv.addr6.sin6_family = hp->h_addrtype;
            conn.serv.addr6.sin6_port = port;
            conn.useIPv4 = false;
        }
        else {
            return;
        }


/*
typedef union 
{
    struct sockaddr_in addr;
    struct sockaddr_in6 addr6;
} SocketAddressType;

typedef struct
{
    bool useIPv4;
    SocketAddressType serv;
    int socketFd;
} UdpConnectionType;



        struct addrinfo hints;
        bzero(&hints, sizeof(hints));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_DGRAM;

        string portStr = to_string(portnum);
        struct addrinfo * servinfo = nullptr;

        int status = getaddrinfo(hostname.c_str(), portStr.c_str(), &hints, &servinfo);
        if (result == 0) {
            for (struct addrinfo *p = servinfo; p != nullptr; p = p->ai_next) {
                if (p->ai_family == AF_INET6) {
                    conn.useIPv4 = false;
                    memcpy(conn.serv.addr6, ((struct sockaddr_in6 *) p->ai_addr)->sin6_addr, sizeof(conn.serv.addr6));
                    break;
                }
                else if (p->ai_family == AF_INET) {
                    conn.useIPv4 = true;
                    memcpy(conn.serv.addr, ((struct sockaddr_in *) p->ai_addr)->sin_addr, sizeof(conn.serv.addr));
                    break;
                }
            }
        }
        else {
            ec = make_error_code(CoapStatus::COAP_ERR_RESOLVE_ADDRESS);
            return;
        }
        if (servinfo != nullptr) {
            freeaddrinfo(servinfo);
        }*/
    }
    debug("portnum: {0:d}", portnum);
    debug("hostname: {}", hostname);
    int fd = socket(conn.useIPv4 ? AF_INET : AF_INET6, SOCK_DGRAM, 0);// create a new socket 
    if (fd == -1) {
        ec = make_error_code(CoapStatus::COAP_ERR_CREATE_SOCKET);
        return;
    }
    conn.socketFd = fd;
    conn.port = portnum;
    ec.clear();
}

int main(int argc, char **argv)
{
    set_level(level::debug);

    CommandLineOptions options;
    if (!parse_arguments(argc, argv, options))
    {
        exit(EXIT_FAILURE);
    }

    debug("blockwise: {}", options.blockwise);
    debug("blockSize: {0:d}", options.blockSize);
    debug("uri: {}", options.uri.c_str());
    debug("request: {}", options.request.c_str());

    struct Request req;
 
    if (!parse_request(options.request, options.filename.empty(), req)) {
        error("parse_request failed");
        exit(EXIT_FAILURE);        
    }

    if (req.records.empty()
        && options.filename.empty()
        && (req.code == METHOD_PUT
        || req.code == METHOD_POST))
    {
        error("the payload or the file name is required");
        exit(EXIT_FAILURE);                
    }

    std::vector<std::uint8_t> payload;
    std::vector<std::uint8_t> *p_payload;
    error_code ec;

    if (options.filename.empty())
    {
        p_payload = &payload;            
    }
    else
    {
        p_payload = nullptr;
    }

    if (!req.records.empty() && p_payload)
    {
        records_to_payload(req.records, payload, ec);
        if (ec.value())
        {
            error("records_to_payload failed: {}", ec.message());
            exit(EXIT_FAILURE);   
        }
    }

    if (signal(SIGINT, signal_handler) == SIG_ERR)
    {
        error("Unable to set SIGINT handler");
        return EXIT_FAILURE;
    }

    initialize_connection(options.uri.c_str(), g_connection, ec);
    if (ec.value())
    {
        error("initialize_connection: {}", ec.message());
        exit(EXIT_FAILURE);   
    }
    info("Connection initialized");

    Buffer buf(BUFFER_SIZE);
    struct sockaddr *server_addr;
    SocketAddressType server;
    int addr_len;

    if (g_connection.useIPv4)
    {
        server_addr = (struct sockaddr *)&server.addr;
        addr_len = sizeof(server.addr);
    }
    else
    {
        server_addr = (struct sockaddr *)&server.addr;
        addr_len = sizeof(server.addr6);
    }

    CoapClient client(
                    ec,
                    req.code,
                    g_connection.port,
                    req.uriPath.c_str(),
                    options.filename.c_str(),
                    SENML_JSON,
                    p_payload,
                    buf,
                    options.blockwise,
                    options.blockSize
                );
    if (ec.value())
    {
        error("Failed to create CoapClient object: {}", ec.message());
        exit(EXIT_FAILURE);
    }

    client.start();
    info("CoAP client has been started");

    int status;
    fd_set rd;
    struct timeval tv;
    int sock = g_connection.socketFd;

    while(!g_terminate && client.is_running())
    {
        FD_ZERO(&rd);
        FD_SET(sock, &rd);

        time_t timeout = client.timeout();
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
            ssize_t received;
            if (FD_ISSET(sock, &rd))
            {
                buf.clear();
                received = recvfrom(sock, buf.data(), buf.length(), MSG_DONTWAIT, server_addr, (socklen_t *)&addr_len);

                if (memcmp(server_addr,
                            g_connection.useIPv4 ? (struct sockaddr *)&g_connection.serv.addr : (struct sockaddr *)&g_connection.serv.addr6,
                            addr_len) != 0)
                {
                    error("The recived address isn't match with the server address");
                    break;
                }
                if (received > 0)
                {
                    info("<-- {0:d} bytes received", received);
                    buf.offset(received);
                    client.received(true);
                }
            }
        }
        else if (timeout)
        {
            info("No data within {0:d} seconds.", (int)timeout);
        }

        client.processing_step();

        if (client.do_send())
        {
            ssize_t sent;
            if ((sent = sendto(sock, buf.data(), buf.offset(),  MSG_DONTWAIT, (const struct sockaddr *)server_addr, addr_len)) == -1)
                error("sendto() returned -1");
            else
                info("--> {0:d} bytes sent", sent);
        }
    }
    close(sock);
    info("CoAP client has terminated");
    exit(EXIT_SUCCESS);
}
