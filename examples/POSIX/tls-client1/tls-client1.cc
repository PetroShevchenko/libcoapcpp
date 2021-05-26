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
#include <wolfssl/options.h>
#include <wolfssl/ssl.h>
#include "common/wolfssl_error.h"

#include "common/tcp_client.h"
#include "common/http_requests.h"
#include "common/my_certificates.h"

#define CIPHER_LIST "ECDHE-ECDSA-AES256-GCM-SHA384:ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-RSA-AES128-GCM-SHA256"

using namespace std;
using namespace spdlog;

class TlsClient : public TcpClient
{
public:
    TlsClient(const char *server, int port)
        : TcpClient(server, port),
        m_socket{nullptr},
        m_address{nullptr},
        m_ctx{nullptr},
        m_ssl{nullptr}
    {}

    ~TlsClient()
    {
        error_code ec;
        disconnect(ec);

        if (m_socket)
            delete m_socket;

        if (m_address)
            delete m_address;
    }

public:
    void connect(error_code &ec) override;
    void send(const void * data, size_t size, error_code &ec) override;
    void receive(error_code &ec, void * data, size_t &size, size_t seconds = 0) override;
    void disconnect(error_code &ec) override;

private:
    void handshake(error_code &ec);

private:
    Socket          *m_socket;
    SocketAddress   *m_address;
    WOLFSSL_CTX     *m_ctx;
    WOLFSSL         *m_ssl;
};

void TlsClient::handshake(error_code &ec)
{
    /* Initialize wolfSSL */
    if (wolfSSL_Init() != WOLFSSL_SUCCESS)
    {
        ec = make_error_code(wolfSSL_get_error(m_ssl, 0));
        debug("wolfSSL_Init() failed: {}",ec.message());
        return;
    }

    wolfSSL_Debugging_ON();

    /* Create and initialize WOLFSSL_CTX */
    m_ctx = wolfSSL_CTX_new(wolfTLSv1_2_client_method());

    if (m_ctx == NULL)
    {
        ec = make_error_code(wolfSSL_get_error(m_ssl, 0));
        debug("wolfSSL_CTX_new() failed: {}",ec.message());
        return;
    }

    /* Set cipher list */
    if (wolfSSL_CTX_set_cipher_list(m_ctx, CIPHER_LIST) != SSL_SUCCESS)
    {
        ec = make_error_code(wolfSSL_get_error(m_ssl, 0));
        debug("wolfSSL_CTX_set_cipher_list() failed: {}",ec.message());
        return;
    }

    /* Load server ROOT certificate into WOLFSSL_CTX */
    if (wolfSSL_CTX_load_verify_buffer(
                    m_ctx,
                    get_root_cert(),
                    get_root_cert_size(),
                    SSL_FILETYPE_PEM
                ) != SSL_SUCCESS)
    {
        ec = make_error_code(wolfSSL_get_error(m_ssl, 0));
        debug("wolfSSL_CTX_load_verify_buffer() failed: {}",ec.message());
        return;
    }

    wolfSSL_CTX_set_verify(m_ctx, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, 0);

    /* Create a WOLFSSL object */
    m_ssl = wolfSSL_new(m_ctx);

    if (m_ssl == NULL)
    {
        ec = make_error_code(wolfSSL_get_error(m_ssl, 0));
        debug("wolfSSL_new() failed: {}",ec.message());
        return;
    }

    /* Attach wolfSSL to the socket */
    int fd = (dynamic_cast<UnixSocket *>(m_socket))->descriptor();
    if (wolfSSL_set_fd(m_ssl, fd) != WOLFSSL_SUCCESS)
    {
        ec = make_error_code(wolfSSL_get_error(m_ssl, 0));
        debug("wolfSSL_set_fd() failed: {}",ec.message());
        return;
    }

    int err;
    do
    {
        err = 0;
        /* Connect to wolfSSL on the server side */
        if (wolfSSL_connect(m_ssl) != SSL_SUCCESS)
        {
            err = wolfSSL_get_error(m_ssl, 0);
            if (err != WC_PENDING_E)
            {
                ec = make_error_code(err);
                debug("wolfSSL_connect() failed: {}",ec.message());
                return;
            }
            debug("wolfSSL_connect() failed: WC_PENDING_E");
        }
    } while(err == WC_PENDING_E);
}

void TlsClient::connect(error_code &ec)
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
        return;
    }

    handshake(ec);
}

void TlsClient::send(const void * data, size_t size, error_code &ec)
{
    if (wolfSSL_write(m_ssl, data, (int)size) != (int)size)
    {
        ec = make_error_code(CoapStatus::COAP_ERR_SEND);
        debug("wolfSSL_write() failed: {}",ec.message());
        return;
    }
}

void TlsClient::receive(error_code &ec, void * data, size_t &size, size_t seconds)
{
    (void)seconds;
    int status = wolfSSL_read(m_ssl, data, sizeof(data)-1);

    if (status == -1)
    {
        ec = make_error_code(CoapStatus::COAP_ERR_RECEIVE);
        debug("wolfSSL_read() failed: {}",ec.message());
        return;
    }
    size = static_cast<size_t>(status);
}

void TlsClient::disconnect(error_code &ec)
{
    if (m_ssl)
    {
        wolfSSL_free(m_ssl);      /* Free the wolfSSL object                  */
        m_ssl = nullptr;
    }
    if (m_ctx)
    {
        wolfSSL_CTX_free(m_ctx);  /* Free the wolfSSL context object          */
        m_ctx = nullptr;
    }
    wolfSSL_Cleanup();
    if (m_socket)
    {
        m_socket->close(ec);
    }
}

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    set_level(level::debug);
    debug("tls-client");

    TlsClient client("cxemotexnika.org", 443);

    error_code ec;

    http_get_request(&client, ec);

    debug("http_get_request() : {}",ec.message());

    return 0;
}
