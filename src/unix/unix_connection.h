#ifndef _UNIX_CONNECTION_H
#define _UNIX_CONNECTION_H
#include "connection.h"
#include "unix_dns_resolver.h"
#include "unix_socket.h"
#include "utils.h"
#include "error.h"
#include <wolfssl/options.h>
#include <wolfssl/ssl.h>
#include <netdb.h>

class UnixUdpClient : public ClientConnection
{
public:
    UnixUdpClient(const char * hostname, int port, std::error_code &ec)
        : ClientConnection(UDP,hostname, port, ec),
          m_dns{new UnixDnsResolver(hostname, port)},
          m_socket{new UnixSocket()},
          m_sockAddr{nullptr}
    {}

    UnixUdpClient(const char * uri, std::error_code &ec)
        : ClientConnection(uri, ec),
          m_dns{new UnixDnsResolver(uri)},
          m_socket{new UnixSocket()},
          m_sockAddr{nullptr}
    {}

    ~UnixUdpClient()
    {
        std::error_code ec;
        disconnect(ec);
        if (m_dns)
        {
            delete m_dns;
            m_dns = nullptr;
        }
    }

public:
    void connect(std::error_code &ec) override;
    void disconnect(std::error_code &ec) override;
    void send(const void * buffer, size_t length, std::error_code &ec) override;
    void receive(void * buffer, size_t &length, std::error_code &ec, size_t seconds = 0) override;

private:
    DnsResolver   *m_dns;
    Socket        *m_socket;
    SocketAddress *m_sockAddr;
};

class UnixDtlsClient : public ClientConnection
{
public:
    UnixDtlsClient(const char * hostname, int port, std::error_code &ec)
        : ClientConnection(DTLS,hostname, port, ec),
          m_dns{new UnixDnsResolver(hostname, port)},
          m_socket{new UnixSocket()},
          m_sockAddr{nullptr},
          m_ctx{nullptr},
          m_ssl{nullptr}
    {}

    UnixDtlsClient(const char * uri, std::error_code &ec)
        : ClientConnection(uri, ec),
          m_dns{new UnixDnsResolver(uri)},
          m_socket{new UnixSocket()},
          m_sockAddr{nullptr},
          m_ctx{nullptr},
          m_ssl{nullptr}
    {}

    ~UnixDtlsClient()
    {
        std::error_code ec;
        disconnect(ec);
        if (m_dns)
        {
            delete m_dns;
            m_dns = nullptr;
        }
    }

public:
    void connect(std::error_code &ec) override;
    void disconnect(std::error_code &ec) override;
    void send(const void * buffer, size_t length, std::error_code &ec) override;
    void receive(void * buffer, size_t &length, std::error_code &ec, size_t seconds = 0) override;

private:
    DnsResolver   *m_dns;
    Socket        *m_socket;
    SocketAddress *m_sockAddr;
    WOLFSSL_CTX   *m_ctx;
    WOLFSSL       *m_ssl;
};

ClientConnection * create_client_connection(ConnectionType type, const char * hostname, int port, std::error_code &ec);
ClientConnection * create_client_connection(const char * uri, std::error_code &ec);

#endif
