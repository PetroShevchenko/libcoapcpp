#include <iostream>
#include <string>
#include "spdlog/spdlog.h"
#include <spdlog/fmt/fmt.h>
#include "socket.h"
#include "unix_socket.h"
#include "unix_dns_resolver.h"
#include "error.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <cstring>

using namespace std;
using namespace spdlog;

class TcpClient
{
public:
    TcpClient(const char *server, int port)
        : m_server{server}, m_port{port}, m_socket{nullptr}, m_address{nullptr}
    {}

    ~TcpClient()
    {
        if (m_socket)
            delete m_socket;

        if (m_address)
            delete m_address;
    }

    void connect(error_code &ec);
    void send(const void * data, size_t size, error_code &ec);
    void receive(void * data, size_t &size, error_code &ec);
    void disconnect(error_code &ec);

private:
    string          m_server;
    int             m_port;
    Socket          *m_socket;
    SocketAddress   *m_address;
};


void TcpClient::connect(error_code &ec)
{
    set_level(level::debug);
    ec = make_error_code(CoapStatus::COAP_ERR_RESOLVE_ADDRESS);

    UnixDnsResolver dns(m_server.c_str(), m_port);

    if (!dns.hostname2address())
    {
        debug("hostname2address() failed");
        return;
    }

    int domain = AF_INET;
    struct in_addr * inp;

    if(dns.address6().size())
    {
        struct sockaddr_in6 sa;
        sa.sin6_family = domain = AF_INET6;
        sa.sin6_port = htons(dns.port());
        inp =  (in_addr *)(&sa.sin6_addr.s6_addr);
        inet_aton (dns.address6().c_str(), inp);
        m_address = new UnixSocketAddress(sa);
    }
    else if(dns.address4().size())
    {
        struct sockaddr_in sa;
        sa.sin_family = domain = AF_INET;
        sa.sin_port = htons(dns.port());
        inp =  (in_addr *)(&sa.sin_addr.s_addr);
        inet_aton (dns.address4().c_str(), inp);
        m_address = new UnixSocketAddress(sa);
    }
    else
    {
        return;
    }

    ec.clear();

    if (m_socket)
    {
        delete m_socket;
        m_socket = nullptr;
    }

    m_socket = new UnixSocket(domain, SOCK_STREAM, 0, ec);

    if(ec.value())
    {
        debug("UnixSocket() failed");
        return;
    }

    m_socket->connect((const SocketAddress *) m_address, ec);
}

void TcpClient::send(const void * data, size_t size, error_code &ec)
{
    m_socket->sendto(data, size, (const SocketAddress *) m_address, ec);
}

void TcpClient::receive(void * data, size_t &size, error_code &ec)
{
    ssize_t r = m_socket->recvfrom(data, size, m_address, ec);
    if (!ec.value())
        size = static_cast<size_t>(r);
}

void TcpClient::disconnect(error_code &ec)
{
    m_socket->close(ec);
}

int main(int argc, char **argv)
{
    set_level(level::debug);
    debug("tcp-client");

    TcpClient client("cxemotexnika.org", 80);

    error_code ec;

    client.connect(ec);
    debug("connect() : {}", ec.message());
    if (ec.value())
        return 1;

    const char * payload = "GET /wp-content/uploads/2020/01/response.txt HTTP/1.1\r\n\r\n";

    client.send(payload, strlen(payload), ec);
    debug("send() : {}", ec.message());
    if (ec.value())
        return 1;

    char response[4096];
    size_t sz = sizeof(response);

    client.receive(response, sz, ec);
    debug("receive() : {}", ec.message());
    if (ec.value())
        return 1;

    debug("There was received {0:d} bytes :",sz);
    debug("{}", response);
    
    client.disconnect(ec);
    debug("disconnect() : {}", ec.message());

    if (ec.value())
        return 1;

    return 0;
}
