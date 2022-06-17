#include "unix_dtls_client.h"
#include "utils.h"
#include "wolfssl_error.h"
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/time.h>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <spdlog/spdlog.h>

using namespace spdlog;

namespace Unix
{

void DtlsClientConnection::handshake(std::error_code &ec)
{
    set_level(level::debug);
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

    /* Load certificate */
    if (wolfSSL_CTX_load_verify_locations(
                    m_ctx,
                    "../../third-party/wolfssl/certs/ca-cert.pem",
                    0
                ) != SSL_SUCCESS)
    {
        ec = make_error_code(wolfSSL_get_error(m_ssl, 0));
        debug("wolfSSL_CTX_load_verify_locations() failed: {}",ec.message());
        return;
    }

    /* Create a WOLFSSL object */
    m_ssl = wolfSSL_new(m_ctx);

    if (m_ssl == NULL)
    {
        ec = make_error_code(wolfSSL_get_error(m_ssl, 0));
        debug("wolfSSL_new() failed: {}",ec.message());
        return;
    }

    UnixSocketAddress * sockAddr = reinterpret_cast<UnixSocketAddress *>(m_sockAddr);

    if (m_sockAddr->type() == SOCKET_TYPE_IP_V4)
    {
        wolfSSL_dtls_set_peer(m_ssl, (void *)&sockAddr->address4(), sizeof(sockAddr->address4()));
    }
    else
    {
        wolfSSL_dtls_set_peer(m_ssl, (void *)&sockAddr->address6(), sizeof(sockAddr->address6()));
    }

    /* Attach wolfSSL to the socket */
    int fd = (dynamic_cast<UnixSocket *>(m_socket))->descriptor();
    if (wolfSSL_set_fd(m_ssl, fd) != WOLFSSL_SUCCESS)
    {
        ec = make_error_code(wolfSSL_get_error(m_ssl, 0));
        debug("wolfSSL_set_fd() failed: {}",ec.message());
    }
}

void DtlsClientConnection::connect(std::error_code &ec)
{
    m_dns->hostname2address(ec);
    if (ec.value())
    {
        return;
    }

    m_sockAddr = m_dns->create_socket_address(ec);
    if (ec.value())
    {
        return;
    }

    if (m_socket)
    {
        delete m_socket;
        m_socket = nullptr;
    }

    m_socket = create_socket(type(), m_dns, ec);
    if (ec.value())
    {
        delete m_sockAddr;
        m_sockAddr = nullptr;
        return;
    }

    handshake(ec);
}

void DtlsClientConnection::close(std::error_code &ec)
{
    ec.clear();

    wolfSSL_shutdown(m_ssl);
    wolfSSL_free(m_ssl);

    if (m_socket)
    {
        delete m_socket;
        m_socket = nullptr;
    }

    if (m_sockAddr)
    {
        delete m_sockAddr;
        m_sockAddr = nullptr;
    }

    wolfSSL_CTX_free(m_ctx);
    wolfSSL_Cleanup();
}

void DtlsClientConnection::send(const void * buffer, size_t length, std::error_code &ec)
{
    ssize_t sent = wolfSSL_write(m_ssl, buffer, length);

    if (sent != (ssize_t)length)
    {
        ec = make_error_code(CoapStatus::COAP_ERR_SEND);
    }
}

void DtlsClientConnection::receive(void * buffer, size_t &length, std::error_code &ec, size_t seconds)
{
    (void)seconds;
    ssize_t received = wolfSSL_read(m_ssl, buffer, length);

    if (received < 0)
    {
        ec = make_error_code(CoapStatus::COAP_ERR_RECEIVE);
        return;
    }

    length = (size_t)received;
}

void DtlsClientConnection::send(const void * buffer, size_t length, const SocketAddress *destAddr, std::error_code &ec)
{
    (void)destAddr;
    send(buffer, length, ec);
}

void DtlsClientConnection::receive(void * buffer, size_t &length, SocketAddress * srcAddr, std::error_code &ec, size_t seconds)
{
    (void)srcAddr;
    receive(buffer, length, ec, seconds);
}

}// namespace unix
