#include "unix_connection.h"
#include "utils.h"
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

static Socket * create_socket(ConnectionType type, DnsResolver *dns, std::error_code &ec)
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

void UnixUdpClient::connect(std::error_code &ec)
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
    }
}

void UnixUdpClient::disconnect(std::error_code &ec)
{
    ec.clear();
    if (m_dns)
    {
        delete m_dns;
        m_dns = nullptr;
    }
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
}

void UnixUdpClient::send(const void * buffer, size_t length, std::error_code &ec)
{
    if (m_socket == nullptr)
    {
        ec = make_error_code(CoapStatus::COAP_ERR_NOT_CONNECTED);
        return;
    }
    ssize_t sent = m_socket->sendto(buffer, length, static_cast<const SocketAddress *>(m_sockAddr), ec);
    if (!ec.value())
    {
        if (static_cast<size_t>(sent) != length)
        {
            ec = make_error_code(CoapStatus::COAP_ERR_INCOMPLETE_SEND);
        }
    }
}

void UnixUdpClient::receive(void * buffer, size_t &length, std::error_code &ec, size_t seconds)
{
    if (m_socket == nullptr)
    {
        ec = make_error_code(CoapStatus::COAP_ERR_NOT_CONNECTED);
        return;
    }

    if (seconds)
    {
        m_socket->set_timeout(seconds, ec);
        if(ec.value())
            return;
    }

    ssize_t received = m_socket->recvfrom(ec, buffer, length);
    if (!ec.value())
    {
        length = static_cast<size_t>(received);
    }
}

void UnixDtlsClient::connect(std::error_code &ec)
{
    //TODO
}

void UnixDtlsClient::disconnect(std::error_code &ec)
{
    //TODO
}

void UnixDtlsClient::send(const void * buffer, size_t length, std::error_code &ec)
{
    //TODO
}

void UnixDtlsClient::receive(void * buffer, size_t &length, std::error_code &ec, size_t seconds)
{
    //TODO
}

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
            return new UnixUdpClient(hostname, port, ec);

        case DTLS:
            return new UnixDtlsClient(hostname, port, ec);

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
            return new UnixUdpClient(uri, ec);

        case DTLS:
            return new UnixDtlsClient(uri, ec);

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
