#ifndef _UNIX_UDP_CLIENT_H
#define _UNIX_UDP_CLIENT_H
#include "connection.h"
#include "unix_dns_resolver.h"
#include "unix_socket.h"
#include "utils.h"
#include "error.h"

namespace Unix
{

class UdpClient : public ClientConnection
{
public:
    UdpClient(const char * hostname, int port, std::error_code &ec)
        : ClientConnection(UDP,hostname, port, ec),
          m_dns{new UnixDnsResolver(hostname, port)},
          m_socket{new UnixSocket()},
          m_sockAddr{nullptr}
    {}

    UdpClient(const char * uri, std::error_code &ec)
        : ClientConnection(uri, ec),
          m_dns{new UnixDnsResolver(uri)},
          m_socket{new UnixSocket()},
          m_sockAddr{nullptr}
    {}

    ~UdpClient()
    {
        std::error_code ec;
        close(ec);
        if (m_dns)
        {
            delete m_dns;
            m_dns = nullptr;
        }
    }

public:
    void connect(std::error_code &ec) override;
    void close(std::error_code &ec) override;
    void send(const void * buffer, size_t length, std::error_code &ec) override;
    void receive(void * buffer, size_t &length, std::error_code &ec, size_t seconds = 0) override;

private:
    DnsResolver   *m_dns;
    Socket        *m_socket;
    SocketAddress *m_sockAddr;
};

} //namespace unix

#endif
