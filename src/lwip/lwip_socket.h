#ifndef _LWIP_SOCKET_H
#define _LWIP_SOCKET_H
#include "api/socket.h"
#include "error.h"
#include "lwipopts.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include <memory>
#include <typeinfo>

#undef close
#undef connect
#undef accept 
#undef sendto
#undef recvfrom 
#undef bind
#undef listen
#undef setsockoption
#undef getsockoption

class LwipSocketAddress : public SocketAddress
{
public:
    LwipSocketAddress()
        : SocketAddress(SOCKET_TYPE_UNSPEC),
          m_address4{0},
          m_address6{0}
    {}

    LwipSocketAddress(
            const struct sockaddr_in & addr4,
            const struct sockaddr_in6 & addr6
        )
        : SocketAddress(SOCKET_TYPE_UNSPEC),
          m_address4{addr4},
          m_address6{addr6}
    {}

    LwipSocketAddress(
            const struct sockaddr_in & addr
        )
        : SocketAddress(SOCKET_TYPE_IP_V4),
          m_address4{addr},
          m_address6{0}
    {}

    LwipSocketAddress(
            const struct sockaddr_in6 & addr
        )
        : SocketAddress(SOCKET_TYPE_IP_V6),
          m_address4{0},
          m_address6{addr}
    {}

    ~LwipSocketAddress() override = default;

public:
    const sockaddr_in & address4() const
    { return m_address4; }

    const sockaddr_in6 & address6() const
    { return m_address6; }

    sockaddr_in & address4()
    { return m_address4; }

    sockaddr_in6 & address6()
    { return m_address6; }

    void address4(const void *value, size_t len, std::error_code &ec);
    void address6(const void *value, size_t len, std::error_code &ec);

private:
    struct sockaddr_in m_address4;
    struct sockaddr_in6 m_address6;
};

class LwipSocket : public Socket
{
public:
    LwipSocket()
    : m_descriptor{-1}, m_address{nullptr}
    {}

    LwipSocket(int domain, int type, int protocol, std::error_code &ec);

    ~LwipSocket() override;

public:
    void close(std::error_code &ec) override;
    void connect(const SocketAddress * addr, std::error_code &ec) override;
    ssize_t sendto(const void * buf, std::size_t len, const SocketAddress * addr, std::error_code &ec) override;
    ssize_t recvfrom(std::error_code &ec, void * buf, std::size_t len, SocketAddress * addr = nullptr) override;
    void bind(const SocketAddress * addr, std::error_code &ec) override;
    Socket * accept(std::error_code * ec = nullptr) override;
    void listen(std::error_code &ec, int max_connections_in_queue = 1) override;
    void set_blocking(bool blocking, std::error_code &ec) override;
    void set_timeout(size_t timeout, std::error_code &ec) override;
    void setsockoption(int level, int option_name, const void *option_value, std::size_t option_len, std::error_code &ec) override;
    void getsockoption(int level, int option_name, void *option_value, std::size_t *option_len, std::error_code &ec) override;

public:
    void descriptor(int value)
    { m_descriptor = value; }

    int descriptor() const
    { return m_descriptor; }

    std::shared_ptr<LwipSocketAddress> & address()
    { return m_address; }

private:
    int m_descriptor;
    std::shared_ptr<LwipSocketAddress> m_address;
};

#endif
