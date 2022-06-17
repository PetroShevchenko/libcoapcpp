#ifndef _UNIX_UDP_CLIENT_H
#define _UNIX_UDP_CLIENT_H
#include "connection.h"
#include "unix_dns_resolver.h"
#include "unix_socket.h"
#include "utils.h"
#include "error.h"

namespace Unix
{

class UdpClientConnection : public ClientConnection
{
public:
    UdpClientConnection(const char * hostname, int port, std::error_code &ec)
        : ClientConnection(UDP,hostname, port, ec),
          m_dns{new UnixDnsResolver(hostname, port)},
          m_socket{new UnixSocket()},
          m_address{nullptr}
    {}
    UdpClientConnection(
            const char * hostname,
            int port,
            std::shared_ptr<Buffer> bufferPtr,
            std::error_code &ec
        )
        : ClientConnection(UDP,hostname, port, std::move(bufferPtr), ec),
          m_dns{new UnixDnsResolver(hostname, port)},
          m_socket{new UnixSocket()},
          m_address{nullptr}
    {}
    UdpClientConnection(const char * uri, std::error_code &ec)
        : ClientConnection(uri, ec),
          m_dns{new UnixDnsResolver(uri)},
          m_socket{new UnixSocket()},
          m_address{nullptr}
    {}
    UdpClientConnection(
            const char * uri,
            std::shared_ptr<Buffer> bufferPtr,
            std::error_code &ec
        )
        : ClientConnection(uri, std::move(bufferPtr), ec),
          m_dns{new UnixDnsResolver(uri)},
          m_socket{new UnixSocket()},
          m_address{nullptr}
    {}
    ~UdpClientConnection()
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
    void send(const void * buffer, size_t length, const SocketAddress *destAddr, std::error_code &ec) override;
    void receive(void * buffer, size_t &length, SocketAddress * srcAddr, std::error_code &ec, size_t seconds = 0) override;
    void send(const void * buffer, size_t length, std::error_code &ec) override;
    void receive(void * buffer, size_t &length, std::error_code &ec, size_t seconds = 0) override;

public:
    const DnsResolver * dns() const
    { return static_cast<const DnsResolver *>(m_dns); }

    const Socket * socket() const
    { return static_cast<const Socket *>(m_socket); }

    const SocketAddress * address() const
    { return static_cast<const SocketAddress *>(m_address); }

private:
    DnsResolver   *m_dns;
    Socket        *m_socket;
    SocketAddress *m_address;
};

} //namespace unix

#endif
