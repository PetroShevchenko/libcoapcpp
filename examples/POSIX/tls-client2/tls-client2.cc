#include <iostream>
#include <string>
#include <cstring>
#include "spdlog/spdlog.h"
#include <spdlog/fmt/fmt.h>
#include "socket.h"
#include "unix_socket.h"
#include "unix_dns_resolver.h"
#include "error.h"
#include "common/tcp_client.h"
#include "common/http_requests.h"
#include "common/my_certificates.h"

#include <cstring>
#include "mbedtls_error.h"

#if !defined(MBEDTLS_CONFIG_FILE)
#include "mbedtls/config.h"
#else
#include MBEDTLS_CONFIG_FILE
#endif

#if defined(MBEDTLS_PLATFORM_C)
#include "mbedtls/platform.h"
#else
#include <cstdio>
#include <cstdlib>
#define mbedtls_time            time
#define mbedtls_time_t          time_t
#define mbedtls_fprintf         fprintf
#define mbedtls_printf          printf
#define mbedtls_exit            exit
#define MBEDTLS_EXIT_SUCCESS    EXIT_SUCCESS
#define MBEDTLS_EXIT_FAILURE    EXIT_FAILURE
#endif /* MBEDTLS_PLATFORM_C */

#include "mbedtls/net_sockets.h"
#include "mbedtls/debug.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"

using namespace std;
using namespace spdlog;

class TlsClient : public TcpClient
{
public:
    TlsClient(const char *server, int port)
        : TcpClient(server, port),
          m_descriptor{new mbedtls_net_context},
          m_entropy{new mbedtls_entropy_context},
          m_ctrDrbg{new mbedtls_ctr_drbg_context},
          m_ssl{new mbedtls_ssl_context},
          m_conf{new mbedtls_ssl_config},
          m_cacert{new mbedtls_x509_crt}
    {}

    ~TlsClient()
    {
        error_code ec;
        disconnect(ec);

        if (m_descriptor)
            delete m_descriptor;

        if (m_entropy)
            delete m_entropy;

        if (m_ctrDrbg)
            delete m_ctrDrbg;

        if (m_ssl)
            delete m_ssl;

        if (m_conf)
            delete m_conf;

        if (m_cacert)
            delete m_cacert;
    }

public:
    void connect(error_code &ec) override;
    void send(const void * data, size_t size, error_code &ec) override;
    void receive(error_code &ec, void * data, size_t &size, size_t seconds = 0 ) override;
    void disconnect(error_code &ec) override;

private:
    void handshake(const char *address, const char * port, error_code &ec);

private:
    mbedtls_net_context         *m_descriptor;
    mbedtls_entropy_context     *m_entropy;
    mbedtls_ctr_drbg_context    *m_ctrDrbg;
    mbedtls_ssl_context         *m_ssl;
    mbedtls_ssl_config          *m_conf;
    mbedtls_x509_crt            *m_cacert;
};

void TlsClient::handshake(const char *address, const char *port, error_code &ec)
{
    const char *pers = "tls-client2";

    mbedtls_net_init( m_descriptor );
    mbedtls_ssl_init( m_ssl );
    mbedtls_ssl_config_init( m_conf );
    mbedtls_x509_crt_init( m_cacert );
    mbedtls_ctr_drbg_init( m_ctrDrbg );
    mbedtls_entropy_init( m_entropy );

    int status = mbedtls_ctr_drbg_seed(
                    m_ctrDrbg,
                    mbedtls_entropy_func,
                    m_entropy,
                    (const unsigned char *) pers,
                    strlen( pers )
                );
    if(status)
    {
        ec = make_error_code(status);
        debug("mbedtls_ctr_drbg_seed() failed {}", ec.message());
        return;
    }

    status = mbedtls_x509_crt_parse(
                    m_cacert,
                    get_root_cert(),
                    get_root_cert_size()
                );
    if(status)
    {
        ec = make_error_code(status);
        debug("mbedtls_x509_crt_parse() failed {}", ec.message());
        return;
    }

    status = mbedtls_net_connect(
                    m_descriptor,
                    address,
                    port,
                    MBEDTLS_NET_PROTO_TCP
                );

    if(status)
    {
        ec = make_error_code(status);
        debug("mbedtls_net_connect() failed {}", ec.message());
        return;
    }

    status = mbedtls_ssl_config_defaults(
                    m_conf,
                    MBEDTLS_SSL_IS_CLIENT,
                    MBEDTLS_SSL_TRANSPORT_STREAM,
                    MBEDTLS_SSL_PRESET_DEFAULT
                );

    if(status)
    {
        ec = make_error_code(status);
        debug("mbedtls_ssl_config_defaults() failed {}", ec.message());
        return;
    }

    mbedtls_ssl_conf_authmode( m_conf, MBEDTLS_SSL_VERIFY_OPTIONAL );
    mbedtls_ssl_conf_ca_chain( m_conf, m_cacert, NULL );
    mbedtls_ssl_conf_rng( m_conf, mbedtls_ctr_drbg_random, m_ctrDrbg );
    //mbedtls_ssl_conf_dbg( m_conf, my_debug, stdout );

    status = mbedtls_ssl_setup( m_ssl, m_conf );

    if(status)
    {
        ec = make_error_code(status);
        debug("mbedtls_ssl_setup() failed {}", ec.message());
        return;
    }

    status = mbedtls_ssl_set_hostname( m_ssl, address );

    if(status)
    {
        ec = make_error_code(status);
        debug("mbedtls_ssl_set_hostname() failed {}", ec.message());
        return;
    }

    mbedtls_ssl_set_bio( m_ssl, m_descriptor, mbedtls_net_send, mbedtls_net_recv, NULL );

    while( ( status = mbedtls_ssl_handshake( m_ssl ) ) != 0 )
    {
        if( status != MBEDTLS_ERR_SSL_WANT_READ && status != MBEDTLS_ERR_SSL_WANT_WRITE )
        {
            ec = make_error_code(status);
            debug("mbedtls_ssl_handshake() failed {}", ec.message());
            return;
        }
    }
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

    const char * addr;

    if (dns.address6().size())
    {
        addr = dns.address6().c_str();
    }
    else
    {
        addr = dns.address4().c_str();
    }

    char port[64];

    snprintf(port, sizeof(port), "%d", dns.port());

    handshake(addr, port, ec);
}

void TlsClient::send(const void * data, size_t size, error_code &ec)
{
    int status;
    while( ( status = mbedtls_ssl_write( m_ssl, (const unsigned char *)data, size ) ) <= 0 )
    {
        if( status != MBEDTLS_ERR_SSL_WANT_READ && status != MBEDTLS_ERR_SSL_WANT_WRITE )
        {
            ec = make_error_code(status);
            debug("mbedtls_ssl_write() failed: {}",ec.message());
            return;
        }
    }
}

void TlsClient::receive(error_code &ec, void * data, size_t &size, size_t seconds)
{
    (void)seconds;
    size_t len;
    int status;
    uint8_t * buffer = static_cast<uint8_t *>(data);
    size_t total = 0;
    debug("start recieving");
    do
    {
        len = size - 1;
        memset(buffer, 0, size);

        status = mbedtls_ssl_read( m_ssl, buffer, len );
        debug("status : {0:d}", status);

        if( status == MBEDTLS_ERR_SSL_WANT_READ || status == MBEDTLS_ERR_SSL_WANT_WRITE )
        {
            debug("read in progress, status is {0:d}", status);
            continue;
        }

        if( status == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY )
        {
            debug("peer closed");
            //ec = make_error_code(status);
            break;
        }

        if( status < 0 )
        {
            ec = make_error_code(status);
            debug("receive() failed : {}", ec.message());
            break;
        }

        if( status == 0 )
        {
            debug("EOF");
            break;
        }

        if (status > 0)
        {
            total += status;
            break;
        }
    }
    while( 1 );

    if (!ec.value())
    {
        size = total;
        debug("{0:d} bytes read", size);
    }
}

void TlsClient::disconnect(error_code &ec)
{
    ec.clear();
    mbedtls_ssl_close_notify( m_ssl );
    mbedtls_net_free( m_descriptor );
    mbedtls_x509_crt_free( m_cacert );
    mbedtls_ssl_free( m_ssl );
    mbedtls_ssl_config_free( m_conf );
    mbedtls_ctr_drbg_free( m_ctrDrbg );
    mbedtls_entropy_free( m_entropy );
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
