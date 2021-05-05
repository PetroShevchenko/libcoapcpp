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

    return new UnixSocket(domain, socktype, 0, ec);
}

void UnixConnection::connect(std::error_code &ec)
{
    if (m_type != ConnectionType::UDP)
    {
        ec = make_error_code(CoapStatus::COAP_ERR_NOT_IMPLEMENTED);
        return;
    }

    if (!m_dns->hostname2address())
    {
        ec = make_error_code(CoapStatus::COAP_ERR_RESOLVE_ADDRESS);
        return;
    }

    struct in_addr * inp;

    if (m_dns->address6().size())
    {
        struct sockaddr_in6 sa;
        sa.sin6_family = AF_INET6;
        sa.sin6_port = htons(m_dns->port());
        inp =  (in_addr *)(&sa.sin6_addr.s6_addr);
        inet_aton (m_dns->address6().c_str(), inp);
        m_sockAddr = new UnixSocketAddress(sa);
    }
    else
    {
        struct sockaddr_in sa;
        sa.sin_family = AF_INET;
        sa.sin_port = htons(m_dns->port());
        inp =  (in_addr *)(&sa.sin_addr.s_addr);
        inet_aton (m_dns->address4().c_str(), inp);
        m_sockAddr = new UnixSocketAddress(sa);
    }

    if (m_sockAddr == nullptr)
    {
        ec = make_error_code(CoapStatus::COAP_ERR_MEMORY_ALLOCATE);
        return;
    }

    if (m_socket)
    {
        delete m_socket;
        m_socket = nullptr;
    }

    m_socket = create_socket(ec);

    if (m_socket == nullptr)
    {
        ec = make_error_code(CoapStatus::COAP_ERR_MEMORY_ALLOCATE);
        return;
    }
}

void UnixConnection::disconnect(std::error_code &ec)
{
    ec.clear();
    if (m_socket)
    {
        delete m_socket;
        m_socket = nullptr;
    }
}
#if 0
ssize_t UnixSocket::sendto(
            const void * buf,
            size_t len,
            const SocketAddress * addr,
            error_code &ec
        )
#endif

void UnixConnection::send(std::error_code &ec)
{
//TODO
}

void UnixConnection::receive(std::error_code &ec)
{
//TODO
}
