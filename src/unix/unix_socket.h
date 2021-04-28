#ifndef _UNIX_SOCKET_H
#define _UNIX_SOCKET_H
#include "socket.h"
#include "error.h"
#include <netinet/in.h>

class UnixSocketAddress : public SocketAddress
{
public:
    UnixSocketAddress()
        : SocketAddress(SOCKET_TYPE_UNSPEC),
          m_address4{0},
          m_address6{0}
    {}

    UnixSocketAddress(
            const struct sockaddr_in & addr4,
            const struct sockaddr_in6 & addr6
        )
        : SocketAddress(SOCKET_TYPE_UNSPEC),
          m_address4{addr4},
          m_address6{addr6}
    {}

    UnixSocketAddress(
            const struct sockaddr_in & addr
        )
        : SocketAddress(SOCKET_TYPE_IP_V4),
          m_address4{addr}
    {}

    UnixSocketAddress(
            const struct sockaddr_in6 & addr
        )
        : SocketAddress(SOCKET_TYPE_IP_V6),
          m_address6{addr}
    {}

    ~UnixSocketAddress() = default;

public:
    const sockaddr_in & address4() const
    { return m_address4; }

    const sockaddr_in6 & address6() const
    { return m_address6; }

    void address4(const void *value, size_t len, std::error_code &ec);
    void address6(const void *value, size_t len, std::error_code &ec);

private:
    struct sockaddr_in m_address4;
    struct sockaddr_in6 m_address6;
};

class UnixSocket : public Socket
{
public:
    UnixSocket()
    {}

    UnixSocket(int domain, int type, int protocol, std::error_code &ec);

    ~UnixSocket();

public:
    void close(std::error_code &ec) override;
    void connect(const SocketAddress * addr, std::error_code &ec) override;
    ssize_t sendto(const void * buf, std::size_t len, const SocketAddress * addr, std::error_code &ec) override;
    ssize_t recvfrom(void * buf, std::size_t len, SocketAddress * addr, std::error_code &ec) override;
    void bind(const SocketAddress * addr, std::error_code &ec) override;
    Socket * accept(std::error_code * ec = nullptr) override;
    void listen(std::error_code &ec, int max_connections_in_queue = 1) override;
    void set_blocking(bool blocking) override;
    void set_timeout(size_t timeout) override;
    void setsockoption(int level, int option_name, const void *option_value, std::size_t option_len, std::error_code &ec) override;
    void getsockoption(int level, int option_name, void *option_value, std::size_t *option_len, std::error_code &ec) override;

private:
    int m_descriptor;
/*    int m_domain;
    int m_type;
    int m_protocol;*/
    SocketAddress * m_address;
};

#endif