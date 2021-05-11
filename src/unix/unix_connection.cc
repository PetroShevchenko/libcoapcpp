#include "unix_connection.h"
#include "unix_dns_resolver.h"
#include "unix_socket.h"
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

UnixConnection::UnixConnection(
            ConnectionType type,
            const char * hostname,
            int port,
            std::error_code &ec
        )
    : m_type{type},
      m_dns{new UnixDnsResolver(hostname, port)},
      m_socket{new UnixSocket()},
      m_sockAddr{nullptr}
{
    if (!is_connection_type(m_type))
    {
        ec = make_system_error(EINVAL);
        return;
    }
}

UnixConnection::UnixConnection(
            const char * uri,
            std::error_code &ec
        )
    : m_type{ConnectionType::UDP},
      m_dns{new UnixDnsResolver(uri)},
      m_socket{new UnixSocket()},
      m_sockAddr{nullptr}
{
    if (!uri2connection_type(uri, m_type))
    {
        ec = make_system_error(EINVAL);
        return;
    }
}

UnixConnection::~UnixConnection()
{
    if (m_socket)
        delete m_socket;
    if (m_dns)
        delete m_dns;
    if (m_sockAddr)
        delete m_sockAddr;
}

Socket * UnixConnection::create_socket(std::error_code &ec)
{
    int domain, socktype;

    if (m_dns == nullptr)
    {
        ec = make_system_error(EFAULT);
        return nullptr;
    }

    if (m_dns->address6().size())
        domain = AF_INET6;
    else
        domain = AF_INET;

    if (m_type == TCP || m_type == TLS)
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

void UnixUdpClientConnection::connect(std::error_code &ec)
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

    m_socket = create_socket(ec);

    if (ec.value())
    {
        delete m_sockAddr;
        m_sockAddr = nullptr;
    }
}

void UnixUdpClientConnection::disconnect(std::error_code &ec)
{
    ec.clear();
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

void UnixUdpClientConnection::send(const void * buffer, size_t length, std::error_code &ec)
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

void UnixUdpClientConnection::receive(void * buffer, size_t &length, std::error_code &ec, size_t seconds)
{
    if (m_socket == nullptr)
    {
        ec = make_error_code(CoapStatus::COAP_ERR_NOT_CONNECTED);
        return;
    }
    if (seconds)
    {
        m_socket->set_blocking(false, ec);
        if(ec.value())
            return;

        m_socket->set_timeout(seconds, ec);
        if(ec.value())
            return;
    }
    else
    {
        m_socket->set_blocking(true, ec);
        if(ec.value())
            return;
    }

    ssize_t received = m_socket->recvfrom(buffer, length, m_sockAddr, ec);
    if (!ec.value())
    {
        length = static_cast<size_t>(received);
    }
}

void UnixDtlsClientConnection::connect(std::error_code &ec)
{
    //TODO
}

void UnixDtlsClientConnection::disconnect(std::error_code &ec)
{
    //TODO
}

void UnixDtlsClientConnection::send(const void * buffer, size_t length, std::error_code &ec)
{
    //TODO
}

void UnixDtlsClientConnection::receive(void * buffer, size_t &length, std::error_code &ec, size_t seconds)
{
    //TODO
}

Connection * create_client_connection(
            ConnectionType type,
            const char * hostname,
            int port,
            std::error_code &ec
        )
{
    switch(type)
    {
        case UDP:
            return new UnixUdpClientConnection(hostname, port, ec);

        case DTLS:
            return new UnixDtlsClientConnection(hostname, port, ec);

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

Connection * create_client_connection(
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
            return new UnixUdpClientConnection(uri, ec);

        case DTLS:
            return new UnixDtlsClientConnection(uri, ec);

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
