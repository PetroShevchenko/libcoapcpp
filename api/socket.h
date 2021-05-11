#ifndef _SOCKET_H
#define _SOCKET_H
#include "error.h"
#include <cstddef>

enum SocketType
{
    SOCKET_TYPE_IP_V4,
    SOCKET_TYPE_IP_V6,
    SOCKET_TYPE_UNSPEC
};

inline bool is_socket_type(SocketType type)
{ return !(type < SOCKET_TYPE_IP_V4 || type > SOCKET_TYPE_UNSPEC); }

struct SocketAddress
{
    SocketAddress(SocketType type)
    : m_type{type}
    {}

    SocketAddress()
    : m_type{SOCKET_TYPE_UNSPEC}
    {}

    virtual ~SocketAddress() = default;

    SocketType type() const
    { return m_type; }

    void type(SocketType value)
    { m_type = value; }

protected:
    SocketType m_type;
};

struct Socket
{
    virtual ~Socket() = default;
    virtual void close(std::error_code &ec) = 0;
    virtual void connect(const SocketAddress * addr, std::error_code &ec) = 0;
    virtual ssize_t sendto(const void * buf, std::size_t len, const SocketAddress * addr, std::error_code &ec) = 0;
    virtual ssize_t recvfrom(std::error_code &ec, void * buf, std::size_t len, SocketAddress * addr = nullptr) = 0;
    virtual void bind(const SocketAddress * addr, std::error_code &ec) = 0;
    virtual Socket * accept(std::error_code * ec = nullptr) = 0;
    virtual void listen(std::error_code &ec, int max_connections_in_queue = 1) = 0;
    virtual void set_blocking(bool blocking, std::error_code &ec) = 0;
    virtual void set_timeout(size_t timeout, std::error_code &ec) = 0;
    virtual void setsockoption(int level, int option_name, const void *option_value, std::size_t option_len, std::error_code &ec) = 0;
    virtual void getsockoption(int level, int option_name, void *option_value, std::size_t *option_len, std::error_code &ec) = 0;
};

#endif
