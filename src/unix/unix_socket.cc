#include "unix_socket.h"
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
#include <fcntl.h>
#include "spdlog/spdlog.h"
#include <spdlog/fmt/fmt.h>

// for OSX
#ifndef MSG_CONFIRM
#define MSG_CONFIRM 0
#endif

using namespace std;
using namespace spdlog;

void UnixSocketAddress::address4(const void *value, size_t len, error_code &ec)
{
    if (value == nullptr)
    {
        ec = make_system_error(EFAULT);
        return;
    }
    if (!len || len < sizeof(struct sockaddr_in))
    {
        ec = make_system_error(EINVAL);
        return;
    }
    memcpy(&m_address4, value, len);
}

void UnixSocketAddress::address6(const void *value, size_t len, error_code &ec)
{
    if (value == nullptr)
    {
        ec = make_system_error(EFAULT);
        return;
    }
    if (!len || len < sizeof(struct sockaddr_in6))
    {
        ec = make_system_error(EINVAL);
        return;
    }
    memcpy(&m_address6, value, len);
}

UnixSocket::UnixSocket(
                int domain,
                int type,
                int protocol,
                error_code &ec
            )
            : m_descriptor{-1},
              m_address{nullptr}
{
    m_descriptor = ::socket(domain, type, protocol);

    if (m_descriptor < 0)
    {
        ec = make_system_error(errno);
        return;
    }
}

UnixSocket::~UnixSocket()
{
    error_code ec;
    close(ec);
}

void UnixSocket::close(error_code &ec)
{
    if (m_descriptor >= 0)
    {
        if (::close(m_descriptor) < 0)
        {
            ec = make_system_error(errno);
        }
        m_descriptor = -1;
    }
}

static const
struct sockaddr * extract_sockaddr(
            const SocketAddress * addr,
            size_t &size
        )
{
    if (addr == nullptr)
        return nullptr;

    const struct sockaddr * sap;
    const UnixSocketAddress * _addr = static_cast<const UnixSocketAddress *>(addr);

    if (addr->type() == SOCKET_TYPE_IP_V4)
    {
        sap = reinterpret_cast<const struct sockaddr *>(&_addr->address4());
        size = sizeof(_addr->address4());
    }
    else
    {
        sap = reinterpret_cast<const struct sockaddr *>(&_addr->address6());
        size = sizeof(_addr->address6());
    }
    return sap;
}

void UnixSocket::connect(const SocketAddress * addr, error_code &ec)
{
    if (addr == nullptr)
    {
        ec = make_system_error(EFAULT);
        return;
    }
    if (!is_socket_type(addr->type()))
    {
        ec = make_system_error(EINVAL);
        return;
    }

    size_t sz;
    const struct sockaddr * sap = extract_sockaddr(addr, sz);

    if (::connect(m_descriptor, sap, sz) < 0)
    {
        ec = make_system_error(errno);
    }
}

ssize_t UnixSocket::sendto(
            const void * buf,
            size_t len,
            const SocketAddress * addr,
            error_code &ec
        )
{
    if (buf == nullptr || addr == nullptr)
    {
        ec = make_system_error(EFAULT);
        return -1;
    }
    if (!len || !is_socket_type(addr->type()))
    {
        ec = make_system_error(EINVAL);
        return -1;
    }

    ssize_t sent;
    size_t sz;
    const struct sockaddr * sap = extract_sockaddr(addr, sz);

    sent = ::sendto (m_descriptor, buf, len, MSG_CONFIRM, sap, sz);
    if (sent < 0)
    {
        ec = make_system_error(errno);
        return -1;
    }
    return sent;
}

ssize_t UnixSocket::recvfrom(
            void * buf,
            size_t len,
            SocketAddress * addr,
            error_code &ec
        )
{
    set_level(level::debug);
    if (buf == nullptr || addr == nullptr)
    {
        ec = make_system_error(EFAULT);
        return -1;
    }
    if (!len)
    {
        ec = make_system_error(EINVAL);
        return -1;
    }

    ssize_t received;
    socklen_t addrLen;
    struct sockaddr address;

    received = ::recvfrom (
                    m_descriptor,
                    (char *)(buf),
                    len,
                    MSG_WAITALL,
                    &address,
                    &addrLen
                );

    if (received < 0)
    {
        ec = make_system_error(errno);
        return -1;
    }

    return received;
}

void UnixSocket::bind(const SocketAddress * addr, error_code &ec)
{
    if (addr == nullptr)
    {
        ec = make_system_error(EFAULT);
        return;
    }
    if (!is_socket_type(addr->type()))
    {
        ec = make_system_error(EINVAL);
        return;
    }

    size_t sz;
    const struct sockaddr * sap = extract_sockaddr(addr, sz);

    if (::bind(m_descriptor, sap, sz) < 0)
    {
        ec = make_system_error(errno);
    }
}

// WARNING: Inside this function, memory is allocated, which must be freed manually
Socket * UnixSocket::accept(error_code * ec)
{
    socklen_t addrLen;
    struct sockaddr address;

    int newSockDesc = ::accept(m_descriptor, &address, &addrLen);

    if (newSockDesc < 0)
    {
        if (ec)
            *ec = make_system_error(errno);
        return nullptr;
    }

    if (address.sa_family != AF_INET
         && address.sa_family != AF_INET6)
    {
        if (ec)
            *ec = make_error_code(CoapStatus::COAP_ERR_SOCKET_DOMAIN);
        return nullptr;
    }

    UnixSocket * newSocket = new UnixSocket();

    if (newSocket == nullptr)
    {
        if (ec)
            *ec = make_error_code(CoapStatus::COAP_ERR_MEMORY_ALLOCATE);
        return nullptr;
    }

    newSocket->descriptor(newSockDesc);
    newSocket->address() = make_shared<UnixSocketAddress>();

    error_code _ec;
    if (address.sa_family == AF_INET)
    {
        newSocket->address()->type(SOCKET_TYPE_IP_V4);
        newSocket->address()->address4(&address, (size_t)addrLen, _ec);
    }
    else
    {
        newSocket->address()->type(SOCKET_TYPE_IP_V6);
        newSocket->address()->address6(&address, (size_t)addrLen, _ec);
    }
    if (_ec.value())
    {
        delete newSocket;
        if (ec)
            *ec = _ec;
        return nullptr;
    }

    return newSocket;
}

void UnixSocket::listen(error_code &ec, int max_connections_in_queue)
{
    if (::listen(m_descriptor, max_connections_in_queue) < 0)
    {
        ec = make_system_error(errno);
    }
}

void UnixSocket::set_blocking(bool blocking, error_code &ec)
{
    int status = fcntl(m_descriptor, F_GETFL, NULL);
    if (status < 0)
    {
        ec = make_system_error(errno);
        return;
    }
    if (blocking)
    {
        status &= ~O_NONBLOCK;
    }
    else
    {
        status |= O_NONBLOCK;
    }

    if (fcntl(m_descriptor, F_GETFL, status) < 0)
    {
        ec = make_system_error(errno);
    }
}

void UnixSocket::set_timeout(size_t seconds, error_code &ec)
{
    struct timeval timeout;
    timeout.tv_sec = seconds;
    timeout.tv_usec = 0;

    if (::setsockopt(
                m_descriptor,
                SOL_SOCKET,
                SO_RCVTIMEO,
                (const void *)&timeout,
                sizeof(timeout)
            ) < 0)
    {
        ec = make_system_error(errno);
    }
}

void UnixSocket::setsockoption(
                int level,
                int option_name,
                const void *option_value,
                size_t option_len,
                error_code &ec
            )
{
    if (::setsockopt(
            m_descriptor,
            level,
            option_name,
            option_value,
            option_len) < 0)
    {
        ec = make_system_error(errno);
    }
}

void UnixSocket::getsockoption(
                int level,
                int option_name,
                void *option_value,
                size_t *option_len,
                error_code &ec
            )
{
    if (::getsockopt(
            m_descriptor,
            level,
            option_name,
            option_value,
            (socklen_t *)option_len) < 0)
    {
        ec = make_system_error(errno);
    }
}
