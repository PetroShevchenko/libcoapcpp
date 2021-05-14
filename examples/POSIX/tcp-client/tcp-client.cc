#include <iostream>
#include <string>
#include "spdlog/spdlog.h"
#include <spdlog/fmt/fmt.h>
#include "socket.h"
#include "unix_socket.h"
#include "unix_dns_resolver.h"
#include "error.h"
#include "common/tcp_client.h"
#include "common/http_requests.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <cstring>

using namespace std;
using namespace spdlog;

class MyTcpClient : public TcpClient
{
public:
     MyTcpClient(const char *server, int port)
        : TcpClient(server, port),
        m_socket{nullptr},
        m_address{nullptr}
    {}

    ~MyTcpClient()
    {
        if (m_socket)
            delete m_socket;

        if (m_address)
            delete m_address;
    }

    void connect(error_code &ec) override;
    void send(const void * data, size_t size, error_code &ec) override;
    void receive(error_code &ec, void * data, size_t &size, size_t seconds = 0) override;
    void disconnect(error_code &ec);

private:
    Socket          *m_socket;
    SocketAddress   *m_address;
};


void MyTcpClient::connect(error_code &ec)
{
    set_level(level::debug);

    UnixDnsResolver dns(server().c_str(), port());

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

void MyTcpClient::send(const void * data, size_t size, error_code &ec)
{
    m_socket->sendto(data, size, (const SocketAddress *) m_address, ec);
}

void MyTcpClient::receive(error_code &ec, void * data, size_t &size, size_t seconds)
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

void MyTcpClient::disconnect(error_code &ec)
{
    m_socket->close(ec);
}

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    set_level(level::debug);
    debug("tcp-client");

    MyTcpClient client("cxemotexnika.org", 80);

    error_code ec;

    http_get_request(&client, ec);

    debug("http_get_request() : {}",ec.message());

    return 0;
}
