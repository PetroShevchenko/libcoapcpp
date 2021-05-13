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
#include "wolfssl_error.h"

#define CIPHER_LIST "ECDHE-ECDSA-AES256-GCM-SHA384:ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-RSA-AES128-GCM-SHA256"


const unsigned char rootCrt[] ="\r\n\
-----BEGIN CERTIFICATE-----\r\n\
MIIDSjCCAjKgAwIBAgIQRK+wgNajJ7qJMDmGLvhAazANBgkqhkiG9w0BAQUFADA/\r\n\
MSQwIgYDVQQKExtEaWdpdGFsIFNpZ25hdHVyZSBUcnVzdCBDby4xFzAVBgNVBAMT\r\n\
DkRTVCBSb290IENBIFgzMB4XDTAwMDkzMDIxMTIxOVoXDTIxMDkzMDE0MDExNVow\n\
PzEkMCIGA1UEChMbRGlnaXRhbCBTaWduYXR1cmUgVHJ1c3QgQ28uMRcwFQYDVQQD\n\
Ew5EU1QgUm9vdCBDQSBYMzCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEB\n\
AN+v6ZdQCINXtMxiZfaQguzH0yxrMMpb7NnDfcdAwRgUi+DoM3ZJKuM/IUmTrE4O\n\
rz5Iy2Xu/NMhD2XSKtkyj4zl93ewEnu1lcCJo6m67XMuegwGMoOifooUMM0RoOEq\n\
OLl5CjH9UL2AZd+3UWODyOKIYepLYYHsUmu5ouJLGiifSKOeDNoJjj4XLh7dIN9b\n\
xiqKqy69cK3FCxolkHRyxXtqqzTWMIn/5WgTe1QLyNau7Fqckh49ZLOMxt+/yUFw\n\
7BZy1SbsOFU5Q9D8/RhcQPGX69Wam40dutolucbY38EVAjqr2m7xPi71XAicPNaD\n\
aeQQmxkqtilX4+U9m5/wAl0CAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNV\n\
HQ8BAf8EBAMCAQYwHQYDVR0OBBYEFMSnsaR7LHH62+FLkHX/xBVghYkQMA0GCSqG\n\
SIb3DQEBBQUAA4IBAQCjGiybFwBcqR7uKGY3Or+Dxz9LwwmglSBd49lZRNI+DT69\n\
ikugdB/OEIKcdBodfpga3csTS7MgROSR6cz8faXbauX+5v3gTt23ADq1cEmv8uXr\n\
AvHRAosZy5Q6XkjEGB5YGV8eAlrwDPGxrancWYaLbumR9YbK+rlmM6pZW87ipxZz\n\
R8srzJmwN0jP41ZL9c8PDHIyh8bwRLtTcm1D9SZImlJnt1ir/md2cXjbDaJWFBM5\n\
JDGFoqgCWjBH4d1QB7wCCZAA62RjYJsWvIjJEubSfZGL+T0yjWW06XyxV3bqxbYo\n\
Ob8VZRzI9neWagqNdwvYkQsEjgfbKbYK7p2CNTUQ\n\
-----END CERTIFICATE-----\n\
";

const size_t rootCrtSize = sizeof(rootCrt);

using namespace std;
using namespace spdlog;

class TlsClient
{
public:
    TlsClient(const char *server, int port)
        : m_server{server}, m_port{port}, m_socket{nullptr}, m_address{nullptr}, m_ctx{nullptr}, m_ssl{nullptr}
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
    void connect(error_code &ec);
    void send(const void * data, size_t size, error_code &ec);
    void receive(error_code &ec, void * data, size_t &size);
    void disconnect(error_code &ec);

private:
    void handshake(error_code &ec);

private:
    string          m_server;
    int             m_port;
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
    if (wolfSSL_CTX_load_verify_buffer(m_ctx, rootCrt, rootCrtSize, SSL_FILETYPE_PEM) != SSL_SUCCESS)
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

void TlsClient::receive(error_code &ec, void * data, size_t &size)
{
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

    client.receive(ec, response, sz);
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
