#ifndef _DTLS_CLIENT_H
#define _DTLS_CLIENT_H
#include "connection.h"
#include "unix_dns_resolver.h"
#include "unix_socket.h"
#include "utils.h"
#include "error.h"
#include <wolfssl/options.h>
#include <wolfssl/ssl.h>
#include <netdb.h>

class DtlsClient : public ClientConnection
{
public:
    DtlsClient(const char * hostname, int port, std::error_code &ec)
        : ClientConnection(DTLS,hostname, port, ec),
          m_dns{new UnixDnsResolver(hostname, port)},
          m_socket{new UnixSocket()},
          m_sockAddr{nullptr},
          m_ctx{nullptr},
          m_ssl{nullptr}
    {}

    DtlsClient(const char * uri, std::error_code &ec)
        : ClientConnection(uri, ec),
          m_dns{new UnixDnsResolver(uri)},
          m_socket{new UnixSocket()},
          m_sockAddr{nullptr},
          m_ctx{nullptr},
          m_ssl{nullptr}
    {}

    ~DtlsClient()
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
    void handshake(std::error_code &ec);

private:
    DnsResolver   *m_dns;
    Socket        *m_socket;
    SocketAddress *m_sockAddr;
    WOLFSSL_CTX   *m_ctx;
    WOLFSSL       *m_ssl;
};


#endif
