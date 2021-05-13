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
    void receive(error_code &ec, void * data, size_t &size, size_t seconds = 0);
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

    UnixDnsResolver dns(m_server.c_str(), m_port);

    dns.hostname2address(ec);

    if (ec.value())
    {
        debug("hostname2address() failed: {}",ec.message());
        return;
    }

    m_address = dns.create_socket_address(ec);

    if (ec.value())
    {
        debug("create_socket_address() failed: {}",ec.message());
        return;
    }

    ec.clear();

    if (m_socket)
    {
        delete m_socket;
        m_socket = nullptr;
    }

    int domain = AF_INET;

    if (m_address->type() == SOCKET_TYPE_IP_V6)
    {
        domain = AF_INET6;
    }
    else
    {
        domain = AF_INET;
    }

    m_socket = new UnixSocket(domain, SOCK_STREAM, 0, ec);

    if(ec.value())
    {
        debug("UnixSocket() failed: {}",ec.message());
        return;
    }

    m_socket->connect((const SocketAddress *) m_address, ec);

    if(ec.value())
    {
        debug("connect() failed: {}",ec.message());
    }
}

void TcpClient::send(const void * data, size_t size, error_code &ec)
{
    m_socket->sendto(data, size, (const SocketAddress *) m_address, ec);
}

void TcpClient::receive(error_code &ec, void * data, size_t &size, size_t seconds)
{
    if (seconds)
    {
        m_socket->set_timeout(seconds, ec);
        if(ec.value())
            return;
    }

    ssize_t r = m_socket->recvfrom(ec, data, size);
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

    const char * payload = "GET /wp-content/uploads/2020/01/response.txt HTTP/1.1\r\n\
Host: cxemotexnika.org\r\n\
User-Agent: Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:88.0) Gecko/20100101 Firefox/88.0\r\n\
Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8\r\n\
Accept-Language: en-US,en;q=0.5\r\n\
Accept-Encoding: gzip, deflate\r\n\
Connection: keep-alive\r\n\
Upgrade-Insecure-Requests: 1\r\n\
Cache-Control: max-age=0\r\n\r\n";

    client.send(payload, strlen(payload), ec);
    debug("send() : {}", ec.message());
    if (ec.value())
        return 1;

    char response[4096];
    size_t sz = sizeof(response);

    client.receive(ec, response, sz, 1);
    debug("receive() : {}", ec.message());
    if (ec.value())
        return 1;

    debug("There were received {0:d} bytes :",sz);
    debug("{}", response);

    client.disconnect(ec);
    debug("disconnect() : {}", ec.message());

    if (ec.value())
        return 1;

    return 0;
}
