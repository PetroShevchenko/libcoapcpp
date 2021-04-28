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

// for OSX
#ifndef MSG_CONFIRM
#define MSG_CONFIRM 0
#endif

using namespace std;

void UnixSocketAddress::address4(const void *value, size_t len, std::error_code &ec)
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

void UnixSocketAddress::address6(const void *value, size_t len, std::error_code &ec)
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

UnixSocket::UnixSocket(int domain, int type, int protocol, error_code &ec)
    : m_descriptor{-1}, m_address{nullptr}
{
    m_descriptor = socket(domain, type, protocol);

    if (m_descriptor < 0)
    {
        ec = make_system_error(errno);
        return;
    }

/*    m_domain = domain;
    m_type = type;
    m_protocol = protocol;*/
}

UnixSocket::~UnixSocket()
{
    error_code ec;
    close(ec);
}

void UnixSocket::close(std::error_code &ec)
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

void UnixSocket::connect(const SocketAddress * addr, std::error_code &ec)
{
    //TODO
}

ssize_t UnixSocket::sendto(const void * buf, std::size_t len, const SocketAddress * addr, std::error_code &ec)
{
    if (buf == nullptr || addr == nullptr)
    {
        ec = make_system_error(EFAULT);
        return -1;
    }
    if (!len || addr->type() == SOCKET_TYPE_UNSPEC)
    {
        ec = make_system_error(EINVAL);
        return -1;
    }

    ssize_t sent;
    const struct sockaddr * sap;
    size_t sz;
    const UnixSocketAddress * _addr = static_cast<const UnixSocketAddress *>(addr);

    if (addr->type() == SOCKET_TYPE_IP_V4)
    {
        sap = (const struct sockaddr *)&(_addr->address4());
        sz = sizeof(_addr->address4());
    }
    else
    {
        sap = (const struct sockaddr *)&(_addr->address6());
        sz = sizeof(_addr->address6());
    }

    sent = ::sendto (m_descriptor, buf, len, MSG_CONFIRM, sap, sz);
    if (sent < 0)
    {
        ec = make_system_error(errno);
        return -1;
    }
    return sent;
}

ssize_t UnixSocket::recvfrom(void * buf, std::size_t len, SocketAddress * addr, std::error_code &ec)
{
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

    received = ::recvfrom (m_descriptor, (char *)(buf), len, MSG_WAITALL, &address, &addrLen);

    if (received < 0)
    {
        ec = make_system_error(errno);
        return -1;
    }

    UnixSocketAddress * _addr = static_cast<UnixSocketAddress *>(addr);

    if (address.sa_family == AF_INET)
    {
        _addr->type(SOCKET_TYPE_IP_V4);
        _addr->address4(&address, (size_t)addrLen, ec);
        if (ec.value())
            return -1;
    }
    else if (address.sa_family == AF_INET6)
    {
        _addr->type(SOCKET_TYPE_IP_V6);
        _addr->address6(&address, (size_t)addrLen, ec);
        if (ec.value())
            return -1;
    }
    else
    {
        ec = make_error_code(CoapStatus::COAP_ERR_SOCKET_DOMAIN); // XXX : replace to socket error code
        return -1;
    }
    return received;
}

void UnixSocket::bind(const SocketAddress * addr, std::error_code &ec)
{
    //TODO
}

Socket * UnixSocket::accept(std::error_code * ec)
{
    return nullptr;
}

void UnixSocket::listen(std::error_code &ec, int max_connections_in_queue)
{
    //TODO
}

void UnixSocket::set_blocking(bool blocking)
{
    //TODO
}

void set_timeout(size_t timeout)
{
    //TODO
}

void setsockoption(int level, int option_name, const void *option_value, std::size_t option_len, std::error_code &ec)
{
    //TODO
}

void getsockoption(int level, int option_name, void *option_value, std::size_t *option_len, std::error_code &ec)
{
    //TODO
}
