#include <arpa/inet.h>
#include "connection.h"
#include "utils.h"
#include "unix_socket.h"

#ifdef USE_CREATE_CLIENT_CONNECTION
#include "unix_udp_client.h"
#include "unix_dtls_client.h"
#endif

#ifdef USE_CREATE_SERVER_CONNECTION
#include "unix_udp_server.h"
//#include "unix_dtls_server.h"
#endif

using namespace Unix;

Socket * create_socket(ConnectionType type, DnsResolver *dns, std::error_code &ec)
{
    int domain, socktype;

    if (dns == nullptr)
    {
        ec = make_system_error(EFAULT);
        return nullptr;
    }

    if (!is_connection_type(type))
    {
        ec = make_system_error(EINVAL);
        return nullptr;
    }

    if (dns->address6().size())
        domain = AF_INET6;
    else
        domain = AF_INET;

    if (type == TCP || type == TLS)
        socktype = SOCK_STREAM;
    else
        socktype = SOCK_DGRAM;

    Socket * sock = new UnixSocket(domain, socktype, 0, ec);
    if (sock == nullptr)
    {
        ec = make_error_code(CoapStatus::COAP_ERR_MEMORY_ALLOCATE);
    }

    return sock;
}

#ifdef USE_CREATE_CLIENT_CONNECTION
ClientConnection * create_client_connection(
            ConnectionType type,
            const char * hostname,
            int port,
            std::error_code &ec
        )
{
    switch(type)
    {
        case UDP:
            return new UdpClient(hostname, port, ec);

        case DTLS:
            return new DtlsClient(hostname, port, ec);

        case TCP:
        case TLS:
            ec = make_error_code(CoapStatus::COAP_ERR_NOT_IMPLEMENTED);
            break;

        default:
            ec = make_system_error(EINVAL);
            break;
    }
    return nullptr;
}

ClientConnection * create_client_connection(
            const char * uri,
            std::error_code &ec
        )
{
    ConnectionType type;

    if (!uri2connection_type(uri, type))
    {
        ec = make_system_error(EINVAL);
        return nullptr;
    }

    switch(type)
    {
        case UDP:
            return new UdpClient(uri, ec);

        case DTLS:
            return new DtlsClient(uri, ec);

        case TCP:
        case TLS:
            ec = make_error_code(CoapStatus::COAP_ERR_NOT_IMPLEMENTED);
            break;

        default:
            ec = make_system_error(EINVAL);
            break;
    }
    return nullptr;
}
#endif// USE_CREATE_CLIENT_CONNECTION

#ifdef USE_CREATE_SERVER_CONNECTION
ServerConnection * create_server_connection(
            ConnectionType type,
            int port,
            bool version4,
            std::error_code &ec
        )
{
    switch(type)
    {
        case UDP:
            return new UdpServer(port, version4, ec);

        case DTLS:
        case TCP:
        case TLS:
            ec = make_error_code(CoapStatus::COAP_ERR_NOT_IMPLEMENTED);
            break;

        default:
            ec = make_system_error(EINVAL);
            break;
    }
    return nullptr;
}

#endif// USE_CREATE_SERVER_CONNECTION
