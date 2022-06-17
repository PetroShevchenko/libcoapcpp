#ifndef _UNIX_DTLS_CLIENT_H
#define _UNIX_DTLS_CLIENT_H
#include "connection.h"
#include "unix_dns_resolver.h"
#include "unix_socket.h"
#include "utils.h"
#include "error.h"
#include <wolfssl/options.h>
#include <wolfssl/ssl.h>
#include <netdb.h>

namespace Unix
{

class DtlsClientConnection : public ClientConnection
{
public:
    DtlsClientConnection(const char * hostname, int port, std::error_code &ec)
        : ClientConnection(DTLS,hostname, port, ec),
          m_dns{new UnixDnsResolver(hostname, port)},
          m_socket{new UnixSocket()},
          m_sockAddr{nullptr},
          m_ctx{nullptr},
          m_ssl{nullptr}
    {}

    DtlsClientConnection(const char * uri, std::error_code &ec)
        : ClientConnection(uri, ec),
          m_dns{new UnixDnsResolver(uri)},
          m_socket{new UnixSocket()},
          m_sockAddr{nullptr},
          m_ctx{nullptr},
          m_ssl{nullptr}
    {}

    ~DtlsClientConnection()
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

private:
    void handshake(std::error_code &ec);

private:
    DnsResolver   *m_dns;
    Socket        *m_socket;
    SocketAddress *m_sockAddr;
    WOLFSSL_CTX   *m_ctx;
    WOLFSSL       *m_ssl;
};

}// namespace unix

#endif
